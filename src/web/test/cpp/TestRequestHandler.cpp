// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <algorithm>
#include <any>
#include <atomic>
#include <cstdint>
#include <exception>
#include <functional>
#include <iterator>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "boost/json/object.hpp"
#include "boost/json/parse.hpp"
#include "boost/json/serialize.hpp"
#include "gtest/gtest.h"
#include "gui/descriptor_registry.h"
#include "gui/gui.h"
#include "gui/heatMap.h"
#include "odb/db.h"
#include "request_handler.h"
#include "tile_generator.h"
#include "tst/nangate45_fixture.h"
#include "utl/Logger.h"
#include "web_viewer_hook.h"

namespace web {
namespace {

struct FakeInspectable
{
  std::string name;
  std::string type;
  odb::Rect bbox;
  gui::Descriptor::Properties properties;
};

class FakeDescriptor : public gui::Descriptor
{
 public:
  std::string getName(const std::any& object) const override
  {
    return std::any_cast<FakeInspectable*>(object)->name;
  }

  std::string getTypeName() const override { return "Fake"; }

  std::string getTypeName(const std::any& object) const override
  {
    return std::any_cast<FakeInspectable*>(object)->type;
  }

  bool getBBox(const std::any& object, odb::Rect& bbox) const override
  {
    bbox = std::any_cast<FakeInspectable*>(object)->bbox;
    return true;
  }

  void visitAllObjects(
      const std::function<void(const gui::Selected&)>&) const override
  {
  }

  Properties getProperties(const std::any& object) const override
  {
    return std::any_cast<FakeInspectable*>(object)->properties;
  }

  gui::Selected makeSelected(const std::any& object) const override
  {
    return gui::Selected(object, this);
  }

  bool lessThan(const std::any& l, const std::any& r) const override
  {
    return std::any_cast<FakeInspectable*>(l)
           < std::any_cast<FakeInspectable*>(r);
  }

  void highlight(const std::any& object, gui::Painter& painter) const override
  {
    painter.drawRect(std::any_cast<FakeInspectable*>(object)->bbox);
  }
};

// Highlights with a flight line instead of a rect — exercises
// ShapeCollector::drawLine (unrouted nets / net-to-sink paths).
class LineFakeDescriptor : public FakeDescriptor
{
 public:
  void highlight(const std::any& object, gui::Painter& painter) const override
  {
    const odb::Rect& b = std::any_cast<FakeInspectable*>(object)->bbox;
    painter.drawLine(b.ll(), b.ur());
  }
};

// Minimal odb descriptors so DescriptorRegistry::makeSelected() works on
// real dbInst/dbNet objects inside the unit tests — the production set
// lives in the full gui library, which the web test does not link.
class TestInstDescriptor : public gui::Descriptor
{
 public:
  std::string getName(const std::any& object) const override
  {
    return std::any_cast<odb::dbInst*>(object)->getName();
  }
  std::string getTypeName() const override { return "Inst"; }
  bool getBBox(const std::any& object, odb::Rect& bbox) const override
  {
    bbox = std::any_cast<odb::dbInst*>(object)->getBBox()->getBox();
    return true;
  }
  bool isInst(const std::any&) const override { return true; }
  void visitAllObjects(
      const std::function<void(const gui::Selected&)>&) const override
  {
  }
  Properties getProperties(const std::any&) const override { return {}; }
  gui::Selected makeSelected(const std::any& object) const override
  {
    return gui::Selected(std::any_cast<odb::dbInst*>(object), this);
  }
  bool lessThan(const std::any& l, const std::any& r) const override
  {
    return std::any_cast<odb::dbInst*>(l)->getId()
           < std::any_cast<odb::dbInst*>(r)->getId();
  }
  void highlight(const std::any& object, gui::Painter& painter) const override
  {
    painter.drawRect(std::any_cast<odb::dbInst*>(object)->getBBox()->getBox());
  }
};

class TestNetDescriptor : public gui::Descriptor
{
 public:
  std::string getName(const std::any& object) const override
  {
    return std::any_cast<odb::dbNet*>(object)->getName();
  }
  std::string getTypeName() const override { return "Net"; }
  bool getBBox(const std::any&, odb::Rect&) const override { return false; }
  bool isNet(const std::any&) const override { return true; }
  void visitAllObjects(
      const std::function<void(const gui::Selected&)>&) const override
  {
  }
  Properties getProperties(const std::any&) const override { return {}; }
  gui::Selected makeSelected(const std::any& object) const override
  {
    return gui::Selected(std::any_cast<odb::dbNet*>(object), this);
  }
  bool lessThan(const std::any& l, const std::any& r) const override
  {
    return std::any_cast<odb::dbNet*>(l)->getId()
           < std::any_cast<odb::dbNet*>(r)->getId();
  }
  void highlight(const std::any&, gui::Painter&) const override {}
};

// Minimal dbNet descriptor so tests can build a gui::Selected whose
// isNet() is true; highlight() stands in for the wire-shape drawing of
// the real NetDescriptor (a big rect the flywire mode must suppress).
class FakeNetDescriptor : public gui::Descriptor
{
 public:
  static constexpr int kWireRectSize = 90000;

  std::string getName(const std::any& object) const override
  {
    return std::any_cast<odb::dbNet*>(object)->getName();
  }

  std::string getTypeName() const override { return "Net"; }

  std::string getTypeName(const std::any&) const override { return "Net"; }

  bool getBBox(const std::any&, odb::Rect& bbox) const override
  {
    bbox = odb::Rect(0, 0, kWireRectSize, kWireRectSize);
    return true;
  }

  bool isNet(const std::any&) const override { return true; }

  void visitAllObjects(
      const std::function<void(const gui::Selected&)>&) const override
  {
  }

  Properties getProperties(const std::any&) const override { return {}; }

  gui::Selected makeSelected(const std::any& object) const override
  {
    return gui::Selected(object, this);
  }

  bool lessThan(const std::any& l, const std::any& r) const override
  {
    return std::any_cast<odb::dbNet*>(l) < std::any_cast<odb::dbNet*>(r);
  }

  void highlight(const std::any&, gui::Painter& painter) const override
  {
    painter.drawRect(odb::Rect(0, 0, kWireRectSize, kWireRectSize));
  }
};

// Minimal dbInst descriptor reporting the instance bbox in its OWN block's
// coordinates, which is what every real gui::Descriptor does.  Registering it
// lets a select request reach writeInspectPayload without a dbSta (the real
// DbInstDescriptor dereferences one in getProperties).
class LocalBBoxInstDescriptor : public gui::Descriptor
{
 public:
  std::string getName(const std::any& object) const override
  {
    return std::any_cast<odb::dbInst*>(object)->getName();
  }

  std::string getTypeName() const override { return "Inst"; }

  std::string getTypeName(const std::any&) const override { return "Inst"; }

  bool getBBox(const std::any& object, odb::Rect& bbox) const override
  {
    bbox = std::any_cast<odb::dbInst*>(object)->getBBox()->getBox();
    return true;
  }

  bool isInst(const std::any&) const override { return true; }

  void visitAllObjects(
      const std::function<void(const gui::Selected&)>&) const override
  {
  }

  Properties getProperties(const std::any&) const override { return {}; }

  gui::Selected makeSelected(const std::any& object) const override
  {
    return gui::Selected(object, this);
  }

  bool lessThan(const std::any& l, const std::any& r) const override
  {
    return std::any_cast<odb::dbInst*>(l) < std::any_cast<odb::dbInst*>(r);
  }

  void highlight(const std::any& object, gui::Painter& painter) const override
  {
    painter.drawRect(std::any_cast<odb::dbInst*>(object)->getBBox()->getBox());
  }
};

class LazyMetadataHeatMap : public gui::HeatMapDataSource
{
 public:
  explicit LazyMetadataHeatMap(utl::Logger* logger, int* populate_calls)
      : gui::HeatMapDataSource(logger,
                               "Lazy Metadata Heat Map",
                               "LazyMeta",
                               "LazyMeta"),
        populate_calls_(populate_calls)
  {
  }

 protected:
  bool populateMap() override
  {
    ++(*populate_calls_);
    return false;
  }

  void combineMapData(bool, double&, double, double, double, double) override {}

 private:
  int* populate_calls_;
};

// Helper to extract payload as string.
std::string payloadStr(const WebSocketResponse& resp)
{
  return std::string(resp.payload.begin(), resp.payload.end());
}

// Helper to parse a JSON literal into a boost::json::object for tests.
boost::json::object parseObj(std::string_view json)
{
  return boost::json::parse(json).as_object();
}

//------------------------------------------------------------------------------
// jsonOr<T> template tests (optional-field accessor)
//------------------------------------------------------------------------------

TEST(JsonOrTest, MissingKeyReturnsDefault)
{
  auto obj = parseObj(R"({"a":1})");
  EXPECT_EQ(jsonOr<int>(obj, "missing", 42), 42);
  EXPECT_EQ(jsonOr<std::string>(obj, "missing", "default"), "default");
  EXPECT_DOUBLE_EQ(jsonOr<double>(obj, "missing", 3.14), 3.14);
  EXPECT_TRUE(jsonOr<bool>(obj, "missing", true));
}

TEST(JsonOrTest, PresentKeyReturnsValue)
{
  auto obj = parseObj(R"({"i":7,"d":2.5,"s":"hi","b":true})");
  EXPECT_EQ(jsonOr<int>(obj, "i", 0), 7);
  EXPECT_DOUBLE_EQ(jsonOr<double>(obj, "d", 0.0), 2.5);
  EXPECT_EQ(jsonOr<std::string>(obj, "s", ""), "hi");
  EXPECT_TRUE(jsonOr<bool>(obj, "b", false));
}

// jsonOr is intentionally strict on type: a present-but-wrongly-typed
// value is a contract violation, not an "use the default" case.
TEST(JsonOrTest, WrongTypePresentValueThrows)
{
  auto obj = parseObj(R"({"i":"not an int"})");
  EXPECT_THROW(jsonOr<int>(obj, "i", 0), std::exception);
}

//------------------------------------------------------------------------------
// TileHandler tests
//------------------------------------------------------------------------------

class TileHandlerTest : public tst::Nangate45Fixture
{
 protected:
  void SetUp() override
  {
    block_->setDieArea(odb::Rect(0, 0, 100000, 100000));
    block_->setCoreArea(odb::Rect(0, 0, 100000, 100000));
    gen_ = std::make_shared<TileGenerator>(
        getDb(), /*sta=*/nullptr, getLogger());
    handler_ = std::make_unique<TileHandler>(gen_);
  }

  // Driver (buf1.Z) and sink (buf2.A) placed buffers on a new net.
  odb::dbNet* makeConnectedNet(const char* net_name)
  {
    odb::dbMaster* master = lib_->findMaster("BUF_X16");
    if (!master) {
      return nullptr;
    }
    const std::string base = net_name;
    odb::dbInst* buf1
        = odb::dbInst::create(block_, master, (base + "_drv").c_str());
    buf1->setLocation(10000, 10000);
    buf1->setPlacementStatus(odb::dbPlacementStatus::PLACED);
    odb::dbInst* buf2
        = odb::dbInst::create(block_, master, (base + "_snk").c_str());
    buf2->setLocation(60000, 60000);
    buf2->setPlacementStatus(odb::dbPlacementStatus::PLACED);
    odb::dbNet* net = odb::dbNet::create(block_, net_name);
    buf1->findITerm("Z")->connect(net);
    buf2->findITerm("A")->connect(net);
    return net;
  }

  // Prime the highlight state the way an inspect would, so an overlay request
  // can exercise a "Flywires only" flip against it.
  void primeInspected(const gui::Selected& sel)
  {
    std::lock_guard<std::mutex> lock(state_.selection_mutex);
    state_.current_inspected = sel;
    state_.highlight_source = SessionState::HighlightSource::kInspected;
  }

  // An overlay-tile request with the "Flywires only" toggle in a given
  // position.
  static WebSocketRequest overlayRequest(uint32_t id, bool flywires_only)
  {
    WebSocketRequest req;
    req.id = id;
    req.type = WebSocketRequest::kOverlayTile;
    req.json = parseObj(flywires_only
                            ? R"({"z":0,"x":0,"y":0,"flywires_only":true})"
                            : R"({"z":0,"x":0,"y":0,"flywires_only":false})");
    return req;
  }

