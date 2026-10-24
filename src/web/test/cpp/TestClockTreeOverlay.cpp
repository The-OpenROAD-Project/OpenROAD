// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <cstddef>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "clock_tree_fixture.h"
#include "db_sta/dbNetwork.hh"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "script_view.h"
#include "sta/Mode.hh"
#include "sta/NetworkClass.hh"
#include "sta/Sdc.hh"
#include "sta/SdcClass.hh"
#include "sta/Units.hh"
#include "tile_generator.h"
#include "web/clockTree.h"
#include "web/core.h"

namespace web {
namespace {

TEST_F(ClockTreeTest, FollowsTheBuffersToTheRegisters)
{
  const std::vector<std::unique_ptr<ClockTree>> built = trees();
  ASSERT_EQ(built.size(), 1);
  const ClockTree& root = *built.front();
  EXPECT_EQ(root.getNetwork()->staToDb(root.getNet()), block_->findNet("clk"));
  EXPECT_EQ(root.getTotalLeaves(false), 4);
  ASSERT_EQ(root.getFanout().size(), 1);
  const ClockTree& c0 = *root.getFanout().front();
  EXPECT_EQ(c0.getSinkCount(), 2);
}

// ClockTreeRenderer's colors: the trunk in the color given, then a new color
// per branch down to max_depth splits; deeper branches keep their parent's.
TEST_F(ClockTreeTest, EachSplitDownToTheDepthGetsNewColors)
{
  const std::vector<std::unique_ptr<ClockTree>> built = trees();
  ASSERT_EQ(built.size(), 1);
  TileGenerator tiles(getDb(), /*sta=*/nullptr, getLogger());
  tiles.eagerInit();
  auto render = [&](const std::optional<int> depth) {
    ImageOverlays overlays;
    if (depth) {
      overlays.passes.emplace_back([&built, depth](Painter& painter) {
        paintClockTree(*built.front(), *depth, Painter::kWhite, painter);
      });
    }
    return tiles.renderImageBuffer(odb::Rect(0, 0, 0, 0),
                                   512,
                                   0,
                                   TileVisibility(),
                                   Color(),
                                   nullptr,
                                   nullptr,
                                   &overlays);
  };
  const std::vector<unsigned char> plain = render(std::nullopt);
  auto added = [&plain](const std::vector<unsigned char>& rgba,
                        const Painter::Color& color) {
    int n = 0;
    for (size_t i = 0; i + 3 < rgba.size(); i += 4) {
      const bool is = std::cmp_equal(rgba[i], color.r)
                      && std::cmp_equal(rgba[i + 1], color.g)
                      && std::cmp_equal(rgba[i + 2], color.b);
      const bool was = std::cmp_equal(plain[i], color.r)
                       && std::cmp_equal(plain[i + 1], color.g)
                       && std::cmp_equal(plain[i + 2], color.b);
      n += is && !was ? 1 : 0;
    }
    return n;
  };
  // ColorGenerator's first five, taken depth first along the tree.
  const Painter::Color red(255, 0, 0);
  const Painter::Color orange(255, 140, 0);
  const Painter::Color gold(255, 215, 0);
  const Painter::Color lime(0, 255, 0);
  const Painter::Color purple(148, 0, 211);

  // Depth 1: one color for each of c0's two branches, b1's included.
  const std::vector<unsigned char> one = render(1);
  EXPECT_GT(added(one, Painter::kWhite), 0);
  EXPECT_GT(added(one, red), 0);
  EXPECT_GT(added(one, orange), 0);
  EXPECT_EQ(added(one, gold), 0);
  EXPECT_EQ(added(one, lime), 0);

  // Depth 2: b1's two branches get theirs too.
  const std::vector<unsigned char> two = render(2);
  EXPECT_GT(added(two, Painter::kWhite), 0);
  EXPECT_GT(added(two, red), 0);
  EXPECT_GT(added(two, orange), 0);
  EXPECT_GT(added(two, gold), 0);
  EXPECT_GT(added(two, lime), 0);
  EXPECT_EQ(added(two, purple), 0);
}

// The trunk stands out from the background, light or dark.
TEST(ClockTreeTrunkTest, ContrastsWithTheBackground)
{
  auto same = [](const Painter::Color& a, const Painter::Color& b) {
    return a.r == b.r && a.g == b.g && a.b == b.b;
  };
  EXPECT_TRUE(same(clockTreeTrunkColor(Painter::kBlack), Painter::kWhite));
  EXPECT_TRUE(
      same(clockTreeTrunkColor(Painter::Color(40, 40, 60)), Painter::kWhite));
  EXPECT_TRUE(same(clockTreeTrunkColor(Painter::kWhite), Painter::kBlack));
  EXPECT_TRUE(same(clockTreeTrunkColor(Painter::Color(200, 210, 190)),
                   Painter::kBlack));
}

class ScriptViewClockTreeTest : public ClockTreeTest
{
 protected:
  bool hasClockTreesRow()
  {
    for (const std::string& path : view_.displayControlPaths()) {
      if (path == "Clock trees") {
        return true;
      }
    }
    return false;
  }

