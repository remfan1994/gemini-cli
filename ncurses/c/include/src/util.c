#include "ttychatter.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

/*
 * Checked allocation helpers.
 *
 * This program is interactive and stateful.  When allocation fails there is no
 * safe partial result to return from deep inside JSON building, screen drawing,
 * or log parsing.  Exiting with a clear message is less surprising than letting
 * an unchecked NULL pointer corrupt a log or crash later in curses.
 */
void *xmalloc(size_t size) {
    void *ptr = malloc(size ? size : 1);
    if (!ptr) {
        fprintf(stderr, "ttychatter: out of memory\n");
        exit(2);
    }
    return ptr;
}

void *xcalloc(size_t count, size_t size) {
    void *ptr = calloc(count ? count : 1, size ? size : 1);
    if (!ptr) {
        fprintf(stderr, "ttychatter: out of memory\n");
        exit(2);
    }
    return ptr;
}

void *xrealloc(void *ptr, size_t size) {
    void *next = realloc(ptr, size ? size : 1);
    if (!next) {
        fprintf(stderr, "ttychatter: out of memory\n");
        exit(2);
    }
    return next;
}

char *xstrdup(const char *s) {
    if (!s) {
        return xstrdup("");
    }
    size_t n = strlen(s);
    char *out = xmalloc(n + 1);
    memcpy(out, s, n + 1);
    return out;
}

char *xstrndup(const char *s, size_t n) {
    char *out = xmalloc(n + 1);
    if (s && n) {
        memcpy(out, s, n);
    }
    out[n] = '\0';
    return out;
}

void buffer_init(Buffer *b) {
    b->data = xstrdup("");
    b->len = 0;
    b->cap = 1;
}

void buffer_free(Buffer *b) {
    if (!b) return;
    free(b->data);
    b->data = NULL;
    b->len = 0;
    b->cap = 0;
}

void buffer_reserve(Buffer *b, size_t needed) {
    if (needed <= b->cap) return;
    size_t cap = b->cap ? b->cap : 1;
    while (cap < needed) {
        if (cap > ((size_t)-1) / 2) {
            fprintf(stderr, "ttychatter: buffer too large\n");
            exit(2);
        }
        cap *= 2;
    }
    b->data = xrealloc(b->data, cap);
    b->cap = cap;
}

void buffer_append_n(Buffer *b, const char *s, size_t n) {
    if (!s || n == 0) return;
    buffer_reserve(b, b->len + n + 1);
    memcpy(b->data + b->len, s, n);
    b->len += n;
    b->data[b->len] = '\0';
}

void buffer_append(Buffer *b, const char *s) {
    if (!s) return;
    buffer_append_n(b, s, strlen(s));
}

void buffer_appendf(Buffer *b, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    va_list copy;
    va_copy(copy, ap);
    int needed = vsnprintf(NULL, 0, fmt, copy);
    va_end(copy);
    if (needed < 0) {
        va_end(ap);
        return;
    }
    size_t old = b->len;
    buffer_reserve(b, old + (size_t)needed + 1);
    vsnprintf(b->data + old, b->cap - old, fmt, ap);
    b->len += (size_t)needed;
    va_end(ap);
}

char *buffer_steal(Buffer *b) {
    char *out = b->data;
    b->data = NULL;
    b->len = 0;
    b->cap = 0;
    return out ? out : xstrdup("");
}

char *ttychatter_strdup_printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    va_list copy;
    va_copy(copy, ap);
    int needed = vsnprintf(NULL, 0, fmt, copy);
    va_end(copy);
    if (needed < 0) {
        va_end(ap);
        return xstrdup("");
    }
    char *out = xmalloc((size_t)needed + 1);
    vsnprintf(out, (size_t)needed + 1, fmt, ap);
    va_end(ap);
    return out;
}

char *trim_in_place(char *s) {
    if (!s) return s;
    while (isspace((unsigned char)*s)) s++;
    if (*s == '\0') return s;
    char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) {
        *end = '\0';
        end--;
    }
    return s;
}

