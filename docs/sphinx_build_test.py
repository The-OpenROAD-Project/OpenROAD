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
import sys
import tempfile

from sphinx.cmd.build import main as sphinx_main

# abspath (not realpath) so the path stays inside the runfiles tree instead
# of resolving symlinks back to the live workspace.
DOCS_DIR = os.path.dirname(os.path.abspath(__file__))


def main() -> int:
    temp_root = tempfile.mkdtemp(prefix="sphinx_test_")
    build_output = os.path.join(temp_root, "_build")
    temp_docs_dir = os.path.join(temp_root, "docs")
    saved_cwd = os.getcwd()
    try:
        # Copy docs directory and root README.md into temporary root
        # so conf.py setup(app) mutations do not affect workspace files.
        shutil.copytree(DOCS_DIR, temp_docs_dir, symlinks=True)

        readme_src = os.path.join(os.path.dirname(DOCS_DIR), "README.md")
        if os.path.exists(readme_src):
            shutil.copy2(readme_src, os.path.join(temp_root, "README.md"))

        # conf.py's setup(app) hook uses cwd-relative paths like "./main"
        # and "../README.md", so it must run with docs/ as cwd.
        os.chdir(temp_docs_dir)

        # Calling sphinx via subprocess would fail because a child python3
        # doesn't inherit the Bazel runfiles PYTHONPATH used by this test.

        args = ["-b", "html", "-T", "-q"]
        if not shutil.which("mmdc"):
            args.extend(["-D", "mermaid_output_format=raw"])
        args.extend([temp_docs_dir, build_output])

        print("Running sphinx-build...", flush=True)
        returncode = sphinx_main(args)

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
        shutil.rmtree(temp_root, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main())
