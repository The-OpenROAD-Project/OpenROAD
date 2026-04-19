#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
"""Bazel test that replicates the ReadTheDocs Sphinx documentation build.

Verifies that ``sphinx-build`` completes without errors, catching broken
cross-references, malformed markup, missing files, and toc.yml issues
before they reach CI.

Usage::

    bazelisk test //docs:sphinx_build_test
    bazelisk test --test_tag_filters=doc_check //docs/...
"""

import os
import shutil
import subprocess
import sys
import tempfile


def _find_docs_dir() -> str:
    """Locate the docs directory at test runtime.

    Under ``bazel test``, sources declared as ``data`` are staged in the
    runfiles tree next to this script. Under a plain Python invocation
    from the workspace, the script lives in the real ``docs/`` directory.
    In both cases the docs content sits alongside this file. ``abspath``
    (not ``realpath``) is used so the script stays inside the runfiles
    tree instead of resolving symlinks back to the live workspace.
    """
    return os.path.dirname(os.path.abspath(__file__))


def main() -> int:
    docs_dir = _find_docs_dir()
    repo_root = os.path.dirname(docs_dir)
    build_output = tempfile.mkdtemp(prefix="sphinx_build_")

    # Track files/dirs to clean up
    cleanup_paths = [build_output]

    # Sanity-check that conf.py was staged (catches broken runfiles configs
    # with a clear error instead of a silent sphinx-build failure).
    conf_py = os.path.join(docs_dir, "conf.py")
    if not os.path.isfile(conf_py):
        print(
            f"FAILED: conf.py not found at {conf_py}. "
            "docs/ sources are not staged in runfiles.",
            file=sys.stderr,
        )
        return 1

    try:
        # --- Pre-build setup (mirrors conf.py setup()) ---

        # Create the `main` symlink that conf.py expects, pointing at the
        # docs_dir's parent (the workspace / runfiles root).
        main_link = os.path.join(docs_dir, "main")
        if os.path.islink(main_link) or os.path.exists(main_link):
            if os.path.islink(main_link):
                os.unlink(main_link)
        os.symlink(repo_root, main_link)
        cleanup_paths.append(main_link)

        # Ensure README.md and README2.md exist in repo_root. conf.py's setup()
        # hook calls shutil.copy + swap_prefix on both; under Bazel runfiles
        # neither file is staged, so we write stubs and clean them up. Under a
        # real workspace run, README.md already exists and is left untouched.
        readme = os.path.join(repo_root, "README.md")
        readme2 = os.path.join(repo_root, "README2.md")
        if not os.path.isfile(readme):
            with open(readme, "w") as f:
                f.write("# OpenROAD (sphinx build stub)\n")
            cleanup_paths.append(readme)
        if not os.path.isfile(readme2):
            shutil.copy2(readme, readme2)
            cleanup_paths.append(readme2)

        # Generate messages glossary (best-effort; needs ../etc and ../src
        # which aren't in Bazel runfiles — skip cleanly if unavailable).
        get_messages = os.path.join(docs_dir, "getMessages.py")
        find_messages = os.path.join(repo_root, "etc", "find_messages.py")
        if os.path.isfile(get_messages) and os.path.isfile(find_messages):
            subprocess.run(
                [sys.executable, "getMessages.py"],
                cwd=docs_dir,
                capture_output=True,
                check=True,
            )
        else:
            print(
                "Skipping getMessages.py (find_messages.py not in runfiles).",
                flush=True,
            )

        # Stub doxygen output directory (only mark for cleanup if we created it)
        readthedocs_dir = os.path.join(repo_root, "_readthedocs")
        created_readthedocs = not os.path.exists(readthedocs_dir)
        doxygen_dir = os.path.join(readthedocs_dir, "html", "doxygen_output")
        os.makedirs(doxygen_dir, exist_ok=True)
        if created_readthedocs:
            cleanup_paths.append(readthedocs_dir)

        # --- Run sphinx-build in-process ---
        # Calling sphinx via subprocess would fail because a child ``python3``
        # doesn't see the Bazel runfiles ``PYTHONPATH`` used by this test.
        from sphinx.cmd.build import main as sphinx_main

        sphinx_args = ["-b", "html", "-T", "-q"]
        # Skip mermaid rendering if mmdc is not installed
        if not shutil.which("mmdc"):
            sphinx_args.extend(["-D", "mermaid_output_format=raw"])
        sphinx_args.extend([docs_dir, build_output])

        print("Running sphinx-build...", flush=True)
        saved_cwd = os.getcwd()
        try:
            os.chdir(docs_dir)
            returncode = sphinx_main(sphinx_args)
        finally:
            os.chdir(saved_cwd)

        # Revert README prefix swaps
        revert = os.path.join(docs_dir, "revert-links.py")
        if os.path.isfile(revert):
            subprocess.run(
                [sys.executable, revert],
                cwd=docs_dir,
                capture_output=True,
                check=True,
            )

        if returncode != 0:
            print(
                f"\nFAILED: sphinx-build exited with status {returncode}",
                file=sys.stderr,
            )
            return 1

        print("PASSED: Sphinx documentation built successfully.")
        return 0

    finally:
        for path in cleanup_paths:
            if os.path.islink(path):
                os.unlink(path)
            elif os.path.isdir(path):
                shutil.rmtree(path, ignore_errors=True)
            elif os.path.isfile(path):
                os.unlink(path)


if __name__ == "__main__":
    sys.exit(main())
