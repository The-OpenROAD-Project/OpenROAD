# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""Mock subprocess calls and assert local.resolve() returns sane LocalContext."""

from __future__ import annotations

import json
import unittest
from unittest import mock

from tldr import local


class LocalResolveTest(unittest.TestCase):
    def test_parse_repo_from_ssh_url(self) -> None:
        self.assertEqual(
            local.parse_repo_from_url(
                "git@github.com:The-OpenROAD-Project/OpenROAD.git"
            ),
            "The-OpenROAD-Project/OpenROAD",
        )

    def test_parse_repo_from_https_url(self) -> None:
        self.assertEqual(
            local.parse_repo_from_url(
                "https://github.com/The-OpenROAD-Project/OpenROAD"
            ),
            "The-OpenROAD-Project/OpenROAD",
        )

    def test_resolve_uses_gh_pr_view(self) -> None:
        gh_payload = json.dumps(
            {
                "number": 4204,
                "headRefOid": "abcdef1234567890",
                "headRepository": {"name": "OpenROAD", "owner": {"login": "alice"}},
                "baseRepository": {
                    "name": "OpenROAD",
                    "owner": {"login": "The-OpenROAD-Project"},
                },
            }
        )

        def fake_run(cmd):
            if cmd[:3] == ["gh", "pr", "view"]:
                return 0, gh_payload
            if cmd[:2] == ["git", "rev-parse"]:
                return 0, "abcdef1234567890"
            return 1, ""

        with mock.patch.object(local, "_run", fake_run):
            ctx = local.resolve()
        self.assertEqual(ctx.repo, "The-OpenROAD-Project/OpenROAD")
        self.assertEqual(ctx.pr, 4204)
        self.assertEqual(ctx.sha, "abcdef1234567890")
        self.assertEqual(ctx.source, "gh-pr-view")

    def test_resolve_falls_back_to_git_head(self) -> None:
        def fake_run(cmd):
            if cmd[:3] == ["gh", "pr", "view"]:
                return 1, ""
            if cmd[:2] == ["git", "rev-parse"]:
                return 0, "deadbeef00000000"
            if cmd[:3] == ["git", "remote", "get-url"]:
                return 0, "git@github.com:The-OpenROAD-Project/OpenROAD.git"
            return 1, ""

        with mock.patch.object(local, "_run", fake_run):
            ctx = local.resolve()
        self.assertIsNone(ctx.pr)
        self.assertEqual(ctx.sha, "deadbeef00000000")
        self.assertEqual(ctx.source, "git-head")
        self.assertEqual(ctx.repo, "The-OpenROAD-Project/OpenROAD")


if __name__ == "__main__":
    unittest.main()
