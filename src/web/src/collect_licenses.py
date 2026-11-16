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

_SHORTHAND_HOSTS = {
    "github": "github.com",
    "gitlab": "gitlab.com",
    "bitbucket": "bitbucket.org",
}

_RULE = "=" * 78

_HEADER = """\
Third-party software in the OpenROAD web viewer

The viewer's browser bundle includes the npm packages below.  Their licences
follow.
"""


def license_id(meta):
    value = meta.get("license") or meta.get("licenses")
    # Older packages give {"type": "MIT", "url": ...} or [{"type": "MIT"}].
    if isinstance(value, list) and value:
        value = value[0]
    if isinstance(value, dict):
        value = value.get("type")
    if isinstance(value, str):
        value = value.strip()
    return value if value and value != "?" and value.upper() != "UNLICENSED" else None


def find_license(pkg_dir):
    files = [
        name
        for name in os.listdir(pkg_dir)
        if os.path.isfile(os.path.join(pkg_dir, name))
    ]
    by_name = {name.lower(): name for name in files}
    for candidate in _LICENSE_NAMES:
        if candidate in by_name:
            return os.path.join(pkg_dir, by_name[candidate])
    # Otherwise a suffixed name, such as LICENSE-MIT or COPYING.txt.
    for name in sorted(files):
        if name.lower().startswith(("license", "licence", "copying")):
            return os.path.join(pkg_dir, name)
    return None


def source_url(meta):
    """Where the package's source can be obtained, as an https URL."""
    repo = meta.get("repository") or {}
    url = repo.get("url") if isinstance(repo, dict) else repo
    url = url or meta.get("homepage")
    if not url:
        raise SystemExit(f"{meta['name']} names no repository or homepage")
    url = re.sub(r"^git\+", "", url)
    url = re.sub(r"^git@([^:/]+):", r"https://\1/", url)
    url = re.sub(r"^(?:git|ssh|http)://(?:git@)?", "https://", url)
    # A fragment names a commit or an anchor, not where the source is.
    url = re.sub(r"#.*$", "", url)
    url = re.sub(r"\.git$", "", url)
    if "://" in url:
        return url
    # npm's shorthands: a bare "user/repo" is on GitHub, "host:user/repo" on host.
    shorthand = re.fullmatch(r"(?:(github|gitlab|bitbucket):)?([\w.-]+/[\w.-]+)", url)
    if not shorthand:
        raise SystemExit(f"{meta['name']}: cannot read {url!r} as a source URL")
    return f"https://{_SHORTHAND_HOSTS[shorthand[1] or 'github']}/{shorthand[2]}"


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
        lic_file = find_license(pkg_dir)
        spdx = license_id(meta)
        if not lic_file and not spdx:
            raise SystemExit(
                f"{meta.get('name', pkg_dir)} has neither a licence file nor a license in package.json"
            )
        if lic_file:
            with open(lic_file, encoding="utf-8") as f:
                text = f.read().strip()
        else:
            text = f"Licensed under {spdx} (declared in package.json)."
        entries[key] = (spdx or "?", source_url(meta), text)

    parts = [_HEADER]
    for (name, version), (spdx, source, text) in sorted(entries.items()):
        heading = f"{name} {version} ({spdx})\nSource: {source}"
        parts.append(f"{_RULE}\n{heading}\n{_RULE}\n\n{text}\n")

    with open(args.output, "w", encoding="utf-8") as out:
        out.write("\n".join(parts))


if __name__ == "__main__":
    main()