int parse_bool_value(const char *s, int fallback) {
    if (!s) return fallback;
    char tmp[32];
    size_t n = strlen(s);
    if (n >= sizeof(tmp)) n = sizeof(tmp) - 1;
    for (size_t i = 0; i < n; i++) tmp[i] = (char)tolower((unsigned char)s[i]);
    tmp[n] = '\0';
    char *v = trim_in_place(tmp);
    if (!strcmp(v, "1") || !strcmp(v, "yes") || !strcmp(v, "true") || !strcmp(v, "on") || !strcmp(v, "y")) return 1;
    if (!strcmp(v, "0") || !strcmp(v, "no") || !strcmp(v, "false") || !strcmp(v, "off") || !strcmp(v, "n")) return 0;
    return fallback;
}

long parse_long_value(const char *s, long fallback, long min_value) {
    if (!s) return fallback;
    errno = 0;
    char *end = NULL;
    long value = strtol(s, &end, 10);
    if (errno || end == s) return fallback;
    if (value < min_value) value = min_value;
    return value;
}

char *safe_getenv(const char *name) {
    const char *value = getenv(name);
    return xstrdup(value ? value : "");
}

static const char *home_dir(void) {
    const char *home = getenv("HOME");
    if (home && *home) return home;
    return ".";
}

char *path_join2(const char *a, const char *b) {
    if (!a || !*a) return xstrdup(b ? b : "");
    if (!b || !*b) return xstrdup(a);
    size_t alen = strlen(a);
    int slash = (alen > 0 && a[alen - 1] == '/');
    return ttychatter_strdup_printf("%s%s%s", a, slash ? "" : "/", b);
}

char *path_join3(const char *a, const char *b, const char *c) {
    char *ab = path_join2(a, b);
    char *abc = path_join2(ab, c);
    free(ab);
    return abc;
}

char *path_join4(const char *a, const char *b, const char *c, const char *d) {
    char *abc = path_join3(a, b, c);
    char *abcd = path_join2(abc, d);
    free(abc);
    return abcd;
}

/*
 * Expand a small, shell-like subset of paths.
 *
 * We support leading ~ and leading environment variables such as $HOME/foo.
 * The config format is intentionally simple, so this is not a full shell parser.
 * It is enough for portable ttychatter paths without invoking a shell.
 */
char *expand_path(const char *path) {
    if (!path || !*path) return xstrdup("");
    if (path[0] == '~') {
        if (path[1] == '\0') return xstrdup(home_dir());
        if (path[1] == '/') return path_join2(home_dir(), path + 2);
    }
    if (path[0] == '$') {
        const char *p = path + 1;
        size_t name_len = 0;
        while (p[name_len] && (isalnum((unsigned char)p[name_len]) || p[name_len] == '_')) name_len++;
        if (name_len > 0) {
            char *name = xstrndup(p, name_len);
            const char *value = getenv(name);
            free(name);
            if (value && *value) {
                const char *rest = p + name_len;
                if (*rest == '/') return path_join2(value, rest + 1);
                if (*rest == '\0') return xstrdup(value);
            }
        }
    }
    return xstrdup(path);
}

int mkdir_p(const char *path) {
    if (!path || !*path) return -1;
    char *tmp = xstrdup(path);
    size_t len = strlen(tmp);
    while (len > 1 && tmp[len - 1] == '/') tmp[--len] = '\0';
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(tmp, 0700) != 0 && errno != EEXIST) {
                int saved = errno;
                free(tmp);
                errno = saved;
                return -1;
            }
            *p = '/';
        }
    }
    if (mkdir(tmp, 0700) != 0 && errno != EEXIST) {
        int saved = errno;
        free(tmp);
        errno = saved;
        return -1;
    }
    free(tmp);
    return 0;
}

int file_exists(const char *path) {
    struct stat st;
    return path && stat(path, &st) == 0;
}

