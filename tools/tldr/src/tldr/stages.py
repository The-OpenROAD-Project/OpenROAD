# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""Track which Jenkins pipeline stage is currently open.

Jenkins consoleText emits lines like::

    [Pipeline] { (Build on ubuntu:22.04)
    [Pipeline] }

Some configurations also emit ``Branch: <name>`` for parallel-leg fan-outs.
This module folds both into a single ``current`` stage string that parsers
read via the StageContext protocol.
"""

from __future__ import annotations

import re
from typing import Iterable, Iterator

_OPEN = re.compile(r"^\[Pipeline\]\s+\{\s+\((?P<stage>[^)]+)\)\s*$")
_CLOSE = re.compile(r"^\[Pipeline\]\s+\}\s*$")
_BRANCH = re.compile(
    r"^\[(?P<branch>[^\]]+)\]\s*\[Pipeline\]\s+\{\s+\((?P<stage>[^)]+)\)\s*$"
)


class StageTracker:
    def __init__(self) -> None:
        self._stack: list[str] = []

    @property
    def current(self) -> str | None:
        return self._stack[-1] if self._stack else None

    def feed(self, line: str) -> None:
        m = _BRANCH.match(line)
        if m:
            self._stack.append(f"{m['stage']} ({m['branch']})")
            return
        m = _OPEN.match(line)
        if m:
            self._stack.append(m["stage"])
            return
        if _CLOSE.match(line) and self._stack:
            self._stack.pop()


def annotate(lines: Iterable[str]) -> Iterator[tuple[str, str | None]]:
    """Yield (line, current_stage) for every input line."""
    tracker = StageTracker()
    for line in lines:
        tracker.feed(line)
        yield line, tracker.current
