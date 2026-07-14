// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <any>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>

#include "boost/json/array.hpp"
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

TEST_F(SelectHandlerTest, SchematicUsesLibertyInverterSymbol)
{
  readLiberty("_main/test/Nangate45/Nangate45_typ.lib");

  odb::dbInst* inverter = placeInst("INV_X1", "inv1", 1000, 0);
  odb::dbNet* input_net = odb::dbNet::create(block_, "in");
  odb::dbNet* output_net = odb::dbNet::create(block_, "out");
  ASSERT_NE(inverter->findITerm("A"), nullptr);
  ASSERT_NE(inverter->findITerm("ZN"), nullptr);
  inverter->findITerm("A")->connect(input_net);
  inverter->findITerm("ZN")->connect(output_net);

  gen_ = std::make_shared<TileGenerator>(getDb(), getSta(), getLogger());
  handler_ = std::make_unique<SelectHandler>(gen_, tcl_eval_);

  WebSocketRequest req;
  req.id = 11;
  req.json
      = parseObj(R"({"inst_name":"inv1","fanin_depth":0,"fanout_depth":0})");
  const WebSocketResponse resp = handler_->handleSchematicCone(req);

  ASSERT_EQ(resp.type, WebSocketResponse::kJson);
  const boost::json::object root
      = boost::json::parse(payloadStr(resp)).as_object();
  const boost::json::object& cell
      = root.at("modules").at("top").at("cells").at("inv1").as_object();
  EXPECT_EQ(cell.at("type").as_string(), "openroad_inverter");
  EXPECT_EQ(cell.at("attributes").at("ref").as_string(), "inv1");
  EXPECT_EQ(cell.at("attributes").at("openroad_master").as_string(), "INV_X1");
  EXPECT_EQ(cell.at("attributes").at("openroad_input_port").as_string(), "A");
  EXPECT_EQ(cell.at("attributes").at("openroad_output_port").as_string(), "ZN");
  EXPECT_EQ(cell.at("port_directions").as_object().at("A").as_string(),
            "input");
  EXPECT_EQ(cell.at("port_directions").as_object().at("Y").as_string(),
            "output");
  EXPECT_TRUE(cell.at("connections").as_object().contains("A"));
  EXPECT_TRUE(cell.at("connections").as_object().contains("Y"));
  EXPECT_FALSE(cell.at("connections").as_object().contains("ZN"));
}

TEST_F(SelectHandlerTest, SchematicUsesLibertyBufferSymbol)
{
  readLiberty("_main/test/Nangate45/Nangate45_typ.lib");

  odb::dbInst* buffer = block_->findInst("buf1");
  ASSERT_NE(buffer, nullptr);
  odb::dbNet* input_net = odb::dbNet::create(block_, "in");
  odb::dbNet* output_net = odb::dbNet::create(block_, "out");
  ASSERT_NE(buffer->findITerm("A"), nullptr);
  ASSERT_NE(buffer->findITerm("Z"), nullptr);
  buffer->findITerm("A")->connect(input_net);
  buffer->findITerm("Z")->connect(output_net);

  gen_ = std::make_shared<TileGenerator>(getDb(), getSta(), getLogger());
  handler_ = std::make_unique<SelectHandler>(gen_, tcl_eval_);

  WebSocketRequest req;
  req.id = 12;
  req.json
      = parseObj(R"({"inst_name":"buf1","fanin_depth":0,"fanout_depth":0})");
  const WebSocketResponse resp = handler_->handleSchematicCone(req);

  ASSERT_EQ(resp.type, WebSocketResponse::kJson);
  const boost::json::object root
      = boost::json::parse(payloadStr(resp)).as_object();
  const boost::json::object& cell
      = root.at("modules").at("top").at("cells").at("buf1").as_object();
  EXPECT_EQ(cell.at("type").as_string(), "openroad_buffer");
  EXPECT_EQ(cell.at("attributes").at("openroad_master").as_string(), "BUF_X16");
  EXPECT_EQ(cell.at("attributes").at("openroad_input_port").as_string(), "A");
  EXPECT_EQ(cell.at("attributes").at("openroad_output_port").as_string(), "Z");
  EXPECT_TRUE(cell.at("connections").as_object().contains("A"));
  EXPECT_TRUE(cell.at("connections").as_object().contains("Y"));
  EXPECT_FALSE(cell.at("connections").as_object().contains("Z"));
}

