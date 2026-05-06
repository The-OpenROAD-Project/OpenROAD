# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""Render findings as a terminal-friendly ASCII table."""

from __future__ import annotations

import shutil
import sys
from typing import Iterable

from .parsers.base import Finding, Severity


def _ellipsize(s: str, width: int) -> str:
    if width <= 0:
        return ""
    if len(s) <= width:
        return s
    if width <= 1:
        return "…"
    return s[: width - 1] + "…"


def render(
    findings: Iterable[Finding],
    *,
    pr: int | None = None,
    sha: str | None = None,
    repo: str | None = None,
    term_width: int | None = None,
    failing_check_urls: list[str] | None = None,
) -> str:
    findings = list(findings)
    if term_width is None:
        term_width = shutil.get_terminal_size((100, 24)).columns
    term_width = max(80, term_width)

    actionable = sum(1 for f in findings if f.severity is Severity.actionable)
    infra = sum(1 for f in findings if f.severity is Severity.infra)

    header_bits: list[str] = []
    if pr is not None:
        header_bits.append(f"PR #{pr}")
    if sha:
        header_bits.append(sha[:7])
    header_bits.append(f"{actionable} actionable, {infra} infra")
    if repo:
        header_bits.append(repo)
    header = "  ".join(header_bits)

    if not findings:
        if failing_check_urls:
            # Distinguish "CI is clean" from "CI failed but we couldn't
            # extract a single finding from any log."
            urls_block = "\n".join(f"  - {u}" for u in failing_check_urls)
            return (
                f"{header}\n\n"
                f"Found {len(failing_check_urls)} failing check(s) but couldn't "
                "extract any actionable findings. Inspect the logs manually:\n"
                f"{urls_block}\n"
            )
        return f"{header}\n\nNo findings — CI is clean for this SHA.\n"

    cols = ("SEV", "STAGE", "KIND", "WHAT")
    sev_w = max(len(cols[0]), max(len(f.severity.value) for f in findings))
    stage_w = max(len(cols[1]), max(len(f.stage or "-") for f in findings))
    kind_w = max(len(cols[2]), max(len(f.kind) for f in findings))
    fixed = sev_w + stage_w + kind_w + 6  # 3 gaps × 2 spaces
    what_w = max(20, term_width - fixed)
    sep = "─" * min(term_width, fixed + what_w)

    out: list[str] = [header, sep]
    out.append(
        f"{cols[0]:<{sev_w}}  {cols[1]:<{stage_w}}  {cols[2]:<{kind_w}}  {cols[3]}"
    )
    out.append(sep)
    for f in findings:
        sev = f.severity.value
        stage = f.stage or "-"
        what = _ellipsize(f.headline, what_w)
        out.append(f"{sev:<{sev_w}}  {stage:<{stage_w}}  {f.kind:<{kind_w}}  {what}")

    out.append("")
    if pr is not None:
        out.append(
            f"Tip: paste `fix https://github.com/{repo or 'OWNER/REPO'}/pull/{pr}` "
            "into your AI coding tool to get a per-finding patch plan,"
        )
        out.append(
            "or run with --format markdown to preview the would-be PR-comment block."
        )
    return "\n".join(out) + "\n"


def to_stdout(findings: Iterable[Finding], **header_kwargs) -> None:
    sys.stdout.write(render(findings, **header_kwargs))
