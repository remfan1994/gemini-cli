#include "ttychatter.h"

#include <ctype.h>
#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *json_string_dup(json_object *obj, const char *key, const char *fallback) {
    json_object *value = NULL;
    if (obj && json_object_object_get_ex(obj, key, &value) && json_object_is_type(value, json_type_string)) {
        return xstrdup(json_object_get_string(value));
    }
    return xstrdup(fallback ? fallback : "");
}

static long json_long_value(json_object *obj, const char *key) {
    json_object *value = NULL;
    if (obj && json_object_object_get_ex(obj, key, &value)) {
        if (json_object_is_type(value, json_type_int)) return json_object_get_int64(value);
        if (json_object_is_type(value, json_type_string)) return parse_long_value(json_object_get_string(value), 0, 0);
    }
    return 0;
}

static void model_info_free(ModelInfo *model) {
    if (!model) return;
    free(model->model_id);
    free(model->api_name);
    free(model->display_name);
    free(model->description);
    for (size_t i = 0; i < model->methods_len; i++) free(model->methods[i]);
    free(model->methods);
    memset(model, 0, sizeof(*model));
}

static ModelInfo model_info_copy(const ModelInfo *src) {
    ModelInfo out;
    memset(&out, 0, sizeof(out));
    out.model_id = xstrdup(src->model_id);
    out.api_name = xstrdup(src->api_name);
    out.display_name = xstrdup(src->display_name);
    out.description = xstrdup(src->description);
    out.input_token_limit = src->input_token_limit;
    out.output_token_limit = src->output_token_limit;
    out.methods_len = src->methods_len;
    out.methods = xcalloc(out.methods_len ? out.methods_len : 1, sizeof(char *));
    for (size_t i = 0; i < out.methods_len; i++) out.methods[i] = xstrdup(src->methods[i]);
    return out;
}

void model_list_init(ModelList *list) {
    list->items = NULL;
    list->len = 0;
    list->cap = 0;
    list->updated_at = NULL;
}

void model_list_free(ModelList *list) {
    if (!list) return;
    for (size_t i = 0; i < list->len; i++) model_info_free(&list->items[i]);
    free(list->items);
    free(list->updated_at);
    list->items = NULL;
    list->updated_at = NULL;
    list->len = 0;
    list->cap = 0;
}

void model_list_add(ModelList *list, const ModelInfo *model) {
    if (list->len == list->cap) {
        list->cap = list->cap ? list->cap * 2 : 16;
        list->items = xrealloc(list->items, list->cap * sizeof(list->items[0]));
    }
    list->items[list->len++] = model_info_copy(model);
}

static int model_has_method(const ModelInfo *model, const char *method) {
    for (size_t i = 0; i < model->methods_len; i++) {
        if (!strcmp(model->methods[i], method)) return 1;
    }
    return 0;
}

int model_supports_generate_content(const ModelInfo *model) {
    return model && model_has_method(model, "generateContent");
}

int model_has_required_token_limits(const ModelInfo *model) {
    return model && model->input_token_limit > 0 && model->output_token_limit > 0;
}

static char *lower_joined_model_text(const ModelInfo *model) {
    char *joined = ttychatter_strdup_printf("%s %s %s %s",
                                           model->model_id ? model->model_id : "",
                                           model->display_name ? model->display_name : "",
                                           model->api_name ? model->api_name : "",
                                           model->description ? model->description : "");
    for (char *p = joined; *p; p++) *p = (char)tolower((unsigned char)*p);
    return joined;
}

int model_looks_preview(const ModelInfo *model) {
    if (!model) return 0;
    char *text = lower_joined_model_text(model);
    const char *markers[] = {"preview", "experimental", " exp", "-exp", "_exp", " beta", "-beta", "_beta", NULL};
    int found = 0;
    for (size_t i = 0; markers[i]; i++) {
        if (strstr(text, markers[i])) {
            found = 1;
            break;
        }
    }
    free(text);
    return found;
}

