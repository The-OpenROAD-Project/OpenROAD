# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

import unittest

from tldr.parsers.clang_tidy import ClangTidyParser
from tldr.strip import strip_line

from _fixture_helpers import read_lines, StaticStageContext


class ClangTidyTest(unittest.TestCase):
    def test_parses_two_warnings(self) -> None:
        raw = read_lines("gha_clang_tidy.txt")
        stripped = [strip_line(l) for l in raw]
        out = list(ClangTidyParser().scan(stripped, StaticStageContext("Clang-Tidy")))
        # Both checks are in the auto-fixable allow-list.
        self.assertEqual(len(out), 2)
        self.assertTrue(any(f.auto_fix_command for f in out))

    def test_extracts_location(self) -> None:
        raw = read_lines("gha_clang_tidy.txt")
        stripped = [strip_line(l) for l in raw]
        out = list(ClangTidyParser().scan(stripped, StaticStageContext("Clang-Tidy")))
        self.assertIn("hier_rtlmp.cpp:2451:23", out[0].location)


if __name__ == "__main__":
    unittest.main()
