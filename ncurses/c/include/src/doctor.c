#include "ttychatter.h"

#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

static void print_row(const char *status, const char *label, const char *detail) {
    printf("%-5s %-22s %s\n", status ? status : "INFO", label ? label : "", detail ? detail : "");
}

static void check_writable_dir(const char *label, const char *path) {
    if (mkdir_p(path) != 0) {
        print_row("FAIL", label, path);
        return;
    }
    char *probe = path_join2(path, ".ttychatter-doctor-write-test");
    FILE *fh = fopen(probe, "wb");
    if (!fh) {
        print_row("FAIL", label, probe);
        free(probe);
        return;
    }
    fputs("ok", fh);
    fclose(fh);
    unlink(probe);
    print_row("OK", label, path);
    free(probe);
}

static void check_dns(void) {
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    struct addrinfo *result = NULL;
    int rc = getaddrinfo("generativelanguage.googleapis.com", "443", &hints, &result);
    if (rc != 0) {
        print_row("WARN", "dns", gai_strerror(rc));
        return;
    }
    print_row("OK", "dns", "generativelanguage.googleapis.com resolves");
    freeaddrinfo(result);
}

static void check_terminal(void) {
    const char *term = getenv("TERM");
    print_row((term && *term) ? "OK" : "WARN", "TERM", (term && *term) ? term : "TERM is not set");
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col && ws.ws_row) {
        char *detail = ttychatter_strdup_printf("%ux%u", (unsigned)ws.ws_col, (unsigned)ws.ws_row);
        print_row((ws.ws_col >= 80 && ws.ws_row >= 20) ? "OK" : "WARN", "terminal size", detail);
        free(detail);
    } else {
        print_row("WARN", "terminal size", "could not determine size");
    }
}

static void check_model_cache(const Config *cfg) {
    ModelList models;
    char *error = NULL;
    if (load_model_cache(cfg, &models, &error) != 0) {
        print_row("WARN", "model cache", error ? error : "not readable");
        free(error);
        model_list_free(&models);
        return;
    }
    ModelList visible = model_filtered_copy(cfg, &models);
    char *detail = ttychatter_strdup_printf("%zu/%zu visible; updated %s", visible.len, models.len, models.updated_at ? models.updated_at : "unknown");
    print_row("OK", "model cache", detail);
    free(detail);
    model_list_free(&visible);
    model_list_free(&models);
}

int print_doctor_report(const Config *cfg) {
    print_row("OK", "program", TTYCHATTER_PROGRAM " " TTYCHATTER_VERSION);
    print_row("OK", "provider", TTYCHATTER_PROVIDER);
    print_row("OK", "config file", cfg->config_file);
    print_row((cfg->api_key && *cfg->api_key) ? "OK" : "WARN", "api key", (cfg->api_key && *cfg->api_key) ? "loaded" : "missing GEMINI_API_KEY/config value");
    check_terminal();
    check_writable_dir("session dir", cfg->session_dir);
    check_writable_dir("attachment dir", cfg->attachment_dir);

    char *cache_parent = xstrdup(cfg->model_cache_file);
    char *slash = strrchr(cache_parent, '/');
    if (slash) *slash = '\0';
    check_writable_dir("cache dir", cache_parent);
    free(cache_parent);

    char *editor = effective_editor(cfg);
    char *detail = NULL;
    int editor_ok = command_exists(editor, &detail);
    print_row(editor_ok ? "OK" : "WARN", "editor", detail ? detail : editor);
    free(detail);
    free(editor);

    check_model_cache(cfg);
    check_dns();
    return 0;
}
