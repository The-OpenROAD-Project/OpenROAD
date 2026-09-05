// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <unistd.h>

#include <cctype>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "boost/json/serialize.hpp"
#include "css_inliner.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "tile_generator.h"
#include "timing_report.h"
#include "tst/nangate45_fixture.h"
#include "utl/decode.h"
#include "web/web.h"

namespace web {
namespace {

// Does the line start with this keyword, as a keyword?  "importantly" and
// "exports" are identifiers, not module syntax.
bool isKeyword(const std::string_view line, const std::string_view keyword)
{
  if (!line.starts_with(keyword)) {
    return false;
  }
  if (line.size() == keyword.size()) {
    return true;
  }
  const char next = line[keyword.size()];
  return std::isalnum(static_cast<unsigned char>(next)) == 0 && next != '_'
         && next != '$';
}

// ─── Fixture ────────────────────────────────────────────────────────────────

class SaveReportTest : public tst::Nangate45Fixture
{
 protected:
  void SetUp() override
  {
    block_->setDieArea(odb::Rect(0, 0, 20000, 20000));
    // Place several instances to ensure non-empty tiles.
    for (int i = 0; i < 10; ++i) {
      placeInst("BUF_X16",
                ("buf" + std::to_string(i)).c_str(),
                2000 + i * 1500,
                2000 + i * 1500);
    }
  }

  void TearDown() override
  {
    for (const auto& path : output_files_) {
      std::filesystem::remove(path);
    }
  }

  odb::dbInst* placeInst(const char* master_name,
                         const char* inst_name,
                         int x,
                         int y)
  {
    odb::dbMaster* master = lib_->findMaster(master_name);
    EXPECT_NE(master, nullptr) << "Master not found: " << master_name;
    odb::dbInst* inst = odb::dbInst::create(block_, master, inst_name);
    inst->setLocation(x, y);
    inst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
    return inst;
  }

  std::string tempHtml(const std::string& label)
  {
    std::string path
        = std::filesystem::temp_directory_path()
          / ("web_test_" + label + "_" + std::to_string(::getpid()) + ".html");
    output_files_.push_back(path);
    return path;
  }

  static std::string readFile(const std::string& path)
  {
    std::ifstream f(path);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
  }

  // Generate a report (no STA — timing sections will be empty but valid).
  void generateReport(const std::string& path,
                      int max_setup = 0,
                      int max_hold = 0)
  {
    WebServer server(getDb(),
                     /*sta=*/nullptr,
                     getLogger(),
                     /*interp=*/nullptr);
    server.saveReport(path, max_setup, max_hold);
  }

  // Count occurrences of a substring in a string.
  static int countOccurrences(const std::string& haystack,
                              const std::string& needle)
  {
    int count = 0;
    size_t pos = 0;
    while ((pos = haystack.find(needle, pos)) != std::string::npos) {
      ++count;
      pos += needle.size();
    }
    return count;
  }

  // Check if a string contains a substring.
  static bool contains(const std::string& haystack, const std::string& needle)
  {
    return haystack.find(needle) != std::string::npos;
  }

