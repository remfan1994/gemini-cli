#!/usr/bin/env sh
# -----------------------------------------------------------------------------
# ttychatter namespace audit
# -----------------------------------------------------------------------------
# This script protects the project from stale names after the ttychatter rename.
# Renames are risky because old command names often remain buried in man pages,
# help output, install scripts, examples, comments, and maintenance documents.
# A stale executable name can confuse users just as badly as a code bug.
#
# Important detail: the current names intentionally contain provider/interface
# words such as "gemini" and "ncurses".  Therefore this script must not merely
# search for substrings like "gemini-ncurses-python", because that phrase appears
# inside the valid new command name "ttychatter-gemini-ncurses-python".  The audit
# uses boundary-aware regular expressions so it catches old standalone names but
# does not flag the new ttychatter-prefixed command family.
# -----------------------------------------------------------------------------

set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
FAIL=0
TMP_MATCH="/tmp/ttychatter-name-audit.$$"
TMP_FAIL="/tmp/ttychatter-name-audit-fail.$$"

say() { printf '%s\n' "$*"; }

is_binary_or_generated() {
  case "$1" in
    */.git/*|*/__pycache__/*|*.pyc|*.zip|*.png|*.jpg|*.jpeg|*.gif|*.webp|*.pdf|*.mp4|*.mp3|*.wav)
      return 0
      ;;
    *)
      return 1
      ;;
  esac
}

is_allowed_legacy_file() {
  case "$1" in
    */docs/NAMESPACE_MIGRATION.md|*/docs/MAINTAINERS_GUIDE.md|*/docs/RELEASE_CANDIDATE_AUDIT.md|*/PROJECT-CHANGELOG-*|*/CHANGELOG*|*/README.md|*/scripts/name-audit.sh)
      return 0
      ;;
    *)
      return 1
      ;;
  esac
}

check_regex() {
  file="$1"
  rel="$2"
  label="$3"
  regex="$4"

  if grep -nE "$regex" "$file" >"$TMP_MATCH" 2>/dev/null; then
    if is_allowed_legacy_file "$file"; then
      printf 'WARN: legacy name in allowed context: %s -> %s\n' "$rel" "$label"
      sed 's/^/  /' "$TMP_MATCH" | head -5
    else
      printf 'FAIL: stale name in current file: %s -> %s\n' "$rel" "$label"
      sed 's/^/  /' "$TMP_MATCH" | head -5
      echo 1 >"$TMP_FAIL"
    fi
  fi
}

say "ttychatter namespace audit"
say "root: $ROOT"
say ""

# Boundary characters treat letters, numbers, underscore, and dash as part of a
# command token.  This prevents matching "gemini-terminal" inside
# "ttychatter-gemini-terminal" if that ever exists, while still matching old
# standalone examples such as "gemini-terminal --help".
B='(^|[^A-Za-z0-9_-])'
E='([^A-Za-z0-9_-]|$)'

find "$ROOT" -type f | while IFS= read -r file; do
  rel=${file#"$ROOT/"}
  if is_binary_or_generated "$file"; then
    continue
  fi

  check_regex "$file" "$rel" "gemini-cli" "${B}gemini-cli${E}"
  check_regex "$file" "$rel" "gemini-terminal-tools" "${B}gemini-terminal-tools${E}"
  check_regex "$file" "$rel" "gemini-terminal-python3" "${B}gemini-terminal-python3${E}"
  check_regex "$file" "$rel" "gemini-terminal" "${B}gemini-terminal${E}"
  check_regex "$file" "$rel" "gemini-ncurses-python" "${B}gemini-ncurses-python${E}"
  check_regex "$file" "$rel" "gemini-curses" "${B}gemini-curses${E}"
done

if [ -f "$TMP_FAIL" ]; then
  FAIL=1
fi
rm -f "$TMP_MATCH" "$TMP_FAIL"

say ""
if [ "$FAIL" -eq 0 ]; then
  say "Namespace audit complete. No blocking stale-name references found."
else
  say "Namespace audit found blocking stale-name references."
fi

exit "$FAIL"
