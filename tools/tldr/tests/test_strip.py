# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

import unittest

from tldr.strip import strip_line


class StripTest(unittest.TestCase):
    def test_removes_jenkins_timestamp(self) -> None:
        self.assertEqual(
            strip_line("[2026-05-04T11:03:22.234Z] hello"),
            "hello",
        )

    def test_removes_ansi(self) -> None:
        self.assertEqual(strip_line("\x1b[31mERROR\x1b[0m: x"), "ERROR: x")

    def test_strips_combined(self) -> None:
        self.assertEqual(
            strip_line("[2026-05-04T11:03:22.234Z] \x1b[1mERROR\x1b[0m: x"),
            "ERROR: x",
        )

    def test_keeps_non_matching_lines_intact(self) -> None:
        self.assertEqual(strip_line("plain"), "plain")

    def test_removes_gha_bare_timestamp(self) -> None:
        self.assertEqual(
            strip_line("2026-05-04T11:00:00.000Z hello"),
            "hello",
        )

    def test_removes_jenkins_pipeline_echo(self) -> None:
        # `set -x`-style shell echo prefix that Jenkins pipeline `sh` steps
        # add to every command line.
        self.assertEqual(strip_line("+ docker pull foo"), "docker pull foo")
        # Combined with a Jenkins timestamp prefix.
        self.assertEqual(
            strip_line("[2026-05-04T10:00:00.123Z] + docker pull foo"),
            "docker pull foo",
        )


if __name__ == "__main__":
    unittest.main()
