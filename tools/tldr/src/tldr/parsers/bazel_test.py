# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""Parse Bazel test failures.

Bazel summarises failures with lines like::

    //src/rsz:test_buffer_ports                                              FAILED in 12.3s

The actual stderr from a failing test is *not* directly under that line —
Bazel prints it inside an explicit block::

    ==================== Test output for //src/rsz:test_buffer_ports:
    AssertionError: 20 != 19 : ...
    FAILED (failures=1)
    ================================================================================

We do two passes:

1. Walk the lines once collecting (FAILED row, target → stage, line-no).
2. On the same pass, when we see ``==================== Test output for
   <target>:`` headers, capture lines up to the matching ``=========…``
   closing rule into that target's ``detail``.

That way the finding for ``//src/odb/test:odb_man_tcl_check`` carries the
``AssertionError: 20 != 19`` line that lives ~50 lines below the FAILED
summary on real logs (PR #10287).
"""

from __future__ import annotations

import re
from typing import Iterable

from .base import Finding, Severity, StageContext

# Bazel target: optional leading `@<repo>` (or `@@<canonical>` for Bzlmod
# resolved-repo names, optionally with version suffix `+1.2.3` or `~tilde`),
# followed by `//<path>:<name>`.
_PATTERN = re.compile(
    r"^\s*(?P<target>(?:@@?[A-Za-z0-9_.\-+~]+)?//[A-Za-z0-9_./:\-+~]+)"
    r"\s+(?:\(.*\)\s+)?FAILED\b(?:\s+in\s+\S+)?"
)

# `==================== Test output for //path:target:` block header.
_TEST_OUTPUT_HEADER = re.compile(
    r"^={4,}\s+Test output for\s+(?P<target>(?:@@?[A-Za-z0-9_.\-+~]+)?"
    r"//[A-Za-z0-9_./:\-+~]+)\s*:\s*$"
)

# Block end: a long `=` rule (Bazel's closing fence is 80 `=`s). Inner
# unittest output emits its own `=`-separator that's typically 70 wide; the
# 78-character lower bound below distinguishes the two.
_BLOCK_END = re.compile(r"^={78,}\s*$")

_DETAIL_LINES = 25


class BazelTestParser:
    name = "bazel_test"

    def applies(self, check_name: str, repo: str) -> bool:
        return True

    def scan(self, lines: Iterable[str], ctx: StageContext) -> Iterable[Finding]:
        # FAILED-summary records, keyed by target so re-runs (Bazel keep_going
        # prints the row twice) collapse to one finding.
        records: dict[str, dict] = {}
        order: list[str] = []
        # Detail blocks. Buffered unconditionally — Bazel emits the
        # "Test output for <target>:" block *before* the FAILED summary
        # in the same log, so we can't gate on records[].
        block_buffers: dict[str, list[str]] = {}
        capture_target: str | None = None

        for i, line in enumerate(lines, start=1):
            m = _PATTERN.match(line)
            if m:
                target = m["target"]
                if target not in records:
                    records[target] = {
                        "first": line,
                        "stage": ctx.current,
                        "line_no": i,
                    }
                    order.append(target)
                # FAILED summary lines are not inside a detail block.
                capture_target = None
                continue

            head = _TEST_OUTPUT_HEADER.match(line)
            if head:
                capture_target = head["target"]
                block_buffers.setdefault(capture_target, [])
                continue

            if capture_target is not None:
                if _BLOCK_END.match(line):
                    capture_target = None
                    continue
                buf = block_buffers[capture_target]
                if len(buf) < _DETAIL_LINES and line.strip():
                    buf.append(line)

        for target in order:
            rec = records[target]
            block = block_buffers.get(target, [])
            # Preserve leading whitespace inside the captured Test-output
            # block so stack traces / diffs / indented assertion messages
            # remain readable. Only the FAILED summary's own line gets
            # collapsed (it has fixed-width spacing for the FAILED column).
            detail_lines = [rec["first"].strip()] + [l for l in block if l.strip()]
            yield Finding(
                parser=self.name,
                severity=Severity.actionable,
                kind="bazel_test_fail",
                headline=f"{target} FAILED",
                detail="\n".join(detail_lines),
                stage=rec["stage"],
                location=target,
                log_line=rec["line_no"],
                dedupe_key=f"bazel_test:{target}",
                human_headline=f"Bazel test failed: `{target}`",
                ai_directive=(
                    f"Bazel test `{target}` is failing. Reproduce locally with "
                    f"`bazelisk test {target}`. The relevant stderr is in this "
                    "finding's detail; fix the underlying code rather than "
                    "loosening the test."
                ),
                verify_command=f"bazelisk test {target}",
            )
