#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
"""Bazel test that replicates the ReadTheDocs Sphinx documentation build.

Runs ``sphinx-build`` against the project's real ``docs/conf.py`` so any
regression that would break ReadTheDocs (broken cross-references,
malformed markup, missing files, ``toc.yml`` issues) is also caught
locally before it reaches CI.

Usage::

    bazelisk test //docs:sphinx_build_test
    bazelisk test --test_tag_filters=doc_check //docs/...
"""

import os
import shutil
import subprocess
import sys
import tempfile

# abspath (not realpath) so the path stays inside the runfiles tree instead
# of resolving symlinks back to the live workspace.
DOCS_DIR = os.path.dirname(os.path.abspath(__file__))


def main() -> int:
    build_output = tempfile.mkdtemp(prefix="sphinx_build_")
    saved_cwd = os.getcwd()
    try:
        # conf.py's setup(app) hook uses cwd-relative paths like "./main"
        # and "../README.md", so it must run with docs/ as cwd. Sphinx
        # invokes setup(app) automatically when it loads conf.py below.
        os.chdir(DOCS_DIR)

        # Calling sphinx via subprocess would fail because a child python3
        # doesn't inherit the Bazel runfiles PYTHONPATH used by this test.
        from sphinx.cmd.build import main as sphinx_main

        args = ["-b", "html", "-T", "-q"]
        if not shutil.which("mmdc"):
            args.extend(["-D", "mermaid_output_format=raw"])
        args.extend([DOCS_DIR, build_output])

        print("Running sphinx-build...", flush=True)
        returncode = sphinx_main(args)

        # conf.py's setup(app) rewrites README.md and README2.md in place.
        # revert-links.py undoes those mutations so the test leaves no trace.
        revert = os.path.join(DOCS_DIR, "revert-links.py")
        if os.path.isfile(revert):
            subprocess.run(
                [sys.executable, revert],
                cwd=DOCS_DIR,
                capture_output=True,
                check=True,
            )

        if returncode != 0:
            print(
                f"FAILED: sphinx-build exited with status {returncode}",
                file=sys.stderr,
            )
            return 1
        print("PASSED: Sphinx documentation built successfully.")
        return 0
    finally:
        os.chdir(saved_cwd)
        shutil.rmtree(build_output, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main())
