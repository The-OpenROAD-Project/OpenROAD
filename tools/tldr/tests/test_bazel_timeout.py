# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

import unittest

from tldr.parsers.bazel_timeout import BazelTimeoutParser

from _fixture_helpers import StaticStageContext


class BazelTimeoutTest(unittest.TestCase):
    def test_parses_timeout(self) -> None:
        out = list(
            BazelTimeoutParser().scan(
                ["ERROR: Timeout after 600 seconds"],
                StaticStageContext("Test"),
            )
        )
        self.assertEqual(len(out), 1)
        self.assertIn("600s", out[0].headline)

    def test_no_match_means_no_finding(self) -> None:
        out = list(BazelTimeoutParser().scan(["all good here"], StaticStageContext()))
        self.assertEqual(out, [])


if __name__ == "__main__":
    unittest.main()
