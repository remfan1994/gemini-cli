# Release candidate audit notes

This document tracks the project areas that should be checked before treating the Python ncurses implementation as stable enough to guide the future C ncurses implementation.

## Current naming

Executables:

```text
gemini-terminal
gemini-terminal-python3
gemini-ncurses-python
gemini-ncurses        # future C version
```

Man pages:

```text
gemini-terminal.1
gemini-terminal-python3.1
gemini-ncurses-python.1
gemini-ncurses.1      # future C version
```

Config path:

```text
~/.config/gemini-cli/config
```

The config path intentionally keeps the historical `gemini-cli` name for backward compatibility.

## Stabilization checklist

### Python ncurses

Check:

```text
--version
--help
--doctor
--models
--update-models
--test-model MODEL
```

In-app checks:

```text
F1 command menu
F2 model tools
F3 API key manager
F4 session browser
F5 options form
F6 memory menu
F7 file/attachment menu
F8 editor menu
F9 credits/about
F10 quit
Ctrl+G send
mousewheel transcript scroll
click command strip
low-resolution display
```

Key expectations:

```text
- no screen traps
- function keys route globally where practical
- F2 does not contact Google until user requests update/list behavior
- model cache survives restart
- options form validates values before writing config
- startup notice is temporary and not logged
- memory snapshots do not corrupt session logs
- attachment failures produce readable errors
```

### Bash/Python3

Check:

```text
--version
--help
--doctor
--models
--update-models
--select-model
--test-model MODEL
--config
--set KEY VALUE
--unset KEY
```

Runtime commands:

```text
:help
:models
:update-models
:select-model
:test-model MODEL
:config
:set KEY VALUE
:unset KEY
:memory
:attach FILE
:editor
:credits
:doctor
:quit
```

### Bash-only

Check:

```text
--version
--help
--doctor
--models
--update-models
--select-model
--test-model MODEL
```

Runtime commands should match the Bash/Python3 version where feasible. Differences should be documented as dependency limitations, not accidental omissions.

## Before starting C

Do not start the C implementation until:

```text
- Python ncurses global navigation is stable
- model-cache behavior is stable
- config keys are settled
- session log format is settled
- memory snapshot format is settled
- attachment conventions are settled
- Project Lead startup notice behavior is settled
- Bash runtime command mode has stabilized
```

The C version should copy behavior, not invent new behavior first.
