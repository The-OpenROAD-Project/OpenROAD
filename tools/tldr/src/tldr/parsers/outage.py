# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""Detect transient outages from the many external services CI pulls from.

OpenROAD's CI talks to a lot of third-party endpoints — Docker / container
registries (docker.io, ghcr.io, gcr.io, quay.io), apt mirrors, PyPI, npm,
crates.io, GitHub's own API, the Bazel remote cache, Jenkins agents, …
Any of them can flake at any time. This parser catalogues the common
"the world is broken, retry the job" signatures so contributors aren't
mis-led into chasing infra noise as if it were their bug.

Every finding it emits is ``Severity.infra``. The renderer puts these
under "Ignore (transient/infra)" and never lists them in the AI directive
section — there's nothing for an AI agent to fix.
"""

from __future__ import annotations

import re
from typing import Iterable

from .base import Finding, Severity, StageContext

# (compiled-regex, source-label, optional human hint).
# Order matters only in that more-specific patterns should come first.
_PATTERNS: list[tuple[re.Pattern[str], str, str | None]] = [
    # Container registries (docker.io, ghcr.io, gcr.io, quay.io …).
    (
        re.compile(r"Error response from daemon:\s+failed to resolve reference"),
        "container-registry",
        "image not yet promoted or registry flake",
    ),
    (
        re.compile(r"Error response from daemon:\s+Get .*TLS handshake timeout"),
        "container-registry",
        "TLS handshake timeout",
    ),
    (
        re.compile(r"\bmanifest .*not found: manifest unknown\b"),
        "container-registry",
        "manifest unknown",
    ),
    (
        re.compile(r"Error response from daemon:\s+pull access denied"),
        "container-registry",
        "transient auth flake",
    ),
    (
        re.compile(r"\bdenied:\s+requested access to the resource is denied\b"),
        "container-registry",
        "transient auth flake",
    ),
    (
        re.compile(r"unauthorized: authentication required"),
        "container-registry",
        "transient auth flake",
    ),
    (
        # Jenkins docker-workflow plugin couldn't `docker run` the image.
        # Seen in three guises on PR #10340 (`Caught exception: ...`,
        # `ERROR: A fatal error occurred ...`, and a bare
        # `java.io.IOException: ...`); match the common substring instead
        # of any one wrapper.
        re.compile(r"Failed to run image\s+'(?P<img>[^']+)'"),
        "container-registry",
        "Jenkins could not run the build container",
    ),
    # Jenkins-side workflow exceptions (infra, not user code). The
    # `ErrorAction$ErrorId` marker tags an exception that Jenkins itself
    # threw — typically a node disconnect or a step that crashed before
    # any pipeline `error` was emitted.
    (
        re.compile(
            r"Also:\s+org\.jenkinsci\.plugins\.workflow\.actions\."
            r"ErrorAction\$ErrorId:"
        ),
        "jenkins-pipeline-error",
        "Jenkins workflow threw before user code ran",
    ),
    # GitHub API itself.
    (
        re.compile(
            r"\b(?:api\.github\.com|github\.com)\b.*\b5(?:02|03|04)\b\s+"
            r"(?:Bad Gateway|Service Unavailable|Gateway Timeout)"
        ),
        "github-api",
        "GitHub returned 5xx",
    ),
    (
        re.compile(r"\bgh:\s+failed to (?:resolve|fetch) .*github\.com"),
        "github-api",
        None,
    ),
    # apt mirrors.
    (
        re.compile(
            r"Failed to fetch https?://(?:archive|security|.*?\.archive)\."
            r"(?:ubuntu|debian)\.org"
        ),
        "apt-mirror",
        None,
    ),
    (
        re.compile(r"^E:\s+Unable to fetch some archives"),
        "apt-mirror",
        None,
    ),
    # PyPI / pip.
    (
        re.compile(r"pip\._vendor\.urllib3\.exceptions\.ReadTimeoutError"),
        "pypi",
        None,
    ),
    (
        re.compile(r"WARNING: Retrying \(Retry\(.*\)\) after connection broken"),
        "pypi",
        None,
    ),
    (
        re.compile(
            r"Could not fetch URL https?://(?:pypi\.org|files\.pythonhosted\.org)"
        ),
        "pypi",
        None,
    ),
    # npm.
    (
        re.compile(r"npm ERR!\s+(?:network|ETIMEDOUT|ECONNRESET|503|EAI_AGAIN)"),
        "npm",
        None,
    ),
    # crates.io / cargo.
    (
        re.compile(r"error: failed to fetch .*crates\.io"),
        "crates-io",
        None,
    ),
    # Bazel remote cache.
    (
        re.compile(r"WARNING: Remote Cache:.*\bUNAVAILABLE\b"),
        "bazel-cache",
        "remote cache unavailable",
    ),
    (
        re.compile(r"\b(?:BulkTransferException|RemoteCacheException)\b"),
        "bazel-cache",
        None,
    ),
    # Jenkins agent disconnect.
    (
        re.compile(r"FATAL: Remote call on .* failed"),
        "jenkins-agent",
        None,
    ),
    (
        re.compile(r"hudson\.remoting\.RequestAbortedException"),
        "jenkins-agent",
        None,
    ),
    (
        re.compile(r"Cannot contact .*: java\.io\.IOException"),
        "jenkins-agent",
        None,
    ),
    # Disk.
    (
        re.compile(r"\bNo space left on device\b"),
        "ci-runner-disk",
        "runner ran out of disk — usually transient if no leak",
    ),
    # Generic network errors. Keep these last (broadest patterns).
    (
        re.compile(r"\bTemporary failure in name resolution\b"),
        "dns",
        None,
    ),
    (
        re.compile(r"\bgetaddrinfo\b.*\bEAI_AGAIN\b"),
        "dns",
        None,
    ),
    (
        re.compile(r"\bCould not resolve host\b"),
        "dns",
        None,
    ),
    (
        re.compile(r"\bConnection (?:refused|reset by peer|timed out)\b"),
        "network",
        None,
    ),
    (
        re.compile(r"\bdial tcp.*\bnetwork is unreachable\b"),
        "network",
        None,
    ),
    (
        re.compile(r"\b(?:i/o timeout|TLS handshake timeout)\b"),
        "network",
        None,
    ),
]


class OutageParser:
    name = "outage"

    def applies(self, check_name: str, repo: str) -> bool:
        return True

    def scan(self, lines: Iterable[str], ctx: StageContext) -> Iterable[Finding]:
        # Dedupe by (source, stage) so a single Connection-refused storm
        # surfaces as one finding per stage rather than hundreds of rows.
        seen: set[tuple[str, str | None]] = set()
        for i, line in enumerate(lines, start=1):
            for pat, source, hint in _PATTERNS:
                m = pat.search(line)
                if not m:
                    continue
                key = (source, ctx.current)
                if key in seen:
                    break
                seen.add(key)
                headline = f"{source}: {hint}" if hint else f"{source}: outage"
                yield Finding(
                    parser=self.name,
                    severity=Severity.infra,
                    kind="outage",
                    headline=headline,
                    detail=line.strip(),
                    stage=ctx.current,
                    log_line=i,
                    dedupe_key=f"outage:{source}:{ctx.current or '-'}",
                    human_headline=(
                        f"`{source}` outage in `{ctx.current or '?'}` "
                        f"({hint or 'transient external dependency'})"
                    ),
                    ai_directive=None,  # infra — nothing for an agent to fix
                    verify_command=None,
                    auto_fix_command=None,
                )
                break