int model_visible_for_config(const Config *cfg, const ModelInfo *model) {
    if (!model || !model->model_id || !*model->model_id) return 0;
    if (cfg->model_filter_generate_content && !model_supports_generate_content(model)) return 0;
    if (cfg->model_filter_require_tokens && !model_has_required_token_limits(model)) return 0;
    if (cfg->model_filter_hide_preview && model_looks_preview(model)) return 0;
    if (cfg->model_min_input_tokens > 0 && model->input_token_limit < cfg->model_min_input_tokens) return 0;
    if (cfg->model_min_output_tokens > 0 && model->output_token_limit < cfg->model_min_output_tokens) return 0;
    return 1;
}

ModelList model_filtered_copy(const Config *cfg, const ModelList *source) {
    ModelList out;
    model_list_init(&out);
    for (size_t i = 0; i < source->len; i++) {
        if (model_visible_for_config(cfg, &source->items[i])) model_list_add(&out, &source->items[i]);
    }
    model_list_sort_for_config(cfg, &out);
    if (source->updated_at) out.updated_at = xstrdup(source->updated_at);
    return out;
}

static const Config *sort_cfg = NULL;

static int cmp_model_id(const char *a, const char *b) {
    if (!a) a = "";
    if (!b) b = "";
    return strcmp(a, b);
}

static int cmp_models(const void *va, const void *vb) {
    const ModelInfo *a = (const ModelInfo *)va;
    const ModelInfo *b = (const ModelInfo *)vb;
    const char *mode = sort_cfg && sort_cfg->model_sort_order ? sort_cfg->model_sort_order : "api";
    if (!strcmp(mode, "model") || !strcmp(mode, "name")) {
        return cmp_model_id(a->model_id, b->model_id);
    }
    if (!strcmp(mode, "display")) {
        const char *ad = a->display_name && *a->display_name ? a->display_name : a->model_id;
        const char *bd = b->display_name && *b->display_name ? b->display_name : b->model_id;
        int c = cmp_model_id(ad, bd);
        return c ? c : cmp_model_id(a->model_id, b->model_id);
    }
    if (!strcmp(mode, "input-desc")) {
        if (a->input_token_limit != b->input_token_limit) return a->input_token_limit < b->input_token_limit ? 1 : -1;
        return cmp_model_id(a->model_id, b->model_id);
    }
    if (!strcmp(mode, "output-desc")) {
        if (a->output_token_limit != b->output_token_limit) return a->output_token_limit < b->output_token_limit ? 1 : -1;
        return cmp_model_id(a->model_id, b->model_id);
    }
    return 0;
}

void model_list_sort_for_config(const Config *cfg, ModelList *list) {
    if (!list || list->len < 2) return;
    sort_cfg = cfg;
    qsort(list->items, list->len, sizeof(list->items[0]), cmp_models);
    sort_cfg = NULL;
}

static json_object *model_to_json(const ModelInfo *model) {
    json_object *obj = json_object_new_object();
    json_object_object_add(obj, "model_id", json_object_new_string(model->model_id ? model->model_id : ""));
    json_object_object_add(obj, "api_name", json_object_new_string(model->api_name ? model->api_name : ""));
    json_object_object_add(obj, "display_name", json_object_new_string(model->display_name ? model->display_name : ""));
    json_object_object_add(obj, "description", json_object_new_string(model->description ? model->description : ""));
    json_object_object_add(obj, "input_token_limit", json_object_new_int64(model->input_token_limit));
    json_object_object_add(obj, "output_token_limit", json_object_new_int64(model->output_token_limit));
    json_object *methods = json_object_new_array();
    for (size_t i = 0; i < model->methods_len; i++) json_object_array_add(methods, json_object_new_string(model->methods[i]));
    json_object_object_add(obj, "methods", methods);
    return obj;
}

