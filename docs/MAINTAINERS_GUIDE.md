# Maintainers Guide

This project is intended to be maintainable by strangers.  Source comments,
plain text documentation, and conservative behavior are part of the project
identity.  The goal is not only to make the tools work today, but to make them
understandable enough that future users can fork, repair, port, and extend the
project without needing the original development conversation.

## Project mission

The project mission is:

```text
robust user-friendly Gemini chat clients for the terminal
```

The project is not primarily a set of developer API examples.  Curl snippets may
appear in documentation for explanation, but each implementation track should be
a real end-user client.

## Implementation tracks

### bash/bash-only/gemini-terminal

Dependency-light Bash client.

Purpose:

```text
as many end-user features as possible without Python
```

This track is allowed to make compromises around JSON parsing and media support,
but it should still remain an actual interactive chat client.

### bash/python3/gemini-terminal-python3

Line-oriented Bash client with Python 3 helpers.

Purpose:

```text
practical shell client with reliable JSON/model/cache/memory helpers
```

Python helpers are used where shell code becomes fragile: JSON construction,
JSON parsing, URL quoting, attachment handling, and context parsing.

### ncurses/python/gemini-ncurses-python

Full-screen Python ncurses client.

Purpose:

```text
behavioral reference for future native C ncurses implementation
```

This is the richest current interface.  It should stabilize before the C version
begins.

### ncurses/c/gemini-ncurses

Planned native C implementation.

Purpose:

```text
traditional native Unix ncurses implementation after Python reference stabilizes
```

Do not begin this implementation until the Python ncurses branch has passed a
serious stabilization pass.

## Naming contracts

Current executable names:

```text
gemini-terminal
gemini-terminal-python3
gemini-ncurses-python
```

Future executable name:

```text
gemini-ncurses
```

Man page names should match executable names:

```text
gemini-terminal.1
gemini-terminal-python3.1
gemini-ncurses-python.1
gemini-ncurses.1
```

Avoid reintroducing the old `gemini-cli` executable name.  The old name remains
only in the shared config path:

```text
~/.config/gemini-cli/config
```

That path is retained for backward compatibility.

## Documentation synchronization rule

When a feature changes, update all applicable layers:

```text
source comments
built-in --help text
man page
implementation changelog
README or docs when project-level behavior changes
```

The code comments are not ornamental.  They are future-proofing.  Comments
should explain:

```text
what the code does
why the code exists
why this design was chosen
what assumptions it makes
what limitations maintainers should know
```

## Session and memory contract

Sessions are user-owned plain text logs.

Default session directory:

```text
~/.gpt
```

Conversation memory is not the same as the session log.  Memory/context is the
recent window sent back to Gemini.  The session log is the permanent archive.

Config key:

```text
CONTEXT_TURNS
```

Legacy alias:

```text
HISTORY_LIMIT
```

Do not use terminology that makes this sound like shell command history.

## Model list contract

Google model names and availability can change.

Use the model-list endpoint and metadata instead of hardcoding claims about the
best model:

```text
https://generativelanguage.googleapis.com/v1beta/models
```

The clients should prefer:

```text
list models
filter metadata
let the user choose
test one chosen model if requested
```

Avoid automatic probing of every model.  That workflow was intentionally removed
from the ncurses branch and replaced with user-operated model testing.

## API key contract

Preferred API-key source:

```sh
export GEMINI_API_KEY="your_key"
```

Config-file keys are convenient but plaintext:

```text
GEMINI_API_KEY=your_key
```

Never print API-key values in diagnostics.  If a command must mention API-key
state, say only whether a key is present.

## Attachment contract

Attachment support varies by implementation, but all tracks should preserve the
principle:

```text
terminal output should remain readable
large generated artifacts should become files
raw session logs should preserve original model output where practical
```

The ncurses/Python implementation currently has the richest file handling.
Bash-only support is necessarily more limited.

## C implementation readiness

Before starting the C version, the Python ncurses version should have:

```text
consistent navigation
stable options UI
stable model-cache workflow
stable attachment behavior
stable memory behavior
clear diagnostics
low-resolution behavior reviewed
known limitations documented
```

See:

```text
docs/PYTHON_NCURSES_STABILIZATION.md
docs/RELEASE_CANDIDATE_AUDIT.md
```

## Generated files

Never commit generated cache files:

```text
__pycache__/
*.pyc
```

Never commit user runtime data:

```text
.gpt/
session logs
attachments
API keys
```

The `.gitignore` exists to help, but maintainers should still inspect commits.
