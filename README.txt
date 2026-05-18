# gemini-terminal-tools

Terminal-first Gemini tools for Unix-like systems.

This project provides lightweight terminal interfaces for Google Gemini, built around plain text logs, local files, persistent sessions, model discovery, and artifact-aware workflows.

Repository:

    https://github.com/remfan1994/gemini-terminal-tools

If your repository URL differs, update this line before publishing.

---

## Project status

Current implementations:

    bash/gemini-cli
        Lightweight line-oriented Bash interface.

    ncurses/python/gemini-curses
        Full-screen Python ncurses interface.

Planned implementations:

    ncurses/c
        Future native C ncurses version.

The Bash version remains the minimal fallback and scriptable utility version.

The Python ncurses version is the current full-featured terminal UI.

---

## Philosophy

gemini-terminal-tools avoids:

- Electron
- browser dependency
- hidden databases
- opaque state
- heavy framework assumptions

The project favors:

- terminal-native interaction
- plain text logs
- user-owned files
- readable source code
- local session storage
- explicit configuration
- scriptability
- recoverable sessions
- artifact-oriented AI interaction

The goal is not only "chat in the terminal."

The goal is a terminal AI workspace.

---

## Repository layout

    bash/
        gemini-cli
        gemini-cli.1
        CHANGELOG.md

    ncurses/
        python/
            gemini-curses
            gemini-curses.1
            CHANGELOG.md

        c/
            planned native ncurses implementation

    README.md
    LICENSE.txt

---

## Shared configuration

Both current implementations use the same config file:

    ~/.config/gemini-cli/config

Example:

    MODEL=gemini-2.5-flash
    CONTEXT_TURNS=12

    SESSION_DIR=~/.gpt
    ATTACHMENT_DIR=~/.gpt/attachments

    GEMINI_API_KEY=your_api_key_here

Useful optional keys:

    EDITOR=vim
    INPUT_PLACEHOLDER=Type your chat message here...
    MONOCHROME=0
    STARTUP_NOTICE=1
    MODEL_TEST_PROMPT=Please reply with a short sentence confirming this model is available for text generation.
    MAX_ATTACHMENT_BYTES=1048576

Legacy key:

    HISTORY_LIMIT=12

HISTORY_LIMIT is still accepted as a legacy alias for CONTEXT_TURNS.

CONTEXT_TURNS is preferred because it describes what the setting actually does: it controls how many recent user/assistant exchanges are sent back to Gemini as conversation context.

---

## API key setup

Preferred method:

    export GEMINI_API_KEY="your_key"

Config-file method:

    GEMINI_API_KEY=your_key

Security note:

Saving an API key in the config file stores it in plaintext. The tools try to set private permissions when writing config files, but environment variables remain the safer default.

The ncurses version can accept a runtime API key through F3 without saving it.

---

## Model discovery

Google's model list can change over time.

The tools do not assume they permanently know the best model.

List models with:

    bash/gemini-cli --models

or:

    ncurses/python/gemini-curses --models

Raw Gemini API model-list command:

    curl "https://generativelanguage.googleapis.com/v1beta/models?key=YOUR_API_KEY"

---

## Model selection and testing

### Bash version

The Bash version supports:

    bash/gemini-cli --models
    bash/gemini-cli --select-model
    bash/gemini-cli --select-model --save
    bash/gemini-cli --autoscan-model
    bash/gemini-cli --autoscan-model --save

The Bash autoscan mode tests available models with a configured challenge and saves a working model if requested.

Default challenge:

    Prompt:
        What is the capital of Texas?

    Expected substring:
        austin

Autoscan finds a functional model. It does not guarantee the best model.

### ncurses Python version

The ncurses version removed autoscan.

Instead, it uses a user-operated model tester:

    ncurses/python/gemini-curses --test-model <model_id>

Inside the interface:

    F2
        Open model selector.

    t
        Test highlighted model.

    Enter
        Activate highlighted model.

    s
        Save highlighted model.

This avoids probing every model automatically. The user selects a model, tests it, reads the response, and decides whether to activate or save it.

---

## Bash interface

Path:

    bash/gemini-cli

Current release line:

    2.4

