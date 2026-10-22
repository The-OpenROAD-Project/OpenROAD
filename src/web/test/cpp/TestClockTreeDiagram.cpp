// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <cstddef>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "clock_tree_diagram.h"
#include "clock_tree_fixture.h"
#include "color.h"
#include "gtest/gtest.h"
#include "script_view.h"
#include "sta/Clock.hh"
#include "tile_generator.h"
#include "web/clockTree.h"

namespace web {
namespace {

using Kind = ClockTreeDiagram::Kind;
using Node = ClockTreeDiagram::Node;

class ClockTreeDiagramTest : public ClockTreeTest
{
 protected:
  ClockTreeDiagram layout()
  {
    const std::vector<std::unique_ptr<ClockTree>> built = trees();
    EXPECT_EQ(built.size(), 1);
    return layoutClockTreeDiagram(*built.front());
  }

  static std::map<std::string, Node> byName(const ClockTreeDiagram& diagram)
  {
    std::map<std::string, Node> nodes;
    for (const Node& node : diagram.nodes) {
      nodes[node.name] = node;
    }
    return nodes;
  }
};

// ClockTreeView's layout: a node per port, cell and register, each where its
// arrival puts it, so every level sits below the one that drives it.
TEST_F(ClockTreeDiagramTest, EachLevelSitsBelowItsDriver)
{
  const ClockTreeDiagram diagram = layout();
  std::map<std::string, Node> nodes = byName(diagram);
  ASSERT_EQ(nodes.size(), 10);
  EXPECT_EQ(nodes["clk"].kind, Kind::kRoot);
  EXPECT_EQ(nodes["b1"].kind, Kind::kBuffer);
  EXPECT_EQ(nodes["ff0"].kind, Kind::kRegister);
  // One net per driver: clk, b0, b1, b2, b3 and b4.
  EXPECT_EQ(diagram.nets.size(), 6);

  for (const auto& [driver, sink] :
       std::vector<std::pair<const char*, const char*>>{
           {"b0", "b1"},
           {"b1", "b3"},
           {"b3", "ff0"},
           {"b2", "ff2"},
       }) {
    EXPECT_GT(nodes[driver].delay_y, 0) << driver;
    EXPECT_LE(nodes[driver].pos.y + nodes[driver].delay_y, nodes[sink].pos.y)
        << driver << " -> " << sink;
  }
  // The root is drawn above its output, which is where it sits.
  EXPECT_LE(nodes["clk"].pos.y, nodes["b0"].pos.y);
  EXPECT_NE(nodes["b3"].pos.x, nodes["b4"].pos.x);
  EXPECT_NE(nodes["ff2"].pos.x, nodes["ff3"].pos.x);

  // y is linear in the arrival, over the scene's height.
  EXPECT_DOUBLE_EQ(diagram.delayToY(diagram.min_delay), 0);
  EXPECT_DOUBLE_EQ(diagram.delayToY(diagram.max_delay), diagram.time_scale);
  EXPECT_NEAR(diagram.yToDelay(diagram.delayToY(diagram.max_delay / 3)),
              diagram.max_delay / 3,
              1e-15);
  for (const Node& node : diagram.nodes) {
    EXPECT_GE(node.pos.x, diagram.scene_min.x) << node.name;
    EXPECT_LE(node.pos.x, diagram.scene_max.x) << node.name;
    EXPECT_GE(node.pos.y, diagram.scene_min.y) << node.name;
    EXPECT_LE(node.pos.y, diagram.scene_max.y) << node.name;
  }
}

// ClockTreeView::save: white, the requested size, the root and the buffers
// filled with their outline color lightened, the time scale on the left.
TEST_F(ClockTreeDiagramTest, TheImageIsTheSizeAskedOnWhite)
{
  const ClockTreeDiagram diagram = layout();
  TileGenerator tiles(getDb(), /*sta=*/nullptr, getLogger());
  constexpr int kWidth = 300;
  constexpr int kHeight = 200;
  const std::vector<unsigned char> image
      = renderClockTreeDiagram(diagram, kWidth, kHeight, tiles);
  ASSERT_EQ(image.size(), 4UL * kWidth * kHeight);

  auto pixel = [&](const int x, const int y) {
    const size_t i = 4 * (static_cast<size_t>(y) * kWidth + x);
    return Color{
        .r = image[i],
        .g = image[i + 1],
        .b = image[i + 2],
        .a = image[i + 3],
    };
  };
  auto count = [&](const Color& color) {
    int n = 0;
    for (int y = 0; y < kHeight; ++y) {
      for (int x = 0; x < kWidth; ++x) {
        n += pixel(x, y) == color ? 1 : 0;
      }
    }
    return n;
  };
  constexpr Color kWhite{.r = 255, .g = 255, .b = 255, .a = 255};
  EXPECT_EQ(pixel(kWidth - 1, 0), kWhite);
  EXPECT_EQ(pixel(kWidth - 1, kHeight - 1), kWhite);
  EXPECT_GT(count(Color{.r = 255, .g = 127, .b = 127, .a = 255}), 0)
      << "the root, Qt::red lighter";
  EXPECT_GT(count(Color{.r = 127, .g = 127, .b = 255, .a = 255}), 0)
      << "the buffers, Qt::blue lighter";
  int dark = 0;
  for (int y = 0; y < kHeight; ++y) {
    dark += pixel(0, y).r < 200 ? 1 : 0;
  }
  EXPECT_GT(dark, kHeight / 2) << "the time scale's edge";
}

class ScriptViewClockImageTest : public ClockTreeTest
{
 protected:
  struct Saved
  {
    std::string clock;
    std::string filename;
    int width;
    int height;
  };

  void SetUp() override
  {
    ClockTreeTest::SetUp();
    view_.setClockTreeImageWriter([this](const ClockTree& tree,
                                         const std::string& filename,
                                         const int width,
                                         const int height) {
      saved_.push_back({
          .clock = tree.getClock()->name(),
          .filename = filename,
          .width = width,
          .height = height,
      });
    });
  }

  ScriptView view_{getDb(), getSta(), getLogger()};
  std::vector<Saved> saved_;
};

// As ClockWidget::saveImage: the clock becomes the viewer's, and the viewer
// is left hidden, as it was.
TEST_F(ScriptViewClockImageTest, SavesTheClockAndLeavesTheViewerAsItWas)
{
  view_.saveClockTreeImage("clk", "tree.png", "", 300, std::nullopt);
  ASSERT_EQ(saved_.size(), 1);
  EXPECT_EQ(saved_[0].clock, "clk");
  EXPECT_EQ(saved_[0].filename, "tree.png");
  EXPECT_EQ(saved_[0].width, 300);
  EXPECT_EQ(saved_[0].height, 1024) << "save_image -web's default size";
  EXPECT_EQ(view_.frame().clock_tree, nullptr) << "the viewer stays hidden";

  view_.showWidget("Clock Tree Viewer", true);
  ASSERT_NE(view_.frame().clock_tree, nullptr);
  EXPECT_EQ(view_.frame().clock_tree->getClock()->name(), "clk");
}

TEST_F(ScriptViewClockImageTest, AnUnknownSceneOrClockIsAnError)
{
  EXPECT_THROW(view_.saveClockTreeImage(
                   "clk", "tree.png", "nope", std::nullopt, std::nullopt),
               std::runtime_error);
  EXPECT_THROW(view_.saveClockTreeImage(
                   "nope", "tree.png", "", std::nullopt, std::nullopt),
               std::runtime_error);
  EXPECT_TRUE(saved_.empty());
}

}  // namespace
}  // namespace web
