# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""Strip ANSI escape sequences and Jenkins timestamp prefixes from log lines."""

from __future__ import annotations

import re
from typing import Iterable, Iterator

# CSI / OSC / private ANSI escapes. Matches the bulk of `tput`-emitted noise.
_ANSI = re.compile(r"\x1B(?:[@-Z\\-_]|\[[0-?]*[ -/]*[@-~])")

# Jenkins consoleText timestamp prefix, e.g. "[2026-05-04T11:03:22.234Z] ".
_TS_BRACKET = re.compile(r"^\[\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}(?:\.\d+)?Z?\]\s?")

# GitHub Actions bare ISO timestamp prefix, e.g. "2026-05-04T11:00:00.000Z ".
_TS_BARE = re.compile(r"^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}(?:\.\d+)?Z\s")

# `[Pipeline] sh` echo prefix that some configurations emit on every step.
_PIPELINE_ECHO = re.compile(r"^\+\s")


def strip_line(line: str) -> str:
    line = _ANSI.sub("", line)
    line = _TS_BRACKET.sub("", line)
    line = _TS_BARE.sub("", line)
    return line.rstrip("\r\n")


def strip_lines(lines: Iterable[str]) -> Iterator[str]:
    for line in lines:
        yield strip_line(line)
