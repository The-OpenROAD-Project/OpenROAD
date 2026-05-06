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
        self.assertEqual(files, ["etc/whittle.py", "tools/tldr/src/tldr/cli.py"])


if __name__ == "__main__":
    unittest.main()
