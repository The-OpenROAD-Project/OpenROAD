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
        out = list(
            ClangFormatParser().scan(stripped, StaticStageContext("Clang-Format"))
        )
        files = sorted(f.location for f in out)
        self.assertEqual(
            files,
            ["src/mpl/src/hier_rtlmp.cpp", "src/rsz/src/Resizer.cpp"],
        )
        for f in out:
            self.assertEqual(f.auto_fix_command[0], "clang-format")

    def test_no_auto_fix_for_src_sta(self) -> None:
        # Per CLAUDE.md rule 2 we never run clang-format on src/sta files.
        # The finding still surfaces, but auto_fix_command is None and the
        # AI directive tells the agent NOT to run clang-format on it.
        line = (
            "src/sta/src/Sdc.cc:42:1: error: code should be clang-formatted"
            " [-Wclang-format-violations]"
        )
        out = list(ClangFormatParser().scan([line], StaticStageContext("Clang-Format")))
        self.assertEqual(len(out), 1)
        self.assertIsNone(out[0].auto_fix_command)
        self.assertIn("do NOT run clang-format", out[0].ai_directive)

    def test_no_auto_fix_for_dot_i(self) -> None:
        # SWIG `.i` interface files share the same exclusion.
        line = (
            "src/foo/foo.i:42:1: error: code should be clang-formatted"
            " [-Wclang-format-violations]"
        )
        out = list(ClangFormatParser().scan([line], StaticStageContext("Clang-Format")))
        # The pattern only matches .cpp/.cc/.h family by design; .i isn't
        # a supported clang-format extension, so the parser shouldn't even
        # emit a finding for it. Keep the test as a regression guard.
        self.assertEqual(out, [])


if __name__ == "__main__":
    unittest.main()
