# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""Surface PR review comments (Gemini Code Assist, human reviewers, etc.) as
``Finding`` records, alongside the failing-CI findings produced by
``cli.discover_findings``.

Two GitHub endpoints feed this module:

* ``/repos/{repo}/pulls/{pr}/comments`` — *inline* code-review comments
  (one per file/line). These have ``path`` and ``line`` and are the
  load-bearing actionable signal.
* ``/repos/{repo}/pulls/{pr}/reviews`` — *top-level* review summaries.
  Surfaced as a single info-level finding so the contributor knows
  someone left a review without flooding the table.

Severity is taken from a Gemini-style markdown image badge in the body
(``![high](...)``, ``![medium](...)``, ``![low](...)``). Comments without
a badge default to ``actionable`` — a reviewer wouldn't be commenting if
they didn't want something done.

Outdated comments (where the line-of-comment was deleted in a later push,
indicated by GitHub setting ``position: null``) are dropped; surfacing
them would just be noise.
"""

from __future__ import annotations

import re
import subprocess
from typing import Callable, Iterable

from ._paginate import flatten_arrays
from .parsers.base import Finding, Severity

Runner = Callable[[list[str]], tuple[int, str]]


_PRIORITY_BADGE = re.compile(
    r"!\[(?P<level>critical|high|medium|low)\]\([^)]*\)",
    re.IGNORECASE,
)


def _default_runner(cmd: list[str]) -> tuple[int, str]:
    result = subprocess.run(
        cmd, capture_output=True, text=True, check=False, timeout=120
    )
    return result.returncode, (result.stdout or "")


def discover(
    repo: str,
    pr: int,
    *,
    runner: Runner | None = None,
) -> Iterable[Finding]:
    """Yield review-comment findings for the given PR."""
    runner = runner or _default_runner

    # Inline comments (per-file/line).
    rc, out = runner(["gh", "api", f"/repos/{repo}/pulls/{pr}/comments", "--paginate"])
    if rc == 0:
        for c in flatten_arrays(out):
            f = _from_inline_comment(c)
            if f is not None:
                yield f

    # Top-level reviews.
    rc, out = runner(["gh", "api", f"/repos/{repo}/pulls/{pr}/reviews", "--paginate"])
    if rc == 0:
        for r in flatten_arrays(out):
            f = _from_review(r)
            if f is not None:
                yield f


def _from_inline_comment(c: dict) -> Finding | None:
    body = (c.get("body") or "").strip()
    if not body:
        return None
    # GitHub sets position to None when the diff has moved past this
    # comment's hunk. The comment is no longer applicable; skip it.
    if c.get("position") is None and not c.get("line"):
        return None
    path = c.get("path") or ""
    line = c.get("line") or c.get("original_line") or "?"
    user = ((c.get("user") or {}).get("login")) or "?"
    severity = _severity_from_body(body)
    headline = _first_meaningful_line(body)
    location = f"{path}:{line}" if path else None
    return Finding(
        parser="review_comment",
        severity=severity,
        kind="review_comment",
        headline=f"{user} @ {path}:{line}: {headline}",
        detail=body,
        location=location,
        log_url=c.get("html_url"),
        dedupe_key=f"review_comment:{c.get('id')}",
        human_headline=f"`{user}` on `{path}:{line}` — {headline}",
        ai_directive=(
            f"A reviewer ({user}) left this comment on `{path}:{line}` "
            "and it needs to be addressed:\n\n" + body
        ),
        verify_command=None,
        auto_fix_command=None,
        collapse_in_human_tldr=False,
    )


def _from_review(r: dict) -> Finding | None:
    body = (r.get("body") or "").strip()
    if not body:
        return None
    state = (r.get("state") or "COMMENTED").upper()
    if state in ("APPROVED", "DISMISSED"):
        return None
    user = ((r.get("user") or {}).get("login")) or "?"
    severity = _severity_from_body(body)
    # Top-level reviews are usually a meta-summary; mark info unless the
    # reviewer explicitly attached a high/critical badge.
    if severity is not Severity.actionable or not _has_priority_badge(body):
        severity = Severity.info
    headline = _first_meaningful_line(body)
    return Finding(
        parser="review_comment",
        severity=severity,
        kind="review_summary",
        headline=f"{user} ({state.lower()}): {headline}",
        detail=body[:2000],
        location=None,
        log_url=r.get("html_url"),
        dedupe_key=f"review_summary:{r.get('id')}",
        human_headline=f"`{user}` posted a top-level review ({state.lower()})",
        ai_directive=(
            f"{user} left a top-level review ({state.lower()}) on this PR. "
            "Read it and act on the points raised:\n\n" + body
        ),
        verify_command=None,
        auto_fix_command=None,
        collapse_in_human_tldr=False,
    )


def _severity_from_body(body: str) -> Severity:
    levels = {m.group("level").lower() for m in _PRIORITY_BADGE.finditer(body)}
    if "critical" in levels or "high" in levels or "medium" in levels:
        return Severity.actionable
    if "low" in levels:
        return Severity.info
    # No explicit badge — treat as actionable; a reviewer commented on it.
    return Severity.actionable


def _has_priority_badge(body: str) -> bool:
    return _PRIORITY_BADGE.search(body) is not None


def _first_meaningful_line(body: str) -> str:
    cleaned = _PRIORITY_BADGE.sub("", body)
    for line in cleaned.splitlines():
        line = line.strip()
        if not line:
            continue
        if line.startswith("```") or line.startswith("![") or line == "---":
            continue
        return line[:120]
    return body[:120]
