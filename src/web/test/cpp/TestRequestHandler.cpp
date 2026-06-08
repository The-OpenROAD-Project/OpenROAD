// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <any>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>

#include "boost/json/object.hpp"
#include "boost/json/parse.hpp"
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

}  // namespace
}  // namespace web
