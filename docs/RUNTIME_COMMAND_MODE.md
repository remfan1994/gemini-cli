# Bash runtime command mode

The Bash implementations do not have ncurses function-key menus, mouse clicks, or full-screen modal screens.  To keep the Bash editions feature-equivalent without pretending they are ncurses applications, they provide runtime colon commands inside the running chat loop.

## Purpose

Runtime command mode exists so users can manage the client without quitting the chat.  It is the Bash equivalent of ncurses feature screens such as Models, Sessions, Options, Memory, Files, Editor, Credits, and Diagnostics.

This keeps the Bash editions scaled down in interface, not out of date in capability.

## How it works

During a running Bash chat session, the user types a line beginning with `:` and submits it with Ctrl+D.  The client recognizes the command locally.

Colon commands are not:

- sent to Gemini
- logged as user chat
- stored in conversation memory

This distinction is important because command mode is local client control, not part of the AI conversation.

## Common commands

    :help
    :rename new-session-name
    :models
    :update-models
    :select-model
    :test-model gemini-2.5-flash
    :model gemini-2.5-flash
    :model-save gemini-2.5-flash
    :config
    :set CONTEXT_TURNS 20
    :unset MODEL
    :set-api-key
    :forget-api-key
    :memory
    :edit-memory
    :clear-memory
    :attach /path/to/file.txt
    :attachments
    :editor
    :credits
    :doctor
    :quit

Some commands are stronger in the Python3 Bash edition than in the Bash-only edition.  The Bash-only edition should support as much as possible without introducing Python as a dependency.

## Sending literal colon messages

If a user wants to send a message to Gemini that begins with a colon, prefix the colon with a backslash:

    \:this message goes to Gemini

The client removes the backslash and sends the rest of the message normally.

## Design rule

Do not convert Bash into a fake full-screen TUI.  Bash gets command-mode verbs.  Ncurses gets menus, mouse, and function keys.
