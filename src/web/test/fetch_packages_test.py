#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
#
# fetch_packages.py empties --dest before filling it, and skips the whole fetch
# when a stamp matches.  Both are quiet when wrong: the first takes whatever
# directory it is handed, the second serves a stale tree as if it were current.
# Neither needs the network to pin, so it is done here.

import importlib.util
import os
import shutil
import subprocess
import sys
import tempfile

THIS_DIR = os.path.dirname(os.path.abspath(__file__))
SOURCE_DIR = os.path.join(THIS_DIR, "..", "third-party")
SCRIPT = os.path.join(SOURCE_DIR, "fetch_packages.py")

failures = []


def check(name, condition, detail=""):
    if not condition:
        failures.append(f"{name}: {detail}" if detail else name)


def load(directory, name):
    """Import a copy of the script, so its stamp reads that copy's files."""
    spec = importlib.util.spec_from_file_location(
        name, os.path.join(directory, "fetch_packages.py")
    )
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def run(dest, *extra):
    """Run the script for real.  Every case here stops before the network."""
    return subprocess.run(
        [sys.executable, SCRIPT, "--dest", dest, *extra],
        capture_output=True,
        text=True,
        check=False,
    )


def a_directory_we_did_not_write_is_refused():
    """The case that would delete the manifest and the script itself."""
    with tempfile.TemporaryDirectory() as work:
        dest = os.path.join(work, "third-party")
        os.makedirs(dest)
        keep = os.path.join(dest, "packages.json")
        with open(keep, "w", encoding="utf-8") as f:
            f.write("{}\n")

        result = run(dest)
        check(
            "a directory with no .stamp is refused",
            result.returncode != 0,
            f"exited {result.returncode}",
        )
        check("the refusal says why", ".stamp" in result.stderr, result.stderr.strip())
        check("nothing was deleted", os.path.exists(keep))


def a_matching_stamp_is_reused():
    """A build directory the script already filled is left alone, and kept."""
    with tempfile.TemporaryDirectory() as work:
        dest = os.path.join(work, "third-party")
        os.makedirs(dest)
        with open(os.path.join(dest, ".stamp"), "w", encoding="utf-8") as f:
            f.write(load(SOURCE_DIR, "fetch_current").stamp_value() + "\n")
        marker = os.path.join(dest, "leaflet", "leaflet.js")
        os.makedirs(os.path.dirname(marker))
        with open(marker, "w", encoding="utf-8") as f:
            f.write("// the tree a previous run left\n")
        emitted = os.path.join(work, "web_third_party.cmake")
        with open(emitted, "w", encoding="utf-8") as f:
            # Naming this tree is what makes it reusable: one describing another
            # --dest would be reused into paths to nothing.
            f.write(f'set(WEB_THIRD_PARTY_FILES\n  "{marker}"\n)\n')

        result = run(dest, "--emit-cmake", emitted)
        check("the tree was not refetched", os.path.exists(marker))
        check(
            "a current stamp short-circuits the fetch",
            result.returncode == 0,
            result.stderr.strip(),
        )


def an_emitted_file_for_another_tree_is_not_reused():
    """A stamp is not enough: the emitted paths have to point into this --dest."""
    with tempfile.TemporaryDirectory() as work:
        dest = os.path.join(work, "third-party")
        os.makedirs(dest)
        with open(os.path.join(dest, ".stamp"), "w", encoding="utf-8") as f:
            f.write(load(SOURCE_DIR, "fetch_current_other").stamp_value() + "\n")
        emitted = os.path.join(work, "web_third_party.cmake")
        with open(emitted, "w", encoding="utf-8") as f:
            f.write('set(WEB_THIRD_PARTY_FILES\n  "/somewhere/else/leaflet.js"\n)\n')

        # Offline in CI, so it fails at the download; what is pinned is that it
        # got past the short-circuit instead of reusing the wrong file.
        result = run(dest, "--emit-cmake", emitted)
        check(
            "an emitted file naming another tree is not reused",
            result.returncode != 0 or "fetching" in result.stderr,
            result.stderr.strip()[:200],
        )


def the_source_tree_is_refused():
    """The directory the script lives in, which WEB_THIRD_PARTY invites."""
    result = run(SOURCE_DIR)
    check(
        "the script's own directory is refused",
        result.returncode != 0,
        f"exited {result.returncode}",
    )
    for name in ("packages.json", "fetch_packages.py"):
        check(f"{name} survived", os.path.exists(os.path.join(SOURCE_DIR, name)))


def the_stamp_covers_its_inputs():
    """A change to what is fetched, or to how, has to invalidate a tree."""
    with tempfile.TemporaryDirectory() as work:
        pristine = os.path.join(work, "pristine")
        edited = os.path.join(work, "edited")
        for copy in (pristine, edited):
            shutil.copytree(
                SOURCE_DIR, copy, ignore=shutil.ignore_patterns("__pycache__")
            )

        base = load(pristine, "fetch_pristine").stamp_value()
        check(
            "the same inputs give the same stamp",
            load(edited, "fetch_same").stamp_value() == base,
        )

        script = os.path.join(edited, "fetch_packages.py")
        with open(script, "rb") as f:
            original_script = f.read()
        with open(script, "wb") as f:
            f.write(original_script + b"\n# a change to the fetch logic\n")
        check(
            "editing the script changes the stamp",
            load(edited, "fetch_edited_script").stamp_value() != base,
        )
        with open(script, "wb") as f:
            f.write(original_script)

        manifest = os.path.join(edited, "packages.json")
        with open(manifest, "rb") as f:
            original_manifest = f.read()
        bumped = original_manifest.replace(b'"version": "1.9.4"', b'"version": "1.9.5"')
        check("the version to bump was found", bumped != original_manifest)
        with open(manifest, "wb") as f:
            f.write(bumped)
        check(
            "bumping a version changes the stamp",
            load(edited, "fetch_edited_manifest").stamp_value() != base,
        )


def main():
    a_directory_we_did_not_write_is_refused()
    a_matching_stamp_is_reused()
    an_emitted_file_for_another_tree_is_not_reused()
    the_source_tree_is_refused()
    the_stamp_covers_its_inputs()

    if failures:
        print("\n".join(failures), file=sys.stderr)
        return 1
    print("fetch_packages.py guards --dest and stamps its own inputs")
    return 0


if __name__ == "__main__":
    sys.exit(main())
