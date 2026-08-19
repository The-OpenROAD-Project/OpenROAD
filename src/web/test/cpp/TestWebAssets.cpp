// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
//
// The viewer used to load JavaScript from CDNs, one of them over plain http
// (issue #11065).  These tests are what keeps a reintroduced reference failing
// here rather than in someone else's browser.

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include "gtest/gtest.h"
#include "request_handler.h"
#include "web_assets.h"

namespace web {
namespace {

// ─── Helpers ────────────────────────────────────────────────────────────────

bool contains(const std::string_view haystack, const std::string_view needle)
{
  return haystack.find(needle) != std::string_view::npos;
}

bool isThirdParty(const std::string_view path)
{
  return path.rfind("/third-party/", 0) == 0;
}

// URLs that are identifiers rather than fetches: XML namespaces, and the one
// netlistsvg uses to tag the DOM it produces.
bool isNamespaceUrl(const std::string_view url)
{
  for (const std::string_view allowed :
       {"http://www.w3.org/", "https://github.com/nturley/netlistsvg"}) {
    if (url.rfind(allowed, 0) == 0) {
      return true;
    }
  }
  return false;
}

// Every absolute URL in an asset that is not a namespace identifier.
std::vector<std::string> externalUrls(const std::string_view text)
{
  std::vector<std::string> found;
  for (size_t pos = 0;;) {
    const size_t begin = text.find("http", pos);
    if (begin == std::string_view::npos) {
      break;
    }
    pos = begin + 4;
    if (text.compare(begin, 7, "http://") != 0
        && text.compare(begin, 8, "https://") != 0) {
      continue;
    }
    const size_t end = text.find_first_of("\"'`) >,;\n", begin);
    const std::string_view url
        = text.substr(begin, end == std::string_view::npos ? 60 : end - begin);
    if (!isNamespaceUrl(url)) {
      found.emplace_back(url);
    }
  }
  return found;
}

// Drop HTML comments, which are allowed to talk about <script> tags.
std::string withoutComments(const std::string_view html)
{
  std::string result;
  for (size_t pos = 0;;) {
    const size_t open = html.find("<!--", pos);
    result += html.substr(pos, open - pos);
    if (open == std::string_view::npos) {
      return result;
    }
    const size_t close = html.find("-->", open);
    if (close == std::string_view::npos) {
      return result;
    }
    pos = close + 3;
  }
}

// Count the inline <script> blocks (those with no src attribute) in the HTML.
int countInlineScripts(const std::string_view markup)
{
  const std::string html = withoutComments(markup);
  int count = 0;
  for (size_t pos = 0;;) {
    const size_t open = html.find("<script", pos);
    if (open == std::string_view::npos) {
      break;
    }
    const size_t attributes_end = html.find('>', open);
    if (attributes_end == std::string_view::npos) {
      break;
    }
    if (!contains(html.substr(open, attributes_end - open), "src=")) {
      ++count;
    }
    pos = attributes_end + 1;
  }
  return count;
}

// ─── Tests ──────────────────────────────────────────────────────────────────

// The policy must name no remote origin, nor a wildcard, which would admit one.
// Properties, not exact text: the relaxations are free to change.
TEST(WebAssets, ContentSecurityPolicyIsSameOriginOnly)
{
  const std::string& csp = contentSecurityPolicy();
  EXPECT_TRUE(contains(csp, "default-src 'none'"));
  EXPECT_TRUE(contains(csp, "script-src 'self'"));
  EXPECT_TRUE(contains(csp, "style-src 'self'"));
  EXPECT_TRUE(contains(csp, "connect-src 'self'"));
  EXPECT_TRUE(contains(csp, "base-uri 'none'"));

  EXPECT_FALSE(contains(csp, "http:"));
  EXPECT_FALSE(contains(csp, "https:"));
  EXPECT_FALSE(contains(csp, "*"));
}

// The saved report cannot be served with headers, so its policy travels in a
// <meta>; the point of it there is that the file never reaches the network.
TEST(WebAssets, ReportContentSecurityPolicyForbidsTheNetwork)
{
  const std::string_view csp = kReportContentSecurityPolicy;
  EXPECT_TRUE(contains(csp, "default-src 'none'"));
  EXPECT_TRUE(contains(csp, "connect-src 'none'"));
  EXPECT_FALSE(contains(csp, "http:"));
  EXPECT_FALSE(contains(csp, "https:"));
  EXPECT_FALSE(contains(csp, "*"));
}

// An inline block the generator missed silently stops running, so hold the two
// to the same count -- and keep 'unsafe-inline' out, which would mask it.
TEST(WebAssets, EveryInlineScriptIsCoveredByAHash)
{
  const EmbeddedAsset* index = findEmbeddedAsset("/index.html");
  ASSERT_NE(index, nullptr);

  const std::string_view hashes = inlineScriptHashes();
  int hash_count = 0;
  for (size_t pos = 0;
       (pos = hashes.find("'sha256-", pos)) != std::string_view::npos;
       ++pos) {
    ++hash_count;
  }
  EXPECT_EQ(hash_count, countInlineScripts(index->content()));
  EXPECT_GT(hash_count, 0);

  const std::string& csp = contentSecurityPolicy();
  EXPECT_TRUE(contains(csp, hashes));
  EXPECT_FALSE(contains(csp, "'unsafe-inline' 'sha256-"));
  // style-src still needs 'unsafe-inline', so check the script directive alone.
  const size_t script_src = csp.find("script-src");
  ASSERT_NE(script_src, std::string::npos);
  const size_t directive_end = csp.find(';', script_src);
  EXPECT_FALSE(contains(csp.substr(script_src, directive_end - script_src),
                        "'unsafe-inline'"));
}

TEST(WebAssets, IndexHtmlHasNoExternalUrl)
{
  const EmbeddedAsset* index = findEmbeddedAsset("/index.html");
  ASSERT_NE(index, nullptr);

  // index.html is the only asset that declares what to load, so it is held to
  // the stricter rule: no absolute URL at all.
  const std::string_view html = index->content();
  EXPECT_FALSE(contains(html, "http://"));
  EXPECT_FALSE(contains(html, "https://"));
}

TEST(WebAssets, NoAssetReferencesAnExternalHost)
{
  for (size_t i = 0; i < embeddedAssetCount(); ++i) {
    const EmbeddedAssetEntry& entry = embeddedAssetAt(i);
    // Library code carries URLs in comments and string tables; what matters is
    // that it is served from here, which the asset table proves.
    if (isThirdParty(entry.path)) {
      continue;
    }
    for (const std::string& url : externalUrls(entry.asset.content())) {
      ADD_FAILURE() << entry.path << " references external URL: " << url;
    }
  }
}

TEST(WebAssets, BrowserLibrariesAreEmbedded)
{
  for (const char* path :
       {"/third-party/leaflet/leaflet.js",
        "/third-party/leaflet/leaflet.css",
        "/third-party/leaflet/images/layers.png",
        "/third-party/golden-layout/golden-layout.esm.js",
        "/third-party/golden-layout/css/goldenlayout-base.css",
        "/third-party/golden-layout/img/lm_close_white.png",
        "/third-party/three/three.module.min.js",
        "/third-party/elkjs/elk.bundled.js",
        "/third-party/netlistsvg/netlistsvg.bundle.js"}) {
    const EmbeddedAsset* asset = findEmbeddedAsset(path);
    ASSERT_NE(asset, nullptr) << "not embedded: " << path;
    EXPECT_GT(asset->size, 0u) << path;
  }
}

TEST(WebAssets, IndexHtmlLoadsTheBrowserLibraries)
{
  const EmbeddedAsset* index = findEmbeddedAsset("/index.html");
  ASSERT_NE(index, nullptr);
  const std::string_view html = index->content();

  for (const char* reference :
       {"third-party/leaflet/leaflet.js",
        "third-party/leaflet/leaflet.css",
        "third-party/golden-layout/css/goldenlayout-base.css",
        "third-party/elkjs/elk.bundled.js",
        "third-party/netlistsvg/netlistsvg.bundle.js",
        "/third-party/three/three.module.min.js",
        "/third-party/golden-layout/golden-layout.esm.js"}) {
    EXPECT_TRUE(contains(html, reference))
        << "index.html does not load " << reference;
  }
  // three and golden-layout are ES modules, resolved by an import map.
  EXPECT_TRUE(contains(html, "type=\"importmap\""));
  // netlistsvg reads ELK off the global scope, so elk has to come first.
  EXPECT_LT(html.find("elkjs/elk.bundled.js"),
            html.find("netlistsvg/netlistsvg.bundle.js"));
}

// Binary assets take a different path through embed_web_assets.py than text
// ones; check one arrived byte-exact.
TEST(WebAssets, BinaryAssetsAreEmbeddedVerbatim)
{
  const EmbeddedAsset* png
      = findEmbeddedAsset("/third-party/leaflet/images/layers.png");
  ASSERT_NE(png, nullptr);
  EXPECT_STREQ(png->content_type, "image/png");
  ASSERT_GT(png->size, 8u);
  EXPECT_EQ(png->content().substr(0, 8),
            std::string_view("\x89PNG\r\n\x1a\n", 8));
}

// Flatten the request target to a file name and every library asset 404s, with
// the table still looking correct.
TEST(WebAssets, ThirdPartyPathsSurviveTargetParsing)
{
  for (const char* target :
       {"/third-party/leaflet/leaflet.js",
        "/third-party/leaflet/images/layers.png",
        "/third-party/golden-layout/img/lm_close_white.png",
        "/third-party/elkjs/elk.bundled.js?v=2"}) {
    const std::string path = assetPathFromTarget(target);
    EXPECT_NE(findEmbeddedAsset(path), nullptr)
        << target << " resolved to " << path;
  }
}

// The stylesheets reach their icons through relative urls, so the served paths
// have to keep the directory layout the packages ship.
TEST(WebAssets, StylesheetIconsResolveToEmbeddedAssets)
{
  const EmbeddedAsset* leaflet_css
      = findEmbeddedAsset("/third-party/leaflet/leaflet.css");
  ASSERT_NE(leaflet_css, nullptr);
  EXPECT_TRUE(contains(leaflet_css->content(), "url(images/layers.png)"));
  EXPECT_NE(findEmbeddedAsset("/third-party/leaflet/images/layers.png"),
            nullptr);

  const EmbeddedAsset* dark_theme = findEmbeddedAsset(
      "/third-party/golden-layout/css/themes/goldenlayout-dark-theme.css");
  ASSERT_NE(dark_theme, nullptr);
  EXPECT_TRUE(contains(dark_theme->content(), "../../img/lm_close_white.png"));
  EXPECT_NE(
      findEmbeddedAsset("/third-party/golden-layout/img/lm_close_white.png"),
      nullptr);
}

}  // namespace
}  // namespace web
