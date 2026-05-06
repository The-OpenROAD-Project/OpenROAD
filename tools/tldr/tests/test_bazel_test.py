# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

import unittest

from tldr.parsers.bazel_test import BazelTestParser
from tldr.strip import strip_line

from _fixture_helpers import read_lines, StaticStageContext


class BazelTestTest(unittest.TestCase):
    def test_parses_two_failed_targets(self) -> None:
        raw = read_lines("jenkins_or_10326_bazel_fail.txt")
        stripped = [strip_line(l) for l in raw]
        out = list(BazelTestParser().scan(stripped, StaticStageContext("Bazel test")))
        targets = sorted(f.location for f in out)
        self.assertEqual(
            targets,
            ["//src/dbSta:test_dbsta", "//src/rsz:test_buffer_ports"],
        )

    def test_verify_command_is_bazelisk(self) -> None:
        raw = read_lines("jenkins_or_10326_bazel_fail.txt")
        stripped = [strip_line(l) for l in raw]
        out = list(BazelTestParser().scan(stripped, StaticStageContext("Bazel test")))
        for f in out:
            self.assertTrue(f.verify_command.startswith("bazelisk test //"))

    def test_external_and_bzlmod_targets_are_recognised(self) -> None:
        out = list(
            BazelTestParser().scan(
                [
                    "@upstream//src/foo:test_bar                                FAILED in 0.5s",
                    "@@protobuf+30.2//python:test                              FAILED in 1.2s",
                    "@my~module//path:t                                        FAILED in 0.1s",
                ],
                StaticStageContext("Test"),
            )
        )
        targets = sorted(f.location for f in out)
        self.assertEqual(
            targets,
            [
                "@@protobuf+30.2//python:test",
                "@my~module//path:t",
                "@upstream//src/foo:test_bar",
            ],
        )

    def test_detail_captures_test_output_block(self) -> None:
        # PR #10287 shape: the FAILED summary is far above (in line order)
        # the actual stderr, which lives in an explicit Bazel block. The
        # parser must correlate them by target.
        log = [
            "//src/odb/test:odb_man_tcl_check       FAILED in 2.5s",
            "Some other unrelated noise.",
            "(50 more lines of build output)",
            "==================== Test output for //src/odb/test:odb_man_tcl_check:",
            "AssertionError: 20 != 19 : ./src/odb: help count (20) != readme count (19)",
            "FAILED (failures=1)",
            "================================================================================",
            "Some trailing summary.",
        ]
        out = list(BazelTestParser().scan(log, StaticStageContext("Bazel test")))
        self.assertEqual(len(out), 1)
        self.assertIn("AssertionError: 20 != 19", out[0].detail)
        self.assertIn("help count", out[0].detail)
        # Trailing summary outside the block is NOT in the detail.
        self.assertNotIn("trailing summary", out[0].detail)

    def test_detail_block_truncated_at_cap(self) -> None:
        log = [
            "//src/x:y FAILED in 1s",
            "==================== Test output for //src/x:y:",
        ]
        log += [f"detail line {i}" for i in range(50)]
        log += [
            "================================================================================"
        ]
        out = list(BazelTestParser().scan(log, StaticStageContext("Test")))
        self.assertEqual(len(out), 1)
        # First line plus up to _DETAIL_LINES from the block.
        self.assertLessEqual(len(out[0].detail.splitlines()), 26)

    def test_block_before_failed_summary(self) -> None:
        # PR #10287 real-log shape: Bazel emits the Test-output block during
        # the failing run (line ~85k) and the FAILED summary line tens of
        # thousands of lines later (line ~180k). The parser must associate
        # them by target name regardless of which appears first.
        log = [
            "(40 lines of build noise)",
            "==================== Test output for //src/odb/test:odb_man_tcl_check:",
            "Traceback (most recent call last):",
            "AssertionError: 20 != 19 : ./src/odb: help count (20) != readme count (19)",
            "================================================================================",
            "(more lines of unrelated build output much later)",
            "//src/odb/test:odb_man_tcl_check       FAILED in 5.1s",
        ]
        out = list(BazelTestParser().scan(log, StaticStageContext("Test")))
        self.assertEqual(len(out), 1)
        self.assertIn("AssertionError: 20 != 19", out[0].detail)

    def test_no_block_means_minimal_detail(self) -> None:
        # If there's no Test-output block (Bazel didn't print one because
        # the test failed for an infrastructure reason), the finding still
        # exists, just with detail=first line only.
        log = ["//src/x:y FAILED in 1s", "Build completed, 1 test FAILED"]
        out = list(BazelTestParser().scan(log, StaticStageContext("Test")))
        self.assertEqual(len(out), 1)
        self.assertEqual(out[0].detail.strip().splitlines(), ["//src/x:y FAILED in 1s"])


if __name__ == "__main__":
    unittest.main()