  std::shared_ptr<TileGenerator> gen_;
  std::unique_ptr<TileHandler> handler_;
  SessionState state_;
};

TEST_F(TileHandlerTest, BoundsReturnsJson)
{
  WebSocketRequest req;
  req.id = 42;
  req.type = WebSocketRequest::kBounds;

  auto resp = handler_->handleTile(req, state_);
  EXPECT_EQ(resp.id, 42u);
  EXPECT_EQ(resp.type, WebSocketResponse::kJson);

  std::string json = payloadStr(resp);
  EXPECT_NE(json.find("\"bounds\""), std::string::npos);
}

TEST_F(TileHandlerTest, TechReturnsJson)
{
  WebSocketRequest req;
  req.id = 7;
  req.type = WebSocketRequest::kTech;

  auto resp = handler_->handleTile(req, state_);
  EXPECT_EQ(resp.id, 7u);
  EXPECT_EQ(resp.type, WebSocketResponse::kJson);

  std::string json = payloadStr(resp);
  EXPECT_NE(json.find("\"layers\""), std::string::npos);
  EXPECT_NE(json.find("\"metal1\""), std::string::npos);
  EXPECT_NE(json.find("\"sites\""), std::string::npos);
  EXPECT_NE(json.find("\"has_liberty\""), std::string::npos);
}

TEST_F(TileHandlerTest, TechIncludesHighlightPalette)
{
  WebSocketRequest req;
  req.id = 8;
  req.type = WebSocketRequest::kTech;

  auto resp = handler_->handleTile(req, state_);
  auto root = parseObj(payloadStr(resp));
  ASSERT_TRUE(root.if_contains("highlight_colors"));
  const auto& colors = root.at("highlight_colors").as_array();
  ASSERT_EQ(colors.size(), static_cast<size_t>(gui::kNumHighlightSet));
  // Group 0 is kGreen with the Qt highlight alpha (100).
  const auto& first = colors[0].as_array();
  EXPECT_EQ(first[0].as_int64(), 0);
  EXPECT_EQ(first[1].as_int64(), 255);
  EXPECT_EQ(first[2].as_int64(), 0);
  EXPECT_EQ(first[3].as_int64(), 100);
}

// PNG dimensions live in the IHDR chunk: 8-byte signature, 4-byte length,
// 4-byte type, then width and height as big-endian uint32.  Read directly so
// this test needs no PNG decoder dependency.
static uint32_t pngWidth(const std::vector<unsigned char>& png)
{
  if (png.size() < 24) {
    return 0;
  }
  return (static_cast<uint32_t>(png[16]) << 24)
         | (static_cast<uint32_t>(png[17]) << 16)
         | (static_cast<uint32_t>(png[18]) << 8)
         | static_cast<uint32_t>(png[19]);
}

// The device pixel ratio comes from the client — only the browser can know the
// display's ratio — and the server honours it rather than snapping it to a
// fixed ladder.  A snapped ratio renders a tile of the wrong size that the
// browser then rescales, which is the moiré beat the tile pipeline exists to
// avoid, and 1.75 and 2.5 are ordinary Windows scale factors.
//
// The client mirrors this in quantizeDpr() in tile-request.js and sizes its
// merged canvas from its own copy, so a divergence here resamples every tile.
// The viewer's options are query parameters, so the asset lookup must never
// see them.  Without this the whole page 404s with "Resource not found." the
// moment any option is used -- which is how ?mergetiles=0 shipped unusable.
TEST(AssetPathFromTarget, StripsTheQueryString)
{
  EXPECT_EQ(assetPathFromTarget("/?mergetiles=0"), "/index.html");
  EXPECT_EQ(assetPathFromTarget("/?tilebudget=256&mergegroups=8"),
            "/index.html");
  EXPECT_EQ(assetPathFromTarget("/main.js?v=2"), "/main.js");
}

TEST(AssetPathFromTarget, StripsAFragment)
{
  EXPECT_EQ(assetPathFromTarget("/#anchor"), "/index.html");
  EXPECT_EQ(assetPathFromTarget("/main.js#top"), "/main.js");
  // Query before fragment, and a '#' inside the query is still a fragment.
  EXPECT_EQ(assetPathFromTarget("/main.js?a=1#top"), "/main.js");
}

TEST(AssetPathFromTarget, MapsRootOntoTheIndexDocument)
{
  EXPECT_EQ(assetPathFromTarget("/"), "/index.html");
  EXPECT_EQ(assetPathFromTarget(""), "/index.html");
}

TEST(AssetPathFromTarget, LeavesAnOrdinaryPathAlone)
{
  EXPECT_EQ(assetPathFromTarget("/style.css"), "/style.css");
  EXPECT_EQ(assetPathFromTarget("/tile-merge.js"), "/tile-merge.js");
}

TEST_F(TileHandlerTest, HonoursTheClientReportedDpr)
{
  struct Case
  {
    double requested;
    uint32_t expected_px;
  };
  // 256 * dpr, rounded — matching renderTileBuffer.
  const Case cases[] = {
      {1.0, 256},
      {1.25, 320},
      {1.75, 448},  // was snapped to 1.5 (384 px) by the old ladder
      {2.0, 512},
      {2.5, 640},  // was snapped to 2.0 (512 px)
  };
  for (const Case& c : cases) {
    WebSocketRequest req;
    req.id = 1;
    req.type = WebSocketRequest::kTile;
    req.json = parseObj(
        R"({"layer":"_instances","z":0,"x":0,"y":0,"visible_layers":[],"dpr":)"
        + std::to_string(c.requested) + "}");
    auto resp = handler_->handleTile(req, state_);
    ASSERT_EQ(resp.type, WebSocketResponse::kPng) << "dpr " << c.requested;
    EXPECT_EQ(pngWidth(resp.payload), c.expected_px)
        << "dpr " << c.requested << " must render at 256*dpr";
  }
}

TEST_F(TileHandlerTest, RoundsDprToBoundTheCacheKeySpace)
{
  // dpr is part of the tile-cache key, so an unrounded ratio would fork its own
  // set of cached tiles for every trailing digit.  Two decimals is the bound.
  WebSocketRequest req;
  req.id = 1;
  req.type = WebSocketRequest::kTile;
  req.json = parseObj(
      R"({"layer":"_instances","z":0,"x":0,"y":0,"visible_layers":[],)"
      R"("dpr":1.3333333})");
  auto resp = handler_->handleTile(req, state_);
  ASSERT_EQ(resp.type, WebSocketResponse::kPng);
  // 1.33, not 1.3333333: round(256 * 1.33) = 340.
  EXPECT_EQ(pngWidth(resp.payload), 340u);
}

TEST_F(TileHandlerTest, ClampsDprIntoRange)
{
  for (const auto& [requested, expected] :
       std::vector<std::pair<std::string, uint32_t>>{
           {"0.5", 256}, {"-1", 256}, {"8", 768}}) {
    WebSocketRequest req;
    req.id = 1;
    req.type = WebSocketRequest::kTile;
    req.json = parseObj(
        R"({"layer":"_instances","z":0,"x":0,"y":0,"visible_layers":[],"dpr":)"
        + requested + "}");
    auto resp = handler_->handleTile(req, state_);
    ASSERT_EQ(resp.type, WebSocketResponse::kPng) << "dpr " << requested;
    EXPECT_EQ(pngWidth(resp.payload), expected) << "dpr " << requested;
  }
}

// The client names the exact device-pixel square the tile will be displayed in,
// because neither end can derive it: a tile's CSS box is a whole number of
// device pixels only when tileSize*dpr is, and at a 1.6667 display scale
// 256*dpr is 426.67 -- a size no image can have.  A tile that is not the size
// of its box is resampled by the browser, which fades its edges into its
// neighbours: the tile seams this replaced.
TEST_F(TileHandlerTest, HonoursTheClientRequestedPixelCount)
{
  // The sizes a 240 CSS px tile works out to across real display ratios.
  for (const uint32_t px : {240u, 300u, 320u, 360u, 400u, 420u, 480u, 720u}) {
    WebSocketRequest req;
    req.id = 1;
    req.type = WebSocketRequest::kTile;
    req.json = parseObj(
        R"({"layer":"_instances","z":0,"x":0,"y":0,"visible_layers":[],)"
        R"("dpr":1.67,"tile_px":)"
        + std::to_string(px) + "}");
    auto resp = handler_->handleTile(req, state_);
    ASSERT_EQ(resp.type, WebSocketResponse::kPng) << px;
    EXPECT_EQ(pngWidth(resp.payload), px)
        << "asked for " << px << " px and must not get 256*dpr";
  }
}

TEST_F(TileHandlerTest, PixelCountOverridesWhateverDprWouldHaveDerived)
{
  // The two are independent inputs: the count sizes the tile, the ratio scales
  // what is authored in CSS px (fonts, stroke widths, the sub-resolution cull).
  for (const char* dpr : {"1", "1.25", "1.67", "3"}) {
    WebSocketRequest req;
    req.id = 1;
    req.type = WebSocketRequest::kTile;
    req.json = parseObj(
        R"({"layer":"_instances","z":0,"x":0,"y":0,"visible_layers":[],"dpr":)"
        + std::string(dpr) + R"(,"tile_px":400})");
    auto resp = handler_->handleTile(req, state_);
    ASSERT_EQ(resp.type, WebSocketResponse::kPng) << dpr;
    EXPECT_EQ(pngWidth(resp.payload), 400u) << "dpr " << dpr;
  }
}

TEST_F(TileHandlerTest, ClampsThePixelCountIntoRange)
{
  // A render allocates (tile_px * supersample)^2 * 4 bytes, so a malformed or
  // hostile count must not be taken at face value.  0 and negatives mean "not
  // specified" and fall back to 256*dpr.
  const std::vector<std::pair<std::string, uint32_t>> cases = {
      {"0", 256},        // unspecified -> 256 * dpr(1)
      {"-5", 256},       // nonsense -> same
      {"16", 32},        // below the floor
      {"100000", 2048},  // above the ceiling
  };
  for (const auto& [requested, expected] : cases) {
    WebSocketRequest req;
    req.id = 1;
    req.type = WebSocketRequest::kTile;
    req.json = parseObj(
        R"({"layer":"_instances","z":0,"x":0,"y":0,"visible_layers":[],)"
        R"("dpr":1,"tile_px":)"
        + requested + "}");
    auto resp = handler_->handleTile(req, state_);
    ASSERT_EQ(resp.type, WebSocketResponse::kPng) << requested;
    EXPECT_EQ(pngWidth(resp.payload), expected) << "tile_px " << requested;
  }
}

TEST_F(TileHandlerTest, PixelCountIsPartOfTheTileCacheKey)
{
  // Two clients on different displays ask for different pixel counts of the
  // same tile, and they are different images.  Without this in the key the
  // second one is served the first one's size and the browser rescales it.
  const auto render = [&](const std::string& px) {
    WebSocketRequest req;
    req.id = 1;
    req.type = WebSocketRequest::kTile;
    req.json = parseObj(
        R"({"layer":"_instances","z":0,"x":0,"y":0,"visible_layers":[],)"
        R"("dpr":1.67,"tile_px":)"
        + px + "}");
    auto resp = handler_->handleTile(req, state_);
    EXPECT_EQ(resp.type, WebSocketResponse::kPng);
    return pngWidth(resp.payload);
  };
  EXPECT_EQ(render("400"), 400u);
  EXPECT_EQ(render("480"), 480u);
  // ...and back, which must come from the cache at its own size, not the last
  // one rendered.
  EXPECT_EQ(render("400"), 400u);
}

TEST_F(TileHandlerTest, TileReturnsPng)
{
  WebSocketRequest req;
  req.id = 99;
  req.type = WebSocketRequest::kTile;
  req.json
      = parseObj(R"({"layer":"metal1","z":0,"x":0,"y":0,"visible_layers":[]})");

  auto resp = handler_->handleTile(req, state_);
  EXPECT_EQ(resp.id, 99u);
  EXPECT_EQ(resp.type, WebSocketResponse::kPng);
  EXPECT_FALSE(resp.payload.empty());
  // PNG magic bytes
  EXPECT_GE(resp.payload.size(), 8u);
  EXPECT_EQ(resp.payload[0], 0x89);
  EXPECT_EQ(resp.payload[1], 'P');
  EXPECT_EQ(resp.payload[2], 'N');
  EXPECT_EQ(resp.payload[3], 'G');
}

TEST_F(TileHandlerTest, EmptyTile)
{
  WebSocketRequest req;
  req.id = 1;
  req.type = WebSocketRequest::kTile;
  req.json
      = parseObj(R"({"layer":"metal1","z":0,"x":0,"y":0,"visible_layers":[]})");

  auto resp = handler_->handleTile(req, state_);
  EXPECT_EQ(resp.type, WebSocketResponse::kPng);  // PNG
  EXPECT_FALSE(resp.payload.empty());
}

TEST_F(TileHandlerTest, BaseTileExcludesHighlights)
{
  // Put a highlight rect in the state — base tiles should NOT include it.
  {
    std::lock_guard<std::mutex> lock(state_.selection_mutex);
    state_.highlight_rects.emplace_back(0, 0, 50000, 50000);
  }

  WebSocketRequest req;
  req.id = 2;
  req.type = WebSocketRequest::kTile;
  req.json = parseObj(
      R"({"layer":"_instances","z":0,"x":0,"y":0,"visible_layers":[]})");

  // Should not crash and should return valid PNG (without highlights)
  auto resp = handler_->handleTile(req, state_);
  EXPECT_EQ(resp.type, WebSocketResponse::kPng);
  EXPECT_FALSE(resp.payload.empty());
}

TEST_F(TileHandlerTest, OverlayTileReturnsPng)
{
  WebSocketRequest req;
  req.id = 10;
  req.type = WebSocketRequest::kOverlayTile;
  req.json = parseObj(R"({"z":0,"x":0,"y":0})");

  auto resp = handler_->handleOverlayTile(req, state_);
  EXPECT_EQ(resp.id, 10u);
  EXPECT_EQ(resp.type, WebSocketResponse::kPng);
  EXPECT_FALSE(resp.payload.empty());
  // PNG magic bytes
  EXPECT_GE(resp.payload.size(), 8u);
  EXPECT_EQ(resp.payload[0], 0x89);
  EXPECT_EQ(resp.payload[1], 'P');
  EXPECT_EQ(resp.payload[2], 'N');
  EXPECT_EQ(resp.payload[3], 'G');
}

// The overlay is composited over the layer tiles in the browser, so it has to
// come back at the same pixel count they do; a 256 px overlay stretched over a
// 400 px layer tile is blurry and no longer sits on the shapes it annotates.
TEST_F(TileHandlerTest, OverlayTileHonoursTheRequestedPixelCount)
{
  {
    std::lock_guard<std::mutex> lock(state_.selection_mutex);
    state_.highlight_rects.emplace_back(0, 0, 50000, 50000);
  }
  for (const uint32_t px : {240u, 320u, 400u, 480u}) {
    WebSocketRequest req;
    req.id = 10;
    req.type = WebSocketRequest::kOverlayTile;
    req.json = parseObj(R"({"z":0,"x":0,"y":0,"dpr":1.67,"tile_px":)"
                        + std::to_string(px) + "}");
    auto resp = handler_->handleOverlayTile(req, state_);
    ASSERT_EQ(resp.type, WebSocketResponse::kPng) << px;
    EXPECT_EQ(pngWidth(resp.payload), px) << "asked for " << px << " px";
  }
}

TEST_F(TileHandlerTest, OverlayTileClampsAndFallsBack)
{
  const std::vector<std::pair<std::string, uint32_t>> cases = {
      {R"("dpr":1)", 256},                    // unspecified -> 256 * dpr
      {R"("dpr":2)", 512},                    // ...which follows dpr
      {R"("dpr":1,"tile_px":0)", 256},        // explicit "not specified"
      {R"("dpr":1,"tile_px":100000)", 2048},  // above the ceiling
  };
  for (const auto& [fields, expected] : cases) {
    WebSocketRequest req;
    req.id = 10;
    req.type = WebSocketRequest::kOverlayTile;
    req.json = parseObj(R"({"z":0,"x":0,"y":0,)" + fields + "}");
    auto resp = handler_->handleOverlayTile(req, state_);
    ASSERT_EQ(resp.type, WebSocketResponse::kPng) << fields;
    EXPECT_EQ(pngWidth(resp.payload), expected) << fields;
  }
}

TEST_F(TileHandlerTest, OverlayTileUsesHighlightState)
{
  // Put a highlight rect in the state
  {
    std::lock_guard<std::mutex> lock(state_.selection_mutex);
    state_.highlight_rects.emplace_back(0, 0, 50000, 50000);
  }

  WebSocketRequest req;
  req.id = 11;
  req.type = WebSocketRequest::kOverlayTile;
  req.json = parseObj(R"({"z":0,"x":0,"y":0})");

  // Should not crash and should return valid PNG with highlights
  auto resp = handler_->handleOverlayTile(req, state_);
  EXPECT_EQ(resp.type, WebSocketResponse::kPng);
  EXPECT_FALSE(resp.payload.empty());
}

//------------------------------------------------------------------------------
// "Flywires only" (Misc toggle) — selection flywires on the overlay
//------------------------------------------------------------------------------

TEST_F(TileHandlerTest, FlywiresOnlySuppressesWireShapes)
{
  odb::dbNet* net = makeConnectedNet("sig");
  ASSERT_NE(net, nullptr);

  static FakeNetDescriptor net_descriptor;
  primeInspected(net_descriptor.makeSelected(std::any(net)));

  // Toggle ON via overlay request: highlight re-derived as flywires.
  WebSocketRequest req = overlayRequest(20, /*flywires_only=*/true);
  auto resp = handler_->handleOverlayTile(req, state_);
  EXPECT_EQ(resp.type, WebSocketResponse::kPng);

  {
    std::lock_guard<std::mutex> lock(state_.selection_mutex);
    EXPECT_FALSE(state_.highlight_lines.empty())
        << "flywires_only should produce driver->sink lines";
    for (const auto& r : state_.highlight_rects) {
      EXPECT_LT(r.dx(), FakeNetDescriptor::kWireRectSize)
          << "wire shapes must be suppressed in flywire mode";
    }
  }

  // Toggle back OFF: unrouted net keeps its flywires (GUI fallback) and
  // the descriptor's wire shapes are collected again.
  req = overlayRequest(21, /*flywires_only=*/false);
  resp = handler_->handleOverlayTile(req, state_);
  EXPECT_EQ(resp.type, WebSocketResponse::kPng);
  {
    std::lock_guard<std::mutex> lock(state_.selection_mutex);
    EXPECT_FALSE(state_.highlight_lines.empty())
        << "unrouted net keeps flywires with the toggle off (GUI parity)";
    bool has_wire_rect = false;
    for (const auto& r : state_.highlight_rects) {
      has_wire_rect
          = has_wire_rect || r.dx() == FakeNetDescriptor::kWireRectSize;
    }
    EXPECT_TRUE(has_wire_rect)
        << "descriptor wire shapes should return when the toggle is off";
  }
}

// A net descriptor that draws the driver->sink fan itself, the way the real
// DbNetDescriptor does for an unrouted net (draw_flywires stays true when
// there is no wire and no guides).  FakeNetDescriptor draws only a wire rect,
// so on its own it cannot catch the fan being collected twice.
class FlywireDrawingNetDescriptor : public FakeNetDescriptor
{
 public:
  void highlight(const std::any& object, gui::Painter& painter) const override
  {
    FakeNetDescriptor::highlight(object, painter);
    painter.drawLine(odb::Point(0, 0), odb::Point(1000, 1000));
  }
};

// ShapeCollector captures drawLine, so the descriptor's own fan and the fan
// collectNetFlightLines derives are the same lines.  Taking both drew every
// flywire twice and let a large net exceed the kMaxFlywires budget.
TEST_F(TileHandlerTest, UnroutedNetFlywiresAreNotCollectedTwice)
{
  odb::dbNet* net = makeConnectedNet("dup");
  ASSERT_NE(net, nullptr);
  ASSERT_EQ(net->getWire(), nullptr);
  ASSERT_TRUE(net->getGuides().empty());

  static FlywireDrawingNetDescriptor net_descriptor;
  primeInspected(net_descriptor.makeSelected(std::any(net)));

  // Toggle off: one driver and one sink, so exactly one flywire.
  WebSocketRequest req = overlayRequest(30, /*flywires_only=*/false);
  // A flip is what re-derives the highlight, so start from the other state.
  {
    std::lock_guard<std::mutex> lock(state_.selection_mutex);
    state_.flywires_only = true;
  }
  auto resp = handler_->handleOverlayTile(req, state_);
  EXPECT_EQ(resp.type, WebSocketResponse::kPng);

  std::lock_guard<std::mutex> lock(state_.selection_mutex);
  EXPECT_EQ(state_.highlight_lines.size(), 1u)
      << "one driver x one sink is one flywire, not one per collector";
}

// A net parked in a persistent highlight group is derived the same way the
// selection is, so "Flywires only" has to reach it too.
TEST_F(TileHandlerTest, HighlightGroupsHonourFlywiresOnly)
{
  odb::dbNet* net = makeConnectedNet("grp");
  ASSERT_NE(net, nullptr);

  static FakeNetDescriptor net_descriptor;
  {
    std::lock_guard<std::mutex> lock(state_.selection_mutex);
    state_.highlight_groups[0].insert(
        net_descriptor.makeSelected(std::any(net)));
  }

  WebSocketRequest req = overlayRequest(31, /*flywires_only=*/true);
  auto resp = handler_->handleOverlayTile(req, state_);
  EXPECT_EQ(resp.type, WebSocketResponse::kPng);

  std::lock_guard<std::mutex> lock(state_.selection_mutex);
  EXPECT_FALSE(state_.highlight_group_lines.empty())
      << "the group member's flywires should be drawn in the group colour";
  for (const auto& colored : state_.highlight_group_rects) {
    EXPECT_LT(colored.rect.dx(), FakeNetDescriptor::kWireRectSize)
        << "a group member's wire shapes must be suppressed in flywire mode";
  }
}

// An edit that moves a group member invalidates the rectangles derived from
// its old placement.  The editing client rebuilds its own; every OTHER
// session learns only through the odb callback that raises this flag, since
// the edit broadcast just asks them to redraw -- from this stale cache.
TEST_F(TileHandlerTest, GeometryChangeRebuildsHighlightGroupShapes)
{
  odb::dbMaster* master = db_->findMaster("BUF_X1");
  ASSERT_NE(master, nullptr);
  odb::dbInst* inst = odb::dbInst::create(block_, master, "moved_inst");
  inst->setLocation(1000, 1000);
  inst->setPlacementStatus(odb::dbPlacementStatus::PLACED);

  static LocalBBoxInstDescriptor inst_descriptor;
  {
    std::lock_guard<std::mutex> lock(state_.selection_mutex);
    state_.highlight_groups[0].insert(
        inst_descriptor.makeSelected(std::any(inst)));
  }

  // Build the cache at the original placement.
  WebSocketRequest req = overlayRequest(32, /*flywires_only=*/true);
  ASSERT_EQ(handler_->handleOverlayTile(req, state_).type,
            WebSocketResponse::kPng);
  odb::Rect before;
  {
    std::lock_guard<std::mutex> lock(state_.selection_mutex);
    ASSERT_EQ(state_.highlight_group_rects.size(), 1u);
    before = state_.highlight_group_rects.front().rect;
  }

  // Move it and raise the flag the session's inDbPostMoveInst would.
  inst->setLocation(40000, 40000);
  state_.highlight_geometry_stale = true;

  // Same toggle position, so only the staleness can trigger the rebuild.
  req = overlayRequest(33, /*flywires_only=*/true);
  ASSERT_EQ(handler_->handleOverlayTile(req, state_).type,
            WebSocketResponse::kPng);

  std::lock_guard<std::mutex> lock(state_.selection_mutex);
  ASSERT_EQ(state_.highlight_group_rects.size(), 1u);
  EXPECT_NE(state_.highlight_group_rects.front().rect, before)
      << "the group rectangle must follow the instance to its new placement";
  EXPECT_FALSE(state_.highlight_geometry_stale)
      << "the flag is consumed so the next overlay does not rebuild again";
}

TEST_F(TileHandlerTest, FlywiresToggleDoesNotResurrectClearedHighlights)
{
  odb::dbNet* net = makeConnectedNet("sig2");
  ASSERT_NE(net, nullptr);

  static FakeNetDescriptor net_descriptor;
  {
    std::lock_guard<std::mutex> lock(state_.selection_mutex);
    state_.current_inspected = net_descriptor.makeSelected(std::any(net));
    // Simulate an explicit "clear highlights" that kept the inspected
    // object (e.g. DRC clear): vectors empty, highlights inactive.
  }

  WebSocketRequest req = overlayRequest(23, /*flywires_only=*/true);
  auto resp = handler_->handleOverlayTile(req, state_);
  EXPECT_EQ(resp.type, WebSocketResponse::kPng);

  std::lock_guard<std::mutex> lock(state_.selection_mutex);
  EXPECT_TRUE(state_.highlight_lines.empty())
      << "toggle flip must not resurrect cleared highlights";
  EXPECT_TRUE(state_.highlight_rects.empty());
}

TEST_F(TileHandlerTest, NonNetPayloadWithIsNetDoesNotThrow)
{
  // A descriptor may report isNet()==true while its payload is NOT a
  // plain dbNet* (e.g. DbNetDescriptor::NetWithSink).  The collector
  // must fall back to the generic highlight path instead of throwing
  // std::bad_any_cast.
  class FakeNetWithSinkDescriptor : public FakeNetDescriptor
  {
   public:
    std::string getName(const std::any&) const override { return "nws"; }
    void highlight(const std::any&, gui::Painter& painter) const override
    {
      painter.drawRect(odb::Rect(0, 0, 1000, 1000));
    }
  };
  static FakeNetWithSinkDescriptor descriptor;
  // Payload is an int — any_cast<odb::dbNet*> by value would throw.
  primeInspected(descriptor.makeSelected(std::any(42)));

  WebSocketRequest req = overlayRequest(24, /*flywires_only=*/true);
  auto resp = handler_->handleOverlayTile(req, state_);
  EXPECT_EQ(resp.type, WebSocketResponse::kPng);

  std::lock_guard<std::mutex> lock(state_.selection_mutex);
  EXPECT_TRUE(state_.highlight_lines.empty());
  EXPECT_FALSE(state_.highlight_rects.empty())
      << "non-dbNet payload should use the generic highlight path";
}

TEST_F(TileHandlerTest, FlywiresSkipSupplyNets)
{
  odb::dbMaster* master = lib_->findMaster("BUF_X16");
  ASSERT_NE(master, nullptr);
  odb::dbInst* buf1 = odb::dbInst::create(block_, master, "buf1");
  buf1->setLocation(10000, 10000);
  buf1->setPlacementStatus(odb::dbPlacementStatus::PLACED);
  odb::dbNet* pwr = odb::dbNet::create(block_, "VDD");
  pwr->setSigType(odb::dbSigType::POWER);
  buf1->findITerm("A")->connect(pwr);

  static FakeNetDescriptor net_descriptor;
  primeInspected(net_descriptor.makeSelected(std::any(pwr)));

  WebSocketRequest req = overlayRequest(22, /*flywires_only=*/true);
  auto resp = handler_->handleOverlayTile(req, state_);
  EXPECT_EQ(resp.type, WebSocketResponse::kPng);

  std::lock_guard<std::mutex> lock(state_.selection_mutex);
  EXPECT_TRUE(state_.highlight_lines.empty())
      << "supply nets must not get flywires (GUI parity)";
}

// A "Flywires only" flip must re-derive the highlight from the SAME selection
// it came from.  Preferring current_inspected (which a click sets to the LAST
// object hit) silently dropped every other member of a shift+click
// multi-selection on the first flip, and flipping back did not restore them
// (reported on the PR #10806 review).
TEST_F(TileHandlerTest, FlywiresFlipPreservesMultiSelection)
{
  odb::dbNet* net_a = makeConnectedNet("multi_a");
  odb::dbNet* net_b = makeConnectedNet("multi_b");
  ASSERT_NE(net_a, nullptr);
  ASSERT_NE(net_b, nullptr);

  static FakeNetDescriptor net_descriptor;
  {
    std::lock_guard<std::mutex> lock(state_.selection_mutex);
    // As after two shift+clicks: both nets selected, the second one inspected.
    state_.selection_set.insert(net_descriptor.makeSelected(std::any(net_a)));
    state_.selection_set.insert(net_descriptor.makeSelected(std::any(net_b)));
    state_.selection_itr = state_.selection_set.begin();
    state_.current_inspected = net_descriptor.makeSelected(std::any(net_b));
    state_.highlight_source = SessionState::HighlightSource::kSelectionSet;
  }

  // Each net has one driver and one sink, so one flywire per net.
  WebSocketRequest req = overlayRequest(25, /*flywires_only=*/true);
  ASSERT_EQ(handler_->handleOverlayTile(req, state_).type,
            WebSocketResponse::kPng);
  {
    std::lock_guard<std::mutex> lock(state_.selection_mutex);
    EXPECT_EQ(state_.highlight_lines.size(), 2u)
        << "both selected nets must keep their flywires across the flip";
  }

  // ... and flipping back keeps both, too (the unrouted-net fallback).
  req = overlayRequest(26, /*flywires_only=*/false);
  ASSERT_EQ(handler_->handleOverlayTile(req, state_).type,
            WebSocketResponse::kPng);
  std::lock_guard<std::mutex> lock(state_.selection_mutex);
  EXPECT_EQ(state_.highlight_lines.size(), 2u)
      << "un-toggling must not narrow the selection either";
}

// The other half of the invariant: when the user followed an inspector link out
// of the selection set, handleInspect deliberately narrowed the highlight to
// that one object, so a flip must NOT widen it back to the whole set.
TEST_F(TileHandlerTest, FlywiresFlipAfterLinkFollowKeepsSingleObject)
{
  odb::dbNet* net_a = makeConnectedNet("link_a");
  odb::dbNet* net_b = makeConnectedNet("link_b");
  odb::dbNet* linked = makeConnectedNet("link_target");
  ASSERT_NE(net_a, nullptr);
  ASSERT_NE(net_b, nullptr);
  ASSERT_NE(linked, nullptr);

  static FakeNetDescriptor net_descriptor;
  {
    std::lock_guard<std::mutex> lock(state_.selection_mutex);
    state_.selection_set.insert(net_descriptor.makeSelected(std::any(net_a)));
    state_.selection_set.insert(net_descriptor.makeSelected(std::any(net_b)));
    state_.selection_itr = state_.selection_set.end();
    state_.current_inspected = net_descriptor.makeSelected(std::any(linked));
    state_.highlight_source = SessionState::HighlightSource::kInspected;
  }

  WebSocketRequest req = overlayRequest(27, /*flywires_only=*/true);
  ASSERT_EQ(handler_->handleOverlayTile(req, state_).type,
            WebSocketResponse::kPng);

  std::lock_guard<std::mutex> lock(state_.selection_mutex);
  EXPECT_EQ(state_.highlight_lines.size(), 1u)
      << "the link target's highlight must not grow into the selection set";
}

// "Flywires only" suppresses the routed wire/guide shapes, NOT the special
// (geometric) routing: the GUI draws SWires at the tail of
// DbNetDescriptor::highlight regardless of the mode.
TEST_F(TileHandlerTest, FlywiresOnlyKeepsSpecialWireShapes)
{
  odb::dbNet* net = makeConnectedNet("special_sig");
  ASSERT_NE(net, nullptr);
  net->setSpecial();
  odb::dbTechLayer* metal1 = getDb()->getTech()->findLayer("metal1");
  ASSERT_NE(metal1, nullptr);
  odb::dbSWire* swire = odb::dbSWire::create(net, odb::dbWireType::ROUTED);
  ASSERT_NE(swire, nullptr);
  odb::dbSBox::create(
      swire, metal1, 20000, 20000, 30000, 21000, odb::dbWireShapeType::NONE);

  static FakeNetDescriptor net_descriptor;
  primeInspected(net_descriptor.makeSelected(std::any(net)));

  WebSocketRequest req = overlayRequest(28, /*flywires_only=*/true);
  ASSERT_EQ(handler_->handleOverlayTile(req, state_).type,
            WebSocketResponse::kPng);

  std::lock_guard<std::mutex> lock(state_.selection_mutex);
  EXPECT_TRUE(state_.highlight_lines.empty())
      << "a routed special net gets no flywires (GUI parity)";
  const auto& rects = state_.highlight_rects;
  EXPECT_NE(std::ranges::find(rects, odb::Rect(20000, 20000, 30000, 21000)),
            rects.end())
      << "the SWire shape must survive flywires-only mode";
}

// Same for supply nets, which is the worst case of the bug: their flywires AND
// their ITerm boxes are suppressed by GUI parity, so before the fix a selected
// power net came back with no highlight at all.
TEST_F(TileHandlerTest, FlywiresOnlySupplyNetKeepsShapes)
{
  odb::dbNet* pwr = odb::dbNet::create(block_, "VDD_swire");
  pwr->setSigType(odb::dbSigType::POWER);
  pwr->setSpecial();
  odb::dbTechLayer* metal1 = getDb()->getTech()->findLayer("metal1");
  ASSERT_NE(metal1, nullptr);
  odb::dbSWire* swire = odb::dbSWire::create(pwr, odb::dbWireType::ROUTED);
  ASSERT_NE(swire, nullptr);
  odb::dbSBox::create(
      swire, metal1, 0, 40000, 100000, 41000, odb::dbWireShapeType::NONE);

  static FakeNetDescriptor net_descriptor;
  primeInspected(net_descriptor.makeSelected(std::any(pwr)));

  WebSocketRequest req = overlayRequest(29, /*flywires_only=*/true);
  ASSERT_EQ(handler_->handleOverlayTile(req, state_).type,
            WebSocketResponse::kPng);

  std::lock_guard<std::mutex> lock(state_.selection_mutex);
  EXPECT_TRUE(state_.highlight_lines.empty())
      << "supply nets must not get flywires (GUI parity)";
  EXPECT_FALSE(state_.highlight_rects.empty())
      << "a selected supply net must still be highlighted by its SWires";
}

// Helper: create a placed BUF_X16 to anchor getBounds(), plus a signal net
// carrying one route guide on metal1 across most of the die.  Returns the net.
static odb::dbNet* makeNetWithGuide(odb::dbBlock* block,
                                    odb::dbLib* lib,
                                    odb::dbDatabase* db)
{
  odb::dbMaster* master = lib->findMaster("BUF_X16");
  odb::dbInst* ll = odb::dbInst::create(block, master, "anchor_ll");
  ll->setLocation(0, 0);
  ll->setPlacementStatus(odb::dbPlacementStatus::PLACED);
  odb::dbInst* ur = odb::dbInst::create(block, master, "anchor_ur");
  ur->setLocation(90000, 90000);
  ur->setPlacementStatus(odb::dbPlacementStatus::PLACED);

  odb::dbNet* net = odb::dbNet::create(block, "guided");
  odb::dbTechLayer* metal1 = db->getTech()->findLayer("metal1");
  odb::dbGuide::create(net,
                       metal1,
                       metal1,
                       odb::Rect(10000, 10000, 90000, 90000),
                       /*is_congested=*/false);
  return net;
}

TEST_F(TileHandlerTest, FocusedNetsGuidesTogglesGuides)
{
  odb::dbNet* net = makeNetWithGuide(block_, lib_, getDb());
  {
    std::lock_guard<std::mutex> lock(state_.focus_nets_mutex);
    state_.focus_net_ids.insert(net->getId());  // net is focused
  }
  gen_->eagerInit();

  // Baseline: nothing drawn on the overlay.
  WebSocketRequest empty_req;
  empty_req.id = 30;
  empty_req.type = WebSocketRequest::kOverlayTile;
  empty_req.json = parseObj(R"({"z":0,"x":0,"y":0})");
  SessionState empty_state;
  const auto blank
      = handler_->handleOverlayTile(empty_req, empty_state).payload;

  // Toggle OFF: focused net has no per-net guide selection → no guides.
  WebSocketRequest off_req;
  off_req.id = 31;
  off_req.type = WebSocketRequest::kOverlayTile;
  off_req.json = parseObj(R"({"z":0,"x":0,"y":0,"focused_nets_guides":false})");
  const auto off = handler_->handleOverlayTile(off_req, state_).payload;
  EXPECT_EQ(off, blank)
      << "guides must not draw when the toggle is off and no per-net selection";

  // Toggle ON: the focused net's guides are drawn.
  WebSocketRequest on_req;
  on_req.id = 32;
  on_req.type = WebSocketRequest::kOverlayTile;
  on_req.json = parseObj(R"({"z":0,"x":0,"y":0,"focused_nets_guides":true})");
  const auto on = handler_->handleOverlayTile(on_req, state_).payload;
  EXPECT_NE(on, blank)
      << "focused nets' guides should be drawn when the toggle is on";
}

TEST_F(TileHandlerTest, PerNetGuidesIgnoreGlobalToggle)
{
  odb::dbNet* net = makeNetWithGuide(block_, lib_, getDb());
  {
    std::lock_guard<std::mutex> lock(state_.route_guides_mutex);
    state_.route_guide_net_ids.insert(net->getId());  // explicit per-net
  }
  gen_->eagerInit();

  WebSocketRequest empty_req;
  empty_req.id = 33;
  empty_req.type = WebSocketRequest::kOverlayTile;
  empty_req.json = parseObj(R"({"z":0,"x":0,"y":0})");
  SessionState empty_state;
  const auto blank
      = handler_->handleOverlayTile(empty_req, empty_state).payload;

  // Global toggle off, but the per-net selection still draws its guides.
  WebSocketRequest req;
  req.id = 34;
  req.type = WebSocketRequest::kOverlayTile;
  req.json = parseObj(R"({"z":0,"x":0,"y":0,"focused_nets_guides":false})");
  const auto out = handler_->handleOverlayTile(req, state_).payload;
  EXPECT_NE(out, blank)
      << "per-net route guides must render regardless of the global toggle";
}

TEST_F(TileHandlerTest, HighlightSelectedTogglesSelectionHighlight)
{
  // Anchor bounds with an instance, then put a selection highlight rect.
  odb::dbMaster* master = lib_->findMaster("BUF_X16");
  odb::dbInst* inst = odb::dbInst::create(block_, master, "anchor");
  inst->setLocation(0, 0);
  inst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
  {
    std::lock_guard<std::mutex> lock(state_.selection_mutex);
    state_.highlight_rects.emplace_back(10000, 10000, 90000, 90000);
  }
  gen_->eagerInit();

  WebSocketRequest empty_req;
  empty_req.id = 40;
  empty_req.type = WebSocketRequest::kOverlayTile;
  empty_req.json = parseObj(R"({"z":0,"x":0,"y":0})");
  SessionState empty_state;
  const auto blank
      = handler_->handleOverlayTile(empty_req, empty_state).payload;

  // Toggle ON (default): selection highlight is drawn.
  WebSocketRequest on_req;
  on_req.id = 41;
  on_req.type = WebSocketRequest::kOverlayTile;
  on_req.json = parseObj(R"({"z":0,"x":0,"y":0,"highlight_selected":true})");
  const auto on = handler_->handleOverlayTile(on_req, state_).payload;
  EXPECT_NE(on, blank) << "selection highlight should draw when toggle is on";

  // Toggle OFF: selection highlight suppressed.
  WebSocketRequest off_req;
  off_req.id = 42;
  off_req.type = WebSocketRequest::kOverlayTile;
  off_req.json = parseObj(R"({"z":0,"x":0,"y":0,"highlight_selected":false})");
  const auto off = handler_->handleOverlayTile(off_req, state_).payload;
  EXPECT_EQ(off, blank)
      << "selection highlight must be hidden when the toggle is off";
}

// The two Misc toggles meet here: "Flywires only" turns the selection's
// highlight into driver->sink lines, and "Highlight selected" must hide the
// selection's highlight as a whole.  Gating only the rects and polys would
// leave the flywires on screen — half a highlight for a toggle that promises
// none.
TEST_F(TileHandlerTest, HighlightSelectedAlsoGatesSelectionFlywires)
{
  odb::dbNet* net = makeConnectedNet("gated_sig");
  ASSERT_NE(net, nullptr);

  static FakeNetDescriptor net_descriptor;
  primeInspected(net_descriptor.makeSelected(std::any(net)));

  // Flywires on: the selection is now carried by highlight_lines.
  WebSocketRequest fly_req = overlayRequest(45, /*flywires_only=*/true);
  const auto with_flywires
      = handler_->handleOverlayTile(fly_req, state_).payload;
  {
    std::lock_guard<std::mutex> lock(state_.selection_mutex);
    ASSERT_FALSE(state_.highlight_lines.empty())
        << "precondition: flywires_only must produce driver->sink lines";
  }

  WebSocketRequest empty_req;
  empty_req.id = 46;
  empty_req.type = WebSocketRequest::kOverlayTile;
  empty_req.json = parseObj(R"({"z":0,"x":0,"y":0})");
  SessionState empty_state;
  const auto blank
      = handler_->handleOverlayTile(empty_req, empty_state).payload;
  ASSERT_NE(with_flywires, blank)
      << "precondition: the flywires must actually draw something";

  // "Highlight selected" off: the flywires go with the rest of the highlight.
  WebSocketRequest off_req;
  off_req.id = 47;
  off_req.type = WebSocketRequest::kOverlayTile;
  off_req.json = parseObj(
      R"({"z":0,"x":0,"y":0,"flywires_only":true,"highlight_selected":false})");
  const auto off = handler_->handleOverlayTile(off_req, state_).payload;
  EXPECT_EQ(off, blank)
      << "selection flywires must be hidden when the toggle is off";

  // The selection itself survives, so turning the toggle back on restores it.
  WebSocketRequest on_req = overlayRequest(48, /*flywires_only=*/true);
  const auto back_on = handler_->handleOverlayTile(on_req, state_).payload;
  EXPECT_NE(back_on, blank)
      << "the flywires must come back when the toggle is on again";
}

TEST_F(TileHandlerTest, OverlayTileMalformedFlagReturnsError)
{
  // Overlay requests run on a bare io_context thread, so a malformed flag
  // type must come back as a kError response, not an exception (which
  // would terminate the whole server).
  WebSocketRequest req;
  req.id = 43;
  req.type = WebSocketRequest::kOverlayTile;
  req.json = parseObj(R"({"z":0,"x":0,"y":0,"highlight_selected":1})");

  WebSocketResponse resp;
  EXPECT_NO_THROW(resp = handler_->handleOverlayTile(req, state_));
  EXPECT_EQ(resp.id, 43u);
  EXPECT_EQ(resp.type, WebSocketResponse::kError);
}

// A zoom that is not an integer is the shape a non-finite client-side number
// arrives in: JSON.stringify writes NaN and Infinity as `null`.  Reaching
// as_int64() with one of those threw, and with no try/catch between the
// handler and io_context::run() that killed the whole openroad process --
// observed as the design vanishing and the session dropping after zooming in
// to the limit and pressing F.  Each case must now be an error response.
TEST_F(TileHandlerTest, TileRejectsNonIntegerZoomInsteadOfThrowing)
{
  struct Case
  {
    const char* what;
    const char* json;
  };
  const Case cases[] = {
      {"null zoom (a non-finite number after JSON.stringify)",
       R"({"layer":"metal1","z":null,"x":0,"y":0})"},
      {"fractional zoom", R"({"layer":"metal1","z":1.5,"x":0,"y":0})"},
      {"null x", R"({"layer":"metal1","z":1,"x":null,"y":0})"},
      {"zoom past the tile-grid ceiling",
       R"({"layer":"metal1","z":60,"x":0,"y":0})"},
      {"missing zoom", R"({"layer":"metal1","x":0,"y":0})"},
  };

  uint32_t id = 700;
  for (const Case& c : cases) {
    WebSocketRequest req;
    req.id = id++;
    req.type = WebSocketRequest::kTile;
    req.json = parseObj(c.json);

    WebSocketResponse resp;
    EXPECT_NO_THROW(resp = handler_->handleTile(req, state_)) << c.what;
    EXPECT_EQ(resp.type, WebSocketResponse::kError) << c.what;
    EXPECT_EQ(resp.id, req.id) << c.what;
  }
}

// Leaflet asks for tiles beyond the 2^z grid whenever the design is smaller
// than the viewport or while panning at the border, and the renderers answer
// those with a transparent tile.  A range check here would turn ordinary
// panning into a stream of errors, so off-grid coordinates must still render.
TEST_F(TileHandlerTest, TileStillServesOffGridCoordinates)
{
  gen_->eagerInit();

  for (const char* json : {R"({"layer":"metal1","z":0,"x":-1,"y":0})",
                           R"({"layer":"metal1","z":0,"x":3,"y":7})",
                           R"({"layer":"metal1","z":2,"x":9999,"y":0})"}) {
    WebSocketRequest req;
    req.id = 710;
    req.type = WebSocketRequest::kTile;
    req.json = parseObj(json);

    WebSocketResponse resp;
    EXPECT_NO_THROW(resp = handler_->handleTile(req, state_)) << json;
    EXPECT_EQ(resp.type, WebSocketResponse::kPng)
        << "off-grid tiles are transparent, not errors: " << json;
  }
}

TEST_F(TileHandlerTest, HoverNotGatedByHighlightSelected)
{
  odb::dbMaster* master = lib_->findMaster("BUF_X16");
  odb::dbInst* inst = odb::dbInst::create(block_, master, "anchor");
  inst->setLocation(0, 0);
  inst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
  {
    std::lock_guard<std::mutex> lock(state_.selection_mutex);
    state_.hover_rects.emplace_back(10000, 10000, 90000, 90000);
  }
  gen_->eagerInit();

  WebSocketRequest empty_req;
  empty_req.id = 43;
  empty_req.type = WebSocketRequest::kOverlayTile;
  empty_req.json = parseObj(R"({"z":0,"x":0,"y":0})");
  SessionState empty_state;
  const auto blank
      = handler_->handleOverlayTile(empty_req, empty_state).payload;

  // Hover must still render even with the selection highlight toggled off.
  WebSocketRequest req;
  req.id = 44;
  req.type = WebSocketRequest::kOverlayTile;
  req.json = parseObj(R"({"z":0,"x":0,"y":0,"highlight_selected":false})");
  const auto out = handler_->handleOverlayTile(req, state_).payload;
  EXPECT_NE(out, blank)
      << "hover highlight is independent of the Highlight selected toggle";
}

TEST_F(TileHandlerTest, HeatMapsReturnsMetadata)
{
  gui::registerBuiltinHeatMapSources(/*sta=*/nullptr, getLogger());
  handler_->initializeHeatMaps(state_);

  WebSocketRequest req;
  req.id = 3;
  req.type = WebSocketRequest::kHeatmaps;

  auto resp = handler_->handleHeatMaps(req, state_);
  EXPECT_EQ(resp.type, WebSocketResponse::kJson);
  const std::string json = payloadStr(resp);
  EXPECT_NE(json.find("\"heatmaps\""), std::string::npos);
  EXPECT_NE(json.find("\"Pin\""), std::string::npos);
  EXPECT_NE(json.find("\"Placement\""), std::string::npos);
}

TEST_F(TileHandlerTest, HeatMapSettingsAreSessionLocal)
{
  gui::registerBuiltinHeatMapSources(/*sta=*/nullptr, getLogger());
  SessionState state1;
  SessionState state2;
  handler_->initializeHeatMaps(state1);
  handler_->initializeHeatMaps(state2);

  WebSocketRequest active_req;
  active_req.type = WebSocketRequest::kSetActiveHeatmap;
  active_req.json = parseObj(R"({"name":"Pin"})");

  EXPECT_EQ(handler_->handleSetActiveHeatMap(active_req, state1).type, 0);
  EXPECT_EQ(handler_->handleSetActiveHeatMap(active_req, state2).type, 0);

  WebSocketRequest set_req;
  set_req.id = 4;
  set_req.type = WebSocketRequest::kSetHeatmap;
  set_req.json
      = parseObj(R"({"name":"Pin","option":"DisplayMin","value":12.5})");

  auto set_resp = handler_->handleSetHeatMap(set_req, state1);
  EXPECT_EQ(set_resp.type, WebSocketResponse::kJson);

  WebSocketRequest meta_req;
  meta_req.id = 5;
  meta_req.type = WebSocketRequest::kHeatmaps;

  const std::string json1
      = payloadStr(handler_->handleHeatMaps(meta_req, state1));
  const std::string json2
      = payloadStr(handler_->handleHeatMaps(meta_req, state2));

  EXPECT_NE(json1, json2);
}

TEST_F(TileHandlerTest, HeatMapShowNumbersCanBeUpdated)
{
  gui::registerBuiltinHeatMapSources(/*sta=*/nullptr, getLogger());
  handler_->initializeHeatMaps(state_);

  WebSocketRequest set_req;
  set_req.id = 8;
  set_req.type = WebSocketRequest::kSetHeatmap;
  set_req.json
      = parseObj(R"({"name":"Pin","option":"ShowNumbers","value":true})");

  auto set_resp = handler_->handleSetHeatMap(set_req, state_);
  EXPECT_EQ(set_resp.type, WebSocketResponse::kJson);
  {
    std::lock_guard<std::mutex> lock(state_.heatmap_mutex);
    ASSERT_TRUE(state_.heatmaps.count("Pin"));
    EXPECT_TRUE(state_.heatmaps.at("Pin")->getShowNumbers());
  }
}

// The browser's number input runs every value through parseFloat, so an
// int-typed setting like Alpha can arrive as a JSON double (e.g. user typed
// 150.5).  The handler must round to int rather than rejecting the request.
// Regression test for the click-to-select breakage's heatmap-side cousin.
TEST_F(TileHandlerTest, HeatMapIntSettingAcceptsFractional)
{
  gui::registerBuiltinHeatMapSources(/*sta=*/nullptr, getLogger());
  handler_->initializeHeatMaps(state_);

  WebSocketRequest set_req;
  set_req.id = 9;
  set_req.type = WebSocketRequest::kSetHeatmap;
  set_req.json = parseObj(R"({"name":"Pin","option":"Alpha","value":150.5})");

  auto set_resp = handler_->handleSetHeatMap(set_req, state_);
  EXPECT_EQ(set_resp.type, WebSocketResponse::kJson) << payloadStr(set_resp);
  {
    std::lock_guard<std::mutex> lock(state_.heatmap_mutex);
    ASSERT_TRUE(state_.heatmaps.count("Pin"));
    // 150.5 rounds to 151 (std::round half-away-from-zero).
    EXPECT_EQ(state_.heatmaps.at("Pin")->getColorAlpha(), 151);
  }
}

TEST_F(TileHandlerTest, HeatMapsMetadataIsLazyForInactiveSources)
{
  static int populate_calls = 0;
  populate_calls = 0;

  gui::registerHeatMapSource(
      "Lazy Metadata Heat Map", "LazyMeta", "LazyMeta", [this]() {
        return std::make_shared<LazyMetadataHeatMap>(getLogger(),
                                                     &populate_calls);
      });

  handler_->initializeHeatMaps(state_);

  WebSocketRequest meta_req;
  meta_req.id = 6;
  meta_req.type = WebSocketRequest::kHeatmaps;

  auto meta_resp = handler_->handleHeatMaps(meta_req, state_);
  EXPECT_EQ(meta_resp.type, WebSocketResponse::kJson);
  EXPECT_EQ(populate_calls, 0);

  WebSocketRequest active_req;
  active_req.id = 7;
  active_req.type = WebSocketRequest::kSetActiveHeatmap;
  active_req.json = parseObj(R"({"name":"LazyMeta"})");

  auto active_resp = handler_->handleSetActiveHeatMap(active_req, state_);
  EXPECT_EQ(active_resp.type, WebSocketResponse::kJson);
  EXPECT_EQ(populate_calls, 1);
}

//------------------------------------------------------------------------------
// SelectHandler tests
//------------------------------------------------------------------------------

class SelectHandlerTest : public tst::Nangate45Fixture
{
 protected:
  void SetUp() override
  {
    block_->setDieArea(odb::Rect(0, 0, 100000, 100000));
    block_->setCoreArea(odb::Rect(0, 0, 100000, 100000));
    placeInst("BUF_X16", "buf1", 0, 0);
    fake_current_ = {.name = "current",
                     .type = "FakeCurrent",
                     .bbox = {0, 0, 100, 100},
                     .properties = {}};
    fake_previous_ = {.name = "previous",
                      .type = "FakePrevious",
                      .bbox = {100, 100, 200, 200},
                      .properties = {}};
    gen_ = std::make_shared<TileGenerator>(
        getDb(), /*sta=*/nullptr, getLogger());
    tcl_eval_ = std::make_shared<TclEvaluator>(/*interp=*/nullptr, getLogger());
    handler_ = std::make_unique<SelectHandler>(gen_, tcl_eval_);
    // The registry owns registered descriptors (unique_ptr) — heap-only.
    auto* registry = gui::DescriptorRegistry::instance();
    registry->registerDescriptor<odb::dbInst*>(new TestInstDescriptor);
    registry->registerDescriptor<odb::dbNet*>(new TestNetDescriptor);
  }