  std::vector<std::string> output_files_;
};

// ─── HTML Structure ─────────────────────────────────────────────────────────

TEST_F(SaveReportTest, GeneratesFile)
{
  const std::string path = tempHtml("generates");
  generateReport(path);
  ASSERT_TRUE(std::filesystem::exists(path));
  EXPECT_GT(std::filesystem::file_size(path), 0u);
}

TEST_F(SaveReportTest, ContainsRequiredHTMLElements)
{
  const std::string path = tempHtml("html_elements");
  generateReport(path);
  const std::string html = readFile(path);

  EXPECT_TRUE(contains(html, "<html>"));
  EXPECT_TRUE(contains(html, "<head>"));
  EXPECT_TRUE(contains(html, "<body>"));
  EXPECT_TRUE(contains(html, "</html>"));
  EXPECT_TRUE(contains(html, "id=\"gl-container\""));
  EXPECT_TRUE(contains(html, "id=\"menu-bar\""));
  EXPECT_TRUE(contains(html, "id=\"loading-overlay\""));
  // The stylesheets are inlined, so their file names are gone; what has to
  // survive is the <link> element, which theme.js toggles by id.
  EXPECT_TRUE(
      contains(html, "<link rel=\"stylesheet\" href=\"data:text/css;base64,"));
  EXPECT_TRUE(contains(html, "id=\"gl-theme-dark\""));
  EXPECT_TRUE(contains(html, "id=\"gl-theme-light\""));
}

TEST_F(SaveReportTest, ContainsStaticCache)
{
  const std::string path = tempHtml("static_cache");
  generateReport(path);
  const std::string html = readFile(path);

  EXPECT_TRUE(contains(html, "window.__STATIC_CACHE__"));
}

TEST_F(SaveReportTest, ContainsInlinedJS)
{
  const std::string path = tempHtml("inlined_js");
  generateReport(path);
  const std::string html = readFile(path);

  EXPECT_TRUE(contains(html, "class WebSocketManager"));
  EXPECT_TRUE(contains(html, "fromCache"));
  EXPECT_TRUE(contains(html, "function buildTileRequestFor"));
  EXPECT_TRUE(contains(html, "function createMergedTileLayer"));
  EXPECT_TRUE(contains(html, "function computeGroupCount"));
  EXPECT_TRUE(contains(html, "TimingWidget"));
  EXPECT_TRUE(contains(html, "ChartsWidget"));
}

// The widget sources share one <script type="module">, with their import/export
// statements stripped: one that survives costs every widget in the report.
TEST_F(SaveReportTest, InlinedScriptHasNoModuleSyntaxLeft)
{
  const std::string path = tempHtml("module_syntax");
  generateReport(path);
  const std::string html = readFile(path);

  const std::string opening = "<script type=\"module\">";
  const size_t begin = html.find(opening);
  ASSERT_NE(begin, std::string::npos);
  const size_t end = html.find("</script>", begin);
  ASSERT_NE(end, std::string::npos);
  const std::string module
      = html.substr(begin + opening.size(), end - begin - opening.size());

  // The generator marks every source, so the first marker is the boundary:
  // before it the header's imports, after it code with no module syntax.
  const size_t body = module.find("// ── ");
  ASSERT_NE(body, std::string::npos);

  int header_imports = 0;
  for (size_t at = 0; at < module.size();) {
    const size_t eol = std::min(module.find('\n', at), module.size());
    std::string_view line(module.data() + at, eol - at);
    const bool in_header = at < body;
    at = eol + 1;

    const size_t indent = line.find_first_not_of(" \t");
    if (indent == std::string_view::npos) {
      continue;
    }
    line.remove_prefix(indent);
    if (!isKeyword(line, "import") && !isKeyword(line, "export")) {
      continue;
    }
    if (in_header && isKeyword(line, "import")) {
      ++header_imports;
      continue;
    }
    ADD_FAILURE() << "leftover module syntax: " << line;
  }
  // GoldenLayout and THREE, however they are resolved.
  EXPECT_EQ(header_imports, 2);
}

// The report opens from file:// with nothing behind it, so nothing in it may
// point at a remote host (issue #11065).
TEST_F(SaveReportTest, IsSelfContained)
{
  const std::string path = tempHtml("self_contained");
  generateReport(path);
  const std::string html = readFile(path);

  // No attribute, url() or specifier may name a remote origin.  Bare "http://"
  // is left alone: the widgets carry XML namespaces, which are identifiers.
  for (const char* fetch : {"src=\"http",
                            "src='http",
                            "href=\"http",
                            "href='http",
                            "url(http",
                            "url(\"http",
                            "url('http",
                            "from 'http",
                            "from \"http"}) {
    EXPECT_FALSE(contains(html, fetch)) << fetch;
  }
  // leaflet as a classic script, three and golden-layout as ES modules the
  // import map redirects to their inlined copies.
  EXPECT_TRUE(
      contains(html, "<script src=\"data:application/javascript;base64,"));
  EXPECT_TRUE(contains(html, "type=\"importmap\""));
  EXPECT_TRUE(
      contains(html, "\"three\": \"data:application/javascript;base64,"));
  EXPECT_TRUE(contains(
      html, "\"golden-layout\": \"data:application/javascript;base64,"));
  EXPECT_TRUE(contains(html, "type=\"module\""));
  EXPECT_TRUE(contains(html, "from 'golden-layout'"));
  EXPECT_TRUE(contains(html, "from 'three'"));
}

// A relative icon url resolves against the saved file's directory, so every
// stylesheet in the report -- the <style> block included -- has to be
// rewritten.
TEST_F(SaveReportTest, StylesheetIconsAreInlined)
{
  const std::string path = tempHtml("css_icons");
  generateReport(path);
  const std::string html = readFile(path);

  const auto expectNoRelativeUrl = [](const std::string& css,
                                      const std::string& what) {
    // Through the production scanner, so the rule this asserts is the one
    // the report was written with.
    for (size_t open = 0;
         (open = findUrlToken(css, open)) != std::string::npos;) {
      const size_t close = css.find(')', open);
      ASSERT_NE(close, std::string::npos) << what;
      const std::string reference = css.substr(open + 4, close - (open + 4));
      EXPECT_TRUE(reference.find("data:") != std::string::npos
                  || reference.find('#') != std::string::npos)
          << what << " has an unresolved url(" << reference << ")";
      open = close;
    }
  };

  // The vendored stylesheets travel as data: URIs.
  const std::string marker = "href=\"data:text/css;base64,";
  int stylesheets = 0;
  for (size_t at = 0; (at = html.find(marker, at)) != std::string::npos;) {
    const size_t start = at + marker.size();
    const size_t end = html.find('"', start);
    ASSERT_NE(end, std::string::npos);
    expectNoRelativeUrl(utl::base64_decode(html.substr(start, end - start)),
                        "a vendored stylesheet");
    ++stylesheets;
    at = end;
  }
  // leaflet, goldenlayout-base and the two themes.
  EXPECT_EQ(stylesheets, 4);

  // The viewer's own style.css is inlined as a <style> block, and goes through
  // the same rewriting.
  const std::string opening = "<style>";
  const size_t begin = html.find(opening);
  ASSERT_NE(begin, std::string::npos);
  const size_t end = html.find("</style>", begin);
  ASSERT_NE(end, std::string::npos);
  expectNoRelativeUrl(
      html.substr(begin + opening.size(), end - begin - opening.size()),
      "the inlined style.css");
}

TEST_F(SaveReportTest, CachesValidTechResponse)
{
  const std::string path = tempHtml("tech");
  generateReport(path);
  const std::string html = readFile(path);

  EXPECT_TRUE(contains(html, "\"tech\":"));
  EXPECT_TRUE(contains(html, "\"layers\":"));
  EXPECT_TRUE(contains(html, "\"has_liberty\":"));
  EXPECT_TRUE(contains(html, "\"dbu_per_micron\":"));
}

TEST_F(SaveReportTest, CachesValidBoundsResponse)
{
  const std::string path = tempHtml("bounds");
  generateReport(path);
  const std::string html = readFile(path);

  EXPECT_TRUE(contains(html, "\"bounds\":"));
  EXPECT_TRUE(contains(html, "\"shapes_ready\":true"));
  EXPECT_TRUE(contains(html, "\"pin_max_size\":"));
}

TEST_F(SaveReportTest, CachesTimingReportJSON)
{
  const std::string path = tempHtml("timing");
  generateReport(path);
  const std::string html = readFile(path);

  EXPECT_TRUE(contains(html, "\"timing_report:setup\":"));
  EXPECT_TRUE(contains(html, "\"timing_report:hold\":"));
  // Both should have paths arrays (even if empty without STA).
  EXPECT_GE(countOccurrences(html, "\"paths\":"), 2);
}

TEST_F(SaveReportTest, CachesSlackHistogramJSON)
{
  const std::string path = tempHtml("histogram");
  generateReport(path);
  const std::string html = readFile(path);

  EXPECT_TRUE(contains(html, "\"slack_histogram:setup\":"));
  EXPECT_TRUE(contains(html, "\"slack_histogram:hold\":"));
}

TEST_F(SaveReportTest, CachesChartFiltersJSON)
{
  const std::string path = tempHtml("filters");
  generateReport(path);
  const std::string html = readFile(path);

  EXPECT_TRUE(contains(html, "\"chart_filters\":"));
  EXPECT_TRUE(contains(html, "\"path_groups\":"));
  EXPECT_TRUE(contains(html, "\"clocks\":"));
}

TEST_F(SaveReportTest, CachesHeatmapsEmpty)
{
  const std::string path = tempHtml("heatmaps");
  generateReport(path);
  const std::string html = readFile(path);

  EXPECT_TRUE(contains(html, "\"heatmaps\":"));
  // Should be an empty stub.
  EXPECT_TRUE(contains(html, "\"heatmaps\":[]"));
}

// ─── Tile Cache ─────────────────────────────────────────────────────────────

TEST_F(SaveReportTest, CachesTilesAtCorrectZoom)
{
  const std::string path = tempHtml("tile_zoom");
  generateReport(path);
  const std::string html = readFile(path);

  // Tiles should be at zoom 1 (the hardcoded default).
  EXPECT_TRUE(contains(html, "zoom: 1"));
  // At least some tile with /1/ zoom level should exist.
  EXPECT_TRUE(contains(html, "/1/"));
}

TEST_F(SaveReportTest, CachesTiles)
{
  const std::string path = tempHtml("tiles");
  generateReport(path);
  const std::string html = readFile(path);

  // At least some tiles should be cached.
  EXPECT_TRUE(contains(html, "tiles: {"));
  // At least one tile key with zoom 1 should exist.
  EXPECT_TRUE(contains(html, "/1/")) << "No tiles at zoom 1 found";
}

TEST_F(SaveReportTest, TilesAreValidBase64)
{
  const std::string path = tempHtml("tile_base64");
  generateReport(path);
  const std::string html = readFile(path);

  // Find any tile value: "layer/1/x/y":"base64..."
  const std::string marker = "/1/";
  auto pos = html.find(marker);
  ASSERT_NE(pos, std::string::npos) << "No tile entry found";
  // Skip to the value after the key.
  pos = html.find("\":\"", pos);
  ASSERT_NE(pos, std::string::npos);
  pos += 3;  // skip ":"
  // Check the base64 prefix: iVBOR is the b64 encoding of \x89PNG\r\n.
  EXPECT_EQ(html.substr(pos, 4), "iVBO") << "Tile doesn't look like base64 PNG";
}

// ─── Overlays ───────────────────────────────────────────────────────────────

TEST_F(SaveReportTest, OverlayArraysPresent)
{
  const std::string path = tempHtml("overlays");
  generateReport(path);
  const std::string html = readFile(path);

  EXPECT_TRUE(contains(html, "setup: ["));
  EXPECT_TRUE(contains(html, "hold: ["));
}

// ─── Shared Serialization ───────────────────────────────────────────────────

TEST_F(SaveReportTest, SerializeTimingPathsRoundTrip)
{
  TimingPathSummary p;
  p.start_clk = "clk1";
  p.end_clk = "clk2";
  p.slack = -0.5f;
  p.arrival = 1.0f;
  p.required = 1.5f;
  p.skew = 0.1f;
  p.path_delay = 0.9f;
  p.logic_depth = 3;
  p.fanout = 5;
  p.start_pin = "a/Z";
  p.end_pin = "b/D";

  TimingNode node;
  node.pin_name = "a/Z";
  node.fanout = 2;
  node.is_rising = true;
  node.is_clock = false;
  node.time = 0.5f;
  node.delay = 0.1f;
  node.slew = 0.02f;
  node.load = 0.01f;
  p.data_nodes.push_back(std::move(node));

  std::vector<TimingPathSummary> paths = {std::move(p)};
  const std::string json = boost::json::serialize(serializeTimingPaths(paths));

  EXPECT_TRUE(contains(json, "\"clk1\""));
  EXPECT_TRUE(contains(json, "\"clk2\""));
  EXPECT_TRUE(contains(json, "\"a/Z\""));
  EXPECT_TRUE(contains(json, "\"paths\":"));
  EXPECT_TRUE(contains(json, "\"data_nodes\":"));
  EXPECT_TRUE(contains(json, "\"capture_nodes\":"));
}

TEST_F(SaveReportTest, SerializeSlackHistogramRoundTrip)
{
  SlackHistogramResult h;
  h.bins.push_back({-1.0f, 0.0f, 10, true});
  h.bins.push_back({0.0f, 1.0f, 20, false});
  h.unconstrained_count = 5;
  h.total_endpoints = 35;
  h.time_unit = "ns";

  const std::string json = boost::json::serialize(serializeSlackHistogram(h));

  EXPECT_TRUE(contains(json, "\"bins\":"));
  EXPECT_TRUE(contains(json, "\"unconstrained_count\":5"));
  EXPECT_TRUE(contains(json, "\"total_endpoints\":35"));
  EXPECT_TRUE(contains(json, "\"time_unit\":\"ns\""));
}

TEST_F(SaveReportTest, SerializeChartFiltersRoundTrip)
{
  ChartFilters f;
  f.path_groups.emplace_back("default");
  f.clocks.emplace_back("clk");

  const std::string json = boost::json::serialize(serializeChartFilters(f));

  EXPECT_TRUE(contains(json, "\"path_groups\":"));
  EXPECT_TRUE(contains(json, "\"default\""));
  EXPECT_TRUE(contains(json, "\"clocks\":"));
  EXPECT_TRUE(contains(json, "\"clk\""));
}

TEST_F(SaveReportTest, SerializeTechResponse)
{
  TileGenerator gen(getDb(), /*sta=*/nullptr, getLogger());
  const std::string json = boost::json::serialize(serializeTechResponse(gen));

  EXPECT_TRUE(contains(json, "\"layers\":"));
  EXPECT_TRUE(contains(json, "\"sites\":"));
  EXPECT_TRUE(contains(json, "\"has_liberty\":"));
  EXPECT_TRUE(contains(json, "\"dbu_per_micron\":"));
}

// Non-routing tech layers (Nangate45's MASTERSLICE poly/active and OVERLAP)
// are grouped into an "Other" category node in the layer hierarchy, mirroring
// the Qt GUI's displayControls grouping.
TEST_F(SaveReportTest, SerializeTechResponseGroupsOtherLayers)
{
  TileGenerator gen(getDb(), /*sta=*/nullptr, getLogger());
  const std::string json = boost::json::serialize(serializeTechResponse(gen));

  EXPECT_TRUE(contains(json, "\"layer_hierarchy\":"));
  EXPECT_TRUE(contains(json, "\"type\":\"category\""));
  EXPECT_TRUE(contains(json, "\"name\":\"Other\""));
}

TEST_F(SaveReportTest, SerializeBoundsShapesReady)
{
  TileGenerator gen(getDb(), /*sta=*/nullptr, getLogger());

  const std::string json_true
      = boost::json::serialize(serializeBoundsResponse(gen, true));
  EXPECT_TRUE(contains(json_true, "\"shapes_ready\":true"));

  const std::string json_false
      = boost::json::serialize(serializeBoundsResponse(gen, false));
  EXPECT_TRUE(contains(json_false, "\"shapes_ready\":false"));
}

// ─── Edge Cases ─────────────────────────────────────────────────────────────

TEST_F(SaveReportTest, ZeroPathsReport)
{
  const std::string path = tempHtml("zero_paths");
  generateReport(path, 0, 0);
  const std::string html = readFile(path);

  ASSERT_TRUE(std::filesystem::exists(path));
  EXPECT_TRUE(contains(html, "\"paths\":[]"));
}

}  // namespace
}  // namespace web
