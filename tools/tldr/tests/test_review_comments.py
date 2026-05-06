# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""Mocks `gh api` to verify Gemini-style and human review-comment parsing."""

from __future__ import annotations

import json
import unittest

from tldr import review_comments
from tldr.parsers.base import Severity

GEMINI_HIGH = (
    "![high](https://www.gstatic.com/codereviewagent/high-priority.svg)\n\n"
    "The current implementation calls `plugin.scan` line-by-line with a "
    "single-element list. This breaks any parser that maintains internal "
    "state across lines (such as `CtestParser`)."
)

GEMINI_MEDIUM = (
    "![medium](https://www.gstatic.com/codereviewagent/medium-priority.svg)\n\n"
    "Splitting paginated JSON output by `\\n\\n` is fragile."
)

HUMAN_PLAIN = "I think this could be simpler — consider extracting a helper."


def _inline(id_: int, body: str, path: str, line: int, position: int = 5) -> dict:
    return {
        "id": id_,
        "body": body,
        "path": path,
        "line": line,
        "position": position,
        "user": {"login": "gemini-code-assist[bot]"},
        "html_url": f"https://github.com/x/y/pull/1#discussion_r{id_}",
    }


def _outdated(id_: int, body: str, path: str, position: int | None = None) -> dict:
    # GitHub sets `line` to None when a comment's hunk has moved off the
    # head diff, regardless of whether `position` is still set.
    return {
        "id": id_,
        "body": body,
        "path": path,
        "line": None,
        "original_line": 42,
        "position": position,
        "user": {"login": "gemini-code-assist[bot]"},
    }


def _review(id_: int, body: str, state: str = "COMMENTED") -> dict:
    return {
        "id": id_,
        "body": body,
        "state": state,
        "user": {"login": "gemini-code-assist[bot]"},
        "html_url": f"https://github.com/x/y/pull/1#pullrequestreview-{id_}",
    }


class ReviewCommentsTest(unittest.TestCase):
    def _runner(self, comments: list[dict], reviews: list[dict]):
        def runner(cmd):
            url = cmd[2]
            if url.endswith("/comments"):
                return 0, json.dumps(comments)
            if url.endswith("/reviews"):
                return 0, json.dumps(reviews)
            return 1, ""

        return runner

    def test_high_badge_is_actionable(self) -> None:
        runner = self._runner(
            [_inline(1, GEMINI_HIGH, "tools/tldr/src/tldr/cli.py", 75)],
            [],
        )
        out = list(review_comments.discover("o/r", 1, runner=runner))
        self.assertEqual(len(out), 1)
        self.assertEqual(out[0].severity, Severity.actionable)
        self.assertEqual(out[0].location, "tools/tldr/src/tldr/cli.py:75")
        self.assertIn("CtestParser", out[0].detail)
        self.assertFalse(out[0].collapse_in_human_tldr)

    def test_medium_badge_is_actionable(self) -> None:
        runner = self._runner(
            [_inline(2, GEMINI_MEDIUM, "tools/tldr/src/tldr/github_api.py", 69)],
            [],
        )
        out = list(review_comments.discover("o/r", 1, runner=runner))
        self.assertEqual(out[0].severity, Severity.actionable)

    def test_human_comment_no_badge_defaults_actionable(self) -> None:
        runner = self._runner(
            [_inline(3, HUMAN_PLAIN, "src/foo.py", 1)],
            [],
        )
        out = list(review_comments.discover("o/r", 1, runner=runner))
        self.assertEqual(out[0].severity, Severity.actionable)

    def test_outdated_comment_dropped(self) -> None:
        runner = self._runner(
            [_outdated(4, GEMINI_HIGH, "deleted.py")],
            [],
        )
        out = list(review_comments.discover("o/r", 1, runner=runner))
        self.assertEqual(out, [])

    def test_outdated_with_nonnull_position_still_dropped(self) -> None:
        # Real-world shape observed on PR #10341: line=null but position=1.
        runner = self._runner(
            [_outdated(9, GEMINI_HIGH, "github_api.py", position=1)],
            [],
        )
        out = list(review_comments.discover("o/r", 1, runner=runner))
        self.assertEqual(out, [])

    def test_approved_review_dropped(self) -> None:
        runner = self._runner([], [_review(5, "lgtm", state="APPROVED")])
        out = list(review_comments.discover("o/r", 1, runner=runner))
        self.assertEqual(out, [])

    def test_top_level_review_is_info_by_default(self) -> None:
        runner = self._runner([], [_review(6, "Some general feedback.")])
        out = list(review_comments.discover("o/r", 1, runner=runner))
        self.assertEqual(len(out), 1)
        self.assertEqual(out[0].severity, Severity.info)
        self.assertEqual(out[0].kind, "review_summary")

    def test_dedupe_key_is_per_comment_id(self) -> None:
        runner = self._runner(
            [
                _inline(7, GEMINI_HIGH, "a.py", 1),
                _inline(8, GEMINI_HIGH, "b.py", 2),
            ],
            [],
        )
        out = list(review_comments.discover("o/r", 1, runner=runner))
        keys = {f.dedupe_key for f in out}
        self.assertEqual(len(keys), 2)


if __name__ == "__main__":
    unittest.main()
