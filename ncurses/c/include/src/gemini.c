#include "ttychatter.h"

#include <curl/curl.h>
#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/*
 * libcurl response accumulator.  libcurl may call the write callback many
 * times with arbitrary chunk sizes, so the response path uses Buffer instead
 * of assuming a single read.  The response body is kept even for HTTP errors
 * because Gemini error JSON is useful to users and maintainers.
 */
static size_t curl_write_cb(char *ptr, size_t size, size_t nmemb, void *userdata) {
    Buffer *b = (Buffer *)userdata;
    size_t n = size * nmemb;
    buffer_append_n(b, ptr, n);
    return n;
}

static int http_request(const char *method,
                        const char *url,
                        const char *payload,
                        long timeout_seconds,
                        char **out_body,
                        long *out_status,
                        char **out_error) {
    if (out_body) *out_body = NULL;
    if (out_status) *out_status = 0;
    if (out_error) *out_error = NULL;

    CURL *curl = curl_easy_init();
    if (!curl) {
        if (out_error) *out_error = xstrdup("libcurl initialization failed");
        return -1;
    }

    Buffer body;
    buffer_init(&body);
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "User-Agent: ttychatter-c/0.1");

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout_seconds > 0 ? timeout_seconds : 120L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    if (!strcmp(method, "POST")) {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload ? payload : "");
    }

    CURLcode code = curl_easy_perform(curl);
    long status = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
    if (out_status) *out_status = status;

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (code != CURLE_OK) {
        if (out_error) *out_error = ttychatter_strdup_printf("network error: %s", curl_easy_strerror(code));
        buffer_free(&body);
        return -1;
    }

    if (out_body) *out_body = buffer_steal(&body);
    else buffer_free(&body);
    return 0;
}

void attachment_list_init(AttachmentList *list) {
    list->items = NULL;
    list->len = 0;
    list->cap = 0;
}

void attachment_list_free(AttachmentList *list) {
    if (!list) return;
    for (size_t i = 0; i < list->len; i++) {
        free(list->items[i].path);
        free(list->items[i].mime_type);
    }
    free(list->items);
    list->items = NULL;
    list->len = 0;
    list->cap = 0;
}

void attachment_list_add(AttachmentList *list, const char *path, const char *mime_type) {
    if (list->len == list->cap) {
        list->cap = list->cap ? list->cap * 2 : 8;
        list->items = xrealloc(list->items, list->cap * sizeof(list->items[0]));
    }
    list->items[list->len].path = xstrdup(path ? path : "");
    list->items[list->len].mime_type = xstrdup(mime_type ? mime_type : "application/octet-stream");
    list->len++;
}

static unsigned char *read_binary_file(const char *path, size_t *out_len) {
    if (out_len) *out_len = 0;
    FILE *fh = fopen(path, "rb");
    if (!fh) return NULL;
    if (fseek(fh, 0, SEEK_END) != 0) {
        fclose(fh);
        return NULL;
    }
    long size = ftell(fh);
    if (size < 0) {
        fclose(fh);
        return NULL;
    }
    rewind(fh);
    unsigned char *data = xmalloc((size_t)size ? (size_t)size : 1);
    if ((size_t)size && fread(data, 1, (size_t)size, fh) != (size_t)size) {
        free(data);
        fclose(fh);
        return NULL;
    }
    fclose(fh);
    if (out_len) *out_len = (size_t)size;
    return data;
}

static char *basename_dup(const char *path) {
    const char *slash = path ? strrchr(path, '/') : NULL;
    return xstrdup(slash ? slash + 1 : (path ? path : ""));
}

