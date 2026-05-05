// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <any>
#include <boost/json/object.hpp>
#include <boost/json/parse.hpp>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>

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

TEST_F(TileHandlerTest, UsesHighlightState)
{
  // Put a highlight rect in the state
  {
    std::lock_guard<std::mutex> lock(state_.selection_mutex);
    state_.highlight_rects.emplace_back(0, 0, 50000, 50000);
  }

  WebSocketRequest req;
  req.id = 2;
  req.type = WebSocketRequest::kTile;
  req.json = parseObj(
      R"({"layer":"_instances","z":0,"x":0,"y":0,"visible_layers":[]})");

  // Should not crash and should return valid PNG
  auto resp = handler_->handleTile(req, state_);
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

TEST_F(DRCHandlerTest, DRCOverlayIncludesVisibleMarkers)
{
  createTestCategory("DRC", 3);

  WebSocketRequest req;
  req.type = WebSocketRequest::kDrcMarkers;
  req.json = parseObj(R"({"category":"DRC"})");
  handler_->handleDRCMarkers(req, state_);

  // All 3 markers should appear in the DRC overlay
  std::lock_guard<std::mutex> lock(state_.drc_mutex);
  EXPECT_EQ(state_.drc_rects.size(), 3u);
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

  // Select category to populate overlay
  {
    WebSocketRequest req;
    req.type = WebSocketRequest::kDrcMarkers;
    req.json = parseObj(R"({"category":"DRC"})");
    handler_->handleDRCMarkers(req, state_);
  }

  // All 3 markers should be in overlay
  {
    std::lock_guard<std::mutex> lock(state_.drc_mutex);
    EXPECT_EQ(state_.drc_rects.size(), 3u);
  }

  // Hide all markers in one batch request
  {
    WebSocketRequest req;
    req.id = 200;
    req.type = WebSocketRequest::kDrcUpdateCategoryVisibility;
    req.json = parseObj(R"({"category":"DRC","visible":false})");

    auto resp = handler_->handleDRCUpdateCategoryVisibility(req, state_);
    EXPECT_EQ(resp.type, WebSocketResponse::kJson);

    std::string json = payloadStr(resp);
    EXPECT_NE(json.find("\"ok\":1"), std::string::npos);
    EXPECT_NE(json.find("\"count\":3"), std::string::npos);
  }

  // All markers should now be hidden
  auto all_markers = top->getAllMarkers();
  for (odb::dbMarker* m : all_markers) {
    EXPECT_FALSE(m->isVisible());
  }

  // Overlay should be empty
  {
    std::lock_guard<std::mutex> lock(state_.drc_mutex);
    EXPECT_TRUE(state_.drc_rects.empty());
  }

  // Show them again
  {
    WebSocketRequest req;
    req.id = 201;
    req.type = WebSocketRequest::kDrcUpdateCategoryVisibility;
    req.json = parseObj(R"({"category":"DRC","visible":true})");

    auto resp = handler_->handleDRCUpdateCategoryVisibility(req, state_);
    EXPECT_EQ(resp.type, WebSocketResponse::kJson);
  }

  for (odb::dbMarker* m : top->getAllMarkers()) {
    EXPECT_TRUE(m->isVisible());
  }

  {
    std::lock_guard<std::mutex> lock(state_.drc_mutex);
    EXPECT_EQ(state_.drc_rects.size(), 3u);
  }
}

}  // namespace
}  // namespace web