  void TearDown() override
  {
    auto* registry = gui::DescriptorRegistry::instance();
    registry->unregisterDescriptor<odb::dbInst*>();
    registry->unregisterDescriptor<odb::dbNet*>();
    tst::Nangate45Fixture::TearDown();
  }

  gui::Selected makeFakeSelected(FakeInspectable* object)
  {
    return gui::Selected(object, &fake_descriptor_);
  }

  odb::dbInst* placeInst(const char* master_name,
                         const char* inst_name,
                         int x,
                         int y)
  {
    odb::dbMaster* master = lib_->findMaster(master_name);
    EXPECT_NE(master, nullptr);
    odb::dbInst* inst = odb::dbInst::create(block_, master, inst_name);
    inst->setLocation(x, y);
    inst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
    return inst;
  }

  std::shared_ptr<TileGenerator> gen_;
  std::shared_ptr<TclEvaluator> tcl_eval_;
  std::unique_ptr<SelectHandler> handler_;
  SessionState state_;
  FakeDescriptor fake_descriptor_;
  FakeInspectable fake_current_;
  FakeInspectable fake_previous_;
};

TEST_F(SelectHandlerTest, SelectAtOriginFindsInstance)
{
  WebSocketRequest req;
  req.id = 10;
  req.type = WebSocketRequest::kSelect;
  req.json
      = parseObj(R"({"dbu_x":1000,"dbu_y":1000,"zoom":0,"visible_layers":[]})");

  auto resp = handler_->handleSelect(req, state_);
  EXPECT_EQ(resp.id, 10u);
  EXPECT_EQ(resp.type, WebSocketResponse::kJson);

  std::string json = payloadStr(resp);
  EXPECT_NE(json.find("\"selected\""), std::string::npos);
}

TEST_F(SelectHandlerTest, SelectAtEmptyAreaReturnsEmptyList)
{
  WebSocketRequest req;
  req.id = 11;
  req.type = WebSocketRequest::kSelect;
  req.json = parseObj(
      R"({"dbu_x":99000,"dbu_y":99000,"zoom":10,"visible_layers":[]})");

  auto resp = handler_->handleSelect(req, state_);
  EXPECT_EQ(resp.type, WebSocketResponse::kJson);

  std::string json = payloadStr(resp);
  EXPECT_NE(json.find("\"selected\":[]"), std::string::npos);
}

// Leaflet's zoomSnap=0 lets the client send fractional zoom values; the
// handler must accept them without erroring out.  Regression test for the
// click-to-select breakage after strict-typing the request fields.
TEST_F(SelectHandlerTest, SelectAcceptsFractionalZoom)
{
  WebSocketRequest req;
  req.id = 12;
  req.type = WebSocketRequest::kSelect;
  req.json = parseObj(
      R"({"dbu_x":1000,"dbu_y":1000,"zoom":1.5,"visible_layers":[]})");

  auto resp = handler_->handleSelect(req, state_);
  EXPECT_EQ(resp.type, WebSocketResponse::kJson) << payloadStr(resp);
}

// A missing required field surfaces as kError (the handler's catch block
// turns the boost::json exception into an error response) rather than UB.
TEST_F(SelectHandlerTest, SelectWithMissingFieldReturnsError)
{
  WebSocketRequest req;
  req.id = 13;
  req.type = WebSocketRequest::kSelect;
  // dbu_x is required but missing.
  req.json = parseObj(R"({"dbu_y":1000,"zoom":1,"visible_layers":[]})");

  auto resp = handler_->handleSelect(req, state_);
  EXPECT_EQ(resp.type, WebSocketResponse::kError);
  EXPECT_NE(payloadStr(resp).find("server error"), std::string::npos);
}

// Ctrl+click parity (Qt selectHighlightConnectedNets): show_connectivity
// expands the selection with the SIGNAL nets on the picked instance's
// ITerms and reports how many were added.
TEST_F(SelectHandlerTest, SelectWithConnectivityAddsSignalNets)
{
  odb::dbInst* inst = block_->findInst("buf1");
  ASSERT_NE(inst, nullptr);
  odb::dbNet* in_net = odb::dbNet::create(block_, "n_in");
  odb::dbNet* out_net = odb::dbNet::create(block_, "n_out");
  ASSERT_NE(inst->findITerm("A"), nullptr);
  ASSERT_NE(inst->findITerm("Z"), nullptr);
  inst->findITerm("A")->connect(in_net);
  inst->findITerm("Z")->connect(out_net);

  WebSocketRequest req;
  req.id = 20;
  req.type = WebSocketRequest::kSelect;
  req.json
      = parseObj(R"({"dbu_x":1000,"dbu_y":1000,"zoom":0,"visible_layers":[],)"
                 R"("show_connectivity":true})");

  auto resp = handler_->handleSelect(req, state_);
  ASSERT_EQ(resp.type, WebSocketResponse::kJson) << payloadStr(resp);
  auto root = parseObj(payloadStr(resp));
  EXPECT_EQ(root.at("connected_added").as_int64(), 2) << payloadStr(resp);
  EXPECT_EQ(root.at("selection_count").as_int64(), 3);
  {
    std::lock_guard<std::mutex> lock(state_.selection_mutex);
    EXPECT_EQ(state_.selection_set.size(), 3u);
  }
}

// Non-SIGNAL nets (power/ground) are never pulled into the selection.
TEST_F(SelectHandlerTest, SelectConnectivityIgnoresNonSignalNets)
{
  odb::dbInst* inst = block_->findInst("buf1");
  ASSERT_NE(inst, nullptr);
  odb::dbNet* in_net = odb::dbNet::create(block_, "n_pwr");
  in_net->setSigType(odb::dbSigType::POWER);
  odb::dbNet* out_net = odb::dbNet::create(block_, "n_sig");
  inst->findITerm("A")->connect(in_net);
  inst->findITerm("Z")->connect(out_net);

  WebSocketRequest req;
  req.id = 21;
  req.type = WebSocketRequest::kSelect;
  req.json
      = parseObj(R"({"dbu_x":1000,"dbu_y":1000,"zoom":0,"visible_layers":[],)"
                 R"("show_connectivity":true})");

  auto root = parseObj(payloadStr(handler_->handleSelect(req, state_)));
  EXPECT_EQ(root.at("connected_added").as_int64(), 1)
      << boost::json::serialize(root);
  EXPECT_EQ(root.at("selection_count").as_int64(), 2);
}

// Without the flag a plain click never expands the selection, and the
// response carries no connected_added field.
TEST_F(SelectHandlerTest, SelectWithoutConnectivityFlagAddsNothing)
{
  odb::dbInst* inst = block_->findInst("buf1");
  ASSERT_NE(inst, nullptr);
  odb::dbNet* out_net = odb::dbNet::create(block_, "n_out");
  inst->findITerm("Z")->connect(out_net);

  WebSocketRequest req;
  req.id = 22;
  req.type = WebSocketRequest::kSelect;
  req.json
      = parseObj(R"({"dbu_x":1000,"dbu_y":1000,"zoom":0,"visible_layers":[]})");

  auto root = parseObj(payloadStr(handler_->handleSelect(req, state_)));
  EXPECT_FALSE(root.contains("connected_added"))
      << boost::json::serialize(root);
  EXPECT_EQ(root.at("selection_count").as_int64(), 1);
}

// Flight lines emitted by a descriptor's highlight() are collected into
// the session so the overlay can render them alongside timing lines.
TEST_F(SelectHandlerTest, InspectCollectsHighlightLines)
{
  LineFakeDescriptor line_descriptor;
  {
    std::lock_guard<std::mutex> lock(state_.selectables_mutex);
    state_.selectables = {gui::Selected(&fake_current_, &line_descriptor)};
  }

  WebSocketRequest req;
  req.id = 23;
  req.type = WebSocketRequest::kInspect;
  req.json = parseObj(R"({"select_id":0})");

  auto resp = handler_->handleInspect(req, state_);
  ASSERT_EQ(resp.type, WebSocketResponse::kJson) << payloadStr(resp);
  {
    std::lock_guard<std::mutex> lock(state_.selection_mutex);
    ASSERT_EQ(state_.highlight_lines.size(), 1u);
    EXPECT_EQ(state_.highlight_lines[0].p1, fake_current_.bbox.ll());
    EXPECT_EQ(state_.highlight_lines[0].p2, fake_current_.bbox.ur());
    EXPECT_TRUE(state_.highlight_rects.empty());
  }
}

TEST_F(SelectHandlerTest, InspectInvalidIdReturnsError)
{
  WebSocketRequest req;
  req.id = 12;
  req.type = WebSocketRequest::kInspect;
  req.json = parseObj(R"({"select_id":999})");

  auto resp = handler_->handleInspect(req, state_);
  EXPECT_EQ(resp.type, WebSocketResponse::kJson);

  std::string json = payloadStr(resp);
  EXPECT_NE(json.find("\"error\""), std::string::npos);
}

// An inspect that resolves to nothing clears the highlight vectors, so it must
// also clear the source tag.  Leaving it set let a later "Flywires only" flip
// re-derive the highlight from the stale current_inspected — resurrecting
// exactly what the tag exists to keep dismissed.
TEST_F(SelectHandlerTest, InvalidInspectDoesNotPrimeHighlights)
{
  {
    std::lock_guard<std::mutex> lock(state_.selection_mutex);
    state_.current_inspected = makeFakeSelected(&fake_current_);
    state_.highlight_source = SessionState::HighlightSource::kInspected;
  }

  WebSocketRequest req;
  req.id = 30;
  req.type = WebSocketRequest::kInspect;
  req.json = parseObj(R"({"select_id":999})");
  ASSERT_EQ(handler_->handleInspect(req, state_).type,
            WebSocketResponse::kJson);

  std::lock_guard<std::mutex> lock(state_.selection_mutex);
  EXPECT_TRUE(state_.highlight_rects.empty());
  EXPECT_EQ(state_.highlight_source, SessionState::HighlightSource::kNone)
      << "an empty selection must not leave the highlights primed";
}

TEST_F(SelectHandlerTest, HoverInvalidIdReturnsOkZeroCount)
{
  WebSocketRequest req;
  req.id = 13;
  req.type = WebSocketRequest::kHover;
  req.json = parseObj(R"({"select_id":999})");

  auto resp = handler_->handleHover(req, state_);
  EXPECT_EQ(resp.type, WebSocketResponse::kJson);

  std::string json = payloadStr(resp);
  EXPECT_NE(json.find("\"ok\":1"), std::string::npos);
  EXPECT_NE(json.find("\"count\":0"), std::string::npos);
}

TEST_F(SelectHandlerTest, SelectClearsTimingState)
{
  // Populate timing state
  {
    std::lock_guard<std::mutex> lock(state_.selection_mutex);
    state_.timing_rects.push_back(
        {odb::Rect(0, 0, 1, 1), {.r = 255, .g = 0, .b = 0, .a = 255}, ""});
    state_.timing_lines.push_back({odb::Point(0, 0),
                                   odb::Point(1, 1),
                                   {.r = 0, .g = 255, .b = 0, .a = 255}});
  }

  WebSocketRequest req;
  req.id = 14;
  req.type = WebSocketRequest::kSelect;
  req.json
      = parseObj(R"({"dbu_x":1000,"dbu_y":1000,"zoom":0,"visible_layers":[]})");

  handler_->handleSelect(req, state_);

  // Timing state should be cleared after select
  std::lock_guard<std::mutex> lock(state_.selection_mutex);
  EXPECT_TRUE(state_.timing_rects.empty());
  EXPECT_TRUE(state_.timing_lines.empty());
}

TEST_F(SelectHandlerTest, SelectClearsInspectorHistoryWhenNothingIsPicked)
{
  {
    std::lock_guard<std::mutex> lock(state_.selection_mutex);
    state_.current_inspected = makeFakeSelected(&fake_current_);
    state_.navigation_history.push_back(makeFakeSelected(&fake_previous_));
  }

  WebSocketRequest req;
  req.id = 15;
  req.type = WebSocketRequest::kSelect;
  req.json = parseObj(
      R"({"dbu_x":99000,"dbu_y":99000,"zoom":10,"visible_layers":[]})");

  auto resp = handler_->handleSelect(req, state_);
  EXPECT_EQ(resp.type, WebSocketResponse::kJson);

  const std::string json = payloadStr(resp);
  EXPECT_NE(json.find("\"can_navigate_back\":0"), std::string::npos);
  EXPECT_NE(json.find("\"selected\":[]"), std::string::npos);

  std::lock_guard<std::mutex> lock(state_.selection_mutex);
  EXPECT_FALSE(state_.current_inspected);
  EXPECT_TRUE(state_.navigation_history.empty());
}

TEST_F(SelectHandlerTest, InspectBackRestoresPreviousObject)
{
  const gui::Selected initial_selected = makeFakeSelected(&fake_current_);
  const gui::Selected block_selected = makeFakeSelected(&fake_previous_);
  ASSERT_TRUE(initial_selected);
  ASSERT_TRUE(block_selected);

  {
    std::lock_guard<std::mutex> lock(state_.selection_mutex);
    state_.current_inspected = initial_selected;
  }
  {
    std::lock_guard<std::mutex> lock(state_.selectables_mutex);
    state_.selectables = {block_selected};
  }

  WebSocketRequest inspect_req;
  inspect_req.id = 17;
  inspect_req.type = WebSocketRequest::kInspect;
  inspect_req.json = parseObj(R"({"select_id":0})");

  auto inspect_resp = handler_->handleInspect(inspect_req, state_);
  EXPECT_EQ(inspect_resp.type, WebSocketResponse::kJson);
  EXPECT_NE(payloadStr(inspect_resp).find("\"can_navigate_back\":1"),
            std::string::npos);

  {
    std::lock_guard<std::mutex> lock(state_.selection_mutex);
    EXPECT_TRUE(state_.current_inspected);
    EXPECT_NE(state_.current_inspected, initial_selected);
    ASSERT_EQ(state_.navigation_history.size(), 1u);
    EXPECT_EQ(state_.navigation_history.back(), initial_selected);
  }

  WebSocketRequest back_req;
  back_req.id = 18;
  back_req.type = WebSocketRequest::kInspectBack;

  auto back_resp = handler_->handleInspectBack(back_req, state_);
  EXPECT_EQ(back_resp.type, WebSocketResponse::kJson);
  EXPECT_NE(payloadStr(back_resp).find("\"can_navigate_back\":0"),
            std::string::npos);
  EXPECT_NE(payloadStr(back_resp).find(initial_selected.getName()),
            std::string::npos);

  {
    std::lock_guard<std::mutex> lock(state_.selection_mutex);
    EXPECT_EQ(state_.current_inspected, initial_selected);
    EXPECT_TRUE(state_.navigation_history.empty());
  }
}

TEST_F(SelectHandlerTest, InspectBackWithoutHistoryKeepsCurrentObject)
{
  const gui::Selected initial_selected = makeFakeSelected(&fake_current_);
  ASSERT_TRUE(initial_selected);
  {
    std::lock_guard<std::mutex> lock(state_.selection_mutex);
    state_.current_inspected = initial_selected;
  }

  WebSocketRequest back_req;
  back_req.id = 20;
  back_req.type = WebSocketRequest::kInspectBack;

  auto back_resp = handler_->handleInspectBack(back_req, state_);
  EXPECT_EQ(back_resp.type, WebSocketResponse::kJson);
  EXPECT_NE(payloadStr(back_resp).find("\"can_navigate_back\":0"),
            std::string::npos);
  EXPECT_NE(payloadStr(back_resp).find(initial_selected.getName()),
            std::string::npos);

  {
    std::lock_guard<std::mutex> lock(state_.selection_mutex);
    EXPECT_EQ(state_.current_inspected, initial_selected);
    EXPECT_TRUE(state_.navigation_history.empty());
  }
}

TEST_F(SelectHandlerTest, InspectRespectsDbuToggle)
{
  fake_current_.bbox = odb::Rect(2000, 4000, 6000, 8000);
  const gui::Selected block_selected = makeFakeSelected(&fake_current_);

  {
    std::lock_guard<std::mutex> lock(state_.selectables_mutex);
    state_.selectables = {block_selected};
  }

  // 1. inspect with use_dbu: true
  WebSocketRequest inspect_req_dbu;
  inspect_req_dbu.id = 21;
  inspect_req_dbu.type = WebSocketRequest::kInspect;
  inspect_req_dbu.json = parseObj(R"({"select_id":0,"use_dbu":true})");

  auto resp_dbu = handler_->handleInspect(inspect_req_dbu, state_);
  EXPECT_EQ(resp_dbu.type, WebSocketResponse::kJson);
  std::string json_dbu = payloadStr(resp_dbu);
  EXPECT_NE(json_dbu.find("\"value\":\"(2000, 4000), (6000, 8000)\""),
            std::string::npos)
      << json_dbu;

  // 2. inspect with use_dbu: false, using select_id: -1 to re-inspect current
  WebSocketRequest inspect_req_um;
  inspect_req_um.id = 22;
  inspect_req_um.type = WebSocketRequest::kInspect;
  inspect_req_um.json = parseObj(R"({"select_id":-1,"use_dbu":false})");

  auto resp_um = handler_->handleInspect(inspect_req_um, state_);
  EXPECT_EQ(resp_um.type, WebSocketResponse::kJson);
  std::string json_um = payloadStr(resp_um);
  EXPECT_NE(json_um.find("\"value\":\"(1, 2), (3, 4)\""), std::string::npos)
      << json_um;
}

// A multi-die design's top chip is hierarchical and owns no dbBlock, so the
// micron conversion has to come from the database rather than from a block --
// otherwise every inspected value (including a tech layer's pitch, width and
// spacing) would fall back to unlabelled raw DBU integers.
TEST_F(SelectHandlerTest, InspectConvertsToMicronsWithoutABlock)
{
  fake_current_.bbox = odb::Rect(2000, 4000, 6000, 8000);

  {
    std::lock_guard<std::mutex> lock(state_.selectables_mutex);
    state_.selectables = {makeFakeSelected(&fake_current_)};
  }

  odb::dbBlock::destroy(block_);
  block_ = nullptr;
  ASSERT_EQ(gen_->getBlock(), nullptr);
  ASSERT_NE(getDb()->getDbuPerMicron(), 0u);

  WebSocketRequest req;
  req.id = 23;
  req.type = WebSocketRequest::kInspect;
  req.json = parseObj(R"({"select_id":0,"use_dbu":false})");

  auto resp = handler_->handleInspect(req, state_);
  EXPECT_EQ(resp.type, WebSocketResponse::kJson);
  const std::string json = payloadStr(resp);
  EXPECT_NE(json.find("\"value\":\"(1, 2), (3, 4)\""), std::string::npos)
      << json;
}

// A PropertyTable (e.g. a layer's two-widths spacing table) is serialized as a
// grid the client draws as a table, not flattened to Property::toString()'s
// "<unknown>".
TEST_F(SelectHandlerTest, InspectSerializesPropertyTable)
{
  gui::PropertyTable table(/*rows=*/2, /*columns=*/2);
  table.setColumnHeader(0, "0 \xC2\xB5m");
  table.setColumnHeader(1, "0.2 \xC2\xB5m\nPRL 0.1 \xC2\xB5m");
  table.setRowHeader(0, "0 \xC2\xB5m");
  table.setRowHeader(1, "0.2 \xC2\xB5m\nPRL 0.1 \xC2\xB5m");
  table.setData(0, 0, "0.065 \xC2\xB5m");
  table.setData(0, 1, "0.1 \xC2\xB5m");
  table.setData(1, 0, "0.1 \xC2\xB5m");
  table.setData(1, 1, "0.2 \xC2\xB5m");
  fake_current_.properties = {{"Two width spacing rules", table}};

  {
    std::lock_guard<std::mutex> lock(state_.selectables_mutex);
    state_.selectables = {makeFakeSelected(&fake_current_)};
  }

  WebSocketRequest req;
  req.id = 30;
  req.type = WebSocketRequest::kInspect;
  req.json = parseObj(R"({"select_id":0,"use_dbu":true})");

  const std::string json = payloadStr(handler_->handleInspect(req, state_));
  EXPECT_EQ(json.find("<unknown>"), std::string::npos) << json;
  EXPECT_NE(json.find("\"table\""), std::string::npos) << json;
  EXPECT_NE(json.find("\"column_headers\""), std::string::npos) << json;
  EXPECT_NE(json.find("\"row_headers\""), std::string::npos) << json;
  // Newlines inside a header survive as real newlines for the client to
  // render as separate lines.
  EXPECT_NE(json.find("0.2 \xC2\xB5m\\nPRL 0.1 \xC2\xB5m"), std::string::npos)
      << json;
  EXPECT_NE(json.find("[[\"0.065 \xC2\xB5m\",\"0.1 \xC2\xB5m\"],"
                      "[\"0.1 \xC2\xB5m\",\"0.2 \xC2\xB5m\"]]"),
            std::string::npos)
      << json;
}

// An unkeyed list value (a layer's width table) becomes numbered children,
// mirroring the Qt inspector's indexed list rows.
TEST_F(SelectHandlerTest, InspectSerializesValueList)
{
  const std::vector<std::any> widths
      = {std::string("0.018 \xC2\xB5m"), std::string("0.09 \xC2\xB5m")};
  fake_current_.properties = {{"Width table", widths}};

  {
    std::lock_guard<std::mutex> lock(state_.selectables_mutex);
    state_.selectables = {makeFakeSelected(&fake_current_)};
  }

  WebSocketRequest req;
  req.id = 31;
  req.type = WebSocketRequest::kInspect;
  req.json = parseObj(R"({"select_id":0,"use_dbu":true})");

  const std::string json = payloadStr(handler_->handleInspect(req, state_));
  EXPECT_EQ(json.find("<unknown>"), std::string::npos) << json;
  EXPECT_NE(json.find("\"children\":[{\"name\":\"1\","
                      "\"value\":\"0.018 \xC2\xB5m\"},"
                      "{\"name\":\"2\",\"value\":\"0.09 \xC2\xB5m\"}]"),
            std::string::npos)
      << json;
}

//------------------------------------------------------------------------------
// Focus nets tests
//------------------------------------------------------------------------------

TEST_F(SelectHandlerTest, FocusNetAddValid)
{
  // Create a net in the block
  odb::dbNet::create(block_, "clk");

  WebSocketRequest req;
  req.id = 20;
  req.type = WebSocketRequest::kSetFocusNets;
  req.json = parseObj(R"({"action":"add","net_name":"clk"})");

  auto resp = handler_->handleSetFocusNets(req, state_);
  EXPECT_EQ(resp.id, 20u);
  EXPECT_EQ(resp.type, WebSocketResponse::kJson);

  std::string json = payloadStr(resp);
  EXPECT_NE(json.find("\"ok\":1"), std::string::npos);
  EXPECT_NE(json.find("\"count\":1"), std::string::npos);

  std::lock_guard<std::mutex> lock(state_.focus_nets_mutex);
  EXPECT_EQ(state_.focus_net_ids.size(), 1u);
}

TEST_F(SelectHandlerTest, FocusNetAddInvalidNetReturnsZeroCount)
{
  WebSocketRequest req;
  req.id = 21;
  req.type = WebSocketRequest::kSetFocusNets;
  req.json = parseObj(R"({"action":"add","net_name":"nonexistent_net"})");

  auto resp = handler_->handleSetFocusNets(req, state_);
  EXPECT_EQ(resp.type, WebSocketResponse::kJson);

  std::string json = payloadStr(resp);
  EXPECT_NE(json.find("\"count\":0"), std::string::npos);

  std::lock_guard<std::mutex> lock(state_.focus_nets_mutex);
  EXPECT_TRUE(state_.focus_net_ids.empty());
}

TEST_F(SelectHandlerTest, FocusNetRemove)
{
  odb::dbNet* net = odb::dbNet::create(block_, "data");

  // Add first
  {
    std::lock_guard<std::mutex> lock(state_.focus_nets_mutex);
    state_.focus_net_ids.insert(net->getId());
  }

  WebSocketRequest req;
  req.id = 22;
  req.type = WebSocketRequest::kSetFocusNets;
  req.json = parseObj(R"({"action":"remove","net_name":"data"})");

  auto resp = handler_->handleSetFocusNets(req, state_);
  EXPECT_EQ(resp.type, WebSocketResponse::kJson);

  std::string json = payloadStr(resp);
  EXPECT_NE(json.find("\"count\":0"), std::string::npos);

  std::lock_guard<std::mutex> lock(state_.focus_nets_mutex);
  EXPECT_TRUE(state_.focus_net_ids.empty());
}

TEST_F(SelectHandlerTest, FocusNetClear)
{
  odb::dbNet* n1 = odb::dbNet::create(block_, "net1");
  odb::dbNet* n2 = odb::dbNet::create(block_, "net2");

  {
    std::lock_guard<std::mutex> lock(state_.focus_nets_mutex);
    state_.focus_net_ids.insert(n1->getId());
    state_.focus_net_ids.insert(n2->getId());
  }

  WebSocketRequest req;
  req.id = 23;
  req.type = WebSocketRequest::kSetFocusNets;
  req.json = parseObj(R"({"action":"clear","net_name":""})");

  auto resp = handler_->handleSetFocusNets(req, state_);
  EXPECT_EQ(resp.type, WebSocketResponse::kJson);

  std::string json = payloadStr(resp);
  EXPECT_NE(json.find("\"count\":0"), std::string::npos);

  std::lock_guard<std::mutex> lock(state_.focus_nets_mutex);
  EXPECT_TRUE(state_.focus_net_ids.empty());
}

TEST_F(SelectHandlerTest, FocusNetAddMultiple)
{
  odb::dbNet::create(block_, "clk");
  odb::dbNet::create(block_, "reset");

  WebSocketRequest req1;
  req1.id = 24;
  req1.type = WebSocketRequest::kSetFocusNets;
  req1.json = parseObj(R"({"action":"add","net_name":"clk"})");
  handler_->handleSetFocusNets(req1, state_);

  WebSocketRequest req2;
  req2.id = 25;
  req2.type = WebSocketRequest::kSetFocusNets;
  req2.json = parseObj(R"({"action":"add","net_name":"reset"})");
  auto resp = handler_->handleSetFocusNets(req2, state_);

  std::string json = payloadStr(resp);
  EXPECT_NE(json.find("\"count\":2"), std::string::npos);

  std::lock_guard<std::mutex> lock(state_.focus_nets_mutex);
  EXPECT_EQ(state_.focus_net_ids.size(), 2u);
}

TEST_F(SelectHandlerTest, FocusNetAddDuplicateNoop)
{
  odb::dbNet::create(block_, "clk");

  WebSocketRequest req;
  req.id = 26;
  req.type = WebSocketRequest::kSetFocusNets;
  req.json = parseObj(R"({"action":"add","net_name":"clk"})");

  handler_->handleSetFocusNets(req, state_);
  auto resp = handler_->handleSetFocusNets(req, state_);

  std::string json = payloadStr(resp);
  EXPECT_NE(json.find("\"count\":1"), std::string::npos);
}

TEST_F(SelectHandlerTest, TileHandlerSnapshotsFocusNets)
{
  // Verify that TileHandler passes focus net state through to tiles.
  odb::dbNet* net = odb::dbNet::create(block_, "focused_net");
  {
    std::lock_guard<std::mutex> lock(state_.focus_nets_mutex);
    state_.focus_net_ids.insert(net->getId());
  }

  auto tile_handler = std::make_unique<TileHandler>(gen_);
  WebSocketRequest req;
  req.id = 27;
  req.type = WebSocketRequest::kTile;
  req.json
      = parseObj(R"({"layer":"metal1","z":0,"x":0,"y":0,"visible_layers":[]})");

  // Should not crash and should return valid PNG
  auto resp = tile_handler->handleTile(req, state_);
  EXPECT_EQ(resp.type, WebSocketResponse::kPng);  // PNG
  EXPECT_FALSE(resp.payload.empty());
}

//------------------------------------------------------------------------------
// Multi-selection tests
//------------------------------------------------------------------------------

// Helper: populate selection_set with two fake objects and point the
// iterator at whichever position (0 = begin, 1 = second).
// Because SelectionSet is std::set, iteration order is by operator<
// (FakeDescriptor::lessThan compares pointer addresses).
void populateSelectionSet(SessionState& st,
                          const gui::Selected& a,
                          const gui::Selected& b,
                          int itr_pos)
{
  std::lock_guard<std::mutex> lock(st.selection_mutex);
  st.selection_set.insert(a);
  st.selection_set.insert(b);
  st.selection_itr = st.selection_set.begin();
  if (itr_pos > 0) {
    std::advance(st.selection_itr, itr_pos);
  }
  st.current_inspected = *st.selection_itr;
}

// Verify that a select response always includes selection metadata.
TEST_F(SelectHandlerTest, SelectResponseIncludesSelectionMetadata)
{
  WebSocketRequest req;
  req.id = 30;
  req.type = WebSocketRequest::kSelect;
  req.json
      = parseObj(R"({"dbu_x":1000,"dbu_y":1000,"zoom":0,"visible_layers":[]})");

  auto resp = handler_->handleSelect(req, state_);
  EXPECT_EQ(resp.type, WebSocketResponse::kJson);

  const std::string json = payloadStr(resp);
  EXPECT_NE(json.find("\"selection_count\""), std::string::npos);
  EXPECT_NE(json.find("\"selection_index\""), std::string::npos);
}

TEST_F(SelectHandlerTest, SelectEmptyAreaClearsSelectionSet)
{
  // Pre-populate selection set
  {
    std::lock_guard<std::mutex> lock(state_.selection_mutex);
    state_.selection_set.insert(makeFakeSelected(&fake_current_));
    state_.selection_itr = state_.selection_set.begin();
  }

  WebSocketRequest req;
  req.id = 31;
  req.type = WebSocketRequest::kSelect;
  req.json = parseObj(
      R"({"dbu_x":99000,"dbu_y":99000,"zoom":10,"visible_layers":[]})");

  auto resp = handler_->handleSelect(req, state_);
  EXPECT_EQ(resp.type, WebSocketResponse::kJson);

  const std::string json = payloadStr(resp);
  EXPECT_NE(json.find("\"selection_count\":0"), std::string::npos);

  std::lock_guard<std::mutex> lock(state_.selection_mutex);
  EXPECT_TRUE(state_.selection_set.empty());
}

// Normal click (no add_to_selection) clears any pre-existing selection set.
TEST_F(SelectHandlerTest, SelectNormalClickClearsPreviousSelectionSet)
{
  populateSelectionSet(state_,
                       makeFakeSelected(&fake_current_),
                       makeFakeSelected(&fake_previous_),
                       1);

  // Normal click at empty area should clear the set
  WebSocketRequest req;
  req.id = 32;
  req.type = WebSocketRequest::kSelect;
  req.json = parseObj(
      R"({"dbu_x":99000,"dbu_y":99000,"zoom":10,"visible_layers":[]})");

  handler_->handleSelect(req, state_);

  std::lock_guard<std::mutex> lock(state_.selection_mutex);
  EXPECT_TRUE(state_.selection_set.empty());
}

// Shift+click on empty space should preserve the existing selection set.
TEST_F(SelectHandlerTest, AddToSelectionEmptyHitPreservesSet)
{
  {
    std::lock_guard<std::mutex> lock(state_.selection_mutex);
    state_.selection_set.insert(makeFakeSelected(&fake_current_));
    state_.selection_itr = state_.selection_set.begin();
  }

  WebSocketRequest req;
  req.id = 33;
  req.type = WebSocketRequest::kSelect;
  req.json = parseObj(
      R"({"dbu_x":99000,"dbu_y":99000,"zoom":10,"visible_layers":[],"add_to_selection":true})");

  handler_->handleSelect(req, state_);

  std::lock_guard<std::mutex> lock(state_.selection_mutex);
  EXPECT_EQ(state_.selection_set.size(), 1u);
}

// Verify SelectionSet deduplicates (std::set property).
TEST_F(SelectHandlerTest, SelectionSetDeduplicates)
{
  const auto sel = makeFakeSelected(&fake_current_);
  {
    std::lock_guard<std::mutex> lock(state_.selection_mutex);
    state_.selection_set.insert(sel);
    state_.selection_set.insert(sel);  // duplicate
  }
  EXPECT_EQ(state_.selection_set.size(), 1u);
}

TEST_F(SelectHandlerTest, SelectNextCyclesForward)
{
  populateSelectionSet(state_,
                       makeFakeSelected(&fake_current_),
                       makeFakeSelected(&fake_previous_),
                       0);

  WebSocketRequest req;
  req.id = 37;
  req.type = WebSocketRequest::kSelectNext;

  auto resp = handler_->handleSelectNext(req, state_);
  EXPECT_EQ(resp.type, WebSocketResponse::kJson);

  const std::string json = payloadStr(resp);
  EXPECT_NE(json.find("\"selection_count\":2"), std::string::npos);
  EXPECT_NE(json.find("\"selection_index\":1"), std::string::npos);

  std::lock_guard<std::mutex> lock(state_.selection_mutex);
  auto expected = std::next(state_.selection_set.begin());
  EXPECT_EQ(state_.selection_itr, expected);
}

TEST_F(SelectHandlerTest, SelectNextWrapsAround)
{
  populateSelectionSet(state_,
                       makeFakeSelected(&fake_current_),
                       makeFakeSelected(&fake_previous_),
                       1);  // at the end

  WebSocketRequest req;
  req.id = 38;
  req.type = WebSocketRequest::kSelectNext;

  auto resp = handler_->handleSelectNext(req, state_);
  const std::string json = payloadStr(resp);
  EXPECT_NE(json.find("\"selection_index\":0"), std::string::npos);

  std::lock_guard<std::mutex> lock(state_.selection_mutex);
  EXPECT_EQ(state_.selection_itr, state_.selection_set.begin());
}

TEST_F(SelectHandlerTest, SelectPrevCyclesBackward)
{
  populateSelectionSet(state_,
                       makeFakeSelected(&fake_current_),
                       makeFakeSelected(&fake_previous_),
                       1);

  WebSocketRequest req;
  req.id = 39;
  req.type = WebSocketRequest::kSelectPrev;

  auto resp = handler_->handleSelectPrev(req, state_);
  const std::string json = payloadStr(resp);
  EXPECT_NE(json.find("\"selection_index\":0"), std::string::npos);

  std::lock_guard<std::mutex> lock(state_.selection_mutex);
  EXPECT_EQ(state_.selection_itr, state_.selection_set.begin());
}

TEST_F(SelectHandlerTest, SelectPrevWrapsAround)
{
  populateSelectionSet(state_,
                       makeFakeSelected(&fake_current_),
                       makeFakeSelected(&fake_previous_),
                       0);  // at the start

  WebSocketRequest req;
  req.id = 40;
  req.type = WebSocketRequest::kSelectPrev;

  auto resp = handler_->handleSelectPrev(req, state_);
  const std::string json = payloadStr(resp);
  EXPECT_NE(json.find("\"selection_index\":1"), std::string::npos);

  std::lock_guard<std::mutex> lock(state_.selection_mutex);
  EXPECT_EQ(state_.selection_itr, std::next(state_.selection_set.begin()));
}

TEST_F(SelectHandlerTest, SelectNextEmptySetReturnsError)
{
  WebSocketRequest req;
  req.id = 41;
  req.type = WebSocketRequest::kSelectNext;

  auto resp = handler_->handleSelectNext(req, state_);
  EXPECT_EQ(resp.type, WebSocketResponse::kJson);

  const std::string json = payloadStr(resp);
  EXPECT_NE(json.find("\"selection_count\":0"), std::string::npos);
  EXPECT_NE(json.find("\"error\""), std::string::npos);
}

TEST_F(SelectHandlerTest, SelectNextClearsNavigationHistory)
{
  populateSelectionSet(state_,
                       makeFakeSelected(&fake_current_),
                       makeFakeSelected(&fake_previous_),
                       0);
  {
    std::lock_guard<std::mutex> lock(state_.selection_mutex);
    state_.navigation_history.push_back(makeFakeSelected(&fake_previous_));
  }

  WebSocketRequest req;
  req.id = 42;
  req.type = WebSocketRequest::kSelectNext;

  handler_->handleSelectNext(req, state_);

  std::lock_guard<std::mutex> lock(state_.selection_mutex);
  EXPECT_TRUE(state_.navigation_history.empty());
}

//------------------------------------------------------------------------------
// select_layer tests
//
// Backs the Display Control panel's click-to-select, mirroring the Qt GUI's
// DisplayControls::displayItemSelected.  These exercise layer resolution and
// the selection-state replacement; the inspect payload's contents come from
// DbTechLayerDescriptor, which this binary does not register (see
// FakeDescriptor above), so a resolved layer is asserted via the response
// being kJson rather than the not-found kError.
//------------------------------------------------------------------------------

TEST_F(SelectHandlerTest, SelectLayerResolvesLayerByName)
{
  ASSERT_NE(getDb()->getTech()->findLayer("metal1"), nullptr);

  WebSocketRequest req;
  req.id = 50;
  req.type = WebSocketRequest::kSelectLayer;
  req.json = parseObj(R"({"layer":"metal1"})");

  auto resp = handler_->handleSelectLayer(req, state_);
  EXPECT_EQ(resp.id, 50u);
  EXPECT_EQ(resp.type, WebSocketResponse::kJson) << payloadStr(resp);

  const std::string json = payloadStr(resp);
  EXPECT_NE(json.find("\"selection_count\""), std::string::npos) << json;
  EXPECT_NE(json.find("\"selection_index\""), std::string::npos) << json;
}

TEST_F(SelectHandlerTest, SelectLayerUnknownNameReturnsError)
{
  WebSocketRequest req;
  req.id = 51;
  req.type = WebSocketRequest::kSelectLayer;
  req.json = parseObj(R"({"layer":"no_such_layer"})");

  auto resp = handler_->handleSelectLayer(req, state_);
  EXPECT_EQ(resp.type, WebSocketResponse::kError);
  EXPECT_NE(payloadStr(resp).find("Layer not found: no_such_layer"),
            std::string::npos)
      << payloadStr(resp);
}

TEST_F(SelectHandlerTest, SelectLayerMissingFieldReturnsError)
{
  WebSocketRequest req;
  req.id = 52;
  req.type = WebSocketRequest::kSelectLayer;
  req.json = parseObj(R"({"use_dbu":true})");

  auto resp = handler_->handleSelectLayer(req, state_);
  EXPECT_EQ(resp.type, WebSocketResponse::kError);
  EXPECT_NE(payloadStr(resp).find("server error"), std::string::npos);
}

// A chiplet path that matches no ChipletNode falls back to the design's
// default tech rather than failing, so a stale path in the frontend (e.g.
// after the hierarchy changed) still resolves the layer.
TEST_F(SelectHandlerTest, SelectLayerUnknownChipletFallsBackToDefaultTech)
{
  WebSocketRequest req;
  req.id = 53;
  req.type = WebSocketRequest::kSelectLayer;
  req.json = parseObj(R"({"layer":"metal1","chiplet":"top.nonexistent"})");

  auto resp = handler_->handleSelectLayer(req, state_);
  EXPECT_EQ(resp.type, WebSocketResponse::kJson) << payloadStr(resp);
}

// Selecting a layer is a plain (non-shift) selection: it replaces whatever
// was selected before and drops the hover/timing overlays, so the canvas
// does not keep painting the previous object's highlight.
TEST_F(SelectHandlerTest, SelectLayerReplacesPreviousSelection)
{
  populateSelectionSet(state_,
                       makeFakeSelected(&fake_current_),
                       makeFakeSelected(&fake_previous_),
                       0);
  {
    std::lock_guard<std::mutex> lock(state_.selection_mutex);
    state_.hover_rects.emplace_back(0, 0, 10, 10);
    state_.timing_rects.push_back(
        {odb::Rect(0, 0, 10, 10), {.r = 255, .g = 0, .b = 0, .a = 255}, ""});
    state_.highlight_rects.emplace_back(0, 0, 10, 10);
    state_.navigation_history.push_back(makeFakeSelected(&fake_previous_));
  }

  WebSocketRequest req;
  req.id = 54;
  req.type = WebSocketRequest::kSelectLayer;
  req.json = parseObj(R"({"layer":"metal1"})");

  auto resp = handler_->handleSelectLayer(req, state_);
  ASSERT_EQ(resp.type, WebSocketResponse::kJson) << payloadStr(resp);

  std::lock_guard<std::mutex> lock(state_.selection_mutex);
  EXPECT_TRUE(state_.hover_rects.empty());
  EXPECT_TRUE(state_.timing_rects.empty());
  EXPECT_TRUE(state_.navigation_history.empty());
  // A tech layer has no geometry of its own, so the previous object's
  // highlight must be gone rather than replaced.
  EXPECT_TRUE(state_.highlight_rects.empty());
  EXPECT_EQ(state_.selection_set.count(makeFakeSelected(&fake_current_)), 0u);
}

// An error path must not leave the session half-updated: the layer lookup
// throws before any state is touched.
TEST_F(SelectHandlerTest, SelectLayerErrorKeepsPreviousSelection)
{
  populateSelectionSet(state_,
                       makeFakeSelected(&fake_current_),
                       makeFakeSelected(&fake_previous_),
                       0);

  WebSocketRequest req;
  req.id = 55;
  req.type = WebSocketRequest::kSelectLayer;
  req.json = parseObj(R"({"layer":"no_such_layer"})");

  auto resp = handler_->handleSelectLayer(req, state_);
  ASSERT_EQ(resp.type, WebSocketResponse::kError);

  std::lock_guard<std::mutex> lock(state_.selection_mutex);
  EXPECT_EQ(state_.selection_set.size(), 2u);
}

TEST_F(SelectHandlerTest, InspectResponseIncludesSelectionMetadata)
{
  populateSelectionSet(state_,
                       makeFakeSelected(&fake_current_),
                       makeFakeSelected(&fake_previous_),
                       0);
  {
    std::lock_guard<std::mutex> lock(state_.selectables_mutex);
    state_.selectables = {makeFakeSelected(&fake_previous_)};
  }

  WebSocketRequest req;
  req.id = 43;
  req.type = WebSocketRequest::kInspect;
  req.json = parseObj(R"({"select_id":0})");

  auto resp = handler_->handleInspect(req, state_);
  const std::string json = payloadStr(resp);
  EXPECT_NE(json.find("\"selection_count\":2"), std::string::npos);
  EXPECT_NE(json.find("\"selection_index\":1"), std::string::npos);
}

TEST_F(SelectHandlerTest, InspectBackResponseIncludesSelectionMetadata)
{
  populateSelectionSet(state_,
                       makeFakeSelected(&fake_current_),
                       makeFakeSelected(&fake_previous_),
                       1);
  {
    std::lock_guard<std::mutex> lock(state_.selection_mutex);
    state_.navigation_history.push_back(makeFakeSelected(&fake_current_));
  }

  WebSocketRequest req;
  req.id = 44;
  req.type = WebSocketRequest::kInspectBack;

  auto resp = handler_->handleInspectBack(req, state_);
  const std::string json = payloadStr(resp);
  EXPECT_NE(json.find("\"selection_count\":2"), std::string::npos);
}

TEST_F(SelectHandlerTest, SelectNextRestoresSelectionSetHighlights)
{
  const auto sel_a = makeFakeSelected(&fake_current_);
  const auto sel_b = makeFakeSelected(&fake_previous_);

  populateSelectionSet(state_, sel_a, sel_b, 0);

  const odb::Rect stale_rect(9999, 9999, 10000, 10000);
  {
    std::lock_guard<std::mutex> lock(state_.selection_mutex);
    state_.highlight_rects = {stale_rect};
    state_.highlight_polys.clear();
  }

  WebSocketRequest req;
  req.id = 50;
  req.type = WebSocketRequest::kSelectNext;
  handler_->handleSelectNext(req, state_);

  std::lock_guard<std::mutex> lock(state_.selection_mutex);
  EXPECT_EQ(state_.highlight_rects.size(), 2u);
  bool found_current = false;
  bool found_previous = false;
  for (const auto& r : state_.highlight_rects) {
    EXPECT_FALSE(r == stale_rect);
    found_current |= r == fake_current_.bbox;
    found_previous |= r == fake_previous_.bbox;
  }
  EXPECT_TRUE(found_current);
  EXPECT_TRUE(found_previous);
}

//------------------------------------------------------------------------------
// Find (name/glob search) tests.  The gui descriptors are not registered in
// this unit context, so makeSelected() yields empty Selecteds and inserting
// more than one into the SelectionSet would dereference a null descriptor in
// the comparator.  We therefore assert the match `count` for patterns matching
// at most one object — enough to exercise the glob (*, ?), exact-match and
// case-folding logic plus the error path.  Multi-match selection and the
// sel_has_inst/sel_has_net flags are covered by the WebSocket end-to-end tests.
//------------------------------------------------------------------------------

class FindHandlerTest : public tst::Nangate45Fixture
{
 protected:
  void SetUp() override
  {
    block_->setDieArea(odb::Rect(0, 0, 100000, 100000));
    gen_ = std::make_shared<TileGenerator>(
        getDb(), /*sta=*/nullptr, getLogger());
    tcl_eval_ = std::make_shared<TclEvaluator>(/*interp=*/nullptr, getLogger());
    handler_ = std::make_unique<SelectHandler>(gen_, tcl_eval_);
  }

