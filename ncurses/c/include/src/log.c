#include "ttychatter.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define SNAPSHOT_PREFIX "CONTEXT_SNAPSHOT_JSON "

static void log_entry_list_add(LogEntryList *list, const char *timestamp, const char *speaker, const char *text) {
    if (list->len == list->cap) {
        list->cap = list->cap ? list->cap * 2 : 16;
        list->items = xrealloc(list->items, list->cap * sizeof(list->items[0]));
    }
    list->items[list->len].timestamp = xstrdup(timestamp ? timestamp : "");
    list->items[list->len].speaker = xstrdup(speaker ? speaker : "");
    list->items[list->len].text = xstrdup(text ? text : "");
    list->len++;
}

void log_entry_list_init(LogEntryList *list) {
    list->items = NULL;
    list->len = 0;
    list->cap = 0;
}

void log_entry_list_free(LogEntryList *list) {
    if (!list) return;
    for (size_t i = 0; i < list->len; i++) {
        free(list->items[i].timestamp);
        free(list->items[i].speaker);
        free(list->items[i].text);
    }
    free(list->items);
    list->items = NULL;
    list->len = 0;
    list->cap = 0;
}

static char *parent_from_path(const char *path) {
    char *copy = xstrdup(path);
    char *slash = strrchr(copy, '/');
    if (!slash) {
        free(copy);
        return xstrdup(".");
    }
    if (slash == copy) slash[1] = '\0';
    else *slash = '\0';
    return copy;
}

int append_log_entry(const char *path, const char *speaker, const char *text, const char *timestamp) {
    char *parent = parent_from_path(path);
    if (mkdir_p(parent) != 0) {
        free(parent);
        return -1;
    }
    free(parent);
    FILE *fh = fopen(path, "ab");
    if (!fh) return -1;
    fprintf(fh, "[%s] %s: %s\n", timestamp ? timestamp : "00:00:00", speaker ? speaker : "system", text ? text : "");
    int rc = fclose(fh);
    return rc == 0 ? 0 : -1;
}

int append_context_snapshot(const char *path, char **context, size_t context_len) {
    json_object *arr = json_object_new_array();
    for (size_t i = 0; i < context_len; i++) {
        json_object_array_add(arr, json_object_new_string(context[i] ? context[i] : ""));
    }
    const char *json = json_object_to_json_string_ext(arr, JSON_C_TO_STRING_PLAIN);
    char *line = ttychatter_strdup_printf("%s%s", SNAPSHOT_PREFIX, json);
    char *ts = now_time_string();
    int rc = append_log_entry(path, "system", line, ts);
    free(ts);
    free(line);
    json_object_put(arr);
    return rc;
}

static int looks_like_log_header(const char *line, char **timestamp, char **speaker, char **text) {
    if (timestamp) *timestamp = NULL;
    if (speaker) *speaker = NULL;
    if (text) *text = NULL;
    if (!line || strlen(line) < 12) return 0;
    if (line[0] != '[' || line[3] != ':' || line[6] != ':' || line[9] != ']' || line[10] != ' ') return 0;
    for (int i = 1; i <= 8; i++) {
        if (i == 3 || i == 6) continue;
        if (!isdigit((unsigned char)line[i])) return 0;
    }
    const char *after = line + 11;
    const char *colon = strstr(after, ": ");
    if (!colon) return 0;
    if (timestamp) *timestamp = xstrndup(line + 1, 8);
    if (speaker) *speaker = xstrndup(after, (size_t)(colon - after));
    if (text) *text = xstrdup(colon + 2);
    return 1;
}

