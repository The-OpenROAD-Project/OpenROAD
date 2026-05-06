# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

import unittest

from tldr.parsers.mac_build import MacBuildParser
from tldr.strip import strip_line

from _fixture_helpers import read_lines, StaticStageContext


class MacBuildTest(unittest.TestCase):
    def test_parses_compile_error(self) -> None:
        raw = read_lines("gha_mac_build.txt")
        stripped = [strip_line(l) for l in raw]
        out = list(MacBuildParser().scan(stripped, StaticStageContext("Mac-Build")))
        self.assertEqual(len(out), 1)
        self.assertIn("src/foo.cpp:42:10", out[0].location)


if __name__ == "__main__":
    unittest.main()
