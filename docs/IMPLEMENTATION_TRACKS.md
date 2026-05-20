# Implementation tracks

This project intentionally carries several terminal clients with different dependency profiles. They should share the same user-facing mission while accepting different implementation tradeoffs.

## bash/bash-only/gemini-terminal

The Bash-only edition is the dependency-light terminal client. It avoids Python and aims to work on older or smaller systems that still have Bash and curl available. It should remain an end-user chat client, not a curl example.

Expected posture:

- keep dependencies low
- preserve interactive chat behavior
- provide config, sessions, logs, runtime colon commands, model listing/testing, and basic attachments where possible
- use optional helper tools when present, but do not make Python required

## bash/python3/gemini-terminal-python3

The Bash/Python3 edition is the practical shell client. Bash controls the interactive workflow while Python 3 handles JSON, parsing, escaping, and other tasks where pure shell code becomes fragile.

Expected posture:

- keep a line-oriented terminal workflow
- use Python 3 helpers for reliability
- stay feature-equivalent with the ncurses client where a line-oriented interface makes sense
- avoid ncurses-style UI behavior

## ncurses/python/gemini-ncurses-python

The Python ncurses edition is the full-screen reference implementation. It is the behavioral prototype for the future C ncurses version.

Expected posture:

- provide full-screen UI behavior
- stabilize global function-key navigation, mousewheel/click behavior, model selection, sessions, attachments, memory, and diagnostics
- serve as the design reference for the C implementation

## ncurses/c/gemini-ncurses

The native C version is planned but should not be started until the Python ncurses edition is stable enough to act as a reference implementation.
