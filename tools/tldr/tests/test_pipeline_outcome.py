# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

import unittest

from tldr.parsers.base import Severity
from tldr.parsers.pipeline_outcome import PipelineOutcomeParser

from _fixture_helpers import StaticStageContext


class PipelineOutcomeTest(unittest.TestCase):
    def test_unstable_emits_info(self) -> None:
        out = list(
            PipelineOutcomeParser().scan(
                ["[some other line]", "Finished: UNSTABLE"],
                StaticStageContext(),
            )
        )
        self.assertEqual(len(out), 1)
        self.assertEqual(out[0].severity, Severity.info)
        self.assertIn("UNSTABLE", out[0].headline)

    def test_aborted_emits_info(self) -> None:
        out = list(
            PipelineOutcomeParser().scan(["Finished: ABORTED"], StaticStageContext())
        )
        self.assertEqual(len(out), 1)
        self.assertIn("ABORTED", out[0].headline)

    def test_success_emits_stale_status_hint(self) -> None:
        out = list(
            PipelineOutcomeParser().scan(["Finished: SUCCESS"], StaticStageContext())
        )
        self.assertEqual(len(out), 1)
        self.assertEqual(out[0].severity, Severity.info)
        self.assertIn("stale", (out[0].human_headline or "").lower())

    def test_failure_emits_nothing(self) -> None:
        # FAILURE is already covered by other parsers; don't duplicate.
        out = list(
            PipelineOutcomeParser().scan(["Finished: FAILURE"], StaticStageContext())
        )
        self.assertEqual(out, [])

    def test_no_outcome_emits_nothing(self) -> None:
        out = list(
            PipelineOutcomeParser().scan(
                ["just a regular log line"], StaticStageContext()
            )
        )
        self.assertEqual(out, [])

    def test_only_applies_to_jenkins_checks(self) -> None:
        p = PipelineOutcomeParser()
        self.assertTrue(p.applies("continuous-integration/jenkins/pr-merge", "o/r"))
        self.assertFalse(p.applies("Black", "o/r"))


if __name__ == "__main__":
    unittest.main()
