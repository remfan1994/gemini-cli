# gemini-terminal-tools

User-friendly Gemini chat clients for the text-based terminal.

Project focus: browsers and browser interfaces are not necessary for sending text messages and attachments back and forth with an AI. The terminal can provide a robust, end-user chat client with sessions, memory, file attachments, model discovery, and readable local storage.

Repository:

    https://github.com/remfan1994/gemini-terminal-tools

---

## Implementations

    bash/python3/
        Practical line-oriented Bash client using Python 3 helper snippets.

    bash/bash-only/
        Dependency-light Bash client avoiding Python.

    ncurses/python/
        Full-screen Python ncurses client and current UI reference.

    ncurses/c/
        Announced future native C ncurses implementation.

---

## Philosophy

This project is not a collection of curl examples. It is a set of end-user terminal chat clients.

The project favors:

- terminal-native workflows
- plain text sessions
- local file ownership
- explicit configuration
- model discovery through metadata
- user-operated model testing
- attachment-aware AI interaction
- verbose source comments for maintainability
- open source forkability

---

## Shared config

Current implementations share:

    ~/.config/gemini-cli/config

Common keys:

    MODEL=gemini-2.5-flash
    CONTEXT_TURNS=12
    SESSION_DIR=~/.gpt
    ATTACHMENT_DIR=~/.gpt/attachments
    GEMINI_API_KEY=your_key_here

The Bash/python3 and ncurses/python editions support the richest config set. The Bash-only edition supports the core subset and makes a best effort with fewer dependencies.

---

## Model workflow

The project avoids automatic probing of every model.

Preferred workflow:

1. update or load the model list
2. filter by metadata such as generateContent support and token limits
3. choose a model
4. test the selected model
5. save the model if desired

Useful commands:

    bash/python3/gemini-cli --update-models
    bash/python3/gemini-cli --models
    bash/python3/gemini-cli --select-model

    bash/bash-only/gemini-cli --update-models
    bash/bash-only/gemini-cli --models
    bash/bash-only/gemini-cli --select-model

    ncurses/python/gemini-curses --update-models
    ncurses/python/gemini-curses --models
    ncurses/python/gemini-curses --test-model gemini-2.5-flash

Run diagnostics:

    ncurses/python/gemini-curses --doctor

Raw Gemini model endpoint:

    https://generativelanguage.googleapis.com/v1beta/models?key=YOUR_API_KEY

---

## Bash/python3 edition

Path:

    bash/python3/gemini-cli

This is the practical shell client. Bash handles the interface and workflow. Python 3 helper snippets handle JSON, URL escaping, attachment extraction, and log parsing.

Dependencies:

- bash
- curl
- python3

---

## Bash-only edition

Path:

    bash/bash-only/gemini-cli

This is the dependency-light shell client. It avoids Python while trying to remain an actual end-user chat application rather than a developer curl wrapper.

Required dependencies:

- bash
- curl
- awk
- sed
- grep
- tr
- mktemp
- base64

Optional dependency:

- jq

If jq is installed, JSON parsing and model metadata handling are stronger. Without jq, the script uses best-effort awk/sed parsing.

---

## Python ncurses edition

Path:

    ncurses/python/gemini-curses

This is the full-screen terminal UI and current behavioral reference implementation.

Features include:

- scrollable transcript
- multiline input
- clickable command strip
- function-key screens
- global function-key navigation
- model cache/filter UI
- user-operated model testing
- API key manager
- session browser
- options form
- memory viewer/editor
- file attachment menu
- external editor menu
- credits/about screen

---

## Future C ncurses edition

Path:

    ncurses/c/ANNOUNCED.md

The C implementation is announced but intentionally postponed until the Python ncurses edition stabilizes enough to serve as the reference behavior.

---

## Documentation discipline

Each maintained implementation should keep these synchronized:

- source comments
- built-in help
- man page
- changelog
- README when project-level behavior changes

Verbose comments are part of the project’s open-source maintainability posture.

---

## License

See LICENSE.txt.
