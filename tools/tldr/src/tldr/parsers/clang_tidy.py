# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""Parse Clang-Tidy diagnostics with a [check-name] suffix."""

from __future__ import annotations

import re
from typing import Iterable

from ..paths import to_repo_relative
from .base import Finding, Severity, StageContext

_PATTERN = re.compile(
    r"^(?P<file>[^:\s]+):(?P<line>\d+):(?P<col>\d+):\s+"
    r"warning:\s+(?P<msg>.+?)\s+\[(?P<check>[a-z0-9.\-,_]+)\]\s*$"
)

# Subset of clang-tidy checks for which `--fix` produces safe, deterministic
# output that does not change semantics. Conservative on purpose.
_AUTO_FIXABLE = frozenset(
    {
        "modernize-use-nullptr",
        "modernize-use-override",
        "modernize-use-using",
        "readability-braces-around-statements",
        "readability-redundant-string-init",
    }
)


class ClangTidyParser:
    name = "clang_tidy"

    def applies(self, check_name: str, repo: str) -> bool:
        return "tidy" in check_name.lower()

    def scan(self, lines: Iterable[str], ctx: StageContext) -> Iterable[Finding]:
        for i, line in enumerate(lines, start=1):
            m = _PATTERN.match(line)
            if not m:
                continue
            file_ = to_repo_relative(m["file"])
            lineno, col = m["line"], m["col"]
            check = m["check"]
            location = f"{file_}:{lineno}:{col}"
            auto_fix = (
                ("clang-tidy", "--fix", f"--checks=-*,{check}", file_)
                if check in _AUTO_FIXABLE
                else None
            )
            yield Finding(
                parser=self.name,
                severity=Severity.actionable,
                kind="clang_tidy",
                headline=f"{location}: {m['msg']} [{check}]",
                detail=line.strip(),
                stage=ctx.current,
                location=location,
                log_line=i,
                dedupe_key=f"clang_tidy:{location}:{check}",
                human_headline=f"Clang-Tidy `{check}` at `{location}`",
                ai_directive=(
                    f"Clang-Tidy raised `{check}` at `{location}`: "
                    f"{m['msg']}. Read the diagnostic, then either fix the "
                    "underlying code (preferred) or, if the warning is "
                    "wrong for this site, add a narrow NOLINT with a "
                    "comment explaining why."
                ),
                verify_command=f"clang-tidy {file_}",
                auto_fix_command=auto_fix,
            )
