#include "ttychatter.h"

#include <ctype.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/*
 * The ncurses UI intentionally stays compact.
 *
 * The Python reference has a richer screen system.  This C port keeps the same
 * product shape: a persistent transcript, a multi-line input area, global
 * function-key navigation, session logs, rolling context memory, model
 * selection, API-key entry, attachments, and editor integration.  The code is
 * verbose because UI state bugs are hard to debug from a terminal screenshot.
 */
typedef struct TranscriptItem {
    char *speaker;
    char *text;
    char *timestamp;
} TranscriptItem;

typedef struct LineList {
    char **lines;
    int *dim;
    size_t len;
    size_t cap;
} LineList;

typedef struct App {
    Config *cfg;
    char *session_name;
    char *session_path;
    AttachmentList attachments;
    TranscriptItem *transcript;
    size_t transcript_len;
    size_t transcript_cap;
    char **context;
    size_t context_len;
    size_t context_cap;
    Buffer input;
    int transcript_scroll;
    int should_quit;
    char *status;
} App;

static void replace_app_string(char **slot, const char *value) {
    free(*slot);
    *slot = xstrdup(value ? value : "");
}

static void set_status(App *app, const char *status) {
    replace_app_string(&app->status, status ? status : "");
}

static void transcript_add(App *app, const char *speaker, const char *text, const char *timestamp) {
    if (app->transcript_len == app->transcript_cap) {
        app->transcript_cap = app->transcript_cap ? app->transcript_cap * 2 : 32;
        app->transcript = xrealloc(app->transcript, app->transcript_cap * sizeof(app->transcript[0]));
    }
    app->transcript[app->transcript_len].speaker = xstrdup(speaker ? speaker : "");
    app->transcript[app->transcript_len].text = xstrdup(text ? text : "");
    app->transcript[app->transcript_len].timestamp = xstrdup(timestamp ? timestamp : "");
    app->transcript_len++;
}

static void transcript_clear(App *app) {
    for (size_t i = 0; i < app->transcript_len; i++) {
        free(app->transcript[i].speaker);
        free(app->transcript[i].text);
        free(app->transcript[i].timestamp);
    }
    free(app->transcript);
    app->transcript = NULL;
    app->transcript_len = 0;
    app->transcript_cap = 0;
}

static void context_add(App *app, const char *line) {
    if (app->context_len == app->context_cap) {
        app->context_cap = app->context_cap ? app->context_cap * 2 : 32;
        app->context = xrealloc(app->context, app->context_cap * sizeof(app->context[0]));
    }
    app->context[app->context_len++] = xstrdup(line ? line : "");
    size_t max_entries = (size_t)(app->cfg->context_turns > 0 ? app->cfg->context_turns : 1) * 2;
    while (app->context_len > max_entries) {
        free(app->context[0]);
        memmove(app->context, app->context + 1, (app->context_len - 1) * sizeof(app->context[0]));
        app->context_len--;
    }
}

static void context_clear(App *app) {
    for (size_t i = 0; i < app->context_len; i++) free(app->context[i]);
    free(app->context);
    app->context = NULL;
    app->context_len = 0;
    app->context_cap = 0;
}

static char *context_join(App *app) {
    Buffer b;
    buffer_init(&b);
    for (size_t i = 0; i < app->context_len; i++) {
        if (i) buffer_append(&b, "\n");
        buffer_append(&b, app->context[i]);
    }
    return buffer_steal(&b);
}

static void line_list_init(LineList *list) {
    list->lines = NULL;
    list->dim = NULL;
    list->len = 0;
    list->cap = 0;
}

static void line_list_add(LineList *list, const char *line, int dim) {
    if (list->len == list->cap) {
        list->cap = list->cap ? list->cap * 2 : 64;
        list->lines = xrealloc(list->lines, list->cap * sizeof(list->lines[0]));
        list->dim = xrealloc(list->dim, list->cap * sizeof(list->dim[0]));
    }
    list->lines[list->len] = xstrdup(line ? line : "");
    list->dim[list->len] = dim;
    list->len++;
}

static void line_list_free(LineList *list) {
    for (size_t i = 0; i < list->len; i++) free(list->lines[i]);
    free(list->lines);
    free(list->dim);
    list->lines = NULL;
    list->dim = NULL;
    list->len = 0;
    list->cap = 0;
}

