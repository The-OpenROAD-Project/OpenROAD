# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""Resolve (repo, pr, sha) for the zero-arg local invocation.

When the user types ``bazelisk run //:tldr`` with no flags, we want to look
at the current branch they're sitting on, derive the open PR for it (via
``gh pr view``), and use that PR's head SHA as the target. Falls back to
``HEAD`` of the inferred remote if no PR is open.
"""

from __future__ import annotations

import json
import re
import subprocess
from dataclasses import dataclass


@dataclass(frozen=True)
class LocalContext:
    repo: str
    pr: int | None
    sha: str
    source: str  # "gh-pr-view" | "git-head" | "explicit"


_SSH_REMOTE = re.compile(r"git@github\.com:(?P<owner>[^/]+)/(?P<repo>[^/.]+)(?:\.git)?$")
_HTTPS_REMOTE = re.compile(
    r"https?://github\.com/(?P<owner>[^/]+)/(?P<repo>[^/.]+?)(?:\.git)?/?$"
)

# When invoked from any repo, default to upstream OpenROAD if no remote
# matches GitHub at all.
DEFAULT_REPO = "The-OpenROAD-Project/OpenROAD"


def _run(cmd: list[str]) -> tuple[int, str]:
    try:
        result = subprocess.run(
            cmd, capture_output=True, text=True, check=False, timeout=30
        )
        return result.returncode, result.stdout.strip()
    except (FileNotFoundError, subprocess.TimeoutExpired):
        return 127, ""


def _git_head() -> str | None:
    rc, out = _run(["git", "rev-parse", "HEAD"])
    return out if rc == 0 and out else None


def _git_remote_url(name: str = "origin") -> str | None:
    rc, out = _run(["git", "remote", "get-url", name])
    return out if rc == 0 and out else None


def parse_repo_from_url(url: str) -> str | None:
    for pat in (_SSH_REMOTE, _HTTPS_REMOTE):
        m = pat.search(url)
        if m:
            return f"{m['owner']}/{m['repo']}"
    return None


def infer_repo() -> str:
    # Prefer `origin` (upstream by convention); if it doesn't look like the
    # upstream OpenROAD repo, fall through to DEFAULT_REPO.
    url = _git_remote_url("origin")
    if url:
        repo = parse_repo_from_url(url)
        if repo and repo.lower().startswith("the-openroad-project/"):
            return repo
        if repo:
            # Some other GitHub remote (e.g. fork). Fall back to upstream.
            return DEFAULT_REPO
    return DEFAULT_REPO


def _gh_pr_view() -> dict | None:
    rc, out = _run(
        [
            "gh",
            "pr",
            "view",
            "--json",
            "number,headRefOid,headRepository,baseRepository",
        ]
    )
    if rc != 0 or not out:
        return None
    try:
        return json.loads(out)
    except json.JSONDecodeError:
        return None


def resolve() -> LocalContext:
    """Return a best-effort (repo, pr, sha) for the current checkout."""
    info = _gh_pr_view()
    if info is not None:
        base = info.get("baseRepository") or {}
        owner_obj = base.get("owner") or {}
        owner = owner_obj.get("login")
        name = base.get("name")
        repo = f"{owner}/{name}" if owner and name else infer_repo()
        return LocalContext(
            repo=repo,
            pr=info.get("number"),
            sha=info.get("headRefOid") or _git_head() or "",
            source="gh-pr-view",
        )

    sha = _git_head()
    if sha:
        return LocalContext(
            repo=infer_repo(), pr=None, sha=sha, source="git-head"
        )
    return LocalContext(repo=infer_repo(), pr=None, sha="", source="git-head")
