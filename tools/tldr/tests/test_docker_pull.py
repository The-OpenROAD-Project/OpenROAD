# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

import unittest

from tldr.parsers.docker_pull import DockerPullParser
from tldr.parsers.base import Severity
from tldr.strip import strip_line

from _fixture_helpers import read_lines, StaticStageContext


class DockerPullTest(unittest.TestCase):
    def test_parses_failed_resolve(self) -> None:
        raw = read_lines("jenkins_docker_pull.txt")
        stripped = [strip_line(l) for l in raw]
        out = list(DockerPullParser().scan(stripped, StaticStageContext("prelude")))
        self.assertEqual(len(out), 1)
        self.assertEqual(out[0].severity, Severity.infra)


if __name__ == "__main__":
    unittest.main()
