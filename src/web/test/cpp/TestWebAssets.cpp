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
#include "remote_urls.h"
#include "web_assets.h"

namespace web {
namespace {

// The report's blob lives outside the served asset table, so the tests below
// reach it by name.
const std::pair<const char*, const EmbeddedAsset*> kReportAssets[] = {
    {"kReportJS", &kReportJS},
};

// What the page would fetch from elsewhere: absolute URLs that are not mere
// identifiers, and scheme-relative ones.
std::vector<std::string> remoteReferences(const std::string_view text)
{
  std::vector<std::string> found = test::externalUrls(text);
  for (std::string& url : test::schemeRelativeUrls(text)) {
    found.push_back(std::move(url));
  }
  return found;
}

// ─── The scanner itself ─────────────────────────────────────────────────────

// A scanner that finds nothing would pass every test below, so it gets its own.
TEST(RemoteUrls, AllowsAnIdentifierOnlyUpToItsBoundary)
{
  EXPECT_TRUE(test::isIdentifierUrl("https://feross.org"));
  EXPECT_TRUE(test::isIdentifierUrl("https://feross.org/buffer"));
  EXPECT_TRUE(test::isIdentifierUrl("http://www.w3.org/2000/svg"));
  EXPECT_FALSE(test::isIdentifierUrl("https://feross.org.example.com/x.js"));
  EXPECT_FALSE(test::isIdentifierUrl("https://leafletjs.com.example/l.js"));
  EXPECT_FALSE(test::isIdentifierUrl("https://unpkg.com/leaflet"));
}

TEST(RemoteUrls, FindsAbsoluteUrlsInAnyCase)
{
  EXPECT_EQ(test::externalUrls("<script src=\"https://unpkg.com/l.js\">"),
            std::vector<std::string>{"https://unpkg.com/l.js"});
  EXPECT_EQ(test::externalUrls("s.src='HTTPS://CDN.example/x.js'").size(), 1u);
  EXPECT_TRUE(
      test::externalUrls("xmlns=\"http://www.w3.org/2000/svg\"").empty());
}

TEST(RemoteUrls, FindsSchemeRelativeFetches)
{
  for (const char* text : {"<script src=\"//cdn.example/x.js\">",
                           "<link href='//cdn.example/x.css'>",
                           "a{background:url(//cdn.example/i.png)}",
                           "@import \"//cdn.example/x.css\";",
                           "fetch('//cdn.example/x')",
                           "import(\"//cdn.example/m.js\")",
                           "import{a}from\"//cdn.example/m.js\""}) {
    EXPECT_EQ(test::schemeRelativeUrls(text).size(), 1u) << text;
  }
  EXPECT_TRUE(test::schemeRelativeUrls("x=1;// a comment\ny=2").empty());
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
  // A module, as in saved reports, so both run the bundle in strict mode.
  EXPECT_NE(html.find("<script type=\"module\" src=\"app.min.js\">"),
            std::string::npos);
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
    const std::vector<std::string> urls = remoteReferences(assetText(*asset));
    EXPECT_TRUE(urls.empty())
        << path << " reaches out to " << (urls.empty() ? "" : urls.front())
        << " (" << urls.size() << " in total)";
  }
}

// A saved report opens with no server and possibly no network, and its script
// is embedded apart from the served assets, so the scan above misses it.
TEST(WebAssets, NoReportAssetReferencesARemoteResource)
{
  for (const auto& [name, asset] : kReportAssets) {
    const std::vector<std::string> urls = remoteReferences(assetText(*asset));
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
  // What netlistsvg's own browser build compiles in, notices and all.
  for (const char* package : {"lodash", "sax", "buffer", "readable-stream"}) {
    EXPECT_NE(text.find(package), std::string::npos) << package;
  }
  size_t notices = 0;
  for (size_t pos = text.find("Permission is hereby granted");
       pos != std::string::npos;
       pos = text.find("Permission is hereby granted", pos + 1)) {
    ++notices;
  }
  EXPECT_GE(notices, 25u);
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

}  // namespace
}  // namespace web