int is_directory(const char *path) {
    struct stat st;
    return path && stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

char *read_text_file(const char *path) {
    FILE *fh = fopen(path, "rb");
    if (!fh) return NULL;
    Buffer b;
    buffer_init(&b);
    char tmp[8192];
    size_t n;
    while ((n = fread(tmp, 1, sizeof(tmp), fh)) > 0) {
        buffer_append_n(&b, tmp, n);
    }
    if (ferror(fh)) {
        fclose(fh);
        buffer_free(&b);
        return NULL;
    }
    fclose(fh);
    return buffer_steal(&b);
}

static char *parent_dir(const char *path) {
    char *copy = xstrdup(path);
    char *slash = strrchr(copy, '/');
    if (!slash) {
        free(copy);
        return xstrdup(".");
    }
    if (slash == copy) {
        slash[1] = '\0';
    } else {
        *slash = '\0';
    }
    return copy;
}

int write_text_file_private(const char *path, const char *text) {
    char *parent = parent_dir(path);
    if (mkdir_p(parent) != 0) {
        free(parent);
        return -1;
    }
    free(parent);
    FILE *fh = fopen(path, "wb");
    if (!fh) return -1;
    if (text && fputs(text, fh) == EOF) {
        fclose(fh);
        return -1;
    }
    if (fclose(fh) != 0) return -1;
    chmod(path, 0600);
    return 0;
}

char *now_time_string(void) {
    time_t t = time(NULL);
    struct tm tmv;
    localtime_r(&t, &tmv);
    char buf[16];
    strftime(buf, sizeof(buf), "%H:%M:%S", &tmv);
    return xstrdup(buf);
}

char *now_datetime_string(void) {
    time_t t = time(NULL);
    struct tm tmv;
    localtime_r(&t, &tmv);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &tmv);
    return xstrdup(buf);
}

char *sanitize_session_name(const char *name) {
    const char *src = name && *name ? name : "session";
    Buffer out;
    buffer_init(&out);
    for (const unsigned char *p = (const unsigned char *)src; *p; p++) {
        if (isalnum(*p) || *p == '.' || *p == '_' || *p == '-') {
            char c[2] = {(char)*p, '\0'};
            buffer_append(&out, c);
        } else {
            buffer_append(&out, "_");
        }
    }
    while (out.len > 0 && (out.data[0] == '.' || out.data[0] == '_' || out.data[0] == '-')) {
        memmove(out.data, out.data + 1, out.len);
        out.len--;
    }
    while (out.len > 0 && (out.data[out.len - 1] == '.' || out.data[out.len - 1] == '_' || out.data[out.len - 1] == '-')) {
        out.data[--out.len] = '\0';
    }
    if (out.len == 0) {
        buffer_append(&out, "session");
    }
    return buffer_steal(&out);
}

char *default_session_name(void) {
    time_t t = time(NULL);
    struct tm tmv;
    localtime_r(&t, &tmv);
    char buf[64];
    strftime(buf, sizeof(buf), "session-%Y-%m-%d-%H-%M-%S", &tmv);
    return xstrdup(buf);
}

char *normalize_model_id(const char *model) {
    char *copy = xstrdup(model ? model : "");
    char *trimmed = trim_in_place(copy);
    char *out;
    if (!strncmp(trimmed, "models/", 7)) {
        out = xstrdup(trimmed + 7);
    } else {
        out = xstrdup(trimmed);
    }
    free(copy);
    return out;
}

char *url_encode_component(const char *s) {
    static const char hex[] = "0123456789ABCDEF";
    Buffer out;
    buffer_init(&out);
    for (const unsigned char *p = (const unsigned char *)(s ? s : ""); *p; p++) {
        unsigned char c = *p;
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            char one[2] = {(char)c, '\0'};
            buffer_append(&out, one);
        } else {
            char enc[4] = {'%', hex[c >> 4], hex[c & 15], '\0'};
            buffer_append(&out, enc);
        }
    }
    return buffer_steal(&out);
}

