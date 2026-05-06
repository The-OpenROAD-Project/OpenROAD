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


if __name__ == "__main__":
    unittest.main()