static void add_wrapped(LineList *list, const char *prefix, const char *text, int width, int dim) {
    if (width < 10) width = 10;
    int prefix_len = (int)strlen(prefix ? prefix : "");
    int available = width - prefix_len;
    if (available < 8) available = width;
    const char *p = text ? text : "";
    int first = 1;
    if (!*p) {
        line_list_add(list, prefix ? prefix : "", dim);
        return;
    }
    while (*p) {
        const char *newline = strchr(p, '\n');
        size_t para_len = newline ? (size_t)(newline - p) : strlen(p);
        size_t offset = 0;
        if (para_len == 0) {
            line_list_add(list, first ? prefix : "", dim);
            first = 0;
        }
        while (offset < para_len) {
            size_t chunk = para_len - offset;
            if (chunk > (size_t)available) chunk = (size_t)available;
            Buffer b;
            buffer_init(&b);
            if (first) buffer_append(&b, prefix ? prefix : "");
            else if (prefix_len > 0) {
                for (int i = 0; i < prefix_len; i++) buffer_append(&b, " ");
            }
            buffer_append_n(&b, p + offset, chunk);
            line_list_add(list, b.data, dim);
            buffer_free(&b);
            offset += chunk;
            first = 0;
        }
        if (!newline) break;
        p = newline + 1;
    }
}

static void safe_mvaddnstr(int y, int x, const char *s, int max_width) {
    if (y < 0 || x < 0 || max_width <= 0) return;
    mvaddnstr(y, x, s ? s : "", max_width);
}

static void draw_main(App *app) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    erase();
    if (rows < 12 || cols < 40) {
        safe_mvaddnstr(0, 0, "ttychatter needs at least 40x12.", cols);
        refresh();
        return;
    }

    char *header = ttychatter_strdup_printf("%s %s | %s | model:%s | key:%s | files:%zu",
                                           TTYCHATTER_PROGRAM,
                                           TTYCHATTER_VERSION,
                                           app->session_name,
                                           config_effective_model(app->cfg),
                                           (app->cfg->api_key && *app->cfg->api_key) ? "loaded" : "missing",
                                           app->attachments.len);
    attron(A_REVERSE);
    safe_mvaddnstr(0, 0, header, cols);
    for (int x = (int)strlen(header); x < cols; x++) addch(' ');
    attroff(A_REVERSE);
    free(header);

    int input_height = 5;
    int status_y = rows - 1;
    int input_y = rows - input_height - 1;
    int command_y = input_y - 1;
    int transcript_y = 1;
    int transcript_h = command_y - transcript_y;

    LineList lines;
    line_list_init(&lines);
    for (size_t i = 0; i < app->transcript_len; i++) {
        char *prefix = ttychatter_strdup_printf("[%s] %s: ", app->transcript[i].timestamp, app->transcript[i].speaker);
        add_wrapped(&lines, prefix, app->transcript[i].text, cols, strcmp(app->transcript[i].speaker, "Gemini") != 0);
        free(prefix);
    }
    int max_scroll = (int)lines.len - transcript_h;
    if (max_scroll < 0) max_scroll = 0;
    if (app->transcript_scroll > max_scroll) app->transcript_scroll = max_scroll;
    if (app->transcript_scroll < 0) app->transcript_scroll = 0;
    int start = (int)lines.len - transcript_h - app->transcript_scroll;
    if (start < 0) start = 0;
    for (int i = 0; i < transcript_h; i++) {
        int idx = start + i;
        if (idx >= 0 && idx < (int)lines.len) {
            if (lines.dim[idx]) attron(A_DIM);
            safe_mvaddnstr(transcript_y + i, 0, lines.lines[idx], cols);
            if (lines.dim[idx]) attroff(A_DIM);
        }
    }
    line_list_free(&lines);

    attron(A_DIM);
    safe_mvaddnstr(command_y, 0, "F1 Help | F2 Models | F3 Key | F4 Sessions | F5 Options | F6 Memory | F7 Files | F8 Editor | F9 Credits | F10 Quit", cols);
    attroff(A_DIM);

    for (int y = input_y; y < status_y; y++) {
        mvhline(y, 0, ' ', cols);
    }
    mvhline(input_y, 0, '-', cols);
    const char *input_text = app->input.len ? app->input.data : app->cfg->input_placeholder;
    if (!app->input.len) attron(A_DIM);
    LineList input_lines;
    line_list_init(&input_lines);
    add_wrapped(&input_lines, "> ", input_text, cols, !app->input.len);
    int visible = input_height - 1;
    int input_start = (int)input_lines.len - visible;
    if (input_start < 0) input_start = 0;
    for (int i = 0; i < visible && input_start + i < (int)input_lines.len; i++) {
        safe_mvaddnstr(input_y + 1 + i, 0, input_lines.lines[input_start + i], cols);
    }
    line_list_free(&input_lines);
    if (!app->input.len) attroff(A_DIM);

    attron(A_REVERSE);
    char *status = ttychatter_strdup_printf("%s", app->status ? app->status : "F1 Menu | Ctrl+G Send | Ctrl+Q Quit");
    safe_mvaddnstr(status_y, 0, status, cols);
    for (int x = (int)strlen(status); x < cols; x++) addch(' ');
    free(status);
    attroff(A_REVERSE);
    refresh();
}