static json_object *build_gemini_payload(const Config *cfg, const char *prompt_text, const AttachmentList *attachments) {
    json_object *root = json_object_new_object();
    json_object *contents = json_object_new_array();
    json_object *content = json_object_new_object();
    json_object *parts = json_object_new_array();

    json_object *text_part = json_object_new_object();
    json_object_object_add(text_part, "text", json_object_new_string(prompt_text ? prompt_text : ""));
    json_object_array_add(parts, text_part);

    if (attachments) {
        for (size_t i = 0; i < attachments->len; i++) {
            const PendingAttachment *item = &attachments->items[i];
            struct stat st;
            char *base = basename_dup(item->path);
            if (stat(item->path, &st) != 0) {
                json_object *part = json_object_new_object();
                char *note = ttychatter_strdup_printf("\n[Attachment unavailable: %s]", item->path);
                json_object_object_add(part, "text", json_object_new_string(note));
                json_object_array_add(parts, part);
                free(note);
                free(base);
                continue;
            }
            if ((size_t)st.st_size > cfg->max_attachment_bytes) {
                json_object *part = json_object_new_object();
                char *note = ttychatter_strdup_printf("\n[Attachment skipped because it is larger than %zu bytes: %s]",
                                                     cfg->max_attachment_bytes,
                                                     base);
                json_object_object_add(part, "text", json_object_new_string(note));
                json_object_array_add(parts, part);
                free(note);
                free(base);
                continue;
            }
            if (text_like_mime(item->mime_type)) {
                char *content_text = read_text_file(item->path);
                json_object *part = json_object_new_object();
                char *note = ttychatter_strdup_printf("\n\n[Attached text file: %s]\n%s", base, content_text ? content_text : "[Could not read attachment]");
                json_object_object_add(part, "text", json_object_new_string(note));
                json_object_array_add(parts, part);
                free(content_text);
                free(note);
            } else {
                size_t len = 0;
                unsigned char *data = read_binary_file(item->path, &len);
                if (!data) {
                    json_object *part = json_object_new_object();
                    char *note = ttychatter_strdup_printf("\n[Could not read binary attachment: %s]", base);
                    json_object_object_add(part, "text", json_object_new_string(note));
                    json_object_array_add(parts, part);
                    free(note);
                } else {
                    char *b64 = base64_encode(data, len);
                    json_object *part = json_object_new_object();
                    json_object *inline_data = json_object_new_object();
                    json_object_object_add(inline_data, "mime_type", json_object_new_string(item->mime_type));
                    json_object_object_add(inline_data, "data", json_object_new_string(b64));
                    json_object_object_add(part, "inline_data", inline_data);
                    json_object_array_add(parts, part);
                    free(b64);
                    free(data);
                }
            }
            free(base);
        }
    }

    json_object_object_add(content, "parts", parts);
    json_object_array_add(contents, content);
    json_object_object_add(root, "contents", contents);
    return root;
}

static char *model_endpoint(const Config *cfg, const char *model_id) {
    char *norm = normalize_model_id(model_id);
    char *encoded_model = url_encode_component(norm);
    char *encoded_key = url_encode_component(cfg->api_key ? cfg->api_key : "");
    char *url = ttychatter_strdup_printf("https://generativelanguage.googleapis.com/v1beta/models/%s:generateContent?key=%s",
                                        encoded_model,
                                        encoded_key);
    free(norm);
    free(encoded_model);
    free(encoded_key);
    return url;
}

static int write_binary_output(const char *path, const unsigned char *data, size_t len) {
    char *copy = xstrdup(path);
    char *slash = strrchr(copy, '/');
    if (slash) {
        *slash = '\0';
        mkdir_p(copy);
    }
    free(copy);
    FILE *fh = fopen(path, "wb");
    if (!fh) return -1;
    if (len && fwrite(data, 1, len, fh) != len) {
        fclose(fh);
        return -1;
    }
    return fclose(fh) == 0 ? 0 : -1;
}