TEST_F(SelectHandlerTest, SchematicUsesLibertyDffSymbol)
{
  readLiberty("_main/test/Nangate45/Nangate45_typ.lib");

  odb::dbInst* dff = placeInst("DFF_X1", "dff1", 1000, 0);
  odb::dbNet* data_net = odb::dbNet::create(block_, "data");
  odb::dbNet* clock_net = odb::dbNet::create(block_, "clock");
  odb::dbNet* q_net = odb::dbNet::create(block_, "q");
  odb::dbNet* qn_net = odb::dbNet::create(block_, "qn");
  ASSERT_NE(dff->findITerm("D"), nullptr);
  ASSERT_NE(dff->findITerm("CK"), nullptr);
  ASSERT_NE(dff->findITerm("Q"), nullptr);
  ASSERT_NE(dff->findITerm("QN"), nullptr);
  dff->findITerm("D")->connect(data_net);
  dff->findITerm("CK")->connect(clock_net);
  dff->findITerm("Q")->connect(q_net);
  dff->findITerm("QN")->connect(qn_net);

  gen_ = std::make_shared<TileGenerator>(getDb(), getSta(), getLogger());
  handler_ = std::make_unique<SelectHandler>(gen_, tcl_eval_);

  WebSocketRequest req;
  req.id = 18;
  req.json
      = parseObj(R"({"inst_name":"dff1","fanin_depth":0,"fanout_depth":0})");
  const WebSocketResponse resp = handler_->handleSchematicCone(req);

  ASSERT_EQ(resp.type, WebSocketResponse::kJson);
  const boost::json::object root
      = boost::json::parse(payloadStr(resp)).as_object();
  const boost::json::object& cell
      = root.at("modules").at("top").at("cells").at("dff1").as_object();
  EXPECT_EQ(cell.at("type").as_string(), "DFF_X1");
  const boost::json::object& attributes = cell.at("attributes").as_object();
  EXPECT_EQ(attributes.at("openroad_symbol_type").as_string(), "openroad_dff");
  EXPECT_EQ(attributes.at("openroad_data_port").as_string(), "D");
  EXPECT_EQ(attributes.at("openroad_clock_port").as_string(), "CK");

  const boost::json::object& symbol_port_directions
      = attributes.at("openroad_symbol_port_directions").as_object();
  EXPECT_EQ(symbol_port_directions.at("D").as_string(), "input");
  EXPECT_EQ(symbol_port_directions.at("CK").as_string(), "input");
  EXPECT_EQ(symbol_port_directions.at("Q").as_string(), "output");
  EXPECT_EQ(symbol_port_directions.at("QN").as_string(), "output");
  const boost::json::object& symbol_connections
      = attributes.at("openroad_symbol_connections").as_object();
  EXPECT_TRUE(symbol_connections.contains("D"));
  EXPECT_TRUE(symbol_connections.contains("CK"));
  EXPECT_TRUE(symbol_connections.contains("Q"));
  EXPECT_TRUE(symbol_connections.contains("QN"));

  EXPECT_EQ(cell.at("port_directions").as_object().at("D").as_string(),
            "input");
  EXPECT_EQ(cell.at("port_directions").as_object().at("CK").as_string(),
            "input");
  EXPECT_EQ(cell.at("port_directions").as_object().at("Q").as_string(),
            "output");
  EXPECT_EQ(cell.at("port_directions").as_object().at("QN").as_string(),
            "output");
  EXPECT_TRUE(cell.at("connections").as_object().contains("D"));
  EXPECT_TRUE(cell.at("connections").as_object().contains("CK"));
  EXPECT_TRUE(cell.at("connections").as_object().contains("Q"));
  EXPECT_TRUE(cell.at("connections").as_object().contains("QN"));
}

