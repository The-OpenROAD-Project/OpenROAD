# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

import unittest

from tldr.parsers.buildifier import BuildifierParser
from tldr.strip import strip_line

from _fixture_helpers import read_lines, StaticStageContext


class BuildifierTest(unittest.TestCase):
    def test_parses_two_files(self) -> None:
        raw = read_lines("gha_buildifier.txt")
        stripped = [strip_line(l) for l in raw]
        out = list(BuildifierParser().scan(stripped, StaticStageContext("Buildifier")))
        files = sorted(f.location for f in out)
        self.assertEqual(files, ["src/bar/BUILD", "src/foo/BUILD.bazel"])
        for f in out:
            self.assertEqual(f.auto_fix_command[0], "buildifier")


if __name__ == "__main__":
    unittest.main()
