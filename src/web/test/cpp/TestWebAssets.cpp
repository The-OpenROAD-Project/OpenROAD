// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
//
// The viewer used to load JavaScript from CDNs, one of them over plain http
// (issue #11065).  Everything it needs is now bundled from npm and embedded in
// the binary, and these tests are what keeps a reintroduced remote reference
// failing here rather than in someone else's browser.

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "gtest/gtest.h"
#include "web_assets.h"

namespace web {
namespace {

// The report's blobs live outside the served asset table -- they are inlined
// into a saved timing report, never handed out over HTTP -- so the tests below
// have to reach them by name.
const std::pair<const char*, const EmbeddedAsset*> kReportAssets[] = {
    {"kReportVendorCSS", &kReportVendorCSS},
    {"kReportThemeDark", &kReportThemeDark},
    {"kReportThemeLight", &kReportThemeLight},
    {"kReportAppCSS", &kReportAppCSS},
    {"kReportJS", &kReportJS},
};

// The bundles quote plenty of absolute URLs that are names, not fetches: XML
// namespaces, JSON Schema dialects, licence and homepage links in the comments
// the minifier keeps.  Only a scheme-relative or http(s) URL in a position that
// would make the browser fetch it matters, so the scan allows these prefixes
// and flags everything else for a human to look at.
bool isIdentifierUrl(const std::string_view url)
{
  for (const std::string_view allowed : {
           "http://www.w3.org/",
           "http://www.eclipse.org/",
           "http:///org/eclipse/",
           "http://json-schema.org/",
           "https://raw.githubusercontent.com/epoberezkin/ajv/",
           "http://github.com/garycourt/uri-js",
           "http://mths.be/",
           "http://underscorejs.org/",
           "https://lodash.com/",
           "https://js.foundation/",
           "https://feross.org",
           "https://npms.io/",
           "https://leafletjs.com",
           "https://discourse.threejs.org/",
           // The namespace netlistsvg stamps into the SVG it produces.
           "https://github.com/nturley/netlistsvg",
       }) {
    if (url.starts_with(allowed)) {
      return true;
    }
  }
  return false;
}

// Every absolute URL in an asset that is not one of the identifiers above.
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
    if (!isIdentifierUrl(url)) {
      found.emplace_back(url);
    }
  }
  return found;
}

// ─── Tests ──────────────────────────────────────────────────────────────────

// The whole page is these two: the HTML carries its own stylesheets, and the
// script carries the app and every library it uses.
TEST(WebAssets, ServesTheTwoBundles)
{
  EXPECT_NE(findEmbeddedAsset("/index.html"), nullptr);
  EXPECT_NE(findEmbeddedAsset("/app.min.js"), nullptr);
  EXPECT_EQ(findEmbeddedAsset("/main.js"), nullptr);
  EXPECT_EQ(findEmbeddedAsset("/style.css"), nullptr);
}

TEST(WebAssets, StoresThemGzipped)
{
  for (const std::string_view path : {"/index.html", "/app.min.js"}) {
    const EmbeddedAsset* asset = findEmbeddedAsset(path);
    ASSERT_NE(asset, nullptr) << path;
    EXPECT_TRUE(asset->gzipped) << path;
    // The gzip magic number: what the server hands to a browser that asked
    // for gzip has to actually be a gzip stream.
    ASSERT_GE(asset->size, 2u) << path;
    EXPECT_EQ(static_cast<unsigned char>(asset->data[0]), 0x1f) << path;
    EXPECT_EQ(static_cast<unsigned char>(asset->data[1]), 0x8b) << path;
    // Compression that did not compress would mean the pipeline is wrong.
    EXPECT_LT(asset->size, asset->original_size) << path;
  }
}

TEST(WebAssets, InflatesToTheRecordedSize)
{
  for (const std::string_view path : {"/index.html", "/app.min.js"}) {
    const EmbeddedAsset* asset = findEmbeddedAsset(path);
    ASSERT_NE(asset, nullptr) << path;
    const std::string text = assetText(*asset);
    EXPECT_EQ(text.size(), asset->original_size) << path;
  }
}

