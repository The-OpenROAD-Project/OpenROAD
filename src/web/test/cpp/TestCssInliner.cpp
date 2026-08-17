// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
//
// The saved report rewrites the url() references inside the stylesheets it
// inlines, because a relative one would resolve against wherever the file was
// saved (issue #11065).  A reference the scanner misses ships a broken report;
// one it invents refuses to save a report the browser reads fine.  Both edges
// are pinned here, against the vendored icons the viewer really serves.

#include <cstddef>
#include <string>
#include <string_view>

#include "css_inliner.h"
#include "gtest/gtest.h"
#include "utl/Logger.h"

namespace web {
namespace {

constexpr std::string_view kLeafletDir = "/third-party/leaflet/";
constexpr std::string_view kLayers = "images/layers.png";

bool contains(const std::string& haystack, const std::string_view needle)
{
  return haystack.find(needle) != std::string::npos;
}

// ─── findUrlToken ───────────────────────────────────────────────────────────

TEST(CssInliner, UrlTokenIsCaseInsensitive)
{
  // css does not care about the case of the token, so neither may we: a
  // dependency bump that emits URL( must not slip a relative reference through.
  for (const char* css : {"a{background:url(x.png)}",
                          "a{background:URL(x.png)}",
                          "a{background:Url(x.png)}",
                          "a{background:uRl(x.png)}"}) {
    EXPECT_EQ(findUrlToken(css, 0), 13u) << css;
  }
}

TEST(CssInliner, UrlTokenHasToBeAToken)
{
  // Matching the tail of an identifier would refuse a report over a reference
  // that is not one.
  EXPECT_EQ(findUrlToken("a{content:myurl(x.png)}", 0), std::string_view::npos);
  EXPECT_EQ(findUrlToken("a{content:foo-url(x.png)}", 0),
            std::string_view::npos);
  EXPECT_EQ(findUrlToken("a{content:x_url(x.png)}", 0), std::string_view::npos);

  // A token boundary, on the other hand, is anything that cannot continue an
  // identifier -- including the start of the stylesheet.
  EXPECT_EQ(findUrlToken("url(x.png)", 0), 0u);
  EXPECT_EQ(findUrlToken("a{background:url(x.png)}", 0), 13u);
  EXPECT_EQ(findUrlToken("@import url(x.css);", 0), 8u);
}

TEST(CssInliner, UrlTokenStartsAtFrom)
{
  const std::string_view css = "a{b:url(1.png);c:url(2.png)}";
  const size_t first = findUrlToken(css, 0);
  ASSERT_EQ(first, 4u);
  EXPECT_EQ(findUrlToken(css, first + 1), 17u);
  EXPECT_EQ(findUrlToken(css, 18), std::string_view::npos);
}

// ─── resolveAssetPath ───────────────────────────────────────────────────────

TEST(CssInliner, ResolvesAgainstTheServedDirectory)
{
  EXPECT_EQ(resolveAssetPath(kLeafletDir, kLayers),
            "/third-party/leaflet/images/layers.png");
  EXPECT_EQ(resolveAssetPath("/third-party/golden-layout/css/themes/",
                             "../../img/lm_close_white.png"),
            "/third-party/golden-layout/img/lm_close_white.png");
  EXPECT_EQ(resolveAssetPath(kLeafletDir, "./images/./layers.png"),
            "/third-party/leaflet/images/layers.png");
  // A reference that starts at the root replaces the directory.
  EXPECT_EQ(resolveAssetPath(kLeafletDir, "/three/x.js"), "/three/x.js");
}

// ─── inlineStylesheetUrls ───────────────────────────────────────────────────

TEST(CssInliner, InlinesEveryReferenceForm)
{
  utl::Logger logger;
  const std::string inlined = "url(\"data:image/png;base64,";

  for (const char* css : {"a{background:url(images/layers.png)}",
                          "a{background:url( images/layers.png )}",
                          "a{background:url('images/layers.png')}",
                          "a{background:url(\"images/layers.png\")}",
                          "a{background:URL(images/layers.png) no-repeat}",
                          "a{background:url(/third-party/leaflet/"
                          "images/layers.png)}"}) {
    ReportAssets assets(&logger);
    const std::string out = inlineStylesheetUrls(css, kLeafletDir, assets);
    EXPECT_TRUE(contains(out, inlined)) << css << " -> " << out;
    EXPECT_FALSE(assets.missing()) << css;
  }
}

TEST(CssInliner, LeavesAloneWhatIsNotAReference)
{
  utl::Logger logger;
  ReportAssets assets(&logger);

  // leaflet.css really carries the first of these.
  for (const char* css : {"a{behavior:url(#default#VML)}",
                          "a{background:url(\"data:image/png;base64,AA\")}",
                          "a{content:myurl(images/layers.png)}"}) {
    EXPECT_EQ(inlineStylesheetUrls(css, kLeafletDir, assets), css) << css;
  }
  EXPECT_FALSE(assets.missing());
}

TEST(CssInliner, AQuotedReferenceMayHoldAParenthesis)
{
  utl::Logger logger;
  ReportAssets assets(&logger);

  // The token ends at the quote, not at the first ')': ending it early would
  // cut the stylesheet in half.  No such icon is vendored, so the reference
  // resolves to nothing -- which is the miss the report is refused over.
  const std::string out = inlineStylesheetUrls(
      "a{background:url(\"a(b).png\")}!", kLeafletDir, assets);
  EXPECT_EQ(out, "a{background:url(\"\")}!");
  EXPECT_TRUE(assets.missing());
}

TEST(CssInliner, MissingReferenceIsReported)
{
  utl::Logger logger;
  ReportAssets assets(&logger);

  inlineStylesheetUrls("a{background:url(nope.png)}", kLeafletDir, assets);
  EXPECT_TRUE(assets.missing());
}

}  // namespace
}  // namespace web
