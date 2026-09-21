// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

// Tests for Opendp's placement density queries, the ones rsz uses to look
// for room to drop a buffer into.
//
// The fixture builds a floorplan whose numbers stay easy to check by hand:
// a 14um x 24um die under a 7 x 12 grid of 2um GCells, with 20 rows filling
// the lower left 10um x 20um of it.  A GCell is 2 rows of 20 sites, so it
// holds exactly 20 of the 2-site core cells, and the two GCell columns and
// rows past the core hold no placement site at all.

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "dpl/Opendp.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "tst/db_fixture.h"

namespace dpl {

namespace {

class PlacementDensityTest : public tst::DbFixture
{
 protected:
  static constexpr int kDbuPerMicron = 1000;
  static constexpr int kSiteWidth = 100;
  static constexpr int kRowHeight = 1000;
  static constexpr int kRowSites = 100;
  static constexpr int kRows = 20;
  static constexpr int kCoreWidth = kRowSites * kSiteWidth;  // 10000
  static constexpr int kCoreHeight = kRows * kRowHeight;     // 20000

  static constexpr int kGCellSize = 2000;
  // The rows start at the die's lower left but stop two GCells short of its
  // upper right, so the GCells past kCoreGCellsX/Y hold no core area.
  static constexpr int kMargin = 2 * kGCellSize;
  static constexpr int kDieWidth = kCoreWidth + kMargin;         // 14000
  static constexpr int kDieHeight = kCoreHeight + kMargin;       // 24000
  static constexpr int kGCellsX = kDieWidth / kGCellSize;        // 7
  static constexpr int kGCellsY = kDieHeight / kGCellSize;       // 12
  static constexpr int kCoreGCellsX = kCoreWidth / kGCellSize;   // 5
  static constexpr int kCoreGCellsY = kCoreHeight / kGCellSize;  // 10

  // Core cells are two sites wide and one row tall.
  static constexpr int kCellWidth = 2 * kSiteWidth;
  static constexpr int kCellsPerGCellRow = kGCellSize / kCellWidth;  // 10
  static constexpr int kRowsPerGCell = kGCellSize / kRowHeight;      // 2
  static constexpr int kCellsPerGCell = kCellsPerGCellRow * kRowsPerGCell;

  void SetUp() override
  {
    db_->setDbuPerMicron(kDbuPerMicron);
    odb::dbTech* tech = odb::dbTech::create(db_.get(), "tech");
    odb::dbTechLayer::create(tech, "metal1", odb::dbTechLayerType::ROUTING);

    lib_ = odb::dbLib::create(db_.get(), "lib", tech);
    site_ = odb::dbSite::create(lib_, "site");
    site_->setWidth(kSiteWidth);
    site_->setHeight(kRowHeight);
    site_->setClass(odb::dbSiteClass::CORE);

    odb::dbChip* chip = odb::dbChip::create(db_.get(), tech);
    block_ = odb::dbBlock::create(chip, "top");
    block_->setDieArea(odb::Rect(0, 0, kDieWidth, kDieHeight));
    for (int row = 0; row < kRows; ++row) {
      odb::dbRow::create(block_,
                         ("row" + std::to_string(row)).c_str(),
                         site_,
                         0,
                         row * kRowHeight,
                         odb::dbOrientType::R0,
                         odb::dbRowDir::HORIZONTAL,
                         kRowSites,
                         kSiteWidth);
    }
    block_->setCoreArea(block_->computeCoreArea());

    core_master_ = makeMaster(
        "core_cell", odb::dbMasterType::CORE, kCellWidth, kRowHeight);
  }

  odb::dbMaster* makeMaster(const char* name,
                            odb::dbMasterType::Value type,
                            int width,
                            int height)
  {
    odb::dbMaster* master = odb::dbMaster::create(lib_, name);
    master->setType(odb::dbMasterType(type));
    master->setWidth(width);
    master->setHeight(height);
    master->setSite(site_);
    master->setFrozen();
    return master;
  }

  odb::dbInst* placeInst(odb::dbMaster* master,
                         int x,
                         int y,
                         odb::dbPlacementStatus::Value status
                         = odb::dbPlacementStatus::PLACED)
  {
    odb::dbInst* inst = odb::dbInst::create(
        block_, master, ("u" + std::to_string(inst_count_++)).c_str());
    inst->setLocation(x, y);
    inst->setPlacementStatus(status);
    return inst;
  }

