#!/usr/bin/env sh
# -----------------------------------------------------------------------------
# gemini-terminal-tools local installer
# -----------------------------------------------------------------------------
# This installer is intentionally conservative.  It installs the project into a
# user-controlled prefix, defaulting to ~/.local, and does not require root.
#
# It copies the three current executable tracks:
#   gemini-terminal          -> Bash-only dependency-light client
#   gemini-terminal-python3  -> Bash client with Python 3 helpers
#   gemini-ncurses-python    -> Python ncurses full-screen client
#
# The installer also installs matching man pages.  It does not write API keys,
# create config files, or touch user session logs.  Runtime configuration remains
# the user's responsibility through ~/.config/gemini-cli/config or environment
# variables.
# -----------------------------------------------------------------------------

set -eu

PREFIX="${PREFIX:-$HOME/.local}"
INSTALL_BASH_ONLY=0
INSTALL_BASH_PYTHON3=0
INSTALL_NCURSES_PYTHON=0
INSTALL_ALL=1
DRY_RUN=0

usage() {
  cat <<EOF_USAGE
Usage: ./install.sh [options]

Options:
  --prefix PATH              Install under PATH instead of ~/.local
  --all                      Install all current clients (default)
  --bash-only                Install only gemini-terminal
  --bash-python3             Install only gemini-terminal-python3
  --ncurses-python           Install only gemini-ncurses-python
  --dry-run                  Print planned actions without copying files
  -h, --help                 Show this help

Environment:
  PREFIX=PATH                Alternative way to set install prefix

Default install locations:
  executable files:          PREFIX/bin/
  man pages:                 PREFIX/share/man/man1/
EOF_USAGE
}

while [ "$#" -gt 0 ]; do
  case "$1" in
    --prefix)
      shift
      [ "${1:-}" ] || { echo "ERROR: --prefix requires a path" >&2; exit 2; }
      PREFIX="$1"
      ;;
    --all)
      INSTALL_ALL=1
      ;;
    --bash-only)
      INSTALL_ALL=0
      INSTALL_BASH_ONLY=1
      ;;
    --bash-python3)
      INSTALL_ALL=0
      INSTALL_BASH_PYTHON3=1
      ;;
    --ncurses-python)
      INSTALL_ALL=0
      INSTALL_NCURSES_PYTHON=1
      ;;
    --dry-run)
      DRY_RUN=1
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "ERROR: unknown option: $1" >&2
      usage >&2
      exit 2
      ;;
  esac
  shift
done

if [ "$INSTALL_ALL" -eq 1 ]; then
  INSTALL_BASH_ONLY=1
  INSTALL_BASH_PYTHON3=1
  INSTALL_NCURSES_PYTHON=1
fi

# Resolve the repository root from the install script location.  This lets users
# run the script from any current working directory.
SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
BIN_DIR="$PREFIX/bin"
MAN_DIR="$PREFIX/share/man/man1"

run() {
  if [ "$DRY_RUN" -eq 1 ]; then
    printf 'DRY-RUN: %s\n' "$*"
  else
    "$@"
  fi
}

install_pair() {
  src_exec="$1"
  dst_exec="$2"
  src_man="$3"
  dst_man="$4"

  if [ ! -f "$src_exec" ]; then
    echo "ERROR: missing executable source: $src_exec" >&2
    exit 1
  fi
  if [ ! -f "$src_man" ]; then
    echo "ERROR: missing man page source: $src_man" >&2
    exit 1
  fi

  run mkdir -p "$BIN_DIR" "$MAN_DIR"
  run cp "$src_exec" "$BIN_DIR/$dst_exec"
  run chmod 755 "$BIN_DIR/$dst_exec"
  run cp "$src_man" "$MAN_DIR/$dst_man"
}

if [ "$INSTALL_BASH_ONLY" -eq 1 ]; then
  install_pair \
    "$SCRIPT_DIR/bash/bash-only/gemini-terminal" \
    "gemini-terminal" \
    "$SCRIPT_DIR/bash/bash-only/gemini-terminal.1" \
    "gemini-terminal.1"
fi

if [ "$INSTALL_BASH_PYTHON3" -eq 1 ]; then
  install_pair \
    "$SCRIPT_DIR/bash/python3/gemini-terminal-python3" \
    "gemini-terminal-python3" \
    "$SCRIPT_DIR/bash/python3/gemini-terminal-python3.1" \
    "gemini-terminal-python3.1"
fi

if [ "$INSTALL_NCURSES_PYTHON" -eq 1 ]; then
  install_pair \
    "$SCRIPT_DIR/ncurses/python/gemini-ncurses-python" \
    "gemini-ncurses-python" \
    "$SCRIPT_DIR/ncurses/python/gemini-ncurses-python.1" \
    "gemini-ncurses-python.1"
fi

cat <<EOF_DONE

Install complete.

Installed prefix:
  $PREFIX

Make sure this directory is in PATH:
  $BIN_DIR

For Bash users, add this to ~/.bashrc if needed:
  export PATH="\$HOME/.local/bin:\$PATH"

Man pages were copied to:
  $MAN_DIR

If man cannot find them, add this to ~/.bashrc if needed:
  export MANPATH="\$HOME/.local/share/man:\${MANPATH:-}"

API key setup is still required before normal use:
  export GEMINI_API_KEY="your_key"

or use the config file:
  ~/.config/gemini-cli/config
EOF_DONE