The Bash version is the lightweight, line-oriented edition.

It is designed for:

- constrained systems
- shell integration
- scriptability
- recoverability
- simple dependencies

Dependencies:

- bash
- curl
- python3

Main features:

- persistent sessions
- named sessions
- resumable sessions
- timestamped logs
- rolling conversation context
- resume-time context rebuild from logs
- config file support
- model listing
- model selection
- autoscan with answer validation
- API-key setup helper
- code block attachment extraction
- configurable session and attachment directories

Basic usage:

    bash/gemini-cli

Named session:

    bash/gemini-cli android-backup

Resume session:

    bash/gemini-cli --resume android-backup

List sessions:

    bash/gemini-cli --list

Set API key:

    bash/gemini-cli --set-api-key

Find and save a working model:

    bash/gemini-cli --autoscan-model --save

Choose a model manually:

    bash/gemini-cli --select-model

Input behavior:

    Type or paste your message.
    Press Ctrl+D to send.
    Press Ctrl+C to exit.

Limitations:

- no full-screen UI
- no ncurses-style editing
- no mouse support
- terminal line editing depends on the shell and terminal emulator

---

## Python ncurses interface

Path:

    ncurses/python/gemini-curses

Current release line:

    0.6.2

The Python ncurses version is the full-screen terminal interface.

It is designed for:

- multiline editing
- wrapped input
- scrollable transcript
- menu-driven interaction
- mousewheel scrolling
- clickable menus
- file browsing
- memory inspection
- model testing
- external editor support

Dependencies:

- python3
- curses support in Python

Main features:

- full-screen terminal UI
- scrollable transcript
- wrapped multiline input
- clickable command strip
- F1 actionable command menu
- F2 model selector
- model tester
- F3 API-key manager
- F4 session browser
- current-session rename
- F5 options menu
- F6 memory viewer/editor
- F7 file and attachment browser
- F8 external editor
- F9 credits/about
- F10 quit
- mousewheel scrolling
- click-based list selection
- click-based input cursor placement
- timestamped logs
- memory snapshots in session logs
- file attachment support
- generated inlineData attachment saving
- monochrome mode
- input placeholder
- startup project lead notice

Basic usage:

    ncurses/python/gemini-curses

Named session:

    ncurses/python/gemini-curses android-backup

Resume session:

    ncurses/python/gemini-curses --resume android-backup

List sessions:

    ncurses/python/gemini-curses --list

List models:

    ncurses/python/gemini-curses --models

Test one model:

    ncurses/python/gemini-curses --test-model gemini-2.5-flash

Controls:

    F1
        Help and command menu.

    Ctrl+G
        Send current message.

    F2
        Model selector and tester.

    F3
        API-key manager.

    F4
        Session browser.

    F5
        Options menu.

    F6
        Memory menu.

    F7
        File and attachment browser.

    F8
        External editor.

    F9
        Credits/about.

    F10 or Ctrl+Q
        Quit.

Mouse support:

- mousewheel scrolls transcript
- mousewheel scrolls menus/lists
- clicks select rows
- double-click activates where supported
- click in input area places cursor

Drag selection remains the terminal emulator's responsibility.

---

## Startup notice

The ncurses version can show a temporary startup message in the chat transcript for a new blank session.

The message appears as if sent by:

    remfan1994

Notice text:

    I strongly encourage everyone to get cruetly-free VEGETARIAN food and remember the 'bloodguilt' curse from the Bible.  -Project Lead, remfan1994

Behavior:

- shown only in new blank sessions
- removed when the user sends the first input
- not written to the session log
- not sent to Gemini
- not stored in memory

This can be controlled with:

    STARTUP_NOTICE=1

or:

    STARTUP_NOTICE=0

---

## Sessions

Sessions are stored as plain text logs.

Default location:

    ~/.gpt/

Example:

    ~/.gpt/android-backup.log

Configurable location:

    SESSION_DIR=/path/to/session/logs

Session logs are intended to be:

- readable
- grep-friendly
- backup-friendly
- editable if necessary
- recoverable without the program

---

## Conversation memory

The tools send recent conversation context back to Gemini with each request.

This is controlled by:

    CONTEXT_TURNS=12