  // The routing GCell grid global routing would have left behind.
  void makeGCellGrid()
  {
    odb::dbGCellGrid* grid = odb::dbGCellGrid::create(block_);
    grid->addGridPatternX(0, kGCellsX, kGCellSize);
    grid->addGridPatternY(0, kGCellsY, kGCellSize);
  }

  // The extent of GCell (gx, gy).
  static odb::Rect gcellRect(int gx, int gy)
  {
    return {gx * kGCellSize,
            gy * kGCellSize,
            (gx + 1) * kGCellSize,
            (gy + 1) * kGCellSize};
  }

  // Packs num_cells core cells into GCell (gx, gy), filling it row by row
  // from its lower left corner.
  void fillGCell(int gx, int gy, int num_cells)
  {
    EXPECT_LE(num_cells, kCellsPerGCell);
    for (int i = 0; i < num_cells; ++i) {
      placeInst(core_master_,
                (gx * kGCellSize) + ((i % kCellsPerGCellRow) * kCellWidth),
                (gy * kGCellSize) + ((i / kCellsPerGCellRow) * kRowHeight));
    }
  }

  // The density queries only need the grid, so they run off the cheap init.
  Opendp& makeOpendp()
  {
    opendp_ = std::make_unique<Opendp>(db_.get(), &logger_);
    opendp_->initPlacementGrid();
    return *opendp_;
  }

