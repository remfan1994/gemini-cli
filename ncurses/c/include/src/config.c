#include "ttychatter.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/*
 * XDG defaults.
 *
 * The Python reference used ~/.gpt for sessions because it grew from an older
 * script.  The native C port starts with the more standard split:
 *   config:   $XDG_CONFIG_HOME/ttychatter/gemini/config
 *   data:     $XDG_DATA_HOME/ttychatter/gemini/sessions
 *   files:    $XDG_DATA_HOME/ttychatter/gemini/attachments
 *   cache:    $XDG_CACHE_HOME/ttychatter/gemini/models.json
 *
 * Users can still override SESSION_DIR, ATTACHMENT_DIR, and MODEL_CACHE_FILE in
 * the simple KEY=VALUE config file.
 */
static char *xdg_path(const char *env_name, const char *fallback_leaf, const char *a, const char *b, const char *c) {
    const char *base = getenv(env_name);
    char *owned = NULL;
    if (!base || !*base) {
        const char *home = getenv("HOME");
        if (!home || !*home) home = ".";
        owned = path_join2(home, fallback_leaf);
        base = owned;
    }
    char *out = path_join4(base, a, b, c);
    free(owned);
    return out;
}

static void replace_string(char **slot, const char *value) {
    free(*slot);
    *slot = xstrdup(value ? value : "");
}

void config_init(Config *cfg) {
    memset(cfg, 0, sizeof(*cfg));
    cfg->model = xstrdup("");
    cfg->context_turns = TTYCHATTER_DEFAULT_CONTEXT_TURNS;
    cfg->api_key = xstrdup("");
    cfg->session_dir = xdg_path("XDG_DATA_HOME", ".local/share", "ttychatter", "gemini", "sessions");
    cfg->attachment_dir = xdg_path("XDG_DATA_HOME", ".local/share", "ttychatter", "gemini", "attachments");
    cfg->editor = xstrdup("");
    cfg->github_url = xstrdup(TTYCHATTER_DEFAULT_GITHUB_URL);
    cfg->model_test_prompt = xstrdup(TTYCHATTER_DEFAULT_MODEL_TEST_PROMPT);
    cfg->monochrome = 0;
    cfg->input_placeholder = xstrdup(TTYCHATTER_DEFAULT_INPUT_PLACEHOLDER);
    cfg->startup_notice = 1;
    cfg->config_file = xdg_path("XDG_CONFIG_HOME", ".config", "ttychatter", "gemini", "config");
    cfg->model_cache_file = xdg_path("XDG_CACHE_HOME", ".cache", "ttychatter", "gemini", "models.json");
    cfg->model_filter_generate_content = 1;
    cfg->model_filter_require_tokens = 1;
    cfg->model_filter_hide_preview = 0;
    cfg->model_min_input_tokens = 1;
    cfg->model_min_output_tokens = 1;
    cfg->model_sort_order = xstrdup("api");
    cfg->max_attachment_bytes = TTYCHATTER_MAX_INLINE_ATTACHMENT_BYTES;
}

void config_free(Config *cfg) {
    if (!cfg) return;
    free(cfg->model);
    free(cfg->api_key);
    free(cfg->session_dir);
    free(cfg->attachment_dir);
    free(cfg->editor);
    free(cfg->github_url);
    free(cfg->model_test_prompt);
    free(cfg->input_placeholder);
    free(cfg->config_file);
    free(cfg->model_cache_file);
    free(cfg->model_sort_order);
    memset(cfg, 0, sizeof(*cfg));
}

