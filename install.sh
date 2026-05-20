#!/usr/bin/env sh
# -----------------------------------------------------------------------------
# ttychatter local installer
# -----------------------------------------------------------------------------
# This installer is intentionally conservative because command-name collisions
# are a real risk for small terminal utilities.  The ttychatter project can be
# used without this installer: users may simply run `chmod +x` on the desired
# executable and launch it directly from the repository checkout.
#
# The installer exists only for users who want commands and man pages copied to
# a normal user-local prefix such as ~/.local.  It follows these safety rules:
#
#   * install into ~/.local by default, never system directories unless asked
#   * never overwrite an existing target file unless --force is supplied
#   * report command-name collisions already present in PATH
#   * refuse to shadow an existing PATH command unless --allow-shadow is given
#   * provide --check and --list-targets modes before any file is copied
#   * support --name-prefix so users can install under names like remfan-...
#
# The point is to make installation convenient without behaving like an
# aggressive package manager.  If a user is unsure, `./install.sh --check --all`
# should be safe and informative.
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
CHECK_ONLY=0
LIST_TARGETS=0
ALLOW_SHADOW=0
CONFLICTS=0
WARNINGS=0

usage() {
  cat <<EOF_USAGE
Usage: ./install.sh [options]

Client selection:
  --all                      Install all current clients (default)
  --bash-only                Install only ttychatter-gemini-bash
  --bash-python3             Install only ttychatter-gemini-python3
  --ncurses-python           Install only ttychatter-gemini-ncurses-python

Install destination:
  --prefix PATH              Install under PATH instead of ~/.local
  --name-prefix PREFIX       Prefix installed command/man names, e.g. remfan-

Safety / inspection:
  --check                    Show targets and collision status, then exit
  --list-targets             Print target paths only, then exit
  --dry-run                  Print planned copy commands without copying files
  --force                    Overwrite existing target files in the prefix
  --allow-shadow             Allow installing a command name already found in PATH
  -h, --help                 Show this help

Environment:
  PREFIX=PATH                Alternative way to set install prefix

Default install locations:
  executable files:          PREFIX/bin/
  man pages:                 PREFIX/share/man/man1/

Collision policy:
  Existing target files are never overwritten unless --force is supplied.
  Existing commands found elsewhere in PATH are treated as collisions unless
  --allow-shadow is supplied.

Manual use without installer:
  chmod +x bash/bash-only/ttychatter-gemini-bash
  ./bash/bash-only/ttychatter-gemini-bash --help
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
    --list-targets)
      LIST_TARGETS=1
      ;;
    --dry-run)
      DRY_RUN=1
      ;;
    --force)
      FORCE=1
      ;;
    --allow-shadow)
      ALLOW_SHADOW=1
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

run() {
  if [ "$DRY_RUN" -eq 1 ]; then
    printf 'DRY-RUN: %s\n' "$*"
  else
    "$@"
  fi
}

canonical_path() {
  # Use Python if present because it is the most reliable way to canonicalize
  # paths portably.  If Python is unavailable, fall back to the raw path.  The
  # installer itself is shell-only; this helper is only a best-effort aid for
  # comparing PATH collisions.
  if command -v python3 >/dev/null 2>&1; then
    python3 -c 'import os, sys; print(os.path.realpath(sys.argv[1]))' "$1"
  else
    printf '%s\n' "$1"
  fi
}

path_command_location() {
  # POSIX command -v is enough to determine whether a command name already has
  # meaning for the user's shell.  That is what matters for collision risk.
  command -v "$1" 2>/dev/null || true
}

record_conflict() {
  CONFLICTS=$((CONFLICTS + 1))
}

record_warning() {
  WARNINGS=$((WARNINGS + 1))
}

report_target() {
  src_exec="$1"
  dst_exec="$2"
  src_man="$3"
  dst_man="$4"

  exec_target="$BIN_DIR/$dst_exec"
  man_target="$MAN_DIR/$dst_man"
  existing_cmd="$(path_command_location "$dst_exec")"

  printf 'command: %s -> %s\n' "$src_exec" "$exec_target"

  if [ ! -f "$src_exec" ]; then
    printf '  status: ERROR source executable missing\n'
    record_conflict
  elif [ -e "$exec_target" ]; then
    printf '  status: CONFLICT target file exists\n'
    record_conflict
  elif [ -n "$existing_cmd" ]; then
    existing_real="$(canonical_path "$existing_cmd")"
    target_real="$(canonical_path "$exec_target")"
    if [ "$existing_real" = "$target_real" ]; then
      printf '  status: ok (same target already in PATH location)\n'
    else
      printf '  status: PATH-COLLISION existing command at %s\n' "$existing_cmd"
      if [ "$ALLOW_SHADOW" -eq 1 ]; then
        record_warning
      else
        record_conflict
      fi
    fi
  else
    printf '  status: ok\n'
  fi

  printf 'manpage: %s -> %s\n' "$src_man" "$man_target"
  if [ ! -f "$src_man" ]; then
    printf '  status: ERROR source man page missing\n'
    record_conflict
  elif [ -e "$man_target" ]; then
    printf '  status: CONFLICT target file exists\n'
    record_conflict
  else
    printf '  status: ok\n'
  fi
}

