#!/usr/bin/env sh
# -----------------------------------------------------------------------------
# ttychatter local installer
# -----------------------------------------------------------------------------
# This installer is intentionally conservative.  The project can be used without
# it by running `chmod +x` on the desired executable and launching it directly.
# The installer exists for users who want commands and man pages placed in a
# standard user-local prefix such as ~/.local.
#
# Safety policy:
#   * install into ~/.local by default, never system directories unless asked
#   * never overwrite an existing command by default
#   * --force is required to overwrite an existing target file
#   * --check reports target paths and collisions without copying anything
#   * --name-prefix can reduce naming-collision risk
# -----------------------------------------------------------------------------

set -eu

PREFIX="${PREFIX:-$HOME/.local}"
NAME_PREFIX=""
INSTALL_BASH_ONLY=0
INSTALL_BASH_PYTHON3=0
INSTALL_NCURSES_PYTHON=0
INSTALL_ALL=1
DRY_RUN=0
FORCE=0
FORCE=0
CHECK_ONLY=0

usage() {
  cat <<EOF_USAGE
Usage: ./install.sh [options]

Options:
  --prefix PATH              Install under PATH instead of ~/.local
  --name-prefix PREFIX       Prefix installed command names, e.g. remfan-
  --all                      Install all current clients (default)
  --bash-only                Install only ttychatter-gemini-bash
  --bash-python3             Install only ttychatter-gemini-python3
  --ncurses-python           Install only ttychatter-gemini-ncurses-python
  --check                    Show targets and collision status, then exit
  --dry-run                  Print planned actions without copying files
  --force                    Overwrite existing installed files
  --force                    Allow overwriting existing target files
  -h, --help                 Show this help

Environment:
  PREFIX=PATH                Alternative way to set install prefix

Default install locations:
  executable files:          PREFIX/bin/
  man pages:                 PREFIX/share/man/man1/

Collision policy:
  Existing files are never overwritten unless --force is supplied.
EOF_USAGE
}

while [ "$#" -gt 0 ]; do
  case "$1" in
    --prefix)
      shift
      [ "${1:-}" ] || { echo "ERROR: --prefix requires a path" >&2; exit 2; }
      PREFIX="$1"
      ;;
    --name-prefix)
      shift
      [ "${1:-}" ] || { echo "ERROR: --name-prefix requires a value" >&2; exit 2; }
      NAME_PREFIX="$1"
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
    --check)
      CHECK_ONLY=1
      ;;
    --dry-run)
      DRY_RUN=1
      ;;
    --force)
      FORCE=1
      ;;
    --force)
      FORCE=1
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

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
BIN_DIR="$PREFIX/bin"
MAN_DIR="$PREFIX/share/man/man1"
CONFLICTS=0

run() {
  if [ "$DRY_RUN" -eq 1 ]; then
    printf 'DRY-RUN: %s\n' "$*"
  else
    "$@"
  fi
}

report_target() {
  src_exec="$1"
  dst_exec="$2"
  src_man="$3"
  dst_man="$4"

  printf 'command: %s -> %s/%s\n' "$src_exec" "$BIN_DIR" "$dst_exec"
  if [ -e "$BIN_DIR/$dst_exec" ]; then
    printf '  status: CONFLICT existing file\n'
    CONFLICTS=$((CONFLICTS + 1))
  else
    printf '  status: ok\n'
  fi

  printf 'manpage: %s -> %s/%s\n' "$src_man" "$MAN_DIR" "$dst_man"
  if [ -e "$MAN_DIR/$dst_man" ]; then
    printf '  status: CONFLICT existing file\n'
    CONFLICTS=$((CONFLICTS + 1))
  else
    printf '  status: ok\n'
  fi
}

