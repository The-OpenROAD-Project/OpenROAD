# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
#
# The page the Bazel build embeds: its libraries come from the bundle, and these
# tests catch a reintroduced remote reference before any browser meets it.

import re
import unittest

_PAGE = {
    "index.html": "src/web/bundle/index.html",
    "app.min.js": "src/web/bundle/app.min.js",
}
_LICENSES = "src/web/bundle/THIRD_PARTY_LICENSES.txt"

# Absolute URLs the bundle quotes as names rather than fetches: XML namespaces,
# homepage links in the kept comments.
_IDENTIFIER_URLS = (
    "http://www.w3.org/",
    "https://leafletjs.com",
    # The namespace netlistsvg stamps into the SVG it produces.
    "https://github.com/nturley/netlistsvg",
)

# Libraries the page still loads from CDNs, until the bundle takes them over.
_STILL_REMOTE = (
    "https://nturley.github.io/netlistsvg/",
    "https://esm.sh/three@0.160.0",
)

_URL = r"[^\"'`) >,;\n]*"
# Schemes and hosts are case-insensitive, so the scan is too.
_ABSOLUTE = re.compile(r"https?://" + _URL, re.I)
# Where a browser fetches a scheme-relative "//host/x" reference.
_SCHEME_RELATIVE = re.compile(
    r"(?:src=|href=|url\(|@import ?|fetch\(|import\(|from ?)[\"'`]?(//" + _URL + ")"
)


def _is_identifier_url(url):
    # An allowed prefix only matches up to a host or path boundary, so
    # "https://leafletjs.com.example/l.js" is not "https://leafletjs.com".
    for allowed in _IDENTIFIER_URLS:
        if not url.startswith(allowed):
            continue
        if allowed.endswith("/") or len(url) == len(allowed):
            return True
        if url[len(allowed)] in "/?#":
            return True
    return False


def external_urls(text):
    """Every absolute http(s) URL in text that is not an identifier."""
    urls = (url.lower() for url in _ABSOLUTE.findall(text))
    return [url for url in urls if not _is_identifier_url(url)]


def scheme_relative_urls(text):
    """References like "//host/x.js" where a browser would fetch them."""
    return _SCHEME_RELATIVE.findall(text)


def _read(path):
    with open(path, encoding="utf-8") as f:
        return f.read()


class Scanner(unittest.TestCase):
    # A scanner that finds nothing would pass every test below.
    def test_finds_absolute_urls_in_any_case(self):
        self.assertEqual(
            external_urls('<script src="https://unpkg.com/l.js">'),
            ["https://unpkg.com/l.js"],
        )
        self.assertEqual(len(external_urls("s.src='HTTPS://CDN.example/x.js'")), 1)
        self.assertEqual(external_urls('xmlns="http://www.w3.org/2000/svg"'), [])

    def test_allows_an_identifier_only_up_to_its_boundary(self):
        self.assertTrue(_is_identifier_url("https://leafletjs.com"))
        self.assertTrue(_is_identifier_url("https://leafletjs.com/reference"))
        self.assertFalse(_is_identifier_url("https://leafletjs.com.example/l.js"))

    def test_finds_scheme_relative_fetches(self):
        for text in (
            '<script src="//cdn.example/x.js">',
            "a{background:url(//cdn.example/i.png)}",
            "fetch('//cdn.example/x')",
            'import{a}from"//cdn.example/m.js"',
        ):
            self.assertEqual(len(scheme_relative_urls(text)), 1, text)
        self.assertEqual(scheme_relative_urls("x=1;// a comment\ny=2"), [])


class Page(unittest.TestCase):
    def test_loads_the_bundle(self):
        html = _read(_PAGE["index.html"])
        self.assertIn('<script type="module" src="app.min.js">', html)
        # The two golden-layout themes theme.js looks up by id.
        self.assertIn('id="gl-theme-dark"', html)
        self.assertIn('id="gl-theme-light"', html)
        # The icons the third-party stylesheets reach through url(), inlined.
        self.assertIn("data:image/png;base64,", html)

    def test_reaches_no_other_remote_resource(self):
        for name, path in _PAGE.items():
            text = _read(path)
            remote = [
                url
                for url in external_urls(text) + scheme_relative_urls(text)
                if not url.startswith(_STILL_REMOTE)
            ]
            self.assertEqual(remote, [], name)

    def test_ships_the_bundled_licences(self):
        text = _read(_LICENSES)
        for package in ("golden-layout", "leaflet"):
            self.assertIn(package, text)


if __name__ == "__main__":
    unittest.main()
