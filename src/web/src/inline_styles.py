#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
#
# Sew the bundled stylesheets into index.html: each --style file replaces the
# marker named after it, e.g. app.min.css fills /*! app.min.css */.

import argparse
import os


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--html", required=True)
    parser.add_argument("--output", "-o", required=True)
    parser.add_argument(
        "--style",
        action="append",
        default=[],
        metavar="FILE",
        required=True,
    )
    args = parser.parse_args()

    with open(args.html, encoding="utf-8") as f:
        html = f.read()

    for path in args.style:
        marker = f"/*! {os.path.basename(path)} */"
        if marker not in html:
            raise SystemExit(f"{args.html} has no placeholder {marker}")
        with open(path, encoding="utf-8") as f:
            css = f.read()
        # A stylesheet containing "</style" would close the block early.  esbuild
        # escapes it in its own output, so this only fires on hand-written CSS.
        if "</style" in css.lower():
            raise SystemExit(f"{path} contains '</style', which would end the block")
        html = html.replace(marker, css, 1)

    # Written aside and renamed so a failure cannot leave a half-written file
    # newer than its inputs, which the next build would keep.
    partial = args.output + ".tmp"
    with open(partial, "w", encoding="utf-8") as out:
        out.write(html)
    os.replace(partial, args.output)


if __name__ == "__main__":
    main()
