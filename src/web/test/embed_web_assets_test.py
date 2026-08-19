#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
#
# The CSP names the inline <script> blocks by hash, so a hash computed over
# anything but the bytes that go into the binary is a block the browser refuses
# to run -- silently, and the viewer's import map is one of those blocks.

import base64
import hashlib
import importlib.util
import os
import sys

THIS_DIR = os.path.dirname(os.path.abspath(__file__))
SCRIPT = os.path.join(THIS_DIR, "..", "src", "embed_web_assets.py")

spec = importlib.util.spec_from_file_location("embed_web_assets", SCRIPT)
embed = importlib.util.module_from_spec(spec)
spec.loader.exec_module(embed)

failures = []


def check(name, condition, detail=""):
    if not condition:
        failures.append(f"{name}: {detail}" if detail else name)


def page(html):
    """One HTML asset, in the shape inline_script_hashes() takes."""
    return [("/index.html", "k_index_html", "text/html", html.encode("utf-8"))]


def expected(body):
    digest = hashlib.sha256(body.encode("utf-8")).digest()
    return f"'sha256-{base64.b64encode(digest).decode('ascii')}'"


def hashes_the_bytes_that_are_served():
    body = '\n{"imports": {"three": "/third-party/three/three.module.min.js"}}\n'
    html = f"<html><head><script type=importmap>{body}</script></head></html>"
    check(
        "a plain inline block hashes verbatim",
        embed.inline_script_hashes(page(html)) == [expected(body)],
    )


def a_comment_inside_a_block_is_part_of_the_bytes():
    """The case that used to produce a hash the browser never matches."""
    body = "\n// <!-- not a comment to the parser -->\nwindow.x = 1;\n"
    html = f"<html><body><script>{body}</script></body></html>"
    check(
        "a block containing <!-- hashes verbatim",
        embed.inline_script_hashes(page(html)) == [expected(body)],
    )


def a_block_inside_a_comment_is_still_ignored():
    """Why the comments are masked at all: the reason must survive the fix."""
    body = "\nwindow.real = 1;\n"
    html = (
        "<html><body>"
        "<!-- <script>window.commented = 1;</script> -->"
        f"<script>{body}</script>"
        "</body></html>"
    )
    check(
        "a commented-out block is not hashed",
        embed.inline_script_hashes(page(html)) == [expected(body)],
    )


def a_sourced_block_has_no_hash():
    html = '<html><body><script type="module" src="main.js"></script></body></html>'
    check("src= blocks are skipped", embed.inline_script_hashes(page(html)) == [])


def a_second_page_fails_the_build():
    """One policy is served with every response, so two pages cannot share it."""
    html = "<html><body><script>window.x = 1;</script></body></html>"
    assets = page(html) + [
        ("/other.html", "k_other_html", "text/html", html.encode("utf-8"))
    ]
    try:
        embed.inline_script_hashes(assets)
    except SystemExit:
        return
    failures.append("a second HTML asset: expected SystemExit, none raised")


def main():
    hashes_the_bytes_that_are_served()
    a_comment_inside_a_block_is_part_of_the_bytes()
    a_block_inside_a_comment_is_still_ignored()
    a_sourced_block_has_no_hash()
    a_second_page_fails_the_build()

    if failures:
        print("\n".join(failures), file=sys.stderr)
        return 1
    print("embed_web_assets.py hashes the bytes it embeds")
    return 0


if __name__ == "__main__":
    sys.exit(main())
