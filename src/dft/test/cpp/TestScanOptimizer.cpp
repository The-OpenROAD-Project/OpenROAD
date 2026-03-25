// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

// Unit tests for OptimizeScanWirelength (Opt.cpp).
//
// TwoOptScanChain and OrOptScanChain live in an anonymous namespace, so they
// are exercised indirectly through the public OptimizeScanWirelength API.
//
// Each test either verifies correctness guarantees (all cells preserved, no
// wirelength increase) or checks that the optimizer finds the known-optimal
// ordering for a hand-crafted instance.

#include <cmath>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "ClockDomain.hh"
#include "Opt.hh"
#include "ScanCell.hh"
#include "ScanPin.hh"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "utl/Logger.h"

namespace dft::test {
namespace {

// ---------------------------------------------------------------------------
// Mock helpers
// ---------------------------------------------------------------------------

// A ScanCell with user-specified placement and per-pin locations.
// isPlaced() returns true so OptimizeScanWirelength does not early-exit.
class PlacedScanCellMock : public ScanCell
{
 public:
  PlacedScanCellMock(const std::string& name,
                     odb::Point origin,
                     odb::Point si_loc,
                     odb::Point so_loc,
                     utl::Logger* logger)
      : ScanCell(name,
                 std::make_unique<ClockDomain>("clk", ClockEdge::Rising),
                 logger),
        origin_(origin),
        si_loc_(si_loc),
        so_loc_(so_loc)
  {
  }

  uint64_t getBits() const override { return 1; }
  void connectScanEnable(const ScanDriver&) const override {}
  void connectScanIn(const ScanDriver&) const override {}
  void connectScanOut(const ScanLoad&) const override {}
  ScanLoad getScanEnable() const override
  {
    return ScanLoad(static_cast<odb::dbITerm*>(nullptr));
  }
  ScanLoad getScanIn() const override
  {
    return ScanLoad(static_cast<odb::dbBTerm*>(nullptr));
  }
  ScanDriver getScanOut() const override
  {
    return ScanDriver(static_cast<odb::dbBTerm*>(nullptr));
  }

  odb::Point getOrigin() const override { return origin_; }
  bool isPlaced() const override { return true; }
  odb::Point getScanInLocation() const override { return si_loc_; }
  odb::Point getScanOutLocation() const override { return so_loc_; }

 private:
  odb::Point origin_;
  odb::Point si_loc_;
  odb::Point so_loc_;
};

// An unplaced ScanCell mock — isPlaced() returns false.
// Used to verify the early-exit guard in OptimizeScanWirelength.
class UnplacedScanCellMock : public ScanCell
{
 public:
  UnplacedScanCellMock(const std::string& name, utl::Logger* logger)
      : ScanCell(name,
                 std::make_unique<ClockDomain>("clk", ClockEdge::Rising),
                 logger)
  {
  }

  uint64_t getBits() const override { return 1; }
  void connectScanEnable(const ScanDriver&) const override {}
  void connectScanIn(const ScanDriver&) const override {}
  void connectScanOut(const ScanLoad&) const override {}
  ScanLoad getScanEnable() const override
  {
    return ScanLoad(static_cast<odb::dbITerm*>(nullptr));
  }
  ScanLoad getScanIn() const override
  {
    return ScanLoad(static_cast<odb::dbBTerm*>(nullptr));
  }
  ScanDriver getScanOut() const override
  {
    return ScanDriver(static_cast<odb::dbBTerm*>(nullptr));
  }