static void modal_text(const char *title, const char *body) {
    int rows, cols;
    int scroll = 0;
    for (;;) {
        getmaxyx(stdscr, rows, cols);
        erase();
        attron(A_REVERSE);
        safe_mvaddnstr(0, 0, title, cols);
        for (int x = (int)strlen(title); x < cols; x++) addch(' ');
        attroff(A_REVERSE);
        LineList lines;
        line_list_init(&lines);
        add_wrapped(&lines, "", body ? body : "", cols, 0);
        int height = rows - 2;
        int max_scroll = (int)lines.len - height;
        if (max_scroll < 0) max_scroll = 0;
        if (scroll > max_scroll) scroll = max_scroll;
        if (scroll < 0) scroll = 0;
        for (int i = 0; i < height; i++) {
            int idx = scroll + i;
            if (idx < (int)lines.len) safe_mvaddnstr(i + 1, 0, lines.lines[idx], cols);
        }
        attron(A_DIM);
        safe_mvaddnstr(rows - 1, 0, "Up/Down scroll. Esc, q, or Enter returns.", cols);
        attroff(A_DIM);
        refresh();
        line_list_free(&lines);
        int ch = getch();
        if (ch == 27 || ch == 'q' || ch == 'Q' || ch == '\n' || ch == '\r') break;
        if (ch == KEY_UP) scroll--;
        else if (ch == KEY_DOWN) scroll++;
        else if (ch == KEY_NPAGE) scroll += 10;
        else if (ch == KEY_PPAGE) scroll -= 10;
    }
}

static char *modal_input(const char *title, const char *prompt, const char *initial, int secret) {
    Buffer input;
    buffer_init(&input);
    buffer_append(&input, initial ? initial : "");
    for (;;) {
        int rows, cols;
        getmaxyx(stdscr, rows, cols);
        erase();
        attron(A_REVERSE);
        safe_mvaddnstr(0, 0, title, cols);
        for (int x = (int)strlen(title); x < cols; x++) addch(' ');
        attroff(A_REVERSE);
        safe_mvaddnstr(2, 2, prompt ? prompt : "Value:", cols - 4);
        char *shown;
        if (secret) {
            shown = xmalloc(input.len + 1);
            memset(shown, '*', input.len);
            shown[input.len] = '\0';
        } else {
            shown = xstrdup(input.data);
        }
        safe_mvaddnstr(4, 2, shown, cols - 4);
        free(shown);
        attron(A_DIM);
        safe_mvaddnstr(rows - 1, 0, "Enter accepts. Esc cancels. Backspace edits.", cols);
        attroff(A_DIM);
        move(4, 2 + (int)input.len);
        refresh();
        int ch = getch();
        if (ch == 27) {
            buffer_free(&input);
            return NULL;
        }
        if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) return buffer_steal(&input);
        if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
            if (input.len > 0) input.data[--input.len] = '\0';
        } else if (ch >= 32 && ch <= 126) {
            char c = (char)ch;
            buffer_append_n(&input, &c, 1);
        }
    }
}

