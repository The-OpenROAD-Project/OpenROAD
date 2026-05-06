# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""Detect transient Docker-pull failures (image not yet promoted, etc.)."""

from __future__ import annotations

import re
from typing import Iterable

from .base import Finding, Severity, StageContext

_PATTERNS = [
    re.compile(r"Error response from daemon: (?P<msg>.*failed to resolve reference.*)"),
    re.compile(r"docker: Error response from daemon: (?P<msg>pull access denied.*)"),
    re.compile(r"(?P<msg>manifest for .*not found: manifest unknown)"),
]


class DockerPullParser:
    name = "docker_pull"

    def applies(self, check_name: str, repo: str) -> bool:
        return True

    def scan(self, lines: Iterable[str], ctx: StageContext) -> Iterable[Finding]:
        for i, line in enumerate(lines, start=1):
            for pat in _PATTERNS:
                m = pat.search(line)
                if not m:
                    continue
                yield Finding(
                    parser=self.name,
                    severity=Severity.infra,
                    kind="docker_pull",
                    headline=m["msg"].strip(),
                    detail=line.strip(),
                    stage=ctx.current,
                    log_line=i,
                    dedupe_key=f"docker_pull:{ctx.current or 'unknown'}",
                    human_headline="Docker image pull failed (likely transient)",
                    ai_directive=None,  # infra, not for the agent
                    verify_command=None,
                )
                break
