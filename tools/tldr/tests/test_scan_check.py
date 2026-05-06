# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""Verify cli._scan_check feeds the whole log to each parser, so stateful
parsers (e.g. CtestParser) actually fire and finding `log_line` numbers
are correct.

This test covers Gemini's [high] finding on `cli.py:75` that the previous
per-line `plugin.scan([line], …)` call broke stateful parsers and reset
the line counter.
"""

from __future__ import annotations

import unittest
from unittest import mock

from tldr import cli, github_api


class ScanCheckTest(unittest.TestCase):
    def _check(self, name: str = "Bazel test") -> github_api.CheckLike:
        return github_api.CheckLike(
            name=name,
            conclusion="failure",
            details_url=None,
            log_hint=None,
        )

    def test_ctest_block_is_picked_up(self) -> None:
        # The CtestParser only fires *after* it has seen the
        # "The following tests FAILED:" header. The previous per-line
        # invocation reset its `in_block` flag on every call and never
        # matched. With the fix, the whole log is fed once.
        log = [
            "Some unrelated build output.",
            "The following tests FAILED:",
            "    42 - rsz_buffer_ports (Failed)",
            "    43 - rsz_estimate_parasitics (Timeout)",
            "Errors while running CTest",
        ]
        with mock.patch.object(cli, "fetch") as fake_fetch:
            fake_fetch.log_lines = lambda *a, **k: iter(log)
            out = list(cli._scan_check("o/r", self._check()))
        ctest_findings = [f for f in out if f.parser == "ctest"]
        self.assertGreaterEqual(len(ctest_findings), 2)
        names = sorted(f.location for f in ctest_findings)
        self.assertEqual(names, ["rsz_buffer_ports", "rsz_estimate_parasitics"])

    def test_log_line_counter_advances(self) -> None:
        # Bazel-test failures on different log lines should report different
        # log_line numbers. The previous per-line reset made every finding
        # report log_line=1.
        log = [
            "noise",
            "noise",
            "//src/foo:a                                FAILED in 0.1s",
            "noise",
            "//src/bar:b                                FAILED in 0.2s",
        ]
        with mock.patch.object(cli, "fetch") as fake_fetch:
            fake_fetch.log_lines = lambda *a, **k: iter(log)
            out = list(cli._scan_check("o/r", self._check()))
        bazel = sorted(
            (f for f in out if f.parser == "bazel_test"),
            key=lambda f: f.log_line or 0,
        )
        self.assertEqual(len(bazel), 2)
        self.assertEqual([f.log_line for f in bazel], [3, 5])

    def test_log_unavailable_emits_info_finding_not_exception(self) -> None:
        # Purged Jenkins build → LogUnavailable. The cli must surface it as
        # a single info-level finding so the user knows the build went away.
        from tldr import fetch as fetch_mod

        def raising_log_lines(*a, **k):
            raise fetch_mod.LogUnavailable(
                check_name="continuous-integration/jenkins/pr-merge",
                url="https://jenkins.openroad.tools/x/consoleText",
                status=404,
            )

        check = github_api.CheckLike(
            name="continuous-integration/jenkins/pr-merge",
            conclusion="error",
            details_url="https://jenkins.openroad.tools/x/consoleText",
            log_hint="https://jenkins.openroad.tools/x/consoleText",
        )
        with mock.patch.object(cli, "fetch") as fake_fetch:
            fake_fetch.log_lines = raising_log_lines
            fake_fetch.LogUnavailable = fetch_mod.LogUnavailable
            out = list(cli._scan_check("o/r", check))
        self.assertEqual(len(out), 1)
        self.assertEqual(out[0].kind, "log_unavailable")
        self.assertEqual(out[0].severity.value, "info")
        self.assertIn("404", out[0].headline)


if __name__ == "__main__":
    unittest.main()
