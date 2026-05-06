# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""Parse Bazel test failures.

Bazel summarises failures with lines like::

    //src/rsz:test_buffer_ports                                              FAILED in 12.3s
"""

from __future__ import annotations

import re
from typing import Iterable

from .base import Finding, Severity, StageContext

# Bazel target: optional leading `@<repo>` (or `@@<canonical>` for Bzlmod
# resolved-repo names, optionally with version suffix `+1.2.3` or `~tilde`),
# followed by `//<path>:<name>`.
_PATTERN = re.compile(
    r"^(?P<target>(?:@@?[A-Za-z0-9_.\-+~]+)?//[A-Za-z0-9_./:\-+~]+)"
    r"\s+(?:\(.*\)\s+)?FAILED\b(?:\s+in\s+\S+)?"
)


class BazelTestParser:
    name = "bazel_test"

    def applies(self, check_name: str, repo: str) -> bool:
        return True

    def scan(self, lines: Iterable[str], ctx: StageContext) -> Iterable[Finding]:
        seen: set[str] = set()
        for i, line in enumerate(lines, start=1):
            m = _PATTERN.match(line)
            if not m:
                continue
            target = m["target"]
            if target in seen:
                continue
            seen.add(target)
            yield Finding(
                parser=self.name,
                severity=Severity.actionable,
                kind="bazel_test_fail",
                headline=f"{target} FAILED",
                detail=line.strip(),
                stage=ctx.current,
                location=target,
                log_line=i,
                dedupe_key=f"bazel_test:{target}",
                human_headline=f"Bazel test failed: `{target}`",
                ai_directive=(
                    f"Bazel test `{target}` is failing. Reproduce locally with "
                    f"`bazelisk test {target}`. Read the test's stderr from "
                    "the Bazel cache (`bazel-testlogs/`) to see the actual "
                    "assertion failure; fix the underlying code rather than "
                    "loosening the test."
                ),
                verify_command=f"bazelisk test {target}",
            )
