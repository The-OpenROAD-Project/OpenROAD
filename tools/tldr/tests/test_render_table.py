# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

import unittest

from tldr.parsers.base import Finding, Severity
from tldr.render_table import render


class RenderTableTest(unittest.TestCase):
    def test_includes_header_counts(self) -> None:
        findings = [
            Finding(parser="p", severity=Severity.actionable, kind="k1", headline="h1"),
            Finding(parser="p", severity=Severity.infra, kind="k2", headline="h2"),
        ]
        out = render(findings, pr=42, sha="abcdef1234", repo="O/R")
        self.assertIn("PR #42", out)
        self.assertIn("abcdef1", out)
        self.assertIn("1 actionable, 1 infra", out)
        self.assertIn("k1", out)
        self.assertIn("k2", out)

    def test_empty_findings(self) -> None:
        out = render([], pr=1, sha="z" * 12)
        self.assertIn("No findings", out)

    def test_empty_with_failing_urls_says_so(self) -> None:
        # When discovery reports failing checks but no parser produced a
        # finding, surface the URLs rather than say "CI is clean".
        out = render(
            [],
            pr=1,
            sha="z" * 12,
            failing_check_urls=[
                "https://jenkins.openroad.tools/job/x/1/display/redirect",
                "https://jenkins.openroad.tools/job/x/2/display/redirect",
            ],
        )
        self.assertIn("couldn't extract", out)
        self.assertIn("Found 2 failing", out)
        self.assertIn("jenkins.openroad.tools/job/x/1", out)


if __name__ == "__main__":
    unittest.main()