static void collect_inline_response_parts(json_object *root, const char *attachment_dir, const char *session_name, Buffer *notices) {
    json_object *candidates = NULL;
    if (!json_object_object_get_ex(root, "candidates", &candidates) || !json_object_is_type(candidates, json_type_array)) return;
    mkdir_p(attachment_dir);
    size_t c_len = json_object_array_length(candidates);
    for (size_t ci = 0; ci < c_len; ci++) {
        json_object *candidate = json_object_array_get_idx(candidates, ci);
        json_object *content = NULL, *parts = NULL;
        if (!json_object_object_get_ex(candidate, "content", &content)) continue;
        if (!json_object_object_get_ex(content, "parts", &parts) || !json_object_is_type(parts, json_type_array)) continue;
        size_t p_len = json_object_array_length(parts);
        for (size_t pi = 0; pi < p_len; pi++) {
            json_object *part = json_object_array_get_idx(parts, pi);
            json_object *inline_data = NULL;
            if (!json_object_object_get_ex(part, "inline_data", &inline_data)) {
                json_object_object_get_ex(part, "inlineData", &inline_data);
            }
            if (!inline_data || !json_object_is_type(inline_data, json_type_object)) continue;
            json_object *mime_obj = NULL, *data_obj = NULL;
            const char *mime = "application/octet-stream";
            if (json_object_object_get_ex(inline_data, "mime_type", &mime_obj) || json_object_object_get_ex(inline_data, "mimeType", &mime_obj)) {
                mime = json_object_get_string(mime_obj);
            }
            if (!json_object_object_get_ex(inline_data, "data", &data_obj)) continue;
            const char *data_text = json_object_get_string(data_obj);
            size_t bin_len = 0;
            unsigned char *bin = base64_decode(data_text, &bin_len);
            if (!bin) continue;
            const char *ext = mime_extension(mime);
            char *filename = ttychatter_strdup_printf("%s-response-%02zu-%02zu%s", session_name, ci + 1, pi + 1, ext);
            char *path = path_join2(attachment_dir, filename);
            if (write_binary_output(path, bin, bin_len) == 0) {
                if (notices->len) buffer_append(notices, "\n");
                buffer_appendf(notices, "[response attachment saved]\nfile: %s\nlocation: %s", filename, attachment_dir);
            } else {
                if (notices->len) buffer_append(notices, "\n");
                buffer_appendf(notices, "[response attachment save failed]\nfile: %s\nlocation: %s", filename, attachment_dir);
            }
            free(path);
            free(filename);
            free(bin);
        }
    }
}

static char *parse_generated_text_and_files(const char *body, const char *attachment_dir, const char *session_name, char **out_error) {
    if (out_error) *out_error = NULL;
    json_object *root = json_tokener_parse(body ? body : "");
    if (!root) {
        if (out_error) *out_error = ttychatter_strdup_printf("Gemini response parse error; raw response: %.1000s", body ? body : "");
        return NULL;
    }

    Buffer text;
    buffer_init(&text);
    json_object *candidates = NULL;
    if (json_object_object_get_ex(root, "candidates", &candidates) && json_object_is_type(candidates, json_type_array)) {
        size_t c_len = json_object_array_length(candidates);
        for (size_t ci = 0; ci < c_len; ci++) {
            json_object *candidate = json_object_array_get_idx(candidates, ci);
            json_object *content = NULL, *parts = NULL;
            if (!json_object_object_get_ex(candidate, "content", &content)) continue;
            if (!json_object_object_get_ex(content, "parts", &parts) || !json_object_is_type(parts, json_type_array)) continue;
            size_t p_len = json_object_array_length(parts);
            for (size_t pi = 0; pi < p_len; pi++) {
                json_object *part = json_object_array_get_idx(parts, pi);
                json_object *chunk = NULL;
                if (json_object_object_get_ex(part, "text", &chunk)) {
                    const char *s = json_object_get_string(chunk);
                    if (s && *s) {
                        if (text.len) buffer_append(&text, "\n");
                        buffer_append(&text, s);
                    }
                }
            }
        }
    }

    Buffer notices;
    buffer_init(&notices);
    collect_inline_response_parts(root, attachment_dir, session_name, &notices);
    if (notices.len) {
        if (text.len) buffer_append(&text, "\n\n");
        buffer_append(&text, notices.data);
    }
    buffer_free(&notices);
    json_object_put(root);

    if (text.len == 0) {
        buffer_free(&text);
        if (out_error) *out_error = xstrdup("Gemini returned an empty text response");
        return NULL;
    }
    return buffer_steal(&text);
}

