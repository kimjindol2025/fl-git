#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
RUNNER="/home/kim/kim/platform/freelang-afj/bootstrap.js"

run_bounded() {
  set +e
  timeout 15s node "$@"
  local code=$?
  set -e
  if [ "$code" -eq 124 ]; then
    echo "BLOCKED: FreeLang AFJ runner timed out; inspect the runtime before implementation." >&2
    exit 2
  fi
  return "$code"
}

run_bounded "$RUNNER" check "$ROOT/src/git.fl"
run_bounded "$RUNNER" check "$ROOT/src/main.fl"
run_bounded "$RUNNER" check "$ROOT/src/tui.fl"
run_bounded "$RUNNER" run "$ROOT/src/main.fl" --help
run_bounded "$RUNNER" check "$ROOT/tests/smoke.fl"
run_bounded "$RUNNER" run "$ROOT/tests/smoke.fl"
"$ROOT/scripts/fl-test"
