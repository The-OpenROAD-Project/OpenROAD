# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""Parse Mac-Build clang/gcc compile errors: file:line:col: error: ..."""

from __future__ import annotations

import re
from typing import Iterable

from ..paths import to_repo_relative
from .base import Finding, Severity, StageContext

_PATTERN = re.compile(
    r"^\s*(?P<file>[^:\s]+\.(?:cpp|cc|cxx|c|h|hh|hpp|m|mm)):"
    r"(?P<line>\d+):(?P<col>\d+):\s+error:\s+(?P<msg>.+)$"
)


class MacBuildParser:
    name = "mac_build"

    def applies(self, check_name: str, repo: str) -> bool:
        return "mac" in check_name.lower() or "build" in check_name.lower()

    def scan(self, lines: Iterable[str], ctx: StageContext) -> Iterable[Finding]:
        seen: set[str] = set()
        for i, line in enumerate(lines, start=1):
            m = _PATTERN.match(line)
            if not m:
                continue
            file_ = to_repo_relative(m["file"])
            location = f"{file_}:{m['line']}:{m['col']}"
            if location in seen:
                continue
            seen.add(location)
            yield Finding(
                parser=self.name,
                severity=Severity.actionable,
                kind="compile_error",
                headline=f"{location}: {m['msg']}",
                detail=line.strip(),
                stage=ctx.current,
                location=location,
                log_line=i,
                dedupe_key=f"compile_error:{location}",
                human_headline=f"Compile error at `{location}`",
                ai_directive=(
                    f"Compile error at `{location}`: {m['msg']}. Inspect the "
                    "code at that location and fix the underlying issue. If "
                    "the error is platform-specific (e.g. missing macOS "
                    "header), guard the affected code path properly rather "
                    "than disabling the build leg."
                ),
                verify_command=None,
            )
