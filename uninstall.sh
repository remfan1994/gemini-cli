#!/usr/bin/env sh
# -----------------------------------------------------------------------------
# gemini-terminal-tools local uninstaller
# -----------------------------------------------------------------------------
# Removes files installed by install.sh from the selected prefix.  It does not
# remove user configuration, API keys, session logs, model caches, or attachment
# files.  Those are user data and should not be deleted by a generic uninstaller.
# -----------------------------------------------------------------------------

set -eu

PREFIX="${PREFIX:-$HOME/.local}"
DRY_RUN=0

usage() {
  cat <<EOF_USAGE
Usage: ./uninstall.sh [options]

Options:
  --prefix PATH              Remove files from PATH instead of ~/.local
  --dry-run                  Print planned removals without deleting files
  -h, --help                 Show this help

This script removes installed executables and man pages only.  It does not
remove ~/.config/gemini-cli, ~/.gpt, session logs, or attachments.
EOF_USAGE
}

while [ "$#" -gt 0 ]; do
  case "$1" in
    --prefix)
      shift
      [ "${1:-}" ] || { echo "ERROR: --prefix requires a path" >&2; exit 2; }
      PREFIX="$1"
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

BIN_DIR="$PREFIX/bin"
MAN_DIR="$PREFIX/share/man/man1"

remove_file() {
  path="$1"
  if [ "$DRY_RUN" -eq 1 ]; then
    printf 'DRY-RUN: rm -f %s\n' "$path"
  else
    rm -f "$path"
  fi
}

for name in gemini-terminal gemini-terminal-python3 gemini-ncurses-python; do
  remove_file "$BIN_DIR/$name"
done

for page in gemini-terminal.1 gemini-terminal-python3.1 gemini-ncurses-python.1; do
  remove_file "$MAN_DIR/$page"
done

cat <<EOF_DONE

Uninstall complete.

User data was not removed.  If you really want to remove config and sessions,
inspect these locations manually:
  ~/.config/gemini-cli/
  ~/.gpt/
EOF_DONE