list_target() {
  src_exec="$1"
  dst_exec="$2"
  src_man="$3"
  dst_man="$4"
  printf '%s -> %s\n' "$src_exec" "$BIN_DIR/$dst_exec"
  printf '%s -> %s\n' "$src_man" "$MAN_DIR/$dst_man"
}

assert_safe_target() {
  dst_exec="$1"
  dst_man="$2"
  exec_target="$BIN_DIR/$dst_exec"
  man_target="$MAN_DIR/$dst_man"
  existing_cmd="$(path_command_location "$dst_exec")"

  if [ -e "$exec_target" ] && [ "$FORCE" -ne 1 ]; then
    echo "ERROR: target command already exists: $exec_target" >&2
    echo "Use --force to overwrite or --name-prefix to install under another name." >&2
    exit 1
  fi

  if [ -e "$man_target" ] && [ "$FORCE" -ne 1 ]; then
    echo "ERROR: target man page already exists: $man_target" >&2
    echo "Use --force to overwrite or --name-prefix to install under another name." >&2
    exit 1
  fi

  if [ -n "$existing_cmd" ]; then
    existing_real="$(canonical_path "$existing_cmd")"
    target_real="$(canonical_path "$exec_target")"
    if [ "$existing_real" != "$target_real" ] && [ "$ALLOW_SHADOW" -ne 1 ]; then
      echo "ERROR: command name already exists in PATH: $dst_exec" >&2
      echo "Existing command: $existing_cmd" >&2
      echo "Use --name-prefix to avoid collision, or --allow-shadow if you really intend to shadow it." >&2
      exit 1
    fi
  fi
}

install_pair() {
  src_exec="$1"
  dst_exec="$2"
  src_man="$3"
  dst_man="$4"

  [ -f "$src_exec" ] || { echo "ERROR: missing executable source: $src_exec" >&2; exit 1; }
  [ -f "$src_man" ] || { echo "ERROR: missing man page source: $src_man" >&2; exit 1; }

  assert_safe_target "$dst_exec" "$dst_man"

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

  if [ "$LIST_TARGETS" -eq 1 ]; then
    list_target "$src_exec" "$dst_exec" "$src_man" "$dst_man"
  elif [ "$CHECK_ONLY" -eq 1 ]; then
    report_target "$src_exec" "$dst_exec" "$src_man" "$dst_man"
  else
    install_pair "$src_exec" "$dst_exec" "$src_man" "$dst_man"
  fi
}

if [ "$INSTALL_BASH_ONLY" -eq 1 ]; then
  handle_pair \
    "$SCRIPT_DIR/bash/bash-only/ttychatter-gemini-bash" \
    "ttychatter-gemini-bash" \
    "$SCRIPT_DIR/bash/bash-only/ttychatter-gemini-bash.1" \
    "ttychatter-gemini-bash.1"
fi

if [ "$INSTALL_BASH_PYTHON3" -eq 1 ]; then
  handle_pair \
    "$SCRIPT_DIR/bash/python3/ttychatter-gemini-python3" \
    "ttychatter-gemini-python3" \
    "$SCRIPT_DIR/bash/python3/ttychatter-gemini-python3.1" \
    "ttychatter-gemini-python3.1"
fi

if [ "$INSTALL_NCURSES_PYTHON" -eq 1 ]; then
  handle_pair \
    "$SCRIPT_DIR/ncurses/python/ttychatter-gemini-ncurses-python" \
    "ttychatter-gemini-ncurses-python" \
    "$SCRIPT_DIR/ncurses/python/ttychatter-gemini-ncurses-python.1" \
    "ttychatter-gemini-ncurses-python.1"
fi

if [ "$LIST_TARGETS" -eq 1 ]; then
  exit 0
fi

if [ "$CHECK_ONLY" -eq 1 ]; then
  if [ "$CONFLICTS" -gt 0 ]; then
    printf '\nConflicts detected: %s\n' "$CONFLICTS"
    printf 'Use --name-prefix, --force, or --allow-shadow if appropriate.\n'
    exit 1
  fi
  if [ "$WARNINGS" -gt 0 ]; then
    printf '\nNo blocking conflicts detected, but warnings were found: %s\n' "$WARNINGS"
    exit 0
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
