#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
#
# The CSP names the inline <script> blocks by hash, so a hash computed over
# anything but the bytes that go into the binary is a block the browser refuses
# to run -- silently, and the viewer's import map is one of those blocks.

import base64
import hashlib
import os
import sys

# Set before the sibling import, so no __pycache__ lands in the source tree.
sys.dont_write_bytecode = True
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from script_test import check, check_raises, load_script, report  # noqa: E402

embed = load_script("src/embed_web_assets.py")


def page(html):
    """One HTML asset, in the shape inline_script_hashes() takes."""
    return [("/index.html", "k_index_html", "text/html", html.encode("utf-8"))]


def expected(body):
    digest = hashlib.sha256(body.encode("utf-8")).digest()
    return f"'sha256-{base64.b64encode(digest).decode('ascii')}'"


def each_block_hashes_the_bytes_that_are_served():
    """Including a block that contains what looks like an HTML comment."""
    cases = {
        "a plain block": (
            '\n{"imports": {"three": "/third-party/three/three.module.min.js"}}\n',
            "<html><head><script type=importmap>{body}</script></head></html>",
        ),
        # This one used to hash the comment away, so the browser refused it.
        "a block containing <!--": (
            "\n// <!-- not a comment to the parser -->\nwindow.x = 1;\n",
            "<html><body><script>{body}</script></body></html>",
        ),
    }
    for name, (body, template) in cases.items():
        html = template.format(body=body)
        check(
            f"{name} hashes verbatim",
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
    check_raises("a second HTML asset", lambda: embed.inline_script_hashes(assets))


def main():
    each_block_hashes_the_bytes_that_are_served()
    a_block_inside_a_comment_is_still_ignored()
    a_sourced_block_has_no_hash()
    a_second_page_fails_the_build()
    return report("embed_web_assets.py hashes the bytes it embeds")


if __name__ == "__main__":
    sys.exit(main())