int gemini_generate(const Config *cfg,
                    const char *model_id,
                    const char *prompt_text,
                    const char *session_name,
                    const char *attachment_dir,
                    const AttachmentList *attachments,
                    long timeout_seconds,
                    char **out_text,
                    char **out_error) {
    if (out_text) *out_text = NULL;
    if (out_error) *out_error = NULL;
    if (!cfg->api_key || !*cfg->api_key) {
        if (out_error) *out_error = xstrdup("No Gemini API key loaded. Press F3 to paste one, or set GEMINI_API_KEY.");
        return -1;
    }

    json_object *payload_obj = build_gemini_payload(cfg, prompt_text, attachments);
    const char *payload = json_object_to_json_string_ext(payload_obj, JSON_C_TO_STRING_PLAIN);
    char *url = model_endpoint(cfg, model_id && *model_id ? model_id : config_effective_model(cfg));
    char *body = NULL;
    long status = 0;
    char *net_error = NULL;
    int rc = http_request("POST", url, payload, timeout_seconds, &body, &status, &net_error);
    free(url);
    json_object_put(payload_obj);
    if (rc != 0) {
        if (out_error) *out_error = net_error ? net_error : xstrdup("Gemini network request failed");
        else free(net_error);
        return -1;
    }
    if (status < 200 || status >= 300) {
        if (out_error) *out_error = ttychatter_strdup_printf("Gemini HTTP error %ld: %.1000s", status, body ? body : "");
        free(body);
        return -1;
    }
    char *parse_error = NULL;
    char *text = parse_generated_text_and_files(body, attachment_dir, session_name, &parse_error);
    free(body);
    if (!text) {
        if (out_error) *out_error = parse_error ? parse_error : xstrdup("Gemini response parse error");
        return -1;
    }
    if (out_text) *out_text = text;
    else free(text);
    return 0;
}

static char *models_endpoint(const Config *cfg, const char *page_token) {
    char *encoded_key = url_encode_component(cfg->api_key ? cfg->api_key : "");
    char *url = NULL;
    if (page_token && *page_token) {
        char *encoded_page = url_encode_component(page_token);
        url = ttychatter_strdup_printf("https://generativelanguage.googleapis.com/v1beta/models?key=%s&pageToken=%s", encoded_key, encoded_page);
        free(encoded_page);
    } else {
        url = ttychatter_strdup_printf("https://generativelanguage.googleapis.com/v1beta/models?key=%s", encoded_key);
    }
    free(encoded_key);
    return url;
}

static char *json_string_optional(json_object *obj, const char *key, const char *fallback) {
    json_object *value = NULL;
    if (obj && json_object_object_get_ex(obj, key, &value) && json_object_is_type(value, json_type_string)) {
        return xstrdup(json_object_get_string(value));
    }
    return xstrdup(fallback ? fallback : "");
}

static long json_long_optional(json_object *obj, const char *key) {
    json_object *value = NULL;
    if (obj && json_object_object_get_ex(obj, key, &value)) {
        if (json_object_is_type(value, json_type_int)) return json_object_get_int64(value);
        if (json_object_is_type(value, json_type_string)) return parse_long_value(json_object_get_string(value), 0, 0);
    }
    return 0;
}

static void free_temp_model(ModelInfo *model) {
    free(model->model_id);
    free(model->api_name);
    free(model->display_name);
    free(model->description);
    for (size_t i = 0; i < model->methods_len; i++) free(model->methods[i]);
    free(model->methods);
    memset(model, 0, sizeof(*model));
}