  void placeInst(const char* master_name, const char* inst_name, int x, int y)
  {
    odb::dbMaster* master = lib_->findMaster(master_name);
    ASSERT_NE(master, nullptr) << master_name;
    odb::dbInst* inst = odb::dbInst::create(block_, master, inst_name);
    inst->setLocation(x, y);
    inst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
  }

  void runFind(const std::string& obj_type,
               const std::string& pattern,
               bool match_case = false)
  {
    WebSocketRequest req;
    req.id = 1;
    req.type = WebSocketRequest::kFind;
    boost::json::object json;
    json["obj_type"] = obj_type;
    json["pattern"] = pattern;
    json["match_case"] = match_case;
    req.json = std::move(json);
    last_resp_ = handler_->handleFind(req, state_);
  }

  // Pattern-matching assertions only.  Drops the previous find's selection
  // first: a find ADDS to the selection (Qt parity), and two of this
  // fixture's null-descriptor Selecteds cannot be ordered against each other
  // -- Selected::operator< dereferences the descriptor once the payload types
  // match.  Accumulation is covered by FindAddsToTheExistingSelection, which
  // pre-seeds a differently-typed payload so the comparison stays safe.
  int64_t findCount(const std::string& obj_type,
                    const std::string& pattern,
                    bool match_case = false)
  {
    {
      std::lock_guard<std::mutex> lock(state_.selection_mutex);
      state_.selection_set.clear();
      state_.selection_itr = state_.selection_set.end();
    }
    runFind(obj_type, pattern, match_case);
    return parseObj(payloadStr(last_resp_)).at("count").as_int64();
  }

