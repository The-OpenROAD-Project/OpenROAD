# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""Entry point for `//:fix-tldr` — deterministic auto-fix runner.

Runs the same diagnosis pipeline as ``//:tldr`` and then, for each finding
that ships an ``auto_fix_command``, shells out to that command in the user's
checkout. **AI-free**: no LLM, no agent. Findings without a recipe are
listed under "Still needs you" and the command exits non-zero.

Side effects: only local file edits performed by the same formatters and
regen scripts a contributor would otherwise type by hand.
"""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
from typing import Sequence

from tldr import local
from tldr.cli import discover_findings, _resolve_sha_for_pr
from tldr.parsers.base import Finding, Severity


def _build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        prog="fix-tldr",
        description=(
            "Run deterministic local fix commands for the subset of //:tldr "
            "findings that have a known recipe. AI-free; findings that "
            "need human judgment are listed and the command exits non-zero."
        ),
    )
    p.add_argument("--pr", type=int, default=None)
    p.add_argument("--sha", type=str, default=None)
    p.add_argument("--repo", type=str, default=None)
    return p


def _run_recipes(
    findings: list[Finding],
    *,
    runner=None,
    cwd: str | None = None,
) -> tuple[list[tuple[Finding, int]], list[Finding]]:
    """Execute each finding's auto_fix_command. Returns (results, leftover).

    `results` is a list of (finding, return-code). `leftover` is the list of
    actionable findings that had no `auto_fix_command`.
    """
    runner = runner or subprocess.run
    results: list[tuple[Finding, int]] = []
    leftover: list[Finding] = []
    for f in findings:
        if f.severity is not Severity.actionable:
            continue
        if not f.auto_fix_command:
            leftover.append(f)
            continue
        rc = runner(list(f.auto_fix_command), cwd=cwd, check=False).returncode
        results.append((f, rc))
    return results, leftover


def _print_summary(
    results: list[tuple[Finding, int]],
    leftover: list[Finding],
) -> None:
    if results:
        print(f"Fixed: {len(results)} recipe(s) ran.")
        for f, rc in results:
            tag = "ok" if rc == 0 else f"FAILED rc={rc}"
            cmd = " ".join(f.auto_fix_command or ())
            print(f"  [{tag}] {f.kind}: {cmd}")
    else:
        print("Fixed: 0 recipes (no auto-fixable findings).")

    if leftover:
        print()
        print(f"Still needs you: {len(leftover)} finding(s) need human judgment.")
        for f in leftover:
            stage = f.stage or "-"
            print(f"  {f.severity.value:<10} {stage:<24} {f.kind:<20} {f.headline}")
    else:
        print()
        print("No findings need human judgment.")


def main(argv: Sequence[str] | None = None) -> int:
    args = _build_parser().parse_args(argv)

    repo = args.repo
    pr = args.pr
    sha = args.sha
    if pr is None and sha is None:
        ctx = local.resolve()
        repo = repo or ctx.repo
        pr = ctx.pr
        sha = ctx.sha
    else:
        repo = repo or local.infer_repo()

    if not sha and pr is not None:
        try:
            sha = _resolve_sha_for_pr(repo, pr)
        except Exception as e:  # noqa: BLE001
            print(f"error: failed to resolve SHA for PR #{pr}: {e}", file=sys.stderr)
            return 2

    if not sha:
        print(
            "error: could not determine a SHA. Pass --pr or --sha, or run "
            "from a checkout with a HEAD commit.",
            file=sys.stderr,
        )
        return 2

    try:
        findings = discover_findings(repo, sha)
    except Exception as e:  # noqa: BLE001
        print(f"error: failed to discover findings: {e}", file=sys.stderr)
        return 1

    cwd = os.environ.get("BUILD_WORKING_DIRECTORY")  # set by `bazelisk run`
    results, leftover = _run_recipes(findings, cwd=cwd)
    _print_summary(results, leftover)

    any_recipe_failed = any(rc != 0 for _, rc in results)
    has_leftover = bool(leftover)
    return 0 if (not has_leftover and not any_recipe_failed) else 1


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
