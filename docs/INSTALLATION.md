# Installation

This project currently ships three terminal clients:

```text
bash/bash-only/gemini-terminal
bash/python3/gemini-terminal-python3
ncurses/python/gemini-ncurses-python
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

### gemini-terminal

Dependency-light Bash client:

```text
bash
curl
common Unix tools: sed, awk, grep, tr, base64, mktemp
optional jq
```

### gemini-terminal-python3

Bash client with Python 3 helpers:

```text
bash
curl
python3
```

### gemini-ncurses-python

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
./install.sh --prefix /opt/gemini-terminal-tools
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
man gemini-terminal
man gemini-terminal-python3
man gemini-ncurses-python
```

## API key setup

Preferred runtime method:

```sh
export GEMINI_API_KEY="your_key"
```

Config-file method:

```text
~/.config/gemini-cli/config
```

Example:

```text
GEMINI_API_KEY=your_key
```

The config path still uses the old `gemini-cli` name for backward
compatibility with earlier releases.

## Verify install

```sh
gemini-terminal --version
gemini-terminal-python3 --version
gemini-ncurses-python --version
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
~/.config/gemini-cli/
~/.gpt/
```

That is intentional.  Config files, API keys, logs, and attachments are user
owned data and should not be deleted by a generic project uninstaller.