TEST_F(SelectHandlerTest, SchematicUsesLibertyDffResetSymbol)
{
  readLiberty("_main/test/Nangate45/Nangate45_typ.lib");

  odb::dbInst* dff = placeInst("DFFR_X1", "dffr1", 1000, 0);
  odb::dbNet* data_net = odb::dbNet::create(block_, "data");
  odb::dbNet* clock_net = odb::dbNet::create(block_, "clock");
  odb::dbNet* reset_net = odb::dbNet::create(block_, "reset_n");
  odb::dbNet* q_net = odb::dbNet::create(block_, "q");
  odb::dbNet* qn_net = odb::dbNet::create(block_, "qn");
  ASSERT_NE(dff->findITerm("D"), nullptr);
  ASSERT_NE(dff->findITerm("CK"), nullptr);
  ASSERT_NE(dff->findITerm("RN"), nullptr);
  ASSERT_NE(dff->findITerm("Q"), nullptr);
  ASSERT_NE(dff->findITerm("QN"), nullptr);
  dff->findITerm("D")->connect(data_net);
  dff->findITerm("CK")->connect(clock_net);
  dff->findITerm("RN")->connect(reset_net);
  dff->findITerm("Q")->connect(q_net);
  dff->findITerm("QN")->connect(qn_net);

  gen_ = std::make_shared<TileGenerator>(getDb(), getSta(), getLogger());
  handler_ = std::make_unique<SelectHandler>(gen_, tcl_eval_);

  WebSocketRequest req;
  req.id = 19;
  req.json
      = parseObj(R"({"inst_name":"dffr1","fanin_depth":0,"fanout_depth":0})");
  const WebSocketResponse resp = handler_->handleSchematicCone(req);

  ASSERT_EQ(resp.type, WebSocketResponse::kJson);
  const boost::json::object root
      = boost::json::parse(payloadStr(resp)).as_object();
  const boost::json::object& cell
      = root.at("modules").at("top").at("cells").at("dffr1").as_object();
  EXPECT_EQ(cell.at("type").as_string(), "DFFR_X1");
  const boost::json::object& attributes = cell.at("attributes").as_object();
  EXPECT_EQ(attributes.at("openroad_symbol_type").as_string(), "openroad_dffr");
  EXPECT_EQ(attributes.at("openroad_data_port").as_string(), "D");
  EXPECT_EQ(attributes.at("openroad_clock_port").as_string(), "CK");
  EXPECT_EQ(attributes.at("openroad_clear_port").as_string(), "RN");

  const boost::json::object& symbol_port_directions
      = attributes.at("openroad_symbol_port_directions").as_object();
  EXPECT_EQ(symbol_port_directions.at("D").as_string(), "input");
  EXPECT_EQ(symbol_port_directions.at("CK").as_string(), "input");
  EXPECT_EQ(symbol_port_directions.at("RN").as_string(), "input");
  EXPECT_EQ(symbol_port_directions.at("Q").as_string(), "output");
  EXPECT_EQ(symbol_port_directions.at("QN").as_string(), "output");
  const boost::json::object& symbol_connections
      = attributes.at("openroad_symbol_connections").as_object();
  EXPECT_TRUE(symbol_connections.contains("D"));
  EXPECT_TRUE(symbol_connections.contains("CK"));
  EXPECT_TRUE(symbol_connections.contains("RN"));
  EXPECT_TRUE(symbol_connections.contains("Q"));
  EXPECT_TRUE(symbol_connections.contains("QN"));

  EXPECT_EQ(cell.at("port_directions").as_object().at("RN").as_string(),
            "input");
  EXPECT_TRUE(cell.at("connections").as_object().contains("RN"));
}

