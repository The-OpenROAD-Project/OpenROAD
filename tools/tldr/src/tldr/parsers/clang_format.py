# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""Parse clang-format diff output ("--- file (original) / +++ file (modified)")."""

from __future__ import annotations

import re
from typing import Iterable

from .base import Finding, Severity, StageContext

# `clang-format --dry-run -Werror` emits diagnostics like
#   src/foo.cpp:42:1: error: code should be clang-formatted [-Wclang-format-violations]
_PATTERN = re.compile(
    r"^(?P<file>[^:\s]+\.(?:cpp|cc|cxx|c|h|hh|hpp)):"
    r"(?P<line>\d+):(?P<col>\d+):\s+error:\s+code should be clang-formatted"
)


class ClangFormatParser:
    name = "clang_format"

    def applies(self, check_name: str, repo: str) -> bool:
        n = check_name.lower()
        return "format" in n or "clang" in n

    def scan(
        self, lines: Iterable[str], ctx: StageContext
    ) -> Iterable[Finding]:
        seen: set[str] = set()
        for i, line in enumerate(lines, start=1):
            m = _PATTERN.match(line)
            if not m:
                continue
            file_ = m["file"]
            if file_ in seen:
                continue
            seen.add(file_)
            yield Finding(
                parser=self.name,
                severity=Severity.actionable,
                kind="clang_format",
                headline=f"{file_} not clang-formatted",
                detail=line.strip(),
                stage=ctx.current,
                location=file_,
                log_line=i,
                dedupe_key=f"clang_format:{file_}",
                human_headline=f"`{file_}` is not clang-formatted",
                ai_directive=(
                    f"`{file_}` is not clang-formatted. Run "
                    f"`clang-format -i {file_}` (skip files under `src/sta/` "
                    "and any `*.i` files per CLAUDE.md rule 2)."
                ),
                verify_command=f"clang-format --dry-run -Werror {file_}",
                auto_fix_command=("clang-format", "-i", file_),
            )
