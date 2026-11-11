// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Checks bin overflow against a hand computation.
//
// The core is split into a 4x4 grid of bins.  A bin holding more cell area
// than its capacity (target density x bin area) overflows by the excess.
// Fixed instances take up capacity too: they count as non-placeable area
// scaled by the target density.  The total overflow is the sum of the
// per-bin excesses divided by the movable cell area.

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "gpl/Replace.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "src/gpl/src/nesterovBase.h"
#include "src/gpl/src/nesterovPlace.h"
#include "src/gpl/src/placerBase.h"
#include "utl/Logger.h"
#include "utl/deleter.h"

namespace gpl {
namespace {

class OverflowTest : public ::testing::Test
{
 protected:
  static constexpr int kDbuPerMicron = 1000;
  static constexpr int kSiteWidth = 200;
  static constexpr int kRowHeight = 2000;
  static constexpr int kRowSites = 100;
  static constexpr int kRows = 10;
  // The core is kRowSites * kSiteWidth by kRows * kRowHeight: 20000 square.
  static constexpr int kCoreLx = kSiteWidth;
  static constexpr int kCoreLy = kRowHeight;
  static constexpr int kBins = 4;
  static constexpr int kBinSize = 5000;
  // Cells wider than sqrt(2) bins keep their own size in the density
  // computation; smaller ones are spread over a larger area.
  static constexpr int kCellSize = 8000;
  static constexpr float kTargetDensity = 0.7f;
  // gpl sums per-bin areas in float and truncates each to an integer DBU^2,
  // and 0.7f is not exactly 0.7.
  static constexpr double kTolerance = 1e-6;

  struct Cell
  {
    int lx;  // relative to the core origin
    int ly;
    bool fixed = false;
  };

  void SetUp() override
  {
    db_ = utl::UniquePtrWithDeleter<odb::dbDatabase>(odb::dbDatabase::create(),
                                                     odb::dbDatabase::destroy);
    db_->setLogger(&logger_);
    db_->setDbuPerMicron(kDbuPerMicron);

    odb::dbTech* tech = odb::dbTech::create(db_.get(), "tech");
    odb::dbTechLayer::create(tech, "metal1", odb::dbTechLayerType::ROUTING);
    odb::dbLib* lib = odb::dbLib::create(db_.get(), "lib", tech);
    odb::dbSite* site = odb::dbSite::create(lib, "site");
    site->setWidth(kSiteWidth);
    site->setHeight(kRowHeight);
    site->setClass(odb::dbSiteClass::CORE);

    odb::dbChip* chip = odb::dbChip::create(db_.get(), tech);
    block_ = odb::dbBlock::create(chip, "top");
    block_->setDieArea(odb::Rect(
        0, 0, (kRowSites + 2) * kSiteWidth, (kRows + 2) * kRowHeight));
    for (int row = 0; row < kRows; ++row) {
      odb::dbRow::create(block_,
                         ("row" + std::to_string(row)).c_str(),
                         site,
                         kSiteWidth,
                         (row + 1) * kRowHeight,
                         odb::dbOrientType::R0,
                         odb::dbRowDir::HORIZONTAL,
                         kRowSites,
                         kSiteWidth);
    }
    block_->setCoreArea(block_->computeCoreArea());

    // Four rows tall: a multi-row standard cell, not a macro.
    master_ = odb::dbMaster::create(lib, "core_cell");
    master_->setType(odb::dbMasterType(odb::dbMasterType::CORE));
    master_->setWidth(kCellSize);
    master_->setHeight(kCellSize);
    master_->setSite(site);
    master_->setFrozen();
  }

  // Places the cells and runs the density setup that computes overflow.
  NesterovBase& place(const std::vector<Cell>& cells)
  {
    for (std::size_t i = 0; i < cells.size(); ++i) {
      odb::dbInst* inst = odb::dbInst::create(
          block_, master_, ("u_" + std::to_string(i)).c_str());
      inst->setLocation(kCoreLx + cells[i].lx, kCoreLy + cells[i].ly);
      inst->setPlacementStatus(cells[i].fixed ? odb::dbPlacementStatus::FIRM
                                              : odb::dbPlacementStatus::PLACED);
    }

    PlaceOptions options;
    options.density = kTargetDensity;
    // Keep cells exactly where the test put them.
    options.initialPlacePerturbationDist = 0.0f;
    options.binGridCntX = kBins;
    options.binGridCntY = kBins;

    const PlacerBaseVars pb_vars(options);
    pbc_ = std::make_shared<PlacerBaseCommon>(db_.get(), pb_vars, &logger_);
    pb_ = std::make_shared<PlacerBase>(
        db_.get(), pbc_, &logger_, /* check_density = */ false);
    const NesterovBaseVars nb_vars(options);
    nbc_ = std::make_shared<NesterovBaseCommon>(
        nb_vars, pbc_, &logger_, /* num_threads = */ 1, Clusters{});
    nb_ = std::make_shared<NesterovBase>(nb_vars, pb_, nbc_, &logger_);
    np_vars_ = std::make_unique<NesterovPlaceVars>(options, nbc_->getHpwl());
    nb_->setNpVars(np_vars_.get());
    nb_->initDensity1();

    EXPECT_EQ(nb_->getBinCntX(), kBins);
    EXPECT_EQ(nb_->getBinCntY(), kBins);
    return *nb_;
  }