TEST_F(SelectHandlerTest, SchematicUsesLibertyDffSetSymbol)
{
  readLiberty("_main/test/Nangate45/Nangate45_typ.lib");

  odb::dbInst* dff = placeInst("DFFS_X1", "dffs1", 1000, 0);
  odb::dbNet* data_net = odb::dbNet::create(block_, "data");
  odb::dbNet* clock_net = odb::dbNet::create(block_, "clock");
  odb::dbNet* set_net = odb::dbNet::create(block_, "set_n");
  odb::dbNet* q_net = odb::dbNet::create(block_, "q");
  odb::dbNet* qn_net = odb::dbNet::create(block_, "qn");
  ASSERT_NE(dff->findITerm("D"), nullptr);
  ASSERT_NE(dff->findITerm("CK"), nullptr);
  ASSERT_NE(dff->findITerm("SN"), nullptr);
  ASSERT_NE(dff->findITerm("Q"), nullptr);
  ASSERT_NE(dff->findITerm("QN"), nullptr);
  dff->findITerm("D")->connect(data_net);
  dff->findITerm("CK")->connect(clock_net);
  dff->findITerm("SN")->connect(set_net);
  dff->findITerm("Q")->connect(q_net);
  dff->findITerm("QN")->connect(qn_net);

  gen_ = std::make_shared<TileGenerator>(getDb(), getSta(), getLogger());
  handler_ = std::make_unique<SelectHandler>(gen_, tcl_eval_);

  WebSocketRequest req;
  req.id = 20;
  req.json
      = parseObj(R"({"inst_name":"dffs1","fanin_depth":0,"fanout_depth":0})");
  const WebSocketResponse resp = handler_->handleSchematicCone(req);

  ASSERT_EQ(resp.type, WebSocketResponse::kJson);
  const boost::json::object root
      = boost::json::parse(payloadStr(resp)).as_object();
  const boost::json::object& cell
      = root.at("modules").at("top").at("cells").at("dffs1").as_object();
  EXPECT_EQ(cell.at("type").as_string(), "DFFS_X1");
  const boost::json::object& attributes = cell.at("attributes").as_object();
  EXPECT_EQ(attributes.at("openroad_symbol_type").as_string(), "openroad_dffs");
  EXPECT_EQ(attributes.at("openroad_data_port").as_string(), "D");
  EXPECT_EQ(attributes.at("openroad_clock_port").as_string(), "CK");
  EXPECT_EQ(attributes.at("openroad_preset_port").as_string(), "SN");

  const boost::json::object& symbol_port_directions
      = attributes.at("openroad_symbol_port_directions").as_object();
  EXPECT_EQ(symbol_port_directions.at("D").as_string(), "input");
  EXPECT_EQ(symbol_port_directions.at("CK").as_string(), "input");
  EXPECT_EQ(symbol_port_directions.at("SN").as_string(), "input");
  EXPECT_EQ(symbol_port_directions.at("Q").as_string(), "output");
  EXPECT_EQ(symbol_port_directions.at("QN").as_string(), "output");
  const boost::json::object& symbol_connections
      = attributes.at("openroad_symbol_connections").as_object();
  EXPECT_TRUE(symbol_connections.contains("D"));
  EXPECT_TRUE(symbol_connections.contains("CK"));
  EXPECT_TRUE(symbol_connections.contains("SN"));
  EXPECT_TRUE(symbol_connections.contains("Q"));
  EXPECT_TRUE(symbol_connections.contains("QN"));

  EXPECT_EQ(cell.at("port_directions").as_object().at("SN").as_string(),
            "input");
  EXPECT_TRUE(cell.at("connections").as_object().contains("SN"));
}

