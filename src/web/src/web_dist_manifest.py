#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
#
# "write" (Bazel) records the hash of every file src/web/dist is built from;
# "check" (CMake, which cannot rebuild dist/) rehashes the files it names.

import argparse
import hashlib
import os
import sys

_FIX = "bazel run //src/web/dist:dist"


def digest(path):
    with open(path, "rb") as f:
        return hashlib.sha256(f.read()).hexdigest()


def read_manifest(path):
    recorded = {}
    with open(path, encoding="utf-8") as f:
        for line in f:
            hexdigest, _, rel = line.rstrip("\n").partition("  ")
            recorded[rel] = hexdigest
    return recorded


def main():
    parser = argparse.ArgumentParser()
    modes = parser.add_subparsers(dest="mode")
    modes.required = True
    write = modes.add_parser("write")
    check = modes.add_parser("check")
    for mode in (write, check):
        mode.add_argument("--root", required=True, help="the src/web directory")
        mode.add_argument("--manifest", required=True)
    write.add_argument("files", nargs="+", help="what dist/ is built from")
    args = parser.parse_args()

    if args.mode == "write":
        entries = {
            os.path.relpath(path, args.root).replace(os.sep, "/"): digest(path)
            for path in args.files
        }
        # sha256sum's format, so `sha256sum -c` can read it too.
        with open(args.manifest, "w", encoding="utf-8") as out:
            out.write("".join(f"{entries[rel]}  {rel}\n" for rel in sorted(entries)))
        return

    stale = []
    for rel, recorded in sorted(read_manifest(args.manifest).items()):
        path = os.path.join(args.root, rel)
        if not os.path.isfile(path) or digest(path) != recorded:
            stale.append(rel)
    if stale:
        sys.exit(
            "src/web/dist is older than these sources:\n  "
            + "\n  ".join(stale)
            + f"\nThe bundler only runs under Bazel; regenerate it with `{_FIX}`."
        )


if __name__ == "__main__":
    main()
