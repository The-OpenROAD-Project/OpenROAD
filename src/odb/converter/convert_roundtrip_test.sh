#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026-2026, The OpenROAD Authors
#
# A converter's contract is entirely in its file header: an old database goes
# in, and what comes out must announce a schema the current OpenROAD accepts.
# design.odb is schema 57, the oldest revision odb has ever been able to read,
# so it exercises the longest run of compatibility code in the snapshot.

set -euo pipefail

CONVERTER=src/odb/converter/convert-57-119
INPUT=src/odb/test/data/design.odb
EXPECT_OUT_SCHEMA=119

# magic1 magic2 schema_major schema_minor, four little-endian uint32 at
# offset 0. Print the minor.
schema_of() {
    od -An -t u4 -N 16 "$1" | awk '{print $4}'
}

magic_of() {
    od -An -t u4 -N 16 "$1" | awk '{print $1}'
}

in_schema=$(schema_of "$INPUT")
echo "input $INPUT is schema $in_schema"

out=$TEST_TMPDIR/converted.odb
"$CONVERTER" "$INPUT" > "$out"

out_schema=$(schema_of "$out")
out_magic=$(magic_of "$out")

# 0x41544845, "ATHE" -- a truncated or logger-polluted stream fails here
# before the schema check can give a misleading answer.
if [ "$out_magic" != "1096042565" ]; then
    echo "FAIL: output is not an OpenDB database (magic $out_magic)" >&2
    exit 1
fi

if [ "$out_schema" != "$EXPECT_OUT_SCHEMA" ]; then
    echo "FAIL: expected output schema $EXPECT_OUT_SCHEMA, got $out_schema" >&2
    exit 1
fi

echo "PASS: $in_schema -> $out_schema"
