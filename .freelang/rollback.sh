#!/usr/bin/env bash
set -Eeuo pipefail
# Select the previous sealed release explicitly; prepare its service plan.
if (($# == 0)) || [[ "$1" == -* ]]; then
  printf 'FL_GIT_ROLLBACK=BLOCKED\nCAUSE=Select the previous sealed code release.\n' >&2
  exit 2
fi
SCRIPT_PATH="$(readlink -f -- "${BASH_SOURCE[0]}")"
SELECTED_RELEASE="$1"
shift
exec "${SCRIPT_PATH%/*}/../scripts/fl-release-prepare-native" --from-release "$SELECTED_RELEASE" "$@"