int load_log_entries(const char *path, LogEntryList *out) {
    log_entry_list_init(out);
    char *content = read_text_file(path);
    if (!content) return 0;

    char *current_ts = NULL;
    char *current_speaker = NULL;
    Buffer current_text;
    buffer_init(&current_text);

    char *save = NULL;
    for (char *line = strtok_r(content, "\n", &save); line; line = strtok_r(NULL, "\n", &save)) {
        char *ts = NULL, *speaker = NULL, *text = NULL;
        if (looks_like_log_header(line, &ts, &speaker, &text)) {
            if (current_speaker) {
                log_entry_list_add(out, current_ts, current_speaker, current_text.data);
            }
            free(current_ts);
            free(current_speaker);
            buffer_free(&current_text);
            buffer_init(&current_text);
            current_ts = ts;
            current_speaker = speaker;
            buffer_append(&current_text, text);
            free(text);
        } else if (current_speaker) {
            buffer_append(&current_text, "\n");
            buffer_append(&current_text, line);
        }
    }
    if (current_speaker) {
        log_entry_list_add(out, current_ts, current_speaker, current_text.data);
    }

    free(current_ts);
    free(current_speaker);
    buffer_free(&current_text);
    free(content);
    return 0;
}

char **latest_context_snapshot(const LogEntryList *entries, size_t *out_len) {
    if (out_len) *out_len = 0;
    if (!entries) return NULL;
    for (size_t idx = entries->len; idx > 0; idx--) {
        const LogEntry *entry = &entries->items[idx - 1];
        if (strcmp(entry->speaker, "system") != 0) continue;
        if (strncmp(entry->text, SNAPSHOT_PREFIX, strlen(SNAPSHOT_PREFIX)) != 0) continue;
        const char *json = entry->text + strlen(SNAPSHOT_PREFIX);
        json_object *arr = json_tokener_parse(json);
        if (!arr || !json_object_is_type(arr, json_type_array)) {
            if (arr) json_object_put(arr);
            continue;
        }
        size_t len = json_object_array_length(arr);
        char **items = xcalloc(len ? len : 1, sizeof(char *));
        size_t used = 0;
        for (size_t i = 0; i < len; i++) {
            json_object *item = json_object_array_get_idx(arr, i);
            if (!json_object_is_type(item, json_type_string)) continue;
            items[used++] = xstrdup(json_object_get_string(item));
        }
        json_object_put(arr);
        if (used > 0) {
            if (out_len) *out_len = used;
            return items;
        }
        free(items);
    }
    return NULL;
}

void free_string_array(char **items, size_t len) {
    if (!items) return;
    for (size_t i = 0; i < len; i++) free(items[i]);
    free(items);
}

static char *safe_attachment_tag(const char *lang) {
    Buffer b;
    buffer_init(&b);
    const char *src = (lang && *lang) ? lang : "text";
    for (const unsigned char *p = (const unsigned char *)src; *p; p++) {
        unsigned char c = (unsigned char)tolower(*p);
        if (isalnum(c) || c == '.' || c == '_' || c == '-') {
            char one[2] = {(char)c, '\0'};
            buffer_append(&b, one);
        } else if (c == '+') {
            buffer_append(&b, "p");
        } else if (c == '#') {
            buffer_append(&b, "sharp");
        } else {
            buffer_append(&b, "_");
        }
    }
    if (b.len == 0) buffer_append(&b, "text");
    return buffer_steal(&b);
}

static const char *extension_for_lang(const char *lang) {
    if (!lang) return ".txt";
    if (!strcmp(lang, "bash") || !strcmp(lang, "sh") || !strcmp(lang, "shell")) return ".sh";
    if (!strcmp(lang, "python") || !strcmp(lang, "py")) return ".py";
    if (!strcmp(lang, "json")) return ".json";
    if (!strcmp(lang, "yaml") || !strcmp(lang, "yml")) return ".yml";
    if (!strcmp(lang, "javascript") || !strcmp(lang, "js")) return ".js";
    if (!strcmp(lang, "html")) return ".html";
    if (!strcmp(lang, "css")) return ".css";
    if (!strcmp(lang, "sql")) return ".sql";
    if (!strcmp(lang, "c")) return ".c";
    if (!strcmp(lang, "cpp") || !strcmp(lang, "c++")) return ".cpp";
    if (!strcmp(lang, "h") || !strcmp(lang, "header")) return ".h";
    return ".txt";
}