static int model_list_contains_id(const ModelList *list, const char *model_id) {
    for (size_t i = 0; i < list->len; i++) {
        if (list->items[i].model_id && !strcmp(list->items[i].model_id, model_id)) return 1;
    }
    return 0;
}

static ModelInfo model_from_google_json(json_object *raw) {
    ModelInfo model;
    memset(&model, 0, sizeof(model));
    model.api_name = json_string_optional(raw, "name", "");
    model.model_id = normalize_model_id(model.api_name);
    model.display_name = json_string_optional(raw, "displayName", model.model_id);
    model.description = json_string_optional(raw, "description", "");
    model.input_token_limit = json_long_optional(raw, "inputTokenLimit");
    model.output_token_limit = json_long_optional(raw, "outputTokenLimit");
    json_object *methods = NULL;
    if (json_object_object_get_ex(raw, "supportedGenerationMethods", &methods) && json_object_is_type(methods, json_type_array)) {
        model.methods_len = json_object_array_length(methods);
        model.methods = xcalloc(model.methods_len ? model.methods_len : 1, sizeof(char *));
        for (size_t i = 0; i < model.methods_len; i++) {
            json_object *m = json_object_array_get_idx(methods, i);
            model.methods[i] = xstrdup(json_object_get_string(m));
        }
    }
    return model;
}

int gemini_fetch_models(const Config *cfg, ModelList *out_models, char **out_error) {
    if (out_error) *out_error = NULL;
    model_list_init(out_models);
    if (!cfg->api_key || !*cfg->api_key) {
        if (out_error) *out_error = xstrdup("No Gemini API key loaded. Press F3 to paste one, or set GEMINI_API_KEY.");
        return -1;
    }

    char *page_token = xstrdup("");
    for (;;) {
        char *url = models_endpoint(cfg, page_token);
        char *body = NULL;
        char *net_error = NULL;
        long status = 0;
        int rc = http_request("GET", url, NULL, 60, &body, &status, &net_error);
        free(url);
        if (rc != 0) {
            if (out_error) *out_error = net_error ? net_error : xstrdup("Gemini model-list network request failed");
            else free(net_error);
            free(page_token);
            model_list_free(out_models);
            return -1;
        }
        if (status < 200 || status >= 300) {
            if (out_error) *out_error = ttychatter_strdup_printf("Gemini model-list HTTP error %ld: %.1000s", status, body ? body : "");
            free(body);
            free(page_token);
            model_list_free(out_models);
            return -1;
        }

        json_object *root = json_tokener_parse(body ? body : "");
        if (!root) {
            if (out_error) *out_error = ttychatter_strdup_printf("Gemini model-list parse error; raw response: %.1000s", body ? body : "");
            free(body);
            free(page_token);
            model_list_free(out_models);
            return -1;
        }
        json_object *models = NULL;
        if (json_object_object_get_ex(root, "models", &models) && json_object_is_type(models, json_type_array)) {
            size_t len = json_object_array_length(models);
            for (size_t i = 0; i < len; i++) {
                json_object *raw = json_object_array_get_idx(models, i);
                if (!json_object_is_type(raw, json_type_object)) continue;
                ModelInfo model = model_from_google_json(raw);
                if (model.model_id && *model.model_id && model_supports_generate_content(&model) && !model_list_contains_id(out_models, model.model_id)) {
                    model_list_add(out_models, &model);
                }
                free_temp_model(&model);
            }
        }
        json_object *next = NULL;
        free(page_token);
        page_token = NULL;
        if (json_object_object_get_ex(root, "nextPageToken", &next) && json_object_is_type(next, json_type_string)) {
            page_token = xstrdup(json_object_get_string(next));
        } else {
            page_token = xstrdup("");
        }
        json_object_put(root);
        free(body);
        if (!*page_token) break;
    }
    free(page_token);
    if (out_models->len == 0) {
        if (out_error) *out_error = xstrdup("Gemini returned no generateContent models");
        model_list_free(out_models);
        return -1;
    }
    return 0;
}