static int modal_menu(const char *title, char **items, size_t len, const char *footer) {
    if (len == 0) {
        modal_text(title, "No items available.");
        return -1;
    }
    int selected = 0;
    int scroll = 0;
    for (;;) {
        int rows, cols;
        getmaxyx(stdscr, rows, cols);
        erase();
        attron(A_REVERSE);
        safe_mvaddnstr(0, 0, title, cols);
        for (int x = (int)strlen(title); x < cols; x++) addch(' ');
        attroff(A_REVERSE);
        int height = rows - 2;
        if (selected < scroll) scroll = selected;
        if (selected >= scroll + height) scroll = selected - height + 1;
        for (int i = 0; i < height && scroll + i < (int)len; i++) {
            if (scroll + i == selected) attron(A_REVERSE);
            safe_mvaddnstr(i + 1, 0, items[scroll + i], cols);
            if (scroll + i == selected) attroff(A_REVERSE);
        }
        attron(A_DIM);
        safe_mvaddnstr(rows - 1, 0, footer ? footer : "Enter selects. Esc cancels.", cols);
        attroff(A_DIM);
        refresh();
        int ch = getch();
        if (ch == 27 || ch == 'q' || ch == 'Q') return -1;
        if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) return selected;
        if (ch == KEY_UP && selected > 0) selected--;
        else if (ch == KEY_DOWN && selected + 1 < (int)len) selected++;
        else if (ch == KEY_NPAGE) selected = selected + 10 < (int)len ? selected + 10 : (int)len - 1;
        else if (ch == KEY_PPAGE) selected = selected > 10 ? selected - 10 : 0;
    }
}

static void open_help(App *app) {
    (void)app;
    modal_text("ttychatter help",
               "Global keys:\n"
               "  F1 Help\n"
               "  F2 Model selector\n"
               "  F3 Paste/save Gemini API key\n"
               "  F4 Session browser\n"
               "  F5 Options\n"
               "  F6 Memory view/clear\n"
               "  F7 Attach a file for the next request\n"
               "  F8 Open external editor for input\n"
               "  F9 Credits\n"
               "  F10 Quit\n\n"
               "Chat keys:\n"
               "  Ctrl+G sends the current message.\n"
               "  Ctrl+Q quits.\n"
               "  Enter inserts a newline.\n"
               "  PageUp/PageDown scroll the transcript.\n\n"
               "Config is stored as KEY=VALUE in the XDG config directory.\n"
               "Sessions and attachments are stored in the XDG data directory.");
}

static void open_credits(App *app) {
    Buffer b;
    buffer_init(&b);
    buffer_appendf(&b, "%s %s\n\n", TTYCHATTER_PROGRAM, TTYCHATTER_VERSION);
    buffer_append(&b, "Native C/ncurses Gemini terminal client.\n\n");
    buffer_append(&b, "Project URL: ");
    buffer_append(&b, app->cfg->github_url);
    buffer_append(&b, "\n\n");
    buffer_append(&b, TTYCHATTER_PROJECT_NOTICE);
    modal_text("credits", b.data);
    buffer_free(&b);
}

static void open_api_key(App *app) {
    char *key = modal_input("API key", "Paste Gemini API key:", "", 1);
    if (!key) return;
    if (*key) {
        config_set_value(app->cfg, "GEMINI_API_KEY", key);
        replace_app_string(&app->cfg->api_key, key);
        set_status(app, "API key saved to config file with private permissions.");
    } else {
        set_status(app, "Empty API key ignored.");
    }
    free(key);
}

static void open_file_menu(App *app) {
    char *path = modal_input("Attach file", "Path to attach for next request:", "", 0);
    if (!path) return;
    char *trimmed = trim_in_place(path);
    if (!*trimmed) {
        free(path);
        set_status(app, "Empty path ignored.");
        return;
    }
    char *expanded = expand_path(trimmed);
    if (!file_exists(expanded)) {
        char *msg = ttychatter_strdup_printf("File not found: %s", expanded);
        set_status(app, msg);
        free(msg);
        free(expanded);
        free(path);
        return;
    }
    char *mime = guess_mime_type(expanded);
    attachment_list_add(&app->attachments, expanded, mime);
    char *msg = ttychatter_strdup_printf("Attached %s as %s for the next send.", expanded, mime);
    set_status(app, msg);
    free(msg);
    free(mime);
    free(expanded);
    free(path);
}

