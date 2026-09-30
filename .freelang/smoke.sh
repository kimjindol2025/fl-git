#!/bin/sh
set -eu

PORT="${FL_GIT_PORT:-40850}"
URL="${FL_GIT_SMOKE_URL:-http://127.0.0.1:${PORT}/}"

if ! command -v curl >/dev/null 2>&1; then
  echo 'FL_GIT_SMOKE=BLOCKED'
  echo 'CAUSE=curl not found'
  exit 2
fi

curl -fsS --max-time "${FL_GIT_SMOKE_TIMEOUT:-10}" "$URL" >/dev/null
echo "FL_GIT_SMOKE=PASS URL=$URL"
