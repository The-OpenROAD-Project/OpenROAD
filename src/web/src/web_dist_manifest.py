#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
#
# The bundles checked in under src/web/dist are only as fresh as the last
# `bazel run //src/web/dist:dist`.  Bazel records what they were built from
# ("write"); the CMake build, which cannot rebuild them, checks that record
# against the sources it is building ("check") and stops rather than embed a
# stale copy.

import argparse
import hashlib
import os
import sys

_FIX = "bazel run //src/web/dist:dist"


def digests(root, files):
    """Map each file, relative to root, to the sha256 of its contents."""
    entries = {}
    for path in files:
        with open(path, "rb") as f:
            digest = hashlib.sha256(f.read()).hexdigest()
        entries[os.path.relpath(path, root).replace(os.sep, "/")] = digest
    return entries


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("mode", choices=["write", "check"])
    parser.add_argument("--root", required=True, help="the src/web directory")
    parser.add_argument("--manifest", required=True)
    parser.add_argument("--stamp", help="touched when check passes")
    parser.add_argument("files", nargs="+", help="what the bundles are built from")
    args = parser.parse_args()

    current = digests(args.root, args.files)

    if args.mode == "write":
        # sha256sum's format, so `sha256sum -c` can read it too.
        text = "".join(f"{current[rel]}  {rel}\n" for rel in sorted(current))
        partial = args.manifest + ".tmp"
        with open(partial, "w", encoding="utf-8") as out:
            out.write(text)
        os.replace(partial, args.manifest)
        return

    recorded = {}
    with open(args.manifest, encoding="utf-8") as f:
        for line in f:
            digest, _, rel = line.rstrip("\n").partition("  ")
            recorded[rel] = digest
    stale = sorted(
        rel
        for rel in current.keys() | recorded.keys()
        if current.get(rel) != recorded.get(rel)
    )
    if stale:
        sys.exit(
            "src/web/dist is older than these sources:\n  "
            + "\n  ".join(stale)
            + f"\nThe bundler only runs under Bazel; regenerate it with `{_FIX}`."
        )
    if args.stamp:
        with open(args.stamp, "w", encoding="utf-8"):
            pass


if __name__ == "__main__":
    main()
