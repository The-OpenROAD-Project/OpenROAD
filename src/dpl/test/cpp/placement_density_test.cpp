// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

// Tests for Opendp's placement density queries, the ones rsz uses to look
// for room to drop a buffer into.
//
// The fixture builds a floorplan whose numbers stay easy to check by hand:
// a 14um x 24um die with 20 rows filling the lower left 10um x 20um of it.
// The tests work in 2um squares of it, each 2 rows of 20 sites, holding
// exactly 20 of the 2-site core cells; the two columns and rows of squares
// past the core hold no placement site at all.

#include <algorithm>
#include <cstdint>
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

  // The square the tests work in.  The rows start at the die's lower left
  // but stop two squares short of its upper right, so the squares past the
  // core hold no core area.
  static constexpr int kSquare = 2000;
  static constexpr int kMargin = 2 * kSquare;
  static constexpr int kDieWidth = kCoreWidth + kMargin;    // 14000
  static constexpr int kDieHeight = kCoreHeight + kMargin;  // 24000

  // Core cells are two sites wide and one row tall.
  static constexpr int kCellWidth = 2 * kSiteWidth;
  static constexpr int kCellsPerSquareRow = kSquare / kCellWidth;  // 10
  static constexpr int kRowsPerSquare = kSquare / kRowHeight;      // 2
  static constexpr int kCellsPerSquare = kCellsPerSquareRow * kRowsPerSquare;

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

  // The extent of the square at (sx, sy).
  static odb::Rect square(int sx, int sy)
  {
    return {sx * kSquare, sy * kSquare, (sx + 1) * kSquare, (sy + 1) * kSquare};
  }

  // Packs num_cells core cells into the square at (sx, sy), filling it row
  // by row from its lower left corner.
  void fillSquare(int sx, int sy, int num_cells)
  {
    EXPECT_LE(num_cells, kCellsPerSquare);
    for (int i = 0; i < num_cells; ++i) {
      placeInst(core_master_,
                (sx * kSquare) + ((i % kCellsPerSquareRow) * kCellWidth),
                (sy * kSquare) + ((i / kCellsPerSquareRow) * kRowHeight));
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

  EXPECT_DOUBLE_EQ(dp.getPlacementDensity(square(2, 5)), 0.0);
  EXPECT_DOUBLE_EQ(dp.getPlacementDensity(block_->getCoreArea()), 0.0);
}

TEST_F(PlacementDensityTest, DensityIsOccupiedOverPlaceableArea)
{
  fillSquare(0, 0, kCellsPerSquare);      // full
  fillSquare(1, 0, kCellsPerSquare / 2);  // half
  Opendp& dp = makeOpendp();

  EXPECT_DOUBLE_EQ(dp.getPlacementDensity(square(0, 0)), 1.0);
  EXPECT_DOUBLE_EQ(dp.getPlacementDensity(square(1, 0)), 0.5);
  EXPECT_DOUBLE_EQ(dp.getPlacementDensity(square(3, 3)), 0.0);

  // A region spanning both: 30 cells over 2 squares worth of sites.
  const odb::Rect both(0, 0, 2 * kSquare, kSquare);
  EXPECT_DOUBLE_EQ(dp.getPlacementDensity(both), 0.75);
}

TEST_F(PlacementDensityTest, RegionNeedNotFollowSquareOrRowBoundaries)
{
  // One cell at the origin: 200 x 1000 DBU.
  fillSquare(0, 0, 1);
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
  // A hard blockage over the square at (0, 0) leaves it with no legal site.
  odb::dbBlockage::create(block_, 0, 0, kSquare, kSquare);
  Opendp& dp = makeOpendp();

  // Nothing fits there, so it reads as full rather than as the emptiest
  // spot in the design.
  EXPECT_DOUBLE_EQ(dp.getPlacementDensity(square(0, 0)), 1.0);
  EXPECT_DOUBLE_EQ(dp.getPlacementDensity(square(1, 0)), 0.0);

  // So is a region entirely off the die.
  EXPECT_DOUBLE_EQ(
      dp.getPlacementDensity(odb::Rect(-5000, -5000, -1000, -1000)), 1.0);
}

TEST_F(PlacementDensityTest, FixedMacroOccupiesItsArea)
{
  odb::dbMaster* macro
      = makeMaster("macro", odb::dbMasterType::BLOCK, kSquare, kSquare);
  placeInst(macro, 3 * kSquare, 0, odb::dbPlacementStatus::FIRM);
  Opendp& dp = makeOpendp();

  EXPECT_DOUBLE_EQ(dp.getPlacementDensity(square(3, 0)), 1.0);
  EXPECT_DOUBLE_EQ(dp.getPlacementDensity(square(4, 0)), 0.0);
}

// The instance set matches the GUI's placement density heat map on its
// default settings: taps and endcaps count, fillers and IO do not.
TEST_F(PlacementDensityTest, FillersDoNotCount)
{
  odb::dbMaster* filler = makeMaster(
      "filler", odb::dbMasterType::CORE_SPACER, kCellWidth, kRowHeight);
  Opendp& dp = makeOpendp();

  // Half the square in real cells, the other half in fillers.
  fillSquare(0, 0, kCellsPerSquare / 2);
  for (int i = 0; i < kCellsPerSquareRow; ++i) {
    placeInst(filler, i * kCellWidth, kRowHeight);
  }

  // A filler can be removed to make room, so it leaves the square half
  // empty rather than full.
  EXPECT_DOUBLE_EQ(dp.getPlacementDensity(square(0, 0)), 0.5);
}

TEST_F(PlacementDensityTest, TapsAndEndcapsCount)
{
  odb::dbMaster* tap = makeMaster(
      "tap", odb::dbMasterType::CORE_WELLTAP, kCellWidth, kRowHeight);
  odb::dbMaster* endcap
      = makeMaster("endcap", odb::dbMasterType::ENDCAP, kCellWidth, kRowHeight);
  Opendp& dp = makeOpendp();

  placeInst(tap, 0, 0);
  placeInst(endcap, kCellWidth, 0);

  // Two cells of the ten that fit in one row of the square.
  const odb::Rect row(0, 0, kSquare, kRowHeight);
  EXPECT_DOUBLE_EQ(dp.getPlacementDensity(row), 0.2);
}

TEST_F(PlacementDensityTest, PadsAndCoversDoNotCount)
{
  odb::dbMaster* pad
      = makeMaster("pad", odb::dbMasterType::PAD, kCellWidth, kRowHeight);
  odb::dbMaster* cover
      = makeMaster("cover", odb::dbMasterType::COVER, kCellWidth, kRowHeight);
  Opendp& dp = makeOpendp();

  // Sitting on core sites, which is not where they normally are, to prove
  // the filter is on the master type rather than on the location.
  placeInst(pad, 0, 0, odb::dbPlacementStatus::FIRM);
  placeInst(cover, kCellWidth, 0, odb::dbPlacementStatus::FIRM);

  EXPECT_DOUBLE_EQ(dp.getPlacementDensity(odb::Rect(0, 0, kSquare, kRowHeight)),
                   0.0);
}

TEST_F(PlacementDensityTest, DensityIsRecomputedAfterInstancesMove)
{
  fillSquare(0, 0, kCellsPerSquare);
  Opendp& dp = makeOpendp();

  ASSERT_DOUBLE_EQ(dp.getPlacementDensity(square(0, 0)), 1.0);
  ASSERT_DOUBLE_EQ(dp.getPlacementDensity(square(4, 9)), 0.0);

  // Move everything to the far corner, the way an optimization step would.
  for (odb::dbInst* inst : block_->getInsts()) {
    const odb::Point origin = inst->getLocation();
    inst->setLocation(origin.x() + (4 * kSquare), origin.y() + (9 * kSquare));
  }

  // No re-init, no explicit invalidation: the next query sees the move.
  EXPECT_DOUBLE_EQ(dp.getPlacementDensity(square(0, 0)), 0.0);
  EXPECT_DOUBLE_EQ(dp.getPlacementDensity(square(4, 9)), 1.0);
}

////////////////////////////////////////////////////////////////
// The instance index bounds each query to the region it asks about, so
// every way the netlist can change under it has to keep the answers right.
// These are the cases a full scan of the block got for free.

TEST_F(PlacementDensityTest, InstancesCreatedAfterTheFirstQueryAreCounted)
{
  Opendp& dp = makeOpendp();

  // Query first, so the index is already built when the cells arrive -- the
  // order rsz works in, inserting buffers between queries.
  ASSERT_DOUBLE_EQ(dp.getPlacementDensity(square(1, 1)), 0.0);

  fillSquare(1, 1, kCellsPerSquare / 2);
  EXPECT_DOUBLE_EQ(dp.getPlacementDensity(square(1, 1)), 0.5);

  fillSquare(1, 1, kCellsPerSquare / 2);
  EXPECT_DOUBLE_EQ(dp.getPlacementDensity(square(1, 1)), 1.0);
}

TEST_F(PlacementDensityTest, InstancesDestroyedAfterTheFirstQueryAreDropped)
{
  fillSquare(1, 1, kCellsPerSquare);
  Opendp& dp = makeOpendp();
  ASSERT_DOUBLE_EQ(dp.getPlacementDensity(square(1, 1)), 1.0);

  std::vector<odb::dbInst*> insts;
  for (odb::dbInst* inst : block_->getInsts()) {
    insts.push_back(inst);
  }
  ASSERT_EQ(insts.size(), static_cast<size_t>(kCellsPerSquare));
  for (size_t i = 0; i < insts.size() / 2; ++i) {
    odb::dbInst::destroy(insts[i]);
  }

  EXPECT_DOUBLE_EQ(dp.getPlacementDensity(square(1, 1)), 0.5);
}

TEST_F(PlacementDensityTest, InstancesMovedFarAwayAreFoundAtTheNewPlace)
{
  fillSquare(0, 0, kCellsPerSquare);
  Opendp& dp = makeOpendp();
  ASSERT_DOUBLE_EQ(dp.getPlacementDensity(square(0, 0)), 1.0);

  // One cell at a time, to the far corner of the core: far enough that the
  // index has to have re-bucketed it, not just tolerated a small step.
  int moved = 0;
  for (odb::dbInst* inst : block_->getInsts()) {
    if (moved++ >= kCellsPerSquare / 2) {
      break;
    }
    const odb::Point origin = inst->getLocation();
    inst->setLocation(origin.x() + (4 * kSquare), origin.y() + (9 * kSquare));
  }

  EXPECT_DOUBLE_EQ(dp.getPlacementDensity(square(0, 0)), 0.5);
  EXPECT_DOUBLE_EQ(dp.getPlacementDensity(square(4, 9)), 0.5);
}

TEST_F(PlacementDensityTest, SwappingToABiggerMasterIsCounted)
{
  odb::dbMaster* wide = makeMaster(
      "wide_cell", odb::dbMasterType::CORE, 4 * kSiteWidth, kRowHeight);
  odb::dbInst* inst = placeInst(core_master_, 0, 0);
  Opendp& dp = makeOpendp();

  const odb::Rect one_row(0, 0, kSquare, kRowHeight);
  ASSERT_DOUBLE_EQ(dp.getPlacementDensity(one_row), 0.1);

  inst->swapMaster(wide);
  EXPECT_DOUBLE_EQ(dp.getPlacementDensity(one_row), 0.2);
}

TEST_F(PlacementDensityTest, MacrosTooBigToBucketAreStillCounted)
{
  // A macro spanning most of the core cannot sit in one bucket, so the index
  // has to keep it where every query sees it.
  odb::dbMaster* macro = makeMaster(
      "big_macro", odb::dbMasterType::BLOCK, kCoreWidth, 4 * kRowHeight);
  placeInst(macro, 0, 0, odb::dbPlacementStatus::FIRM);
  Opendp& dp = makeOpendp();

  // Covers rows 0-3 everywhere, so every square along the bottom is full and
  // the ones above it are empty.
  EXPECT_DOUBLE_EQ(dp.getPlacementDensity(square(0, 0)), 1.0);
  EXPECT_DOUBLE_EQ(dp.getPlacementDensity(square(4, 0)), 1.0);
  EXPECT_DOUBLE_EQ(dp.getPlacementDensity(square(0, 1)), 1.0);
  EXPECT_DOUBLE_EQ(dp.getPlacementDensity(square(0, 2)), 0.0);
}

TEST_F(PlacementDensityTest, IndexedAndFullScanDensitiesAgree)
{
  // Scatter cells unevenly over the core so the buckets fill unevenly, and
  // drop one in the margin outside it to make sure it is not dragged in.
  for (int sx = 0; sx < 5; ++sx) {
    for (int sy = 0; sy < 10; ++sy) {
      fillSquare(sx, sy, ((sx * 7) + (sy * 3)) % (kCellsPerSquare + 1));
    }
  }
  placeInst(core_master_, kCoreWidth + 100, kCoreHeight + 100);
  Opendp& dp = makeOpendp();

  // The slow way: every instance in the block, clipped to the region.
  const auto fullScanOccupied = [&](const odb::Rect& region) {
    int64_t occupied = 0;
    for (odb::dbInst* inst : block_->getInsts()) {
      if (!inst->getPlacementStatus().isPlaced()
          || !inst->getMaster()->isCoreAutoPlaceable()) {
        continue;
      }
      const odb::Rect bbox = inst->getBBox()->getBox();
      if (bbox.overlaps(region)) {
        occupied += bbox.intersect(region).area();
      }
    }
    return occupied;
  };

  // Every site inside the core is legal here -- no blockages, rows spanning
  // the whole width -- so a region inside it has exactly its own area to
  // place in, and the density is the occupied area over that.
  const auto expectedDensity = [&](const odb::Rect& region) {
    return std::min(1.0,
                    static_cast<double>(fullScanOccupied(region))
                        / static_cast<double>(region.area()));
  };

  for (int sx = 0; sx < 5; ++sx) {
    for (int sy = 0; sy < 10; ++sy) {
      const odb::Rect region = square(sx, sy);
      EXPECT_DOUBLE_EQ(dp.getPlacementDensity(region), expectedDensity(region))
          << "square " << sx << "," << sy;
    }
  }

  // And over regions following neither square nor row nor cell boundaries.
  for (const odb::Rect& region : {odb::Rect(1234, 5678, 9012, 15678),
                                  odb::Rect(0, 0, 150, 600),
                                  odb::Rect(3999, 3999, 4001, 4001),
                                  odb::Rect(0, 0, kCoreWidth, kCoreHeight)}) {
    EXPECT_DOUBLE_EQ(dp.getPlacementDensity(region), expectedDensity(region))
        << region;
  }
}

// Rebuilding the grid has to be repeatable: a later pass must not see
// leftovers from an earlier one.
TEST_F(PlacementDensityTest, PlacementGridCanBeRebuilt)
{
  fillSquare(2, 2, kCellsPerSquare / 2);
  Opendp& dp = makeOpendp();

  const double square_density = dp.getPlacementDensity(square(2, 2));
  const double core_density = dp.getPlacementDensity(block_->getCoreArea());
  ASSERT_DOUBLE_EQ(square_density, 0.5);
  ASSERT_GT(core_density, 0.0);

  for (int i = 0; i < 3; ++i) {
    dp.initPlacementGrid();
    EXPECT_DOUBLE_EQ(dp.getPlacementDensity(square(2, 2)), square_density)
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
  EXPECT_THROW(dp.reportPlacementDensity(odb::Rect(0, 0, 0, kSquare)),
               std::runtime_error);
  EXPECT_THROW(dp.reportPlacementDensity(odb::Rect(0, 0, kSquare, 0)),
               std::runtime_error);
  EXPECT_THROW(dp.reportPlacementDensity(odb::Rect(0, 0, 0, 0)),
               std::runtime_error);

  // Rect normalizes, so opposite corners in either order name the same
  // rectangle and both report.
  EXPECT_NO_THROW(dp.reportPlacementDensity(odb::Rect(0, 0, kSquare, kSquare)));
  EXPECT_NO_THROW(dp.reportPlacementDensity(odb::Rect(kSquare, kSquare, 0, 0)));

  // A region off the rows has no site to report a density over, which is
  // reported rather than being an error.
  EXPECT_NO_THROW(dp.reportPlacementDensity(
      odb::Rect(kCoreWidth, kCoreHeight, kDieWidth, kDieHeight)));
}

TEST_F(PlacementDensityTest, HasPlacementSiteTellsFullFromSiteless)
{
  fillSquare(0, 0, kCellsPerSquare);
  odb::dbBlockage::create(block_, square(1, 0).xMin(), 0, 2 * kSquare, kSquare);
  Opendp& dp = makeOpendp();

  // Three regions that all read 1.0, for two different reasons.
  const odb::Rect off_the_rows(kCoreWidth, kCoreHeight, kDieWidth, kDieHeight);
  ASSERT_DOUBLE_EQ(dp.getPlacementDensity(square(0, 0)), 1.0);
  ASSERT_DOUBLE_EQ(dp.getPlacementDensity(square(1, 0)), 1.0);
  ASSERT_DOUBLE_EQ(dp.getPlacementDensity(off_the_rows), 1.0);

  EXPECT_TRUE(dp.hasPlacementSite(square(0, 0)));   // full
  EXPECT_FALSE(dp.hasPlacementSite(square(1, 0)));  // blocked
  EXPECT_FALSE(dp.hasPlacementSite(off_the_rows));  // outside the rows
  EXPECT_TRUE(dp.hasPlacementSite(square(2, 0)));   // empty
}

TEST_F(PlacementDensityTest, BothInitPathsGiveTheSameDensities)
{
  fillSquare(0, 0, kCellsPerSquare);
  fillSquare(1, 0, kCellsPerSquare / 2);
  odb::dbBlockage::create(block_,
                          square(3, 0).xMin(),
                          square(3, 0).yMin(),
                          square(3, 0).xMax(),
                          square(3, 0).yMax());
  odb::dbMaster* macro
      = makeMaster("macro", odb::dbMasterType::BLOCK, kSquare, kSquare);
  placeInst(macro, 2 * kSquare, 0, odb::dbPlacementStatus::FIRM);

  Opendp full(db_.get(), &logger_);
  full.initMacrosAndGrid();

  Opendp light(db_.get(), &logger_);
  light.initPlacementGrid();

  // Over whole squares, over a region following nothing in particular, and
  // over the core.
  for (int sx = 0; sx < 5; ++sx) {
    EXPECT_DOUBLE_EQ(light.getPlacementDensity(square(sx, 0)),
                     full.getPlacementDensity(square(sx, 0)))
        << "square " << sx;
  }
  const odb::Rect region(500, 500, (3 * kSquare) + 500, kSquare + 500);
  EXPECT_DOUBLE_EQ(light.getPlacementDensity(region),
                   full.getPlacementDensity(region));
  EXPECT_DOUBLE_EQ(light.getPlacementDensity(block_->getCoreArea()),
                   full.getPlacementDensity(block_->getCoreArea()));
}

TEST_F(PlacementDensityTest, QueriesWithoutTheirPrerequisitesError)
{
  Opendp uninitialized(db_.get(), &logger_);
  EXPECT_THROW(uninitialized.getPlacementDensity(block_->getCoreArea()),
               std::runtime_error);
  EXPECT_THROW(uninitialized.hasPlacementSite(block_->getCoreArea()),
               std::runtime_error);
  EXPECT_THROW(uninitialized.reportPlacementDensity(block_->getCoreArea()),
               std::runtime_error);

  Opendp& dp = makeOpendp();
  EXPECT_NO_THROW(dp.getPlacementDensity(block_->getCoreArea()));
  EXPECT_NO_THROW(dp.hasPlacementSite(block_->getCoreArea()));
  EXPECT_NO_THROW(dp.reportPlacementDensity(block_->getCoreArea()));
}

}  // namespace

}  // namespace dpl