static void open_editor(App *app) {
    char *editor = effective_editor(app->cfg);
    char tmpl[] = "/tmp/ttychatter-input-XXXXXX";
    int fd = mkstemp(tmpl);
    if (fd < 0) {
        set_status(app, "Could not create temporary editor file.");
        free(editor);
        return;
    }
    if (app->input.len) write(fd, app->input.data, app->input.len);
    close(fd);
    char *quoted_editor = shell_quote(editor);
    char *quoted_file = shell_quote(tmpl);
    char *cmd = ttychatter_strdup_printf("%s %s", quoted_editor, quoted_file);
    endwin();
    int rc = system(cmd);
    refresh();
    char *edited = read_text_file(tmpl);
    unlink(tmpl);
    if (rc == 0 && edited) {
        buffer_free(&app->input);
        buffer_init(&app->input);
        buffer_append(&app->input, edited);
        set_status(app, "Editor content loaded into input.");
    } else {
        set_status(app, "Editor did not return usable content.");
    }
    free(edited);
    free(cmd);
    free(quoted_file);
    free(quoted_editor);
    free(editor);
}

static void load_session(App *app) {
    transcript_clear(app);
    context_clear(app);
    LogEntryList entries;
    load_log_entries(app->session_path, &entries);
    size_t start = entries.len > 80 ? entries.len - 80 : 0;
    for (size_t i = start; i < entries.len; i++) {
        if (!strcmp(entries.items[i].speaker, "system") && !strncmp(entries.items[i].text, "CONTEXT_SNAPSHOT_JSON ", 22)) continue;
        transcript_add(app, entries.items[i].speaker, entries.items[i].text, entries.items[i].timestamp);
    }
    size_t snapshot_len = 0;
    char **snapshot = latest_context_snapshot(&entries, &snapshot_len);
    if (snapshot) {
        for (size_t i = 0; i < snapshot_len; i++) context_add(app, snapshot[i]);
        free_string_array(snapshot, snapshot_len);
    } else {
        size_t max = (size_t)app->cfg->context_turns * 2;
        size_t begin = entries.len > max ? entries.len - max : 0;
        for (size_t i = begin; i < entries.len; i++) {
            if (!strcmp(entries.items[i].speaker, "system")) continue;
            char *line = ttychatter_strdup_printf("%s: %s", !strcmp(entries.items[i].speaker, "Gemini") ? "Assistant" : "User", entries.items[i].text);
            context_add(app, line);
            free(line);
        }
    }
    log_entry_list_free(&entries);
    if (app->transcript_len == 0 && app->cfg->startup_notice) {
        transcript_add(app, "system", "Welcome to ttychatter. Press F3 to load an API key, F2 to choose a model, and Ctrl+G to send.", "--:--:--");
    }
    set_status(app, "F1 Help | Ctrl+G Send | F10 Quit");
}

static void reset_session(App *app, const char *name) {
    char *safe = sanitize_session_name(name);
    char *filename = ttychatter_strdup_printf("%s.log", safe);
    char *path = path_join2(app->cfg->session_dir, filename);
    replace_app_string(&app->session_name, safe);
    replace_app_string(&app->session_path, path);
    load_session(app);
    free(path);
    free(filename);
    free(safe);
}

static void open_sessions(App *app) {
    size_t len = 0;
    char **names = list_session_names(app->cfg->session_dir, &len);
    size_t menu_len = len + 1;
    char **items = xcalloc(menu_len, sizeof(char *));
    items[0] = xstrdup("<new session>");
    for (size_t i = 0; i < len; i++) items[i + 1] = xstrdup(names[i]);
    int choice = modal_menu("sessions", items, menu_len, "Enter opens. Esc cancels.");
    if (choice == 0) {
        char *name = modal_input("new session", "Session name:", "", 0);
        if (name && *trim_in_place(name)) reset_session(app, name);
        free(name);
    } else if (choice > 0) {
        reset_session(app, items[choice]);
    }
    free_string_array(names, len);
    free_string_array(items, menu_len);
}

static char *model_row(const ModelInfo *m) {
    return ttychatter_strdup_printf("%-34s input:%-8ld output:%-8ld %s",
                                   m->model_id ? m->model_id : "",
                                   m->input_token_limit,
                                   m->output_token_limit,
                                   (m->display_name && *m->display_name) ? m->display_name : "");
}

