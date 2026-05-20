#!/usr/bin/env sh
# -----------------------------------------------------------------------------
# ttychatter smoke test
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

check "bash/python3 syntax" bash -n "$ROOT/gemini/bash/python3/ttychatter-gemini-python3"
check "bash/bash-only syntax" bash -n "$ROOT/gemini/bash/bash-only/ttychatter-gemini-bash"
check "ncurses/python syntax" python3 -m py_compile "$ROOT/gemini/ncurses/python/ttychatter-gemini-ncurses-python"

check "ttychatter-gemini-python3 --version" "$ROOT/gemini/bash/python3/ttychatter-gemini-python3" --version
check "ttychatter-gemini-bash --version" "$ROOT/gemini/bash/bash-only/ttychatter-gemini-bash" --version
check "ttychatter-gemini-ncurses-python --version" "$ROOT/gemini/ncurses/python/ttychatter-gemini-ncurses-python" --version

check "ttychatter-gemini-python3 --help" "$ROOT/gemini/bash/python3/ttychatter-gemini-python3" --help >/dev/null
check "ttychatter-gemini-bash --help" "$ROOT/gemini/bash/bash-only/ttychatter-gemini-bash" --help >/dev/null
check "ttychatter-gemini-ncurses-python --help" "$ROOT/gemini/ncurses/python/ttychatter-gemini-ncurses-python" --help >/dev/null

if [ "$FAIL" -ne 0 ]; then
  say ""
  say "Smoke test failed."
  exit 1
fi

say ""
say "Smoke test passed."
