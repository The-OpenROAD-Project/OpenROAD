# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""Render findings as the sentinel-bracketed PR-comment Markdown block.

The block has two stacked sections:

1. **TL;DR for humans** — 3–6 plain-English bullets a contributor can scan in
   five seconds.
2. **Instructions for an AI coding tool** — a structured agent prompt with
   project norms, per-finding directives, verification commands, and a
   "done when" line.

The bracketed span is delimited by ``<!-- tldr:begin -->`` …
``<!-- tldr:end -->``. The publisher (``publish.py``) replaces exactly that
span when upserting into a PR body.
"""

from __future__ import annotations

from collections import OrderedDict
from typing import Iterable

from .parsers.base import Finding, Severity

BEGIN = "<!-- tldr:begin -->"
END = "<!-- tldr:end -->"


# Source of truth: tools/OpenROAD/CLAUDE.md. A unit test asserts these bullets
# match the rules in CLAUDE.md so the two cannot drift.
PROJECT_NORMS = [
    "Sign every commit with `git commit -s` (DCO compliance).",
    (
        "Trace bugs to their root cause; do not silence symptoms or add "
        "workarounds. Find the data creation point, not the serialization "
        "point. (CLAUDE.md rule 5.)"
    ),
    "Do not edit files under `src/sta/` without explicit confirmation.",
    (
        "Run `clang-format -i` on every touched C++ file before committing "
        "(skip `src/sta/*` and `*.i` files)."
    ),
    (
        "OpenROAD must be bit-deterministic — no wallclock budgets or "
        "time-based heuristics."
    ),
    (
        "After fixing, re-summarize with `bazelisk run //:tldr` and confirm "
        "zero `actionable` findings before pushing."
    ),
]


def _human_bullets(findings: list[Finding]) -> list[str]:
    """Collapse same-kind findings into one bullet, except for findings that
    opt out via ``collapse_in_human_tldr=False`` (e.g. review comments)."""
    by_kind: OrderedDict[str, list[Finding]] = OrderedDict()
    for f in findings:
        by_kind.setdefault(f.kind, []).append(f)

    bullets: list[str] = []
    for kind, group in by_kind.items():
        # Findings that opt out of collapsing — every member gets its own bullet.
        if not group[0].collapse_in_human_tldr:
            for f in group:
                if f.human_headline:
                    bullets.append(f"- {f.human_headline}.")
                else:
                    bullets.append(f"- `{f.kind}`: {f.headline}")
            continue

        first = group[0]
        if first.human_headline:
            if len(group) == 1:
                bullets.append(f"- {first.human_headline}.")
            else:
                bullets.append(
                    f"- {first.human_headline} (and {len(group) - 1} more "
                    f"of kind `{kind}`)."
                )
        else:
            if len(group) == 1:
                bullets.append(f"- `{kind}`: {first.headline}")
            else:
                bullets.append(f"- `{kind}` × {len(group)}: {first.headline}, …")
    return bullets


def _ai_section(findings: list[Finding], pr_url: str | None) -> list[str]:
    actionable = [f for f in findings if f.severity is Severity.actionable]
    infra = [f for f in findings if f.severity is Severity.infra]

    out: list[str] = ["### Instructions for an AI coding tool", ""]
    if pr_url:
        out.append(
            f"You are an AI coding agent. The PR at <{pr_url}> has failing "
            "CI. Fix it."
        )
    else:
        out.append(
            "You are an AI coding agent. The PR linked from this comment "
            "has failing CI. Fix it."
        )
    out.append("")
    out.append("**Project norms (load-bearing):**")
    for n in PROJECT_NORMS:
        out.append(f"- {n}")
    out.append("")

    if actionable:
        out.append("**What to fix (in order):**")
        out.append("")
        for idx, f in enumerate(actionable, start=1):
            directive = f.ai_directive or f.headline
            verify = f" Verify with `{f.verify_command}`." if f.verify_command else ""
            location = f" at `{f.location}`" if f.location else ""
            out.append(f"{idx}. **{f.kind}**{location}: {directive}{verify}")
            out.append("")

    if infra:
        ignore_lines = ", ".join(f"`{f.kind}` ({f.headline})" for f in infra)
        out.append(f"**Ignore (transient/infra):** {ignore_lines}.")
        out.append("")

    out.append(
        "**Done when:** every actionable item above is fixed, "
        "`bazelisk test //...` passes locally, and `bazelisk run //:tldr` "
        "reports zero `actionable` findings."
    )
    return out


def _findings_detail(findings: list[Finding]) -> list[str]:
    out: list[str] = ["### Findings (full detail)", ""]
    for f in findings:
        loc = f" — `{f.location}`" if f.location else ""
        stage = f" *(stage: {f.stage})*" if f.stage else ""
        out.append(f"**{f.severity.value}** `{f.kind}`{loc}{stage}")
        out.append("")
        out.append(f"{f.headline}")
        if f.detail and f.detail != f.headline:
            out.append("")
            out.append("```")
            out.append(f.detail)
            out.append("```")
        out.append("")
    return out


def render(
    findings: Iterable[Finding],
    *,
    sha: str | None = None,
    pr_url: str | None = None,
) -> str:
    findings = list(findings)
    actionable = sum(1 for f in findings if f.severity is Severity.actionable)
    infra = sum(1 for f in findings if f.severity is Severity.infra)

    sha_label = f"`{sha[:7]}`" if sha else "(unknown SHA)"
    title = f"## CI summary for {sha_label} — {actionable} actionable, {infra} infra"

    parts: list[str] = [BEGIN, title, ""]

    parts.append("### TL;DR (humans)")
    parts.append("")
    if findings:
        parts.extend(_human_bullets(findings))
    else:
        parts.append("- No findings — CI is clean for this SHA.")
    parts.append("")
    if pr_url:
        parts.append(
            f"To delegate the fix: paste `fix {pr_url}` into your AI coding "
            "tool; it will read the section below."
        )
        parts.append("")

    parts.extend(_ai_section(findings, pr_url))
    parts.append("")
    if findings:
        parts.extend(_findings_detail(findings))
    parts.append(END)
    return "\n".join(parts) + "\n"
