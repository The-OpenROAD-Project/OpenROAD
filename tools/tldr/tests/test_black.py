# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

import unittest

from tldr.parsers.black import BlackParser
from tldr.strip import strip_line

from _fixture_helpers import read_lines, StaticStageContext


class BlackTest(unittest.TestCase):
    def test_parses_two_files(self) -> None:
        raw = read_lines("gha_black.txt")
        stripped = [strip_line(l) for l in raw]
        out = list(BlackParser().scan(stripped, StaticStageContext("Black")))
        files = sorted(f.location for f in out)
        # CI absolute paths must be normalised to repo-relative form so
        # the recipes can find the files locally.
        self.assertEqual(files, ["etc/whittle.py", "tools/tldr/src/tldr/cli.py"])
        for f in out:
            self.assertEqual(f.auto_fix_command[0], "black")
            self.assertFalse(f.auto_fix_command[1].startswith("/"))


if __name__ == "__main__":
    unittest.main()
