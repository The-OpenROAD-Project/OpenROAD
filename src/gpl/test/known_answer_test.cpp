// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Global placement on synthetic designs built so that the good placement is
// known in advance, checked for the structure the placer must recover. Each
// connected design runs with and without initial placement; without it,
// every cell starts at the core center and Nesterov placement alone has to
// find the structure.

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <string>
#include <vector>

#include "gpl/Replace.h"
#include "gpl_design_builder.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "tst/fixture.h"

namespace gpl {
namespace {

// Each value's rank in v, with ties given their mean rank.
std::vector<double> ranks(const std::vector<double>& v)
{
  std::vector<std::size_t> order(v.size());
  std::iota(order.begin(), order.end(), 0);
  std::ranges::sort(order, [&](const std::size_t a, const std::size_t b) {
    return v[a] < v[b];
  });
  std::vector<double> rank(v.size());
  for (std::size_t first = 0; first < order.size();) {
    std::size_t last = first;
    while (last + 1 < order.size() && v[order[last + 1]] == v[order[first]]) {
      ++last;
    }
    for (std::size_t k = first; k <= last; ++k) {
      rank[order[k]] = (first + last) / 2.0;
    }
    first = last + 1;
  }
  return rank;
}

// Spearman rank correlation: 1 when b increases with a.
double spearman(const std::vector<double>& a, const std::vector<double>& b)
{
  const std::vector<double> ra = ranks(a);
  const std::vector<double> rb = ranks(b);
  const double mean = (a.size() - 1) / 2.0;
  double cov = 0;
  double var_a = 0;
  double var_b = 0;
  for (std::size_t i = 0; i < a.size(); ++i) {
    cov += (ra[i] - mean) * (rb[i] - mean);
    var_a += (ra[i] - mean) * (ra[i] - mean);
    var_b += (rb[i] - mean) * (rb[i] - mean);
  }
  return cov / std::sqrt(var_a * var_b);
}

odb::Point center(odb::dbInst* inst)
{
  return inst->getBBox()->getBox().center();
}

class KnownAnswerTest : public tst::Fixture
{
 protected:
  PlaceResult place(const PlaceOptions& options)
  {
    Replace replace{db_.get(), sta_.get(), nullptr, nullptr, nullptr, &logger_};
    return replace.doPlace(1, options);
  }
};

class ConnectedTest : public KnownAnswerTest,
                      public testing::WithParamInterface<bool>
{
 protected:
  PlaceResult place()
  {
    PlaceOptions options;
    if (!GetParam()) {
      options.initialPlaceMaxIter = 0;
    }
    return KnownAnswerTest::place(options);
  }
};

// A chain between IOs at mid height on the left and right die edges lines
// up between them in chain order.
TEST_P(ConnectedTest, ChainLinesUpInOrder)
{
  DesignBuilder builder(db_.get());
  const std::vector<odb::dbInst*> chain = makeChain(builder, 20);

  EXPECT_EQ(place().stop, PlaceStop::kConverged);

  std::vector<double> order;
  std::vector<double> x;
  for (std::size_t i = 0; i < chain.size(); ++i) {
    order.push_back(i);
    x.push_back(center(chain[i]).x());
    EXPECT_LE(std::abs(center(chain[i]).y() - builder.core().yCenter()),
              DesignBuilder::kRowHeight)
        << chain[i]->getName();
  }
  EXPECT_GT(spearman(order, x), 0.95);
}

// A lattice whose boundary cells are tied to IOs at matching places on the
// die edges is placed as a lattice: column order along x, row order along y.
// The cells cover 60% of the core.
TEST_P(ConnectedTest, LatticeKeepsItsShape)
{
  DesignBuilder builder(db_.get(), /*cell_sites=*/6);
  const std::vector<std::vector<odb::dbInst*>> lattice
      = makeLattice(builder, 10, 10);

  EXPECT_EQ(place().stop, PlaceStop::kConverged);

  std::vector<double> column;
  std::vector<double> row;
  std::vector<double> x;
  std::vector<double> y;
  for (std::size_t i = 0; i < lattice.size(); ++i) {
    for (std::size_t j = 0; j < lattice[i].size(); ++j) {
      column.push_back(i);
      row.push_back(j);
      x.push_back(center(lattice[i][j]).x());
      y.push_back(center(lattice[i][j]).y());
    }
  }
  EXPECT_GT(spearman(column, x), 0.95);
  EXPECT_GT(spearman(row, y), 0.95);

  // Ordered is not enough: the lattice must also spread over the core.
  odb::Rect extent;
  extent.mergeInit();
  for (const std::vector<odb::dbInst*>& column_cells : lattice) {
    for (odb::dbInst* inst : column_cells) {
      extent.merge(inst->getBBox()->getBox());
    }
  }
  const odb::Rect core = builder.core();
  // Measured 0.88 of the core width and 0.92 of its height.
  EXPECT_GT(extent.dx(), 0.8 * core.dx());
  EXPECT_GT(extent.dy(), 0.8 * core.dy());
}

// Two lattices with no nets between them, anchored to opposite die edges,
// end up on their anchors' sides of the core, apart from each other.
TEST_P(ConnectedTest, UnrelatedClustersStayApart)
{
  DesignBuilder builder(db_.get());
  const TwoClusters clusters = makeTwoClusters(builder, 5);

  EXPECT_EQ(place().stop, PlaceStop::kConverged);

  const int middle = builder.core().xCenter();
  auto bbox = [](const std::vector<odb::dbInst*>& cells) {
    odb::Rect box;
    box.mergeInit();
    for (odb::dbInst* inst : cells) {
      box.merge(inst->getBBox()->getBox());
    }
    return box;
  };
  for (odb::dbInst* inst : clusters.left) {
    EXPECT_LT(center(inst).x(), middle) << inst->getName();
  }
  for (odb::dbInst* inst : clusters.right) {
    EXPECT_GT(center(inst).x(), middle) << inst->getName();
  }
  const odb::Rect left = bbox(clusters.left);
  const odb::Rect right = bbox(clusters.right);
  const int64_t shared
      = left.overlaps(right) ? left.intersect(right).area() : 0;
  EXPECT_LT(shared, 0.1 * std::min(left.area(), right.area()));
}

INSTANTIATE_TEST_SUITE_P(Placement,
                         ConnectedTest,
                         testing::Bool(),
                         [](const testing::TestParamInfo<bool>& info) {
                           return std::string(info.param ? "WithInitialPlace"
                                                         : "NesterovOnly");
                         });

// Checks that cells, together 60% of the core, fill it evenly: the cell area
// in each tile of a 4 x 4 grid over the core is within 0.5 to 1.5 times the
// mean. Each tile covers 2 x 2 of gpl's 8 x 8 density bins, the grid its
// overflow is measured on; tiles that cut across bins see unevenness the
// overflow target allows.
void expectEvenlySpread(const std::vector<odb::dbInst*>& cells,
                        const odb::Rect& core)
{
  constexpr int kTiles = 4;
  const int width = core.dx() / kTiles;
  const int height = core.dy() / kTiles;
  std::vector<double> used(kTiles * kTiles);
  for (odb::dbInst* inst : cells) {
    const odb::Rect box = inst->getBBox()->getBox();
    for (int tx = 0; tx < kTiles; ++tx) {
      for (int ty = 0; ty < kTiles; ++ty) {
        const odb::Rect tile(core.xMin() + tx * width,
                             core.yMin() + ty * height,
                             core.xMin() + (tx + 1) * width,
                             core.yMin() + (ty + 1) * height);
        if (tile.overlaps(box)) {
          used[tx * kTiles + ty] += tile.intersect(box).area();
        }
      }
    }
  }
  const double mean
      = std::accumulate(used.begin(), used.end(), 0.0) / used.size();
  for (std::size_t t = 0; t < used.size(); ++t) {
    EXPECT_GT(used[t], 0.5 * mean) << "tile " << t;
    EXPECT_LT(used[t], 1.5 * mean) << "tile " << t;
  }
}

// Cells on no nets, piled up in the lower-left corner of the core, are
// spread by Nesterov placement's density force alone until every part of
// the core holds about the same cell area. Measured 0.76 to 1.29 times the
// mean tile area.
TEST_F(KnownAnswerTest, DensitySpreadsUnconnectedCells)
{
  constexpr int kCellSites = 6;
  DesignBuilder builder(db_.get(), kCellSites);
  const std::vector<odb::dbInst*> cells = makeUnconnected(builder, 100);
  const odb::Rect core = builder.core();
  // A 10 x 10 pile of overlapping cells, a half cell wide and a quarter row
  // apart, covering the lower-left third of the core in each direction.
  constexpr int kPerRow = 10;
  constexpr int kStepX = kCellSites * DesignBuilder::kSiteWidth / 2;
  constexpr int kStepY = DesignBuilder::kRowHeight / 4;
  for (std::size_t k = 0; k < cells.size(); ++k) {
    cells[k]->setLocation(core.xMin() + (k % kPerRow) * kStepX,
                          core.yMin() + (k / kPerRow) * kStepY);
    cells[k]->setPlacementStatus(odb::dbPlacementStatus::PLACED);
  }

  PlaceOptions options;
  options.initialPlaceMaxIter = 0;
  options.uniformTargetDensityMode = true;
  EXPECT_EQ(place(options).stop, PlaceStop::kConverged);
  expectEvenlySpread(cells, core);
}

// Unplaced cells on no nets, through the whole flow: initial placement
// leaves them at the core center, and Nesterov placement spreads them over
// the core. When no net anchors any position, initial placement must still
// hold each cell where it is, not move them all to one point. Measured 0.73
// to 1.21 times the mean tile area.
TEST_F(KnownAnswerTest, UnplacedUnconnectedCellsSpread)
{
  DesignBuilder builder(db_.get(), /*cell_sites=*/6);
  const std::vector<odb::dbInst*> cells = makeUnconnected(builder, 100);

  PlaceOptions options;
  options.uniformTargetDensityMode = true;
  EXPECT_EQ(place(options).stop, PlaceStop::kConverged);
  expectEvenlySpread(cells, builder.core());
}

}  // namespace
}  // namespace gpl
