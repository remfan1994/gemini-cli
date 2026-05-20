#ifndef TTYCHATTER_H
#define TTYCHATTER_H

/*
 * ttychatter.h
 *
 * Shared declarations for the native C Gemini terminal client.
 *
 * The project intentionally keeps a small dependency surface: libc, ncurses,
 * libcurl, and json-c.  This header is deliberately explicit rather than clever
 * because terminal AI clients tend to become long-lived tools.  Future
 * maintainers should be able to see the persistent data model, configuration
 * contract, API transport boundary, and ncurses boundary without chasing a
 * complex framework.
 */

#include <stddef.h>
#include <time.h>

#define TTYCHATTER_PROGRAM "ttychatter"
#define TTYCHATTER_VERSION "0.1.0"
#define TTYCHATTER_PROVIDER "gemini"
#define TTYCHATTER_FALLBACK_MODEL "gemini-2.5-flash"
#define TTYCHATTER_DEFAULT_CONTEXT_TURNS 12
#define TTYCHATTER_DEFAULT_MODEL_TEST_PROMPT \
    "Please reply with one short sentence confirming this model is available for text generation."
#define TTYCHATTER_DEFAULT_INPUT_PLACEHOLDER "Type your chat message here. Ctrl+G sends."
#define TTYCHATTER_DEFAULT_GITHUB_URL "https://github.com/remfan1994/ttychatter"
#define TTYCHATTER_PROJECT_NOTICE \
    "I strongly encourage everyone to get cruetly-free VEGETARIAN food and remember the 'bloodguilt' curse from the Bible. -Project Lead, remfan1994"
#define TTYCHATTER_MAX_INLINE_ATTACHMENT_BYTES (20UL * 1024UL * 1024UL)

/*
 * Simple growable string buffer.
 *
 * C code that builds JSON, messages, or screen text should not repeatedly
 * realloc by hand.  Buffer centralizes the rules: data is always NUL
 * terminated, len excludes the NUL, and failed allocation exits through the
 * project's checked allocation helpers.
 */
typedef struct Buffer {
    char *data;
    size_t len;
    size_t cap;
} Buffer;

/*
 * Configuration shared with the Python/Bash family.
 *
 * The C port moves default directories to XDG paths, but it keeps the same
 * simple KEY=VALUE config format and accepts legacy HISTORY_LIMIT as an alias
 * for CONTEXT_TURNS.  Secrets are stored only if the user explicitly saves an
 * API key, and the config file is written with mode 0600 when possible.
 */
typedef struct Config {
    char *model;
    int context_turns;
    char *api_key;
    char *session_dir;
    char *attachment_dir;
    char *editor;
    char *github_url;
    char *model_test_prompt;
    int monochrome;
    char *input_placeholder;
    int startup_notice;
    char *config_file;
    char *model_cache_file;
    int model_filter_generate_content;
    int model_filter_require_tokens;
    int model_filter_hide_preview;
    long model_min_input_tokens;
    long model_min_output_tokens;
    char *model_sort_order;
    size_t max_attachment_bytes;
} Config;

/*
 * Gemini model metadata.  Model selection is metadata-driven.  The program does
 * not blindly probe every model because that would surprise users, burn quota,
 * and make failures harder to understand.  Users can test one selected model
 * when they choose to.
 */
typedef struct ModelInfo {
    char *model_id;
    char *api_name;
    char *display_name;
    char *description;
    long input_token_limit;
    long output_token_limit;
    char **methods;
    size_t methods_len;
} ModelInfo;

typedef struct ModelList {
    ModelInfo *items;
    size_t len;
    size_t cap;
    char *updated_at;
} ModelList;

/*
 * Files staged by the user for the next prompt.  Text files are inlined as text;
 * binary files are base64 encoded as inline_data Gemini parts.
 */
typedef struct PendingAttachment {
    char *path;
    char *mime_type;
} PendingAttachment;

typedef struct AttachmentList {
    PendingAttachment *items;
    size_t len;
    size_t cap;
} AttachmentList;

/*
 * Human-readable session log entry.  The log format mirrors the Python version:
 * [HH:MM:SS] Speaker: first line
 * continuation lines are stored without another prefix.
 */
typedef struct LogEntry {
    char *timestamp;
    char *speaker;
    char *text;
} LogEntry;

typedef struct LogEntryList {
    LogEntry *items;
    size_t len;
    size_t cap;
} LogEntryList;