TEST(WebAssets, TheIndexIsTheBundledPage)
{
  const EmbeddedAsset* asset = findEmbeddedAsset("/index.html");
  ASSERT_NE(asset, nullptr);
  const std::string html = assetText(*asset);
  EXPECT_NE(html.find("app.min.js"), std::string::npos);
  // The two golden-layout themes theme.js looks up by id.
  EXPECT_NE(html.find("gl-theme-dark"), std::string::npos);
  EXPECT_NE(html.find("gl-theme-light"), std::string::npos);
  // The icons the third-party stylesheets reach through url(), inlined.
  EXPECT_NE(html.find("data:image/png;base64,"), std::string::npos);
}

// The point of the whole exercise: nothing the page loads comes off the
// network.
TEST(WebAssets, NoAssetReferencesARemoteResource)
{
  for (const std::string_view path : {"/index.html", "/app.min.js"}) {
    const EmbeddedAsset* asset = findEmbeddedAsset(path);
    ASSERT_NE(asset, nullptr) << path;
    const std::string text = assetText(*asset);
    const std::vector<std::string> urls = externalUrls(text);
    EXPECT_TRUE(urls.empty())
        << path << " reaches out to " << (urls.empty() ? "" : urls.front())
        << " (" << urls.size() << " in total)";
  }
}

// A saved report is the copy that gets emailed around and opened months later,
// with no server and possibly no network.  Its four blobs are embedded
// separately from the served ones, so the scan above never reaches them -- and
// a CDN reintroduced here would be exactly the regression #11065 was filed for.
TEST(WebAssets, NoReportAssetReferencesARemoteResource)
{
  for (const auto& [name, asset] : kReportAssets) {
    const std::vector<std::string> urls = externalUrls(assetText(*asset));
    EXPECT_TRUE(urls.empty())
        << name << " reaches out to " << (urls.empty() ? "" : urls.front())
        << " (" << urls.size() << " in total)";
  }
}

// The binary now distributes those libraries, so it has to carry their
// licences as well.
TEST(WebAssets, ServesTheThirdPartyLicenses)
{
  const EmbeddedAsset* asset = findEmbeddedAsset("/THIRD_PARTY_LICENSES.txt");
  ASSERT_NE(asset, nullptr);
  EXPECT_TRUE(asset->gzipped);
  EXPECT_EQ(std::string_view(asset->content_type), "text/plain; charset=utf-8");
  const std::string text = assetText(*asset);
  for (const char* package :
       {"elkjs", "golden-layout", "leaflet", "netlistsvg", "three"}) {
    EXPECT_NE(text.find(package), std::string::npos) << package;
  }
  EXPECT_NE(text.find("Eclipse Public License"), std::string::npos);
  EXPECT_NE(text.find("BSD"), std::string::npos);
  EXPECT_NE(text.find("MIT"), std::string::npos);
}

// The report's blobs are stored gzipped too; saveReport() inflates them.
TEST(WebAssets, StoresTheReportAssetsGzipped)
{
  for (const auto& [name, asset] : kReportAssets) {
    EXPECT_TRUE(asset->gzipped) << name;
    ASSERT_GE(asset->size, 2u) << name;
    EXPECT_EQ(static_cast<unsigned char>(asset->data[0]), 0x1f) << name;
    EXPECT_EQ(static_cast<unsigned char>(asset->data[1]), 0x8b) << name;
    EXPECT_LT(asset->size, asset->original_size) << name;
    EXPECT_EQ(assetText(*asset).size(), asset->original_size) << name;
  }
}

// A scheme-relative "//cdn.example.com/x.js" is a fetch too, and the scan above
// would not see it.
TEST(WebAssets, TheIndexHasNoSchemeRelativeReference)
{
  const EmbeddedAsset* asset = findEmbeddedAsset("/index.html");
  ASSERT_NE(asset, nullptr);
  const std::string html = assetText(*asset);
  EXPECT_EQ(html.find("src=\"//"), std::string::npos);
  EXPECT_EQ(html.find("href=\"//"), std::string::npos);
}

}  // namespace
}  // namespace web
