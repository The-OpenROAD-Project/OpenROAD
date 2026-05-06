# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""Parse ORFS metric-guard failures.

Example::

    [ERROR] finish__timing__setup__tns fail test: -1.21838 >= -0.685
"""

from __future__ import annotations

import re
from typing import Iterable

from .base import Finding, Severity, StageContext

_PATTERN = re.compile(
    r"^\s*\[ERROR\]\s+(?P<metric>\S+)\s+fail test:\s+"
    r"(?P<actual>-?\d+(?:\.\d+)?)\s*(?P<op>>=|<=|>|<|==|!=)\s*"
    r"(?P<bound>-?\d+(?:\.\d+)?)"
)


class MetricGuardParser:
    name = "metric_guard"

    def applies(self, check_name: str, repo: str) -> bool:
        return True

    def scan(self, lines: Iterable[str], ctx: StageContext) -> Iterable[Finding]:
        for i, line in enumerate(lines, start=1):
            m = _PATTERN.match(line)
            if not m:
                continue
            metric = m["metric"]
            yield Finding(
                parser=self.name,
                severity=Severity.actionable,
                kind="metric_guard",
                headline=f"{metric} {m['actual']} {m['op']} {m['bound']}",
                detail=line.strip(),
                stage=ctx.current,
                log_line=i,
                dedupe_key=f"metric_guard:{metric}",
                human_headline=(
                    f"Metric guard tripped: `{metric}` "
                    f"({m['actual']} vs. baseline {m['bound']})"
                ),
                ai_directive=(
                    f"The QoR metric `{metric}` regressed past its baseline "
                    f"({m['actual']} {m['op']} {m['bound']}). Identify which "
                    "change in this PR caused the regression and fix the root "
                    "cause; do not relax the baseline. Bit-determinism is a "
                    "load-bearing project requirement (see CLAUDE.md)."
                ),
                verify_command="make metrics",
            )
