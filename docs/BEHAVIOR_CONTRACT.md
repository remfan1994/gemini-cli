# ttychatter behavior contract

This document records the project behavior that maintainers should preserve across implementations.

The project now has multiple clients with different dependency profiles. They do not need identical user interfaces, but they should preserve the same user-facing concepts where practical. This document exists so the future C ncurses implementation can copy settled behavior instead of inventing a new application from scratch.

## Project mission

ttychatter exists to provide robust, user-friendly Gemini chat clients for the terminal.

The project is not a collection of curl examples. It is not primarily developer sample code. The intent is that users can conduct AI chat, manage sessions, attach files, select models, and preserve conversation history without relying on a browser interface.

## Implementation identities

Current executable family:

```text
ttychatter-gemini-bash              dependency-light Bash-only client
ttychatter-gemini-python3      Bash client with Python 3 helper reliability
ttychatter-gemini-ncurses-python        Python ncurses reference client
ttychatter-gemini-ncurses               planned native C ncurses client
```

The Python ncurses client is the current full-screen behavioral reference. The C ncurses client should not begin by adding new behavior. It should begin by copying stable behavior from the Python ncurses client.

## Shared config path

All current Gemini clients share the ttychatter provider config path:

```text
~/.config/ttychatter/gemini/config
```

The current provider-specific config path is `~/.config/ttychatter/gemini/config`.

## Shared storage concepts

The project uses plain files rather than a hidden database.

Shared storage concepts:

```text
SESSION_DIR       session logs
ATTACHMENT_DIR    extracted code blocks, saved inline data, local artifacts
model cache       cached model-list response or derived model metadata
```

The default session directory is normally:

```text
~/.gpt
```

The default attachment directory is normally:

```text
~/.gpt/attachments
```

## Model workflow

The settled model workflow is metadata-first and user-operated.

Preferred behavior:

```text
1. List or update model metadata using the Gemini models endpoint.
2. Filter models by metadata such as generateContent support and token limits.
3. Let the user choose a model.
4. Let the user test one chosen model.
5. Let the user activate or save the chosen model.
```

Avoid reintroducing automatic model probing. The old autoscan concept was removed because probing every available model was judged to be a worse workflow than model discovery plus user-operated testing.

## Model testing

Model testing should send one request to one selected model and display the response. The user decides if the model is acceptable.

The model tester should not silently save a model unless the user explicitly asks to save.

## Runtime command mode in Bash clients

Bash clients should provide local runtime commands with a colon prefix.

Colon commands are local client commands. They are not sent to Gemini, not logged as user chat, and not stored in context memory.

Examples:

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

A literal user message beginning with a colon should be escapable with a leading backslash:

```text
\:this is sent to Gemini
```

## Function-key navigation in ncurses

The ncurses clients should avoid screen traps.

Global function-key navigation is a project requirement. A user who accidentally opens F3 should be able to press F2, F4, F5, and so on without having to escape back to the main screen first.

The exact available keys may evolve, but the principle should remain:

```text
F1 help / command menu
F2 model tools
F3 API key tools
F4 session tools
F5 options
F6 memory
F7 files / attachments
F8 editor
F9 credits / about
F10 quit
```

## Project Lead startup notice

New blank sessions may show the temporary Project Lead notice from `remfan1994`.

This notice is UI text only. It must not be written to the session log, sent to Gemini, or stored in memory.

Expected behavior:

```text
- show only in new blank sessions
- remove when the user sends the first real message
- do not show when resuming a non-empty session
- do not include in model context
```

## Memory behavior

The project distinguishes permanent logs from model context.

```text
session log      permanent human-readable archive
context buffer   recent user/assistant entries resent to Gemini
```

`CONTEXT_TURNS` is the preferred config key. `HISTORY_LIMIT` is a legacy alias.

The context buffer should preserve recent conversation continuity without sending an unbounded full log every turn.

## Session logs

Session logs should remain human-readable plain text. Implementations may add system entries for context snapshots or metadata, but logs must remain useful to a person reading them directly.

## Attachments

The attachment system should preserve this distinction:

```text
raw Gemini response       kept in session log where practical
terminal display          may replace large code blocks or data with attachment notices
attachment files          saved in ATTACHMENT_DIR
```

Text attachments are the safest cross-implementation feature. Media attachment behavior depends on Gemini API/model support and should be documented as conditional.

## External editor behavior

The external editor concept is part of the project.

The preferred editor selection order is:

```text
1. EDITOR config key
2. VISUAL environment variable
3. EDITOR environment variable
4. common editor fallback such as nano or vi
```

Bash clients can expose editor behavior through runtime commands. Ncurses clients can expose it through F8.

## Diagnostics

Clients should expose diagnostics where practical.

Preferred command:

```text
--doctor
```

Diagnostics should never print API-key values. Diagnostics should separate local environment problems from Gemini API/network problems.

## C implementation posture

The C implementation should copy behavior rather than redesign it.

Before starting C, this contract should be reviewed against the Python ncurses behavior. Any C deviations should be deliberate and documented.
