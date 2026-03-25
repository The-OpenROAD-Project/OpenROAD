// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <any>
#include <functional>
#include <memory>
#include <mutex>
#include <string>

#include "gtest/gtest.h"
#include "gui/gui.h"
#include "gui/heatMap.h"
#include "json_builder.h"
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

//------------------------------------------------------------------------------
// json_escape tests (no fixture needed)
//------------------------------------------------------------------------------

TEST(JsonEscapeTest, PlainString)
{
  EXPECT_EQ(json_escape("hello"), "hello");
}

TEST(JsonEscapeTest, EscapesSpecialChars)
{
  EXPECT_EQ(json_escape("a\"b"), "a\\\"b");
  EXPECT_EQ(json_escape("a\\b"), "a\\\\b");
  EXPECT_EQ(json_escape("a\nb"), "a\\nb");
  EXPECT_EQ(json_escape("a\tb"), "a\\tb");
  EXPECT_EQ(json_escape("a\rb"), "a\\rb");
}

TEST(JsonEscapeTest, ControlChars)
{
  std::string input(1, '\x01');
  EXPECT_EQ(json_escape(input), "\\u0001");
}

//------------------------------------------------------------------------------
// dispatch_request tests (BOUNDS, LAYERS, INFO)
//------------------------------------------------------------------------------

class DispatchRequestTest : public tst::Nangate45Fixture
{
 protected:
  void SetUp() override
  {
    block_->setDieArea(odb::Rect(0, 0, 100000, 100000));
    gen_ = std::make_shared<TileGenerator>(
        getDb(), /*sta=*/nullptr, getLogger());
  }

  std::shared_ptr<TileGenerator> gen_;
};

TEST_F(DispatchRequestTest, BoundsReturnsJson)
{
  WebSocketRequest req;
  req.id = 42;
  req.type = WebSocketRequest::BOUNDS;

  auto resp = dispatch_request(req, *gen_);
  EXPECT_EQ(resp.id, 42u);
  EXPECT_EQ(resp.type, 0);

  std::string json = payloadStr(resp);
  EXPECT_NE(json.find("\"bounds\""), std::string::npos);
}

TEST_F(DispatchRequestTest, TechReturnsJson)
{
  WebSocketRequest req;
  req.id = 7;
  req.type = WebSocketRequest::TECH;

  auto resp = dispatch_request(req, *gen_);
  EXPECT_EQ(resp.id, 7u);
  EXPECT_EQ(resp.type, 0);

  std::string json = payloadStr(resp);
  EXPECT_NE(json.find("\"layers\""), std::string::npos);
  EXPECT_NE(json.find("\"metal1\""), std::string::npos);
  EXPECT_NE(json.find("\"sites\""), std::string::npos);
  EXPECT_NE(json.find("\"has_liberty\""), std::string::npos);
}

TEST_F(DispatchRequestTest, TileReturnsPng)
{
  WebSocketRequest req;
  req.id = 99;
  req.type = WebSocketRequest::TILE;
  req.layer = "metal1";
  req.z = 0;
  req.x = 0;
  req.y = 0;

  auto resp = dispatch_request(req, *gen_);
  EXPECT_EQ(resp.id, 99u);
  EXPECT_EQ(resp.type, 1);  // PNG
  EXPECT_FALSE(resp.payload.empty());
  // PNG magic bytes
  EXPECT_GE(resp.payload.size(), 8u);
  EXPECT_EQ(resp.payload[0], 0x89);
  EXPECT_EQ(resp.payload[1], 'P');
  EXPECT_EQ(resp.payload[2], 'N');
  EXPECT_EQ(resp.payload[3], 'G');
}