  ScriptView view_{getDb(), getSta(), getLogger()};
};

// As ClockWidget: the tree is drawn while the viewer is shown with a clock
// chosen, and ClockTreeRenderer's row exists only then.
TEST_F(ScriptViewClockTreeTest, TheTreeIsDrawnWhileTheViewerShowsAClock)
{
  EXPECT_EQ(view_.frame().clock_tree, nullptr);
  view_.showWidget("Clock Tree Viewer", true);
  EXPECT_EQ(view_.frame().clock_tree, nullptr) << "no clock chosen yet";
  EXPECT_FALSE(hasClockTreesRow());

  view_.selectClockviewerClock("clk", std::nullopt);
  ASSERT_NE(view_.frame().clock_tree, nullptr);
  EXPECT_EQ(view_.frame().clock_tree->getClock()->name(), "clk");
  EXPECT_EQ(view_.frame().clock_tree_depth, 1);
  EXPECT_TRUE(hasClockTreesRow());

  view_.setDisplayControlVisible("Clock trees", false);
  EXPECT_EQ(view_.frame().clock_tree, nullptr);

  view_.showWidget("clock_viewer", false);
  EXPECT_EQ(view_.frame().clock_tree, nullptr);
  EXPECT_FALSE(hasClockTreesRow());

  // Shown again, the renderer registers again and its row starts on.
  view_.showWidget("Clock Tree Viewer", true);
  EXPECT_TRUE(hasClockTreesRow());
  EXPECT_NE(view_.frame().clock_tree, nullptr);
}

TEST_F(ScriptViewClockTreeTest, TheDepthStaysWithItsClock)
{
  view_.selectClockviewerClock("clk", 3);
  EXPECT_EQ(view_.frame().clock_tree_depth, 3);
  view_.selectClockviewerClock("clk", std::nullopt);
  EXPECT_EQ(view_.frame().clock_tree_depth, 3);
}

// ClockWidget populates before it looks for the clock, so an unknown one
// still leaves the viewer shown, on the first clock.
TEST_F(ScriptViewClockTreeTest, AnUnknownClockIsAnError)
{
  EXPECT_THROW(view_.selectClockviewerClock("nope", std::nullopt),
               std::runtime_error);
  ASSERT_NE(view_.frame().clock_tree, nullptr);
  EXPECT_EQ(view_.frame().clock_tree->getClock()->name(), "clk");
}

// As ClockWidget::populate: a clock on a port with no net has no tree, so it
// is not one of the viewer's clocks.
TEST_F(ScriptViewClockTreeTest, AClockWithNoNetIsNotListed)
{
  odb::dbBTerm* port
      = odb::dbBTerm::create(odb::dbNet::create(block_, "nc"), "floating");
  port->disconnect();
  sta::dbNetwork* network = sta_->getDbNetwork();
  sta::PinSet pins(network);
  pins.insert(network->dbToSta(port));
  const float period = sta_->units()->timeUnit()->userToSta(1.0);
  sta_->makeClock("floating",
                  pins,
                  false,
                  period,
                  sta::FloatSeq{0.0, period / 2},
                  "",
                  sta_->cmdMode());

  EXPECT_THROW(view_.selectClockviewerClock("floating", std::nullopt),
               std::runtime_error);
  ASSERT_NE(view_.frame().clock_tree, nullptr);
  EXPECT_EQ(view_.frame().clock_tree->getClock()->name(), "clk");
}

// The trees point into the SDC as well: a clock defined again is the one the
// next image draws.
TEST_F(ScriptViewClockTreeTest, AClockDefinedAgainIsTheOneDrawn)
{
  view_.selectClockviewerClock("clk", std::nullopt);
  ASSERT_NE(view_.frame().clock_tree, nullptr);
  defineClock();
  const std::shared_ptr<const ClockTree> tree = view_.frame().clock_tree;
  ASSERT_NE(tree, nullptr);
  EXPECT_EQ(tree->getClock(), sta_->cmdMode()->sdc()->findClock("clk"));
}

// The trees point into the netlist, so each image builds its own.
TEST_F(ScriptViewClockTreeTest, EachImageBuildsItsOwnTree)
{
  view_.selectClockviewerClock("clk", std::nullopt);
  const std::shared_ptr<const ClockTree> before = view_.frame().clock_tree;
  odb::dbNet::destroy(odb::dbNet::create(block_, "unused"));
  const std::shared_ptr<const ClockTree> after = view_.frame().clock_tree;
  ASSERT_NE(after, nullptr);
  EXPECT_NE(after, before);
}

}  // namespace
}  // namespace web
