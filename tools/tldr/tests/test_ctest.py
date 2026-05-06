# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

import unittest

from tldr.parsers.ctest import CtestParser
from tldr.strip import strip_line

from _fixture_helpers import read_lines, StaticStageContext


class CtestTest(unittest.TestCase):
    def test_parses_two_rows(self) -> None:
        raw = read_lines("jenkins_or_10326_bazel_fail.txt")
        stripped = [strip_line(l) for l in raw]
        out = list(CtestParser().scan(stripped, StaticStageContext("Test")))
        names = sorted(f.location for f in out)
        self.assertEqual(names, ["rsz_buffer_ports", "rsz_estimate_parasitics"])


if __name__ == "__main__":
    unittest.main()
