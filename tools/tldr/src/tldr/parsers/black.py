# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""Parse Black formatter "would reformat" lines."""

from __future__ import annotations

import re
from typing import Iterable

from ..paths import to_repo_relative
from .base import Finding, Severity, StageContext

_PATTERN = re.compile(r"^would reformat\s+(?P<file>\S.+\.py)\s*$")


class BlackParser:
    name = "black"

    def applies(self, check_name: str, repo: str) -> bool:
        return "black" in check_name.lower()

    def scan(self, lines: Iterable[str], ctx: StageContext) -> Iterable[Finding]:
        seen: set[str] = set()
        for i, line in enumerate(lines, start=1):
            m = _PATTERN.match(line)
            if not m:
                continue
            file_ = to_repo_relative(m["file"])
            if file_ in seen:
                continue
            seen.add(file_)
            yield Finding(
                parser=self.name,
                severity=Severity.actionable,
                kind="black",
                headline=f"{file_} not black-formatted",
                detail=line.strip(),
                stage=ctx.current,
                location=file_,
                log_line=i,
                dedupe_key=f"black:{file_}",
                human_headline=f"`{file_}` is not Black-formatted",
                ai_directive=f"Run `black {file_}`.",
                verify_command=f"black --check {file_}",
                auto_fix_command=("black", file_),
            )
