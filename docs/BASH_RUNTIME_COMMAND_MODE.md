# Bash runtime command mode

The Bash clients do not have ncurses screens, function-key menus, or mouse support. To keep them feature-equivalent in a line-oriented form, both Bash editions use runtime colon-command mode.

This is the Bash equivalent of the ncurses F-key menu system.

## Design rule

A line beginning with `:` is a local client command.

It is not sent to Gemini, not logged as chat, and not stored in conversation memory.

A line beginning with `\:` is sent to Gemini literally, without the backslash.

This rule prevents accidental client-command leakage into the model prompt while still allowing users to talk about colon-prefixed strings.

## Why colon commands instead of key combinations?

The Bash clients gather multiline input through the terminal and send on Ctrl+D. Raw control-key handling is unreliable in this context, especially across older terminals, ChromeOS containers, SSH sessions, rescue shells, and terminal multiplexers.

Colon commands are portable and discoverable:

```text
:help
:models
:select-model
:memory
:quit
```

This keeps the Bash versions friendly to old systems without needing ncurses or readline.

## Command vocabulary

Common commands:

```text
:help
:quit
:credits
:doctor
```

Model commands:

```text
:models
:update-models
:select-model
:test-model MODEL_ID
:model MODEL_ID
:model-save MODEL_ID
```

Config commands:

```text
:config
:set KEY VALUE
:unset KEY
:set-api-key
:forget-api-key
```

Session commands:

```text
:rename NEW_NAME
```

Memory commands:

```text
:memory
:clear-memory
:edit-memory        # available in the Python3 Bash edition
```

File/editor commands:

```text
:attach FILE
:attachments
:editor
```

## Maintenance note

When adding a new ncurses feature, ask whether a line-oriented Bash equivalent belongs in runtime command mode.

Do not force full-screen behavior into Bash. Bash parity means capability parity, not UI parity.