  gui::Selected makeFakeSelected(FakeInspectable* object)
  {
    return gui::Selected(object, &fake_descriptor_);
  }

  std::shared_ptr<TileGenerator> gen_;
  std::shared_ptr<TclEvaluator> tcl_eval_;
  std::unique_ptr<SelectHandler> handler_;
  SessionState state_;
  WebSocketResponse last_resp_;
  FakeDescriptor fake_descriptor_;
  FakeInspectable fake_preselected_{.name = "preselected",
                                    .type = "FakePreselected",
                                    .bbox = {0, 0, 10, 10},
                                    .properties = {}};
};

TEST_F(FindHandlerTest, InstGlobExactAndNoMatch)
{
  placeInst("BUF_X16", "u_buf0", 0, 0);
  placeInst("BUF_X16", "reg_a", 1000, 0);

  EXPECT_EQ(findCount("inst", "u_*"), 1);     // glob '*'
  EXPECT_EQ(findCount("inst", "u_bu?0"), 1);  // glob '?'
  EXPECT_EQ(findCount("inst", "u_buf0"), 1);  // exact
  EXPECT_EQ(findCount("inst", "zzz*"), 0);    // no match
}

TEST_F(FindHandlerTest, RespectsMatchCase)
{
  placeInst("BUF_X16", "MixedCase", 0, 0);

  EXPECT_EQ(findCount("inst", "mixedcase", /*match_case=*/false), 1);
  EXPECT_EQ(findCount("inst", "mixedcase", /*match_case=*/true), 0);
}

TEST_F(FindHandlerTest, NetByName)
{
  odb::dbNet::create(block_, "clk");

  EXPECT_EQ(findCount("net", "cl*"), 1);
  EXPECT_EQ(findCount("net", "nope"), 0);
}

TEST_F(FindHandlerTest, UnknownTypeReturnsError)
{
  runFind("bogus", "*");
  EXPECT_EQ(last_resp_.type, WebSocketResponse::kError);
}

// Qt parity: Gui::select() passes its matches to MainWindow::addSelected, so
// a search must not discard a selection the user already had.
//
// The pre-existing entry is FakeDescriptor-backed on purpose.  Its payload
// type (FakeInspectable*) differs from the one an unregistered dbInst
// descriptor yields, and Selected::operator< compares differing payload types
// by type_info alone -- so the set stays ordered without dereferencing the
// null descriptor that makes multi-match asserts impossible here (see the
// comment above this fixture).
TEST_F(FindHandlerTest, FindAddsToTheExistingSelection)
{
  placeInst("BUF_X16", "u_buf0", 0, 0);
  {
    std::lock_guard<std::mutex> lock(state_.selection_mutex);
    state_.selection_set.insert(makeFakeSelected(&fake_preselected_));
  }

  runFind("inst", "u_buf0");

  const auto root = parseObj(payloadStr(last_resp_));
  EXPECT_EQ(root.at("count").as_int64(), 1);
  // 2, not 1: the found instance joined the pre-existing selection.
  EXPECT_EQ(root.at("selection_count").as_int64(), 2);
  std::lock_guard<std::mutex> lock(state_.selection_mutex);
  EXPECT_TRUE(
      state_.selection_set.contains(makeFakeSelected(&fake_preselected_)))
      << "the pre-existing selection must survive a find";
}

// set_property tests (descriptor editors)
//------------------------------------------------------------------------------

// FakeDescriptor with descriptor editors, mirroring the shapes found in
// dbDescriptors.cpp: a string editor (Name), a bool editor (Dont Touch),
// a number editor (Weight), an option-list editor (Orientation), and a
// string editor that exercises Property::convert_string (Location).
class EditableFakeDescriptor : public FakeDescriptor
{
 public:
  enum class EditMode
  {
    kAccept,
    kReject,
    kThrow
  };
  EditMode edit_mode = EditMode::kAccept;
  mutable std::any last_value;
  mutable int edit_calls = 0;
  mutable int location_dbu = -1;
  mutable bool location_ok = false;

  Properties getProperties(const std::any& object) const override
  {
    auto* fake = std::any_cast<FakeInspectable*>(object);
    return {{"Name", fake->name},
            {"Dont Touch", false},
            {"Weight", 42},
            {"Orientation", std::string("R0")},
            {"Location", std::string("0")}};
  }

  // Action test hooks: "Jump" selects jump_target, "Refresh" keeps the
  // selection, "Explode" throws, "Delete" simulates an odb destroy by
  // raising *stale_flag (what the session's inDb*Destroy callback does),
  // "Insert Buffer" must be suppressed, and "deselect" is the reserved
  // lifecycle callback.
  FakeInspectable* jump_target = nullptr;
  std::atomic<bool>* stale_flag = nullptr;
  mutable int deselect_calls = 0;

  Actions getActions(const std::any& object) const override
  {
    Actions actions;
    actions.push_back({std::string(gui::Descriptor::kDeselectAction),
                       [this]() -> gui::Selected {
                         ++deselect_calls;
                         return {};
                       }});
    actions.push_back({"Jump", [this]() -> gui::Selected {
                         return gui::Selected(jump_target, this);
                       }});
    actions.push_back({"Refresh", [this, object]() -> gui::Selected {
                         return gui::Selected(object, this);
                       }});
    actions.push_back({"Explode", []() -> gui::Selected {
                         throw std::runtime_error("action exploded");
                       }});
    actions.push_back({"Insert Buffer", []() -> gui::Selected { return {}; }});
    actions.push_back({"Delete", [this]() -> gui::Selected {
                         if (stale_flag != nullptr) {
                           *stale_flag = true;
                         }
                         return {};
                       }});
    return actions;
  }

  Editors getEditors(const std::any& object) const override
  {
    auto* fake = std::any_cast<FakeInspectable*>(object);
    Editors editors;
    editors.emplace("Name", makeEditor([this, fake](const std::any& value) {
                      return commit(value, [&] {
                        fake->name = std::any_cast<std::string>(value);
                      });
                    }));
    editors.emplace("Dont Touch", makeEditor([this](const std::any& value) {
                      return commit(value);
                    }));
    editors.emplace("Weight", makeEditor([this](const std::any& value) {
                      return commit(value);
                    }));
    editors.emplace(
        "Orientation",
        makeEditor([this](const std::any& value) { return commit(value); },
                   {{"R0", 0}, {"R90", 90}, {"R180", 180}}));
    editors.emplace(
        "Location", makeEditor([this](const std::any& value) {
          return commit(value, [&] {
            location_dbu = gui::Descriptor::Property::convert_string(
                std::any_cast<std::string>(value), &location_ok);
          });
        }));
    return editors;
  }

 private:
  bool commit(const std::any& value,
              const std::function<void()>& apply = nullptr) const
  {
    ++edit_calls;
    last_value = value;
    if (edit_mode == EditMode::kThrow) {
      throw std::runtime_error("edit exploded");
    }
    if (edit_mode == EditMode::kReject) {
      return false;
    }
    if (apply) {
      apply();
    }
    return true;
  }
};

class SetPropertyTest : public SelectHandlerTest
{
 protected:
  void SetUp() override
  {
    SelectHandlerTest::SetUp();
    handler_->setBroadcastFn(
        [this](const std::string& json) { broadcasts_.push_back(json); });
    editable_descriptor_.jump_target = &fake_previous_;
    editable_descriptor_.stale_flag = &state_.selection_stale;
    std::lock_guard<std::mutex> lock(state_.selection_mutex);
    state_.current_inspected
        = gui::Selected(&fake_current_, &editable_descriptor_);
  }

  boost::json::object setProperty(const std::string& request_json)
  {
    WebSocketRequest req;
    req.id = 77;
    req.type = WebSocketRequest::kSetProperty;
    req.json = parseObj(request_json);
    auto resp = handler_->handleSetProperty(req, state_);
    EXPECT_EQ(resp.type, WebSocketResponse::kJson) << payloadStr(resp);
    return parseObj(payloadStr(resp));
  }

