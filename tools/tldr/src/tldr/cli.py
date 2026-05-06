# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""argparse-based CLI for `tldr`.

This file is also the entry point for the ``//:tldr`` py_binary via
``__main__.py``.
"""

from __future__ import annotations

import argparse
import sys
from typing import Iterable, Sequence

from . import (
    github_api,
    fetch,
    local,
    parsers as parsers_pkg,
    render_markdown,
    render_table,
    stages,
    strip,
)
from .dedupe import collapse
from .parsers.base import Finding, StageContext


class _LiveStageContext:
    """Wraps a `stages.StageTracker` so it presents the StageContext protocol."""

    def __init__(self, tracker: stages.StageTracker) -> None:
        self._tracker = tracker

    @property
    def current(self) -> str | None:
        return self._tracker.current


def _scan_check(
    repo: str,
    check: github_api.CheckLike,
    *,
    runner=None,
    url_opener=None,
) -> Iterable[Finding]:
    # Pull all parsers; they self-filter on (check_name, repo).
    plugins = [p for p in parsers_pkg.REGISTRY.values() if p.applies(check.name, repo)]
    if not plugins:
        return []

    # Materialise the log so we can replay it through each parser. The logs
    # are big but trimmed in tests, and in production we only fetch a given
    # check's log once.
    raw_lines = list(fetch.log_lines(check, runner=runner, url_opener=url_opener))
    stripped = [strip.strip_line(l) for l in raw_lines]

    out: list[Finding] = []
    for plugin in plugins:
        # Each plugin gets the entire stream through one `scan()` call so it
        # can maintain state across lines (e.g. CtestParser's `in_block`
        # flag) and `enumerate` advances correctly inside the parser. The
        # stage tracker is fed lazily by a generator wrapped around the
        # lines, so `sub_ctx.current` is accurate at the moment the parser
        # yields each finding.
        sub_tracker = stages.StageTracker()
        sub_ctx = _LiveStageContext(sub_tracker)

        def _tracked(t=sub_tracker):
            for line in stripped:
                t.feed(line)
                yield line

        for f in plugin.scan(_tracked(), sub_ctx):
            out.append(f.with_stage(sub_ctx.current))
    return out


def discover_findings(
    repo: str,
    sha: str,
    *,
    pr: int | None = None,
    runner=None,
    url_opener=None,
) -> list[Finding]:
    checks = github_api.discover_failing_checks(repo, sha, runner=runner)
    findings: list[Finding] = []
    for ch in checks:
        findings.extend(_scan_check(repo, ch, runner=runner, url_opener=url_opener))
    if pr is not None:
        from . import review_comments

        findings.extend(review_comments.discover(repo, pr, runner=runner))
    return collapse(findings)


def _resolve_sha_for_pr(repo: str, pr: int, *, runner=None) -> str:
    info = github_api.get_pr(repo, pr, runner=runner)
    return info.get("head", {}).get("sha") or ""


def build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        prog="tldr",
        description=(
            "Summarize CI failures for a pull request. "
            "Zero args: derive the current branch's open PR via git+gh and "
            "print a terminal table to stdout."
        ),
    )
    p.add_argument("--pr", type=int, default=None, help="PR number")
    p.add_argument("--sha", type=str, default=None, help="Head SHA (overrides --pr)")
    p.add_argument(
        "--repo",
        type=str,
        default=None,
        help=(
            "GitHub repo as OWNER/NAME. "
            "Default: inferred from `git remote get-url origin` or "
            f"`{local.DEFAULT_REPO}`."
        ),
    )
    p.add_argument(
        "--format",
        choices=("table", "markdown"),
        default=None,
        help="Output format. Default: table for terminal, markdown when --post.",
    )
    p.add_argument(
        "--target",
        choices=("body", "comment"),
        default="body",
        help="Where to upsert the bracketed block when --post is set.",
    )
    p.add_argument(
        "--post",
        action="store_true",
        help=(
            "Actually edit the PR body/comment. Off by default. "
            "Requires GH_TOKEN and a PR number."
        ),
    )
    return p


def main(argv: Sequence[str] | None = None) -> int:
    args = build_parser().parse_args(argv)

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
        except Exception as e:  # noqa: BLE001 — surface gh errors to the user
            print(f"error: failed to resolve SHA for PR #{pr}: {e}", file=sys.stderr)
            return 2

    if not sha:
        print(
            "error: could not determine a SHA. Pass --pr or --sha, or run "
            "from a checkout with a HEAD commit.",
            file=sys.stderr,
        )
        return 2

    fmt = args.format or ("markdown" if args.post else "table")

    try:
        findings = discover_findings(repo, sha, pr=pr)
    except Exception as e:  # noqa: BLE001
        print(f"error: failed to discover findings: {e}", file=sys.stderr)
        return 1

    pr_url = f"https://github.com/{repo}/pull/{pr}" if pr else None

    if fmt == "markdown":
        block = render_markdown.render(findings, sha=sha, pr_url=pr_url)
        if args.post:
            if pr is None:
                print("error: --post requires --pr", file=sys.stderr)
                return 2
            from . import publish

            try:
                pr_info = github_api.get_pr(repo, pr)
                new_body = publish.upsert(pr_info.get("body") or "", block)
                github_api.update_pr_body(repo, pr, new_body)
            except publish.MalformedBodyError as e:
                print(
                    f"error: refusing to write — PR body is malformed: {e}",
                    file=sys.stderr,
                )
                return 3
            print(block, end="")
        else:
            sys.stdout.write(block)
    else:
        sys.stdout.write(render_table.render(findings, pr=pr, sha=sha, repo=repo))

    return 0
