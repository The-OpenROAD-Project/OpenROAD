# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""Parse Bazel test timeouts."""

from __future__ import annotations

import re
from typing import Iterable

from .base import Finding, Severity, StageContext

_PATTERN = re.compile(
    r"\bTimeout\s+after\s+(?P<seconds>\d+)\s+seconds\b", re.IGNORECASE
)


class BazelTimeoutParser:
    name = "bazel_timeout"

    def applies(self, check_name: str, repo: str) -> bool:
        return True

    def scan(self, lines: Iterable[str], ctx: StageContext) -> Iterable[Finding]:
        for i, line in enumerate(lines, start=1):
            m = _PATTERN.search(line)
            if not m:
                continue
            yield Finding(
                parser=self.name,
                severity=Severity.actionable,
                kind="bazel_timeout",
                headline=f"Test timed out after {m['seconds']}s",
                detail=line.strip(),
                stage=ctx.current,
                log_line=i,
                dedupe_key=f"bazel_timeout:{ctx.current or 'unknown'}",
                human_headline=f"Bazel test timed out after {m['seconds']}s",
                ai_directive=(
                    "A Bazel test exceeded its declared timeout. Check whether "
                    "the change introduced an algorithmic regression (likely "
                    "and fixable) or whether the test's `timeout=` attribute "
                    "needs to be raised (rare; only if the test legitimately "
                    "needs more time on this size of input)."
                ),
                verify_command=None,
            )
