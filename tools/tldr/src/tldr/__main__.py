# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""Entry point for `python -m tldr` and the `//tools/tldr:tldr` py_binary.

Uses absolute imports so the same file works both as a package member
(``python -m tldr``) and as a Bazel py_binary main (where Python runs it as
the top-level ``__main__`` script with no parent package).
"""

from __future__ import annotations

import sys

from tldr.cli import main

if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
