# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""Normalize file paths emitted by CI to repo-relative form.

CI runners report absolute paths like
``/home/runner/work/OpenROAD/OpenROAD/tools/foo.py`` (GitHub Actions) or
``/var/jenkins_home/workspace/<job>/src/bar.cpp`` (Jenkins). For findings to
be actionable on a contributor's local checkout — including the
``//:fix-tldr`` recipes — the path needs to be repo-relative.
"""

from __future__ import annotations

import re

# GitHub Actions runner: /home/runner/work/<repo>/<repo>/<rest>
_GHA = re.compile(r"^/home/runner/work/[^/]+/[^/]+/")

# Jenkins workspace: /var/jenkins_home/workspace/<job>/<rest>, with optional
# extra @<n> path component for parallel runs.
_JENKINS = re.compile(r"^/var/jenkins_home/workspace/[^/]+(?:@\d+)?/")

# Generic /github/workspace/ (used by composite actions, container jobs).
_GH_WORKSPACE = re.compile(r"^/github/workspace/")


def to_repo_relative(path: str) -> str:
    """Strip a known CI workspace prefix; otherwise return `path` unchanged."""
    for pat in (_GHA, _JENKINS, _GH_WORKSPACE):
        path = pat.sub("", path, count=1)
    return path
