# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""Verify the outage parser catches the common transient failures CI sees."""

from __future__ import annotations

import unittest

from tldr.parsers.base import Severity
from tldr.parsers.outage import OutageParser
from tldr.stages import StageTracker
from tldr.strip import strip_line

from _fixture_helpers import StaticStageContext, read_lines


class _LiveCtx:
    def __init__(self, tracker: StageTracker) -> None:
        self._t = tracker

    @property
    def current(self) -> str | None:
        return self._t.current


class OutageTest(unittest.TestCase):
    def _scan_with_stages(self, lines: list[str]):
        tracker = StageTracker()
        ctx = _LiveCtx(tracker)
        out = []
        for line in lines:
            tracker.feed(line)
            for f in OutageParser().scan([line], ctx):
                out.append(f)
        return out

    def test_assortment_finds_each_source_once(self) -> None:
        raw = read_lines("jenkins_outages_assortment.txt")
        stripped = [strip_line(l) for l in raw]
        out = self._scan_with_stages(stripped)
        sources = sorted(f.headline.split(":")[0] for f in out)
        # Each outage source appears at least once. (Each appears exactly
        # once per stage; the fixture puts each in its own stage, so each
        # source-finding is unique.)
        for src in [
            "container-registry",
            "apt-mirror",
            "pypi",
            "bazel-cache",
            "github-api",
            "jenkins-agent",
        ]:
            self.assertIn(src, sources, f"{src} not found")

    def test_all_findings_are_infra_severity(self) -> None:
        raw = read_lines("jenkins_outages_assortment.txt")
        stripped = [strip_line(l) for l in raw]
        out = self._scan_with_stages(stripped)
        for f in out:
            self.assertEqual(f.severity, Severity.infra)
            self.assertIsNone(f.ai_directive)
            self.assertIsNone(f.auto_fix_command)

    def test_dedupes_within_a_stage(self) -> None:
        # Repeated "Connection refused" within one stage should collapse.
        out = list(
            OutageParser().scan(
                [
                    "Connection refused",
                    "Connection refused again",
                    "Connection refused yet again",
                ],
                StaticStageContext("stage-A"),
            )
        )
        self.assertEqual(len(out), 1)

    def test_emits_one_per_distinct_source(self) -> None:
        out = list(
            OutageParser().scan(
                [
                    "Connection refused",
                    "Temporary failure in name resolution",
                    "No space left on device",
                ],
                StaticStageContext("setup"),
            )
        )
        sources = sorted(f.headline.split(":")[0] for f in out)
        self.assertEqual(sources, ["ci-runner-disk", "dns", "network"])

    def test_jenkins_failed_to_run_image(self) -> None:
        # PR #10340: Jenkins docker-workflow plugin couldn't run the image.
        # The same root cause appears in three wrappers; all three should
        # be recognised as a container-registry outage.
        for line in [
            "Caught exception: Failed to run image 'gcr.io/x/y:z'. Error: ",
            "ERROR: A fatal error occurred in the pipeline: "
            "Failed to run image 'gcr.io/x/y:z'. Error: ",
            "java.io.IOException: Failed to run image 'gcr.io/x/y:z'. Error: ",
        ]:
            out = list(OutageParser().scan([line], StaticStageContext("Build binary")))
            self.assertEqual(len(out), 1, f"no match for: {line!r}")
            self.assertEqual(out[0].severity, Severity.infra)
            self.assertTrue(out[0].headline.startswith("container-registry:"))

    def test_jenkins_workflow_error_action(self) -> None:
        # Jenkins-side workflow exception (vs. user-code failure).
        out = list(
            OutageParser().scan(
                [
                    "Also: org.jenkinsci.plugins.workflow.actions."
                    "ErrorAction$ErrorId: deadbeef",
                ],
                StaticStageContext("Test"),
            )
        )
        self.assertEqual(len(out), 1)
        self.assertEqual(out[0].severity, Severity.infra)
        self.assertTrue(out[0].headline.startswith("jenkins-pipeline-error:"))

    def test_no_findings_on_clean_log(self) -> None:
        out = list(
            OutageParser().scan(
                [
                    "Building target //foo:bar",
                    "Test passed",
                    "All done.",
                ],
                StaticStageContext("Build"),
            )
        )
        self.assertEqual(out, [])


if __name__ == "__main__":
    unittest.main()
