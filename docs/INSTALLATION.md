# Installation

This project currently ships three terminal clients:

```text
gemini/bash/bash-only/ttychatter-gemini-bash
gemini/bash/python3/ttychatter-gemini-python3
gemini/ncurses/python/ttychatter-gemini-ncurses-python
```

The clients are intentionally installable without root.  The recommended local
install prefix is:

```text
~/.local
```

That gives:

```text
~/.local/bin/
~/.local/share/man/man1/
```

## Requirements

### ttychatter-gemini-bash

Dependency-light Bash client:

```text
bash
curl
common Unix tools: sed, awk, grep, tr, base64, mktemp
optional jq
```

### ttychatter-gemini-python3

Bash client with Python 3 helpers:

```text
bash
curl
python3
```

### ttychatter-gemini-ncurses-python

Full-screen ncurses client:

```text
python3
Python curses support
```

On many Linux systems, Python curses is already present.  On some minimal
systems it may be packaged separately.

## Install all clients

From the repository root:

```sh
./install.sh
```

This installs all current clients into:

```text
~/.local/bin
~/.local/share/man/man1
```

## Install selected clients

```sh
./install.sh --bash-only
./install.sh --bash-python3
./install.sh --ncurses-python
```

A custom prefix may be supplied:

```sh
./install.sh --prefix /opt/ttychatter
```

A dry run is available:

```sh
./install.sh --dry-run
```

## PATH setup

If `~/.local/bin` is not already in your PATH, add this to `~/.bashrc`:

```sh
export PATH="$HOME/.local/bin:$PATH"
```

Then reload the shell:

```sh
. ~/.bashrc
```

## Man page setup

If your `man` command cannot find user-installed man pages, add this to
`~/.bashrc`:

```sh
export MANPATH="$HOME/.local/share/man:${MANPATH:-}"
```

Then try:

```sh
man ttychatter-gemini-bash
man ttychatter-gemini-python3
man ttychatter-gemini-ncurses-python
```

## API key setup

Preferred runtime method:

```sh
export GEMINI_API_KEY="your_key"
```

Config-file method:

```text
~/.config/ttychatter/gemini/config
```

Example:

```text
GEMINI_API_KEY=your_key
```

The config path still uses the old `ttychatter-gemini-python3` name for backward
compatibility with earlier releases.

## Verify install

```sh
ttychatter-gemini-bash --version
ttychatter-gemini-python3 --version
ttychatter-gemini-ncurses-python --version
```

A repository-local smoke test is available before or after install:

```sh
scripts/smoke-test.sh
```

The smoke test avoids network calls and does not require an API key.

## Uninstall

```sh
./uninstall.sh
```

The uninstaller removes installed executables and man pages only.  It does not
remove user data:

```text
~/.config/ttychatter/gemini/
~/.gpt/
```

That is intentional.  Config files, API keys, logs, and attachments are user
owned data and should not be deleted by a generic project uninstaller.
