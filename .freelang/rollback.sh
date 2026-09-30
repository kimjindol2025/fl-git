#!/bin/sh
set -eu

if ! command -v pm2 >/dev/null 2>&1 || ! pm2 describe fl-git >/dev/null 2>&1; then
  echo 'FL_GIT_ROLLBACK=BLOCKED'
  echo 'CAUSE=pm2 process fl-git is not registered'
  exit 2
fi

pm2 restart fl-git --update-env
echo 'FL_GIT_ROLLBACK=RESTART_CURRENT_ARTIFACT'