TEST_F(SelectHandlerTest, SchematicUsesLibertyAndSymbol)
{
  readLiberty("_main/test/Nangate45/Nangate45_typ.lib");

  odb::dbInst* and_gate = placeInst("AND2_X1", "and1", 1000, 0);
  odb::dbNet* input_net1 = odb::dbNet::create(block_, "in1");
  odb::dbNet* input_net2 = odb::dbNet::create(block_, "in2");
  odb::dbNet* output_net = odb::dbNet::create(block_, "out");
  ASSERT_NE(and_gate->findITerm("A1"), nullptr);
  ASSERT_NE(and_gate->findITerm("A2"), nullptr);
  ASSERT_NE(and_gate->findITerm("ZN"), nullptr);
  and_gate->findITerm("A1")->connect(input_net1);
  and_gate->findITerm("A2")->connect(input_net2);
  and_gate->findITerm("ZN")->connect(output_net);

  gen_ = std::make_shared<TileGenerator>(getDb(), getSta(), getLogger());
  handler_ = std::make_unique<SelectHandler>(gen_, tcl_eval_);

  WebSocketRequest req;
  req.id = 13;
  req.json
      = parseObj(R"({"inst_name":"and1","fanin_depth":0,"fanout_depth":0})");
  const WebSocketResponse resp = handler_->handleSchematicCone(req);

  ASSERT_EQ(resp.type, WebSocketResponse::kJson);
  const boost::json::object root
      = boost::json::parse(payloadStr(resp)).as_object();
  const boost::json::object& cell
      = root.at("modules").at("top").at("cells").at("and1").as_object();
  EXPECT_EQ(cell.at("type").as_string(), "openroad_and2");
  EXPECT_EQ(cell.at("attributes").at("ref").as_string(), "and1");
  EXPECT_EQ(cell.at("attributes").at("openroad_master").as_string(), "AND2_X1");
  const boost::json::array& input_ports
      = cell.at("attributes").at("openroad_input_ports").as_array();
  ASSERT_EQ(input_ports.size(), 2u);
  EXPECT_EQ(input_ports[0].as_string(), "A1");
  EXPECT_EQ(input_ports[1].as_string(), "A2");
  EXPECT_EQ(cell.at("attributes").at("openroad_output_port").as_string(), "ZN");
  EXPECT_EQ(cell.at("port_directions").as_object().at("A1").as_string(),
            "input");
  EXPECT_EQ(cell.at("port_directions").as_object().at("A2").as_string(),
            "input");
  EXPECT_EQ(cell.at("port_directions").as_object().at("Y").as_string(),
            "output");
  EXPECT_TRUE(cell.at("connections").as_object().contains("A1"));
  EXPECT_TRUE(cell.at("connections").as_object().contains("A2"));
  EXPECT_TRUE(cell.at("connections").as_object().contains("Y"));
  EXPECT_FALSE(cell.at("connections").as_object().contains("ZN"));
}

TEST_F(SelectHandlerTest, SchematicUsesLibertyNandSymbol)
{
  readLiberty("_main/test/Nangate45/Nangate45_typ.lib");

  odb::dbInst* nand_gate = placeInst("NAND2_X1", "nand1", 1000, 0);
  odb::dbNet* input_net1 = odb::dbNet::create(block_, "in1");
  odb::dbNet* input_net2 = odb::dbNet::create(block_, "in2");
  odb::dbNet* output_net = odb::dbNet::create(block_, "out");
  ASSERT_NE(nand_gate->findITerm("A1"), nullptr);
  ASSERT_NE(nand_gate->findITerm("A2"), nullptr);
  ASSERT_NE(nand_gate->findITerm("ZN"), nullptr);
  nand_gate->findITerm("A1")->connect(input_net1);
  nand_gate->findITerm("A2")->connect(input_net2);
  nand_gate->findITerm("ZN")->connect(output_net);

  gen_ = std::make_shared<TileGenerator>(getDb(), getSta(), getLogger());
  handler_ = std::make_unique<SelectHandler>(gen_, tcl_eval_);

  WebSocketRequest req;
  req.id = 14;
  req.json
      = parseObj(R"({"inst_name":"nand1","fanin_depth":0,"fanout_depth":0})");
  const WebSocketResponse resp = handler_->handleSchematicCone(req);

  ASSERT_EQ(resp.type, WebSocketResponse::kJson);
  const boost::json::object root
      = boost::json::parse(payloadStr(resp)).as_object();
  const boost::json::object& cell
      = root.at("modules").at("top").at("cells").at("nand1").as_object();
  EXPECT_EQ(cell.at("type").as_string(), "openroad_nand2");
  EXPECT_EQ(cell.at("attributes").at("openroad_master").as_string(),
            "NAND2_X1");
  const boost::json::array& input_ports
      = cell.at("attributes").at("openroad_input_ports").as_array();
  ASSERT_EQ(input_ports.size(), 2u);
  EXPECT_EQ(input_ports[0].as_string(), "A1");
  EXPECT_EQ(input_ports[1].as_string(), "A2");
  EXPECT_EQ(cell.at("attributes").at("openroad_output_port").as_string(), "ZN");
  EXPECT_TRUE(cell.at("connections").as_object().contains("A1"));
  EXPECT_TRUE(cell.at("connections").as_object().contains("A2"));
  EXPECT_TRUE(cell.at("connections").as_object().contains("Y"));
  EXPECT_FALSE(cell.at("connections").as_object().contains("ZN"));
}

