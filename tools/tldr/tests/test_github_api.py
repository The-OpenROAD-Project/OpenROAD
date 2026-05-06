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


class JenkinsConsoleTextUrlTest(unittest.TestCase):
    def test_strips_display_redirect(self) -> None:
        self.assertEqual(
            github_api._jenkins_console_text_url(
                "https://jenkins.openroad.tools/job/X/job/PR-1-merge/2/display/redirect"
            ),
            "https://jenkins.openroad.tools/job/X/job/PR-1-merge/2/consoleText",
        )

    def test_handles_trailing_slash(self) -> None:
        self.assertEqual(
            github_api._jenkins_console_text_url(
                "https://jenkins.openroad.tools/job/X/job/PR-1-merge/2/display/redirect/"
            ),
            "https://jenkins.openroad.tools/job/X/job/PR-1-merge/2/consoleText",
        )

    def test_returns_none_for_non_jenkins(self) -> None:
        self.assertIsNone(
            github_api._jenkins_console_text_url(
                "https://openroad--10341.org.readthedocs.build/en/10341/"
            )
        )

    def test_returns_none_for_empty(self) -> None:
        self.assertIsNone(github_api._jenkins_console_text_url(None))
        self.assertIsNone(github_api._jenkins_console_text_url(""))


class DiscoverFailingChecksTest(unittest.TestCase):
    """Verify the state/conclusion filters catch real-world failure shapes
    seen across the dogfooded sample of 10 PRs."""

    def _stub(self, checks_payload: list, statuses_payload: list):
        def runner(cmd):
            url = cmd[-2]  # `... --paginate`, so url is second-to-last
            if "/check-runs" in url:
                return 0, json.dumps({"check_runs": checks_payload})
            if "/statuses" in url:
                return 0, json.dumps(statuses_payload)
            return 1, ""

        return runner

    def test_status_state_error_is_treated_as_failure(self) -> None:
        # 9 of 10 dogfooded PRs reported `state="error"` for Jenkins; before
        # the fix the tool ignored every one.
        runner = self._stub(
            [],
            [
                {
                    "context": "continuous-integration/jenkins/pr-merge",
                    "state": "error",
                    "target_url": (
                        "https://jenkins.openroad.tools/job/x/2/display/redirect"
                    ),
                }
            ],
        )
        out = github_api.discover_failing_checks("o/r", "abc123", runner=runner)
        self.assertEqual(len(out), 1)
        self.assertEqual(out[0].conclusion, "error")
        self.assertEqual(
            out[0].log_hint,
            "https://jenkins.openroad.tools/job/x/2/consoleText",
        )

    def test_status_state_failure_still_caught(self) -> None:
        runner = self._stub(
            [],
            [
                {
                    "context": "continuous-integration/jenkins/pr-head",
                    "state": "failure",
                    "target_url": (
                        "https://jenkins.openroad.tools/job/y/3/display/redirect"
                    ),
                }
            ],
        )
        out = github_api.discover_failing_checks("o/r", "abc123", runner=runner)
        self.assertEqual(len(out), 1)
        self.assertEqual(out[0].conclusion, "failure")

    def test_check_run_cancelled_is_caught(self) -> None:
        # Jenkins-agent disconnect mid-run shows up as `cancelled`.
        runner = self._stub(
            [{"id": 999, "name": "Mac-Build", "conclusion": "cancelled"}],
            [],
        )
        out = github_api.discover_failing_checks("o/r", "abc", runner=runner)
        self.assertEqual(len(out), 1)
        self.assertEqual(out[0].conclusion, "cancelled")

    def test_pending_status_is_not_a_failure(self) -> None:
        runner = self._stub(
            [],
            [
                {
                    "context": "ci/jenkins",
                    "state": "pending",
                    "target_url": "https://jenkins.openroad.tools/x",
                }
            ],
        )
        out = github_api.discover_failing_checks("o/r", "abc", runner=runner)
        self.assertEqual(out, [])

    def test_dedupes_by_context_first_wins(self) -> None:
        # Latest status per context comes first; older entries are ignored.
        runner = self._stub(
            [],
            [
                {
                    "context": "ci/jenkins",
                    "state": "error",
                    "target_url": "https://jenkins.openroad.tools/2/display/redirect",
                },
                {
                    "context": "ci/jenkins",
                    "state": "success",
                    "target_url": "https://jenkins.openroad.tools/1/display/redirect",
                },
            ],
        )
        out = github_api.discover_failing_checks("o/r", "abc", runner=runner)
        self.assertEqual(len(out), 1)
        self.assertEqual(out[0].conclusion, "error")


if __name__ == "__main__":
    unittest.main()