  EditableFakeDescriptor editable_descriptor_;
  std::vector<std::string> broadcasts_;
};

TEST_F(SetPropertyTest, InspectPayloadCarriesEditors)
{
  WebSocketRequest req;
  req.id = 70;
  req.type = WebSocketRequest::kInspect;
  req.json = parseObj(R"({"select_id":-1})");
  auto root = parseObj(payloadStr(handler_->handleInspect(req, state_)));

  std::map<std::string, boost::json::object> by_name;
  for (const auto& p : root.at("properties").as_array()) {
    const auto& obj = p.as_object();
    by_name[std::string(obj.at("name").as_string())] = obj;
  }
  ASSERT_TRUE(by_name.count("Name"));
  EXPECT_TRUE(by_name["Name"].at("editable").as_bool());
  EXPECT_EQ(by_name["Name"].at("editor").as_object().at("type").as_string(),
            "string");
  EXPECT_EQ(
      by_name["Dont Touch"].at("editor").as_object().at("type").as_string(),
      "bool");
  EXPECT_EQ(by_name["Weight"].at("editor").as_object().at("type").as_string(),
            "number");
  const auto& orient = by_name["Orientation"].at("editor").as_object();
  EXPECT_EQ(orient.at("type").as_string(), "list");
  const auto& options = orient.at("options").as_array();
  ASSERT_EQ(options.size(), 3u);
  EXPECT_EQ(options[1].as_string(), "R90");
}

TEST_F(SetPropertyTest, StringEditAcceptedAndBroadcast)
{
  auto root = setProperty(R"({"name":"Name","value":"renamed"})");
  EXPECT_EQ(root.at("ok").as_int64(), 1);
  EXPECT_EQ(fake_current_.name, "renamed");
  // Payload is rebuilt after the edit, reflecting the new value.
  EXPECT_EQ(root.at("name").as_string(), "renamed");
  ASSERT_EQ(broadcasts_.size(), 1u);
  auto push = parseObj(broadcasts_[0]);
  EXPECT_EQ(push.at("type").as_string(), "refresh");
  // Edits can change the design bounds (and with them the tile
  // georeference); the push carries the current bounds so clients can
  // resync their coordinate transforms without an extra round-trip.
  ASSERT_TRUE(push.if_contains("bounds"));
  const auto expected = serializeBoundsResponse(*gen_, gen_->shapesReady());
  EXPECT_EQ(boost::json::serialize(push.at("bounds")),
            boost::json::serialize(expected.at("bounds")));
}

// Documents the dynamic-bounds behavior the client resync exists for:
// content added or moved outside the current block bbox changes the
// served bounds (block bbox + pin-label margin).
TEST_F(SetPropertyTest, BoundsFollowBlockBBoxChanges)
{
  const auto before = boost::json::serialize(
      serializeBoundsResponse(*gen_, true).at("bounds"));
  placeInst("BUF_X16", "far_away", -200000, -200000);
  const auto after = boost::json::serialize(
      serializeBoundsResponse(*gen_, true).at("bounds"));
  EXPECT_NE(before, after);
}

TEST_F(SetPropertyTest, RejectedEditReportsErrorWithoutBroadcast)
{
  editable_descriptor_.edit_mode = EditableFakeDescriptor::EditMode::kReject;
  auto root = setProperty(R"({"name":"Name","value":"renamed"})");
  EXPECT_EQ(root.at("ok").as_int64(), 0);
  EXPECT_NE(std::string(root.at("error").as_string()).find("rejected"),
            std::string::npos);
  EXPECT_EQ(fake_current_.name, "current");
  EXPECT_TRUE(broadcasts_.empty());
}

TEST_F(SetPropertyTest, ThrowingEditIsContained)
{
  editable_descriptor_.edit_mode = EditableFakeDescriptor::EditMode::kThrow;
  auto root = setProperty(R"({"name":"Name","value":"renamed"})");
  EXPECT_EQ(root.at("ok").as_int64(), 0);
  EXPECT_EQ(root.at("error").as_string(), "edit exploded");
  EXPECT_TRUE(broadcasts_.empty());
}

TEST_F(SetPropertyTest, ListEditCommitsOptionValueByIndex)
{
  auto root = setProperty(
      R"({"name":"Orientation","option_index":1,"option_name":"R90"})");
  EXPECT_EQ(root.at("ok").as_int64(), 1);
  // The callback must receive the option's exact std::any (int 90 here).
  EXPECT_EQ(std::any_cast<int>(editable_descriptor_.last_value), 90);
}

TEST_F(SetPropertyTest, ListEditRejectsBadIndexAndStaleName)
{
  auto root = setProperty(
      R"({"name":"Orientation","option_index":7,"option_name":"R270"})");
  EXPECT_EQ(root.at("ok").as_int64(), 0);
  EXPECT_NE(std::string(root.at("error").as_string()).find("option index"),
            std::string::npos);

  root = setProperty(
      R"({"name":"Orientation","option_index":1,"option_name":"R180"})");
  EXPECT_EQ(root.at("ok").as_int64(), 0);
  EXPECT_NE(std::string(root.at("error").as_string()).find("options changed"),
            std::string::npos);
  EXPECT_EQ(editable_descriptor_.edit_calls, 0);
}

TEST_F(SetPropertyTest, BoolAndNumberMarshalling)
{
  setProperty(R"({"name":"Dont Touch","value":true})");
  const bool* flag = std::any_cast<bool>(&editable_descriptor_.last_value);
  ASSERT_NE(flag, nullptr) << "a bool editor must be handed a bool";
  EXPECT_TRUE(*flag);

  // JSON integers arrive at the callback as double (Qt delegate parity).
  setProperty(R"({"name":"Weight","value":5})");
  const double* weight
      = std::any_cast<double>(&editable_descriptor_.last_value);
  ASSERT_NE(weight, nullptr) << "a number editor must be handed a double";
  EXPECT_DOUBLE_EQ(*weight, 5.0);
}

TEST_F(SetPropertyTest, UnknownPropertyOrNothingInspected)
{
  auto root = setProperty(R"({"name":"Nope","value":"x"})");
  EXPECT_EQ(root.at("ok").as_int64(), 0);
  EXPECT_NE(std::string(root.at("error").as_string()).find("not editable"),
            std::string::npos);

  {
    std::lock_guard<std::mutex> lock(state_.selection_mutex);
    state_.current_inspected = gui::Selected();
  }
  root = setProperty(R"({"name":"Name","value":"x"})");
  EXPECT_EQ(root.at("ok").as_int64(), 0);
  EXPECT_NE(std::string(root.at("error").as_string()).find("nothing"),
            std::string::npos);
  EXPECT_TRUE(broadcasts_.empty());
}

// The scoped unit format must install a working Property::convert_string
// for the duration of the edit: in micron mode "1.5" is 1.5 µm (Nangate45:
// 2000 DBU/µm → 3000); in DBU mode only integers parse.  The library
// default (restored afterwards) returns 0 without setting the ok flag.
TEST_F(SetPropertyTest, ConvertStringInstalledDuringEdit)
{
  auto root = setProperty(R"({"name":"Location","value":"1.5"})");
  EXPECT_EQ(root.at("ok").as_int64(), 1);
  EXPECT_TRUE(editable_descriptor_.location_ok);
  EXPECT_EQ(editable_descriptor_.location_dbu, 3000);

  root = setProperty(
      R"({"name":"Location","value":"12.34 µm","use_dbu":false})");
  EXPECT_TRUE(editable_descriptor_.location_ok);
  EXPECT_EQ(editable_descriptor_.location_dbu, 24680);

  root = setProperty(R"({"name":"Location","value":"1.5","use_dbu":true})");
  EXPECT_FALSE(editable_descriptor_.location_ok);
  root = setProperty(R"({"name":"Location","value":"150","use_dbu":true})");
  EXPECT_TRUE(editable_descriptor_.location_ok);
  EXPECT_EQ(editable_descriptor_.location_dbu, 150);

  // Restored default after the handler: returns 0, never touches ok.
  bool ok = false;
  EXPECT_EQ(gui::Descriptor::Property::convert_string("5", &ok), 0);
  EXPECT_FALSE(ok);
}

//------------------------------------------------------------------------------
// trigger_action tests (descriptor actions)
//------------------------------------------------------------------------------

class TriggerActionTest : public SetPropertyTest
{
 protected:
  boost::json::object triggerAction(const std::string& request_json)
  {
    WebSocketRequest req;
    req.id = 88;
    req.type = WebSocketRequest::kTriggerAction;
    req.json = parseObj(request_json);
    auto resp = handler_->handleTriggerAction(req, state_);
    EXPECT_EQ(resp.type, WebSocketResponse::kJson) << payloadStr(resp);
    return parseObj(payloadStr(resp));
  }
};

TEST_F(TriggerActionTest, PayloadFiltersSuppressedAndReservedActions)
{
  WebSocketRequest req;
  req.id = 80;
  req.type = WebSocketRequest::kInspect;
  req.json = parseObj(R"({"select_id":-1})");
  auto root = parseObj(payloadStr(handler_->handleInspect(req, state_)));

  ASSERT_TRUE(root.if_contains("actions"));
  std::set<std::string> names;
  for (const auto& a : root.at("actions").as_array()) {
    names.emplace(a.as_string());
  }
  EXPECT_EQ(names,
            (std::set<std::string>{"Jump", "Refresh", "Explode", "Delete"}));
  // "Insert Buffer" (Qt dialog), "deselect" (reserved), and the universal
  // "Zoom to" (client-side button exists) must not be offered.
}

TEST_F(TriggerActionTest, ActionChangingSelectionUpdatesStateAndHistory)
{
  auto root = triggerAction(R"({"name":"Jump"})");
  EXPECT_EQ(root.at("ok").as_int64(), 1);
  EXPECT_EQ(root.at("deleted").as_int64(), 0);
  EXPECT_EQ(root.at("name").as_string(), "previous");
  EXPECT_EQ(root.at("can_navigate_back").as_int64(), 1);
  // The old object's reserved deselect callback ran.
  EXPECT_EQ(editable_descriptor_.deselect_calls, 1);
  {
    std::lock_guard<std::mutex> lock(state_.selection_mutex);
    EXPECT_TRUE(state_.current_inspected
                == gui::Selected(&fake_previous_, &editable_descriptor_));
    ASSERT_EQ(state_.navigation_history.size(), 1u);
  }
  ASSERT_EQ(broadcasts_.size(), 1u);
  EXPECT_NE(broadcasts_[0].find("refresh"), std::string::npos);
}

TEST_F(TriggerActionTest, ActionKeepingSelectionRefreshesPayload)
{
  auto root = triggerAction(R"({"name":"Refresh"})");
  EXPECT_EQ(root.at("ok").as_int64(), 1);
  EXPECT_EQ(root.at("name").as_string(), "current");
  EXPECT_EQ(editable_descriptor_.deselect_calls, 0);
  std::lock_guard<std::mutex> lock(state_.selection_mutex);
  EXPECT_TRUE(state_.current_inspected
              == gui::Selected(&fake_current_, &editable_descriptor_));
}

TEST_F(TriggerActionTest, DeleteClearsSelectionStateAndReportsDeleted)
{
  {
    std::lock_guard<std::mutex> lock(state_.selection_mutex);
    state_.navigation_history.emplace_back(&fake_previous_,
                                           &editable_descriptor_);
    state_.selection_set.insert(state_.current_inspected);
    state_.selection_itr = state_.selection_set.begin();
    state_.highlight_rects.push_back(fake_current_.bbox);
  }

  auto root = triggerAction(R"({"name":"Delete"})");
  EXPECT_EQ(root.at("ok").as_int64(), 1);
  EXPECT_EQ(root.at("deleted").as_int64(), 1);
  EXPECT_EQ(root.at("can_navigate_back").as_int64(), 0);
  EXPECT_EQ(root.at("selection_count").as_int64(), 0);
  EXPECT_FALSE(root.if_contains("properties"));
  {
    std::lock_guard<std::mutex> lock(state_.selection_mutex);
    EXPECT_FALSE(state_.current_inspected);
    EXPECT_TRUE(state_.navigation_history.empty());
    EXPECT_TRUE(state_.selection_set.empty());
    EXPECT_TRUE(state_.highlight_rects.empty());
  }
  EXPECT_FALSE(state_.selection_stale.load());
  ASSERT_EQ(broadcasts_.size(), 1u);
  EXPECT_NE(broadcasts_[0].find("refresh"), std::string::npos);
}

TEST_F(TriggerActionTest, ThrowingActionIsContained)
{
  auto root = triggerAction(R"({"name":"Explode"})");
  EXPECT_EQ(root.at("ok").as_int64(), 0);
  EXPECT_EQ(root.at("error").as_string(), "action exploded");
  EXPECT_TRUE(broadcasts_.empty());
}

TEST_F(TriggerActionTest, UnknownSuppressedAndReservedNamesAreRefused)
{
  auto root = triggerAction(R"({"name":"Nope"})");
  EXPECT_EQ(root.at("ok").as_int64(), 0);
  EXPECT_NE(std::string(root.at("error").as_string()).find("no longer"),
            std::string::npos);

  root = triggerAction(R"({"name":"Insert Buffer"})");
  EXPECT_EQ(root.at("ok").as_int64(), 0);
  EXPECT_NE(std::string(root.at("error").as_string()).find("not available"),
            std::string::npos);

  root = triggerAction(R"({"name":"deselect"})");
  EXPECT_EQ(root.at("ok").as_int64(), 0);
  EXPECT_TRUE(broadcasts_.empty());
}

TEST_F(TriggerActionTest, StaleSelectionIsDroppedBeforeUse)
{
  // Simulate a destroy from another session / a Tcl command.
  state_.selection_stale = true;

  // set_property refuses with a specific reason.
  auto root = setProperty(R"({"name":"Name","value":"x"})");
  EXPECT_EQ(root.at("ok").as_int64(), 0);
  EXPECT_NE(std::string(root.at("error").as_string()).find("invalidated"),
            std::string::npos);
  {
    std::lock_guard<std::mutex> lock(state_.selection_mutex);
    EXPECT_FALSE(state_.current_inspected);
  }

  // Inspect of the (now cleared) state degrades to the empty payload.
  state_.selection_stale = true;
  WebSocketRequest req;
  req.id = 81;
  req.type = WebSocketRequest::kInspect;
  req.json = parseObj(R"({"select_id":-1})");
  root = parseObj(payloadStr(handler_->handleInspect(req, state_)));
  EXPECT_TRUE(root.if_contains("error"));
  EXPECT_FALSE(state_.selection_stale.load());
}

//------------------------------------------------------------------------------
// Highlight-group tests (16 color-coded groups, Qt GUI parity)
//------------------------------------------------------------------------------

class HighlightGroupTest : public SelectHandlerTest
{
 protected:
  void SetUp() override
  {
    SelectHandlerTest::SetUp();
    std::lock_guard<std::mutex> lock(state_.selection_mutex);
    state_.current_inspected = makeFakeSelected(&fake_current_);
  }

  boost::json::object send(WebSocketRequest::Type type,
                           const std::string& request_json)
  {
    WebSocketRequest req;
    req.id = 90;
    req.type = type;
    req.json = parseObj(request_json);
    WebSocketResponse resp;
    switch (type) {
      case WebSocketRequest::kHighlight:
        resp = handler_->handleHighlight(req, state_);
        break;
      case WebSocketRequest::kUnhighlight:
        resp = handler_->handleUnhighlight(req, state_);
        break;
      default:
        resp = handler_->handleClearHighlights(req, state_);
        break;
    }
    EXPECT_EQ(resp.type, WebSocketResponse::kJson) << payloadStr(resp);
    return parseObj(payloadStr(resp));
  }

  size_t groupSize(int group)
  {
    std::lock_guard<std::mutex> lock(state_.selection_mutex);
    return state_.highlight_groups[group].size();
  }
};

TEST_F(HighlightGroupTest, HighlightAddsToGroupAndReportsIt)
{
  auto root = send(WebSocketRequest::kHighlight, R"({"group":4})");
  EXPECT_EQ(root.at("ok").as_int64(), 1);
  EXPECT_EQ(root.at("highlight_group").as_int64(), 4);
  EXPECT_EQ(groupSize(4), 1u);

  // Overlay shapes carry group 4's palette color (kRed, alpha 100).
  std::lock_guard<std::mutex> lock(state_.selection_mutex);
  ASSERT_EQ(state_.highlight_group_rects.size(), 1u);
  const auto& cr = state_.highlight_group_rects[0];
  EXPECT_TRUE(cr.filled);
  EXPECT_EQ(cr.rect, fake_current_.bbox);
  EXPECT_EQ(cr.color.r, 255);
  EXPECT_EQ(cr.color.g, 0);
  EXPECT_EQ(cr.color.b, 0);
  EXPECT_EQ(cr.color.a, 100);
}

TEST_F(HighlightGroupTest, HighlightCollectsGroupFlightLines)
{
  // A member whose highlight() draws lines (e.g. an unrouted net) must
  // still appear in the overlay, tinted with the group color.
  LineFakeDescriptor line_descriptor;
  {
    std::lock_guard<std::mutex> lock(state_.selection_mutex);
    state_.current_inspected = gui::Selected(&fake_current_, &line_descriptor);
  }
  auto root = send(WebSocketRequest::kHighlight, R"({"group":4})");
  EXPECT_EQ(root.at("ok").as_int64(), 1);

  std::lock_guard<std::mutex> lock(state_.selection_mutex);
  EXPECT_TRUE(state_.highlight_group_rects.empty());
  ASSERT_EQ(state_.highlight_group_lines.size(), 1u);
  const auto& line = state_.highlight_group_lines[0];
  EXPECT_EQ(line.p1, fake_current_.bbox.ll());
  EXPECT_EQ(line.p2, fake_current_.bbox.ur());
  EXPECT_EQ(line.color.r, 255);
  EXPECT_EQ(line.color.g, 0);
  EXPECT_EQ(line.color.b, 0);
  EXPECT_EQ(line.color.a, 100);
}

TEST_F(HighlightGroupTest, HighlightMovesBetweenGroupsUniquely)
{
  send(WebSocketRequest::kHighlight, R"({"group":2})");
  auto root = send(WebSocketRequest::kHighlight, R"({"group":7})");
  EXPECT_EQ(root.at("highlight_group").as_int64(), 7);
  EXPECT_EQ(groupSize(2), 0u);
  EXPECT_EQ(groupSize(7), 1u);
}

TEST_F(HighlightGroupTest, UnhighlightRemovesFromAnyGroup)
{
  send(WebSocketRequest::kHighlight, R"({"group":3})");
  auto root = send(WebSocketRequest::kUnhighlight, R"({})");
  EXPECT_EQ(root.at("ok").as_int64(), 1);
  EXPECT_EQ(root.at("highlight_group").as_int64(), -1);
  EXPECT_EQ(groupSize(3), 0u);
  std::lock_guard<std::mutex> lock(state_.selection_mutex);
  EXPECT_TRUE(state_.highlight_group_rects.empty());
}

TEST_F(HighlightGroupTest, InvalidGroupIsRejected)
{
  auto root = send(WebSocketRequest::kHighlight, R"({"group":16})");
  EXPECT_EQ(root.at("ok").as_int64(), 0);
  EXPECT_NE(std::string(root.at("error").as_string()).find("invalid"),
            std::string::npos);
  root = send(WebSocketRequest::kHighlight, R"({"group":-1})");
  EXPECT_EQ(root.at("ok").as_int64(), 0);

  root = send(WebSocketRequest::kClearHighlights, R"({"group":16})");
  EXPECT_EQ(root.at("ok").as_int64(), 0);
}

TEST_F(HighlightGroupTest, HighlightWithNothingInspectedFails)
{
  {
    std::lock_guard<std::mutex> lock(state_.selection_mutex);
    state_.current_inspected = gui::Selected();
  }
  auto root = send(WebSocketRequest::kHighlight, R"({"group":0})");
  EXPECT_EQ(root.at("ok").as_int64(), 0);
  EXPECT_NE(std::string(root.at("error").as_string()).find("nothing"),
            std::string::npos);
}

TEST_F(HighlightGroupTest, ClearOneGroupOrAll)
{
  send(WebSocketRequest::kHighlight, R"({"group":2})");
  {
    // A second object in another group, inserted directly.
    std::lock_guard<std::mutex> lock(state_.selection_mutex);
    state_.highlight_groups[5].insert(makeFakeSelected(&fake_previous_));
  }

  auto root = send(WebSocketRequest::kClearHighlights, R"({"group":2})");
  EXPECT_EQ(root.at("ok").as_int64(), 1);
  EXPECT_EQ(root.at("cleared").as_int64(), 1);
  EXPECT_EQ(groupSize(2), 0u);
  EXPECT_EQ(groupSize(5), 1u);

  // Default group (-1) clears everything left.
  root = send(WebSocketRequest::kClearHighlights, R"({})");
  EXPECT_EQ(root.at("cleared").as_int64(), 1);
  EXPECT_EQ(groupSize(5), 0u);
  std::lock_guard<std::mutex> lock(state_.selection_mutex);
  EXPECT_TRUE(state_.highlight_group_rects.empty());
}

TEST_F(HighlightGroupTest, InspectPayloadCarriesHighlightGroup)
{
  send(WebSocketRequest::kHighlight, R"({"group":9})");

  WebSocketRequest req;
  req.id = 91;
  req.type = WebSocketRequest::kInspect;
  req.json = parseObj(R"({"select_id":-1})");
  auto root = parseObj(payloadStr(handler_->handleInspect(req, state_)));
  EXPECT_EQ(root.at("highlight_group").as_int64(), 9);
}

TEST_F(HighlightGroupTest, OverlayRendersGroupShapes)
{
  send(WebSocketRequest::kHighlight, R"({"group":0})");

  TileHandler tile_handler(gen_);
  WebSocketRequest req;
  req.id = 92;
  req.type = WebSocketRequest::kOverlayTile;
  req.json = parseObj(R"({"z":0,"x":0,"y":0})");
  auto resp = tile_handler.handleOverlayTile(req, state_);
  EXPECT_EQ(resp.type, WebSocketResponse::kPng);
  EXPECT_FALSE(resp.payload.empty());
}

TEST_F(HighlightGroupTest, StalenessClearsHighlightGroups)
{
  send(WebSocketRequest::kHighlight, R"({"group":6})");
  state_.selection_stale = true;

  WebSocketRequest req;
  req.id = 93;
  req.type = WebSocketRequest::kInspect;
  req.json = parseObj(R"({"select_id":-1})");
  handler_->handleInspect(req, state_);

  EXPECT_EQ(groupSize(6), 0u);
  std::lock_guard<std::mutex> lock(state_.selection_mutex);
  EXPECT_TRUE(state_.highlight_group_rects.empty());
}

//------------------------------------------------------------------------------
// Selection-browser tests (list_selection / inspect_* / deselect)
//------------------------------------------------------------------------------

class SelectionBrowserTest : public HighlightGroupTest
{
 protected:
  boost::json::object listSelection()
  {
    WebSocketRequest req;
    req.id = 95;
    req.type = WebSocketRequest::kListSelection;
    req.json = parseObj(R"({})");
    auto resp = handler_->handleListSelection(req, state_);
    EXPECT_EQ(resp.type, WebSocketResponse::kJson) << payloadStr(resp);
    return parseObj(payloadStr(resp));
  }
};

TEST_F(SelectionBrowserTest, ListEmptyState)
{
  {
    std::lock_guard<std::mutex> lock(state_.selection_mutex);
    state_.current_inspected = gui::Selected();
  }
  auto root = listSelection();
  EXPECT_EQ(root.at("ok").as_int64(), 1);
  EXPECT_EQ(root.at("selection").as_array().size(), 0u);
  ASSERT_EQ(root.at("groups").as_array().size(),
            static_cast<size_t>(gui::kNumHighlightSet));
  EXPECT_FALSE(root.at("truncated").as_bool());
}

TEST_F(SelectionBrowserTest, ListSelectionAndGroups)
{
  populateSelectionSet(state_,
                       makeFakeSelected(&fake_current_),
                       makeFakeSelected(&fake_previous_),
                       0);
  {
    // populateSelectionSet points current_inspected at whichever object
    // sorts first (pointer order); pin it so the highlight is
    // deterministic.
    std::lock_guard<std::mutex> lock(state_.selection_mutex);
    state_.current_inspected = makeFakeSelected(&fake_current_);
  }
  // fake_current_ into group 4 via the real handler.
  send(WebSocketRequest::kHighlight, R"({"group":4})");

  auto root = listSelection();
  const auto& selection = root.at("selection").as_array();
  ASSERT_EQ(selection.size(), 2u);
  std::set<std::string> names;
  std::set<std::string> types;
  for (const auto& row : selection) {
    names.emplace(row.as_object().at("name").as_string());
    types.emplace(row.as_object().at("type").as_string());
    EXPECT_TRUE(row.as_object().if_contains("bbox"));
  }
  EXPECT_EQ(names, (std::set<std::string>{"current", "previous"}));
  EXPECT_EQ(types, (std::set<std::string>{"FakeCurrent", "FakePrevious"}));

  const auto& groups = root.at("groups").as_array();
  ASSERT_EQ(groups[4].as_array().size(), 1u);
  EXPECT_EQ(groups[4].as_array()[0].as_object().at("name").as_string(),
            "current");
  EXPECT_EQ(groups[0].as_array().size(), 0u);
}

TEST_F(SelectionBrowserTest, InspectSelectionRowByIndex)
{
  populateSelectionSet(state_,
                       makeFakeSelected(&fake_current_),
                       makeFakeSelected(&fake_previous_),
                       0);

  WebSocketRequest req;
  req.id = 96;
  req.type = WebSocketRequest::kInspectSelection;
  req.json = parseObj(R"({"index":1})");
  auto root
      = parseObj(payloadStr(handler_->handleInspectSelection(req, state_)));
  EXPECT_EQ(root.at("ok").as_int64(), 1);
  // Row 1 = second element in set order; verify state followed the payload.
  {
    std::lock_guard<std::mutex> lock(state_.selection_mutex);
    EXPECT_EQ(std::string(root.at("name").as_string()),
              state_.current_inspected.getName());
    EXPECT_TRUE(state_.selection_itr != state_.selection_set.end());
  }
  EXPECT_EQ(root.at("selection_index").as_int64(), 1);
  EXPECT_EQ(root.at("selection_count").as_int64(), 2);

  req.json = parseObj(R"({"index":7})");
  root = parseObj(payloadStr(handler_->handleInspectSelection(req, state_)));
  EXPECT_EQ(root.at("ok").as_int64(), 0);
  EXPECT_NE(std::string(root.at("error").as_string()).find("row index"),
            std::string::npos);
}

TEST_F(SelectionBrowserTest, InspectGroupRowByIndex)
{
  send(WebSocketRequest::kHighlight, R"({"group":9})");
  {
    // Move inspection elsewhere so the row click has to switch it back.
    std::lock_guard<std::mutex> lock(state_.selection_mutex);
    state_.current_inspected = makeFakeSelected(&fake_previous_);
  }

  WebSocketRequest req;
  req.id = 97;
  req.type = WebSocketRequest::kInspectGroup;
  req.json = parseObj(R"({"group":9,"index":0})");
  auto root = parseObj(payloadStr(handler_->handleInspectGroup(req, state_)));
  EXPECT_EQ(root.at("ok").as_int64(), 1);
  EXPECT_EQ(root.at("name").as_string(), "current");
  EXPECT_EQ(root.at("highlight_group").as_int64(), 9);

  req.json = parseObj(R"({"group":16,"index":0})");
  root = parseObj(payloadStr(handler_->handleInspectGroup(req, state_)));
  EXPECT_EQ(root.at("ok").as_int64(), 0);

  req.json = parseObj(R"({"group":9,"index":3})");
  root = parseObj(payloadStr(handler_->handleInspectGroup(req, state_)));
  EXPECT_EQ(root.at("ok").as_int64(), 0);
}

TEST_F(SelectionBrowserTest, DeselectRemovesRowAndRederivesHighlights)
{
  populateSelectionSet(state_,
                       makeFakeSelected(&fake_current_),
                       makeFakeSelected(&fake_previous_),
                       1);

  WebSocketRequest req;
  req.id = 98;
  req.type = WebSocketRequest::kDeselect;
  req.json = parseObj(R"({"index":0})");
  auto root = parseObj(payloadStr(handler_->handleDeselect(req, state_)));
  EXPECT_EQ(root.at("ok").as_int64(), 1);
  EXPECT_EQ(root.at("selection_count").as_int64(), 1);
  {
    std::lock_guard<std::mutex> lock(state_.selection_mutex);
    EXPECT_EQ(state_.selection_set.size(), 1u);
    EXPECT_TRUE(state_.selection_itr != state_.selection_set.end());
    // Multi-highlight shapes re-derived from the remaining member.
    EXPECT_EQ(state_.highlight_rects.size(), 1u);
  }

  req.json = parseObj(R"({"index":5})");
  root = parseObj(payloadStr(handler_->handleDeselect(req, state_)));
  EXPECT_EQ(root.at("ok").as_int64(), 0);
}

TEST_F(SelectionBrowserTest, StalenessClearsBeforeListing)
{
  populateSelectionSet(state_,
                       makeFakeSelected(&fake_current_),
                       makeFakeSelected(&fake_previous_),
                       0);
  send(WebSocketRequest::kHighlight, R"({"group":2})");
  state_.selection_stale = true;

  auto root = listSelection();
  EXPECT_EQ(root.at("ok").as_int64(), 1);
  EXPECT_EQ(root.at("selection").as_array().size(), 0u);
  EXPECT_EQ(root.at("groups").as_array()[2].as_array().size(), 0u);
}

//------------------------------------------------------------------------------
// DRCHandler tests
//------------------------------------------------------------------------------

class DRCHandlerTest : public tst::Nangate45Fixture
{
 protected:
  void SetUp() override
  {
    block_->setDieArea(odb::Rect(0, 0, 100000, 100000));
    block_->setCoreArea(odb::Rect(0, 0, 100000, 100000));
    gen_ = std::make_shared<TileGenerator>(
        getDb(), /*sta=*/nullptr, getLogger());
    handler_ = std::make_unique<DRCHandler>(gen_);
  }

