# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
"""Shared helpers for unit tests."""

from __future__ import annotations

import pathlib

FIXTURES = pathlib.Path(__file__).parent / "fixtures"


def read_lines(name: str) -> list[str]:
    return (FIXTURES / name).read_text().splitlines()


class StaticStageContext:
    def __init__(self, stage: str | None = None) -> None:
        self._stage = stage

    @property
    def current(self) -> str | None:
        return self._stage
