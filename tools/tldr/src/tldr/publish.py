# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""Sentinel-safe upsert of the bracketed Markdown block into a PR body or comment.

The publisher matches the strict regex
``<!-- tldr:begin -->[\\s\\S]*?<!-- tldr:end -->`` and replaces exactly that
span. Outside the span the body is byte-exact. Malformed bodies (orphan
begin/end, double begin) raise ``MalformedBodyError`` rather than
corrupting the body.

This module is exercised only by tests in this PR; no in-tree caller passes
``--post`` to the CLI. Maintainers will wire ``--post`` up via a workflow
after beta-testing.
"""

from __future__ import annotations

import re

from .render_markdown import BEGIN, END

_SPAN = re.compile(re.escape(BEGIN) + r"[\s\S]*?" + re.escape(END))


class MalformedBodyError(Exception):
    """Raised when the body's existing sentinel state is ambiguous."""


def upsert(body: str, block: str) -> str:
    """Return a new body with `block` placed inside the sentinel span.

    `block` must itself begin with `BEGIN` and end with `END`; this function
    verifies that and replaces (or appends) the bracketed region.
    """
    if not block.startswith(BEGIN) or BEGIN not in block or END not in block:
        raise ValueError("block must contain both begin and end sentinels")
    if block.count(BEGIN) != 1 or block.count(END) != 1:
        raise ValueError("block must contain exactly one begin/end sentinel")

    body = body or ""
    n_begin = body.count(BEGIN)
    n_end = body.count(END)
    # Counting well-formed bracketed spans on top of the substring counts
    # catches nested or interleaved sentinels that the substring-only check
    # would miss (e.g. ``BEGIN ... BEGIN ... END`` parses as a single span
    # but n_begin would be 2 — the body is still malformed and we refuse).
    n_blocks = len(_SPAN.findall(body))

    if n_begin == 0 and n_end == 0:
        # First run: append after one blank line.
        sep = (
            ""
            if body.endswith("\n\n") or body == ""
            else ("\n\n" if not body.endswith("\n") else "\n")
        )
        return body + sep + block

    if n_begin != 1 or n_end != 1 or n_blocks != 1:
        raise MalformedBodyError(
            f"sentinel mismatch: {n_begin} begin, {n_end} end, "
            f"{n_blocks} valid block(s)"
        )

    # Replace exactly the bracketed span.
    new_body, count = _SPAN.subn(lambda _m: block.rstrip("\n"), body, count=1)
    if count != 1:
        raise MalformedBodyError(
            "sentinels present but the bracketed span could not be matched "
            "(check for nested or malformed sentinels)"
        )
    return new_body