Important distinction:

    session log
        Permanent archive.

    context memory
        Recent window sent back to Gemini.

The Bash version rebuilds recent context from existing session logs when resuming or reopening a named session.

The ncurses version stores memory snapshots in session logs and prefers the latest snapshot when resuming. It can also rebuild memory from recent chat entries.

---

## Attachments

### Code block extraction

When Gemini returns fenced markdown code blocks, the tools can extract those blocks into files.

Example model response:

    ```python
    print("hello")
    ```

Terminal display becomes:

    [attachment saved]
    file: session-python-01.py
    location: ~/.gpt/attachments

The raw Gemini response remains preserved in the session log.

### Attachment directory

Default:

    ~/.gpt/attachments

Configurable:

    ATTACHMENT_DIR=/path/to/attachments

### Local file attachments

The ncurses version supports attaching local files through F7.

Text-like files can be included as text.

Binary/media files can be sent as Gemini REST inlineData-style request parts where supported.

Compatibility depends on:

- selected model
- Gemini API support
- file MIME type
- file size
- current API limits

### Generated attachments

If Gemini returns inline data parts, the ncurses version attempts to save them into the attachment directory.

Generated image, video, and other media support depends on what the selected model actually returns.

---

## External editor support

The ncurses version supports external editor workflow through F8.

Editor selection order:

    1. EDITOR config key
    2. VISUAL environment variable
    3. EDITOR environment variable
    4. nano or vi if found

Example config:

    EDITOR=vim

This is intended to support mutt-style editing of longer prompts.

---

## Monochrome and display options

The ncurses version supports monochrome mode:

    MONOCHROME=1

Default:

    MONOCHROME=0

In normal mode:

- Gemini messages display normally
- user transcript messages may be dim/grey
- typed input remains normal/white
- placeholder text is dim/grey

In monochrome mode, the UI avoids relying on grey/dim styling for meaning.

Input placeholder:

    INPUT_PLACEHOLDER=Type your chat message here...

---

## Man pages

Each implementation includes its own man page.

Bash:

    bash/gemini-cli.1

Python ncurses:

    ncurses/python/gemini-curses.1

Example install location:

    ~/.local/share/man/man1/

Example:

    mkdir -p ~/.local/share/man/man1
    cp bash/gemini-cli.1 ~/.local/share/man/man1/
    cp ncurses/python/gemini-curses.1 ~/.local/share/man/man1/

Then:

    man gemini-cli
    man gemini-curses

---

## Changelogs

Each implementation maintains its own changelog.

    bash/CHANGELOG.md
    ncurses/python/CHANGELOG.md

The project tries to keep these synchronized with:

- source behavior
- man pages
- built-in help
- release changes

---

## Development notes

The project currently has three implementation tracks:

    bash
        Minimal, scriptable, line-oriented.

    ncurses/python
        Full-screen TUI, fast development, standard library only.

    ncurses/c
        Planned future native version.

Python ncurses is the current advanced UI path.

C ncurses is planned for a more traditional native Unix implementation.

---

## Future directions

Possible future work:

- native C ncurses version
- shared design notes for Bash/Python/C implementations
- shared test prompts
- attachment browser improvements
- transcript search
- model capability filtering
- token usage display
- richer media attachment handling
- plugin/helper scripts
- install scripts
- packaging
- shell completion

---

## Security notes

API keys are sensitive.

Preferred:

    export GEMINI_API_KEY="your_key"

Convenient but less secure:

    GEMINI_API_KEY=your_key

in:

    ~/.config/gemini-cli/config

Do not commit API keys.

Do not publish session logs containing secrets.

Do not publish attachments containing secrets.

---

## Known limitations

General:

- requires internet access
- API quota restrictions apply
- model availability can change
- file/media support depends on model/API capability

Bash:

- basic terminal input only
- Ctrl+D send workflow
- no full-screen UI
- no mouse support

ncurses/python:

- requires Python curses support
- mouse support depends on terminal emulator behavior
- drag selection belongs to the terminal emulator
- media attachment support depends on Gemini model/API support
- generated media attachments depend on what the model actually returns

---

## License

See:

    LICENSE.txt
