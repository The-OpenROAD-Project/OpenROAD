# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""Parse OpenROAD module messages: [ERROR XYZ-NNNN] ..."""

from __future__ import annotations

import re
from typing import Iterable

from .base import Finding, Severity, StageContext

_PATTERN = re.compile(
    r"^\[(?P<level>ERROR|WARNING)\s+(?P<code>[A-Z]{2,5}-\d{4})\]\s+(?P<msg>.+)$"
)


class OpenroadMsgParser:
    name = "openroad_msg"

    def applies(self, check_name: str, repo: str) -> bool:
        return True

    def scan(self, lines: Iterable[str], ctx: StageContext) -> Iterable[Finding]:
        for i, line in enumerate(lines, start=1):
            m = _PATTERN.match(line)
            if not m:
                continue
            if m["level"] != "ERROR":
                continue
            code = m["code"]
            yield Finding(
                parser=self.name,
                severity=Severity.actionable,
                kind="openroad_error",
                headline=f"[{code}] {m['msg'].strip()}",
                detail=line.strip(),
                stage=ctx.current,
                log_line=i,
                dedupe_key=f"openroad_msg:{code}",
                human_headline=f"OpenROAD error `{code}`",
                ai_directive=(
                    f"OpenROAD raised error `{code}`. Look up its definition "
                    "via `find_messages.py` (in `etc/`) to find the source "
                    "module and the conditions that emit it. Fix the root "
                    "cause in the emitting module."
                ),
                verify_command=f"git grep -n {code!r} src/",
            )
