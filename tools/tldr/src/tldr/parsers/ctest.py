# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""Parse CTest failure summaries.

CTest emits a "The following tests FAILED:" header followed by lines like::

      42 - my_test (Failed)
"""

from __future__ import annotations

import re
from typing import Iterable

from .base import Finding, Severity, StageContext

_HEADER = re.compile(r"^The following tests FAILED:")
_ROW = re.compile(r"^\s*(?P<num>\d+)\s+-\s+(?P<name>\S+)\s+\((?P<reason>[^)]+)\)\s*$")


class CtestParser:
    name = "ctest"

    def applies(self, check_name: str, repo: str) -> bool:
        return True

    def scan(
        self, lines: Iterable[str], ctx: StageContext
    ) -> Iterable[Finding]:
        in_block = False
        for i, line in enumerate(lines, start=1):
            if _HEADER.match(line):
                in_block = True
                continue
            if not in_block:
                continue
            m = _ROW.match(line)
            if not m:
                # End of block once we hit a non-row line.
                if line.strip() == "":
                    continue
                in_block = False
                continue
            name = m["name"]
            yield Finding(
                parser=self.name,
                severity=Severity.actionable,
                kind="ctest_fail",
                headline=f"{name} ({m['reason']})",
                detail=line.strip(),
                stage=ctx.current,
                location=name,
                log_line=i,
                dedupe_key=f"ctest:{name}",
                human_headline=f"CTest failed: `{name}`",
                ai_directive=(
                    f"CTest case `{name}` failed with reason "
                    f"`{m['reason']}`. Reproduce with `ctest -R ^{name}$ -V` "
                    "from the build directory and fix the underlying issue."
                ),
                verify_command=f"ctest -R ^{name}$ -V",
            )
