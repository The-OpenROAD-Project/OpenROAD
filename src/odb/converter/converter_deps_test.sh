#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026-2026, The OpenROAD Authors
#
# Converters are copied next to the openroad binary and have to run there with
# nothing else installed. They cannot be fully static -- the hermetic
# toolchain ships no libc.a -- so what actually keeps them portable is that
# they need no more shared libraries than openroad itself. Assert that
# directly, so a snapshot that starts dragging in a new .so is caught here
# rather than on a user's machine.

set -euo pipefail

CONVERTER=src/odb/converter/convert-57-119

ALLOWED='^(linux-vdso|libc|libm|libpthread|libdl|librt|ld-linux.*)\.so'

if command -v ldd >/dev/null 2>&1; then
    # One line per entry, reduced to a bare soname: ldd prints "name => path"
    # for resolved libraries but a bare absolute path for the loader itself.
    unexpected=$(ldd "$CONVERTER" \
        | sed -E 's/^[[:space:]]*//; s/ =>.*//; s/ \(0x[0-9a-f]*\)$//; s@.*/@@' \
        | grep -v '^$' \
        | grep -Ev "$ALLOWED" || true)
elif command -v otool >/dev/null 2>&1; then
    # macOS: the equivalent floor is the OS-provided libraries, which live
    # under /usr/lib and /System. Anything else would have to be shipped.
    unexpected=$(otool -L "$CONVERTER" \
        | tail -n +2 \
        | sed -E 's/^[[:space:]]*//; s/ \(compatibility.*//' \
        | grep -v '^$' \
        | grep -Ev '^(/usr/lib/|/System/)' || true)
else
    echo "SKIP: neither ldd nor otool available" >&2
    exit 0
fi

if [ -n "$unexpected" ]; then
    echo "FAIL: converter depends on unexpected shared libraries:" >&2
    echo "$unexpected" >&2
    exit 1
fi

echo "PASS: only the base system library set"
