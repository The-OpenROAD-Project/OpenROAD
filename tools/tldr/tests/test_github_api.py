# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""Verify update_pr_body sends the body via stdin (covers Gemini's medium
finding that argv-passing risks ARG_MAX for ~64KB PR bodies)."""

from __future__ import annotations

import json
import unittest

from tldr import github_api


class UpdatePrBodyTest(unittest.TestCase):
    def test_body_is_passed_via_stdin_not_argv(self) -> None:
        large_body = "X" * 200_000  # well past common ARG_MAX
        captured: dict = {}

        def fake_runner(cmd: list[str], stdin: str | None) -> tuple[int, str]:
            captured["cmd"] = cmd
            captured["stdin"] = stdin
            return 0, ""

        github_api.update_pr_body("o/r", 42, large_body, runner=fake_runner)

        # The body must NOT appear anywhere in argv.
        self.assertNotIn(large_body, " ".join(captured["cmd"]))
        # The body must be the JSON payload sent to stdin.
        payload = json.loads(captured["stdin"])
        self.assertEqual(payload["body"], large_body)
        # gh api invocation uses --input -.
        self.assertIn("--input", captured["cmd"])
        self.assertIn("-", captured["cmd"])
        self.assertEqual(captured["cmd"][:2], ["gh", "api"])
        self.assertIn("PATCH", captured["cmd"])

    def test_nonzero_runner_raises(self) -> None:
        def runner(cmd, stdin):
            return 1, "boom"

        with self.assertRaisesRegex(RuntimeError, "boom"):
            github_api.update_pr_body("o/r", 1, "body", runner=runner)


if __name__ == "__main__":
    unittest.main()