  odb::dbLib* lib_ = nullptr;
  odb::dbSite* site_ = nullptr;
  odb::dbBlock* block_ = nullptr;
  odb::dbMaster* core_master_ = nullptr;
  std::unique_ptr<Opendp> opendp_;
  int inst_count_ = 0;
};

TEST_F(PlacementDensityTest, EmptyCoreIsZeroDensity)
{
  Opendp& dp = makeOpendp();

  EXPECT_DOUBLE_EQ(dp.getPlacementDensity(gcellRect(2, 5)), 0.0);
  EXPECT_DOUBLE_EQ(dp.getPlacementDensity(block_->getCoreArea()), 0.0);
}

TEST_F(PlacementDensityTest, DensityIsOccupiedOverPlaceableArea)
{
  fillGCell(0, 0, kCellsPerGCell);      // full
  fillGCell(1, 0, kCellsPerGCell / 2);  // half
  Opendp& dp = makeOpendp();

  EXPECT_DOUBLE_EQ(dp.getPlacementDensity(gcellRect(0, 0)), 1.0);
  EXPECT_DOUBLE_EQ(dp.getPlacementDensity(gcellRect(1, 0)), 0.5);
  EXPECT_DOUBLE_EQ(dp.getPlacementDensity(gcellRect(3, 3)), 0.0);

  // A region spanning both: 30 cells over 2 GCells worth of sites.
  const odb::Rect both(0, 0, 2 * kGCellSize, kGCellSize);
  EXPECT_DOUBLE_EQ(dp.getPlacementDensity(both), 0.75);
}

TEST_F(PlacementDensityTest, RegionNeedNotFollowGCellOrRowBoundaries)
{
  // One cell at the origin: 200 x 1000 DBU.
  fillGCell(0, 0, 1);
  Opendp& dp = makeOpendp();

  // A region covering exactly that cell.
  EXPECT_DOUBLE_EQ(
      dp.getPlacementDensity(odb::Rect(0, 0, kCellWidth, kRowHeight)), 1.0);
  // Half of it, cut across the middle of the cell and of the row.
  EXPECT_DOUBLE_EQ(
      dp.getPlacementDensity(odb::Rect(0, 0, kCellWidth / 2, kRowHeight / 2)),
      1.0);
  // The cell fills a quarter of this 2x2-cell region.
  EXPECT_DOUBLE_EQ(
      dp.getPlacementDensity(odb::Rect(0, 0, 2 * kCellWidth, 2 * kRowHeight)),
      0.25);
}

TEST_F(PlacementDensityTest, RegionWithoutSitesIsFull)
{
  // A hard blockage over GCell (0, 0) leaves it with no legal site.
  odb::dbBlockage::create(block_, 0, 0, kGCellSize, kGCellSize);
  Opendp& dp = makeOpendp();

  // Nothing fits there, so it reads as full rather than as the emptiest
  // spot in the design.
  EXPECT_DOUBLE_EQ(dp.getPlacementDensity(gcellRect(0, 0)), 1.0);
  EXPECT_DOUBLE_EQ(dp.getPlacementDensity(gcellRect(1, 0)), 0.0);

  // So is a region entirely off the die.
  EXPECT_DOUBLE_EQ(
      dp.getPlacementDensity(odb::Rect(-5000, -5000, -1000, -1000)), 1.0);
}

TEST_F(PlacementDensityTest, FixedMacroOccupiesItsArea)
{
  odb::dbMaster* macro
      = makeMaster("macro", odb::dbMasterType::BLOCK, kGCellSize, kGCellSize);
  placeInst(macro, 3 * kGCellSize, 0, odb::dbPlacementStatus::FIRM);
  Opendp& dp = makeOpendp();

  EXPECT_DOUBLE_EQ(dp.getPlacementDensity(gcellRect(3, 0)), 1.0);
  EXPECT_DOUBLE_EQ(dp.getPlacementDensity(gcellRect(4, 0)), 0.0);
}

TEST_F(PlacementDensityTest, DensityIsRecomputedAfterInstancesMove)
{
  fillGCell(0, 0, kCellsPerGCell);
  Opendp& dp = makeOpendp();

  ASSERT_DOUBLE_EQ(dp.getPlacementDensity(gcellRect(0, 0)), 1.0);
  ASSERT_DOUBLE_EQ(dp.getPlacementDensity(gcellRect(4, 9)), 0.0);

  // Move everything to the far corner, the way an optimization step would.
  for (odb::dbInst* inst : block_->getInsts()) {
    const odb::Point origin = inst->getLocation();
    inst->setLocation(origin.x() + (4 * kGCellSize),
                      origin.y() + (9 * kGCellSize));
  }

  // No re-init, no explicit invalidation: the next query sees the move.
  EXPECT_DOUBLE_EQ(dp.getPlacementDensity(gcellRect(0, 0)), 0.0);
  EXPECT_DOUBLE_EQ(dp.getPlacementDensity(gcellRect(4, 9)), 1.0);
}

TEST_F(PlacementDensityTest, GCellDensitiesCoverTheRequestedRegion)
{
  makeGCellGrid();
  Opendp& dp = makeOpendp();

  // A region covering a single GCell.
  const std::vector<GCellDensity> one = dp.getGCellDensities(gcellRect(2, 3));
  ASSERT_EQ(one.size(), 1u);
  EXPECT_EQ(one[0].gcell, gcellRect(2, 3));

  // A region straddling four of them reports all four, in row major order.
  const odb::Rect quad(gcellRect(1, 1).xMin() + 1,
                       gcellRect(1, 1).yMin() + 1,
                       gcellRect(2, 2).xMax() - 1,
                       gcellRect(2, 2).yMax() - 1);
  const std::vector<GCellDensity> four = dp.getGCellDensities(quad);
  ASSERT_EQ(four.size(), 4u);
  EXPECT_EQ(four[0].gcell, gcellRect(1, 1));
  EXPECT_EQ(four[1].gcell, gcellRect(2, 1));
  EXPECT_EQ(four[2].gcell, gcellRect(1, 2));
  EXPECT_EQ(four[3].gcell, gcellRect(2, 2));

  // The whole die is every GCell.
  EXPECT_EQ(dp.getGCellDensities(block_->getDieArea()).size(),
            static_cast<size_t>(kGCellsX * kGCellsY));

  // An empty region, and one off the die, have no GCells in them.
  EXPECT_TRUE(dp.getGCellDensities(odb::Rect()).empty());
  EXPECT_TRUE(
      dp.getGCellDensities(odb::Rect(-5000, -5000, -1000, -1000)).empty());
}

TEST_F(PlacementDensityTest, GCellDensitiesMatchThePerRegionQuery)
{
  makeGCellGrid();
  // A gradient along the bottom row of core GCells: 20, 15, 10, 5 and 0
  // cells.
  for (int gx = 0; gx < kCoreGCellsX; ++gx) {
    fillGCell(gx, 0, kCellsPerGCell - (gx * 5));
  }
  Opendp& dp = makeOpendp();

  const odb::Rect bottom_row(0, 0, kCoreWidth, kGCellSize);
  const std::vector<GCellDensity> densities = dp.getGCellDensities(bottom_row);

  ASSERT_EQ(densities.size(), static_cast<size_t>(kCoreGCellsX));
  for (int gx = 0; gx < kCoreGCellsX; ++gx) {
    EXPECT_EQ(densities[gx].gcell, gcellRect(gx, 0)) << "gcell " << gx;
    EXPECT_DOUBLE_EQ(densities[gx].density, 1.0 - (gx * 0.25))
        << "gcell " << gx;
    // The per-GCell report and the per-region query agree.
    EXPECT_DOUBLE_EQ(densities[gx].density,
                     dp.getPlacementDensity(densities[gx].gcell))
        << "gcell " << gx;
  }
}

TEST_F(PlacementDensityTest, GCellDensitiesSplitInstancesAcrossGCells)
{
  makeGCellGrid();
  // A macro covering the left half of GCell (1, 0) and the right half of
  // GCell (0, 0).
  odb::dbMaster* macro
      = makeMaster("macro", odb::dbMasterType::BLOCK, kGCellSize, kGCellSize);
  placeInst(macro, kGCellSize / 2, 0, odb::dbPlacementStatus::FIRM);
  Opendp& dp = makeOpendp();

  const std::vector<GCellDensity> densities
      = dp.getGCellDensities(odb::Rect(0, 0, 2 * kGCellSize, kGCellSize));
  ASSERT_EQ(densities.size(), 2u);
  EXPECT_DOUBLE_EQ(densities[0].density, 0.5);
  EXPECT_DOUBLE_EQ(densities[1].density, 0.5);
}

TEST_F(PlacementDensityTest, FindsTheEmptiestGCellNearAPin)
{
  makeGCellGrid();
  // Everything around the pin is packed except GCell (3, 1).
  for (int gy = 0; gy < 3; ++gy) {
    for (int gx = 0; gx < kGCellsX; ++gx) {
      fillGCell(gx, gy, (gx == 3 && gy == 1) ? 0 : kCellsPerGCell);
    }
  }
  Opendp& dp = makeOpendp();

  const odb::Point pin(kGCellSize + 500, kGCellSize + 500);
  const odb::Rect search(pin.x() - (2 * kGCellSize),
                         pin.y() - (2 * kGCellSize),
                         pin.x() + (2 * kGCellSize),
                         pin.y() + (2 * kGCellSize));

  const std::vector<GCellDensity> densities = dp.getGCellDensities(search);
  ASSERT_FALSE(densities.empty());
  const GCellDensity& emptiest = *std::ranges::min_element(
      densities, {}, [](const GCellDensity& d) { return d.density; });

  EXPECT_EQ(emptiest.gcell, gcellRect(3, 1));
  EXPECT_DOUBLE_EQ(emptiest.density, 0.0);
}

TEST_F(PlacementDensityTest, GCellRegionGrowsWithRadius)
{
  makeGCellGrid();
  Opendp& dp = makeOpendp();

  // A point in the middle of GCell (1, 2).
  const odb::Point pt(kGCellSize + 500, (2 * kGCellSize) + 500);

  EXPECT_EQ(dp.getGCellRegion(pt, 0), gcellRect(1, 2));
  EXPECT_EQ(dp.getGCellRegion(pt, 1),
            odb::Rect(gcellRect(0, 1).ll(), gcellRect(2, 3).ur()));
  // Radius 2 would reach past the die's lower left, so it stops there.
  EXPECT_EQ(dp.getGCellRegion(pt, 2),
            odb::Rect(gcellRect(0, 0).ll(), gcellRect(3, 4).ur()));
}

TEST_F(PlacementDensityTest, GCellRegionIsClippedToTheCore)
{
  makeGCellGrid();
  Opendp& dp = makeOpendp();

  // The last GCell of the core, at its upper right corner.  Radius 1 grows
  // the window into the margin, which has no rows, so it gets clipped back.
  const odb::Point corner((kCoreWidth - kGCellSize) + 500,
                          (kCoreHeight - kGCellSize) + 500);
  EXPECT_EQ(dp.getGCellRegion(corner, 0),
            gcellRect(kCoreGCellsX - 1, kCoreGCellsY - 1));
  EXPECT_EQ(dp.getGCellRegion(corner, 1),
            odb::Rect(gcellRect(kCoreGCellsX - 2, kCoreGCellsY - 2).ll(),
                      block_->getCoreArea().ur()));

  // A GCell out in the margin has no core area in it at all.  It abuts the
  // core here, so clipping it leaves a zero width region.
  const odb::Point abutting(kCoreWidth + 500, kCoreHeight + 500);
  const odb::Rect abutting_region = dp.getGCellRegion(abutting, 0);
  EXPECT_TRUE(abutting_region.dx() <= 0 || abutting_region.dy() <= 0);

  // A GCell a further one out clips to a rect whose low edge is above its
  // high edge.  Rect's constructor normalizes that into a plausible looking
  // region, so this has to come back empty rather than flipped.
  const odb::Point beyond(500, kCoreHeight + kGCellSize + 500);
  const odb::Rect beyond_region = dp.getGCellRegion(beyond, 0);
  EXPECT_TRUE(beyond_region.dx() <= 0 || beyond_region.dy() <= 0)
      << "clipped to " << beyond_region;
}

TEST_F(PlacementDensityTest, RadiusRegionIgnoresInstancesOutsideTheCore)
{
  makeGCellGrid();
  // A macro parked in the die margin, to the right of the core, where there
  // are no rows to hold it.
  odb::dbMaster* macro
      = makeMaster("macro", odb::dbMasterType::BLOCK, kGCellSize, kGCellSize);
  placeInst(macro, kCoreWidth, 0, odb::dbPlacementStatus::FIRM);
  Opendp& dp = makeOpendp();

  // Measured over the raw GCell window, the macro's area counts against the
  // sites of the last core GCell and reads as full.
  const odb::Rect unclipped(gcellRect(kCoreGCellsX - 1, 0).ll(),
                            gcellRect(kCoreGCellsX, 0).ur());
  EXPECT_DOUBLE_EQ(dp.getPlacementDensity(unclipped), 1.0);

  // Clipped to the core it is left out, and the region reads as the empty
  // core GCells it actually covers.
  const odb::Point pt((kCoreWidth - kGCellSize) + 500, 500);
  const odb::Rect region = dp.getGCellRegion(pt, 1);
  EXPECT_EQ(region.xMax(), kCoreWidth);
  EXPECT_DOUBLE_EQ(dp.getPlacementDensity(region), 0.0);
}

TEST_F(PlacementDensityTest, RadiusRegionAveragesOverItsGCells)
{
  makeGCellGrid();
  // Fill the 3x3 window around GCell (2, 2) unevenly: one full GCell, one
  // half full, the other seven empty.
  fillGCell(1, 1, kCellsPerGCell);
  fillGCell(2, 2, kCellsPerGCell / 2);
  Opendp& dp = makeOpendp();

  const odb::Point pt((2 * kGCellSize) + 500, (2 * kGCellSize) + 500);
  const odb::Rect region = dp.getGCellRegion(pt, 1);
  ASSERT_EQ(region, odb::Rect(gcellRect(1, 1).ll(), gcellRect(3, 3).ur()));

  // 1.5 GCells of cells over 9 GCells of sites.
  EXPECT_DOUBLE_EQ(dp.getPlacementDensity(region), 1.5 / 9.0);
}

TEST_F(PlacementDensityTest, GCellRegionRejectsBadArguments)
{
  makeGCellGrid();
  Opendp& dp = makeOpendp();

  const odb::Point pt(kGCellSize + 500, kGCellSize + 500);
  EXPECT_THROW(dp.getGCellRegion(pt, -1), std::runtime_error);
  EXPECT_THROW(dp.getGCellRegion(odb::Point(kDieWidth + 1000, 0), 0),
               std::runtime_error);
  EXPECT_THROW(dp.getGCellRegion(odb::Point(0, -1000), 0), std::runtime_error);
}

// initPlacementGrid() skips the netlist import that dominates
// initMacrosAndGrid() on a large design.  It has to leave the density
// queries reading exactly the same thing.
TEST_F(PlacementDensityTest, BothInitPathsGiveTheSameDensities)
{
  makeGCellGrid();
  fillGCell(0, 0, kCellsPerGCell);
  fillGCell(1, 0, kCellsPerGCell / 2);
  odb::dbBlockage::create(block_,
                          gcellRect(3, 0).xMin(),
                          gcellRect(3, 0).yMin(),
                          gcellRect(3, 0).xMax(),
                          gcellRect(3, 0).yMax());
  odb::dbMaster* macro
      = makeMaster("macro", odb::dbMasterType::BLOCK, kGCellSize, kGCellSize);
  placeInst(macro, 2 * kGCellSize, 0, odb::dbPlacementStatus::FIRM);

  const odb::Rect bottom_row(0, 0, kCoreWidth, kGCellSize);

  Opendp full(db_.get(), &logger_);
  full.initMacrosAndGrid();
  const std::vector<GCellDensity> from_full
      = full.getGCellDensities(bottom_row);

  Opendp light(db_.get(), &logger_);
  light.initPlacementGrid();
  const std::vector<GCellDensity> from_light
      = light.getGCellDensities(bottom_row);

  ASSERT_EQ(from_light.size(), from_full.size());
  for (size_t i = 0; i < from_full.size(); ++i) {
    EXPECT_EQ(from_light[i].gcell, from_full[i].gcell) << "gcell " << i;
    EXPECT_DOUBLE_EQ(from_light[i].density, from_full[i].density)
        << "gcell " << i;
  }

  // And the same for an arbitrary region and for a radius window.
  const odb::Rect region(500, 500, (3 * kGCellSize) + 500, kGCellSize + 500);
  EXPECT_DOUBLE_EQ(light.getPlacementDensity(region),
                   full.getPlacementDensity(region));
  const odb::Point pt(kGCellSize + 500, 500);
  EXPECT_EQ(light.getGCellRegion(pt, 1), full.getGCellRegion(pt, 1));
  EXPECT_DOUBLE_EQ(light.getPlacementDensity(light.getGCellRegion(pt, 1)),
                   full.getPlacementDensity(full.getGCellRegion(pt, 1)));
}

// Rebuilding the grid has to be repeatable: a later pass must not see
// leftovers from an earlier one.
TEST_F(PlacementDensityTest, PlacementGridCanBeRebuilt)
{
  fillGCell(2, 2, kCellsPerGCell / 2);
  Opendp& dp = makeOpendp();

  const double gcell_density = dp.getPlacementDensity(gcellRect(2, 2));
  const double core_density = dp.getPlacementDensity(block_->getCoreArea());
  ASSERT_DOUBLE_EQ(gcell_density, 0.5);
  ASSERT_GT(core_density, 0.0);

  for (int i = 0; i < 3; ++i) {
    dp.initPlacementGrid();
    EXPECT_DOUBLE_EQ(dp.getPlacementDensity(gcellRect(2, 2)), gcell_density)
        << "rebuild " << i;
    EXPECT_DOUBLE_EQ(dp.getPlacementDensity(block_->getCoreArea()),
                     core_density)
        << "rebuild " << i;
  }
}

TEST_F(PlacementDensityTest, ReportPlacementDensityRejectsAnEmptyRegion)
{
  Opendp& dp = makeOpendp();

  // Zero width, zero height, and both.
  EXPECT_THROW(dp.reportPlacementDensity(odb::Rect(0, 0, 0, kGCellSize)),
               std::runtime_error);
  EXPECT_THROW(dp.reportPlacementDensity(odb::Rect(0, 0, kGCellSize, 0)),
               std::runtime_error);
  EXPECT_THROW(dp.reportPlacementDensity(odb::Rect(0, 0, 0, 0)),
               std::runtime_error);

  // Rect normalizes, so opposite corners in either order name the same
  // rectangle and both report.
  EXPECT_NO_THROW(
      dp.reportPlacementDensity(odb::Rect(0, 0, kGCellSize, kGCellSize)));
  EXPECT_NO_THROW(
      dp.reportPlacementDensity(odb::Rect(kGCellSize, kGCellSize, 0, 0)));

  // A region off the rows has no site to report a density over, which is
  // reported rather than being an error.
  EXPECT_NO_THROW(dp.reportPlacementDensity(
      odb::Rect(kCoreWidth, kCoreHeight, kDieWidth, kDieHeight)));
}

TEST_F(PlacementDensityTest, QueriesWithoutTheirPrerequisitesError)
{
  Opendp uninitialized(db_.get(), &logger_);
  EXPECT_THROW(uninitialized.getPlacementDensity(block_->getCoreArea()),
               std::runtime_error);
  EXPECT_THROW(uninitialized.getGCellDensities(block_->getCoreArea()),
               std::runtime_error);

  EXPECT_THROW(uninitialized.reportPlacementDensity(block_->getCoreArea()),
               std::runtime_error);

  // The per-GCell queries need a GCell grid; the per-region ones do not.
  Opendp& dp = makeOpendp();
  EXPECT_NO_THROW(dp.getPlacementDensity(block_->getCoreArea()));
  EXPECT_NO_THROW(dp.reportPlacementDensity(block_->getCoreArea()));
  EXPECT_THROW(dp.getGCellDensities(block_->getCoreArea()), std::runtime_error);
}

}  // namespace

}  // namespace dpl
