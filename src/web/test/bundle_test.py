# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
#
# The page the Bazel build embeds and the script a saved report inlines load
# nothing remote; these tests catch a reintroduced reference.

import re
import unittest

_PAGE = {
    "index.html": "src/web/bundle/index.html",
    "app.min.js": "src/web/bundle/app.min.js",
}
# Inlined into every saved report, which opens with no server at all.
_REPORT_JS = "src/web/bundle/report.min.js"
_LICENSES = "src/web/bundle/THIRD_PARTY_LICENSES.txt"
_ENTRIES = ("src/web/src/entry-app.js", "src/web/src/entry-report.js")

# Absolute URLs the bundle quotes as names rather than fetches, each matched
# whole and in lower case: XML namespaces, ELK's model URIs, links in comments
# and messages.
_IDENTIFIER_URLS = {
    "http://www.w3.org/1999/xhtml",
    "http://www.w3.org/1999/xlink",
    "http://www.w3.org/2000/svg",
    "http://www.w3.org/2000/xmlns/",
    "http://www.w3.org/2001/xmlschema",
    "http://www.w3.org/xml/1998/namespace",
    "http://www.eclipse.org/elk/elkgraph",
    "http://www.eclipse.org/emf/2002/ecore",
    "http://www.eclipse.org/emf/2003/xmltype",
    "http:///org/eclipse/emf/ecore/util/extendedmetadata",
    "http://mths.be/fromcodepoint",
    "http://underscorejs.org/license",
    "https://lodash.com/",
    "https://lodash.com/license",
    "https://openjsf.org/",
    "https://npms.io/search?q=ponyfill",
    "https://leafletjs.com",
    "https://discourse.threejs.org/t/updates-to-lighting-in-three-js-r155/53733",
    # The namespace netlistsvg stamps into the SVG it produces.
    "https://github.com/nturley/netlistsvg",
}

_URL = r"[^\"'`) >,;\n]*"
# Schemes and hosts are case-insensitive, so the scan is too.
_ABSOLUTE = re.compile(r"https?://" + _URL, re.I)
# Where a browser fetches a scheme-relative "//host/x" reference.
_SCHEME_RELATIVE = re.compile(
    r"(?:src=|href=|url\(|@import ?|fetch\(|import\(|from ?)[\"'`]?(//" + _URL + ")"
)


def _is_identifier_url(url):
    # A fragment never reaches a server; a comment may end the URL's sentence.
    return url.split("#", 1)[0].rstrip(".") in _IDENTIFIER_URLS


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

    def test_allows_only_a_whole_identifier(self):
        self.assertTrue(_is_identifier_url("https://leafletjs.com"))
        self.assertTrue(_is_identifier_url("http://www.w3.org/2000/svg#x"))
        self.assertFalse(_is_identifier_url("https://leafletjs.com/l.js"))
        self.assertFalse(_is_identifier_url("https://leafletjs.com.example/l.js"))
        self.assertFalse(_is_identifier_url("https://lodash.com/lodash.js"))

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

    def test_reaches_no_remote_resource(self):
        for name, path in [*_PAGE.items(), ("report.min.js", _REPORT_JS)]:
            text = _read(path)
            self.assertEqual(external_urls(text) + scheme_relative_urls(text), [], name)

    def test_marks_what_a_saved_report_copies(self):
        html = _read(_PAGE["index.html"])
        block = re.search(
            r"<!-- saved-report:begin -->(.*)<!-- saved-report:end -->", html, re.S
        )
        self.assertIsNotNone(block)
        self.assertIn('id="gl-theme-dark"', block.group(1))
        self.assertNotRegex(block.group(1), r"\b(src|href)=")

    def test_report_script_cannot_end_its_block(self):
        text = _read(_REPORT_JS).lower()
        self.assertNotIn("</script", text)
        self.assertNotIn("<!--", text)

    def test_ships_the_bundled_licences(self):
        text = _read(_LICENSES)
        # netlistsvg's dependencies too: the bundle carries their code.
        for package in (
            "clone",
            "elkjs",
            "golden-layout",
            "leaflet",
            "lodash",
            "netlistsvg",
            "onml",
            "sax",
            "three",
        ):
            self.assertIn(f"\n{package} ", text)
        # EPL-1.0 asks where elkjs's source can be obtained; every entry says.
        self.assertEqual(text.count("\nSource: https://"), text.count("=" * 78) // 2)
        self.assertIn("\nSource: https://github.com/OpenKieler/elkjs\n", text)


class Entries(unittest.TestCase):
    # main.js builds the panels as it loads; the globals they read come first.
    def test_import_main_last(self):
        for entry in _ENTRIES:
            imports = re.findall(r"^import '\./([\w.-]+)';$", _read(entry), re.M)
            self.assertEqual(imports[-1], "main.js", entry)


if __name__ == "__main__":
    unittest.main()
