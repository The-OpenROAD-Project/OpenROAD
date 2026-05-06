# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

import unittest

from tldr.parsers.lockfile import LockfileParser
from tldr.strip import strip_line

from _fixture_helpers import read_lines, StaticStageContext


class LockfileTest(unittest.TestCase):
    def test_parses_module_lock_drift(self) -> None:
        raw = read_lines("gha_check_bazel_lock.txt")
        stripped = [strip_line(l) for l in raw]
        out = list(LockfileParser().scan(stripped, StaticStageContext()))
        self.assertEqual(len(out), 1)
        self.assertEqual(out[0].location, "MODULE.bazel.lock")
        self.assertEqual(out[0].auto_fix_command[0], "bazelisk")


if __name__ == "__main__":
    unittest.main()
