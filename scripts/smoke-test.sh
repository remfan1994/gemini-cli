#!/usr/bin/env sh
# -----------------------------------------------------------------------------
# gemini-terminal-tools smoke test
# -----------------------------------------------------------------------------
# This script checks syntax and basic non-network commands.  It intentionally
# avoids API calls, model-list updates, and Gemini generation requests so it can
# run on developer machines without credentials or internet access.
# -----------------------------------------------------------------------------

set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
FAIL=0

say() { printf '%s\n' "$*"; }

check() {
  label="$1"
  shift
  say "[check] $label"
  if "$@"; then
    say "  ok"
  else
    say "  FAIL"
    FAIL=1
  fi
}

check "bash/python3 syntax" bash -n "$ROOT/bash/python3/gemini-terminal-python3"
check "bash/bash-only syntax" bash -n "$ROOT/bash/bash-only/gemini-terminal"
check "ncurses/python syntax" python3 -m py_compile "$ROOT/ncurses/python/gemini-ncurses-python"

check "gemini-terminal-python3 --version" "$ROOT/bash/python3/gemini-terminal-python3" --version
check "gemini-terminal --version" "$ROOT/bash/bash-only/gemini-terminal" --version
check "gemini-ncurses-python --version" "$ROOT/ncurses/python/gemini-ncurses-python" --version

check "gemini-terminal-python3 --help" "$ROOT/bash/python3/gemini-terminal-python3" --help >/dev/null
check "gemini-terminal --help" "$ROOT/bash/bash-only/gemini-terminal" --help >/dev/null
check "gemini-ncurses-python --help" "$ROOT/ncurses/python/gemini-ncurses-python" --help >/dev/null

if [ "$FAIL" -ne 0 ]; then
  say ""
  say "Smoke test failed."
  exit 1
fi

say ""
say "Smoke test passed."