int save_model_cache(const Config *cfg, const ModelList *models) {
    json_object *root = json_object_new_object();
    char *when = now_datetime_string();
    json_object_object_add(root, "updated_at", json_object_new_string(when));
    free(when);
    json_object *arr = json_object_new_array();
    for (size_t i = 0; i < models->len; i++) json_object_array_add(arr, model_to_json(&models->items[i]));
    json_object_object_add(root, "models", arr);
    const char *json = json_object_to_json_string_ext(root, JSON_C_TO_STRING_PRETTY);
    int rc = write_text_file_private(cfg->model_cache_file, json);
    json_object_put(root);
    return rc;
}

static ModelInfo model_from_json(json_object *obj) {
    ModelInfo model;
    memset(&model, 0, sizeof(model));
    model.model_id = json_string_dup(obj, "model_id", "");
    model.api_name = json_string_dup(obj, "api_name", model.model_id);
    model.display_name = json_string_dup(obj, "display_name", model.model_id);
    model.description = json_string_dup(obj, "description", "");
    model.input_token_limit = json_long_value(obj, "input_token_limit");
    model.output_token_limit = json_long_value(obj, "output_token_limit");
    json_object *methods = NULL;
    if (json_object_object_get_ex(obj, "methods", &methods) && json_object_is_type(methods, json_type_array)) {
        model.methods_len = json_object_array_length(methods);
        model.methods = xcalloc(model.methods_len ? model.methods_len : 1, sizeof(char *));
        for (size_t i = 0; i < model.methods_len; i++) {
            json_object *m = json_object_array_get_idx(methods, i);
            model.methods[i] = xstrdup(json_object_get_string(m));
        }
    }
    return model;
}

int load_model_cache(const Config *cfg, ModelList *models, char **out_error) {
    if (out_error) *out_error = NULL;
    model_list_init(models);
    char *text = read_text_file(cfg->model_cache_file);
    if (!text) {
        if (out_error) *out_error = xstrdup("No cached model list found. Use --update-models first.");
        return -1;
    }
    json_object *root = json_tokener_parse(text);
    free(text);
    if (!root || !json_object_is_type(root, json_type_object)) {
        if (root) json_object_put(root);
        if (out_error) *out_error = xstrdup("Could not parse model cache JSON.");
        return -1;
    }
    json_object *updated = NULL;
    if (json_object_object_get_ex(root, "updated_at", &updated)) {
        models->updated_at = xstrdup(json_object_get_string(updated));
    }
    json_object *arr = NULL;
    if (json_object_object_get_ex(root, "models", &arr) && json_object_is_type(arr, json_type_array)) {
        size_t len = json_object_array_length(arr);
        for (size_t i = 0; i < len; i++) {
            json_object *item = json_object_array_get_idx(arr, i);
            if (!json_object_is_type(item, json_type_object)) continue;
            ModelInfo model = model_from_json(item);
            if (model.model_id && *model.model_id) model_list_add(models, &model);
            model_info_free(&model);
        }
    }
    json_object_put(root);
    if (models->len == 0) {
        if (out_error) *out_error = xstrdup("Cached model list is empty. Use --update-models first.");
        return -1;
    }
    return 0;
}

char *model_filter_summary(const Config *cfg, size_t before_count, size_t after_count) {
    Buffer b;
    buffer_init(&b);
    buffer_appendf(&b, "Showing %zu/%zu models | ", after_count, before_count);
    int first = 1;
#define ADD_FLAG(text) do { if (!first) buffer_append(&b, ", "); buffer_append(&b, (text)); first = 0; } while (0)
    if (cfg->model_filter_generate_content) ADD_FLAG("generateContent");
    if (cfg->model_filter_require_tokens) ADD_FLAG("token limits");
    if (cfg->model_filter_hide_preview) ADD_FLAG("hide preview/experimental");
    char *tmp = ttychatter_strdup_printf("min input:%ld", cfg->model_min_input_tokens);
    ADD_FLAG(tmp);
    free(tmp);
    tmp = ttychatter_strdup_printf("min output:%ld", cfg->model_min_output_tokens);
    ADD_FLAG(tmp);
    free(tmp);
    tmp = ttychatter_strdup_printf("sort:%s", cfg->model_sort_order ? cfg->model_sort_order : "api");
    ADD_FLAG(tmp);
    free(tmp);
#undef ADD_FLAG
    return buffer_steal(&b);
}

