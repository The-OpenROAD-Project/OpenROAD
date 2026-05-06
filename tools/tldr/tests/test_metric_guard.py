# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

import unittest

from tldr.parsers.metric_guard import MetricGuardParser
from tldr.strip import strip_line

from _fixture_helpers import read_lines, StaticStageContext


class MetricGuardTest(unittest.TestCase):
    def test_parses_two_failures(self) -> None:
        raw = read_lines("jenkins_metric_guard.txt")
        stripped = [strip_line(l) for l in raw]
        out = list(MetricGuardParser().scan(stripped, StaticStageContext("Metrics")))
        # Two findings, but they share the same metric name so dedupe_key
        # collapses to one. The parser still emits both raw findings; collapse
        # is the renderer's job.
        self.assertEqual(len(out), 2)
        self.assertTrue(all("setup__tns" in f.headline for f in out))

    def test_first_finding_carries_human_headline(self) -> None:
        raw = read_lines("jenkins_metric_guard.txt")
        stripped = [strip_line(l) for l in raw]
        out = list(MetricGuardParser().scan(stripped, StaticStageContext("Metrics")))
        self.assertIn("Metric guard tripped", out[0].human_headline or "")


if __name__ == "__main__":
    unittest.main()