static void open_models(App *app) {
    ModelList models;
    char *error = NULL;
    if (load_model_cache(app->cfg, &models, &error) != 0) {
        free(error);
        error = NULL;
        if (gemini_fetch_models(app->cfg, &models, &error) != 0) {
            char *msg = ttychatter_strdup_printf("Could not load models: %s", error ? error : "unknown error");
            modal_text("models", msg);
            set_status(app, msg);
            free(msg);
            free(error);
            return;
        }
        save_model_cache(app->cfg, &models);
    }
    ModelList visible = model_filtered_copy(app->cfg, &models);
    if (visible.len == 0) {
        modal_text("models", "No models are visible after filters. Adjust MODEL_FILTER_* settings in the config file.");
        model_list_free(&visible);
        model_list_free(&models);
        return;
    }

    int selected = 0;
    int scroll = 0;
    for (;;) {
        int rows, cols;
        getmaxyx(stdscr, rows, cols);
        erase();
        char *title = model_filter_summary(app->cfg, models.len, visible.len);
        attron(A_REVERSE);
        safe_mvaddnstr(0, 0, title, cols);
        for (int x = (int)strlen(title); x < cols; x++) addch(' ');
        attroff(A_REVERSE);
        free(title);
        int height = rows - 2;
        if (selected < scroll) scroll = selected;
        if (selected >= scroll + height) scroll = selected - height + 1;
        for (int i = 0; i < height && scroll + i < (int)visible.len; i++) {
            int idx = scroll + i;
            char *row = model_row(&visible.items[idx]);
            if (idx == selected) attron(A_REVERSE);
            safe_mvaddnstr(i + 1, 0, row, cols);
            if (idx == selected) attroff(A_REVERSE);
            free(row);
        }
        attron(A_DIM);
        safe_mvaddnstr(rows - 1, 0, "Enter saves selected model. t tests. u updates cache. Esc returns.", cols);
        attroff(A_DIM);
        refresh();
        int ch = getch();
        if (ch == 27 || ch == 'q' || ch == 'Q') break;
        if (ch == KEY_UP && selected > 0) selected--;
        else if (ch == KEY_DOWN && selected + 1 < (int)visible.len) selected++;
        else if (ch == KEY_NPAGE) selected = selected + 10 < (int)visible.len ? selected + 10 : (int)visible.len - 1;
        else if (ch == KEY_PPAGE) selected = selected > 10 ? selected - 10 : 0;
        else if (ch == 'u' || ch == 'U') {
            model_list_free(&visible);
            model_list_free(&models);
            error = NULL;
            if (gemini_fetch_models(app->cfg, &models, &error) != 0) {
                char *msg = ttychatter_strdup_printf("Update failed: %s", error ? error : "unknown error");
                modal_text("models", msg);
                free(msg);
                free(error);
                return;
            }
            save_model_cache(app->cfg, &models);
            visible = model_filtered_copy(app->cfg, &models);
            selected = 0;
            scroll = 0;
        } else if (ch == 't' || ch == 'T') {
            AttachmentList none;
            attachment_list_init(&none);
            char *text = NULL;
            char *err = NULL;
            int rc = gemini_generate(app->cfg, visible.items[selected].model_id, app->cfg->model_test_prompt, "model-test", app->cfg->attachment_dir, &none, 60, &text, &err);
            attachment_list_free(&none);
            if (rc == 0) modal_text("model test", text);
            else modal_text("model test failed", err ? err : "unknown error");
            free(text);
            free(err);
        } else if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
            const char *model_id = visible.items[selected].model_id;
            config_set_value(app->cfg, "MODEL", model_id);
            replace_app_string(&app->cfg->model, model_id);
            char *msg = ttychatter_strdup_printf("Model saved: %s", model_id);
            set_status(app, msg);
            free(msg);
            break;
        }
    }
    model_list_free(&visible);
    model_list_free(&models);
}

static void open_options(App *app) {
    char *items[] = {
        "Set MODEL manually",
        "Set CONTEXT_TURNS",
        "Toggle MONOCHROME",
        "Toggle STARTUP_NOTICE",
        "Show config summary"
    };
    int choice = modal_menu("options", items, 5, "Enter selects. Esc returns.");
    if (choice == 0) {
        char *model = modal_input("model", "Model id:", config_effective_model(app->cfg), 0);
        if (model && *trim_in_place(model)) {
            char *norm = normalize_model_id(model);
            config_set_value(app->cfg, "MODEL", norm);
            replace_app_string(&app->cfg->model, norm);
            set_status(app, "Model saved.");
            free(norm);
        }
        free(model);
    } else if (choice == 1) {
        char tmp[32];
        snprintf(tmp, sizeof(tmp), "%d", app->cfg->context_turns);
        char *value = modal_input("context", "Number of prior turns to keep:", tmp, 0);
        if (value) {
            int turns = (int)parse_long_value(value, app->cfg->context_turns, 1);
            app->cfg->context_turns = turns;
            snprintf(tmp, sizeof(tmp), "%d", turns);
            config_set_value(app->cfg, "CONTEXT_TURNS", tmp);
            set_status(app, "Context turn count saved.");
            free(value);
        }
    } else if (choice == 2) {
        app->cfg->monochrome = !app->cfg->monochrome;
        config_set_value(app->cfg, "MONOCHROME", app->cfg->monochrome ? "1" : "0");
        set_status(app, "MONOCHROME toggled.");
    } else if (choice == 3) {
        app->cfg->startup_notice = !app->cfg->startup_notice;
        config_set_value(app->cfg, "STARTUP_NOTICE", app->cfg->startup_notice ? "1" : "0");
        set_status(app, "STARTUP_NOTICE toggled.");
    } else if (choice == 4) {
        char *summary = config_summary(app->cfg);
        modal_text("config summary", summary);
        free(summary);
    }
}

