# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

import unittest

from tldr.parsers.openroad_msg import OpenroadMsgParser
from tldr.strip import strip_line

from _fixture_helpers import read_lines, StaticStageContext


class OpenroadMsgTest(unittest.TestCase):
    def test_parses_error_with_code(self) -> None:
        raw = read_lines("jenkins_or_10326_bazel_fail.txt")
        stripped = [strip_line(l) for l in raw]
        out = list(OpenroadMsgParser().scan(stripped, StaticStageContext("Bazel test")))
        codes = [f.headline.split("]")[0][1:] for f in out]
        self.assertIn("RSZ-2007", codes)

    def test_skips_warnings(self) -> None:
        out = list(
            OpenroadMsgParser().scan(
                ["[WARNING ABC-0001] heads up"], StaticStageContext()
            )
        )
        self.assertEqual(out, [])


if __name__ == "__main__":
    unittest.main()