/* util.c */
void *xmalloc(size_t size);
void *xcalloc(size_t count, size_t size);
void *xrealloc(void *ptr, size_t size);
char *xstrdup(const char *s);
char *xstrndup(const char *s, size_t n);
void buffer_init(Buffer *b);
void buffer_free(Buffer *b);
void buffer_reserve(Buffer *b, size_t needed);
void buffer_append(Buffer *b, const char *s);
void buffer_append_n(Buffer *b, const char *s, size_t n);
void buffer_appendf(Buffer *b, const char *fmt, ...);
char *buffer_steal(Buffer *b);
char *trim_in_place(char *s);
int parse_bool_value(const char *s, int fallback);
long parse_long_value(const char *s, long fallback, long min_value);
char *path_join2(const char *a, const char *b);
char *path_join3(const char *a, const char *b, const char *c);
char *path_join4(const char *a, const char *b, const char *c, const char *d);
char *expand_path(const char *path);
int mkdir_p(const char *path);
int file_exists(const char *path);
int is_directory(const char *path);
char *read_text_file(const char *path);
int write_text_file_private(const char *path, const char *text);
char *now_time_string(void);
char *now_datetime_string(void);
char *sanitize_session_name(const char *name);
char *default_session_name(void);
char *normalize_model_id(const char *model);
char *url_encode_component(const char *s);
char *base64_encode(const unsigned char *data, size_t len);
unsigned char *base64_decode(const char *text, size_t *out_len);
char *guess_mime_type(const char *path);
const char *mime_extension(const char *mime_type);
int text_like_mime(const char *mime_type);
char *shell_quote(const char *s);
char *effective_editor(const Config *cfg);
char *ttychatter_strdup_printf(const char *fmt, ...);
int command_exists(const char *command_line, char **detail);
char *safe_getenv(const char *name);

/* config.c */
void config_init(Config *cfg);
void config_free(Config *cfg);
void config_load(Config *cfg);
const char *config_effective_model(const Config *cfg);
int config_set_value(const Config *cfg, const char *key, const char *value);
char *config_summary(const Config *cfg);

/* log.c */
void log_entry_list_init(LogEntryList *list);
void log_entry_list_free(LogEntryList *list);
int append_log_entry(const char *path, const char *speaker, const char *text, const char *timestamp);
int append_context_snapshot(const char *path, char **context, size_t context_len);
int load_log_entries(const char *path, LogEntryList *out);
char **latest_context_snapshot(const LogEntryList *entries, size_t *out_len);
char *extract_code_attachments(const char *text, const char *attachment_dir, const char *session_name);
char **list_session_names(const char *session_dir, size_t *out_len);
void free_string_array(char **items, size_t len);

/* gemini.c */
void attachment_list_init(AttachmentList *list);
void attachment_list_free(AttachmentList *list);
void attachment_list_add(AttachmentList *list, const char *path, const char *mime_type);
int gemini_generate(const Config *cfg,
                    const char *model_id,
                    const char *prompt_text,
                    const char *session_name,
                    const char *attachment_dir,
                    const AttachmentList *attachments,
                    long timeout_seconds,
                    char **out_text,
                    char **out_error);
int gemini_fetch_models(const Config *cfg, ModelList *out_models, char **out_error);

/* models.c */
void model_list_init(ModelList *list);
void model_list_free(ModelList *list);
void model_list_add(ModelList *list, const ModelInfo *model);
int model_supports_generate_content(const ModelInfo *model);
int model_has_required_token_limits(const ModelInfo *model);
int model_looks_preview(const ModelInfo *model);
int model_visible_for_config(const Config *cfg, const ModelInfo *model);
ModelList model_filtered_copy(const Config *cfg, const ModelList *source);
void model_list_sort_for_config(const Config *cfg, ModelList *list);
int save_model_cache(const Config *cfg, const ModelList *models);
int load_model_cache(const Config *cfg, ModelList *models, char **out_error);
char *model_filter_summary(const Config *cfg, size_t before_count, size_t after_count);
void print_model_rows(const Config *cfg, const ModelList *models);
int command_update_models(Config *cfg);
int command_print_models(Config *cfg, int update_first);
int command_test_model(Config *cfg, const char *model_id);

/* doctor.c */
int print_doctor_report(const Config *cfg);

/* ui.c */
int run_curses_app(Config *cfg, const char *session_name, const char *session_path);

#endif /* TTYCHATTER_H */
