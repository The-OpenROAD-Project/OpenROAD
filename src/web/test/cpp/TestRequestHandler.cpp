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
#include <vector>

#include "boost/json/object.hpp"
#include "boost/json/parse.hpp"
#include "boost/json/serialize.hpp"
#include "gtest/gtest.h"
#include "gui/gui.h"
#include "gui/heatMap.h"
#include "odb/db.h"
#include "request_handler.h"
#include "tile_generator.h"
#include "tst/nangate45_fixture.h"

namespace web {
namespace {

struct FakeInspectable
{
  std::string name;
  std::string type;
  odb::Rect bbox;
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

  Properties getProperties(const std::any&) const override { return {}; }

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
    fake_current_
        = {.name = "current", .type = "FakeCurrent", .bbox = {0, 0, 100, 100}};
    fake_previous_ = {.name = "previous",
                      .type = "FakePrevious",
                      .bbox = {100, 100, 200, 200}};
    gen_ = std::make_shared<TileGenerator>(
        getDb(), /*sta=*/nullptr, getLogger());
    tcl_eval_ = std::make_shared<TclEvaluator>(/*interp=*/nullptr, getLogger());
    handler_ = std::make_unique<SelectHandler>(gen_, tcl_eval_);
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
                      return commit(value, [&] { std::any_cast<bool>(value); });
                    }));
    editors.emplace("Weight", makeEditor([this](const std::any& value) {
                      return commit(value,
                                    [&] { std::any_cast<double>(value); });
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
  EXPECT_TRUE(std::any_cast<bool>(editable_descriptor_.last_value));

  // JSON integers arrive at the callback as double (Qt delegate parity).
  setProperty(R"({"name":"Weight","value":5})");
  EXPECT_DOUBLE_EQ(std::any_cast<double>(editable_descriptor_.last_value), 5.0);
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

}  // namespace
}  // namespace web
