// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <algorithm>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

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
      = gen_->addLabel({30, 40}, "world", white, 12, "top left", "");
  EXPECT_EQ(n0, "label0");
  EXPECT_EQ(n1, "label1");

  const auto draw = gen_->labelsForDraw();
  ASSERT_EQ(draw.size(), 2u);
  EXPECT_EQ(draw[0].text, "hello");
  EXPECT_EQ(draw[1].size, 12);
  EXPECT_EQ(draw[1].anchor, "top left");
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
  EXPECT_TRUE(gen_->updateLabel("L", {7, 8}, "new", red, 20, "top left"));

  const auto draw = gen_->labelsForDraw();
  ASSERT_EQ(draw.size(), 1u);
  EXPECT_EQ(draw[0].text, "new");
  EXPECT_EQ(draw[0].pos.x(), 7);
  EXPECT_EQ(draw[0].pos.y(), 8);
  EXPECT_EQ(draw[0].size, 20);
  EXPECT_EQ(draw[0].anchor, "top left");
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
  gen_->addLabel({5, 6}, "hi", c, 8, "bottom right", "L");
  const boost::json::array arr = gen_->labelsJson();
  ASSERT_EQ(arr.size(), 1u);
  const auto& o = arr[0].as_object();
  EXPECT_EQ(o.at("name").as_string(), "L");
  EXPECT_EQ(o.at("x").as_int64(), 5);
  EXPECT_EQ(o.at("y").as_int64(), 6);
  EXPECT_EQ(o.at("text").as_string(), "hi");
  EXPECT_EQ(o.at("size").as_int64(), 8);
  EXPECT_EQ(o.at("anchor").as_string(), "bottom right");
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

// The spellings are shared with the Qt GUI's add_label, so pin them here
// against gui::Painter::anchors() (src/gui/src/painter.cpp), which libweb
// cannot link and therefore cannot assert against at compile time.
TEST_F(LabelTest, AnchorNamesMatchTheQtGui)
{
  std::vector<std::string> got = anchorNames();
  std::sort(got.begin(), got.end());
  const std::vector<std::string> want = {"bottom center",
                                         "bottom left",
                                         "bottom right",
                                         "center",
                                         "left center",
                                         "right center",
                                         "top center",
                                         "top left",
                                         "top right"};
  EXPECT_EQ(got, want);
  for (const std::string& name : anchorNames()) {
    EXPECT_TRUE(isValidAnchor(name)) << name;
  }
}

TEST_F(LabelTest, UnknownAnchorIsNotValid)
{
  // The old web-only spelling: it must not quietly keep working, or a script
  // using it would centre every label with no diagnostic.
  EXPECT_FALSE(isValidAnchor("top_left"));
  EXPECT_FALSE(isValidAnchor("Top Left"));
  EXPECT_FALSE(isValidAnchor("middle"));
  // Empty is the caller's cue to substitute "center", not a name itself.
  EXPECT_FALSE(isValidAnchor(""));
}

TEST_F(LabelTest, HandlerRejectsUnknownAnchor)
{
  auto handler = std::make_unique<TileHandler>(gen_);

  WebSocketRequest add;
  add.id = 1;
  add.type = WebSocketRequest::kAddLabel;
  add.json
      = parseObj(R"({"x":1,"y":2,"text":"t","size":0,"anchor":"top_left"})");
  auto resp = handler->handleAddLabel(add);
  EXPECT_EQ(resp.type, WebSocketResponse::kError);
  EXPECT_NE(payloadStr(resp).find("anchor not recognized"), std::string::npos);
  // Refused outright — no half-created label left behind.
  EXPECT_TRUE(gen_->labelsForDraw().empty());
}

// The Inspector's Anchor picker is built from this list, so shipping it is
// what keeps the offered choices and the accepted ones from drifting apart.
TEST_F(LabelTest, ListLabelsShipsTheAnchorChoices)
{
  auto handler = std::make_unique<TileHandler>(gen_);
  WebSocketRequest list;
  list.id = 1;
  list.type = WebSocketRequest::kListLabels;
  const boost::json::object root
      = parseObj(payloadStr(handler->handleListLabels(list)));

  ASSERT_TRUE(root.contains("anchors"));
  const boost::json::array& arr = root.at("anchors").as_array();
  ASSERT_EQ(arr.size(), anchorNames().size());
  for (const auto& v : arr) {
    EXPECT_TRUE(isValidAnchor(std::string(v.as_string())));
  }
}

TEST_F(LabelTest, EveryAnchorIsAcceptedByTheHandler)
{
  auto handler = std::make_unique<TileHandler>(gen_);
  uint32_t id = 0;
  for (const std::string& name : anchorNames()) {
    WebSocketRequest add;
    add.id = ++id;
    add.type = WebSocketRequest::kAddLabel;
    add.json = parseObj(R"({"x":0,"y":0,"text":"t","size":0,"anchor":")" + name
                        + R"("})");
    auto resp = handler->handleAddLabel(add);
    EXPECT_EQ(resp.type, WebSocketResponse::kJson) << name;
    EXPECT_NE(payloadStr(resp).find("\"ok\":true"), std::string::npos) << name;
  }
  EXPECT_EQ(gen_->labelsForDraw().size(), anchorNames().size());
}

}  // namespace
}  // namespace web
