# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""Detect MODULE.bazel.lock / requirements lock-file drift."""

from __future__ import annotations

import re
from typing import Iterable

from .base import Finding, Severity, StageContext

_PATTERN = re.compile(
    r"\b(?P<file>(?:MODULE\.bazel\.lock|requirements_lock_[\w.]+\.txt))\b.*"
    r"\b(?:out of date|differs|needs to be regenerated)\b",
    re.IGNORECASE,
)


class LockfileParser:
    name = "lockfile"

    def applies(self, check_name: str, repo: str) -> bool:
        return True

    def scan(self, lines: Iterable[str], ctx: StageContext) -> Iterable[Finding]:
        seen: set[str] = set()
        for i, line in enumerate(lines, start=1):
            m = _PATTERN.search(line)
            if not m:
                continue
            file_ = m["file"]
            if file_ in seen:
                continue
            seen.add(file_)
            yield Finding(
                parser=self.name,
                severity=Severity.actionable,
                kind="lockfile_drift",
                headline=f"{file_} out of date",
                detail=line.strip(),
                stage=ctx.current,
                location=file_,
                log_line=i,
                dedupe_key=f"lockfile:{file_}",
                human_headline=f"`{file_}` is out of date — regenerate it",
                ai_directive=(
                    f"`{file_}` is out of date. Regenerate via "
                    "`bazelisk run //bazel:requirements.update` (for "
                    "requirements locks) or `bazelisk mod deps --lockfile_mode=update` "
                    "(for `MODULE.bazel.lock`)."
                ),
                verify_command="bazelisk mod deps --lockfile_mode=error",
                auto_fix_command=(
                    ("bazelisk", "run", "//bazel:requirements.update")
                    if "requirements" in file_
                    else ("bazelisk", "mod", "deps", "--lockfile_mode=update")
                ),
            )
