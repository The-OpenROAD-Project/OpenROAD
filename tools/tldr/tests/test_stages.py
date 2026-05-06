# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

import unittest

from tldr.stages import StageTracker


class StagesTest(unittest.TestCase):
    def test_open_close(self) -> None:
        t = StageTracker()
        self.assertIsNone(t.current)
        t.feed("[Pipeline] { (Build on ubuntu:22.04)")
        self.assertEqual(t.current, "Build on ubuntu:22.04")
        t.feed("[Pipeline] }")
        self.assertIsNone(t.current)

    def test_nested(self) -> None:
        t = StageTracker()
        t.feed("[Pipeline] { (Outer)")
        t.feed("[Pipeline] { (Inner)")
        self.assertEqual(t.current, "Inner")
        t.feed("[Pipeline] }")
        self.assertEqual(t.current, "Outer")
        t.feed("[Pipeline] }")
        self.assertIsNone(t.current)

    def test_branch_prefix(self) -> None:
        t = StageTracker()
        t.feed("[bp_fe_top nangate45 on ubuntu:22.04] [Pipeline] { (Branch: bp_fe_top)")
        self.assertEqual(
            t.current, "Branch: bp_fe_top (bp_fe_top nangate45 on ubuntu:22.04)"
        )


if __name__ == "__main__":
    unittest.main()
