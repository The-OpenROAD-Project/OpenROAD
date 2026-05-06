# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""End-to-end smoke test for cli.main with the GitHub interactions stubbed."""

from __future__ import annotations

import io
import unittest
from unittest import mock

from tldr import cli, github_api
from tldr.parsers.base import Finding, Severity


class CliTest(unittest.TestCase):
    def _stub_findings(self) -> list[Finding]:
        return [
            Finding(
                parser="orfs_flow",
                severity=Severity.actionable,
                kind="flow_test_fail",
                headline="Test bp_fe_top nangate45 failed",
                stage="Test ubuntu:22.04",
                location="designs/nangate45/bp_fe_top/config.mk",
                dedupe_key="orfs_flow:bp_fe_top:nangate45",
                human_headline="Flow test bp_fe_top/nangate45 failed",
                ai_directive="Diff against master and fix root cause.",
                verify_command="make DESIGN_CONFIG=designs/nangate45/bp_fe_top/config.mk",
            ),
        ]

    def test_table_format_default(self) -> None:
        with mock.patch.object(cli, "discover_findings", return_value=self._stub_findings()):
            buf = io.StringIO()
            with mock.patch("sys.stdout", buf):
                rc = cli.main(["--pr", "42", "--sha", "abc1234567", "--repo", "O/R"])
            self.assertEqual(rc, 0)
            self.assertIn("PR #42", buf.getvalue())
            self.assertIn("flow_test_fail", buf.getvalue())

    def test_markdown_format(self) -> None:
        with mock.patch.object(cli, "discover_findings", return_value=self._stub_findings()):
            buf = io.StringIO()
            with mock.patch("sys.stdout", buf):
                rc = cli.main(
                    ["--pr", "42", "--sha", "abc1234567", "--repo", "O/R", "--format", "markdown"]
                )
            self.assertEqual(rc, 0)
            self.assertIn("<!-- tldr:begin -->", buf.getvalue())
            self.assertIn("### TL;DR (humans)", buf.getvalue())
            self.assertIn("### Instructions for an AI coding tool", buf.getvalue())

    def test_post_requires_pr(self) -> None:
        # When --post is set without --pr, the resolver may return pr=None and
        # the publish step must refuse.
        with mock.patch.object(cli, "discover_findings", return_value=[]):
            with mock.patch("sys.stderr", io.StringIO()) as err:
                with mock.patch("sys.stdout", io.StringIO()):
                    rc = cli.main(["--sha", "abc1234567", "--repo", "O/R", "--post"])
            self.assertNotEqual(rc, 0)
            self.assertIn("--post requires --pr", err.getvalue())


if __name__ == "__main__":
    unittest.main()
