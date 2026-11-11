// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <cstdint>
#include <random>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "rsz/PlacementDensityGrid.hh"
#include "tst/db_fixture.h"

namespace rsz {
namespace {

using odb::dbBlock;
using odb::dbInst;
using odb::dbLib;
using odb::dbMaster;
using odb::dbMasterType;
using odb::dbOrientType;
using odb::dbPlacementStatus;
using odb::dbSite;
using odb::Rect;

// A core of kRows rows of kRowSites sites, and a master of each kind the
// heat map tells apart.
class PlacementDensityGridTest : public tst::DbFixture
{
 protected:
  static constexpr int kDbuPerMicron = 1000;
  static constexpr int kSiteWidth = 200;
  static constexpr int kRowHeight = 2000;
  static constexpr int kRowSites = 100;
  static constexpr int kRows = 10;
  // The core, 20 microns a side, starts one site and one row in from the
  // die origin.
  static constexpr int kCoreLx = kSiteWidth;
  static constexpr int kCoreLy = kRowHeight;
  static constexpr int kCoreUx = kCoreLx + (kRowSites * kSiteWidth);
  static constexpr int kCoreUy = kCoreLy + (kRows * kRowHeight);
  // Bins of ten sites by one row.
  static constexpr int kBinWidth = 10 * kSiteWidth;
  static constexpr int kBinHeight = kRowHeight;

  void SetUp() override
  {
    db_->setDbuPerMicron(kDbuPerMicron);
    odb::dbTech* tech = odb::dbTech::create(db_.get(), "tech");
    odb::dbTechLayer::create(tech, "metal1", odb::dbTechLayerType::ROUTING);

    lib_ = dbLib::create(db_.get(), "lib", tech);
    site_ = dbSite::create(lib_, "site");
    site_->setWidth(kSiteWidth);
    site_->setHeight(kRowHeight);
    site_->setClass(odb::dbSiteClass::CORE);

    odb::dbChip* chip = odb::dbChip::create(db_.get(), tech);
    block_ = dbBlock::create(chip, "top");
    block_->setDieArea(Rect(0, 0, kCoreUx + kSiteWidth, kCoreUy + kRowHeight));
    for (int row = 0; row < kRows; ++row) {
      odb::dbRow::create(block_,
                         ("row" + std::to_string(row)).c_str(),
                         site_,
                         kCoreLx,
                         rowY(row),
                         dbOrientType::R0,
                         odb::dbRowDir::HORIZONTAL,
                         kRowSites,
                         kSiteWidth);
    }
    block_->setCoreArea(block_->computeCoreArea());

    cell_ = makeMaster("cell", dbMasterType::CORE, 2, 1);
    wide_cell_ = makeMaster("wide_cell", dbMasterType::CORE, 4, 1);
    tap_ = makeMaster("tap", dbMasterType::CORE_WELLTAP, 1, 1);
    endcap_ = makeMaster("endcap", dbMasterType::ENDCAP, 1, 1);
    filler_ = makeMaster("filler", dbMasterType::CORE_SPACER, 1, 1);
    pad_ = makeMaster("pad", dbMasterType::PAD, 4, 1);
    cover_ = makeMaster("cover", dbMasterType::COVER, 4, 1);
    macro_ = makeMaster("macro", dbMasterType::BLOCK, 20, 3);
  }

  // A master sites wide and rows tall.
  dbMaster* makeMaster(const char* name,
                       const dbMasterType::Value type,
                       const int sites,
                       const int rows)
  {
    dbMaster* master = dbMaster::create(lib_, name);
    master->setType(dbMasterType(type));
    master->setWidth(sites * kSiteWidth);
    master->setHeight(rows * kRowHeight);
    master->setSite(site_);
    master->setFrozen();
    return master;
  }

  // An instance of master with its lower left corner at (x, y).
  dbInst* place(dbMaster* master,
                const int x,
                const int y,
                const dbPlacementStatus status = dbPlacementStatus::PLACED)
  {
    dbInst* inst = dbInst::create(
        block_, master, ("u" + std::to_string(inst_count_++)).c_str());
    inst->setLocation(x, y);
    inst->setPlacementStatus(status);
    return inst;
  }

  static int siteX(const int site) { return kCoreLx + (site * kSiteWidth); }
  static int rowY(const int row) { return kCoreLy + (row * kRowHeight); }
  static Rect core() { return {kCoreLx, kCoreLy, kCoreUx, kCoreUy}; }
  static int64_t area(const dbMaster* master)
  {
    return int64_t{master->getWidth()} * master->getHeight();
  }

