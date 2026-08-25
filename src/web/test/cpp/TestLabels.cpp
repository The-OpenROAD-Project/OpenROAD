// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <algorithm>
#include <cstdint>
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

// Labels live in the shared TileGenerator, so a mutation from one session (or
// from Tcl) changes what every other client should draw.  Nothing else tells
// them: labels are outside ODB, so no design-change callback covers them.
class LabelBroadcastTest : public LabelTest
{
 protected:
  void SetUp() override
  {
    LabelTest::SetUp();
    handler_ = std::make_unique<TileHandler>(gen_);
    handler_->setBroadcastFn(
        [this](const std::string& msg) { sent_.push_back(msg); });
  }

  WebSocketResponse add(const std::string& name)
  {
    WebSocketRequest req;
    req.id = ++id_;
    req.type = WebSocketRequest::kAddLabel;
    req.json = parseObj(R"({"x":1,"y":2,"text":"t","name":")" + name + R"("})");
    return handler_->handleAddLabel(req);
  }

  std::unique_ptr<TileHandler> handler_;
  std::vector<std::string> sent_;
  uint32_t id_ = 0;
};

TEST_F(LabelBroadcastTest, AddBroadcastsTheNewSet)
{
  ASSERT_EQ(add("L0").type, WebSocketResponse::kJson);
  ASSERT_EQ(sent_.size(), 1u);

  const boost::json::object msg = parseObj(sent_[0]);
  EXPECT_EQ(msg.at("type").as_string(), "labels_changed");
  // The set rides along so a client can repaint without a round-trip.
  ASSERT_TRUE(msg.contains("labels"));
  EXPECT_EQ(msg.at("labels").as_array().size(), 1u);
}

TEST_F(LabelBroadcastTest, RejectedAddDoesNotBroadcast)
{
  ASSERT_EQ(add("L0").type, WebSocketResponse::kJson);
  sent_.clear();
  // Duplicate name: refused, so nothing changed for anyone to hear about.
  add("L0");
  EXPECT_TRUE(sent_.empty());
}

TEST_F(LabelBroadcastTest, UpdateDeleteAndClearBroadcast)
{
  ASSERT_EQ(add("L0").type, WebSocketResponse::kJson);
  sent_.clear();

  WebSocketRequest upd;
  upd.id = 10;
  upd.type = WebSocketRequest::kUpdateLabel;
  upd.json = parseObj(R"({"name":"L0","x":5,"y":6,"text":"new"})");
  ASSERT_EQ(handler_->handleUpdateLabel(upd).type, WebSocketResponse::kJson);
  EXPECT_EQ(sent_.size(), 1u);

  WebSocketRequest del;
  del.id = 11;
  del.type = WebSocketRequest::kDeleteLabel;
  del.json = parseObj(R"({"name":"L0"})");
  ASSERT_EQ(handler_->handleDeleteLabel(del).type, WebSocketResponse::kJson);
  EXPECT_EQ(sent_.size(), 2u);

  WebSocketRequest clr;
  clr.id = 12;
  clr.type = WebSocketRequest::kClearLabels;
  clr.json = parseObj("{}");
  handler_->handleClearLabels(clr);
  EXPECT_EQ(sent_.size(), 3u);
}

TEST_F(LabelBroadcastTest, MissingTargetDoesNotBroadcast)
{
  WebSocketRequest del;
  del.id = 1;
  del.type = WebSocketRequest::kDeleteLabel;
  del.json = parseObj(R"({"name":"nope"})");
  handler_->handleDeleteLabel(del);

  WebSocketRequest upd;
  upd.id = 2;
  upd.type = WebSocketRequest::kUpdateLabel;
  upd.json = parseObj(R"({"name":"nope","x":1,"y":1,"text":"t"})");
  handler_->handleUpdateLabel(upd);

  EXPECT_TRUE(sent_.empty());
}

TEST_F(LabelBroadcastTest, WorksWithNoBroadcastFnInstalled)
{
  // The handler is also constructed in contexts with no session registry.
  TileHandler bare(gen_);
  WebSocketRequest req;
  req.id = 1;
  req.type = WebSocketRequest::kAddLabel;
  req.json = parseObj(R"({"x":1,"y":2,"text":"t"})");
  EXPECT_EQ(bare.handleAddLabel(req).type, WebSocketResponse::kJson);
}

}  // namespace
}  // namespace web
