# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

import unittest

from tldr.paths import to_repo_relative


class PathsTest(unittest.TestCase):
    def test_strips_gha_workspace(self) -> None:
        self.assertEqual(
            to_repo_relative(
                "/home/runner/work/OpenROAD/OpenROAD/tools/tldr/src/tldr/cli.py"
            ),
            "tools/tldr/src/tldr/cli.py",
        )

    def test_strips_jenkins_workspace(self) -> None:
        self.assertEqual(
            to_repo_relative("/var/jenkins_home/workspace/OpenROAD-PR/src/foo.cpp"),
            "src/foo.cpp",
        )

    def test_strips_jenkins_parallel_suffix(self) -> None:
        self.assertEqual(
            to_repo_relative("/var/jenkins_home/workspace/OpenROAD-PR@2/src/foo.cpp"),
            "src/foo.cpp",
        )

    def test_strips_github_workspace(self) -> None:
        self.assertEqual(
            to_repo_relative("/github/workspace/src/bar.cpp"),
            "src/bar.cpp",
        )

    def test_already_relative_unchanged(self) -> None:
        self.assertEqual(
            to_repo_relative("tools/tldr/src/tldr/cli.py"),
            "tools/tldr/src/tldr/cli.py",
        )

    def test_unknown_absolute_unchanged(self) -> None:
        self.assertEqual(to_repo_relative("/opt/x/y.cpp"), "/opt/x/y.cpp")


if __name__ == "__main__":
    unittest.main()