TEST_F(SelectHandlerTest, SchematicUsesLibertyOrSymbol)
{
  readLiberty("_main/test/Nangate45/Nangate45_typ.lib");

  odb::dbInst* or_gate = placeInst("OR2_X1", "or1", 1000, 0);
  odb::dbNet* input_net1 = odb::dbNet::create(block_, "in1");
  odb::dbNet* input_net2 = odb::dbNet::create(block_, "in2");
  odb::dbNet* output_net = odb::dbNet::create(block_, "out");
  ASSERT_NE(or_gate->findITerm("A1"), nullptr);
  ASSERT_NE(or_gate->findITerm("A2"), nullptr);
  ASSERT_NE(or_gate->findITerm("ZN"), nullptr);
  or_gate->findITerm("A1")->connect(input_net1);
  or_gate->findITerm("A2")->connect(input_net2);
  or_gate->findITerm("ZN")->connect(output_net);

  gen_ = std::make_shared<TileGenerator>(getDb(), getSta(), getLogger());
  handler_ = std::make_unique<SelectHandler>(gen_, tcl_eval_);

  WebSocketRequest req;
  req.id = 15;
  req.json
      = parseObj(R"({"inst_name":"or1","fanin_depth":0,"fanout_depth":0})");
  const WebSocketResponse resp = handler_->handleSchematicCone(req);

  ASSERT_EQ(resp.type, WebSocketResponse::kJson);
  const boost::json::object root
      = boost::json::parse(payloadStr(resp)).as_object();
  const boost::json::object& cell
      = root.at("modules").at("top").at("cells").at("or1").as_object();
  EXPECT_EQ(cell.at("type").as_string(), "openroad_or2");
  EXPECT_EQ(cell.at("attributes").at("openroad_master").as_string(), "OR2_X1");
  const boost::json::array& input_ports
      = cell.at("attributes").at("openroad_input_ports").as_array();
  ASSERT_EQ(input_ports.size(), 2u);
  EXPECT_EQ(input_ports[0].as_string(), "A1");
  EXPECT_EQ(input_ports[1].as_string(), "A2");
  EXPECT_EQ(cell.at("attributes").at("openroad_output_port").as_string(), "ZN");
  EXPECT_EQ(cell.at("port_directions").as_object().at("A1").as_string(),
            "input");
  EXPECT_EQ(cell.at("port_directions").as_object().at("A2").as_string(),
            "input");
  EXPECT_EQ(cell.at("port_directions").as_object().at("Y").as_string(),
            "output");
  EXPECT_TRUE(cell.at("connections").as_object().contains("A1"));
  EXPECT_TRUE(cell.at("connections").as_object().contains("A2"));
  EXPECT_TRUE(cell.at("connections").as_object().contains("Y"));
  EXPECT_FALSE(cell.at("connections").as_object().contains("ZN"));
}