static const char *model_badges(const ModelInfo *model) {
    static char buf[64];
    buf[0] = '\0';
    if (model_supports_generate_content(model)) strcat(buf, "gen");
    if (model_has_required_token_limits(model)) strcat(buf, buf[0] ? "/tokens" : "tokens");
    if (model_looks_preview(model)) strcat(buf, buf[0] ? "/preview" : "preview");
    if (!buf[0]) strcpy(buf, "metadata");
    return buf;
}

void print_model_rows(const Config *cfg, const ModelList *models) {
    (void)cfg;
    for (size_t i = 0; i < models->len; i++) {
        const ModelInfo *m = &models->items[i];
        printf("%s\t%s\tinput:%ld\toutput:%ld\t%s\n",
               m->model_id ? m->model_id : "",
               (m->display_name && *m->display_name) ? m->display_name : (m->model_id ? m->model_id : ""),
               m->input_token_limit,
               m->output_token_limit,
               model_badges(m));
    }
}

int command_update_models(Config *cfg) {
    ModelList models;
    model_list_init(&models);
    char *error = NULL;
    if (gemini_fetch_models(cfg, &models, &error) != 0) {
        fprintf(stderr, "ERROR: %s\n", error ? error : "could not fetch models");
        free(error);
        model_list_free(&models);
        return 1;
    }
    save_model_cache(cfg, &models);
    ModelList visible = model_filtered_copy(cfg, &models);
    char *summary = model_filter_summary(cfg, models.len, visible.len);
    fprintf(stderr, "# updated cache: %s\n", cfg->model_cache_file);
    fprintf(stderr, "# %s\n", summary);
    print_model_rows(cfg, &visible);
    free(summary);
    model_list_free(&visible);
    model_list_free(&models);
    return 0;
}

int command_print_models(Config *cfg, int update_first) {
    ModelList models;
    model_list_init(&models);
    char *error = NULL;
    if (update_first || load_model_cache(cfg, &models, &error) != 0) {
        free(error);
        error = NULL;
        model_list_free(&models);
        model_list_init(&models);
        if (gemini_fetch_models(cfg, &models, &error) != 0) {
            fprintf(stderr, "ERROR: %s\n", error ? error : "could not fetch models");
            free(error);
            model_list_free(&models);
            return 1;
        }
        save_model_cache(cfg, &models);
    }
    ModelList visible = model_filtered_copy(cfg, &models);
    char *summary = model_filter_summary(cfg, models.len, visible.len);
    fprintf(stderr, "# %s\n", summary);
    print_model_rows(cfg, &visible);
    free(summary);
    model_list_free(&visible);
    model_list_free(&models);
    return 0;
}

int command_test_model(Config *cfg, const char *model_id) {
    AttachmentList attachments;
    attachment_list_init(&attachments);
    char *text = NULL;
    char *error = NULL;
    int rc = gemini_generate(cfg,
                             model_id,
                             cfg->model_test_prompt,
                             "model-test",
                             cfg->attachment_dir,
                             &attachments,
                             60,
                             &text,
                             &error);
    attachment_list_free(&attachments);
    if (rc != 0) {
        fprintf(stderr, "ERROR: %s\n", error ? error : "model test failed");
        free(error);
        return 1;
    }
    printf("%s\n", text ? text : "");
    free(text);
    return 0;
}
