// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <memory>
#include <mutex>
#include <string>

#include "clock_tree_report.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "request_handler.h"
#include "tile_generator.h"
#include "tst/nangate45_fixture.h"

namespace web {
namespace {

// Helper to extract payload as string.
std::string payloadStr(const WebSocketResponse& resp)
{
  return std::string(resp.payload.begin(), resp.payload.end());
}

//------------------------------------------------------------------------------
// ClockTreeNode::typeToString tests (no fixture needed)
//------------------------------------------------------------------------------

TEST(ClockTreeNodeTest, TypeToStringRoot)
{
  EXPECT_STREQ(ClockTreeNode::typeToString(ClockTreeNode::ROOT), "root");
}

TEST(ClockTreeNodeTest, TypeToStringBuffer)
{
  EXPECT_STREQ(ClockTreeNode::typeToString(ClockTreeNode::BUFFER), "buffer");
}

TEST(ClockTreeNodeTest, TypeToStringInverter)
{
  EXPECT_STREQ(ClockTreeNode::typeToString(ClockTreeNode::INVERTER),
               "inverter");
}

TEST(ClockTreeNodeTest, TypeToStringClockGate)
{
  EXPECT_STREQ(ClockTreeNode::typeToString(ClockTreeNode::CLOCK_GATE),
               "clock_gate");
}

TEST(ClockTreeNodeTest, TypeToStringRegister)
{
  EXPECT_STREQ(ClockTreeNode::typeToString(ClockTreeNode::REGISTER),
               "register");
}

TEST(ClockTreeNodeTest, TypeToStringMacro)
{
  EXPECT_STREQ(ClockTreeNode::typeToString(ClockTreeNode::MACRO), "macro");
}

TEST(ClockTreeNodeTest, TypeToStringUnknown)
{
  EXPECT_STREQ(ClockTreeNode::typeToString(ClockTreeNode::UNKNOWN), "unknown");
}

//------------------------------------------------------------------------------
// ClockTreeData default values
//------------------------------------------------------------------------------

TEST(ClockTreeDataTest, DefaultValues)
{
  ClockTreeData data;
  EXPECT_TRUE(data.clock_name.empty());
  EXPECT_EQ(data.min_arrival, 0.0f);
  EXPECT_EQ(data.max_arrival, 0.0f);
  EXPECT_TRUE(data.time_unit.empty());
  EXPECT_TRUE(data.nodes.empty());
}

//------------------------------------------------------------------------------
// ClockTreeNode default values
//------------------------------------------------------------------------------

TEST(ClockTreeNodeTest, DefaultValues)
{
  ClockTreeNode node;
  EXPECT_EQ(node.id, 0);
  EXPECT_EQ(node.parent_id, -1);
  EXPECT_TRUE(node.name.empty());
  EXPECT_TRUE(node.pin_name.empty());
  EXPECT_EQ(node.type, ClockTreeNode::UNKNOWN);
  EXPECT_EQ(node.arrival, 0.0f);
  EXPECT_EQ(node.delay, 0.0f);
  EXPECT_EQ(node.fanout, 0);
  EXPECT_EQ(node.level, 0);
  EXPECT_EQ(node.dbu_x, 0);
  EXPECT_EQ(node.dbu_y, 0);
}

//------------------------------------------------------------------------------
// ClockTreeHandler::handleClockTreeHighlight tests
//------------------------------------------------------------------------------

class ClockTreeHighlightTest : public tst::Nangate45Fixture
{
 protected:
  void SetUp() override
  {
    block_->setDieArea(odb::Rect(0, 0, 100000, 100000));
    placeInst("BUF_X16", "clkbuf1", 5000, 5000);
    gen_ = std::make_shared<TileGenerator>(
        getDb(), /*sta=*/nullptr, getLogger());
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
  SessionState state_;
};

TEST_F(ClockTreeHighlightTest, HighlightExistingInstance)
{
  auto handler = std::make_unique<ClockTreeHandler>(gen_, nullptr, nullptr);

  WebSocketRequest req;
  req.id = 1;
  req.type = WebSocketRequest::CLOCK_TREE_HIGHLIGHT;
  req.clock_tree_inst_name = "clkbuf1";

  auto resp = handler->handleClockTreeHighlight(req, state_);
  EXPECT_EQ(resp.id, 1u);
  EXPECT_EQ(resp.type, 0);

  std::string json = payloadStr(resp);
  EXPECT_NE(json.find("\"ok\""), std::string::npos);

  // Should have populated highlight_rects with the instance bbox
  std::lock_guard<std::mutex> lock(state_.selection_mutex);
  EXPECT_EQ(state_.highlight_rects.size(), 1u);
}

TEST_F(ClockTreeHighlightTest, HighlightNonExistentInstance)
{
  auto handler = std::make_unique<ClockTreeHandler>(gen_, nullptr, nullptr);

  WebSocketRequest req;
  req.id = 2;
  req.type = WebSocketRequest::CLOCK_TREE_HIGHLIGHT;
  req.clock_tree_inst_name = "does_not_exist";

  auto resp = handler->handleClockTreeHighlight(req, state_);
  EXPECT_EQ(resp.type, 0);

  // No instance found → no highlight rects
  std::lock_guard<std::mutex> lock(state_.selection_mutex);
  EXPECT_TRUE(state_.highlight_rects.empty());
}

TEST_F(ClockTreeHighlightTest, EmptyNameClearsState)
{
  // Pre-populate some state
  {
    std::lock_guard<std::mutex> lock(state_.selection_mutex);
    state_.highlight_rects.emplace_back(0, 0, 100, 100);
    state_.timing_rects.push_back(
        {odb::Rect(0, 0, 1, 1), {.r = 255, .g = 0, .b = 0, .a = 255}, ""});
  }

  auto handler = std::make_unique<ClockTreeHandler>(gen_, nullptr, nullptr);

  WebSocketRequest req;
  req.id = 3;
  req.type = WebSocketRequest::CLOCK_TREE_HIGHLIGHT;
  req.clock_tree_inst_name = "";

  auto resp = handler->handleClockTreeHighlight(req, state_);
  EXPECT_EQ(resp.type, 0);

  // All state should be cleared
  std::lock_guard<std::mutex> lock(state_.selection_mutex);
  EXPECT_TRUE(state_.highlight_rects.empty());
  EXPECT_TRUE(state_.timing_rects.empty());
  EXPECT_TRUE(state_.timing_lines.empty());
}

}  // namespace
}  // namespace web
