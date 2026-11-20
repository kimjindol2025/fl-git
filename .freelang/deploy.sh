#!/usr/bin/env bash
set -Eeuo pipefail
# Prepare a Native release and user-service plan; activation is separate.
SCRIPT_PATH="$(readlink -f -- "${BASH_SOURCE[0]}")"
exec "${SCRIPT_PATH%/*}/../scripts/fl-release-prepare-native" "$@"