  // Create a simple DRC category with markers for testing.
  odb::dbMarkerCategory* createTestCategory(const char* name, int num_markers)
  {
    auto* top = odb::dbMarkerCategory::create(chip_, name);
    auto* sub = odb::dbMarkerCategory::create(top, "MinSpacing");
    for (int i = 0; i < num_markers; ++i) {
      auto* marker = odb::dbMarker::create(sub);
      marker->addShape(odb::Rect(i * 1000, 0, i * 1000 + 500, 500));
    }
    return top;
  }

  odb::dbMarkerCategory* createDirectMarkerCategory(const char* name,
                                                    int num_markers)
  {
    auto* top = odb::dbMarkerCategory::create(chip_, name);
    for (int i = 0; i < num_markers; ++i) {
      auto* marker = odb::dbMarker::create(top);
      marker->addShape(odb::Rect(i * 1000, 0, i * 1000 + 500, 500));
    }
    return top;
  }

  std::shared_ptr<TileGenerator> gen_;
  std::unique_ptr<DRCHandler> handler_;
  SessionState state_;
};

TEST_F(DRCHandlerTest, CategoriesEmpty)
{
  WebSocketRequest req;
  req.id = 100;
  req.type = WebSocketRequest::kDrcCategories;

  auto resp = handler_->handleDRCCategories(req);
  EXPECT_EQ(resp.id, 100u);
  EXPECT_EQ(resp.type, WebSocketResponse::kJson);

  std::string json = payloadStr(resp);
  EXPECT_NE(json.find("\"categories\""), std::string::npos);
  // Should be an empty array
  EXPECT_NE(json.find("\"categories\":[]"), std::string::npos);
}

TEST_F(DRCHandlerTest, CategoriesWithMarkers)
{
  createTestCategory("DRC", 3);
  createTestCategory("LVS", 2);

  WebSocketRequest req;
  req.id = 101;
  req.type = WebSocketRequest::kDrcCategories;

  auto resp = handler_->handleDRCCategories(req);
  EXPECT_EQ(resp.type, WebSocketResponse::kJson);

  std::string json = payloadStr(resp);
  EXPECT_NE(json.find("\"DRC\""), std::string::npos);
  EXPECT_NE(json.find("\"LVS\""), std::string::npos);
}

TEST_F(DRCHandlerTest, MarkersForCategory)
{
  createTestCategory("DRC", 3);

  WebSocketRequest req;
  req.id = 102;
  req.type = WebSocketRequest::kDrcMarkers;
  req.json = parseObj(R"({"category":"DRC"})");

  auto resp = handler_->handleDRCMarkers(req, state_);
  EXPECT_EQ(resp.type, WebSocketResponse::kJson);

  std::string json = payloadStr(resp);
  EXPECT_NE(json.find("\"MinSpacing\""), std::string::npos);
  EXPECT_NE(json.find("\"markers\""), std::string::npos);
  // Should have set active category
  EXPECT_EQ(state_.active_drc_category, "DRC");
}

TEST_F(DRCHandlerTest, MarkersDirectlyUnderCategory)
{
  createDirectMarkerCategory("DRC", 2);

  WebSocketRequest req;
  req.id = 109;
  req.type = WebSocketRequest::kDrcMarkers;
  req.json = parseObj(R"({"category":"DRC"})");

  auto resp = handler_->handleDRCMarkers(req, state_);
  EXPECT_EQ(resp.type, WebSocketResponse::kJson);

  std::string json = payloadStr(resp);
  EXPECT_NE(json.find("\"name\":\"DRC\""), std::string::npos);
  EXPECT_NE(json.find("\"markers\""), std::string::npos);
  EXPECT_NE(json.find("\"total_count\":2"), std::string::npos);
}

TEST_F(DRCHandlerTest, MarkersForEmptyCategory)
{
  WebSocketRequest req;
  req.id = 103;
  req.type = WebSocketRequest::kDrcMarkers;
  req.json = parseObj(R"({"category":""})");

  auto resp = handler_->handleDRCMarkers(req, state_);
  EXPECT_EQ(resp.type, WebSocketResponse::kJson);

  std::string json = payloadStr(resp);
  EXPECT_NE(json.find("\"subcategories\":[]"), std::string::npos);
}

TEST_F(DRCHandlerTest, MarkersForNonExistentCategory)
{
  WebSocketRequest req;
  req.id = 104;
  req.type = WebSocketRequest::kDrcMarkers;
  req.json = parseObj(R"({"category":"NonExistent"})");

  auto resp = handler_->handleDRCMarkers(req, state_);
  EXPECT_EQ(resp.type, WebSocketResponse::kJson);

  std::string json = payloadStr(resp);
  EXPECT_NE(json.find("\"error\""), std::string::npos);
}

TEST_F(DRCHandlerTest, UpdateMarkerVisited)
{
  auto* top = createTestCategory("DRC", 1);
  auto all_markers = top->getAllMarkers();
  ASSERT_EQ(all_markers.size(), 1u);
  odb::dbMarker* marker = *all_markers.begin();
  EXPECT_FALSE(marker->isVisited());

  // First select the category
  {
    WebSocketRequest cat_req;
    cat_req.type = WebSocketRequest::kDrcMarkers;
    cat_req.json = parseObj(R"({"category":"DRC"})");
    handler_->handleDRCMarkers(cat_req, state_);
  }

  WebSocketRequest req;
  req.id = 105;
  req.type = WebSocketRequest::kDrcUpdateMarker;
  req.json = parseObj(R"({"marker_id":)" + std::to_string(marker->getId())
                      + R"(,"field":"visited","value":true})");

  auto resp = handler_->handleDRCUpdateMarker(req, state_);
  EXPECT_EQ(resp.type, WebSocketResponse::kJson);

  std::string json = payloadStr(resp);
  EXPECT_NE(json.find("\"ok\":1"), std::string::npos);
  EXPECT_TRUE(marker->isVisited());
}

TEST_F(DRCHandlerTest, UpdateMarkerVisible)
{
  auto* top = createTestCategory("DRC", 1);
  auto all_markers = top->getAllMarkers();
  odb::dbMarker* marker = *all_markers.begin();
  EXPECT_TRUE(marker->isVisible());  // default is visible

  // First select the category
  {
    WebSocketRequest cat_req;
    cat_req.type = WebSocketRequest::kDrcMarkers;
    cat_req.json = parseObj(R"({"category":"DRC"})");
    handler_->handleDRCMarkers(cat_req, state_);
  }

  WebSocketRequest req;
  req.id = 106;
  req.type = WebSocketRequest::kDrcUpdateMarker;
  req.json = parseObj(R"({"marker_id":)" + std::to_string(marker->getId())
                      + R"(,"field":"visible","value":false})");

  auto resp = handler_->handleDRCUpdateMarker(req, state_);
  EXPECT_EQ(resp.type, WebSocketResponse::kJson);
  EXPECT_FALSE(marker->isVisible());

  // DRC overlay should now be empty since the only marker is hidden
  std::lock_guard<std::mutex> lock(state_.drc_mutex);
  EXPECT_TRUE(state_.drc_rects.empty());
}

TEST_F(DRCHandlerTest, HighlightMarker)
{
  auto* top = createTestCategory("DRC", 1);
  auto all_markers = top->getAllMarkers();
  odb::dbMarker* marker = *all_markers.begin();

  // First select the category
  {
    WebSocketRequest cat_req;
    cat_req.type = WebSocketRequest::kDrcMarkers;
    cat_req.json = parseObj(R"({"category":"DRC"})");
    handler_->handleDRCMarkers(cat_req, state_);
  }

  WebSocketRequest req;
  req.id = 107;
  req.type = WebSocketRequest::kDrcHighlight;
  req.json
      = parseObj(R"({"marker_id":)" + std::to_string(marker->getId()) + "}");

  auto resp = handler_->handleDRCHighlight(req, state_);
  EXPECT_EQ(resp.type, WebSocketResponse::kJson);

  std::string json = payloadStr(resp);
  EXPECT_NE(json.find("\"ok\":1"), std::string::npos);
  EXPECT_NE(json.find("\"bbox\""), std::string::npos);

  // Marker should now be visited
  EXPECT_TRUE(marker->isVisited());

  // Highlight rects should contain the marker bbox
  std::lock_guard<std::mutex> lock(state_.selection_mutex);
  EXPECT_EQ(state_.highlight_rects.size(), 1u);
}

TEST_F(DRCHandlerTest, HighlightClear)
{
  // Put some existing highlights
  {
    std::lock_guard<std::mutex> lock(state_.selection_mutex);
    state_.highlight_rects.emplace_back(0, 0, 100, 100);
  }

  WebSocketRequest req;
  req.id = 108;
  req.type = WebSocketRequest::kDrcHighlight;
  req.json = parseObj(R"({"marker_id":-1})");

  auto resp = handler_->handleDRCHighlight(req, state_);
  EXPECT_EQ(resp.type, WebSocketResponse::kJson);

  std::string json = payloadStr(resp);
  EXPECT_NE(json.find("\"ok\":0"), std::string::npos);

  // Highlights should be cleared
  std::lock_guard<std::mutex> lock(state_.selection_mutex);
  EXPECT_TRUE(state_.highlight_rects.empty());
}

TEST_F(DRCHandlerTest, SelectCategoryStartsWithEmptyOverlay)
{
  auto* top = createTestCategory("DRC", 3);

  WebSocketRequest req;
  req.type = WebSocketRequest::kDrcMarkers;
  req.json = parseObj(R"({"category":"DRC"})");
  handler_->handleDRCMarkers(req, state_);

  // Selecting a category clears all markers' visibility — the user must
  // explicitly check individual markers (or use the batch toggle) to see them.
  for (odb::dbMarker* m : top->getAllMarkers()) {
    EXPECT_FALSE(m->isVisible());
  }
  std::lock_guard<std::mutex> lock(state_.drc_mutex);
  EXPECT_TRUE(state_.drc_rects.empty());
}

TEST_F(DRCHandlerTest, SelectEmptyCategoryClearsOverlay)
{
  createTestCategory("DRC", 3);

  // Select category first
  {
    WebSocketRequest req;
    req.type = WebSocketRequest::kDrcMarkers;
    req.json = parseObj(R"({"category":"DRC"})");
    handler_->handleDRCMarkers(req, state_);
  }

  // Now deselect
  {
    WebSocketRequest req;
    req.type = WebSocketRequest::kDrcMarkers;
    req.json = parseObj(R"({"category":""})");
    handler_->handleDRCMarkers(req, state_);
  }

  std::lock_guard<std::mutex> lock(state_.drc_mutex);
  EXPECT_TRUE(state_.drc_rects.empty());
  EXPECT_TRUE(state_.active_drc_category.empty());
}

TEST_F(DRCHandlerTest, UpdateCategoryVisibilityBatch)
{
  auto* top = createTestCategory("DRC", 3);

  // Select category — handleDRCMarkers starts every marker invisible.
  {
    WebSocketRequest req;
    req.type = WebSocketRequest::kDrcMarkers;
    req.json = parseObj(R"({"category":"DRC"})");
    handler_->handleDRCMarkers(req, state_);
  }

  // Overlay starts empty (all markers invisible by design).
  {
    std::lock_guard<std::mutex> lock(state_.drc_mutex);
    EXPECT_TRUE(state_.drc_rects.empty());
  }
  for (odb::dbMarker* m : top->getAllMarkers()) {
    EXPECT_FALSE(m->isVisible());
  }

  // Show all markers in one batch request
  {
    WebSocketRequest req;
    req.id = 200;
    req.type = WebSocketRequest::kDrcUpdateCategoryVisibility;
    req.json = parseObj(R"({"category":"DRC","visible":true})");

    auto resp = handler_->handleDRCUpdateCategoryVisibility(req, state_);
    EXPECT_EQ(resp.type, WebSocketResponse::kJson);

    std::string json = payloadStr(resp);
    EXPECT_NE(json.find("\"ok\":1"), std::string::npos);
    EXPECT_NE(json.find("\"count\":3"), std::string::npos);
  }

  // All markers should now be visible
  for (odb::dbMarker* m : top->getAllMarkers()) {
    EXPECT_TRUE(m->isVisible());
  }

  // Overlay should hold all 3 rects
  {
    std::lock_guard<std::mutex> lock(state_.drc_mutex);
    EXPECT_EQ(state_.drc_rects.size(), 3u);
  }

  // Hide them again
  {
    WebSocketRequest req;
    req.id = 201;
    req.type = WebSocketRequest::kDrcUpdateCategoryVisibility;
    req.json = parseObj(R"({"category":"DRC","visible":false})");

    auto resp = handler_->handleDRCUpdateCategoryVisibility(req, state_);
    EXPECT_EQ(resp.type, WebSocketResponse::kJson);
  }

  for (odb::dbMarker* m : top->getAllMarkers()) {
    EXPECT_FALSE(m->isVisible());
  }

  {
    std::lock_guard<std::mutex> lock(state_.drc_mutex);
    EXPECT_TRUE(state_.drc_rects.empty());
  }
}

//------------------------------------------------------------------------------
// Schematic handler tests — verify leaf cells are classified into standard
// logic-gate schematic symbols (Yosys primitives understood by netlistsvg)
// instead of anonymous boxes.
//------------------------------------------------------------------------------

class SchematicHandlerTest : public tst::Nangate45Fixture
{
 protected:
  void SetUp() override
  {
    readLiberty("_main/test/Nangate45/Nangate45_typ.lib");
    block_->setDieArea(odb::Rect(0, 0, 100000, 100000));
    sta_->postReadDef(block_);
    sta_->getDbNetwork()->setBlock(block_);

    gen_ = std::make_shared<TileGenerator>(getDb(), getSta(), getLogger());
    tcl_eval_ = std::make_shared<TclEvaluator>(/*interp=*/nullptr, getLogger());
    handler_ = std::make_unique<SelectHandler>(gen_, tcl_eval_);
  }

  // Instantiate a gate, wiring its named pins to fresh nets so the cell shows
  // up with connections in the schematic JSON.
  void makeGate(const char* master_name,
                const char* inst_name,
                const std::vector<tst::InstOptions::ITermInfo>& iterms)
  {
    odb::dbMaster* master = lib_->findMaster(master_name);
    ASSERT_NE(master, nullptr) << master_name;
    tst::InstOptions opts;
    opts.iterms = iterms;
    makeInst(block_, master, inst_name, opts);
  }

  // Returns modules.top.cells from a schematic_full response.
  boost::json::object fullCells()
  {
    WebSocketRequest req;
    req.id = 1;
    req.type = WebSocketRequest::kSchematicFull;
    req.json = parseObj("{}");
    auto resp = handler_->handleSchematicFull(req);
    EXPECT_EQ(resp.type, WebSocketResponse::kJson) << payloadStr(resp);
    return boost::json::parse(payloadStr(resp))
        .as_object()
        .at("modules")
        .as_object()
        .at("top")
        .as_object()
        .at("cells")
        .as_object();
  }

