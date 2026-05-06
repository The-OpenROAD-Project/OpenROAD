# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""Parse ORFS flow-test failure lines: ERROR: Test <design> <platform> failed."""

from __future__ import annotations

import re
from typing import Iterable

from .base import Finding, Severity, StageContext

_PATTERN = re.compile(r"^ERROR: Test (?P<design>\S+) (?P<platform>\S+) failed\b")


class OrfsFlowParser:
    name = "orfs_flow"

    def applies(self, check_name: str, repo: str) -> bool:
        return repo.endswith("OpenROAD-flow-scripts") or "jenkins" in check_name

    def scan(
        self, lines: Iterable[str], ctx: StageContext
    ) -> Iterable[Finding]:
        for i, line in enumerate(lines, start=1):
            m = _PATTERN.match(line)
            if not m:
                continue
            design, platform = m["design"], m["platform"]
            yield Finding(
                parser=self.name,
                severity=Severity.actionable,
                kind="flow_test_fail",
                headline=f"Test {design} {platform} failed",
                detail=line.strip(),
                stage=ctx.current,
                location=f"designs/{platform}/{design}/config.mk",
                log_line=i,
                dedupe_key=f"orfs_flow:{design}:{platform}",
                human_headline=f"Flow test {design}/{platform} failed",
                ai_directive=(
                    f"The flow test for design `{design}` on `{platform}` is failing. "
                    f"Reproduce locally with "
                    f"`make DESIGN_CONFIG=designs/{platform}/{design}/config.mk`. "
                    "Diff the failing stage's logs against `master` for the same "
                    "design to localise the regression. Trace the bug upstream "
                    "to its root cause; do not patch the symptom."
                ),
                verify_command=(
                    f"make DESIGN_CONFIG=designs/{platform}/{design}/config.mk"
                ),
            )
