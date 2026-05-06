# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

import unittest

from tldr.parsers.base import Finding, Severity
from tldr.render_markdown import BEGIN, END, PROJECT_NORMS, render


class RenderMarkdownTest(unittest.TestCase):
    def test_renders_two_section_block(self) -> None:
        findings = [
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
            Finding(
                parser="docker_pull",
                severity=Severity.infra,
                kind="docker_pull",
                headline="image not found",
                stage="prelude",
                dedupe_key="docker_pull:prelude",
                human_headline="Docker image pull failed (likely transient)",
            ),
        ]

        block = render(findings, sha="abc1234567", pr_url="https://example.test/pull/1")

        self.assertTrue(block.startswith(BEGIN))
        self.assertTrue(block.rstrip().endswith(END))
        # Order matters: human TL;DR before AI section.
        h_idx = block.index("### TL;DR (humans)")
        a_idx = block.index("### Instructions for an AI coding tool")
        self.assertLess(h_idx, a_idx)
        # Human bullets present.
        self.assertIn("Flow test bp_fe_top/nangate45 failed", block)
        # AI directive present.
        self.assertIn("Diff against master", block)
        # Verify command surfaced as a markdown inline code span.
        self.assertIn(
            "`make DESIGN_CONFIG=designs/nangate45/bp_fe_top/config.mk`",
            block,
        )
        # Infra finding goes under "Ignore".
        self.assertIn("**Ignore (transient/infra):**", block)
        # Done-when line.
        self.assertIn("**Done when:**", block)
        # Header counts.
        self.assertIn("1 actionable, 1 infra", block)

    def test_norms_listed(self) -> None:
        block = render([], sha="zzzzzzz")
        for norm in PROJECT_NORMS:
            # Match a salient prefix of each norm rather than the whole bullet
            # (Markdown wrapping etc. shouldn't matter for the assertion).
            self.assertIn(norm.split(".")[0].split("(")[0].strip(), block)


if __name__ == "__main__":
    unittest.main()