char *base64_encode(const unsigned char *data, size_t len) {
    static const char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t out_len = 4 * ((len + 2) / 3);
    char *out = xmalloc(out_len + 1);
    size_t i = 0, j = 0;
    while (i < len) {
        unsigned int octet_a = i < len ? data[i++] : 0;
        unsigned int octet_b = i < len ? data[i++] : 0;
        unsigned int octet_c = i < len ? data[i++] : 0;
        unsigned int triple = (octet_a << 16) | (octet_b << 8) | octet_c;
        out[j++] = table[(triple >> 18) & 0x3F];
        out[j++] = table[(triple >> 12) & 0x3F];
        out[j++] = table[(triple >> 6) & 0x3F];
        out[j++] = table[triple & 0x3F];
    }
    if (len % 3) {
        out[out_len - 1] = '=';
        if (len % 3 == 1) out[out_len - 2] = '=';
    }
    out[out_len] = '\0';
    return out;
}

static int b64_value(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    if (c == '=') return -2;
    return -1;
}

unsigned char *base64_decode(const char *text, size_t *out_len) {
    Buffer clean;
    buffer_init(&clean);
    for (const char *p = text ? text : ""; *p; p++) {
        if (!isspace((unsigned char)*p)) buffer_append_n(&clean, p, 1);
    }
    size_t len = clean.len;
    if (len == 0 || len % 4 != 0) {
        buffer_free(&clean);
        if (out_len) *out_len = 0;
        return NULL;
    }
    size_t pad = 0;
    if (clean.data[len - 1] == '=') pad++;
    if (clean.data[len - 2] == '=') pad++;
    size_t bytes = (len / 4) * 3 - pad;
    unsigned char *out = xmalloc(bytes ? bytes : 1);
    size_t j = 0;
    for (size_t i = 0; i < len; i += 4) {
        int a = b64_value(clean.data[i]);
        int b = b64_value(clean.data[i + 1]);
        int c = b64_value(clean.data[i + 2]);
        int d = b64_value(clean.data[i + 3]);
        if (a < 0 || b < 0 || c == -1 || d == -1) {
            free(out);
            buffer_free(&clean);
            if (out_len) *out_len = 0;
            return NULL;
        }
        unsigned int triple = ((unsigned int)a << 18) | ((unsigned int)b << 12) |
                              ((unsigned int)(c < 0 ? 0 : c) << 6) |
                              (unsigned int)(d < 0 ? 0 : d);
        if (j < bytes) out[j++] = (unsigned char)((triple >> 16) & 0xFF);
        if (j < bytes) out[j++] = (unsigned char)((triple >> 8) & 0xFF);
        if (j < bytes) out[j++] = (unsigned char)(triple & 0xFF);
    }
    buffer_free(&clean);
    if (out_len) *out_len = bytes;
    return out;
}

static int ends_with_ci(const char *s, const char *suffix) {
    size_t sl = strlen(s), su = strlen(suffix);
    if (su > sl) return 0;
    s += sl - su;
    for (size_t i = 0; i < su; i++) {
        if (tolower((unsigned char)s[i]) != tolower((unsigned char)suffix[i])) return 0;
    }
    return 1;
}

char *guess_mime_type(const char *path) {
    if (!path) return xstrdup("application/octet-stream");
    if (ends_with_ci(path, ".txt") || ends_with_ci(path, ".md") || ends_with_ci(path, ".c") ||
        ends_with_ci(path, ".h") || ends_with_ci(path, ".sh") || ends_with_ci(path, ".py") ||
        ends_with_ci(path, ".log")) return xstrdup("text/plain");
    if (ends_with_ci(path, ".json")) return xstrdup("application/json");
    if (ends_with_ci(path, ".xml")) return xstrdup("application/xml");
    if (ends_with_ci(path, ".yml") || ends_with_ci(path, ".yaml")) return xstrdup("application/x-yaml");
    if (ends_with_ci(path, ".html") || ends_with_ci(path, ".htm")) return xstrdup("text/html");
    if (ends_with_ci(path, ".css")) return xstrdup("text/css");
    if (ends_with_ci(path, ".js")) return xstrdup("text/javascript");
    if (ends_with_ci(path, ".png")) return xstrdup("image/png");
    if (ends_with_ci(path, ".jpg") || ends_with_ci(path, ".jpeg")) return xstrdup("image/jpeg");
    if (ends_with_ci(path, ".gif")) return xstrdup("image/gif");
    if (ends_with_ci(path, ".webp")) return xstrdup("image/webp");
    if (ends_with_ci(path, ".pdf")) return xstrdup("application/pdf");
    return xstrdup("application/octet-stream");
}

