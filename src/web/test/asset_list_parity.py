#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
#
# The web assets are listed once for Bazel and once for CMake, and nothing else
# ties the two together: a file forgotten in one 404s under that build system,
# and the report's list is ordered, since its files share one scope.  Both have
# drifted once already.  This test is the tie.

import os
import re
import sys

WEB_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")


def read(name):
    with open(os.path.join(WEB_DIR, name), encoding="utf-8") as f:
        return f.read()


def bazel_list(build, name):
    """The paths in a `name = [ ... ]` list assignment."""
    body = re.search(rf"^{name} = \[(.*?)^\]", build, re.M | re.S).group(1)
    return re.findall(r'"([^"]+)"', body)


def cmake_list(cmakelists, name):
    """The paths in a `set(NAME ...)` block, comments dropped."""
    body = re.search(rf"^set\({name}\n(.*?)^\)", cmakelists, re.M | re.S).group(1)
    return [
        line.strip()
        for line in body.splitlines()
        if line.strip() and not line.strip().startswith("#")
    ]


def cmake_command_args(cmakelists, after):
    """The src/ paths passed on a command line, in order."""
    body = cmakelists[cmakelists.index(after) :]
    body = body[: body.index("DEPENDS")]
    return re.findall(r"\$\{CMAKE_CURRENT_SOURCE_DIR\}/(\S+)", body)


def main():
    build = read("BUILD")
    cmakelists = read("CMakeLists.txt")

    problems = []

    # The embedded assets: same set, since the served path is derived from the
    # file path the same way on both sides.
    bazel_assets = set(bazel_list(build, "_WEB_ASSET_FILES"))
    cmake_assets = set(cmake_list(cmakelists, "WEB_ASSET_FILES"))
    for missing in sorted(bazel_assets - cmake_assets):
        problems.append(f"embedded in BUILD but not in CMakeLists.txt: {missing}")
    for missing in sorted(cmake_assets - bazel_assets):
        problems.append(f"embedded in CMakeLists.txt but not in BUILD: {missing}")

    # The report JS: same order, not just the same set.
    bazel_js = bazel_list(build, "_REPORT_JS_FILES")
    cmake_js = [
        path
        for path in cmake_command_args(cmakelists, "embed_report_assets.py")
        if path != "src/style.css" and path != "src/embed_report_assets.py"
    ]
    if bazel_js != cmake_js:
        problems.append(
            "the report JS lists differ (order matters -- the files share one "
            f"scope):\n  BUILD: {bazel_js}\n  CMake: {cmake_js}"
        )

    if problems:
        print("\n".join(problems), file=sys.stderr)
        return 1

    print(
        f"{len(bazel_assets)} embedded assets and {len(bazel_js)} report JS files "
        "match between BUILD and CMakeLists.txt"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
