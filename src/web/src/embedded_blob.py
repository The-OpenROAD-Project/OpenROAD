# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
#
# What embed_web_assets.py and embed_report_assets.py share: a gzip that gives
# the same bytes from the same zlib, and writing the result as a C array.

import gzip
import io

# Offset of the OS field in a gzip header, and its "unknown" value; some
# Pythons write the host OS there.
_GZIP_OS_OFFSET = 9
_GZIP_OS_UNKNOWN = 0xFF


def gzip_bytes(data):
    """gzip with mtime=0 and a fixed OS byte: same input and zlib, same bytes."""
    # GzipFile, not gzip.compress(): the latter takes mtime only from 3.8.
    buffer = io.BytesIO()
    with gzip.GzipFile(fileobj=buffer, mode="wb", compresslevel=9, mtime=0) as f:
        f.write(data)
    packed = bytearray(buffer.getvalue())
    packed[_GZIP_OS_OFFSET] = _GZIP_OS_UNKNOWN
    return bytes(packed)


# One escape per byte value, so the loop below is a table lookup.
_OCTAL_ESCAPES = [f"\\{b:03o}" for b in range(256)]

# Octal escapes are 4 chars each; 20 bytes keeps the emitted line at 80 columns.
_BYTES_PER_LINE = 20


def write_char_array(out, ident, data):
    """Write data as `static const char <ident>_data[]`, in octal escapes.

    Its length has to be passed along separately: a gzip stream may contain a
    NUL, so sizeof - 1 is only right by accident.
    """
    escaped = "".join(map(_OCTAL_ESCAPES.__getitem__, data))
    width = _BYTES_PER_LINE * 4
    lines = (escaped[i : i + width] for i in range(0, len(escaped), width))
    out.write(f'static const char {ident}_data[] =\n    "')
    out.write('"\n    "'.join(lines))
    out.write('";\n\n')