  // The instance area the heat map counts in rect, from scratch: placed
  // instances other than fillers, pads and covers.
  int64_t expectedArea(const Rect& rect) const
  {
    int64_t area = 0;
    for (dbInst* inst : block_->getInsts()) {
      dbMaster* master = inst->getMaster();
      const Rect box = inst->getBBox()->getBox();
      if (inst->getPlacementStatus().isPlaced() && !master->isFiller()
          && !master->isPad() && !master->isCover() && box.overlaps(rect)) {
        area += box.intersect(rect).area();
      }
    }
    return area;
  }

  // Every bin holds what a count from scratch gives.
  void expectBinsMatchTheDb(PlacementDensityGrid& grid) const
  {
    grid.getDensity(core());
    ASSERT_GT(grid.getBinCountX(), 0);
    for (int y = 0; y < grid.getBinCountY(); y++) {
      for (int x = 0; x < grid.getBinCountX(); x++) {
        EXPECT_EQ(grid.getBinOccupiedArea(x, y),
                  expectedArea(grid.getBinRect(x, y)))
            << "bin " << x << ", " << y;
      }
    }
  }

  dbLib* lib_ = nullptr;
  dbSite* site_ = nullptr;
  dbBlock* block_ = nullptr;
  dbMaster* cell_ = nullptr;
  dbMaster* wide_cell_ = nullptr;
  dbMaster* tap_ = nullptr;
  dbMaster* endcap_ = nullptr;
  dbMaster* filler_ = nullptr;
  dbMaster* pad_ = nullptr;
  dbMaster* cover_ = nullptr;
  dbMaster* macro_ = nullptr;
  int inst_count_ = 0;
};

TEST_F(PlacementDensityGridTest, CountsWhatTheHeatMapCounts)
{
  // Counted: a placed cell, a fixed macro, a tap and an endcap.
  place(cell_, siteX(0), rowY(0));
  place(macro_, siteX(50), rowY(5), dbPlacementStatus::FIRM);
  place(tap_, siteX(10), rowY(1), dbPlacementStatus::LOCKED);
  place(endcap_, siteX(99), rowY(2), dbPlacementStatus::FIRM);
  // Left out: a filler, a pad, a cover and an unplaced cell.
  place(filler_, siteX(20), rowY(0));
  place(pad_, siteX(30), rowY(0), dbPlacementStatus::FIRM);
  place(cover_, siteX(30), rowY(1), dbPlacementStatus::COVER);
  place(cell_, siteX(40), rowY(0), dbPlacementStatus::UNPLACED);

  PlacementDensityGrid grid(block_, kBinWidth, kBinHeight);
  const PlacementDensity density = grid.getDensity(core());
  const int64_t counted
      = area(cell_) + area(macro_) + area(tap_) + area(endcap_);
  EXPECT_EQ(density.region_area, core().area());
  EXPECT_DOUBLE_EQ(density.instance_area, static_cast<double>(counted));
  EXPECT_DOUBLE_EQ(
      density.getDensity(),
      static_cast<double>(counted) / static_cast<double>(core().area()));
  expectBinsMatchTheDb(grid);
}

TEST_F(PlacementDensityGridTest, IsBuiltByTheFirstQuery)
{
  place(cell_, siteX(0), rowY(0));
  PlacementDensityGrid grid(block_, kBinWidth, kBinHeight);
  EXPECT_EQ(grid.getBinCountX(), 0);

  // An edit before the first query is read by the build.
  place(cell_, siteX(50), rowY(5));
  EXPECT_DOUBLE_EQ(grid.getDensity(core()).instance_area,
                   static_cast<double>(2 * area(cell_)));
  EXPECT_EQ(grid.getBinCountX(), kRowSites / 10);
  EXPECT_EQ(grid.getBinCountY(), kRows);
}

TEST_F(PlacementDensityGridTest, DefaultBinsFollowTheHeatMap)
{
  // Before global placement has run, bins of 10 microns.
  {
    PlacementDensityGrid grid(block_);
    grid.getDensity(core());
    EXPECT_EQ(grid.getBinCountX(), 2);
    EXPECT_EQ(grid.getBinCountY(), 2);
    EXPECT_EQ(grid.getBinRect(0, 0),
              Rect(kCoreLx,
                   kCoreLy,
                   kCoreLx + (10 * kDbuPerMicron),
                   kCoreLy + (10 * kDbuPerMicron)));
  }

  // Then the bins global placement published, the last ones clipped to the
  // core.
  odb::dbIntProperty::create(block_, "gpl_bin_size_x", 3000);
  odb::dbIntProperty::create(block_, "gpl_bin_size_y", 4000);
  {
    PlacementDensityGrid grid(block_);
    grid.getDensity(core());
    EXPECT_EQ(grid.getBinCountX(), 7);
    EXPECT_EQ(grid.getBinCountY(), 5);
    EXPECT_EQ(grid.getBinRect(6, 4),
              Rect(kCoreLx + 18000, kCoreLy + 16000, kCoreUx, kCoreUy));
  }

  // Coarsened to at most 1024 bins a side.
  odb::dbIntProperty::find(block_, "gpl_bin_size_x")->setValue(10);
  odb::dbIntProperty::find(block_, "gpl_bin_size_y")->setValue(10);
  {
    PlacementDensityGrid grid(block_);
    grid.getDensity(core());
    EXPECT_EQ(grid.getBinCountX(), 1000);
    EXPECT_EQ(grid.getBinCountY(), 1000);
  }

  // A size asked for wins.
  {
    PlacementDensityGrid grid(block_, kBinWidth, kBinHeight);
    grid.getDensity(core());
    EXPECT_EQ(grid.getBinCountX(), kRowSites / 10);
    EXPECT_EQ(grid.getBinCountY(), kRows);
  }
}

TEST_F(PlacementDensityGridTest, RegionQueries)
{
  // A wide cell over the first four sites of bin (0, 0), a cell straddling
  // bins (0, 3) and (1, 3), and a macro over bins (3..4, 5..7).
  place(wide_cell_, siteX(0), rowY(0));
  place(cell_, siteX(9), rowY(3));
  place(macro_, siteX(30), rowY(5), dbPlacementStatus::FIRM);
  PlacementDensityGrid grid(block_, kBinWidth, kBinHeight);

  // Regions made of whole bins are exact.
  for (const Rect& region : {core(),
                             Rect(siteX(0), rowY(0), siteX(10), rowY(1)),
                             Rect(siteX(0), rowY(3), siteX(10), rowY(4)),
                             Rect(siteX(10), rowY(2), siteX(50), rowY(8))}) {
    const PlacementDensity density = grid.getDensity(region);
    EXPECT_EQ(density.region_area, region.area());
    EXPECT_DOUBLE_EQ(density.instance_area,
                     static_cast<double>(expectedArea(region)));
  }

  // A region over part of a bin takes that part of the bin's area, as if
  // spread evenly: the right half of bin (0, 0) gets half the wide cell,
  // though the cell sits in the left half.
  const Rect right_half(siteX(5), rowY(0), siteX(10), rowY(1));
  EXPECT_DOUBLE_EQ(grid.getDensity(right_half).instance_area,
                   0.5 * static_cast<double>(area(wide_cell_)));
  // And so across the edges of a region over several bins: a quarter of
  // bin (0, 3) and half of bin (1, 3) each hold half of the cell.
  const Rect across(siteX(7) + (kSiteWidth / 2), rowY(3), siteX(15), rowY(4));
  EXPECT_DOUBLE_EQ(grid.getDensity(across).instance_area,
                   (0.25 + 0.5) * 0.5 * static_cast<double>(area(cell_)));

  // A region reaching out of the core is clipped to it.
  const Rect hanging(0, 0, siteX(10), rowY(1));
  const PlacementDensity clipped = grid.getDensity(hanging);
  EXPECT_EQ(clipped.region_area,
            Rect(kCoreLx, kCoreLy, siteX(10), rowY(1)).area());
  EXPECT_DOUBLE_EQ(clipped.instance_area,
                   static_cast<double>(area(wide_cell_)));

  // Nothing outside the core, or in an empty region.
  for (const Rect& region : {Rect(0, 0, kCoreLx, kCoreLy),
                             Rect(siteX(5), rowY(5), siteX(5), rowY(6))}) {
    const PlacementDensity density = grid.getDensity(region);
    EXPECT_EQ(density.region_area, 0);
    EXPECT_EQ(density.instance_area, 0);
    EXPECT_EQ(density.getDensity(), 0);
  }
}

TEST_F(PlacementDensityGridTest, BinsMatchTheDbUnderRandomEdits)
{
  std::mt19937 rng(1);
  const auto pick = [&rng](const int lo, const int hi) {
    return std::uniform_int_distribution<int>(lo, hi)(rng);
  };
  // Anywhere over the core, at times hanging out of it.
  const auto random_x = [&] { return pick(0, kCoreUx); };
  const auto random_y = [&] { return pick(0, kCoreUy); };
  const std::vector<dbMaster*> masters
      = {cell_, wide_cell_, tap_, endcap_, filler_, pad_, cover_, macro_};
  const std::vector<dbPlacementStatus> statuses = {dbPlacementStatus::PLACED,
                                                   dbPlacementStatus::UNPLACED,
                                                   dbPlacementStatus::FIRM,
                                                   dbPlacementStatus::LOCKED,
                                                   dbPlacementStatus::COVER};
  const std::vector<dbOrientType> orients = {dbOrientType::R0,
                                             dbOrientType::R90,
                                             dbOrientType::R180,
                                             dbOrientType::MX,
                                             dbOrientType::MYR90};

  std::vector<dbInst*> insts;
  insts.reserve(60);
  for (int i = 0; i < 60; i++) {
    insts.push_back(place(masters[pick(0, 3)], random_x(), random_y()));
  }
  // Bins that line up with neither the sites nor the rows, the last ones
  // clipped to the core.
  PlacementDensityGrid grid(block_, 1700, 1300);
  expectBinsMatchTheDb(grid);

  for (int step = 1; step <= 1000; step++) {
    dbInst* inst = insts[pick(0, static_cast<int>(insts.size()) - 1)];
    const bool fixed = inst->getPlacementStatus().isFixed();
    switch (pick(0, 5)) {
      case 0:
        if (!fixed) {
          inst->setLocation(random_x(), random_y());
        }
        break;
      case 1:
        if (!fixed) {
          inst->setOrient(orients[pick(0, 4)]);
        }
        break;
      case 2:
        if (!fixed && inst->getMaster() == cell_) {
          inst->swapMaster(wide_cell_);
        } else if (!fixed && inst->getMaster() == wide_cell_) {
          inst->swapMaster(cell_);
        }
        break;
      case 3:
        inst->setPlacementStatus(statuses[pick(0, 4)]);
        break;
      case 4:
        insts.push_back(place(
            masters[pick(0, 7)], random_x(), random_y(), statuses[pick(0, 4)]));
        break;
      default:
        if (insts.size() > 1) {
          std::erase(insts, inst);
          dbInst::destroy(inst);
        }
        break;
    }
    if (step % 25 == 0) {
      expectBinsMatchTheDb(grid);
      // A grid built afresh answers alike.
      PlacementDensityGrid fresh(block_, 1700, 1300);
      for (int query = 0; query < 10; query++) {
        const Rect region(random_x(), random_y(), random_x(), random_y());
        const PlacementDensity got = grid.getDensity(region);
        const PlacementDensity want = fresh.getDensity(region);
        EXPECT_EQ(got.region_area, want.region_area);
        EXPECT_DOUBLE_EQ(got.instance_area, want.instance_area);
      }
    }
  }
}

TEST_F(PlacementDensityGridTest, JournalUndoLeavesNothingBehind)
{
  dbInst* moved = place(cell_, siteX(0), rowY(0));
  dbInst* resized = place(cell_, siteX(20), rowY(2));
  PlacementDensityGrid grid(block_, kBinWidth, kBinHeight);
  expectBinsMatchTheDb(grid);

  // A repair move rolled back: a buffer added, placed and turned, a cell
  // moved and another resized.
  odb::dbDatabase::beginEco(block_);
  dbInst* buffer = dbInst::create(block_, cell_, "buffer");
  buffer->setLocation(siteX(60), rowY(6));
  buffer->setPlacementStatus(dbPlacementStatus::PLACED);
  buffer->setOrient(dbOrientType::R90);
  moved->setLocation(siteX(35), rowY(4));
  resized->swapMaster(wide_cell_);
  expectBinsMatchTheDb(grid);
  odb::dbDatabase::undoEco(block_);

  // Undo restores the buffer's orientation and status without callbacks,
  // yet nothing of it is left.
  EXPECT_EQ(block_->findInst("buffer"), nullptr);
  expectBinsMatchTheDb(grid);
}

TEST_F(PlacementDensityGridTest, ANewCoreIsCutIntoBinsAfresh)
{
  place(cell_, siteX(0), rowY(0));
  place(cell_, siteX(80), rowY(0));
  PlacementDensityGrid grid(block_, kBinWidth, kBinHeight);
  EXPECT_DOUBLE_EQ(grid.getDensity(core()).instance_area,
                   static_cast<double>(2 * area(cell_)));

  // The left half of the core holds one of the cells.
  const Rect left_half(kCoreLx, kCoreLy, siteX(50), kCoreUy);
  block_->setCoreArea(left_half);
  const PlacementDensity density = grid.getDensity(core());
  EXPECT_EQ(grid.getBinCountX(), 5);
  EXPECT_EQ(density.region_area, left_half.area());
  EXPECT_DOUBLE_EQ(density.instance_area, static_cast<double>(area(cell_)));
  expectBinsMatchTheDb(grid);
}

}  // namespace
}  // namespace rsz