TEST_F(SelectHandlerTest, SchematicUsesLibertyXorSymbol)
{
  readLiberty("_main/test/Nangate45/Nangate45_typ.lib");

  odb::dbInst* xor_gate = placeInst("XOR2_X1", "xor1", 1000, 0);
  odb::dbNet* input_net1 = odb::dbNet::create(block_, "in1");
  odb::dbNet* input_net2 = odb::dbNet::create(block_, "in2");
  odb::dbNet* output_net = odb::dbNet::create(block_, "out");
  ASSERT_NE(xor_gate->findITerm("A"), nullptr);
  ASSERT_NE(xor_gate->findITerm("B"), nullptr);
  ASSERT_NE(xor_gate->findITerm("Z"), nullptr);
  xor_gate->findITerm("A")->connect(input_net1);
  xor_gate->findITerm("B")->connect(input_net2);
  xor_gate->findITerm("Z")->connect(output_net);

  gen_ = std::make_shared<TileGenerator>(getDb(), getSta(), getLogger());
  handler_ = std::make_unique<SelectHandler>(gen_, tcl_eval_);

  WebSocketRequest req;
  req.id = 16;
  req.json
      = parseObj(R"({"inst_name":"xor1","fanin_depth":0,"fanout_depth":0})");
  const WebSocketResponse resp = handler_->handleSchematicCone(req);

  ASSERT_EQ(resp.type, WebSocketResponse::kJson);
  const boost::json::object root
      = boost::json::parse(payloadStr(resp)).as_object();
  const boost::json::object& cell
      = root.at("modules").at("top").at("cells").at("xor1").as_object();
  EXPECT_EQ(cell.at("type").as_string(), "openroad_xor2");
  EXPECT_EQ(cell.at("attributes").at("openroad_master").as_string(), "XOR2_X1");
  const boost::json::array& input_ports
      = cell.at("attributes").at("openroad_input_ports").as_array();
  ASSERT_EQ(input_ports.size(), 2u);
  EXPECT_EQ(input_ports[0].as_string(), "A");
  EXPECT_EQ(input_ports[1].as_string(), "B");
  EXPECT_EQ(cell.at("attributes").at("openroad_output_port").as_string(), "Z");
  EXPECT_EQ(cell.at("port_directions").as_object().at("A1").as_string(),
            "input");
  EXPECT_EQ(cell.at("port_directions").as_object().at("A2").as_string(),
            "input");
  EXPECT_EQ(cell.at("port_directions").as_object().at("Y").as_string(),
            "output");
  EXPECT_TRUE(cell.at("connections").as_object().contains("A1"));
  EXPECT_TRUE(cell.at("connections").as_object().contains("A2"));
  EXPECT_TRUE(cell.at("connections").as_object().contains("Y"));
  EXPECT_FALSE(cell.at("connections").as_object().contains("A"));
  EXPECT_FALSE(cell.at("connections").as_object().contains("B"));
  EXPECT_FALSE(cell.at("connections").as_object().contains("Z"));
}

TEST_F(SelectHandlerTest, SchematicUsesLibertyXnorSymbol)
{
  readLiberty("_main/test/Nangate45/Nangate45_typ.lib");

  odb::dbInst* xnor_gate = placeInst("XNOR2_X1", "xnor1", 1000, 0);
  odb::dbNet* input_net1 = odb::dbNet::create(block_, "in1");
  odb::dbNet* input_net2 = odb::dbNet::create(block_, "in2");
  odb::dbNet* output_net = odb::dbNet::create(block_, "out");
  ASSERT_NE(xnor_gate->findITerm("A"), nullptr);
  ASSERT_NE(xnor_gate->findITerm("B"), nullptr);
  ASSERT_NE(xnor_gate->findITerm("ZN"), nullptr);
  xnor_gate->findITerm("A")->connect(input_net1);
  xnor_gate->findITerm("B")->connect(input_net2);
  xnor_gate->findITerm("ZN")->connect(output_net);

  gen_ = std::make_shared<TileGenerator>(getDb(), getSta(), getLogger());
  handler_ = std::make_unique<SelectHandler>(gen_, tcl_eval_);

  WebSocketRequest req;
  req.id = 17;
  req.json
      = parseObj(R"({"inst_name":"xnor1","fanin_depth":0,"fanout_depth":0})");
  const WebSocketResponse resp = handler_->handleSchematicCone(req);

  ASSERT_EQ(resp.type, WebSocketResponse::kJson);
  const boost::json::object root
      = boost::json::parse(payloadStr(resp)).as_object();
  const boost::json::object& cell
      = root.at("modules").at("top").at("cells").at("xnor1").as_object();
  EXPECT_EQ(cell.at("type").as_string(), "openroad_xnor2");
  EXPECT_EQ(cell.at("attributes").at("openroad_master").as_string(),
            "XNOR2_X1");
  const boost::json::array& input_ports
      = cell.at("attributes").at("openroad_input_ports").as_array();
  ASSERT_EQ(input_ports.size(), 2u);
  EXPECT_EQ(input_ports[0].as_string(), "A");
  EXPECT_EQ(input_ports[1].as_string(), "B");
  EXPECT_EQ(cell.at("attributes").at("openroad_output_port").as_string(), "ZN");
  EXPECT_EQ(cell.at("port_directions").as_object().at("A1").as_string(),
            "input");
  EXPECT_EQ(cell.at("port_directions").as_object().at("A2").as_string(),
            "input");
  EXPECT_EQ(cell.at("port_directions").as_object().at("Y").as_string(),
            "output");
  EXPECT_TRUE(cell.at("connections").as_object().contains("A1"));
  EXPECT_TRUE(cell.at("connections").as_object().contains("A2"));
  EXPECT_TRUE(cell.at("connections").as_object().contains("Y"));
  EXPECT_FALSE(cell.at("connections").as_object().contains("A"));
  EXPECT_FALSE(cell.at("connections").as_object().contains("B"));
  EXPECT_FALSE(cell.at("connections").as_object().contains("ZN"));
}

