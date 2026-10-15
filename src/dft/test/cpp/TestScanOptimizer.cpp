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
#include "KMeans.hh"
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

// ---------------------------------------------------------------------------
// 3-Opt tests (Item 3: direction-preserving subtour swap)
// ---------------------------------------------------------------------------

// Construct a case where two interleaved subtours benefit from being swapped.
// The optimizer (2-Opt + 3-Opt) should find the optimal ordering.
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
// 2-Opt.
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

// ---------------------------------------------------------------------------
// KMeansClusters tests
// ---------------------------------------------------------------------------

// Helper: build a PlacedScanCellMock at the given point (SI == SO == origin).
std::unique_ptr<PlacedScanCellMock> MakeCell(const std::string& name,
                                              odb::Point p,
                                              utl::Logger* logger)
{
  return std::make_unique<PlacedScanCellMock>(name, p, p, p, logger);
}

// k=1: every cell lands in cluster 0 regardless of position.
TEST(TestKMeans, KEqualsOne)
{
  utl::Logger logger;
  std::vector<std::unique_ptr<PlacedScanCellMock>> owned;
  std::vector<ScanCell*> cells;
  for (int i = 0; i < 5; i++) {
    owned.push_back(MakeCell("c" + std::to_string(i), {i * 100, 0}, &logger));
    cells.push_back(owned.back().get());
  }

  const std::vector<int> assignments = KMeansClusters(cells, 1);

  ASSERT_EQ(assignments.size(), 5u);
  for (int a : assignments) {
    EXPECT_EQ(a, 0);
  }
}

// n < k: degenerate case must not crash and all cells fall in cluster 0.
TEST(TestKMeans, CellsFewerThanK)
{
  utl::Logger logger;
  std::vector<std::unique_ptr<PlacedScanCellMock>> owned;
  std::vector<ScanCell*> cells;
  for (int i = 0; i < 3; i++) {
    owned.push_back(MakeCell("c" + std::to_string(i), {i * 100, 0}, &logger));
    cells.push_back(owned.back().get());
  }

  const std::vector<int> assignments = KMeansClusters(cells, 5);

  ASSERT_EQ(assignments.size(), 3u);
  for (int a : assignments) {
    EXPECT_EQ(a, 0);
  }
}

// 16 cells in 4 tight, well-separated quadrants — k=4 must recover them.
// Intra-cluster max Manhattan distance: 200.  Inter-cluster min: ~800.
TEST(TestKMeans, QuadrantRecovery)
{
  utl::Logger logger;
  // Each quadrant has 4 cells in a 100×100 box; quadrant corners are at
  // (0,0), (1000,0), (0,1000), (1000,1000).
  const odb::Point qoffsets[4] = {{0, 0}, {1000, 0}, {0, 1000}, {1000, 1000}};
  const odb::Point within[4] = {{0, 0}, {100, 0}, {0, 100}, {100, 100}};

  std::vector<std::unique_ptr<PlacedScanCellMock>> owned;
  std::vector<ScanCell*> cells;
  for (int q = 0; q < 4; q++) {
    for (int w = 0; w < 4; w++) {
      odb::Point p(qoffsets[q].x() + within[w].x(),
                   qoffsets[q].y() + within[w].y());
      owned.push_back(
          MakeCell("c" + std::to_string(q * 4 + w), p, &logger));
      cells.push_back(owned.back().get());
    }
  }

  const std::vector<int> assignments = KMeansClusters(cells, 4);

  ASSERT_EQ(assignments.size(), 16u);

  // All four cells in each quadrant must share the same cluster label.
  for (int q = 0; q < 4; q++) {
    const int base = q * 4;
    EXPECT_EQ(assignments[base + 1], assignments[base]) << "quadrant " << q;
    EXPECT_EQ(assignments[base + 2], assignments[base]) << "quadrant " << q;
    EXPECT_EQ(assignments[base + 3], assignments[base]) << "quadrant " << q;
  }

  // All four quadrants must have distinct cluster labels.
  EXPECT_NE(assignments[0], assignments[4]);
  EXPECT_NE(assignments[0], assignments[8]);
  EXPECT_NE(assignments[0], assignments[12]);
  EXPECT_NE(assignments[4], assignments[8]);
  EXPECT_NE(assignments[4], assignments[12]);
  EXPECT_NE(assignments[8], assignments[12]);
}

// Two calls with identical input must produce identical output.
TEST(TestKMeans, Deterministic)
{
  utl::Logger logger;
  std::vector<std::unique_ptr<PlacedScanCellMock>> owned;
  std::vector<ScanCell*> cells;
  // 20 cells at pseudo-scattered positions.
  for (int i = 0; i < 20; i++) {
    odb::Point p(i * 37, (i * 53) % 200);
    owned.push_back(MakeCell("c" + std::to_string(i), p, &logger));
    cells.push_back(owned.back().get());
  }

  const std::vector<int> a1 = KMeansClusters(cells, 4);
  const std::vector<int> a2 = KMeansClusters(cells, 4);

  EXPECT_EQ(a1, a2);
}

// n=1000 cells: algorithm must terminate and return valid assignments.
TEST(TestKMeans, ConvergesOnLargeInput)
{
  utl::Logger logger;
  const int N = 1000;
  std::vector<std::unique_ptr<PlacedScanCellMock>> owned;
  std::vector<ScanCell*> cells;

  // Deterministic pseudo-random positions via a simple LCG.
  int rx = 12345, ry = 67890;
  for (int i = 0; i < N; i++) {
    rx = (rx * 1103515245 + 12345) & 0x7fffffff;
    ry = (ry * 1103515245 + 12345) & 0x7fffffff;
    odb::Point p(rx % 100000, ry % 100000);
    owned.push_back(MakeCell("c" + std::to_string(i), p, &logger));
    cells.push_back(owned.back().get());
  }

  const std::vector<int> assignments = KMeansClusters(cells, 4);

  ASSERT_EQ(assignments.size(), static_cast<size_t>(N));
  for (int a : assignments) {
    EXPECT_GE(a, 0);
    EXPECT_LT(a, 4);
  }
}

}  // namespace
}  // namespace dft::test
