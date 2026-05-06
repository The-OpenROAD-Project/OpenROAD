# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""Surface the Jenkins pipeline's `Finished: <state>` outcome marker.

Jenkins logs end with one of:

  Finished: SUCCESS
  Finished: FAILURE
  Finished: UNSTABLE
  Finished: ABORTED
  Finished: NOT_BUILT

UNSTABLE and ABORTED are particularly informative when the GitHub status
flips a PR red without a matching `FAILURE` line in the log — the user
probably wants to know *which* of the two it was. SUCCESS is also worth
surfacing as info: if GitHub status is `error` but the log says SUCCESS,
the GitHub commit-status is stale (observed on PR #10332).
"""

from __future__ import annotations

import re
from typing import Iterable

from .base import Finding, Severity, StageContext

_PATTERN = re.compile(
    r"^Finished:\s+(?P<state>SUCCESS|FAILURE|UNSTABLE|ABORTED|NOT_BUILT)\b"
)


class PipelineOutcomeParser:
    name = "pipeline_outcome"

    def applies(self, check_name: str, repo: str) -> bool:
        return "jenkins" in check_name.lower()

    def scan(self, lines: Iterable[str], ctx: StageContext) -> Iterable[Finding]:
        last_state: str | None = None
        for line in lines:
            m = _PATTERN.match(line)
            if m:
                last_state = m["state"]
        if last_state is None:
            return
        if last_state == "FAILURE":
            # FAILURE is already covered by the parsers that picked up the
            # actual failing step; emitting another finding here would just
            # duplicate the row.
            return
        if last_state == "SUCCESS":
            # Only surface when GitHub status is `failure`/`error` but the
            # build itself succeeded — i.e. the status is stale. We can't
            # know the GitHub state from inside this parser, so let the
            # discovery layer decide; emit info anyway so a stale-status
            # check is one comparison away.
            yield Finding(
                parser=self.name,
                severity=Severity.info,
                kind="pipeline_outcome",
                headline="Jenkins pipeline finished SUCCESS",
                detail="`Finished: SUCCESS` — GitHub status is likely stale.",
                stage=None,
                dedupe_key="pipeline_outcome:SUCCESS",
                human_headline=(
                    "Jenkins pipeline finished SUCCESS — GitHub status may be stale"
                ),
                ai_directive=None,
                verify_command=None,
                auto_fix_command=None,
            )
            return
        # UNSTABLE / ABORTED / NOT_BUILT — informational, no AI action.
        yield Finding(
            parser=self.name,
            severity=Severity.info,
            kind="pipeline_outcome",
            headline=f"Jenkins pipeline finished {last_state}",
            detail=f"`Finished: {last_state}`",
            stage=None,
            dedupe_key=f"pipeline_outcome:{last_state}",
            human_headline=(
                f"Jenkins pipeline ended `{last_state}` "
                "(not a hard failure — see log for details)"
            ),
            ai_directive=None,
            verify_command=None,
            auto_fix_command=None,
        )
