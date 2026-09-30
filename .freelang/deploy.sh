#!/bin/sh
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd -P)"
WEB="$ROOT/web"
PORT="${FL_GIT_PORT:-40850}"

if ! command -v pm2 >/dev/null 2>&1; then
  echo 'FL_GIT_DEPLOY=BLOCKED'
  echo 'CAUSE=pm2 not found'
  exit 2
fi
if ! pm2 describe fl-git >/dev/null 2>&1; then
  echo 'FL_GIT_DEPLOY=BLOCKED'
  echo 'CAUSE=pm2 process fl-git is not registered'
  exit 2
fi

cd "$WEB"
FL_GIT_REPO="$ROOT" PORT="$PORT" FL_HTTP_ALLOW="${FL_HTTP_ALLOW:-127.0.0.1,localhost}" node fl-front-build.js
pm2 restart fl-git --update-env
echo 'FL_GIT_DEPLOY=PASS'
