// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// The exact check must ask the question the annealer answers: every shape
// the annealer can give a macro is one the packing problem allows, and a
// placement the check finds can be handed back to the annealer as a
// sequence pair.

#include <algorithm>
#include <cstdint>
#include <random>
#include <vector>

#include "gtest/gtest.h"
#include "object.h"
#include "odb/geom.h"
#include "packing.h"
#include "packingAdapter.h"
#include "utl/Logger.h"

namespace mpl {
namespace {

bool allows(const PackingBlock& block, const int width, const int height)
{
  for (const Tiling& shape : block.shapes) {
    if (shape.width() == width && shape.height() == height) {
      return true;
    }
  }
  for (const SoftShape& shape : block.soft_shapes) {
    if (shape.width.min <= width && width <= shape.width.max
        && height == shape.area / width) {
      return true;
    }
  }
  return false;
}

class PackingAdapter : public ::testing::Test
{
 protected:
  utl::Logger logger_;
  std::mt19937 rng_{7};
  std::uniform_real_distribution<float> unit_{0.0f, 1.0f};

  // Every shape resizeRandomly() gives `macro` is allowed by its block.
  void expectEveryResizeAllowed(SoftMacro macro)
  {
    const PackingProblem problem = toPackingProblem(
        odb::Rect(0, 0, 1000000, 1000000), std::vector<SoftMacro>{macro}, 0);
    ASSERT_EQ(problem.blocks.size(), 1);
    for (int i = 0; i < 2000; i++) {
      macro.resizeRandomly(unit_, rng_);
      ASSERT_TRUE(
          allows(problem.blocks[0], macro.getWidth(), macro.getHeight()))
          << macro.getWidth() << " x " << macro.getHeight();
    }
  }
};

TEST_F(PackingAdapter, StdCellClusterShapesAreAllowed)
{
  Cluster cluster(1, "std", &logger_);
  cluster.setClusterType(StdCellCluster);
  SoftMacro macro(&cluster);
  macro.setShapes(IntervalList{Interval(1000, 3000)}, 2000003);
  expectEveryResizeAllowed(macro);
}

TEST_F(PackingAdapter, MixedClusterWithSeveralPiecesIsAllowed)
{
  Cluster cluster(2, "mixed", &logger_);
  cluster.setClusterType(MixedCluster);
  SoftMacro macro(&cluster);
  macro.setShapes(IntervalList{Interval(1000, 1500), Interval(2500, 4000)},
                  5000017);
  expectEveryResizeAllowed(macro);
}

TEST_F(PackingAdapter, MacroClusterTilingsAreAllowed)
{
  Cluster cluster(3, "macros", &logger_);
  cluster.setClusterType(HardMacroCluster);
  SoftMacro macro(&cluster);
  macro.setShapes(TilingList{Tiling(400, 100), Tiling(200, 200)}, true);
  expectEveryResizeAllowed(macro);
}

TEST_F(PackingAdapter, TerminalsAndEmptyMacrosTakeNoRoom)
{
  Cluster cluster(4, "macros", &logger_);
  cluster.setClusterType(HardMacroCluster);
  SoftMacro packed(&cluster);
  packed.setShapes(TilingList{Tiling(10, 10)}, true);
  // A blockage is fixed where it is.
  SoftMacro blockage(odb::Rect(5, 6, 25, 36), "blockage");
  // Zero area: takes no room.
  SoftMacro empty(odb::Point(0, 0), "empty", 0, 0, nullptr);
  // Past the sequence pair: a terminal for wirelength only.
  SoftMacro io(odb::Point(0, 0), "io", 500, 500, nullptr);
  std::vector<int> macro_of_block;
  const PackingProblem problem
      = toPackingProblem(odb::Rect(100, 100, 200, 200),
                         std::vector<SoftMacro>{packed, blockage, empty, io},
                         3,
                         &macro_of_block);
  ASSERT_EQ(problem.blocks.size(), 2);
  EXPECT_EQ(macro_of_block, (std::vector<int>{0, 1}));
  EXPECT_EQ(problem.width, 100);
  ASSERT_TRUE(problem.blocks[1].isFixed());
  EXPECT_EQ(*problem.blocks[1].fixed_x, 5);
  EXPECT_EQ(*problem.blocks[1].fixed_y, 6);
  EXPECT_EQ(problem.blocks[1].shapes.front(), Tiling(20, 30));
}

TEST_F(PackingAdapter, HardMacrosAreRigidWithTheirHalos)
{
  HardMacro macro(30, 40, "m");
  const PackingProblem problem = toPackingProblem(
      odb::Rect(0, 0, 100, 100), std::vector<HardMacro>{macro}, 0);
  ASSERT_EQ(problem.blocks.size(), 1);
  EXPECT_FALSE(problem.blocks[0].isFixed());
  EXPECT_EQ(problem.blocks[0].shapes, std::vector<Tiling>{Tiling(30, 40)});
}

}  // namespace
}  // namespace mpl
