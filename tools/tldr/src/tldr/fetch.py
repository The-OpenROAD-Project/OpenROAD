# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""Stream a check's log lines.

Jenkins consoleText is anonymous over plain HTTP; GHA logs need an
authenticated ``gh api`` call. Both are wrapped behind the ``log_lines``
function so callers don't care.
"""

from __future__ import annotations

import subprocess
import urllib.request
from typing import Callable, Iterator

from .github_api import CheckLike

# A runner returns (rc, stdout_text) so tests can swap it out.
Runner = Callable[[list[str]], tuple[int, str]]


def _default_runner(cmd: list[str]) -> tuple[int, str]:
    result = subprocess.run(
        cmd, capture_output=True, text=True, check=False, timeout=300
    )
    return (
        result.returncode,
        (result.stdout or "") + ("" if result.returncode == 0 else result.stderr or ""),
    )


def _fetch_url(url: str, *, opener: Callable[[str], Iterator[str]] | None = None) -> Iterator[str]:
    if opener is not None:
        yield from opener(url)
        return
    with urllib.request.urlopen(url, timeout=300) as resp:  # noqa: S310 (caller-provided URL)
        for raw in resp:
            yield raw.decode("utf-8", errors="replace")


def log_lines(
    check: CheckLike,
    *,
    runner: Runner | None = None,
    url_opener: Callable[[str], Iterator[str]] | None = None,
) -> Iterator[str]:
    """Yield lines for a single failing check.

    For Jenkins, hits the ``consoleText`` URL anonymously. For GHA, calls
    ``gh api /repos/.../jobs/<id>/logs`` (which returns the plain log).
    """
    runner = runner or _default_runner
    if check.log_hint and check.log_hint.startswith("http"):
        yield from _fetch_url(check.log_hint, opener=url_opener)
        return
    if check.log_hint:
        rc, out = runner(["gh", "api", check.log_hint])
        if rc != 0:
            return
        for line in out.splitlines():
            yield line + "\n"
