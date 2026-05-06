# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""Parser contract: Finding, Severity, Parser Protocol."""

from __future__ import annotations

from dataclasses import dataclass, field
from enum import Enum
from typing import Iterable, Protocol


class Severity(str, Enum):
    actionable = "actionable"
    infra = "infra"
    info = "info"


@dataclass(frozen=True)
class Finding:
    parser: str
    severity: Severity
    kind: str
    headline: str
    detail: str = ""
    stage: str | None = None
    location: str | None = None
    log_url: str | None = None
    log_line: int | None = None
    dedupe_key: str = ""

    # Optional fields consumed by the renderers.
    human_headline: str | None = None
    ai_directive: str | None = None
    verify_command: str | None = None
    auto_fix_command: tuple[str, ...] | None = None
    extras: tuple[tuple[str, str], ...] = field(default_factory=tuple)

    def with_stage(self, stage: str | None) -> "Finding":
        return Finding(
            parser=self.parser,
            severity=self.severity,
            kind=self.kind,
            headline=self.headline,
            detail=self.detail,
            stage=stage if self.stage is None else self.stage,
            location=self.location,
            log_url=self.log_url,
            log_line=self.log_line,
            dedupe_key=self.dedupe_key or f"{self.parser}:{self.kind}:{self.headline}",
            human_headline=self.human_headline,
            ai_directive=self.ai_directive,
            verify_command=self.verify_command,
            auto_fix_command=self.auto_fix_command,
            extras=self.extras,
        )


class StageContext(Protocol):
    """What Jenkins pipeline stage is currently active for this line."""

    @property
    def current(self) -> str | None: ...


class Parser(Protocol):
    """A small, single-purpose log-failure parser.

    Implementations live in this package alongside `base.py`. Each parser is
    registered in `parsers/__init__.py:REGISTRY`.
    """

    name: str

    def applies(self, check_name: str, repo: str) -> bool: ...

    def scan(self, lines: Iterable[str], ctx: StageContext) -> Iterable[Finding]: ...
