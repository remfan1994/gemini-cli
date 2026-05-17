# gemini-terminal-tools

A collection of terminal-first Gemini AI interfaces and utilities designed around Unix-style workflows, persistent sessions, filesystem-oriented storage, and minimal dependencies.

The project currently includes:

- a lightweight Bash interface
- a full-screen ncurses interface
- shared configuration philosophy
- shared session storage model
- shared attachment extraction system

---

# Philosophy

This project intentionally avoids:

- Electron
- browser dependency
- heavy frameworks
- opaque storage systems
- hidden state

Instead, the project emphasizes:

- plain text logs
- filesystem persistence
- readable source code
- terminal-native interaction
- lightweight tooling
- recoverable sessions
- artifact-oriented AI workflows

The goal is not merely "AI chat in terminal."

The goal is a reusable terminal AI workspace.

---

# Repository Structure

```text
bash/
    gemini-cli
    gemini-cli.1
    CHANGELOG.md

ncurses/
    gemini-curses
    gemini-curses.1
    CHANGELOG.md

README.md
LICENSE.txt
Implementations
Bash Version (bash/gemini-cli)

The Bash version is the lightweight, scriptable edition.

It is designed for:

constrained systems
shell integration
recoverability
simplicity
minimal runtime overhead
Features
persistent sessions
named/resumable chats
timestamped logs
rolling conversational memory
attachment extraction
model discovery
model autoscan
numbered model selection
API-key setup helper
config-file support
Dependencies
bash
curl
python3
Strengths
highly portable
easy to inspect
shell-friendly
easy to script around
Limitations
basic terminal input handling
Ctrl+D send workflow
no full-screen UI
no readline-level editing
ncurses Version (ncurses/gemini-curses)

The ncurses version is the full-screen terminal interface.

It exists because shell input handling eventually becomes insufficient for:

multiline editing
wrapped input
cursor management
scrolling transcripts
modal interaction
model selection UX
Features
full-screen curses UI
wrapped multiline input
scrollable transcript
named/resumable sessions
rolling conversational memory
attachment extraction
in-app model selector (F2)
in-app model autoscan (F4)
in-app API-key entry (F3)
transcript replay on resume
model metadata viewer
Dependencies
python3
curses (standard Python module)
Strengths
proper terminal interaction
better readability
cleaner workflow
better future expansion path
Limitations
larger than Bash version
still evolving
no mouse support yet
Shared Concepts
Session Model

Sessions are stored as plain text logs:

SESSION_DIR/<session>.log

Sessions are:

resumable
human-readable
easy to back up
grep-friendly
Attachment System

When Gemini returns fenced markdown code blocks, they are:

extracted from terminal output
saved as files
replaced with attachment notices

Example:

[attachment saved]
file: linux-help-python-01.py

The original raw Gemini output remains preserved in the session log.

Model Discovery

Google's model list changes over time.

This project intentionally avoids pretending to permanently know the "best" model.

Available models can be listed with:

gemini-cli --models
gemini-curses --models

Or directly from the Gemini API:

curl "https://generativelanguage.googleapis.com/v1beta/models?key=YOUR_API_KEY"
Model Autoscan

Autoscan exists for users who do not know which model to choose.

Autoscan:

fetches the live Gemini model list
filters to generateContent-capable models
tests candidates with a tiny prompt
activates the first model that successfully returns text

This avoids relying on hardcoded assumptions about future Google model names.

Examples:

gemini-cli --autoscan-model
gemini-curses --autoscan-model
Configuration

Shared config file:

~/.config/gemini-cli/config

Example:

MODEL=gemini-2.5-flash
HISTORY_LIMIT=12

SESSION_DIR=~/.gpt
ATTACHMENT_DIR=~/.gpt/attachments

GEMINI_API_KEY=your_api_key_here

Environment variables override config-file API keys:

export GEMINI_API_KEY="your_key"
Security Notes

Saving API keys in the config file stores them in plaintext.

Environment variables are preferred when possible.

The tools attempt to chmod the config file to 0600 when writing sensitive values.

Documentation

Each implementation maintains synchronized:

source comments
built-in help
man page
changelog

Feature additions are intended to update all four layers together.

Future Directions

Planned or possible future work:

native C ncurses version
streaming output
mouse support
transcript search
attachment browser
slash commands
model capability filters
token usage statistics
plugin/helper utilities
shared core library
License

See LICENSE.txt.

Project Status

Active experimental terminal AI workspace project.

The Bash edition is currently the lightweight stable interface.

The ncurses edition is the primary future-facing interface.
