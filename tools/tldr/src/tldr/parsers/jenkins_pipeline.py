# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""Catch fatal Jenkins pipeline errors that don't otherwise match a parser."""

from __future__ import annotations

import re
from typing import Iterable

from .base import Finding, Severity, StageContext

_PATTERN = re.compile(
    r"^\s*ERROR: A fatal error occurred in the pipeline: (?P<msg>.+)$"
)


class JenkinsPipelineParser:
    name = "jenkins_pipeline"

    def applies(self, check_name: str, repo: str) -> bool:
        return "jenkins" in check_name.lower()

    def scan(self, lines: Iterable[str], ctx: StageContext) -> Iterable[Finding]:
        for i, line in enumerate(lines, start=1):
            m = _PATTERN.match(line)
            if not m:
                continue
            yield Finding(
                parser=self.name,
                severity=Severity.actionable,
                kind="jenkins_pipeline",
                headline=m["msg"].strip(),
                detail=line.strip(),
                stage=ctx.current,
                log_line=i,
                dedupe_key=f"jenkins_pipeline:{ctx.current or 'unknown'}:{m['msg'].strip()}",
                human_headline=f"Jenkins pipeline aborted in stage `{ctx.current or 'unknown'}`",
                ai_directive=(
                    "Jenkins reported a fatal pipeline error. The actionable "
                    "cause is usually an earlier finding from another parser; "
                    "investigate that first. If no other parser fired, scan "
                    "the log around the indicated line for the underlying "
                    "tool failure."
                ),
                verify_command=None,
            )
