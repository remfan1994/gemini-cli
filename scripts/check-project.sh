#!/usr/bin/env bash
# check-project.sh
#
# Maintainer smoke-test harness for ttychatter.
#
# This script intentionally does not perform network calls and does not require
# an API key. It checks the things that should always work locally: syntax,
# version output, help output, and selected diagnostic commands. It is meant to
# be safe to run before committing a release candidate.

set -u

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
FAIL=0

say() {
  printf '%s\n' "$*"
}

pass() {
  say "PASS: $*"
}

fail() {
  say "FAIL: $*"
  FAIL=1
}

run_check() {
  local label="$1"
  shift
  if "$@" >/tmp/gtt-check.out 2>/tmp/gtt-check.err; then
    pass "$label"
  else
    fail "$label"
    say "  command: $*"
    sed 's/^/  stderr: /' /tmp/gtt-check.err | head -20
    sed 's/^/  stdout: /' /tmp/gtt-check.out | head -20
  fi
}

check_file_exists() {
  local path="$1"
  if [ -f "$ROOT/$path" ]; then
    pass "file exists: $path"
  else
    fail "missing file: $path"
  fi
}

say "ttychatter local smoke test"
say "root: $ROOT"
say ""

check_file_exists "ncurses/python/ttychatter-gemini-ncurses-python"
check_file_exists "bash/python3/ttychatter-gemini-python3"
check_file_exists "bash/bash-only/ttychatter-gemini-bash"
check_file_exists "README.md"
check_file_exists ".gitignore"

if command -v python3 >/dev/null 2>&1; then
  run_check "python ncurses syntax" python3 -m py_compile "$ROOT/ncurses/python/ttychatter-gemini-ncurses-python"
  run_check "ttychatter-gemini-ncurses-python --version" "$ROOT/ncurses/python/ttychatter-gemini-ncurses-python" --version
  run_check "ttychatter-gemini-ncurses-python --help" "$ROOT/ncurses/python/ttychatter-gemini-ncurses-python" --help
else
  fail "python3 not found; cannot check ncurses/python syntax"
fi

run_check "bash/python3 syntax" bash -n "$ROOT/bash/python3/ttychatter-gemini-python3"
run_check "bash/bash-only syntax" bash -n "$ROOT/bash/bash-only/ttychatter-gemini-bash"
run_check "ttychatter-gemini-python3 --version" "$ROOT/bash/python3/ttychatter-gemini-python3" --version
run_check "ttychatter-gemini-bash --version" "$ROOT/bash/bash-only/ttychatter-gemini-bash" --version
run_check "ttychatter-gemini-python3 --help" "$ROOT/bash/python3/ttychatter-gemini-python3" --help
run_check "ttychatter-gemini-bash --help" "$ROOT/bash/bash-only/ttychatter-gemini-bash" --help

run_check "namespace audit" "$ROOT/scripts/name-audit.sh"
run_check "installer collision check" "$ROOT/install.sh" --check --all
run_check "installer dry run" "$ROOT/install.sh" --dry-run --all

say ""
if [ "$FAIL" -eq 0 ]; then
  say "All local smoke tests passed."
else
  say "One or more local smoke tests failed."
fi

exit "$FAIL"