  static int64_t overlap(const Cell& cell, int bin_x, int bin_y)
  {
    const int lx = std::max(cell.lx, bin_x * kBinSize);
    const int ly = std::max(cell.ly, bin_y * kBinSize);
    const int ux = std::min(cell.lx + kCellSize, (bin_x + 1) * kBinSize);
    const int uy = std::min(cell.ly + kCellSize, (bin_y + 1) * kBinSize);
    if (lx >= ux || ly >= uy) {
      return 0;
    }
    return static_cast<int64_t>(ux - lx) * (uy - ly);
  }

  // The overflow definition, computed independently of gpl.
  static double expectedOverflow(const std::vector<Cell>& cells)
  {
    const double capacity
        = kTargetDensity * static_cast<double>(kBinSize) * kBinSize;
    double excess = 0;
    for (int bx = 0; bx < kBins; ++bx) {
      for (int by = 0; by < kBins; ++by) {
        double used = 0;
        for (const Cell& cell : cells) {
          const double area = overlap(cell, bx, by);
          used += cell.fixed ? area * kTargetDensity : area;
        }
        excess += std::max(0.0, used - capacity);
      }
    }
    double movable_area = 0;
    for (const Cell& cell : cells) {
      if (!cell.fixed) {
        movable_area += static_cast<double>(kCellSize) * kCellSize;
      }
    }
    return excess / movable_area;
  }

  utl::Logger logger_;
  utl::UniquePtrWithDeleter<odb::dbDatabase> db_;
  odb::dbBlock* block_ = nullptr;
  odb::dbMaster* master_ = nullptr;
  std::shared_ptr<PlacerBaseCommon> pbc_;
  std::shared_ptr<PlacerBase> pb_;
  std::shared_ptr<NesterovBaseCommon> nbc_;
  std::shared_ptr<NesterovBase> nb_;
  std::unique_ptr<NesterovPlaceVars> np_vars_;
};

// Each cell straddles a 2x2 block of bins and puts 4000 x 4000 = 16e6 in
// each, under the 0.7 x 25e6 = 17.5e6 capacity.
TEST_F(OverflowTest, SpreadCellsDoNotOverflow)
{
  const std::vector<Cell> cells{{1000, 1000}, {11000, 11000}};
  ASSERT_EQ(expectedOverflow(cells), 0.0);

  const NesterovBase& nb = place(cells);
  EXPECT_EQ(nb.getSumOverflow(), 0.0f);
  EXPECT_EQ(nb.getSumOverflowUnscaled(), 0.0f);
}

// Four cells on the same spot put 64e6 in each of four bins: 46.5e6 over
// capacity per bin, 186e6 in total, over 256e6 of cell area.
TEST_F(OverflowTest, StackedCellsOverflow)
{
  const std::vector<Cell> cells(4, Cell{1000, 1000});
  ASSERT_NEAR(expectedOverflow(cells), 186.0 / 256.0, kTolerance);

  const NesterovBase& nb = place(cells);
  EXPECT_NEAR(nb.getSumOverflow(), 186.0 / 256.0, kTolerance);
  EXPECT_NEAR(nb.getSumOverflowUnscaled(), 186.0 / 256.0, kTolerance);
}

// A cell in the core corner covers its first bin fully, with 25e6 against
// 17.5e6 of capacity, and overflows it by 7.5e6. It puts only 15e6 in each
// edge bin and 9e6 in the diagonal bin, which stay under capacity: 7.5e6
// over 64e6 of cell area.
TEST_F(OverflowTest, PartialOverlapOverflowsOnlyFullBins)
{
  const std::vector<Cell> cells{{0, 0}};
  ASSERT_NEAR(expectedOverflow(cells), 7.5 / 64.0, kTolerance);

  const NesterovBase& nb = place(cells);
  EXPECT_NEAR(nb.getSumOverflow(), 7.5 / 64.0, kTolerance);
  EXPECT_NEAR(nb.getSumOverflowUnscaled(), 7.5 / 64.0, kTolerance);
}

// A fixed instance under a movable one uses 0.7 of its overlap with each
// bin as capacity.  Both sit 4000 wide in each of two bin columns and
// 3000 / 5000 tall in two bin rows, so the bins hold 12e6 + 8.4e6 and
// 20e6 + 14e6 against 17.5e6 of capacity: 2 x (2.9e6 + 16.5e6) = 38.8e6 over
// 64e6 of movable area.  gpl snaps fixed instances outward to the site grid,
// so this one starts on a row boundary to keep its size.
TEST_F(OverflowTest, FixedInstanceUsesCapacity)
{
  const std::vector<Cell> cells{{1000, 2000, /* fixed = */ true}, {1000, 2000}};
  ASSERT_NEAR(expectedOverflow(cells), 38.8 / 64.0, kTolerance);

  const NesterovBase& nb = place(cells);
  EXPECT_NEAR(nb.getSumOverflow(), 38.8 / 64.0, kTolerance);
  EXPECT_NEAR(nb.getSumOverflowUnscaled(), 38.8 / 64.0, kTolerance);
}

}  // namespace
}  // namespace gpl
