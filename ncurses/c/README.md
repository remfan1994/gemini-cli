# ttychatter

Native C/ncurses Gemini terminal client for ttychatter.

This is the C port of the Python ncurses reference implementation. It keeps the
same main product shape: full-screen terminal chat, Gemini REST transport,
model listing/cache, model testing, session logs, rolling context memory,
file attachments, generated-code extraction, editor integration, doctor checks,
and global function-key navigation.

## Dependencies

On Debian or Ubuntu style systems:

```sh
sudo apt-get install build-essential pkg-config libcurl4-openssl-dev libjson-c-dev libncurses-dev
```

Optional but useful:

```sh
sudo apt-get install gdb valgrind clang clang-format
```

## Build

```sh
make
```

Run:

```sh
./ttychatter
```

Install:

```sh
sudo make install
```

## Configuration

The C port uses XDG-style defaults.

Config file:

```text
$XDG_CONFIG_HOME/ttychatter/gemini/config
```

If `XDG_CONFIG_HOME` is unset, this becomes:

```text
~/.config/ttychatter/gemini/config
```

Data files:

```text
$XDG_DATA_HOME/ttychatter/gemini/sessions
$XDG_DATA_HOME/ttychatter/gemini/attachments
```

If `XDG_DATA_HOME` is unset, this becomes:

```text
~/.local/share/ttychatter/gemini/sessions
~/.local/share/ttychatter/gemini/attachments
```

Model cache:

```text
$XDG_CACHE_HOME/ttychatter/gemini/models.json
```

If `XDG_CACHE_HOME` is unset, this becomes:

```text
~/.cache/ttychatter/gemini/models.json
```

The config format is deliberately simple:

```text
KEY=VALUE
```

Useful keys:

```text
GEMINI_API_KEY=your_key_here
MODEL=gemini-2.5-flash
CONTEXT_TURNS=12
SESSION_DIR=~/.local/share/ttychatter/gemini/sessions
ATTACHMENT_DIR=~/.local/share/ttychatter/gemini/attachments
EDITOR=vi
MODEL_FILTER_GENERATE_CONTENT=1
MODEL_FILTER_REQUIRE_TOKENS=1
MODEL_FILTER_HIDE_PREVIEW=0
MODEL_MIN_INPUT_TOKENS=1
MODEL_MIN_OUTPUT_TOKENS=1
MODEL_SORT_ORDER=api
MAX_ATTACHMENT_BYTES=20971520
```

`GEMINI_API_KEY` in the environment overrides the config file. This lets you use
secrets without saving them to disk.

## CLI

```text
ttychatter [SESSION]
ttychatter --resume NAME
ttychatter --list
ttychatter --models
ttychatter --update-models
ttychatter --test-model MODEL
ttychatter --doctor
ttychatter --version
```

## Interactive keys

```text
F1   Help
F2   Models
F3   API key
F4   Sessions
F5   Options
F6   Memory
F7   Files
F8   External editor
F9   Credits
F10  Quit
Ctrl+G  Send current message
Ctrl+Q  Quit
PageUp/PageDown  Scroll transcript
Enter  Insert newline
Ctrl+U Clear input
```

## Sessions and attachments

Each session is a plain text log. Logs are intentionally readable and greppable.
After each assistant response, the current rolling context is saved as a JSON
snapshot inside the log. This lets resumed sessions recover useful memory
without replaying an entire old transcript.

When Gemini returns fenced code blocks, ttychatter saves them under the
attachment directory and replaces the block in the displayed transcript with a
file notice. This keeps code useful as ordinary filesystem objects.

## Notes for maintainers

The C implementation avoids SDK assumptions and talks to Gemini through REST.
That keeps behavior close to the Python and Bash variants, and makes request and
response handling visible in source.

Model selection is metadata-driven. The client lists models, caches the model
metadata, filters by capabilities/token limits, and lets the user test one
selected model. It does not automatically probe every available model.
