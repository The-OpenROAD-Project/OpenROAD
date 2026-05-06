# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""Robust parser for `gh api --paginate` output.

`gh api --paginate` concatenates page bodies. For endpoints that return a
JSON array, the result is `[…][…][…]`; for single-object endpoints it's
`{…}{…}…`. Splitting by `\\n\\n` is fragile because nothing prevents a
double newline inside a JSON string value (e.g. inside a check-run's
`output.summary` field). Use ``json.JSONDecoder.raw_decode`` to walk one
top-level JSON value at a time.
"""

from __future__ import annotations

import json
from typing import Iterable


def iter_objects(text: str) -> Iterable[dict | list]:
    """Yield each top-level JSON value from concatenated `gh api --paginate` output."""
    decoder = json.JSONDecoder()
    n = len(text)
    i = 0
    while i < n:
        while i < n and text[i] in " \r\n\t":
            i += 1
        if i >= n:
            return
        try:
            obj, end = decoder.raw_decode(text, idx=i)
        except json.JSONDecodeError:
            return
        yield obj
        i = end


def flatten_arrays(text: str) -> list[dict]:
    """Parse paginated array endpoints into one flat list of records."""
    out: list[dict] = []
    for obj in iter_objects(text):
        if isinstance(obj, list):
            for item in obj:
                if isinstance(item, dict):
                    out.append(item)
        elif isinstance(obj, dict):
            out.append(obj)
    return out