TEST_F(DispatchRequestTest, UnknownTypeReturnsError)
{
  WebSocketRequest req;
  req.id = 5;
  req.type = WebSocketRequest::UNKNOWN;

  auto resp = dispatch_request(req, *gen_);
  EXPECT_EQ(resp.type, 2);  // error
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

TEST_F(TileHandlerTest, EmptyTile)
{
  WebSocketRequest req;
  req.id = 1;
  req.type = WebSocketRequest::TILE;
  req.layer = "metal1";
  req.z = 0;
  req.x = 0;
  req.y = 0;

  auto resp = handler_->handleTile(req, state_);
  EXPECT_EQ(resp.type, 1);  // PNG
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
  req.type = WebSocketRequest::TILE;
  req.layer = "_instances";
  req.z = 0;
  req.x = 0;
  req.y = 0;

  // Should not crash and should return valid PNG
  auto resp = handler_->handleTile(req, state_);
  EXPECT_EQ(resp.type, 1);
  EXPECT_FALSE(resp.payload.empty());
}

TEST_F(TileHandlerTest, HeatMapsReturnsMetadata)
{
  gui::registerBuiltinHeatMapSources(/*sta=*/nullptr, getLogger());
  handler_->initializeHeatMaps(state_);

  WebSocketRequest req;
  req.id = 3;
  req.type = WebSocketRequest::HEATMAPS;

  auto resp = handler_->handleHeatMaps(req, state_);
  EXPECT_EQ(resp.type, 0);
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
  active_req.type = WebSocketRequest::SET_ACTIVE_HEATMAP;
  active_req.heatmap_name = "Pin";

  EXPECT_EQ(handler_->handleSetActiveHeatMap(active_req, state1).type, 0);
  EXPECT_EQ(handler_->handleSetActiveHeatMap(active_req, state2).type, 0);

  WebSocketRequest set_req;
  set_req.id = 4;
  set_req.type = WebSocketRequest::SET_HEATMAP;
  set_req.heatmap_name = "Pin";
  set_req.heatmap_option = "DisplayMin";
  set_req.raw_json = R"({"value":12.5})";

  auto set_resp = handler_->handleSetHeatMap(set_req, state1);
  EXPECT_EQ(set_resp.type, 0);

  WebSocketRequest meta_req;
  meta_req.id = 5;
  meta_req.type = WebSocketRequest::HEATMAPS;

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
  set_req.type = WebSocketRequest::SET_HEATMAP;
  set_req.heatmap_name = "Pin";
  set_req.heatmap_option = "ShowNumbers";
  set_req.raw_json = R"({"value":true})";

  auto set_resp = handler_->handleSetHeatMap(set_req, state_);
  EXPECT_EQ(set_resp.type, 0);
  {
    std::lock_guard<std::mutex> lock(state_.heatmap_mutex);
    ASSERT_TRUE(state_.heatmaps.count("Pin"));
    EXPECT_TRUE(state_.heatmaps.at("Pin")->getShowNumbers());
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
  meta_req.type = WebSocketRequest::HEATMAPS;

  auto meta_resp = handler_->handleHeatMaps(meta_req, state_);
  EXPECT_EQ(meta_resp.type, 0);
  EXPECT_EQ(populate_calls, 0);

  WebSocketRequest active_req;
  active_req.id = 7;
  active_req.type = WebSocketRequest::SET_ACTIVE_HEATMAP;
  active_req.heatmap_name = "LazyMeta";

  auto active_resp = handler_->handleSetActiveHeatMap(active_req, state_);
  EXPECT_EQ(active_resp.type, 0);
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
    fake_current_ = {"current", "FakeCurrent", odb::Rect(0, 0, 100, 100)};
    fake_previous_
        = {"previous", "FakePrevious", odb::Rect(100, 100, 200, 200)};
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
  req.type = WebSocketRequest::SELECT;
  req.select_x = 1000;
  req.select_y = 1000;
  req.select_zoom = 0;

  auto resp = handler_->handleSelect(req, state_);
  EXPECT_EQ(resp.id, 10u);
  EXPECT_EQ(resp.type, 0);

  std::string json = payloadStr(resp);
  EXPECT_NE(json.find("\"selected\""), std::string::npos);
}

TEST_F(SelectHandlerTest, SelectAtEmptyAreaReturnsEmptyList)
{
  WebSocketRequest req;
  req.id = 11;
  req.type = WebSocketRequest::SELECT;
  req.select_x = 99000;
  req.select_y = 99000;
  req.select_zoom = 10;  // high zoom = small area

  auto resp = handler_->handleSelect(req, state_);
  EXPECT_EQ(resp.type, 0);

  std::string json = payloadStr(resp);
  EXPECT_NE(json.find("\"selected\": []"), std::string::npos);
}

TEST_F(SelectHandlerTest, InspectInvalidIdReturnsError)
{
  WebSocketRequest req;
  req.id = 12;
  req.type = WebSocketRequest::INSPECT;
  req.select_id = 999;  // no selectables stored

  auto resp = handler_->handleInspect(req, state_);
  EXPECT_EQ(resp.type, 0);

  std::string json = payloadStr(resp);
  EXPECT_NE(json.find("\"error\""), std::string::npos);
}

TEST_F(SelectHandlerTest, HoverInvalidIdReturnsOkZeroCount)
{
  WebSocketRequest req;
  req.id = 13;
  req.type = WebSocketRequest::HOVER;
  req.select_id = 999;

  auto resp = handler_->handleHover(req, state_);
  EXPECT_EQ(resp.type, 0);

  std::string json = payloadStr(resp);
  EXPECT_NE(json.find("\"ok\": 1"), std::string::npos);
  EXPECT_NE(json.find("\"count\": 0"), std::string::npos);
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
  req.type = WebSocketRequest::SELECT;
  req.select_x = 1000;
  req.select_y = 1000;
  req.select_zoom = 0;

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
  req.type = WebSocketRequest::SELECT;
  req.select_x = 99000;
  req.select_y = 99000;
  req.select_zoom = 10;

  auto resp = handler_->handleSelect(req, state_);
  EXPECT_EQ(resp.type, 0);

  const std::string json = payloadStr(resp);
  EXPECT_NE(json.find("\"can_navigate_back\": 0"), std::string::npos);
  EXPECT_NE(json.find("\"selected\": []"), std::string::npos);

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
  inspect_req.type = WebSocketRequest::INSPECT;
  inspect_req.select_id = 0;

  auto inspect_resp = handler_->handleInspect(inspect_req, state_);
  EXPECT_EQ(inspect_resp.type, 0);
  EXPECT_NE(payloadStr(inspect_resp).find("\"can_navigate_back\": 1"),
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
  back_req.type = WebSocketRequest::INSPECT_BACK;

  auto back_resp = handler_->handleInspectBack(back_req, state_);
  EXPECT_EQ(back_resp.type, 0);
  EXPECT_NE(payloadStr(back_resp).find("\"can_navigate_back\": 0"),
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
  back_req.type = WebSocketRequest::INSPECT_BACK;

  auto back_resp = handler_->handleInspectBack(back_req, state_);
  EXPECT_EQ(back_resp.type, 0);
  EXPECT_NE(payloadStr(back_resp).find("\"can_navigate_back\": 0"),
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
  req.type = WebSocketRequest::SET_FOCUS_NETS;
  req.focus_action = "add";
  req.focus_net_name = "clk";

  auto resp = handler_->handleSetFocusNets(req, state_);
  EXPECT_EQ(resp.id, 20u);
  EXPECT_EQ(resp.type, 0);

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
  req.type = WebSocketRequest::SET_FOCUS_NETS;
  req.focus_action = "add";
  req.focus_net_name = "nonexistent_net";

  auto resp = handler_->handleSetFocusNets(req, state_);
  EXPECT_EQ(resp.type, 0);

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
  req.type = WebSocketRequest::SET_FOCUS_NETS;
  req.focus_action = "remove";
  req.focus_net_name = "data";

  auto resp = handler_->handleSetFocusNets(req, state_);
  EXPECT_EQ(resp.type, 0);

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
  req.type = WebSocketRequest::SET_FOCUS_NETS;
  req.focus_action = "clear";
  req.focus_net_name = "";

  auto resp = handler_->handleSetFocusNets(req, state_);
  EXPECT_EQ(resp.type, 0);

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
  req1.type = WebSocketRequest::SET_FOCUS_NETS;
  req1.focus_action = "add";
  req1.focus_net_name = "clk";
  handler_->handleSetFocusNets(req1, state_);

  WebSocketRequest req2;
  req2.id = 25;
  req2.type = WebSocketRequest::SET_FOCUS_NETS;
  req2.focus_action = "add";
  req2.focus_net_name = "reset";
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
  req.type = WebSocketRequest::SET_FOCUS_NETS;
  req.focus_action = "add";
  req.focus_net_name = "clk";

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
  req.type = WebSocketRequest::TILE;
  req.layer = "metal1";
  req.z = 0;
  req.x = 0;
  req.y = 0;

  // Should not crash and should return valid PNG
  auto resp = tile_handler->handleTile(req, state_);
  EXPECT_EQ(resp.type, 1);  // PNG
  EXPECT_FALSE(resp.payload.empty());
}

}  // namespace
}  // namespace web