install_pair() {
  src_exec="$1"
  dst_exec="$2"
  src_man="$3"
  dst_man="$4"

  [ -f "$src_exec" ] || { echo "ERROR: missing executable source: $src_exec" >&2; exit 1; }
  [ -f "$src_man" ] || { echo "ERROR: missing man page source: $src_man" >&2; exit 1; }

  if [ -e "$BIN_DIR/$dst_exec" ] && [ "$FORCE" -ne 1 ]; then
    echo "ERROR: target command already exists: $BIN_DIR/$dst_exec" >&2
    echo "Use --force to overwrite or --name-prefix to install under another name." >&2
    exit 1
  fi

  if [ -e "$MAN_DIR/$dst_man" ] && [ "$FORCE" -ne 1 ]; then
    echo "ERROR: target man page already exists: $MAN_DIR/$dst_man" >&2
    echo "Use --force to overwrite or --name-prefix to install under another name." >&2
    exit 1
  fi

  if [ "$FORCE" -ne 1 ]; then
    if [ -e "$BIN_DIR/$dst_exec" ]; then
      echo "ERROR: install target already exists: $BIN_DIR/$dst_exec" >&2
      echo "Use --force to overwrite, or install manually under a different name." >&2
      exit 1
    fi
    if [ -e "$MAN_DIR/$dst_man" ]; then
      echo "ERROR: man page target already exists: $MAN_DIR/$dst_man" >&2
      echo "Use --force to overwrite, or install manually under a different name." >&2
      exit 1
    fi
  fi

  run mkdir -p "$BIN_DIR" "$MAN_DIR"
  run cp "$src_exec" "$BIN_DIR/$dst_exec"
  run chmod 755 "$BIN_DIR/$dst_exec"
  run cp "$src_man" "$MAN_DIR/$dst_man"
}

handle_pair() {
  src_exec="$1"
  base_exec="$2"
  src_man="$3"
  base_man="$4"
  dst_exec="${NAME_PREFIX}${base_exec}"
  dst_man="${NAME_PREFIX}${base_man}"

  if [ "$CHECK_ONLY" -eq 1 ]; then
    report_target "$src_exec" "$dst_exec" "$src_man" "$dst_man"
  else
    install_pair "$src_exec" "$dst_exec" "$src_man" "$dst_man"
  fi
}

if [ "$INSTALL_BASH_ONLY" -eq 1 ]; then
  handle_pair \
    "$SCRIPT_DIR/gemini/bash/bash-only/ttychatter-gemini-bash" \
    "ttychatter-gemini-bash" \
    "$SCRIPT_DIR/gemini/bash/bash-only/ttychatter-gemini-bash.1" \
    "ttychatter-gemini-bash.1"
fi

if [ "$INSTALL_BASH_PYTHON3" -eq 1 ]; then
  handle_pair \
    "$SCRIPT_DIR/gemini/bash/python3/ttychatter-gemini-python3" \
    "ttychatter-gemini-python3" \
    "$SCRIPT_DIR/gemini/bash/python3/ttychatter-gemini-python3.1" \
    "ttychatter-gemini-python3.1"
fi

if [ "$INSTALL_NCURSES_PYTHON" -eq 1 ]; then
  handle_pair \
    "$SCRIPT_DIR/gemini/ncurses/python/ttychatter-gemini-ncurses-python" \
    "ttychatter-gemini-ncurses-python" \
    "$SCRIPT_DIR/gemini/ncurses/python/ttychatter-gemini-ncurses-python.1" \
    "ttychatter-gemini-ncurses-python.1"
fi

if [ "$CHECK_ONLY" -eq 1 ]; then
  if [ "$CONFLICTS" -gt 0 ]; then
    printf '\nConflicts detected: %s\n' "$CONFLICTS"
    printf 'Use --name-prefix or --force if appropriate.\n'
    exit 1
  fi
  printf '\nNo install conflicts detected.\n'
  exit 0
fi

cat <<EOF_DONE

Install complete.

Installed prefix:
  $PREFIX

Make sure this directory is in PATH:
  $BIN_DIR

Man pages were copied to:
  $MAN_DIR

API key setup is still required before normal use:
  export GEMINI_API_KEY="your_key"

or use the config file:
  ~/.config/ttychatter/gemini/config
EOF_DONE
