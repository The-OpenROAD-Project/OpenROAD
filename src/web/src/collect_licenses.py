#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
#
# Gather the licences of the npm packages bundled into the web viewer.  The
# binary serves the result as /THIRD_PARTY_LICENSES.txt and copies it into
# every saved report, since both now carry those packages' code (issue #11065).
#
# Each argument is a package directory holding its package.json and licence.

import argparse
import json
import os

_LICENSE_NAMES = ("license", "license.md", "license.txt", "licence", "copying")

_RULE = "=" * 78

_HEADER = """\
Third-party software in the OpenROAD web viewer

The viewer's browser bundle includes the npm packages below; a timing report
saved from it carries only golden-layout and leaflet.  Their licences follow.
"""

_FOOTER = """\
netlistsvg's prebuilt browser bundle also carries code from its own
dependencies.  The licence comments that code ships with are kept in the
"Bundled license information" block at the end of app.min.js.
"""


def find_license(pkg_dir):
    by_name = {name.lower(): name for name in os.listdir(pkg_dir)}
    for candidate in _LICENSE_NAMES:
        if candidate in by_name:
            return os.path.join(pkg_dir, by_name[candidate])
    raise SystemExit(f"{pkg_dir} has no licence file")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", "-o", required=True)
    parser.add_argument("packages", nargs="+", help="npm package directories")
    args = parser.parse_args()

    entries = []
    for pkg_dir in args.packages:
        with open(os.path.join(pkg_dir, "package.json"), encoding="utf-8") as f:
            meta = json.load(f)
        with open(find_license(pkg_dir), encoding="utf-8") as f:
            text = f.read().strip()
        entries.append((meta["name"], meta["version"], meta.get("license", "?"), text))
    entries.sort()

    parts = [_HEADER]
    for name, version, spdx, text in entries:
        parts.append(f"{_RULE}\n{name} {version} ({spdx})\n{_RULE}\n\n{text}\n")
    parts.append(_RULE + "\n\n" + _FOOTER)
    body = "\n".join(parts)

    # Written aside and renamed: a failure must not leave a truncated file newer
    # than its inputs, which the next build would keep.
    partial = args.output + ".tmp"
    with open(partial, "w", encoding="utf-8") as out:
        out.write(body)
    os.replace(partial, args.output)


if __name__ == "__main__":
    main()