  std::shared_ptr<TileGenerator> gen_;
  std::shared_ptr<TclEvaluator> tcl_eval_;
  std::unique_ptr<SelectHandler> handler_;
};

TEST_F(SchematicHandlerTest, LeafGatesGetKindHint)
{
  makeGate("BUF_X1", "g_buf", {{"i", "A"}, {"o", "Z"}});
  makeGate("INV_X1", "g_inv", {{"i", "A"}, {"o", "ZN"}});
  makeGate("AND2_X1", "g_and", {{"a", "A1"}, {"b", "A2"}, {"o", "ZN"}});
  makeGate("OR2_X1", "g_or", {{"a", "A1"}, {"b", "A2"}, {"o", "ZN"}});
  makeGate("NAND2_X1", "g_nand", {{"a", "A1"}, {"b", "A2"}, {"o", "ZN"}});
  makeGate("NOR2_X1", "g_nor", {{"a", "A1"}, {"b", "A2"}, {"o", "ZN"}});
  makeGate("XOR2_X1", "g_xor", {{"a", "A"}, {"b", "B"}, {"o", "Z"}});
  makeGate("XNOR2_X1", "g_xnor", {{"a", "A"}, {"b", "B"}, {"o", "ZN"}});

  boost::json::object cells = fullCells();

  auto kind = [&](const char* inst) {
    return std::string(cells.at(inst).as_object().at("gate_kind").as_string());
  };
  EXPECT_EQ(kind("g_buf"), "buf");
  EXPECT_EQ(kind("g_inv"), "not");
  EXPECT_EQ(kind("g_and"), "and");
  EXPECT_EQ(kind("g_or"), "or");
  EXPECT_EQ(kind("g_nand"), "nand");
  EXPECT_EQ(kind("g_nor"), "nor");
  EXPECT_EQ(kind("g_xor"), "xor");
  EXPECT_EQ(kind("g_xnor"), "xnor");
}

TEST_F(SchematicHandlerTest, MultiInputGatesGetKindHint)
{
  // Gates with more than two inputs (a flat AND/OR of ports) classify as the
  // basic kind; the viewer derives the input count from the ports, so no
  // gate_terms are emitted (those are only for compound AOI/OAI).
  makeGate("NAND3_X1",
           "g_nand3",
           {{"a", "A1"}, {"b", "A2"}, {"c", "A3"}, {"o", "ZN"}});
  makeGate("NAND4_X1",
           "g_nand4",
           {{"a", "A1"}, {"b", "A2"}, {"c", "A3"}, {"d", "A4"}, {"o", "ZN"}});
  makeGate("AND3_X1",
           "g_and3",
           {{"a", "A1"}, {"b", "A2"}, {"c", "A3"}, {"o", "ZN"}});
  makeGate("NOR4_X1",
           "g_nor4",
           {{"a", "A1"}, {"b", "A2"}, {"c", "A3"}, {"d", "A4"}, {"o", "ZN"}});

  boost::json::object cells = fullCells();
  auto& nand3 = cells.at("g_nand3").as_object();
  EXPECT_EQ(std::string(nand3.at("gate_kind").as_string()), "nand");
  EXPECT_FALSE(nand3.contains("gate_terms"));
  EXPECT_EQ(
      std::string(cells.at("g_nand4").as_object().at("gate_kind").as_string()),
      "nand");
  EXPECT_EQ(
      std::string(cells.at("g_and3").as_object().at("gate_kind").as_string()),
      "and");
  EXPECT_EQ(
      std::string(cells.at("g_nor4").as_object().at("gate_kind").as_string()),
      "nor");
}

TEST_F(SchematicHandlerTest, GateKeepsMasterNameAndRealPins)
{
  makeGate("AND2_X1", "g_and", {{"a", "A1"}, {"b", "A2"}, {"o", "ZN"}});

  boost::json::object cells = fullCells();
  auto& cell = cells.at("g_and").as_object();

  // The hint must not replace the real type or pin names — netlistsvg still
  // needs them to draw the instance and port labels.
  EXPECT_EQ(std::string(cell.at("type").as_string()), "AND2_X1");

  auto& conns = cell.at("connections").as_object();
  EXPECT_TRUE(conns.contains("A1"));
  EXPECT_TRUE(conns.contains("A2"));
  EXPECT_TRUE(conns.contains("ZN"));

  auto& dirs = cell.at("port_directions").as_object();
  EXPECT_EQ(std::string(dirs.at("A1").as_string()), "input");
  EXPECT_EQ(std::string(dirs.at("ZN").as_string()), "output");
}

TEST_F(SchematicHandlerTest, AoiOaiGatesGetKindAndTerms)
{
  // AOI/OAI cells classify as compound gates with first-level term sizes.
  makeGate("AOI21_X1", "g_aoi21", {{"a", "A"}, {"b", "B1"}, {"c", "B2"}});
  makeGate("AOI22_X1",
           "g_aoi22",
           {{"a", "A1"}, {"b", "A2"}, {"c", "B1"}, {"d", "B2"}});
  makeGate("OAI21_X1", "g_oai21", {{"a", "A"}, {"b", "B1"}, {"c", "B2"}});
  makeGate("AOI211_X1",
           "g_aoi211",
           {{"a", "A"}, {"b", "B"}, {"c", "C1"}, {"d", "C2"}});

  boost::json::object cells = fullCells();

  // gate_terms groups the input pin names by first-level term; check the kind
  // and the per-term sizes.
  auto check =
      [&](const char* inst, const char* kind, std::vector<int> want_sizes) {
        auto& cell = cells.at(inst).as_object();
        EXPECT_EQ(std::string(cell.at("gate_kind").as_string()), kind) << inst;
        auto& terms = cell.at("gate_terms").as_array();
        std::vector<int> got;
        for (auto& t : terms) {
          got.push_back(static_cast<int>(t.as_array().size()));
        }
        std::sort(got.begin(), got.end());
        std::sort(want_sizes.begin(), want_sizes.end());
        EXPECT_EQ(got, want_sizes) << inst;
      };

  check("g_aoi21", "aoi", {2, 1});
  check("g_aoi22", "aoi", {2, 2});
  check("g_oai21", "oai", {2, 1});
  check("g_aoi211", "aoi", {2, 1, 1});

  // The groups carry the real input pin names, which the viewer uses to align
  // each input to its port.  AOI21 = !(A | (B1 & B2)).
  std::set<std::string> pins;
  for (auto& t : cells.at("g_aoi21").as_object().at("gate_terms").as_array()) {
    for (auto& p : t.as_array()) {
      pins.insert(std::string(p.as_string()));
    }
  }
  EXPECT_EQ(pins, (std::set<std::string>{"A", "B1", "B2"}));
}

TEST_F(SchematicHandlerTest, AoiGateKeepsMasterNameAndRealPins)
{
  makeGate("AOI21_X1", "g_aoi", {{"a", "A"}, {"b", "B1"}, {"c", "B2"}});

  boost::json::object cells = fullCells();
  auto& cell = cells.at("g_aoi").as_object();
  // The hint must not replace the real type or pin names.
  EXPECT_EQ(std::string(cell.at("type").as_string()), "AOI21_X1");
  auto& conns = cell.at("connections").as_object();
  EXPECT_TRUE(conns.contains("A"));
  EXPECT_TRUE(conns.contains("B1"));
  EXPECT_TRUE(conns.contains("B2"));
}

TEST_F(SchematicHandlerTest, NestedInversionStillClassifies)
{
  // Higher drive-strength variants can model the output with stacked inverters,
  // so the Liberty function is nested NOTs, e.g. AOI211_X4 is
  // !(!(!(((C1 & C2) | B) | A))).  Classification must peel all the inversions
  // (odd count -> inverting) and still recognise the AOI211, not fall back to a
  // box like a single-peel would.
  makeGate("AOI211_X4",
           "g_aoi211x4",
           {{"a", "C1"}, {"b", "C2"}, {"c", "B"}, {"d", "A"}});

  boost::json::object cells = fullCells();
  auto& cell = cells.at("g_aoi211x4").as_object();
  EXPECT_EQ(std::string(cell.at("gate_kind").as_string()), "aoi");

  std::vector<int> got;
  for (auto& t : cell.at("gate_terms").as_array()) {
    got.push_back(static_cast<int>(t.as_array().size()));
  }
  std::sort(got.begin(), got.end());
  EXPECT_EQ(got, (std::vector<int>{1, 1, 2}));
}

TEST_F(SchematicHandlerTest, DanglingPortsStillEmitted)
{
  // An instance whose pins are all unconnected must still emit every port with
  // a connection bit, so netlistsvg draws the full symbol (with dangling stubs)
  // instead of collapsing the cell to a bare name label with no shape.  The
  // synthetic bits stand in for the missing nets and must be distinct.
  makeGate("AND2_X1", "g_dangling", {});

  boost::json::object cells = fullCells();
  auto& cell = cells.at("g_dangling").as_object();

  // The gate is still classified from its Liberty function regardless of
  // connectivity.
  EXPECT_EQ(std::string(cell.at("gate_kind").as_string()), "and");

  auto& dirs = cell.at("port_directions").as_object();
  EXPECT_EQ(std::string(dirs.at("A1").as_string()), "input");
  EXPECT_EQ(std::string(dirs.at("A2").as_string()), "input");
  EXPECT_EQ(std::string(dirs.at("ZN").as_string()), "output");

  auto& conns = cell.at("connections").as_object();
  ASSERT_TRUE(conns.contains("A1"));
  ASSERT_TRUE(conns.contains("A2"));
  ASSERT_TRUE(conns.contains("ZN"));

  // Power/ground pins must not leak into the schematic as dangling stubs; only
  // the three signal pins are emitted.
  EXPECT_FALSE(dirs.contains("VDD"));
  EXPECT_FALSE(dirs.contains("VSS"));
  EXPECT_EQ(dirs.size(), 3u);
  EXPECT_EQ(conns.size(), 3u);

  // Each dangling pin gets its own synthetic bit id (no two pins share a net).
  std::set<int64_t> bits;
  for (const char* pin : {"A1", "A2", "ZN"}) {
    auto& arr = conns.at(pin).as_array();
    ASSERT_EQ(arr.size(), 1u) << pin;
    bits.insert(arr.at(0).as_int64());
  }
  EXPECT_EQ(bits.size(), 3u);
}

TEST_F(SchematicHandlerTest, MuxAndUnknownCellsHaveNoKindHint)
{
  // A MUX is not an AOI/OAI (its terms contain an inverted select), so it stays
  // a plain box.
  makeGate("MUX2_X1", "g_mux", {{"a", "A"}, {"b", "B"}, {"s", "S"}});

  boost::json::object cells = fullCells();
  auto& cell = cells.at("g_mux").as_object();
  EXPECT_EQ(std::string(cell.at("type").as_string()), "MUX2_X1");
  EXPECT_FALSE(cell.contains("gate_kind"));
}

// ─── Render cancellation (issue #10463 perf) ─────────────────────────────

TEST_F(TileHandlerTest, CancelSkipsQueuedTileRender)
{
  // Client abandons request id=99 (it panned/zoomed away).
  WebSocketRequest cancel;
  cancel.id = 200;
  cancel.type = WebSocketRequest::kCancel;
  cancel.json = parseObj(R"({"cancel_id":99})");
  auto ack = handler_->handleCancel(cancel, state_);
  EXPECT_EQ(ack.type, WebSocketResponse::kJson);

  // The matching queued tile render is skipped (best-effort).
  WebSocketRequest tile;
  tile.id = 99;
  tile.type = WebSocketRequest::kTile;
  tile.json
      = parseObj(R"({"layer":"metal1","z":0,"x":0,"y":0,"visible_layers":[]})");
  auto resp = handler_->handleTile(tile, state_);
  EXPECT_EQ(resp.type, WebSocketResponse::kError);

  // The cancellation is consumed: re-issuing the same id now renders.
  auto resp2 = handler_->handleTile(tile, state_);
  EXPECT_EQ(resp2.type, WebSocketResponse::kPng);
}

TEST_F(TileHandlerTest, CancelIdsArrayMarksAll)
{
  WebSocketRequest cancel;
  cancel.id = 201;
  cancel.type = WebSocketRequest::kCancel;
  cancel.json = parseObj(R"({"cancel_ids":[5,6]})");
  handler_->handleCancel(cancel, state_);
  std::lock_guard<std::mutex> lock(state_.cancelled_mutex);
  EXPECT_EQ(state_.cancelled_ids.count(5), 1u);
  EXPECT_EQ(state_.cancelled_ids.count(6), 1u);
}

// Regression: the inspect payload's `bbox` must be in ROOT/world coordinates.
//
// A gui::Descriptor reports a bbox in the object's own block coordinates, while
// selectAt already transforms SelectionResult::bbox into world space.  The
// client draws its selection outline from the payload's `bbox`, so if that one
// stays block-local, every object inside a translated dbChipInst is outlined at
// the untransformed location — off by exactly the chiplet offset.
TEST_F(SelectHandlerTest, InspectBboxIsInWorldCoordinatesForAChiplet)
{
  // handleSelect resolves the picked hit through the descriptor registry, which
  // nothing else in this binary populates.  Scoped so the process-global
  // registry is left as it was found even if an assertion below fires.  The
  // registry holds descriptors by unique_ptr, so this must be heap-allocated.
  struct ScopedInstDescriptor
  {
    ScopedInstDescriptor() : registry(gui::DescriptorRegistry::instance())
    {
      registry->registerDescriptor<odb::dbInst*>(new LocalBBoxInstDescriptor);
    }
    ~ScopedInstDescriptor() { registry->unregisterDescriptor<odb::dbInst*>(); }
    gui::DescriptorRegistry* registry;
  } scoped_inst_descriptor;

  // Re-root the design under a HIER chip holding one instance of the fixture's
  // chip, translated far enough that a block-local bbox cannot be mistaken for
  // a world one.
  constexpr int kOffsetX = 400000;
  constexpr int kOffsetY = 250000;
  odb::dbChip* master = getDb()->getChip();
  ASSERT_NE(master, nullptr);
  odb::dbChip* root = odb::dbChip::create(
      getDb(), /*tech=*/nullptr, "root", odb::dbChip::ChipType::HIER);
  odb::dbChipInst* chiplet = odb::dbChipInst::create(root, master, "die0");
  chiplet->setLoc(odb::Point3D(kOffsetX, kOffsetY, 0));
  getDb()->setTopChip(root);

  // buf1 sits at the origin of its own block, so in world space it sits at the
  // chiplet offset.
  odb::dbInst* buf1 = block_->findInst("buf1");
  ASSERT_NE(buf1, nullptr);
  const odb::Rect local = buf1->getBBox()->getBox();

  WebSocketRequest req;
  req.id = 77;
  req.type = WebSocketRequest::kSelect;
  req.json = parseObj(R"({"dbu_x":401000,"dbu_y":251000,"zoom":0,)"
                      R"("visible_layers":[]})");
  auto resp = handler_->handleSelect(req, state_);
  ASSERT_EQ(resp.type, WebSocketResponse::kJson);

  const boost::json::object root_obj = parseObj(payloadStr(resp));
  const auto& selected = root_obj.at("selected").as_array();
  ASSERT_FALSE(selected.empty()) << payloadStr(resp);
  ASSERT_TRUE(root_obj.contains("bbox")) << payloadStr(resp);

  const auto& bbox = root_obj.at("bbox").as_array();
  ASSERT_EQ(bbox.size(), 4u);
  const odb::Rect payload_bbox(bbox[0].to_number<int>(),
                               bbox[1].to_number<int>(),
                               bbox[2].to_number<int>(),
                               bbox[3].to_number<int>());

  // Translated by the chiplet offset, not left in block-local space.
  EXPECT_EQ(payload_bbox,
            odb::Rect(local.xMin() + kOffsetX,
                      local.yMin() + kOffsetY,
                      local.xMax() + kOffsetX,
                      local.yMax() + kOffsetY));
  EXPECT_NE(payload_bbox, local);

  // And it agrees with the world-space bbox selectAt reports for the same hit,
  // which is what the two used to disagree about.
  const auto& hit = selected[0].as_object().at("bbox").as_array();
  EXPECT_EQ(payload_bbox,
            odb::Rect(hit[0].to_number<int>(),
                      hit[1].to_number<int>(),
                      hit[2].to_number<int>(),
                      hit[3].to_number<int>()));
}

// ─── Custom UI registry (create_menu_item / create_toolbar_button) ───────────

// No design needed: the registry lives entirely in WebViewerHook and only
// uses a Logger to report duplicate-name errors.
class CustomUiTest : public ::testing::Test
{
 protected:
  utl::Logger* getLogger() { return &logger_; }
  boost::json::object registry(const WebViewerHook& hook)
  {
    return boost::json::parse(hook.customUiJson()).as_object();
  }

  utl::Logger logger_;
};

TEST_F(CustomUiTest, AutoGeneratesSequentialKeys)
{
  WebViewerHook hook;
  EXPECT_EQ(hook.addToolbarButton(
                getLogger(), "", "A", "sa", "", "", false, "", false),
            "button0");
  EXPECT_EQ(hook.addToolbarButton(
                getLogger(), "", "B", "sb", "", "", false, "", false),
            "button1");
  EXPECT_EQ(hook.addMenuItem(getLogger(), "", "", "M", "sm", "", false),
            "action0");
  EXPECT_EQ(hook.addMenuItem(getLogger(), "", "", "N", "sn", "", false),
            "action1");

  boost::json::object root = registry(hook);
  EXPECT_EQ(root.at("toolbar").as_array().size(), 2u);
  EXPECT_EQ(root.at("menu").as_array().size(), 2u);
}

TEST_F(CustomUiTest, DefaultMenuPathAndFieldsSerialized)
{
  WebViewerHook hook;
  hook.addMenuItem(getLogger(),
                   "m1",
                   /*path=*/"",
                   "Hello",
                   "puts hi",
                   "Ctrl+H",
                   /*echo=*/true);
  boost::json::object item
      = registry(hook).at("menu").as_array().at(0).as_object();
  EXPECT_EQ(std::string(item.at("key").as_string()), "m1");
  EXPECT_EQ(std::string(item.at("path").as_string()), "Custom Scripts");
  EXPECT_EQ(std::string(item.at("text").as_string()), "Hello");
  EXPECT_EQ(std::string(item.at("script").as_string()), "puts hi");
  EXPECT_EQ(std::string(item.at("shortcut").as_string()), "Ctrl+H");
  EXPECT_TRUE(item.at("echo").as_bool());
}

TEST_F(CustomUiTest, ToggleButtonFieldsSerialized)
{
  WebViewerHook hook;
  hook.addToolbarButton(getLogger(),
                        "freeze",
                        "Freeze",
                        "on",
                        "🔍",
                        "tip",
                        /*toggle=*/true,
                        /*script_off=*/"off",
                        /*echo=*/false);
  boost::json::object b
      = registry(hook).at("toolbar").as_array().at(0).as_object();
  EXPECT_EQ(std::string(b.at("icon").as_string()), "🔍");
  EXPECT_EQ(std::string(b.at("tooltip").as_string()), "tip");
  EXPECT_TRUE(b.at("toggle").as_bool());
  EXPECT_EQ(std::string(b.at("script_off").as_string()), "off");
}

TEST_F(CustomUiTest, DuplicateNameErrors)
{
  WebViewerHook hook;
  hook.addToolbarButton(getLogger(), "dup", "A", "s", "", "", false, "", false);
  EXPECT_THROW(hook.addToolbarButton(
                   getLogger(), "dup", "B", "s", "", "", false, "", false),
               std::runtime_error);

  hook.addMenuItem(getLogger(), "mdup", "", "A", "s", "", false);
  EXPECT_THROW(hook.addMenuItem(getLogger(), "mdup", "", "B", "s", "", false),
               std::runtime_error);
}

TEST_F(CustomUiTest, RemoveIsIdempotent)
{
  WebViewerHook hook;
  hook.addToolbarButton(getLogger(), "b", "A", "s", "", "", false, "", false);
  hook.addMenuItem(getLogger(), "m", "", "A", "s", "", false);

  hook.removeToolbarButton("nope");  // no-op, must not throw
  hook.removeMenuItem("nope");
  EXPECT_EQ(registry(hook).at("toolbar").as_array().size(), 1u);

  hook.removeToolbarButton("b");
  hook.removeMenuItem("m");
  boost::json::object root = registry(hook);
  EXPECT_EQ(root.at("toolbar").as_array().size(), 0u);
  EXPECT_EQ(root.at("menu").as_array().size(), 0u);
}

// ─── EditHandler: Global Connect + Insert Buffer ─────────────────────────────

class EditHandlerTest : public tst::Nangate45Fixture
{
 protected:
  void SetUp() override
  {
    block_->setDieArea(odb::Rect(0, 0, 100000, 100000));
    // No STA/liberty in this fixture, so buffer_info's master list is empty;
    // driver/load classification and the odb insert path still work.
    gen_ = std::make_shared<TileGenerator>(
        getDb(), /*sta=*/nullptr, getLogger());
    tcl_eval_ = std::make_shared<TclEvaluator>(/*interp=*/nullptr, getLogger());
    handler_ = std::make_unique<EditHandler>(gen_, tcl_eval_);
  }

  odb::dbInst* placeInst(const char* master_name, const char* inst_name)
  {
    odb::dbMaster* master = lib_->findMaster(master_name);
    EXPECT_NE(master, nullptr) << master_name;
    odb::dbInst* inst = odb::dbInst::create(block_, master, inst_name);
    inst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
    return inst;
  }

  // Build a net driven by drv/Z with one load ld/A.  Returns the net name.
  std::string makeSimpleNet(const char* name)
  {
    odb::dbInst* drv = placeInst("BUF_X1", "drv");
    odb::dbInst* ld = placeInst("BUF_X1", "ld");
    odb::dbNet* net = odb::dbNet::create(block_, name);
    drv->findITerm("Z")->connect(net);
    ld->findITerm("A")->connect(net);
    return name;
  }

  boost::json::object callObj(const WebSocketResponse& resp)
  {
    return parseObj(payloadStr(resp));
  }

  std::shared_ptr<TileGenerator> gen_;
  std::shared_ptr<TclEvaluator> tcl_eval_;
  std::unique_ptr<EditHandler> handler_;
};

TEST_F(EditHandlerTest, GlobalConnectInfoListsRulesNetsRegions)
{
  odb::dbNet* vdd = odb::dbNet::create(block_, "VDD");
  vdd->setSpecial();
  vdd->setSigType(odb::dbSigType::POWER);
  odb::dbGlobalConnect::create(vdd, nullptr, ".*", "^VDD$");

  WebSocketRequest req;
  req.json = parseObj("{}");
  boost::json::object root = callObj(handler_->handleGlobalConnectInfo(req));

  ASSERT_EQ(root.at("rules").as_array().size(), 1u);
  const auto& rule = root.at("rules").as_array().at(0).as_object();
  EXPECT_EQ(std::string(rule.at("net").as_string()), "VDD");
  EXPECT_EQ(std::string(rule.at("pin").as_string()), "^VDD$");
  // VDD is special → present in the nets list.
  bool found = false;
  for (const auto& n : root.at("nets").as_array()) {
    if (std::string(n.as_string()) == "VDD") {
      found = true;
    }
  }
  EXPECT_TRUE(found);
}

TEST_F(EditHandlerTest, GlobalConnectDeleteRemovesRuleByFields)
{
  odb::dbNet* vdd = odb::dbNet::create(block_, "VDD");
  vdd->setSpecial();
  odb::dbGlobalConnect::create(vdd, nullptr, ".*", "^VDD$");
  ASSERT_EQ(block_->getGlobalConnects().size(), 1u);

  // Match by fields (inst/pin/net/region), not a positional index.
  WebSocketRequest req;
  req.json = parseObj(R"({"inst":".*","pin":"^VDD$","net":"VDD","region":""})");
  boost::json::object root = callObj(handler_->handleGlobalConnectDelete(req));
  EXPECT_EQ(root.at("ok").as_int64(), 1);
  EXPECT_EQ(block_->getGlobalConnects().size(), 0u);
}

TEST_F(EditHandlerTest, GlobalConnectDeleteNoMatchIsNoop)
{
  odb::dbNet* vdd = odb::dbNet::create(block_, "VDD");
  vdd->setSpecial();
  odb::dbGlobalConnect::create(vdd, nullptr, ".*", "^VDD$");

  WebSocketRequest req;
  req.json
      = parseObj(R"({"inst":".*","pin":"^NOPE$","net":"VDD","region":""})");
  boost::json::object root = callObj(handler_->handleGlobalConnectDelete(req));
  EXPECT_EQ(root.at("ok").as_int64(), 0);
  EXPECT_EQ(block_->getGlobalConnects().size(), 1u);
}

TEST_F(EditHandlerTest, GlobalConnectApplyWithoutRulesReportsNoRules)
{
  // No rules defined: must report had_rules=false and avoid the ODB-0378
  // "Global connections are not set up." warning path being surfaced.
  WebSocketRequest req;
  req.json = parseObj(R"({"force":true})");
  boost::json::object root = callObj(handler_->handleGlobalConnectApply(req));
  EXPECT_TRUE(root.at("ok").as_bool());
  EXPECT_FALSE(root.at("had_rules").as_bool());
  EXPECT_EQ(root.at("connected").as_int64(), 0);
}

TEST_F(EditHandlerTest, GlobalConnectApplyWithRules)
{
  odb::dbNet* vdd = odb::dbNet::create(block_, "VDD");
  vdd->setSpecial();
  vdd->setSigType(odb::dbSigType::POWER);
  odb::dbGlobalConnect::create(vdd, nullptr, ".*", "^VDD$");

  WebSocketRequest req;
  req.json = parseObj(R"({"force":true})");
  boost::json::object root = callObj(handler_->handleGlobalConnectApply(req));
  EXPECT_TRUE(root.at("ok").as_bool());
  EXPECT_TRUE(root.at("had_rules").as_bool());
}

TEST_F(EditHandlerTest, BufferInfoClassifiesDriverAndLoad)
{
  makeSimpleNet("n1");
  WebSocketRequest req;
  req.json = parseObj(R"({"net":"n1"})");
  boost::json::object root = callObj(handler_->handleBufferInfo(req));

  EXPECT_TRUE(root.at("can_buffer").as_bool());
  ASSERT_EQ(root.at("drivers").as_array().size(), 1u);
  EXPECT_EQ(root.at("loads").as_array().size(), 1u);
  EXPECT_EQ(
      std::string(
          root.at("drivers").as_array().at(0).as_object().at("id").as_string()),
      "I:drv/Z");
}

TEST_F(EditHandlerTest, BufferInfoUnknownNet)
{
  WebSocketRequest req;
  req.json = parseObj(R"({"net":"nope"})");
  boost::json::object root = callObj(handler_->handleBufferInfo(req));
  EXPECT_FALSE(root.at("can_buffer").as_bool());
}

TEST_F(EditHandlerTest, InsertBufferAfterDriver)
{
  makeSimpleNet("n1");
  WebSocketRequest req;
  req.json = parseObj(
      R"({"net":"n1","mode":"driver","pins":["I:drv/Z"],"master":"BUF_X1"})");
  boost::json::object root = callObj(handler_->handleInsertBuffer(req));
  EXPECT_TRUE(root.at("ok").as_bool());
  EXPECT_FALSE(std::string(root.at("inst").as_string()).empty());
}

TEST_F(EditHandlerTest, InsertBufferUnknownMasterErrors)
{
  makeSimpleNet("n1");
  WebSocketRequest req;
  req.json = parseObj(
      R"({"net":"n1","mode":"driver","pins":["I:drv/Z"],"master":"NOPE"})");
  WebSocketResponse resp = handler_->handleInsertBuffer(req);
  EXPECT_EQ(resp.type, WebSocketResponse::kError);
}

TEST_F(EditHandlerTest, InsertBufferIsPlacedSoItRenders)
{
  makeSimpleNet("n1");
  WebSocketRequest req;
  req.json = parseObj(
      R"({"net":"n1","mode":"driver","pins":["I:drv/Z"],"master":"BUF_X1"})");
  boost::json::object root = callObj(handler_->handleInsertBuffer(req));
  ASSERT_TRUE(root.at("ok").as_bool());

  odb::dbInst* buf
      = block_->findInst(std::string(root.at("inst").as_string()).c_str());
  ASSERT_NE(buf, nullptr);
  // The buffer is placed at the pin so the web tile index (which only draws
  // placed instances) renders it; detailed_placement still legalizes it.
  EXPECT_TRUE(buf->isPlaced());
}

}  // namespace
}  // namespace web
