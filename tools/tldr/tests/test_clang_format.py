# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

import unittest

from tldr.parsers.clang_format import ClangFormatParser
from tldr.strip import strip_line

from _fixture_helpers import read_lines, StaticStageContext


class ClangFormatTest(unittest.TestCase):
    def test_parses_two_files(self) -> None:
        raw = read_lines("gha_clang_format.txt")
        stripped = [strip_line(l) for l in raw]
        out = list(ClangFormatParser().scan(stripped, StaticStageContext("Clang-Format")))
        files = sorted(f.location for f in out)
        self.assertEqual(
            files,
            ["src/mpl/src/hier_rtlmp.cpp", "src/rsz/src/Resizer.cpp"],
        )
        for f in out:
            self.assertEqual(f.auto_fix_command[0], "clang-format")


if __name__ == "__main__":
    unittest.main()