static int next_attachment_number(const char *attachment_dir, const char *session_name, const char *tag, const char *ext) {
    int highest = 0;
    DIR *dir = opendir(attachment_dir);
    if (!dir) return 1;
    char *prefix = ttychatter_strdup_printf("%s-%s-", session_name, tag);
    size_t prefix_len = strlen(prefix);
    size_t ext_len = strlen(ext);
    struct dirent *de;
    while ((de = readdir(dir))) {
        const char *name = de->d_name;
        size_t len = strlen(name);
        if (len <= prefix_len + ext_len) continue;
        if (strncmp(name, prefix, prefix_len) != 0) continue;
        if (strcmp(name + len - ext_len, ext) != 0) continue;
        int n = atoi(name + prefix_len);
        if (n > highest) highest = n;
    }
    closedir(dir);
    free(prefix);
    return highest + 1;
}

static int write_binary_file(const char *path, const char *data, size_t len) {
    char *parent = parent_from_path(path);
    int rc = mkdir_p(parent);
    free(parent);
    if (rc != 0) return -1;
    FILE *fh = fopen(path, "wb");
    if (!fh) return -1;
    if (len && fwrite(data, 1, len, fh) != len) {
        fclose(fh);
        return -1;
    }
    return fclose(fh) == 0 ? 0 : -1;
}

/*
 * Convert fenced code blocks into files.
 *
 * The transcript remains readable, while generated code becomes ordinary files
 * users can compile, edit, diff, or run.  This mirrors the Python reference but
 * avoids a regex dependency by using a small scanner for ```LANG ... ``` blocks.
 */
char *extract_code_attachments(const char *text, const char *attachment_dir, const char *session_name) {
    mkdir_p(attachment_dir);
    Buffer out;
    buffer_init(&out);
    const char *p = text ? text : "";

    while (*p) {
        const char *start = strstr(p, "```");
        if (!start) {
            buffer_append(&out, p);
            break;
        }
        buffer_append_n(&out, p, (size_t)(start - p));
        const char *lang_start = start + 3;
        const char *line_end = strchr(lang_start, '\n');
        if (!line_end) {
            buffer_append(&out, start);
            break;
        }
        char *raw_lang = xstrndup(lang_start, (size_t)(line_end - lang_start));
        char *lang_trim = trim_in_place(raw_lang);
        for (char *q = lang_trim; *q; q++) *q = (char)tolower((unsigned char)*q);
        const char *code_start = line_end + 1;
        const char *end = strstr(code_start, "```");
        if (!end) {
            buffer_append(&out, start);
            free(raw_lang);
            break;
        }
        char *tag = safe_attachment_tag(lang_trim);
        const char *ext = extension_for_lang(lang_trim);
        int number = next_attachment_number(attachment_dir, session_name, tag, ext);
        char *filename = ttychatter_strdup_printf("%s-%s-%02d%s", session_name, tag, number, ext);
        char *path = path_join2(attachment_dir, filename);
        if (write_binary_file(path, code_start, (size_t)(end - code_start)) == 0) {
            buffer_appendf(&out, "\n[attachment saved]\nfile: %s\nlocation: %s\n", filename, attachment_dir);
        } else {
            buffer_appendf(&out, "\n[attachment save failed]\nfile: %s\nlocation: %s\n", filename, attachment_dir);
        }
        free(path);
        free(filename);
        free(tag);
        free(raw_lang);
        p = end + 3;
    }
    return buffer_steal(&out);
}

static int cmp_string_ptr(const void *a, const void *b) {
    const char *const *sa = (const char *const *)a;
    const char *const *sb = (const char *const *)b;
    return strcmp(*sa, *sb);
}

char **list_session_names(const char *session_dir, size_t *out_len) {
    if (out_len) *out_len = 0;
    mkdir_p(session_dir);
    DIR *dir = opendir(session_dir);
    if (!dir) return NULL;
    char **items = NULL;
    size_t len = 0, cap = 0;
    struct dirent *de;
    while ((de = readdir(dir))) {
        const char *name = de->d_name;
        size_t n = strlen(name);
        if (n <= 4 || strcmp(name + n - 4, ".log") != 0) continue;
        if (len == cap) {
            cap = cap ? cap * 2 : 16;
            items = xrealloc(items, cap * sizeof(items[0]));
        }
        items[len++] = xstrndup(name, n - 4);
    }
    closedir(dir);
    qsort(items, len, sizeof(items[0]), cmp_string_ptr);
    if (out_len) *out_len = len;
    return items;
}
