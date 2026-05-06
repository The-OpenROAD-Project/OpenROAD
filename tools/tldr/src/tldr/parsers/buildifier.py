# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""Parse Buildifier "not formatted" diagnostics."""

from __future__ import annotations

import re
from typing import Iterable

from ..paths import to_repo_relative
from .base import Finding, Severity, StageContext

# Buildifier's -mode=check emits one line per file needing reformatting:
#   tools/foo/BUILD.bazel # reformat
# Older outputs occasionally use the "<file>: not formatted" colon form.
_PATTERN = re.compile(
    r"^(?P<file>\S+(?:BUILD(?:\.bazel)?|\.bzl|MODULE\.bazel|WORKSPACE(?:\.bazel)?))"
    r"(?:\s*#\s*reformat\b|:\s*(?:not formatted|needs reformat)\b)"
)


class BuildifierParser:
    name = "buildifier"

    def applies(self, check_name: str, repo: str) -> bool:
        return "buildifier" in check_name.lower()

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
                kind="buildifier",
                headline=f"{file_} not buildifier-formatted",
                detail=line.strip(),
                stage=ctx.current,
                location=file_,
                log_line=i,
                dedupe_key=f"buildifier:{file_}",
                human_headline=f"`{file_}` is not Buildifier-formatted",
                ai_directive=f"Run `buildifier -mode=fix {file_}`.",
                verify_command=f"buildifier -mode=check {file_}",
                auto_fix_command=("buildifier", "-mode=fix", file_),
            )
