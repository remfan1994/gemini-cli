# Python ncurses stabilization checklist

The Python ncurses implementation is the reference for the future C version.  Before the C implementation begins, the Python client should be stable enough that the C version can copy behavior rather than invent it.

## Required checks

- `gemini-ncurses-python --version`
- `gemini-ncurses-python --help`
- `gemini-ncurses-python --doctor`
- `gemini-ncurses-python --models` with cache present
- `gemini-ncurses-python --update-models` online
- `gemini-ncurses-python --test-model MODEL`

## UI checks

- F1 command menu opens and launches commands.
- F2 model menu does not contact Google until user chooses update/load behavior.
- F3 API-key manager does not save secrets by default.
- F4 session browser opens sessions and renames current session.
- F5 options form saves and cancels predictably.
- F6 memory viewer/editor handles clear/rebuild/edit.
- F7 files/attachments browser handles missing directories and size limits.
- F8 editor menu detects common editors and allows manual editor path.
- F9 credits/about displays and wraps properly.
- Function keys remain global across feature screens where practical.
- Mousewheel scrolls transcript/lists.
- Drag selection remains terminal-emulator behavior, not application behavior.

## Low-resolution checks

- Header does not make the program unusable.
- Footer/command strip does not hide critical information.
- Long text wraps.
- Dialogs become scrollable pages when needed.
- Status messages do not overwrite modal content.

## Error checks

- Offline/DNS failure produces a readable error screen.
- Missing API key is clear and recoverable.
- Corrupt model cache does not crash the UI.
- Bad config values fall back safely or produce actionable diagnostics.
- Missing editor is reported clearly.
- Oversized attachment is refused clearly.

## Source comments

The source should remain intentionally verbose.  Comments are part of project survivability: future maintainers, forkers, and C-port authors should be able to understand not only what the code does, but why the behavior exists.

## Cross-implementation parity checks

The Bash editions should not copy ncurses menus, but they should preserve practical capabilities through runtime colon commands.

Before starting the C version, verify that:

- `gemini-terminal-python3` has runtime colon commands documented in help/man page.
- `gemini-terminal` has runtime colon commands documented in help/man page.
- Bash command mode can rename sessions, inspect config, list/test/select models, inspect memory, use editor workflows where supported, and show diagnostics.
- Bash-only limitations are intentional, documented, and caused by dependency constraints rather than neglect.
