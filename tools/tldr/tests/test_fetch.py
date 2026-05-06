# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""Verify fetch.log_lines surfaces purged-build 404s as LogUnavailable
rather than silently aborting (caught by dogfooding against PRs whose
Jenkins builds had been retention-deleted)."""

from __future__ import annotations

import unittest
import urllib.error

from tldr import fetch
from tldr.github_api import CheckLike


def _check(url: str = "https://jenkins.openroad.tools/x/consoleText") -> CheckLike:
    return CheckLike(
        name="continuous-integration/jenkins/pr-merge",
        conclusion="error",
        details_url=url,
        log_hint=url,
    )


class FetchLogUnavailableTest(unittest.TestCase):
    def test_404_raises_log_unavailable(self) -> None:
        def opener(url: str):
            raise urllib.error.HTTPError(url, 404, "Not Found", hdrs=None, fp=None)
            yield  # pragma: no cover (generator function)

        with self.assertRaises(fetch.LogUnavailable) as cm:
            list(fetch.log_lines(_check(), url_opener=opener))
        self.assertEqual(cm.exception.status, 404)
        self.assertEqual(cm.exception.check_name, _check().name)

    def test_url_error_raises_log_unavailable(self) -> None:
        def opener(url: str):
            raise urllib.error.URLError("connection reset")
            yield  # pragma: no cover

        with self.assertRaises(fetch.LogUnavailable) as cm:
            list(fetch.log_lines(_check(), url_opener=opener))
        self.assertIsNone(cm.exception.status)

    def test_successful_fetch_yields_lines(self) -> None:
        def opener(url: str):
            yield "first line\n"
            yield "second line\n"

        out = list(fetch.log_lines(_check(), url_opener=opener))
        self.assertEqual(out, ["first line\n", "second line\n"])

    def test_gha_log_via_runner_404_raises(self) -> None:
        # Non-http log_hint goes through the gh-api runner path.
        check = CheckLike(
            name="Mac-Build",
            conclusion="failure",
            details_url=None,
            log_hint="/repos/o/r/actions/jobs/123/logs",
        )

        def runner(cmd):
            return 1, "404 Not Found"

        with self.assertRaises(fetch.LogUnavailable):
            list(fetch.log_lines(check, runner=runner))


if __name__ == "__main__":
    unittest.main()
