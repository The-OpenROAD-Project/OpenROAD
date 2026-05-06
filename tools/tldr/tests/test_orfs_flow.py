# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

import unittest

from tldr.parsers.orfs_flow import OrfsFlowParser
from tldr.strip import strip_line

from _fixture_helpers import read_lines, StaticStageContext


class OrfsFlowTest(unittest.TestCase):
    def test_finds_three_failures(self) -> None:
        raw = read_lines("jenkins_orfs_4204_flow_fail.txt")
        stripped = [strip_line(l) for l in raw]
        out = list(OrfsFlowParser().scan(stripped, StaticStageContext("Test")))
        designs = sorted(f.headline for f in out)
        self.assertEqual(
            designs,
            [
                "Test bp_fe_top nangate45 failed",
                "Test mempool_group nangate45 failed",
                "Test microwatt sky130hd failed",
            ],
        )

    def test_each_finding_has_repro_command(self) -> None:
        raw = read_lines("jenkins_orfs_4204_flow_fail.txt")
        stripped = [strip_line(l) for l in raw]
        out = list(OrfsFlowParser().scan(stripped, StaticStageContext("Test")))
        for f in out:
            self.assertIn("DESIGN_CONFIG", f.verify_command or "")

    def test_dedupe_keys_distinct(self) -> None:
        raw = read_lines("jenkins_orfs_4204_flow_fail.txt")
        stripped = [strip_line(l) for l in raw]
        out = list(OrfsFlowParser().scan(stripped, StaticStageContext("Test")))
        keys = {f.dedupe_key for f in out}
        self.assertEqual(len(keys), 3)


if __name__ == "__main__":
    unittest.main()
