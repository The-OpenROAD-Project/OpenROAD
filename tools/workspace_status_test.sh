#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

set -euo pipefail

status_script="$PWD/$1"
export PATH="$(dirname "$PWD/$2"):$PATH"
export GIT_CONFIG_GLOBAL=/dev/null
export GIT_CONFIG_NOSYSTEM=1
unset OPENROAD_VERSION

check_version() {
  local expected=$1
  local actual
  actual=$(bash "$status_script" | awk '/^STABLE_GIT_VERSION / {print $2}')
  if [[ "$actual" != "$expected" ]]; then
    printf 'Expected version %s, got %s\n' "$expected" "$actual" >&2
    return 1
  fi
}

cd "$TEST_TMPDIR"
mkdir -p checkout/export
cd checkout
git init -q --template=
printf 'tracked\n' > tracked.txt
git add tracked.txt
git -c user.name=Test -c user.email=test@example.com commit -qm 'Initial commit'
git tag 26Q4
check_version 26Q4

printf 'edited\n' >> tracked.txt
check_version 26Q4-dirty

cd export
check_version unknown
OPENROAD_VERSION=source-export check_version source-export
