#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
#
# Usage (from the OpenROAD repo root):
#   bash .agents/skills/design-smells/run.sh extract <paths...> -o facts.json
#   bash .agents/skills/design-smells/run.sh detect facts.json [options]
#
# extract needs libclang.  This script finds the host's newest libclang
# (override with LIBCLANG=/path/to/libclang.so and CLANG_RESOURCE_DIR=...),
# and on first use installs the matching Python bindings into
# ~/.cache/openroad-design-smells/venv-<major>.

set -euo pipefail

here="$(cd "$(dirname "$0")" && pwd)"
cmd="${1:-}"
shift || true

case "${cmd}" in
  detect)
    exec python3 "${here}/scripts/detect.py" "$@"
    ;;
  extract) ;;
  *)
    sed -n '4,12p' "$0" | sed 's/^# \{0,1\}//'
    exit 2
    ;;
esac

# Print the path with the highest version numbers.  Older macOS sort has no
# -V, so compare the numbers in each path in Python.
newest() {
  python3 -c '
import re, sys
paths = [p for p in sys.stdin.read().split("\n") if p]
if paths:
    print(max(paths, key=lambda p: [int(n) for n in re.findall(r"\d+", p)]))
'
}

if [ -z "${LIBCLANG:-}" ]; then
  LIBCLANG="$(ls -1 /usr/lib/llvm-*/lib/libclang-[0-9]*.so.[0-9]* \
                    /opt/homebrew/opt/llvm/lib/libclang.dylib \
                    /usr/local/opt/llvm/lib/libclang.dylib 2>/dev/null \
              | newest || true)"
fi
if [ -z "${LIBCLANG}" ] || [ ! -e "${LIBCLANG}" ]; then
  echo "libclang not found; install LLVM (e.g. apt install libclang-19-dev)" \
       "or set LIBCLANG=/path/to/libclang.so" >&2
  exit 1
fi

llvm_lib="$(dirname "${LIBCLANG}")"
if [ -z "${CLANG_RESOURCE_DIR:-}" ]; then
  CLANG_RESOURCE_DIR="$(ls -1d "${llvm_lib}"/clang/[0-9]* 2>/dev/null \
                        | newest || true)"
fi
if [ -z "${CLANG_RESOURCE_DIR}" ] || [ ! -d "${CLANG_RESOURCE_DIR}/include" ]; then
  echo "clang resource dir (with include/stddef.h) not found next to" \
       "${LIBCLANG}; set CLANG_RESOURCE_DIR" >&2
  exit 1
fi
major="$(basename "${CLANG_RESOURCE_DIR}" | cut -d. -f1)"

venv="${HOME}/.cache/openroad-design-smells/venv-${major}"
if [ ! -x "${venv}/bin/python" ] \
   || ! "${venv}/bin/python" -c 'import clang.cindex' 2>/dev/null; then
  echo "installing clang==${major}.* Python bindings into ${venv}" >&2
  python3 -m venv "${venv}"
  "${venv}/bin/pip" install --quiet "clang==${major}.*"
fi

export LIBCLANG CLANG_RESOURCE_DIR
exec "${venv}/bin/python" "${here}/scripts/extract.py" "$@"
