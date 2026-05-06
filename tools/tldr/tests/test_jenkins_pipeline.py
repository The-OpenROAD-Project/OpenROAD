# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

import unittest

from tldr.parsers.jenkins_pipeline import JenkinsPipelineParser

from _fixture_helpers import StaticStageContext


class JenkinsPipelineTest(unittest.TestCase):
    def test_parses_fatal(self) -> None:
        out = list(
            JenkinsPipelineParser().scan(
                ["ERROR: A fatal error occurred in the pipeline: docker exec failed"],
                StaticStageContext("Stage X"),
            )
        )
        self.assertEqual(len(out), 1)
        self.assertIn("docker exec failed", out[0].headline)


if __name__ == "__main__":
    unittest.main()
