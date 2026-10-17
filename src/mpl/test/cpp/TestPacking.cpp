// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// What checkPacking() must do, checked against answers known by
// construction and against a validator written here, never against the
// solver's own view of legality.

#include <algorithm>
#include <cstdint>
#include <functional>
#include <random>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "PackingTestUtil.h"
#include "gtest/gtest.h"
#include "packing.h"

namespace mpl {
namespace {

PackingBlock rigid(const int width, const int height)
{
  PackingBlock block;
  block.shapes.emplace_back(width, height);
  return block;
}

PackingBlock fixedAt(const int x,
                     const int y,
                     const int width,
                     const int height)
{
  PackingBlock block = rigid(width, height);
  block.fixed_x = x;
  block.fixed_y = y;
  return block;
}

PackingBlock soft(const int64_t area, const int min_width, const int max_width)
{
  PackingBlock block;
  block.soft_shapes.push_back({Interval(min_width, max_width), area});
  return block;
}

// Cut a width x height rectangle into `pieces` rigid blocks by random
// guillotine cuts: a perfect packing exists by construction.
std::vector<PackingBlock> plantGuillotine(const int width,
                                          const int height,
                                          const int pieces,
                                          std::mt19937& rng)
{
  struct Piece
  {
    int width;
    int height;
  };
  std::vector<Piece> work{{width, height}};
  while (static_cast<int>(work.size()) < pieces) {
    std::uniform_int_distribution<size_t> pick(0, work.size() - 1);
    const size_t index = pick(rng);
    const Piece piece = work[index];
    const bool vertical = piece.width >= piece.height;
    const int side = vertical ? piece.width : piece.height;
    if (side < 2) {
      continue;
    }
    std::uniform_int_distribution<int> cut(1, side - 1);
    const int at = cut(rng);
    work.erase(work.begin() + index);
    if (vertical) {
      work.push_back({at, piece.height});
      work.push_back({piece.width - at, piece.height});
    } else {
      work.push_back({piece.width, at});
      work.push_back({piece.width, piece.height - at});
    }
  }
  std::vector<PackingBlock> blocks;
  for (const Piece& piece : work) {
    blocks.push_back(rigid(piece.width, piece.height));
  }
  std::shuffle(blocks.begin(), blocks.end(), rng);
  return blocks;
}

TEST(Packing, EmptyProblemFits)
{
  const PackingProblem problem{100, 100, {}};
  EXPECT_EQ(checkPacking(problem).status, PackingStatus::kFits);
}

// A problem that cannot be stated is refused, not answered.
TEST(Packing, MalformedProblemIsRefused)
{
  EXPECT_THROW(checkPacking({0, 100, {}}), std::invalid_argument);
  EXPECT_THROW(checkPacking({100, -1, {}}), std::invalid_argument);
  EXPECT_THROW(checkPacking({100, 100, {PackingBlock{}}}),
               std::invalid_argument);
  PackingBlock half_fixed = rigid(10, 10);
  half_fixed.fixed_x = 0;
  EXPECT_THROW(checkPacking({100, 100, {half_fixed}}), std::invalid_argument);
  PackingBlock empty_piece;
  empty_piece.soft_shapes.push_back({Interval(0, 10), 100});
  EXPECT_THROW(checkPacking({100, 100, {empty_piece}}), std::invalid_argument);
}

TEST(Packing, PlantedPerfectPackingsFit)
{
  std::mt19937 rng(1);
  for (int pieces = 1; pieces <= 12; pieces++) {
    PackingProblem problem{1000, 700, plantGuillotine(1000, 700, pieces, rng)};
    const PackingResult result = checkPacking(problem);
    EXPECT_TRUE(isLegal(problem, result)) << pieces << " pieces";
  }
}

TEST(Packing, OneRowShortDoesNotFitByArea)
{
  std::mt19937 rng(2);
  for (int pieces = 2; pieces <= 12; pieces++) {
    PackingProblem problem{1000, 699, plantGuillotine(1000, 700, pieces, rng)};
    // A piece that spans the full height no longer fits on its own.
    const bool full_height = std::any_of(
        problem.blocks.begin(),
        problem.blocks.end(),
        [](const PackingBlock& b) { return b.shapes.front().height() == 700; });
    const PackingResult result = checkPacking(problem);
    EXPECT_EQ(result.status, PackingStatus::kDoesNotFit) << pieces;
    EXPECT_EQ(result.cause,
              full_height ? PackingCause::kBlockTooTall : PackingCause::kArea)
        << pieces;
  }
}

// Two s x s squares in a (2s - 1) x (2s - 1) outline: the area suffices,
// but neither side holds both.
TEST(Packing, TwoSquaresDoNotFitByArrangement)
{
  const int s = 10;
  const PackingProblem problem{
      2 * s - 1, 2 * s - 1, {rigid(s, s), rigid(s, s)}};
  const PackingResult result = checkPacking(problem);
  EXPECT_EQ(result.status, PackingStatus::kDoesNotFit);
  EXPECT_EQ(result.cause, PackingCause::kArrangement);
}

// Blocks wider than half the outline cannot share a row.
TEST(Packing, WideBlocksStack)
{
  const PackingProblem fits{
      100, 30, {rigid(60, 10), rigid(60, 10), rigid(60, 10)}};
  EXPECT_TRUE(isLegal(fits, checkPacking(fits)));
  const PackingProblem too_low{
      100, 29, {rigid(60, 10), rigid(60, 10), rigid(60, 10)}};
  EXPECT_EQ(checkPacking(too_low).status, PackingStatus::kDoesNotFit);
}

TEST(Packing, BlockWiderThanOutlineIsNamed)
{
  const PackingProblem problem{100, 100, {rigid(10, 10), rigid(101, 5)}};
  const PackingResult result = checkPacking(problem);
  EXPECT_EQ(result.status, PackingStatus::kDoesNotFit);
  EXPECT_EQ(result.cause, PackingCause::kBlockTooWide);
  EXPECT_EQ(result.block, 1);
}

TEST(Packing, BlockTallerThanOutlineIsNamed)
{
  const PackingProblem problem{100, 100, {rigid(5, 101)}};
  const PackingResult result = checkPacking(problem);
  EXPECT_EQ(result.cause, PackingCause::kBlockTooTall);
  EXPECT_EQ(result.block, 0);
}

TEST(Packing, AlternativeShapeIsUsedWhenOnlyItFits)
{
  PackingBlock block;
  block.shapes = {Tiling(200, 10), Tiling(10, 200)};
  const PackingProblem problem{50, 300, {block}};
  const PackingResult result = checkPacking(problem);
  ASSERT_TRUE(isLegal(problem, result));
  EXPECT_EQ(result.placement[0].width, 10);
}

TEST(Packing, MovableBlocksAvoidFixedOnes)
{
  // A fixed block in the middle leaves an L of free space.
  const PackingProblem problem{
      100, 100, {fixedAt(0, 0, 60, 60), rigid(40, 100), rigid(60, 40)}};
  EXPECT_TRUE(isLegal(problem, checkPacking(problem)));
}

TEST(Packing, FixedBlocksMayOverlapEachOther)
{
  const PackingProblem problem{
      100,
      100,
      {fixedAt(0, 0, 50, 50), fixedAt(25, 25, 50, 50), rigid(10, 10)}};
  EXPECT_TRUE(isLegal(problem, checkPacking(problem)));
}

TEST(Packing, FixedBlockLeavesNoRoom)
{
  const PackingProblem problem{
      100, 100, {fixedAt(0, 0, 60, 100), rigid(41, 10)}};
  EXPECT_EQ(checkPacking(problem).status, PackingStatus::kDoesNotFit);
}

TEST(Packing, SoftBlockTakesTheShapeThatFits)
{
  // Area 1000 fits a 100 x 10 slot only at width 91 or more.
  const PackingProblem problem{100, 50, {rigid(100, 40), soft(1000, 10, 100)}};
  const PackingResult result = checkPacking(problem);
  ASSERT_TRUE(isLegal(problem, result));
  EXPECT_GE(result.placement[1].width, 91);
}

TEST(Packing, SoftBlockOutsideItsWidthsDoesNotFit)
{
  const PackingProblem problem{100, 50, {rigid(100, 40), soft(1000, 10, 90)}};
  EXPECT_EQ(checkPacking(problem).status, PackingStatus::kDoesNotFit);
}

// However small the budget, the answer is one of the three statuses.
TEST(Packing, TinyBudgetIsUndecidedNotAnError)
{
  std::mt19937 rng(6);
  const PackingProblem problem{1000, 700, plantGuillotine(1000, 700, 12, rng)};
  for (const double budget : {1e-6, 1e-3, 0.01}) {
    PackingLimits limits;
    limits.deterministic_time = budget;
    EXPECT_NO_THROW(checkPacking(problem, limits)) << budget;
  }
}

TEST(Packing, SameInputSameAnswer)
{
  std::mt19937 rng(3);
  const PackingProblem problem{900, 900, plantGuillotine(900, 900, 10, rng)};
  const PackingResult a = checkPacking(problem);
  const PackingResult b = checkPacking(problem);
  ASSERT_EQ(a.status, PackingStatus::kFits);
  ASSERT_EQ(a.placement.size(), b.placement.size());
  for (size_t i = 0; i < a.placement.size(); i++) {
    EXPECT_EQ(a.placement[i].x, b.placement[i].x);
    EXPECT_EQ(a.placement[i].y, b.placement[i].y);
  }
}

// Identical rectangles that cannot rotate: an exhaustive search on a small
// grid agrees with the answer for every outline.
bool bruteForceFits(const int width,
                    const int height,
                    const int w,
                    const int h,
                    const int n,
                    std::vector<std::pair<int, int>>& placed)
{
  if (static_cast<int>(placed.size()) == n) {
    return true;
  }
  // Place the next block at or after the previous one (they are identical).
  const int start = placed.empty() ? 0
                                   : placed.back().second * (width + 1)
                                         + placed.back().first + 1;
  for (int p = start; p <= (height - h) * (width + 1) + width - w; p++) {
    const int x = p % (width + 1);
    const int y = p / (width + 1);
    if (x + w > width || y + h > height) {
      continue;
    }
    bool free = true;
    for (const auto& [px, py] : placed) {
      free &= x >= px + w || px >= x + w || y >= py + h || py >= y + h;
    }
    if (!free) {
      continue;
    }
    placed.emplace_back(x, y);
    if (bruteForceFits(width, height, w, h, n, placed)) {
      return true;
    }
    placed.pop_back();
  }
  return false;
}

TEST(Packing, IdenticalBlocksAgreeWithExhaustiveSearch)
{
  for (int width = 4; width <= 7; width++) {
    for (int height = 3; height <= 6; height++) {
      for (int n = 1; n <= 5; n++) {
        const int w = 2;
        const int h = 3;
        std::vector<PackingBlock> blocks(n, rigid(w, h));
        const PackingProblem problem{width, height, blocks};
        std::vector<std::pair<int, int>> placed;
        const bool fits = bruteForceFits(width, height, w, h, n, placed);
        const PackingResult result = checkPacking(problem);
        EXPECT_EQ(result.status,
                  fits ? PackingStatus::kFits : PackingStatus::kDoesNotFit)
            << width << " x " << height << ", " << n << " blocks";
        if (fits) {
          EXPECT_TRUE(isLegal(problem, result));
        }
      }
    }
  }
}

// A planted W x H perfect packing kept at width W must need exactly
// height H: the area alone forbids less, the plant proves H suffices.
TEST(PackingFix, SmallestHeightOfPlantedPackingIsItsHeight)
{
  std::mt19937 rng(4);
  for (int pieces = 2; pieces <= 8; pieces++) {
    const PackingProblem problem{
        1000, 300, plantGuillotine(1000, 700, pieces, rng)};
    const DimensionFix fix = smallestHeight(problem, 1);
    ASSERT_TRUE(fix.found) << pieces;
    EXPECT_EQ(fix.lower, 700) << pieces;
    EXPECT_EQ(fix.upper, 700) << pieces;
  }
}

TEST(PackingFix, SmallestWidthOfPlantedPackingIsItsWidth)
{
  std::mt19937 rng(5);
  for (int pieces = 2; pieces <= 8; pieces++) {
    const PackingProblem problem{
        3000, 700, plantGuillotine(1000, 700, pieces, rng)};
    const DimensionFix fix = smallestWidth(problem, 1);
    ASSERT_TRUE(fix.found) << pieces;
    EXPECT_EQ(fix.lower, 1000) << pieces;
    EXPECT_EQ(fix.upper, 1000) << pieces;
  }
}

// A grid without a step has no smallest outline to search for.
TEST(PackingFix, NonPositiveStepIsRefused)
{
  const PackingProblem problem{100, 100, {rigid(10, 10)}};
  EXPECT_THROW(smallestHeight(problem, 0), std::invalid_argument);
  EXPECT_THROW(smallestWidth(problem, -1), std::invalid_argument);
  EXPECT_THROW(smallestSameAspect(problem, 1, 0), std::invalid_argument);
  EXPECT_THROW(sameAspectHeight({0, 100, {}}, 10, 1), std::invalid_argument);
}

// Every proposed fix, applied, fits; one step less does not.
TEST(PackingFix, FixesAreTightOnTheGrid)
{
  const int step = 7;
  const PackingProblem problem{
      50, 50, {rigid(30, 30), rigid(30, 30), rigid(20, 45)}};
  for (const auto& [name, fix, apply] :
       std::vector<std::tuple<std::string,
                              DimensionFix,
                              std::function<PackingProblem(int)>>>{
           {"height",
            smallestHeight(problem, step),
            [&](int v) { return PackingProblem{50, v, problem.blocks}; }},
           {"width",
            smallestWidth(problem, step),
            [&](int v) { return PackingProblem{v, 50, problem.blocks}; }},
       }) {
    ASSERT_TRUE(fix.found) << name;
    EXPECT_EQ(fix.lower, fix.upper) << name;
    EXPECT_EQ(fix.upper % step, 0) << name;
    EXPECT_EQ(checkPacking(apply(fix.upper)).status, PackingStatus::kFits)
        << name;
    EXPECT_EQ(checkPacking(apply(fix.upper - step)).status,
              PackingStatus::kDoesNotFit)
        << name;
  }
}

TEST(PackingFix, SameAspectKeepsTheRatio)
{
  // 2:1 outline; two 60 x 60 squares need 120 wide, so 120 x 60.
  const PackingProblem problem{100, 50, {rigid(60, 60), rigid(60, 60)}};
  const DimensionFix fix = smallestSameAspect(problem, 1, 1);
  ASSERT_TRUE(fix.found);
  EXPECT_EQ(fix.upper, 120);
}

TEST(PackingFix, BlockWiderThanAnyHeightAllowsIsNotFixedByHeight)
{
  const PackingProblem problem{100, 100, {rigid(150, 10)}};
  EXPECT_FALSE(smallestHeight(problem, 1).found);
}

}  // namespace
}  // namespace mpl