static void open_memory(App *app) {
    int scroll = 0;
    for (;;) {
        Buffer b;
        buffer_init(&b);
        if (app->context_len == 0) buffer_append(&b, "Context memory is empty.\n");
        for (size_t i = 0; i < app->context_len; i++) {
            buffer_appendf(&b, "%zu. %s\n\n", i + 1, app->context[i]);
        }

        int rows, cols;
        getmaxyx(stdscr, rows, cols);
        erase();
        attron(A_REVERSE);
        safe_mvaddnstr(0, 0, "memory", cols);
        for (int x = 6; x < cols; x++) addch(' ');
        attroff(A_REVERSE);

        LineList lines;
        line_list_init(&lines);
        add_wrapped(&lines, "", b.data, cols, 0);
        int height = rows - 2;
        int max_scroll = (int)lines.len - height;
        if (max_scroll < 0) max_scroll = 0;
        if (scroll > max_scroll) scroll = max_scroll;
        if (scroll < 0) scroll = 0;
        for (int i = 0; i < height; i++) {
            int idx = scroll + i;
            if (idx < (int)lines.len) safe_mvaddnstr(i + 1, 0, lines.lines[idx], cols);
        }
        attron(A_DIM);
        safe_mvaddnstr(rows - 1, 0, "c clears memory. Up/Down scroll. Esc, q, or Enter returns.", cols);
        attroff(A_DIM);
        refresh();
        line_list_free(&lines);
        buffer_free(&b);

        int ch = getch();
        if (ch == 27 || ch == 'q' || ch == 'Q' || ch == '\n' || ch == '\r') break;
        if (ch == 'c' || ch == 'C') {
            context_clear(app);
            append_context_snapshot(app->session_path, app->context, app->context_len);
            set_status(app, "Context memory cleared.");
            break;
        }
        if (ch == KEY_UP) scroll--;
        else if (ch == KEY_DOWN) scroll++;
        else if (ch == KEY_NPAGE) scroll += 10;
        else if (ch == KEY_PPAGE) scroll -= 10;
    }
}

