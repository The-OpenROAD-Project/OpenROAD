// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <memory>
#include <string>
#include <string_view>

#include "boost/json/object.hpp"
#include "boost/json/parse.hpp"
#include "color.h"
#include "gtest/gtest.h"
#include "request_handler.h"
#include "tile_generator.h"
#include "tst/nangate45_fixture.h"

namespace web {
namespace {

std::string payloadStr(const WebSocketResponse& resp)
{
  return std::string(resp.payload.begin(), resp.payload.end());
}

boost::json::object parseObj(std::string_view json)
{
  return boost::json::parse(json).as_object();
}

class LabelTest : public tst::Nangate45Fixture
{
 protected:
  void SetUp() override
  {
    gen_ = std::make_shared<TileGenerator>(
        getDb(), /*sta=*/nullptr, getLogger());
  }
  std::shared_ptr<TileGenerator> gen_;
};

TEST_F(LabelTest, AddAutoNamesAndStores)
{
  const Color white{.r = 255, .g = 255, .b = 255, .a = 255};
  const std::string n0
      = gen_->addLabel({10, 20}, "hello", white, 0, "center", "");
  const std::string n1
      = gen_->addLabel({30, 40}, "world", white, 12, "top_left", "");
  EXPECT_EQ(n0, "label0");
  EXPECT_EQ(n1, "label1");

  const auto draw = gen_->labelsForDraw();
  ASSERT_EQ(draw.size(), 2u);
  EXPECT_EQ(draw[0].text, "hello");
  EXPECT_EQ(draw[1].size, 12);
  EXPECT_EQ(draw[1].anchor, "top_left");
}

TEST_F(LabelTest, DuplicateNameRejected)
{
  const Color c{.r = 1, .g = 2, .b = 3, .a = 255};
  EXPECT_EQ(gen_->addLabel({0, 0}, "a", c, 0, "center", "mine"), "mine");
  EXPECT_EQ(gen_->addLabel({1, 1}, "b", c, 0, "center", "mine"), "");
  EXPECT_EQ(gen_->labelsForDraw().size(), 1u);
}

TEST_F(LabelTest, AutoNameSkipsUserTakenId)
{
  const Color c{.r = 0, .g = 0, .b = 0, .a = 255};
  // A user manually grabs "label0"; the next auto-generated name must not
  // collide with it (which would otherwise return "").
  EXPECT_EQ(gen_->addLabel({0, 0}, "manual", c, 0, "center", "label0"),
            "label0");
  const std::string automatic
      = gen_->addLabel({1, 1}, "auto", c, 0, "center", "");
  EXPECT_FALSE(automatic.empty());
  EXPECT_NE(automatic, "label0");
  EXPECT_EQ(gen_->labelsForDraw().size(), 2u);
}

TEST_F(LabelTest, DeleteAndClear)
{
  const Color c{.r = 0, .g = 0, .b = 0, .a = 255};
  gen_->addLabel({0, 0}, "a", c, 0, "center", "a");
  gen_->addLabel({1, 1}, "b", c, 0, "center", "b");
  EXPECT_TRUE(gen_->deleteLabel("a"));
  EXPECT_FALSE(gen_->deleteLabel("missing"));
  EXPECT_EQ(gen_->labelsForDraw().size(), 1u);
  gen_->clearLabels();
  EXPECT_TRUE(gen_->labelsForDraw().empty());
}

TEST_F(LabelTest, UpdateMutatesInPlace)
{
  const Color c{.r = 0, .g = 0, .b = 0, .a = 255};
  gen_->addLabel({0, 0}, "old", c, 0, "center", "L");

  const Color red{.r = 255, .g = 0, .b = 0, .a = 255};
  EXPECT_TRUE(gen_->updateLabel("L", {7, 8}, "new", red, 20, "top_left"));

  const auto draw = gen_->labelsForDraw();
  ASSERT_EQ(draw.size(), 1u);
  EXPECT_EQ(draw[0].text, "new");
  EXPECT_EQ(draw[0].pos.x(), 7);
  EXPECT_EQ(draw[0].pos.y(), 8);
  EXPECT_EQ(draw[0].size, 20);
  EXPECT_EQ(draw[0].anchor, "top_left");
  EXPECT_EQ(draw[0].color.r, 255);
}

TEST_F(LabelTest, UpdateMissingIsNoOp)
{
  const Color c{.r = 0, .g = 0, .b = 0, .a = 255};
  gen_->addLabel({0, 0}, "a", c, 0, "center", "a");
  EXPECT_FALSE(gen_->updateLabel("missing", {1, 1}, "x", c, 0, "center"));
  const auto draw = gen_->labelsForDraw();
  ASSERT_EQ(draw.size(), 1u);
  EXPECT_EQ(draw[0].text, "a");
}

TEST_F(LabelTest, JsonRoundTrip)
{
  const Color c{.r = 10, .g = 20, .b = 30, .a = 255};
  gen_->addLabel({5, 6}, "hi", c, 8, "bottom_right", "L");
  const boost::json::array arr = gen_->labelsJson();
  ASSERT_EQ(arr.size(), 1u);
  const auto& o = arr[0].as_object();
  EXPECT_EQ(o.at("name").as_string(), "L");
  EXPECT_EQ(o.at("x").as_int64(), 5);
  EXPECT_EQ(o.at("y").as_int64(), 6);
  EXPECT_EQ(o.at("text").as_string(), "hi");
  EXPECT_EQ(o.at("size").as_int64(), 8);
  EXPECT_EQ(o.at("anchor").as_string(), "bottom_right");
  EXPECT_EQ(o.at("color").as_object().at("r").as_int64(), 10);
}

TEST_F(LabelTest, HandlerAddThenList)
{
  auto handler = std::make_unique<TileHandler>(gen_);

  WebSocketRequest add;
  add.id = 1;
  add.type = WebSocketRequest::kAddLabel;
  add.json = parseObj(
      R"({"x":100,"y":200,"text":"probe","size":14,"anchor":"center"})");
  auto add_resp = handler->handleAddLabel(add);
  EXPECT_EQ(add_resp.type, WebSocketResponse::kJson);
  EXPECT_NE(payloadStr(add_resp).find("\"ok\":true"), std::string::npos);

  WebSocketRequest list;
  list.id = 2;
  list.type = WebSocketRequest::kListLabels;
  auto list_resp = handler->handleListLabels(list);
  EXPECT_NE(payloadStr(list_resp).find("probe"), std::string::npos);
  EXPECT_EQ(gen_->labelsForDraw().size(), 1u);
}

}  // namespace
}  // namespace web