  odb::Point getOrigin() const override { return odb::Point(); }
  bool isPlaced() const override { return false; }
  odb::Point getScanInLocation() const override { return odb::Point(); }
  odb::Point getScanOutLocation() const override { return odb::Point(); }
};

// ---------------------------------------------------------------------------
// Utility functions
// ---------------------------------------------------------------------------

// Manhattan distance between two points (avoids int overflow).
int64_t manhattanDist(const odb::Point& a, const odb::Point& b)
{
  return std::abs(static_cast<int64_t>(a.x()) - b.x())
       + std::abs(static_cast<int64_t>(a.y()) - b.y());
}

// Total scan chain wirelength: sum of |SO[i] -> SI[i+1]| for i in [0, n-2].
int64_t chainWirelength(const std::vector<std::unique_ptr<ScanCell>>& cells)
{
  int64_t total = 0;
  for (size_t i = 0; i + 1 < cells.size(); i++) {
    total += manhattanDist(cells[i]->getScanOutLocation(),
                           cells[i + 1]->getScanInLocation());
  }
  return total;
}

// Collect cell names into a set for membership checks.
std::unordered_set<std::string> cellNameSet(
    const std::vector<std::unique_ptr<ScanCell>>& cells)
{
  std::unordered_set<std::string> names;
  for (const auto& c : cells) {
    names.insert(std::string(c->getName()));
  }
  return names;
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

// An empty vector should not crash or introduce any cells.
TEST(TestScanOptimizer, EmptyVector)
{
  utl::Logger logger;
  std::vector<std::unique_ptr<ScanCell>> cells;
  OptimizeScanWirelength(cells, &logger);
  EXPECT_TRUE(cells.empty());
}

// A single placed cell should not crash and must remain unchanged.
TEST(TestScanOptimizer, SingleCell)
{
  utl::Logger logger;
  std::vector<std::unique_ptr<ScanCell>> cells;
  cells.push_back(std::make_unique<PlacedScanCellMock>(
      "cell0",
      odb::Point(0, 0),
      odb::Point(0, 0),
      odb::Point(5, 0),
      &logger));
  OptimizeScanWirelength(cells, &logger);
  ASSERT_EQ(cells.size(), 1u);
  EXPECT_EQ(std::string(cells[0]->getName()), "cell0");
}

// If any cell is unplaced, OptimizeScanWirelength exits immediately without
// reordering (defensive guard). The input order must be fully preserved.
TEST(TestScanOptimizer, UnplacedCellsExitEarly)
{
  utl::Logger logger;
  std::vector<std::unique_ptr<ScanCell>> cells;
  cells.push_back(std::make_unique<UnplacedScanCellMock>("c0", &logger));
  cells.push_back(std::make_unique<UnplacedScanCellMock>("c1", &logger));
  cells.push_back(std::make_unique<UnplacedScanCellMock>("c2", &logger));

  OptimizeScanWirelength(cells, &logger);

  ASSERT_EQ(cells.size(), 3u);
  EXPECT_EQ(std::string(cells[0]->getName()), "c0");
  EXPECT_EQ(std::string(cells[1]->getName()), "c1");
  EXPECT_EQ(std::string(cells[2]->getName()), "c2");
}

// After any optimization, the exact same set of cells must be present —
// none lost, none duplicated.
TEST(TestScanOptimizer, AllCellsPreservedAfterOptimization)
{
  utl::Logger logger;
  // 8 cells at scattered positions in the plane.
  std::vector<odb::Point> positions = {{100, 200},
                                       {500, 100},
                                       {300, 400},
                                       {700, 50},
                                       {50, 600},
                                       {900, 300},
                                       {200, 700},
                                       {600, 500}};

  std::unordered_set<std::string> expected_names;
  std::vector<std::unique_ptr<ScanCell>> cells;
  for (size_t i = 0; i < positions.size(); i++) {
    const std::string name = "cell" + std::to_string(i);
    expected_names.insert(name);
    const auto& p = positions[i];
    // SO slightly to the right of SI to create mild asymmetry.
    cells.push_back(std::make_unique<PlacedScanCellMock>(
        name, p, p, odb::Point(p.x() + 5, p.y()), &logger));
  }

  OptimizeScanWirelength(cells, &logger);

  ASSERT_EQ(cells.size(), positions.size());
  EXPECT_EQ(cellNameSet(cells), expected_names);
}

// Five cells placed on a horizontal line (x = 0, 10, 20, 30, 40) with
// symmetric SI == SO == origin. Input is given in reverse order.
// The nearest-neighbour pass starts from the lower-left cell (origin) and
// produces the forward sequence, which has the minimum wirelength of 40.
TEST(TestScanOptimizer, HorizontalLineOptimalWirelength)
{
  utl::Logger logger;
  const int N = 5;
  // Input: c4(40), c3(30), c2(20), c1(10), c0(0) — reversed.
  std::vector<std::unique_ptr<ScanCell>> cells;
  for (int i = N - 1; i >= 0; i--) {
    const odb::Point p(i * 10, 0);
    cells.push_back(std::make_unique<PlacedScanCellMock>(
        "c" + std::to_string(i), p, p, p, &logger));
  }

  OptimizeScanWirelength(cells, &logger);

  ASSERT_EQ(cells.size(), static_cast<size_t>(N));
  // Optimal wirelength for 5 evenly-spaced cells on a line is 4 × 10 = 40.
  EXPECT_EQ(chainWirelength(cells), 40);
}

// Six cells on a horizontal line with asymmetric SI/SO:
//   SI is at x*10, SO is at x*10 + 5 (the scan signal "exits" rightward).
// In the reversed input order the total hop cost is 5 * 15 = 75.
// The optimal (forward) order has cost 5 * 5 = 25.
// 2-Opt should converge to the optimal ordering.
TEST(TestScanOptimizer, AsymmetricPinsTwoOptFindsOptimalOrder)
{
  utl::Logger logger;
  const int N = 6;
  // Input: c5, c4, c3, c2, c1, c0 (reversed).
  // Wirelength before:  SO[ci] = 10i+5, SI[c(i-1)] = 10(i-1)
  //   each hop = (10i+5) - 10(i-1) = 15  →  total = 5*15 = 75
  // Wirelength optimal: SO[ci] = 10i+5, SI[c(i+1)] = 10(i+1)
  //   each hop = 10(i+1) - (10i+5) = 5   →  total = 5*5  = 25
  std::vector<std::unique_ptr<ScanCell>> cells;
  for (int i = N - 1; i >= 0; i--) {
    const odb::Point origin(i * 10, 0);
    const odb::Point si(i * 10, 0);
    const odb::Point so(i * 10 + 5, 0);
    cells.push_back(std::make_unique<PlacedScanCellMock>(
        "c" + std::to_string(i), origin, si, so, &logger));
  }

  const int64_t before = chainWirelength(cells);
  ASSERT_EQ(before, 75);

  OptimizeScanWirelength(cells, &logger);
  const int64_t after = chainWirelength(cells);

  EXPECT_LT(after, before);
  EXPECT_EQ(after, 25);
  ASSERT_EQ(cells.size(), static_cast<size_t>(N));
}

// Or-Opt test: one outlier cell inserted in the middle of a compact cluster.
// NN starts at the lower-left corner and greedily builds c0→c1→...→c4,
// appending the outlier last. Or-Opt then confirms no better relocation exists.
// The test verifies that optimization does not increase wirelength and that
// the result is better than the bad input order.
TEST(TestScanOptimizer, OrOptDoesNotIncreaseWirelengthWithOutlier)
{
  utl::Logger logger;
  // Compact cluster: c0(0,0) to c4(40,0).
  // Outlier at x=1000, inserted between c1 and c2 in the input.
  std::vector<std::unique_ptr<ScanCell>> cells;
  for (int i = 0; i < 5; i++) {
    const odb::Point p(i * 10, 0);
    cells.push_back(std::make_unique<PlacedScanCellMock>(
        "c" + std::to_string(i), p, p, p, &logger));
  }
  cells.insert(cells.begin() + 2,
               std::make_unique<PlacedScanCellMock>(
                   "outlier",
                   odb::Point(1000, 0),
                   odb::Point(1000, 0),
                   odb::Point(1000, 0),
                   &logger));
  // Input order: c0, c1, outlier, c2, c3, c4.
  // Wirelength: 10 + 990 + 980 + 10 + 10 = 2000.
  const int64_t before = chainWirelength(cells);
  ASSERT_EQ(before, 2000);

  OptimizeScanWirelength(cells, &logger);
  const int64_t after = chainWirelength(cells);

  // Optimal: cluster first, outlier last → 40 + 960 = 1000.
  EXPECT_LE(after, before);
  EXPECT_EQ(after, 1000);
  ASSERT_EQ(cells.size(), 6u);
}

// ---------------------------------------------------------------------------
// Weighted distance tests (Item 2: vertical_weight parameter)
// ---------------------------------------------------------------------------

// Weighted wirelength: sum of |dx| + wv * |dy| for consecutive scan-out → scan-in.
int64_t weightedChainWirelength(
    const std::vector<std::unique_ptr<ScanCell>>& cells,
    double wv)
{
  int64_t total = 0;
  for (size_t i = 0; i + 1 < cells.size(); i++) {
    const auto& so = cells[i]->getScanOutLocation();
    const auto& si = cells[i + 1]->getScanInLocation();
    int64_t dx = std::abs(static_cast<int64_t>(so.x()) - si.x());
    int64_t dy = std::abs(static_cast<int64_t>(so.y()) - si.y());
    total += dx + static_cast<int64_t>(std::round(dy * wv));
  }
  return total;
}

// Passing vertical_weight = 1.0 explicitly must produce the same result as
// the default (unweighted) call — ensures backward compatibility.
TEST(TestScanOptimizer, WeightedDistanceDefaultIsUnweighted)
{
  utl::Logger logger;
  const int N = 6;

  // Build two identical copies of the same reversed horizontal chain.
  auto make_cells = [&]() {
    std::vector<std::unique_ptr<ScanCell>> cells;
    for (int i = N - 1; i >= 0; i--) {
      const odb::Point origin(i * 10, 0);
      const odb::Point si(i * 10, 0);
      const odb::Point so(i * 10 + 5, 0);
      cells.push_back(std::make_unique<PlacedScanCellMock>(
          "c" + std::to_string(i), origin, si, so, &logger));
    }
    return cells;
  };

  auto cells_default = make_cells();
  auto cells_explicit = make_cells();

  OptimizeScanWirelength(cells_default, &logger);
  OptimizeScanWirelength(cells_explicit, &logger, 1.0);

  ASSERT_EQ(cells_default.size(), cells_explicit.size());
  EXPECT_EQ(chainWirelength(cells_default), chainWirelength(cells_explicit));
  // Both should find the same ordering.
  for (size_t i = 0; i < cells_default.size(); i++) {
    EXPECT_EQ(std::string(cells_default[i]->getName()),
              std::string(cells_explicit[i]->getName()));
  }
}

// With a very high vertical weight, the optimizer should prefer orderings that
// minimise row crossings.
//
// Layout: two rows of 3 cells each.
//   Row 1 (y=100): d0(0,100)  d1(100,100)  d2(200,100)
//   Row 0 (y=0):   c0(0,0)    c1(100,0)    c2(200,0)
//
// Input order deliberately alternates rows: c0, d0, c1, d1, c2, d2.
// With wv=1.0, any serpentine is reasonable.
// With wv=10.0, the optimizer should strongly prefer finishing one row before
// crossing to the next, producing at most one row crossing.
TEST(TestScanOptimizer, WeightedDistancePreferHorizontal)
{
  utl::Logger logger;
  std::vector<std::unique_ptr<ScanCell>> cells;

  // Alternating-row input order.
  for (int i = 0; i < 3; i++) {
    odb::Point p0(i * 100, 0);
    odb::Point p1(i * 100, 100);
    cells.push_back(std::make_unique<PlacedScanCellMock>(
        "c" + std::to_string(i), p0, p0, p0, &logger));
    cells.push_back(std::make_unique<PlacedScanCellMock>(
        "d" + std::to_string(i), p1, p1, p1, &logger));
  }

  const int64_t before_weighted = weightedChainWirelength(cells, 10.0);
  OptimizeScanWirelength(cells, &logger, 10.0);
  const int64_t after_weighted = weightedChainWirelength(cells, 10.0);

  ASSERT_EQ(cells.size(), 6u);
  EXPECT_LT(after_weighted, before_weighted);

  // Count row crossings (transitions where dy != 0 between consecutive cells).
  int crossings = 0;
  for (size_t i = 0; i + 1 < cells.size(); i++) {
    if (cells[i]->getScanOutLocation().y()
        != cells[i + 1]->getScanInLocation().y()) {
      crossings++;
    }
  }
  // Should have at most 1 row crossing (traverse row 0, cross, traverse row 1).
  EXPECT_LE(crossings, 1);
}

// ---------------------------------------------------------------------------
// 3-Opt tests (Item 3: direction-preserving subtour swap)
// ---------------------------------------------------------------------------

// Construct a case where two interleaved subtours benefit from being swapped.
// The optimizer (2-Opt + 3-Opt + Or-Opt) should find the optimal ordering.
//
// Layout: 8 cells on a line at x = 0,10,20,...,70.
// Input order: 0, 4, 1, 5, 2, 6, 3, 7 (two interleaved ascending runs).
// Optimal order: 0,1,2,3,4,5,6,7 with wirelength 7*10 = 70.
TEST(TestScanOptimizer, ThreeOptSwapsInterleavedSubtours)
{
  utl::Logger logger;
  const int order[] = {0, 4, 1, 5, 2, 6, 3, 7};
  std::vector<std::unique_ptr<ScanCell>> cells;
  for (int idx : order) {
    const odb::Point p(idx * 10, 0);
    cells.push_back(std::make_unique<PlacedScanCellMock>(
        "c" + std::to_string(idx), p, p, p, &logger));
  }

  const int64_t before = chainWirelength(cells);
  OptimizeScanWirelength(cells, &logger);
  const int64_t after = chainWirelength(cells);

  ASSERT_EQ(cells.size(), 8u);
  EXPECT_LT(after, before);
  EXPECT_EQ(after, 70);
  EXPECT_EQ(cellNameSet(cells).size(), 8u);
}

// ---------------------------------------------------------------------------
// k-NN + large instance test (Item 1: candidate edge filtering)
// ---------------------------------------------------------------------------

// With 60 cells (> kMaxNeighbors=50), the k-NN filter is active, meaning not
// all edges are considered.  Verify that all cells are preserved and that
// wirelength does not increase compared to the input order.
TEST(TestScanOptimizer, LargeInstanceAllCellsPreserved)
{
  utl::Logger logger;
  const int N = 60;
  std::unordered_set<std::string> expected_names;
  std::vector<std::unique_ptr<ScanCell>> cells;

  // Cells on a grid: 6 rows × 10 columns, given in column-major order
  // (a deliberately bad ordering that zig-zags across rows).
  for (int col = 0; col < 10; col++) {
    for (int row = 0; row < 6; row++) {
      const int idx = col * 6 + row;
      const std::string name = "cell" + std::to_string(idx);
      expected_names.insert(name);
      const odb::Point p(col * 100, row * 100);
      cells.push_back(std::make_unique<PlacedScanCellMock>(
          name, p, p, odb::Point(p.x() + 5, p.y()), &logger));
    }
  }

  const int64_t before = chainWirelength(cells);
  OptimizeScanWirelength(cells, &logger);
  const int64_t after = chainWirelength(cells);

  ASSERT_EQ(cells.size(), static_cast<size_t>(N));
  EXPECT_EQ(cellNameSet(cells), expected_names);
  EXPECT_LE(after, before);
}

// Small chains (< 6 cells) should not crash in 3-Opt and still optimise via
// 2-Opt and Or-Opt.
TEST(TestScanOptimizer, SmallChainHandledCorrectly)
{
  utl::Logger logger;
  // 4 cells in reverse order on a line.
  std::vector<std::unique_ptr<ScanCell>> cells;
  for (int i = 3; i >= 0; i--) {
    const odb::Point p(i * 10, 0);
    cells.push_back(std::make_unique<PlacedScanCellMock>(
        "c" + std::to_string(i), p, p, p, &logger));
  }

  OptimizeScanWirelength(cells, &logger);
  ASSERT_EQ(cells.size(), 4u);
  // Optimal wirelength = 3 * 10 = 30.
  EXPECT_EQ(chainWirelength(cells), 30);
}

}  // namespace
}  // namespace dft::test