TEST_F(SelectHandlerTest, SchematicUsesLibertyNorSymbol)
{
  readLiberty("_main/test/Nangate45/Nangate45_typ.lib");

  odb::dbInst* nor_gate = placeInst("NOR2_X1", "nor1", 1000, 0);
  odb::dbNet* input_net1 = odb::dbNet::create(block_, "in1");
  odb::dbNet* input_net2 = odb::dbNet::create(block_, "in2");
  odb::dbNet* output_net = odb::dbNet::create(block_, "out");
  ASSERT_NE(nor_gate->findITerm("A1"), nullptr);
  ASSERT_NE(nor_gate->findITerm("A2"), nullptr);
  ASSERT_NE(nor_gate->findITerm("ZN"), nullptr);
  nor_gate->findITerm("A1")->connect(input_net1);
  nor_gate->findITerm("A2")->connect(input_net2);
  nor_gate->findITerm("ZN")->connect(output_net);

  gen_ = std::make_shared<TileGenerator>(getDb(), getSta(), getLogger());
  handler_ = std::make_unique<SelectHandler>(gen_, tcl_eval_);

  WebSocketRequest req;
  req.id = 16;
  req.json
      = parseObj(R"({"inst_name":"nor1","fanin_depth":0,"fanout_depth":0})");
  const WebSocketResponse resp = handler_->handleSchematicCone(req);

  ASSERT_EQ(resp.type, WebSocketResponse::kJson);
  const boost::json::object root
      = boost::json::parse(payloadStr(resp)).as_object();
  const boost::json::object& cell
      = root.at("modules").at("top").at("cells").at("nor1").as_object();
  EXPECT_EQ(cell.at("type").as_string(), "openroad_nor2");
  EXPECT_EQ(cell.at("attributes").at("openroad_master").as_string(), "NOR2_X1");
  const boost::json::array& input_ports
      = cell.at("attributes").at("openroad_input_ports").as_array();
  ASSERT_EQ(input_ports.size(), 2u);
  EXPECT_EQ(input_ports[0].as_string(), "A1");
  EXPECT_EQ(input_ports[1].as_string(), "A2");
  EXPECT_EQ(cell.at("attributes").at("openroad_output_port").as_string(), "ZN");
  EXPECT_TRUE(cell.at("connections").as_object().contains("A1"));
  EXPECT_TRUE(cell.at("connections").as_object().contains("A2"));
  EXPECT_TRUE(cell.at("connections").as_object().contains("Y"));
  EXPECT_FALSE(cell.at("connections").as_object().contains("ZN"));
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
