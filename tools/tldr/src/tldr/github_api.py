# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""Thin GitHub API wrappers used by fetch.py and the (future) publisher.

Shells out to the ``gh`` CLI rather than constructing HTTP requests
directly — ``gh`` already handles auth, retries, and rate-limit waits, and
is required infrastructure for ``--post`` anyway.

These helpers are *small and unit-testable*. ``fetch.py`` and ``publish.py``
take a callable so tests can swap the subprocess shim.
"""

from __future__ import annotations

import json
import subprocess
from dataclasses import dataclass
from typing import Callable, Iterable

from ._paginate import iter_objects


@dataclass(frozen=True)
class CheckLike:
    """Either a check-run (GitHub Actions) or a status (Jenkins)."""

    name: str
    conclusion: str  # success / failure / neutral / skipped / cancelled / ""
    details_url: str | None
    log_hint: str | None  # for Jenkins, the consoleText URL; for GHA, an API path


def _gh(
    args: list[str], runner: Callable[[list[str]], tuple[int, str]] | None = None
) -> str:
    runner = runner or _default_runner
    rc, out = runner(["gh"] + args)
    if rc != 0:
        raise RuntimeError(f"gh {args[0]} failed (rc={rc}): {out}")
    return out


def _default_runner(cmd: list[str]) -> tuple[int, str]:
    result = subprocess.run(
        cmd, capture_output=True, text=True, check=False, timeout=120
    )
    return result.returncode, (result.stdout or "") + (
        "" if result.returncode == 0 else result.stderr or ""
    )


def discover_failing_checks(
    repo: str,
    sha: str,
    *,
    runner: Callable[[list[str]], tuple[int, str]] | None = None,
) -> list[CheckLike]:
    """Return failing checks for `repo@sha` from both check-runs and statuses."""
    out: list[CheckLike] = []

    # GitHub check-runs.
    raw = _gh(
        [
            "api",
            f"/repos/{repo}/commits/{sha}/check-runs",
            "--paginate",
        ],
        runner=runner,
    )
    for data in iter_objects(raw):
        if not isinstance(data, dict):
            continue
        for cr in data.get("check_runs", []) or []:
            if (cr.get("conclusion") or "") in ("failure", "timed_out"):
                out.append(
                    CheckLike(
                        name=cr.get("name", ""),
                        conclusion=cr.get("conclusion", ""),
                        details_url=cr.get("details_url"),
                        log_hint=(
                            f"/repos/{repo}/actions/jobs/{cr.get('id')}/logs"
                            if cr.get("id") is not None
                            else None
                        ),
                    )
                )

    # Legacy commit statuses (used by Jenkins).
    raw = _gh(
        [
            "api",
            f"/repos/{repo}/commits/{sha}/statuses",
            "--paginate",
        ],
        runner=runner,
    )
    # Statuses API returns a list of statuses, latest first per context.
    seen: set[str] = set()
    for arr in iter_objects(raw):
        if not isinstance(arr, list):
            continue
        for st in arr:
            ctx = st.get("context", "")
            if ctx in seen:
                continue
            seen.add(ctx)
            if (st.get("state") or "") == "failure":
                target = st.get("target_url")
                console_text = (
                    f"{target.rstrip('/')}/consoleText"
                    if target and "jenkins" in (target or "")
                    else None
                )
                out.append(
                    CheckLike(
                        name=ctx,
                        conclusion="failure",
                        details_url=target,
                        log_hint=console_text,
                    )
                )

    return out


def get_pr(
    repo: str,
    pr: int,
    *,
    runner: Callable[[list[str]], tuple[int, str]] | None = None,
) -> dict:
    raw = _gh(
        ["api", f"/repos/{repo}/pulls/{pr}"],
        runner=runner,
    )
    return json.loads(raw)


def update_pr_body(
    repo: str,
    pr: int,
    body: str,
    *,
    runner: Callable[[list[str], str | None], tuple[int, str]] | None = None,
) -> None:
    """Patch the PR body in place.

    PR bodies can be up to ~64KB; passing them as a `-f body=…` argv flag
    risks blowing past `ARG_MAX` on some runners. We send the JSON payload
    via stdin instead (`gh api --input -`).
    """
    payload = json.dumps({"body": body})
    cmd = ["gh", "api", "-X", "PATCH", f"/repos/{repo}/pulls/{pr}", "--input", "-"]
    if runner is None:
        result = subprocess.run(
            cmd,
            input=payload,
            capture_output=True,
            text=True,
            check=False,
            timeout=120,
        )
        if result.returncode != 0:
            raise RuntimeError(
                f"gh api PATCH failed (rc={result.returncode}): "
                f"{result.stderr or result.stdout}"
            )
        return
    rc, out = runner(cmd, payload)
    if rc != 0:
        raise RuntimeError(f"gh api PATCH failed (rc={rc}): {out}")
