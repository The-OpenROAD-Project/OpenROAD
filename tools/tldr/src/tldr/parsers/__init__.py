# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""Parser registry. Each parser is a thin module under this package."""

from __future__ import annotations

from .base import Finding, Parser, Severity, StageContext
from . import (
    bazel_test,
    bazel_timeout,
    black,
    buildifier,
    clang_format,
    clang_tidy,
    ctest,
    docker_pull,
    jenkins_pipeline,
    lockfile,
    mac_build,
    metric_guard,
    openroad_msg,
    orfs_flow,
)

REGISTRY: dict[str, Parser] = {
    "orfs_flow": orfs_flow.OrfsFlowParser(),
    "metric_guard": metric_guard.MetricGuardParser(),
    "openroad_msg": openroad_msg.OpenroadMsgParser(),
    "bazel_test": bazel_test.BazelTestParser(),
    "bazel_timeout": bazel_timeout.BazelTimeoutParser(),
    "ctest": ctest.CtestParser(),
    "docker_pull": docker_pull.DockerPullParser(),
    "jenkins_pipeline": jenkins_pipeline.JenkinsPipelineParser(),
    "clang_tidy": clang_tidy.ClangTidyParser(),
    "clang_format": clang_format.ClangFormatParser(),
    "mac_build": mac_build.MacBuildParser(),
    "lockfile": lockfile.LockfileParser(),
    "black": black.BlackParser(),
    "buildifier": buildifier.BuildifierParser(),
}


def for_repo(repo: str, enabled: list[str] | None = None) -> list[Parser]:
    """Return parsers enabled for a given repo, in registry order."""
    keys = enabled if enabled is not None else list(REGISTRY)
    return [REGISTRY[k] for k in keys if k in REGISTRY]


__all__ = [
    "Finding",
    "Parser",
    "REGISTRY",
    "Severity",
    "StageContext",
    "for_repo",
]
