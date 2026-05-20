# Runtime command parity

The Bash clients use runtime colon commands as the line-oriented equivalent of the Python ncurses function-key menus. A colon command is handled locally by the client and is not sent to Gemini, not stored as a user chat message, and not added to conversation memory.

To send a literal message beginning with a colon, prefix it with a backslash.

Example:

    \:this text is sent to Gemini

## Expected shared commands

The Bash/Python3 and Bash-only clients should keep the following commands aligned where practical:

    :help
    :models
    :update-models
    :select-model
    :test-model MODEL
    :model MODEL
    :model-save MODEL
    :config
    :set KEY VALUE
    :unset KEY
    :set-api-key
    :forget-api-key
    :memory
    :clear-memory
    :attach PATH
    :attachments
    :editor
    :credits
    :doctor
    :quit

## Commands that may differ

Some commands may be stronger in the Bash/Python3 edition because Python makes JSON, memory parsing, and attachment parsing more reliable.

Known acceptable differences should be documented in the man pages rather than hidden.

## Maintenance rule

When a new ncurses function-key screen is added, consider whether the Bash clients need an equivalent colon command.

When a new Bash colon command is added, consider whether the ncurses client needs an equivalent menu item or function key.