static void send_current_message(App *app) {
    char *message = xstrdup(app->input.data);
    char *trimmed = trim_in_place(message);
    if (!*trimmed && app->attachments.len == 0) {
        set_status(app, "Empty message ignored.");
        free(message);
        buffer_free(&app->input);
        buffer_init(&app->input);
        return;
    }
    if (!*trimmed && app->attachments.len > 0) {
        free(message);
        message = xstrdup("Please review the attached file(s).");
        trimmed = message;
    }

    Buffer display;
    buffer_init(&display);
    buffer_append(&display, trimmed);
    if (app->attachments.len) {
        buffer_append(&display, "\n[Pending attachments sent: ");
        for (size_t i = 0; i < app->attachments.len; i++) {
            if (i) buffer_append(&display, ", ");
            const char *slash = strrchr(app->attachments.items[i].path, '/');
            buffer_append(&display, slash ? slash + 1 : app->attachments.items[i].path);
        }
        buffer_append(&display, "]");
    }

    /*
     * Build the request prompt from prior memory plus the current message.
     * The current user message is added to memory after capturing prior context
     * so the request does not duplicate it.
     */
    char *prior_context = context_join(app);

    char *ts = now_time_string();
    const char *user = getenv("USER");
    if (!user || !*user) user = "user";
    transcript_add(app, user, display.data, ts);
    append_log_entry(app->session_path, user, display.data, ts);
    char *ctx_user = ttychatter_strdup_printf("User: %s", display.data);
    context_add(app, ctx_user);
    free(ctx_user);
    free(ts);

    buffer_free(&app->input);
    buffer_init(&app->input);
    app->transcript_scroll = 0;
    set_status(app, "Generating response...");
    draw_main(app);

    char *prompt = NULL;
    if (*prior_context) prompt = ttychatter_strdup_printf("%s\nUser: %s", prior_context, trimmed);
    else prompt = ttychatter_strdup_printf("User: %s", trimmed);
    free(prior_context);

    char *raw = NULL;
    char *error = NULL;
    int rc = gemini_generate(app->cfg, config_effective_model(app->cfg), prompt, app->session_name, app->cfg->attachment_dir, &app->attachments, 120, &raw, &error);
    attachment_list_free(&app->attachments);
    attachment_list_init(&app->attachments);
    free(prompt);

    char *reply_ts = now_time_string();
    if (rc != 0) {
        const char *err = error ? error : "Gemini request failed.";
        transcript_add(app, "system", err, reply_ts);
        append_log_entry(app->session_path, "system", err, reply_ts);
        set_status(app, "Request failed.");
        free(error);
    } else {
        append_log_entry(app->session_path, "Gemini", raw, reply_ts);
        char *cleaned = extract_code_attachments(raw, app->cfg->attachment_dir, app->session_name);
        transcript_add(app, "Gemini", cleaned, reply_ts);
        char *ctx_assistant = ttychatter_strdup_printf("Assistant: %s", cleaned);
        context_add(app, ctx_assistant);
        append_context_snapshot(app->session_path, app->context, app->context_len);
        set_status(app, "Response complete.");
        free(ctx_assistant);
        free(cleaned);
        free(raw);
    }
    free(reply_ts);
    buffer_free(&display);
    free(message);
}

static void handle_key(App *app, int ch) {
    if (ch == KEY_F(1)) open_help(app);
    else if (ch == KEY_F(2)) open_models(app);
    else if (ch == KEY_F(3)) open_api_key(app);
    else if (ch == KEY_F(4)) open_sessions(app);
    else if (ch == KEY_F(5)) open_options(app);
    else if (ch == KEY_F(6)) open_memory(app);
    else if (ch == KEY_F(7)) open_file_menu(app);
    else if (ch == KEY_F(8)) open_editor(app);
    else if (ch == KEY_F(9)) open_credits(app);
    else if (ch == KEY_F(10)) app->should_quit = 1;
    else if (ch == 7) send_current_message(app);
    else if (ch == 17) app->should_quit = 1;
    else if (ch == KEY_PPAGE) app->transcript_scroll += 10;
    else if (ch == KEY_NPAGE) app->transcript_scroll -= 10;
    else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
        if (app->input.len > 0) app->input.data[--app->input.len] = '\0';
    } else if (ch == 21) {
        buffer_free(&app->input);
        buffer_init(&app->input);
        set_status(app, "Input cleared.");
    } else if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
        buffer_append(&app->input, "\n");
    } else if (ch == '\t') {
        buffer_append(&app->input, "    ");
    } else if (ch >= 32 && ch <= 126) {
        char c = (char)ch;
        buffer_append_n(&app->input, &c, 1);
    }
}

static void app_free(App *app) {
    free(app->session_name);
    free(app->session_path);
    attachment_list_free(&app->attachments);
    transcript_clear(app);
    context_clear(app);
    buffer_free(&app->input);
    free(app->status);
}

int run_curses_app(Config *cfg, const char *session_name, const char *session_path) {
    App app;
    memset(&app, 0, sizeof(app));
    app.cfg = cfg;
    app.session_name = xstrdup(session_name);
    app.session_path = xstrdup(session_path);
    attachment_list_init(&app.attachments);
    buffer_init(&app.input);
    app.status = xstrdup("F1 Help | Ctrl+G Send | F10 Quit");

    mkdir_p(cfg->session_dir);
    mkdir_p(cfg->attachment_dir);

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(1);
    load_session(&app);
    if (!cfg->api_key || !*cfg->api_key) set_status(&app, "No API key loaded. Press F3 or set GEMINI_API_KEY.");
    else if (!cfg->model || !*cfg->model) set_status(&app, "No MODEL configured. Press F2 to choose one.");

    while (!app.should_quit) {
        draw_main(&app);
        int ch = getch();
        handle_key(&app, ch);
    }
    endwin();
    app_free(&app);
    return 0;
}