const char *mime_extension(const char *mime_type) {
    if (!mime_type) return ".bin";
    if (!strcmp(mime_type, "image/png")) return ".png";
    if (!strcmp(mime_type, "image/jpeg")) return ".jpg";
    if (!strcmp(mime_type, "image/gif")) return ".gif";
    if (!strcmp(mime_type, "image/webp")) return ".webp";
    if (!strcmp(mime_type, "application/pdf")) return ".pdf";
    if (!strcmp(mime_type, "application/json")) return ".json";
    if (!strncmp(mime_type, "text/", 5)) return ".txt";
    return ".bin";
}

int text_like_mime(const char *mime_type) {
    if (!mime_type) return 0;
    return !strncmp(mime_type, "text/", 5) || !strcmp(mime_type, "application/json") ||
           !strcmp(mime_type, "application/xml") || !strcmp(mime_type, "application/x-yaml");
}

char *shell_quote(const char *s) {
    Buffer b;
    buffer_init(&b);
    buffer_append(&b, "'");
    for (const char *p = s ? s : ""; *p; p++) {
        if (*p == '\'') buffer_append(&b, "'\\''");
        else buffer_append_n(&b, p, 1);
    }
    buffer_append(&b, "'");
    return buffer_steal(&b);
}

char *effective_editor(const Config *cfg) {
    if (cfg && cfg->editor && *cfg->editor) return xstrdup(cfg->editor);
    const char *visual = getenv("VISUAL");
    if (visual && *visual) return xstrdup(visual);
    const char *editor = getenv("EDITOR");
    if (editor && *editor) return xstrdup(editor);
    return xstrdup("vi");
}

int command_exists(const char *command_line, char **detail) {
    if (detail) *detail = NULL;
    if (!command_line || !*command_line) {
        if (detail) *detail = xstrdup("empty command");
        return 0;
    }
    while (isspace((unsigned char)*command_line)) command_line++;
    char exe[PATH_MAX];
    size_t n = 0;
    int quoted = 0;
    char quote = '\0';
    for (const char *p = command_line; *p && n < sizeof(exe) - 1; p++) {
        if (!quoted && isspace((unsigned char)*p)) break;
        if (!quoted && (*p == '\'' || *p == '"')) {
            quoted = 1;
            quote = *p;
            continue;
        }
        if (quoted && *p == quote) {
            quoted = 0;
            continue;
        }
        exe[n++] = *p;
    }
    exe[n] = '\0';
    if (n == 0) {
        if (detail) *detail = xstrdup("empty command");
        return 0;
    }
    if (strchr(exe, '/')) {
        char *expanded = expand_path(exe);
        int ok = access(expanded, X_OK) == 0;
        if (detail) *detail = ok ? xstrdup(expanded) : ttychatter_strdup_printf("not executable or not found: %s", expanded);
        free(expanded);
        return ok;
    }
    const char *path = getenv("PATH");
    if (!path) path = "/bin:/usr/bin";
    char *copy = xstrdup(path);
    for (char *part = strtok(copy, ":"); part; part = strtok(NULL, ":")) {
        char *candidate = path_join2(*part ? part : ".", exe);
        if (access(candidate, X_OK) == 0) {
            if (detail) *detail = xstrdup(candidate);
            free(candidate);
            free(copy);
            return 1;
        }
        free(candidate);
    }
    free(copy);
    if (detail) *detail = ttychatter_strdup_printf("not found in PATH: %s", exe);
    return 0;
}
