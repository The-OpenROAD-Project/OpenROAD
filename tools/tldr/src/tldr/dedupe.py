# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""Collapse identical findings across OS/platform legs by dedupe_key."""

from __future__ import annotations

from collections import OrderedDict
from typing import Iterable

from .parsers.base import Finding


def collapse(findings: Iterable[Finding]) -> list[Finding]:
    """Return one Finding per unique dedupe_key, preserving first-seen order."""
    by_key: OrderedDict[str, Finding] = OrderedDict()
    for f in findings:
        key = f.dedupe_key or f"{f.parser}:{f.kind}:{f.headline}"
        if key not in by_key:
            by_key[key] = f
    return list(by_key.values())


def group_by_kind(findings: Iterable[Finding]) -> "OrderedDict[str, list[Finding]]":
    """Bucket findings by kind, preserving first-seen kind order."""
    out: OrderedDict[str, list[Finding]] = OrderedDict()
    for f in findings:
        out.setdefault(f.kind, []).append(f)
    return out
