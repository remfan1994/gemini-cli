#include "ttychatter.h"

#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Program bootstrap.
 *
 * The CLI intentionally mirrors the Python reference so scripts and users do
 * not need to relearn the command surface when moving to the C port.  Most
 * interactive behavior lives in ui.c; main.c should stay thin: load config,
 * run noninteractive commands, choose a session, and enter ncurses.
 */
typedef struct Args {
    const char *session_name;
    const char *resume_name;
    const char *test_model;
    int list;
    int models;
    int update_models;
    int doctor;
    int version;
    int help;
} Args;

static void print_help(void) {
    printf("Usage: ttychatter [SESSION]\n");
    printf("       ttychatter --resume NAME\n");
    printf("       ttychatter --list\n");
    printf("       ttychatter --models\n");
    printf("       ttychatter --update-models\n");
    printf("       ttychatter --test-model MODEL\n");
    printf("       ttychatter --doctor\n");
    printf("\n");
    printf("Native C/ncurses Gemini terminal client.\n");
    printf("Config: $XDG_CONFIG_HOME/ttychatter/gemini/config\n");
    printf("Data:   $XDG_DATA_HOME/ttychatter/gemini/\n");
}

static int parse_args(int argc, char **argv, Args *args) {
    memset(args, 0, sizeof(*args));
    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        if (!strcmp(a, "help") || !strcmp(a, "--help") || !strcmp(a, "-h")) args->help = 1;
        else if (!strcmp(a, "--version")) args->version = 1;
        else if (!strcmp(a, "--list")) args->list = 1;
        else if (!strcmp(a, "--models")) args->models = 1;
        else if (!strcmp(a, "--update-models")) args->update_models = 1;
        else if (!strcmp(a, "--doctor")) args->doctor = 1;
        else if (!strcmp(a, "--resume")) {
            if (i + 1 >= argc) {
                fprintf(stderr, "ttychatter: --resume requires a name\n");
                return -1;
            }
            args->resume_name = argv[++i];
        } else if (!strcmp(a, "--test-model")) {
            if (i + 1 >= argc) {
                fprintf(stderr, "ttychatter: --test-model requires a model id\n");
                return -1;
            }
            args->test_model = argv[++i];
        } else if (a[0] == '-') {
            fprintf(stderr, "ttychatter: unknown option: %s\n", a);
            return -1;
        } else if (!args->session_name) {
            args->session_name = a;
        } else {
            fprintf(stderr, "ttychatter: extra positional argument: %s\n", a);
            return -1;
        }
    }
    return 0;
}

static int list_sessions_command(const Config *cfg) {
    size_t len = 0;
    char **names = list_session_names(cfg->session_dir, &len);
    for (size_t i = 0; i < len; i++) printf("%s\n", names[i]);
    free_string_array(names, len);
    return 0;
}

int main(int argc, char **argv) {
    Args args;
    if (parse_args(argc, argv, &args) != 0) return 2;
    if (args.help) {
        print_help();
        return 0;
    }
    if (args.version) {
        printf("%s version %s\n", TTYCHATTER_PROGRAM, TTYCHATTER_VERSION);
        return 0;
    }

    Config cfg;
    config_init(&cfg);
    config_load(&cfg);
    mkdir_p(cfg.session_dir);
    mkdir_p(cfg.attachment_dir);

    curl_global_init(CURL_GLOBAL_DEFAULT);

    int rc = 0;
    if (args.list) rc = list_sessions_command(&cfg);
    else if (args.doctor) rc = print_doctor_report(&cfg);
    else if (args.update_models) rc = command_update_models(&cfg);
    else if (args.models) rc = command_print_models(&cfg, 0);
    else if (args.test_model) rc = command_test_model(&cfg, args.test_model);
    else {
        char *raw_name = NULL;
        if (args.resume_name) raw_name = xstrdup(args.resume_name);
        else if (args.session_name) raw_name = xstrdup(args.session_name);
        else raw_name = default_session_name();
        char *session_name = sanitize_session_name(raw_name);
        char *filename = ttychatter_strdup_printf("%s.log", session_name);
        char *session_path = path_join2(cfg.session_dir, filename);
        rc = run_curses_app(&cfg, session_name, session_path);
        free(session_path);
        free(filename);
        free(session_name);
        free(raw_name);
    }

    curl_global_cleanup();
    config_free(&cfg);
    return rc;
}
