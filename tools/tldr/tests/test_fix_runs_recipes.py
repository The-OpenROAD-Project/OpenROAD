# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""Verify the //:fix-tldr binary runs the right local recipes."""

from __future__ import annotations

import io
import subprocess
import unittest
from unittest import mock

from tldr import fix_main
from tldr.parsers.base import Finding, Severity


def _f(kind: str, *, recipe=None, severity=Severity.actionable) -> Finding:
    return Finding(
        parser="x",
        severity=severity,
        kind=kind,
        headline=f"{kind} headline",
        dedupe_key=f"x:{kind}",
        auto_fix_command=tuple(recipe) if recipe else None,
    )


class _CompletedProcess:
    def __init__(self, returncode: int) -> None:
        self.returncode = returncode


class FixRunsRecipesTest(unittest.TestCase):
    def test_runs_recipe_for_each_finding_with_command(self) -> None:
        findings = [
            _f("clang_format", recipe=("clang-format", "-i", "src/foo.cpp")),
            _f("buildifier", recipe=("buildifier", "-mode=fix", "BUILD")),
            _f("flow_test_fail"),  # no recipe — leftover
        ]
        calls: list[list[str]] = []

        def fake_run(cmd, cwd=None, check=False):
            calls.append(cmd)
            return _CompletedProcess(returncode=0)

        results, leftover = fix_main._run_recipes(findings, runner=fake_run)
        self.assertEqual(len(results), 2)
        self.assertEqual(len(leftover), 1)
        self.assertEqual(leftover[0].kind, "flow_test_fail")
        self.assertEqual(
            sorted([c[0] for c in calls]),
            ["buildifier", "clang-format"],
        )

    def test_exit_code_nonzero_when_leftover_present(self) -> None:
        findings = [_f("flow_test_fail")]
        with mock.patch.object(fix_main, "discover_findings", return_value=findings):
            with mock.patch.object(fix_main.local, "resolve") as resolve:
                resolve.return_value = mock.Mock(repo="O/R", pr=1, sha="abc")
                with mock.patch("sys.stdout", io.StringIO()):
                    rc = fix_main.main([])
        self.assertEqual(rc, 1)

    def test_exit_code_zero_when_all_fixed(self) -> None:
        findings = [_f("clang_format", recipe=("clang-format", "-i", "x.cpp"))]
        with mock.patch.object(fix_main, "discover_findings", return_value=findings):
            with mock.patch.object(fix_main.local, "resolve") as resolve:
                resolve.return_value = mock.Mock(repo="O/R", pr=1, sha="abc")
                with mock.patch.object(
                    subprocess, "run", return_value=_CompletedProcess(returncode=0)
                ):
                    with mock.patch("sys.stdout", io.StringIO()):
                        rc = fix_main.main([])
        self.assertEqual(rc, 0)


if __name__ == "__main__":
    unittest.main()
