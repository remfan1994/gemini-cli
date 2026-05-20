#!/usr/bin/env sh
# Conservative ttychatter uninstaller.
# Removes installed command/manpage files only. It never deletes API keys,
# config files, session logs, attachments, or model cache data.

set -eu
PREFIX="${PREFIX:-$HOME/.local}"
NAME_PREFIX=""
DRY_RUN=0

usage() {
  cat <<EOF_USAGE
Usage: ./uninstall.sh [options]

Options:
  --prefix PATH              Remove from PATH instead of ~/.local
  --name-prefix PREFIX       Match prefixed install names
  --dry-run                  Print planned removals without deleting files
  -h, --help                 Show this help
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

run_rm() {
  if [ "$DRY_RUN" -eq 1 ]; then
    printf 'DRY-RUN: rm -f %s\n' "$1"
  else
    rm -f "$1"
  fi
}

for name in ttychatter-gemini-bash ttychatter-gemini-python3 ttychatter-gemini-ncurses-python; do
  run_rm "$BIN_DIR/${NAME_PREFIX}${name}"
done

for page in ttychatter-gemini-bash.1 ttychatter-gemini-python3.1 ttychatter-gemini-ncurses-python.1; do
  run_rm "$MAN_DIR/${NAME_PREFIX}${page}"
done

cat <<EOF_DONE

Uninstall complete.

User data was not removed. Inspect these manually if needed:
  ~/.config/ttychatter/gemini/
  ~/.cache/ttychatter/gemini/
  ~/.gpt/
EOF_DONE