static void apply_config_key(Config *cfg, const char *key, const char *value, int *context_explicit) {
    if (!strcmp(key, "MODEL")) {
        char *norm = normalize_model_id(value);
        replace_string(&cfg->model, norm);
        free(norm);
    } else if (!strcmp(key, "CONTEXT_TURNS")) {
        cfg->context_turns = (int)parse_long_value(value, cfg->context_turns, 1);
        if (context_explicit) *context_explicit = 1;
    } else if (!strcmp(key, "HISTORY_LIMIT")) {
        if (!context_explicit || !*context_explicit) cfg->context_turns = (int)parse_long_value(value, cfg->context_turns, 1);
    } else if (!strcmp(key, "GEMINI_API_KEY")) {
        replace_string(&cfg->api_key, value);
    } else if (!strcmp(key, "SESSION_DIR")) {
        char *expanded = expand_path(value);
        replace_string(&cfg->session_dir, expanded);
        free(expanded);
    } else if (!strcmp(key, "ATTACHMENT_DIR")) {
        char *expanded = expand_path(value);
        replace_string(&cfg->attachment_dir, expanded);
        free(expanded);
    } else if (!strcmp(key, "EDITOR")) {
        replace_string(&cfg->editor, value);
    } else if (!strcmp(key, "GITHUB_URL")) {
        replace_string(&cfg->github_url, value);
    } else if (!strcmp(key, "MODEL_TEST_PROMPT")) {
        if (value && *value) replace_string(&cfg->model_test_prompt, value);
    } else if (!strcmp(key, "MONOCHROME")) {
        cfg->monochrome = parse_bool_value(value, cfg->monochrome);
    } else if (!strcmp(key, "INPUT_PLACEHOLDER")) {
        if (value && *value) replace_string(&cfg->input_placeholder, value);
    } else if (!strcmp(key, "STARTUP_NOTICE")) {
        cfg->startup_notice = parse_bool_value(value, cfg->startup_notice);
    } else if (!strcmp(key, "MODEL_CACHE_FILE")) {
        char *expanded = expand_path(value);
        replace_string(&cfg->model_cache_file, expanded);
        free(expanded);
    } else if (!strcmp(key, "MODEL_FILTER_GENERATE_CONTENT")) {
        cfg->model_filter_generate_content = parse_bool_value(value, cfg->model_filter_generate_content);
    } else if (!strcmp(key, "MODEL_FILTER_REQUIRE_TOKENS")) {
        cfg->model_filter_require_tokens = parse_bool_value(value, cfg->model_filter_require_tokens);
    } else if (!strcmp(key, "MODEL_FILTER_HIDE_PREVIEW")) {
        cfg->model_filter_hide_preview = parse_bool_value(value, cfg->model_filter_hide_preview);
    } else if (!strcmp(key, "MODEL_MIN_INPUT_TOKENS")) {
        cfg->model_min_input_tokens = parse_long_value(value, cfg->model_min_input_tokens, 0);
    } else if (!strcmp(key, "MODEL_MIN_OUTPUT_TOKENS")) {
        cfg->model_min_output_tokens = parse_long_value(value, cfg->model_min_output_tokens, 0);
    } else if (!strcmp(key, "MODEL_SORT_ORDER")) {
        if (value && *value) replace_string(&cfg->model_sort_order, value);
    } else if (!strcmp(key, "MAX_ATTACHMENT_BYTES")) {
        long parsed = parse_long_value(value, (long)cfg->max_attachment_bytes, 0);
        cfg->max_attachment_bytes = (size_t)parsed;
    }
}

void config_load(Config *cfg) {
    int context_explicit = 0;
    char *text = read_text_file(cfg->config_file);
    if (text) {
        char *save = NULL;
        for (char *line = strtok_r(text, "\n", &save); line; line = strtok_r(NULL, "\n", &save)) {
            char *trimmed = trim_in_place(line);
            if (!*trimmed || *trimmed == '#') continue;
            char *eq = strchr(trimmed, '=');
            if (!eq) continue;
            *eq = '\0';
            char *key = trim_in_place(trimmed);
            char *value = trim_in_place(eq + 1);
            apply_config_key(cfg, key, value, &context_explicit);
        }
        free(text);
    }

    /* Environment wins over saved config so users can avoid writing secrets. */
    const char *env_key = getenv("GEMINI_API_KEY");
    if (env_key && *env_key) replace_string(&cfg->api_key, env_key);
}

const char *config_effective_model(const Config *cfg) {
    return (cfg && cfg->model && *cfg->model) ? cfg->model : TTYCHATTER_FALLBACK_MODEL;
}

int config_set_value(const Config *cfg, const char *key, const char *value) {
    char *text = read_text_file(cfg->config_file);
    Buffer out;
    buffer_init(&out);
    int replaced = 0;

    if (text) {
        char *save = NULL;
        for (char *line = strtok_r(text, "\n", &save); line; line = strtok_r(NULL, "\n", &save)) {
            char *copy = xstrdup(line);
            char *trimmed = trim_in_place(copy);
            size_t key_len = strlen(key);
            if (!strncmp(trimmed, key, key_len) && trimmed[key_len] == '=') {
                buffer_appendf(&out, "%s=%s\n", key, value ? value : "");
                replaced = 1;
            } else {
                buffer_append(&out, line);
                buffer_append(&out, "\n");
            }
            free(copy);
        }
        free(text);
    }
    if (!replaced) buffer_appendf(&out, "%s=%s\n", key, value ? value : "");

    char *final_text = buffer_steal(&out);
    int rc = write_text_file_private(cfg->config_file, final_text);
    free(final_text);
    chmod(cfg->config_file, 0600);
    return rc;
}

char *config_summary(const Config *cfg) {
    Buffer b;
    buffer_init(&b);
    buffer_appendf(&b, "program=%s %s\n", TTYCHATTER_PROGRAM, TTYCHATTER_VERSION);
    buffer_appendf(&b, "config_file=%s\n", cfg->config_file);
    buffer_appendf(&b, "session_dir=%s\n", cfg->session_dir);
    buffer_appendf(&b, "attachment_dir=%s\n", cfg->attachment_dir);
    buffer_appendf(&b, "model_cache_file=%s\n", cfg->model_cache_file);
    buffer_appendf(&b, "model=%s\n", config_effective_model(cfg));
    buffer_appendf(&b, "api_key=%s\n", (cfg->api_key && *cfg->api_key) ? "loaded" : "missing");
    buffer_appendf(&b, "context_turns=%d\n", cfg->context_turns);
    buffer_appendf(&b, "editor=%s\n", cfg->editor && *cfg->editor ? cfg->editor : "VISUAL/EDITOR/vi");
    return buffer_steal(&b);
}
