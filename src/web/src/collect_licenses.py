#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
#
# Gather the licences of the npm packages bundled into the web viewer, which the
# binary serves as /THIRD_PARTY_LICENSES.txt.

import argparse
import json
import os
import re

_LICENSE_NAMES = ("license", "license.md", "license.txt", "licence", "copying")

_RULE = "=" * 78

_HEADER = """\
Third-party software in the OpenROAD web viewer

The viewer's browser bundle includes the npm packages below.  Their licences
follow.
"""


def find_license(pkg_dir):
    by_name = {name.lower(): name for name in os.listdir(pkg_dir)}
    for candidate in _LICENSE_NAMES:
        if candidate in by_name:
            return os.path.join(pkg_dir, by_name[candidate])
    raise SystemExit(f"{pkg_dir} has no licence file")


def source_url(meta):
    """Where the package's source can be obtained, as an https URL."""
    repo = meta.get("repository") or meta.get("homepage") or ""
    url = repo.get("url", "") if isinstance(repo, dict) else repo
    url = re.sub(r"^git\+", "", url)
    url = re.sub(r"^git@([^:/]+):", r"https://\1/", url)
    url = re.sub(r"^(?:git|ssh)://(?:git@)?", "https://", url)
    url = re.sub(r"\.git$", "", url)
    # npm reads a bare "user/repo" as a GitHub repository.
    return url if "://" in url else f"https://github.com/{url}"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", "-o", required=True)
    parser.add_argument("packages", nargs="+", help="npm package directories")
    args = parser.parse_args()

    # A package reached through several dependents is listed once per path.
    entries = {}
    for pkg_dir in args.packages:
        with open(os.path.join(pkg_dir, "package.json"), encoding="utf-8") as f:
            meta = json.load(f)
        key = (meta["name"], meta["version"])
        if key in entries:
            continue
        with open(find_license(pkg_dir), encoding="utf-8") as f:
            text = f.read().strip()
        entries[key] = (meta.get("license", "?"), source_url(meta), text)

    parts = [_HEADER]
    for (name, version), (spdx, source, text) in sorted(entries.items()):
        heading = f"{name} {version} ({spdx})\nSource: {source}"
        parts.append(f"{_RULE}\n{heading}\n{_RULE}\n\n{text}\n")

    with open(args.output, "w", encoding="utf-8") as out:
        out.write("\n".join(parts))


if __name__ == "__main__":
    main()
