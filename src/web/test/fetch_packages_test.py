#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
#
# fetch_packages.py empties --dest before filling it, and skips the whole fetch
# when a stamp matches.  Both are quiet when wrong: the first takes whatever
# directory it is handed, the second serves a stale tree as if it were current.
# Neither needs the network to pin, so no case here reaches it.

import json
import os
import shutil
import subprocess
import sys
import tempfile

# Set before the sibling import, so no __pycache__ lands in the source tree.
sys.dont_write_bytecode = True
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from script_test import check, load_module, load_script, report  # noqa: E402

SOURCE_DIR = os.path.join(
    os.path.dirname(os.path.abspath(__file__)), "..", "third-party"
)
SCRIPT = os.path.join(SOURCE_DIR, "fetch_packages.py")

fetch = load_script("third-party/fetch_packages.py")


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


def a_matching_stamp_is_reused():
    """A tree a previous run left is kept, and still described to CMake."""
    with tempfile.TemporaryDirectory() as work:
        dest = os.path.join(work, "third-party")
        os.makedirs(dest)
        with open(os.path.join(dest, ".stamp"), "w", encoding="utf-8") as f:
            f.write(fetch.stamp_value() + "\n")
        marker = os.path.join(dest, "leaflet", "leaflet.js")
        os.makedirs(os.path.dirname(marker))
        with open(marker, "w", encoding="utf-8") as f:
            f.write("// the tree a previous run left\n")
        emitted = os.path.join(work, "web_third_party.cmake")

        result = run(dest, "--emit-cmake", emitted)
        check("the tree was not refetched", os.path.exists(marker))
        check(
            "a current stamp short-circuits the fetch",
            result.returncode == 0,
            result.stderr.strip(),
        )
        # Written from the manifest, not carried over from the earlier run, so
        # there is no stale file to detect.
        check("the emitted CMake was still written", os.path.exists(emitted))
        if os.path.exists(emitted):
            check(
                "it names this tree", marker in open(emitted, encoding="utf-8").read()
            )


def the_emitted_paths_come_from_the_manifest():
    """asset_paths() is what both the fetch and the reuse path emit."""
    manifest = fetch.read_manifest()
    assets = fetch.asset_paths(manifest, "/somewhere")
    expected = sum(
        len(s["files"]) + ("bundle" in s) for s in manifest["packages"].values()
    )
    check("every served file is listed", len(assets) == expected, f"{len(assets)}")
    check(
        "the served paths are absolute under /third-party/",
        all(p.startswith("/third-party/") for p in assets),
    )
    check(
        "the file paths are under the given dest",
        all(p.startswith("/somewhere/") for p in assets.values()),
    )


def the_stamp_covers_its_inputs():
    """A change to what is fetched, or to how, has to invalidate a tree."""

    def bump_version(text):
        manifest = json.loads(text)
        name = sorted(manifest["packages"])[0]
        manifest["packages"][name]["version"] += "-bumped"
        return json.dumps(manifest)

    edits = [
        ("editing the script", "fetch_packages.py", lambda t: t + "\n# a change\n"),
        ("bumping a version", "packages.json", bump_version),
    ]
    with tempfile.TemporaryDirectory() as work:
        copy = os.path.join(work, "third-party")
        shutil.copytree(SOURCE_DIR, copy, ignore=shutil.ignore_patterns("__pycache__"))
        script = os.path.join(copy, "fetch_packages.py")
        base = load_module(script).stamp_value()
        check("the same inputs give the same stamp", base == fetch.stamp_value())

        for what, name, edit in edits:
            path = os.path.join(copy, name)
            with open(path, encoding="utf-8") as f:
                original = f.read()
            with open(path, "w", encoding="utf-8") as f:
                f.write(edit(original))
            check(
                f"{what} changes the stamp", load_module(script).stamp_value() != base
            )
            with open(path, "w", encoding="utf-8") as f:
                f.write(original)


def main():
    a_directory_we_did_not_write_is_refused()
    the_source_tree_is_refused()
    a_matching_stamp_is_reused()
    the_emitted_paths_come_from_the_manifest()
    the_stamp_covers_its_inputs()
    return report("fetch_packages.py guards --dest and stamps its own inputs")


if __name__ == "__main__":
    sys.exit(main())
