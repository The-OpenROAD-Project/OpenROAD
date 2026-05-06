# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""Stream a check's log lines.

Jenkins consoleText is anonymous over plain HTTP; GHA logs need an
authenticated ``gh api`` call. Both are wrapped behind the ``log_lines``
function so callers don't care.
"""

from __future__ import annotations

import subprocess
import urllib.error
import urllib.request
from typing import Callable, Iterator

from .github_api import CheckLike


class LogUnavailable(Exception):
    """Raised when a check's log can't be fetched (purged build, 404, …).

    Carries the check name and a hint URL so the caller can emit an
    info-level finding pointing the user at where to look manually.
    """

    def __init__(self, check_name: str, url: str | None, status: int | None) -> None:
        super().__init__(
            f"log for {check_name} unavailable"
            + (f" (HTTP {status})" if status is not None else "")
        )
        self.check_name = check_name
        self.url = url
        self.status = status


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


def _fetch_url(
    url: str, *, opener: Callable[[str], Iterator[str]] | None = None
) -> Iterator[str]:
    if opener is not None:
        yield from opener(url)
        return
    with urllib.request.urlopen(
        url, timeout=300
    ) as resp:  # noqa: S310 (caller-provided URL)
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

    Raises ``LogUnavailable`` when the log can't be fetched (HTTP 404 or
    similar) so the caller can surface an info-level finding rather than
    silently dropping the check from the table.
    """
    runner = runner or _default_runner
    if check.log_hint and check.log_hint.startswith("http"):
        try:
            yield from _fetch_url(check.log_hint, opener=url_opener)
        except urllib.error.HTTPError as e:
            raise LogUnavailable(check.name, check.log_hint, e.code) from e
        except urllib.error.URLError as e:
            raise LogUnavailable(check.name, check.log_hint, None) from e
        return
    if check.log_hint:
        rc, out = runner(["gh", "api", check.log_hint])
        if rc != 0:
            raise LogUnavailable(check.name, check.log_hint, None)
        for line in out.splitlines():
            yield line + "\n"
