// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "request_handler.h"

#include <string>

#include "gtest/gtest.h"
#include "odb/db.h"
#include "tst/nangate45_fixture.h"

namespace web {
namespace {

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

  auto resp = dispatch_request(req, gen_);
  EXPECT_EQ(resp.id, 42u);
  EXPECT_EQ(resp.type, 0);

  std::string json = payloadStr(resp);
  EXPECT_NE(json.find("\"bounds\""), std::string::npos);
}

TEST_F(DispatchRequestTest, LayersReturnsJson)
{
  WebSocketRequest req;
  req.id = 7;
  req.type = WebSocketRequest::LAYERS;

  auto resp = dispatch_request(req, gen_);
  EXPECT_EQ(resp.id, 7u);
  EXPECT_EQ(resp.type, 0);

  std::string json = payloadStr(resp);
  EXPECT_NE(json.find("\"layers\""), std::string::npos);
  EXPECT_NE(json.find("\"metal1\""), std::string::npos);
}

TEST_F(DispatchRequestTest, InfoWithoutSta)
{
  WebSocketRequest req;
  req.id = 1;
  req.type = WebSocketRequest::INFO;

  auto resp = dispatch_request(req, gen_);
  EXPECT_EQ(resp.type, 0);

  std::string json = payloadStr(resp);
  EXPECT_NE(json.find("\"has_liberty\": false"), std::string::npos);
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

  auto resp = dispatch_request(req, gen_);
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

  auto resp = dispatch_request(req, gen_);
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
    state_.highlight_rects.push_back(odb::Rect(0, 0, 50000, 50000));
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

//------------------------------------------------------------------------------
// SelectHandler tests
//------------------------------------------------------------------------------

class SelectHandlerTest : public tst::Nangate45Fixture
{
 protected:
  void SetUp() override
  {
    block_->setDieArea(odb::Rect(0, 0, 100000, 100000));
    placeInst("BUF_X16", "buf1", 0, 0);
    gen_ = std::make_shared<TileGenerator>(
        getDb(), /*sta=*/nullptr, getLogger());
    handler_ = std::make_unique<SelectHandler>(gen_);
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
  std::unique_ptr<SelectHandler> handler_;
  SessionState state_;
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

TEST_F(SelectHandlerTest, HoverInvalidIdReturnsError)
{
  WebSocketRequest req;
  req.id = 13;
  req.type = WebSocketRequest::HOVER;
  req.select_id = 999;

  auto resp = handler_->handleHover(req, state_);
  EXPECT_EQ(resp.type, 0);

  std::string json = payloadStr(resp);
  EXPECT_NE(json.find("\"error\""), std::string::npos);
}

TEST_F(SelectHandlerTest, SelectClearsTimingState)
{
  // Populate timing state
  {
    std::lock_guard<std::mutex> lock(state_.selection_mutex);
    state_.timing_rects.push_back(
        {odb::Rect(0, 0, 1, 1), {255, 0, 0, 255}, ""});
    state_.timing_lines.push_back(
        {odb::Point(0, 0), odb::Point(1, 1), {0, 255, 0, 255}});
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

}  // namespace
}  // namespace web
