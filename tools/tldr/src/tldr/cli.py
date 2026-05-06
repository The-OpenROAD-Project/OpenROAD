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
from .parsers.base import Finding, Severity, StageContext


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

    # Materialise the *stripped* log so we can replay it through each parser.
    # We don't keep `raw` around — Jenkins consoleText commonly runs hundreds
    # of MB and holding both raw and stripped doubles peak memory on the
    # runner. Stream-strip line-by-line and discard the raw form.
    # If the log is gone (purged build, 404), surface that as a single
    # info-level finding rather than silently dropping the check.
    try:
        stripped = [
            strip.strip_line(l)
            for l in fetch.log_lines(check, runner=runner, url_opener=url_opener)
        ]
    except fetch.LogUnavailable as e:
        return [
            Finding(
                parser="fetch",
                severity=Severity.info,
                kind="log_unavailable",
                headline=f"{check.name}: log unavailable"
                + (f" (HTTP {e.status})" if e.status else ""),
                detail=str(e),
                location=None,
                log_url=e.url,
                dedupe_key=f"log_unavailable:{check.name}",
                human_headline=(
                    f"`{check.name}` log is no longer available "
                    + (f"(HTTP {e.status})" if e.status else "(fetch error)")
                ),
                ai_directive=None,
                verify_command=None,
                auto_fix_command=None,
            )
        ]

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
            f = f.with_stage(sub_ctx.current)
            # Thread the failing-check's full URL into each finding so the
            # markdown render can link "see the full log" right next to
            # the finding.
            if f.log_url is None and check.details_url:
                f = _with_log_url(f, check.details_url)
            out.append(f)
    return out


def _with_log_url(f: Finding, url: str) -> Finding:
    """Return a copy of `f` with `log_url` set."""
    return Finding(
        parser=f.parser,
        severity=f.severity,
        kind=f.kind,
        headline=f.headline,
        detail=f.detail,
        stage=f.stage,
        location=f.location,
        log_url=url,
        log_line=f.log_line,
        dedupe_key=f.dedupe_key,
        human_headline=f.human_headline,
        ai_directive=f.ai_directive,
        verify_command=f.verify_command,
        auto_fix_command=f.auto_fix_command,
        extras=f.extras,
        collapse_in_human_tldr=f.collapse_in_human_tldr,
    )


def discover_findings(
    repo: str,
    sha: str,
    *,
    pr: int | None = None,
    runner=None,
    url_opener=None,
) -> tuple[list[Finding], list[str]]:
    """Return (findings, failing_check_urls).

    The URL list is for the renderer's "no findings extracted" fallback —
    when the discovery code reports failing checks but no parser produced
    a finding, we still want to point the user at the logs to look at.
    """
    checks = github_api.discover_failing_checks(repo, sha, runner=runner)
    findings: list[Finding] = []
    for ch in checks:
        findings.extend(_scan_check(repo, ch, runner=runner, url_opener=url_opener))
    if pr is not None:
        from . import review_comments

        findings.extend(review_comments.discover(repo, pr, runner=runner))
    failing_urls = [c.details_url for c in checks if c.details_url]
    return collapse(findings), failing_urls


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
        findings, failing_urls = discover_findings(repo, sha, pr=pr)
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
        sys.stdout.write(
            render_table.render(
                findings,
                pr=pr,
                sha=sha,
                repo=repo,
                failing_check_urls=failing_urls,
            )
        )

    return 0
