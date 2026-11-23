// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <random>
#include <string>
#include <utility>
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
  // Bins of ten sites by one row, which the tests line up on the core.
  static constexpr int kBinWidth = 10 * kSiteWidth;
  static constexpr int kBinHeight = kRowHeight;
  // The track pitches on metal2, metal3 and metal4.  The router's gcells are
  // 15 of the middle one.
  static constexpr int kTrackPitches[] = {200, 300, 400};
  static constexpr int kGCellSize = 15 * 300;

  void SetUp() override
  {
    db_->setDbuPerMicron(kDbuPerMicron);
    odb::dbTech* tech = odb::dbTech::create(db_.get(), "tech");
    odb::dbTechLayer::create(tech, "metal1", odb::dbTechLayerType::ROUTING);
    for (const char* name : {"metal2", "metal3", "metal4"}) {
      odb::dbTechLayer::create(tech, name, odb::dbTechLayerType::ROUTING)
          ->setDirection(odb::dbTechLayerDir::VERTICAL);
    }

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

  // What the router's gcells need: tracks on metal2 to metal4, and the
  // routing layers set.
  void addRoutingTracks()
  {
    for (int level = 2; level <= 4; level++) {
      odb::dbTechLayer* layer = db_->getTech()->findRoutingLayer(level);
      const int pitch = kTrackPitches[level - 2];
      odb::dbTrackGrid::create(block_, layer)
          ->addGridPatternX(0, kCoreUx / pitch, pitch);
    }
    block_->setMaxRoutingLayer(4);
  }

  // A row over sites [first, last) of row.
  void addRow(const int row, const int first, const int last)
  {
    odb::dbRow::create(
        block_,
        ("row" + std::to_string(row) + "_" + std::to_string(first)).c_str(),
        site_,
        siteX(first),
        rowY(row),
        dbOrientType::R0,
        odb::dbRowDir::HORIZONTAL,
        last - first,
        kSiteWidth);
  }

  // Takes sites [lo, hi) out of row, as cut_rows does around a macro.
  void cutRow(const int row, const int lo, const int hi)
  {
    const std::string name = "row" + std::to_string(row);
    for (odb::dbRow* db_row : block_->getRows()) {
      if (db_row->getName() == name) {
        odb::dbRow::destroy(db_row);
        break;
      }
    }
    for (const auto& [first, last] :
         {std::pair{0, lo}, std::pair{hi, kRowSites}}) {
      if (last > first) {
        addRow(row, first, last);
      }
    }
  }

  static int siteX(const int site) { return kCoreLx + (site * kSiteWidth); }
  static int rowY(const int row) { return kCoreLy + (row * kRowHeight); }
  static Rect core() { return {kCoreLx, kCoreLy, kCoreUx, kCoreUy}; }
  static int64_t area(const dbMaster* master)
  {
    return int64_t{master->getWidth()} * master->getHeight();
  }

  // Whether the heat map counts inst: placed, and not a filler, a pad or a
  // cover.
  static bool counted(dbInst* inst)
  {
    dbMaster* master = inst->getMaster();
    return inst->getPlacementStatus().isPlaced() && !master->isFiller()
           && !master->isPad() && !master->isCover();
  }

  // The instance area the heat map counts in rect, from scratch.
  int64_t expectedArea(const Rect& rect) const
  {
    int64_t area = 0;
    for (dbInst* inst : block_->getInsts()) {
      const Rect box = inst->getBBox()->getBox();
      if (counted(inst) && box.overlaps(rect)) {
        area += box.intersect(rect).area();
      }
    }
    return area;
  }

  // The blocked area in rect, which must lie in the core, from scratch: rect
  // is cut at every edge of the shapes that decide it, and each piece is in
  // or out as a whole.
  int64_t expectedBlockedArea(const Rect& rect) const
  {
    std::vector<Rect> rows;
    for (odb::dbRow* row : block_->getRows()) {
      rows.push_back(row->getBBox());
    }
    std::vector<Rect> blocking;
    for (odb::dbBlockage* blockage : block_->getBlockages()) {
      if (!blockage->isSoft()) {
        blocking.push_back(blockage->getBBox()->getBox());
      }
    }
    // Instance area, not blocked: under macros and instances with a hard
    // halo.
    std::vector<Rect> own;
    for (dbInst* inst : block_->getInsts()) {
      if (!inst->getPlacementStatus().isPlaced()) {
        continue;
      }
      const Rect box = inst->getBBox()->getBox();
      const odb::dbBox* halo = inst->getHalo();
      const bool hard_halo = halo != nullptr && !halo->isSoft();
      if (hard_halo) {
        const Rect widths = inst->getTransformedHalo();
        blocking.emplace_back(box.xMin() - widths.xMin(),
                              box.yMin() - widths.yMin(),
                              box.xMax() + widths.xMax(),
                              box.yMax() + widths.yMax());
      }
      if ((hard_halo || inst->getMaster()->isBlock()) && counted(inst)) {
        own.push_back(box);
      }
    }

    std::vector<int> xs = {rect.xMin(), rect.xMax()};
    std::vector<int> ys = {rect.yMin(), rect.yMax()};
    for (const std::vector<Rect>* shapes : {&rows, &blocking, &own}) {
      for (const Rect& shape : *shapes) {
        for (const int x : {shape.xMin(), shape.xMax()}) {
          if (x > rect.xMin() && x < rect.xMax()) {
            xs.push_back(x);
          }
        }
        for (const int y : {shape.yMin(), shape.yMax()}) {
          if (y > rect.yMin() && y < rect.yMax()) {
            ys.push_back(y);
          }
        }
      }
    }
    std::ranges::sort(xs);
    xs.erase(std::ranges::unique(xs).begin(), xs.end());
    std::ranges::sort(ys);
    ys.erase(std::ranges::unique(ys).begin(), ys.end());

    // Whether the piece with its lower left corner at (x, y) is in shapes.
    const auto covers
        = [](const std::vector<Rect>& shapes, const int x, const int y) {
            return std::ranges::any_of(shapes, [&](const Rect& shape) {
              return shape.xMin() <= x && x < shape.xMax() && shape.yMin() <= y
                     && y < shape.yMax();
            });
          };
    int64_t area = 0;
    for (size_t i = 0; i + 1 < xs.size(); i++) {
      for (size_t j = 0; j + 1 < ys.size(); j++) {
        const int x = xs[i];
        const int y = ys[j];
        if ((!covers(rows, x, y) || covers(blocking, x, y))
            && !covers(own, x, y)) {
          area += int64_t{xs[i + 1] - x} * (ys[j + 1] - y);
        }
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
        const Rect bin = grid.getBinRect(x, y);
        EXPECT_EQ(grid.getBinOccupiedArea(x, y), expectedArea(bin))
            << "bin " << x << ", " << y;
        EXPECT_EQ(grid.getBinBlockedArea(x, y), expectedBlockedArea(bin))
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

  PlacementDensityGrid grid(block_, core().ll(), kBinWidth, kBinHeight);
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
  PlacementDensityGrid grid(block_, core().ll(), kBinWidth, kBinHeight);
  EXPECT_EQ(grid.getBinCountX(), 0);

  // An edit before the first query is read by the build.
  place(cell_, siteX(50), rowY(5));
  EXPECT_DOUBLE_EQ(grid.getDensity(core()).instance_area,
                   static_cast<double>(2 * area(cell_)));
  EXPECT_EQ(grid.getBinCountX(), kRowSites / 10);
  EXPECT_EQ(grid.getBinCountY(), kRows);
}

TEST_F(PlacementDensityGridTest, DefaultBinsAreTheRoutersGCells)
{
  addRoutingTracks();
  // Cells across gcell edges, a macro over several gcells, and a cell in the
  // top right corner, in the gcells the router stretches to the die edge.
  place(wide_cell_, kGCellSize - kSiteWidth, rowY(1));
  place(cell_, siteX(30), kGCellSize - (kRowHeight / 2));
  place(macro_, siteX(40), rowY(3), dbPlacementStatus::FIRM);
  place(cell_, kCoreUx - (2 * kSiteWidth), rowY(kRows - 1));

  PlacementDensityGrid grid(block_);
  grid.getDensity(core());
  EXPECT_EQ(grid.getBinWidth(), kGCellSize);
  EXPECT_EQ(grid.getBinHeight(), kGCellSize);
  // The gcells start at the die origin, below and left of the core.
  EXPECT_EQ(grid.getBinRect(0, 0),
            Rect(kCoreLx, kCoreLy, kGCellSize, kGCellSize));
  expectBinsMatchTheDb(grid);

  // Every gcell, as the router lays them out, gets its exact area.
  const Rect die = block_->getDieArea();
  const int x_grids = die.dx() / kGCellSize;
  const int y_grids = die.dy() / kGCellSize;
  for (int y = 0; y < y_grids; y++) {
    for (int x = 0; x < x_grids; x++) {
      const Rect gcell(
          die.xMin() + (x * kGCellSize),
          die.yMin() + (y * kGCellSize),
          x == x_grids - 1 ? die.xMax() : die.xMin() + ((x + 1) * kGCellSize),
          y == y_grids - 1 ? die.yMax() : die.yMin() + ((y + 1) * kGCellSize));
      EXPECT_DOUBLE_EQ(grid.getDensity(gcell).instance_area,
                       static_cast<double>(expectedArea(gcell)))
          << "gcell " << x << ", " << y;
    }
  }
}

TEST_F(PlacementDensityGridTest, TheGCellsFollowTheDie)
{
  addRoutingTracks();
  place(cell_, siteX(0), rowY(0));
  PlacementDensityGrid grid(block_);
  grid.getDensity(core());
  EXPECT_EQ(grid.getBinRect(0, 0),
            Rect(kCoreLx, kCoreLy, kGCellSize, kGCellSize));

  // A die reaching further down and left moves the gcells with it.
  const int shift = kGCellSize / 3;
  block_->setDieArea(
      Rect(-shift, -shift, kCoreUx + kSiteWidth, kCoreUy + kRowHeight));
  grid.getDensity(core());
  EXPECT_EQ(grid.getBinRect(0, 0),
            Rect(kCoreLx, kCoreLy, kGCellSize - shift, kGCellSize - shift));
  expectBinsMatchTheDb(grid);
}

TEST_F(PlacementDensityGridTest, RegionQueries)
{
  // A wide cell over the first four sites of bin (0, 0), a cell straddling
  // bins (0, 3) and (1, 3), and a macro over bins (3..4, 5..7).
  place(wide_cell_, siteX(0), rowY(0));
  place(cell_, siteX(9), rowY(3));
  place(macro_, siteX(30), rowY(5), dbPlacementStatus::FIRM);
  PlacementDensityGrid grid(block_, core().ll(), kBinWidth, kBinHeight);

  // Regions made of whole bins are exact.
  for (const Rect& region : {
           core(),
           Rect(siteX(0), rowY(0), siteX(10), rowY(1)),
           Rect(siteX(0), rowY(3), siteX(10), rowY(4)),
           Rect(siteX(10), rowY(2), siteX(50), rowY(8)),
       }) {
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
  for (const Rect& region : {
           Rect(0, 0, kCoreLx, kCoreLy),
           Rect(siteX(5), rowY(5), siteX(5), rowY(6)),
       }) {
    const PlacementDensity density = grid.getDensity(region);
    EXPECT_EQ(density.region_area, 0);
    EXPECT_EQ(density.instance_area, 0);
    EXPECT_EQ(density.getDensity(), 0);
  }
}

TEST_F(PlacementDensityGridTest, BinsMatchTheDbUnderRandomEdits)
{
  // A fixed seed keeps the edit sequence reproducible.
  std::mt19937 rng(1);  // NOLINT(bugprone-random-generator-seed)
  const auto pick = [&rng](const int lo, const int hi) {
    return std::uniform_int_distribution<int>(lo, hi)(rng);
  };
  // Anywhere over the core, at times hanging out of it.
  const auto random_x = [&] { return pick(0, kCoreUx); };
  const auto random_y = [&] { return pick(0, kCoreUy); };
  const std::vector<dbMaster*> masters
      = {cell_, wide_cell_, tap_, endcap_, filler_, pad_, cover_, macro_};
  const std::vector<dbPlacementStatus> statuses = {
      dbPlacementStatus::PLACED,
      dbPlacementStatus::UNPLACED,
      dbPlacementStatus::FIRM,
      dbPlacementStatus::LOCKED,
      dbPlacementStatus::COVER,
  };
  const std::vector<dbOrientType> orients = {
      dbOrientType::R0,
      dbOrientType::R90,
      dbOrientType::R180,
      dbOrientType::MX,
      dbOrientType::MYR90,
  };

  std::vector<dbInst*> insts;
  insts.reserve(60);
  for (int i = 0; i < 60; i++) {
    insts.push_back(place(masters[pick(0, 3)], random_x(), random_y()));
  }
  // Blocked area: a row cut, a hard blockage and a soft one, and below, the
  // hard halos of macros and pads.
  cutRow(4, 30, 60);
  odb::dbBlockage::create(block_, siteX(10), rowY(6), siteX(25), rowY(8));
  odb::dbBlockage::create(block_, siteX(60), rowY(1), siteX(90), rowY(3))
      ->setSoft();
  // Bins on a grid through the die origin that lines up with neither the
  // sites, the rows nor the core, so the first and last ones are clipped to
  // the core.
  PlacementDensityGrid grid(block_, odb::Point(0, 0), 1700, 1300);
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
      case 4: {
        dbMaster* master = masters[pick(0, 7)];
        dbInst* made = dbInst::create(
            block_, master, ("u" + std::to_string(inst_count_++)).c_str());
        // odb has no callback for a halo, so it is set before the instance
        // is placed.
        if (master == macro_ || master == pad_) {
          made->setHalo(
              pick(0, 500), pick(0, 500), pick(0, 500), pick(0, 500), false);
        }
        made->setLocation(random_x(), random_y());
        made->setPlacementStatus(statuses[pick(0, 4)]);
        insts.push_back(made);
        break;
      }
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
      PlacementDensityGrid fresh(block_, odb::Point(0, 0), 1700, 1300);
      for (int query = 0; query < 10; query++) {
        const Rect region(random_x(), random_y(), random_x(), random_y());
        const PlacementDensity got = grid.getDensity(region);
        const PlacementDensity want = fresh.getDensity(region);
        EXPECT_EQ(got.region_area, want.region_area);
        EXPECT_DOUBLE_EQ(got.instance_area, want.instance_area);
        EXPECT_DOUBLE_EQ(got.blocked_area, want.blocked_area);
      }
    }
  }
}

TEST_F(PlacementDensityGridTest, JournalUndoLeavesNothingBehind)
{
  dbInst* moved = place(cell_, siteX(0), rowY(0));
  dbInst* resized = place(cell_, siteX(20), rowY(2));
  PlacementDensityGrid grid(block_, core().ll(), kBinWidth, kBinHeight);
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
  PlacementDensityGrid grid(block_, core().ll(), kBinWidth, kBinHeight);
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

TEST_F(PlacementDensityGridTest, BlockedAreaIsWhereNoCellCanGo)
{
  // Row 9 stops 20 sites short of the core's right edge.
  cutRow(9, 80, kRowSites);
  // Hard blockages: one hanging out of the core, two that overlap, which
  // count once, and one over the end of row 9, which is blocked anyway.  A
  // partial blockage blocks whole, as in detailed placement; a soft one
  // does not block.
  odb::dbBlockage::create(block_, 0, 0, siteX(5), rowY(1));
  odb::dbBlockage::create(block_, siteX(70), rowY(2), siteX(80), rowY(4));
  odb::dbBlockage::create(block_, siteX(75), rowY(3), siteX(85), rowY(4));
  odb::dbBlockage::create(block_, siteX(85), rowY(9), siteX(95), rowY(10));
  odb::dbBlockage::create(block_, siteX(80), rowY(2), siteX(90), rowY(3))
      ->setMaxDensity(50);
  odb::dbBlockage::create(block_, siteX(90), rowY(2), siteX(100), rowY(4))
      ->setSoft();
  place(cell_, siteX(0), rowY(5));

  PlacementDensityGrid grid(block_, core().ll(), kBinWidth, kBinHeight);
  const PlacementDensity density = grid.getDensity(core());
  // Half of bin (0, 0), bins (7, 2) and (7, 3), half of bin (8, 3), bin
  // (8, 2), and bins (8, 9) and (9, 9).
  const int64_t bin = int64_t{kBinWidth} * kBinHeight;
  const int64_t blocked = (bin / 2) + (2 * bin) + (bin / 2) + bin + (2 * bin);
  EXPECT_DOUBLE_EQ(density.blocked_area, static_cast<double>(blocked));
  EXPECT_EQ(grid.getBinBlockedArea(8, 3), bin / 2);
  EXPECT_EQ(grid.getBinBlockedArea(9, 2), 0);
  EXPECT_DOUBLE_EQ(density.getFreeArea(),
                   static_cast<double>(core().area() - blocked - area(cell_)));
  // The heat map's density leaves the blocked area out.
  EXPECT_DOUBLE_EQ(
      density.getDensity(),
      static_cast<double>(area(cell_)) / static_cast<double>(core().area()));
  expectBinsMatchTheDb(grid);
}

TEST_F(PlacementDensityGridTest, MacrosAreInstanceAreaAndTheirHalosBlocked)
{
  // A macro over sites 40 to 60 of rows 5 to 7, with a hard halo a site
  // wide on its left and right, and the rows cut around both, as cut_rows
  // does.
  place(macro_, siteX(40), rowY(5), dbPlacementStatus::FIRM)
      ->setHalo(kSiteWidth, 0, kSiteWidth, 0, false);
  for (int row = 5; row < 8; row++) {
    cutRow(row, 39, 61);
  }
  // A macro on the rows, over sites 0 to 20 of rows 0 to 2, with a hard halo
  // two sites wide on its right.
  place(macro_, siteX(0), rowY(0), dbPlacementStatus::FIRM)
      ->setHalo(0, 0, 2 * kSiteWidth, 0, false);
  // A cell with a hard halo a site wide, and a wide cell with a soft one.
  place(cell_, siteX(80), rowY(8))
      ->setHalo(kSiteWidth, 0, kSiteWidth, 0, false);
  place(wide_cell_, siteX(80), rowY(0))
      ->setHalo(2 * kSiteWidth, 0, 2 * kSiteWidth, 0, true);

  PlacementDensityGrid grid(block_, core().ll(), kBinWidth, kBinHeight);
  // The area of a site in a row.
  const int64_t site = int64_t{kSiteWidth} * kRowHeight;
  const auto expect_areas = [&](const Rect& region,
                                const int64_t instance,
                                const int64_t blocked) {
    const PlacementDensity density = grid.getDensity(region);
    EXPECT_DOUBLE_EQ(density.instance_area, static_cast<double>(instance));
    EXPECT_DOUBLE_EQ(density.blocked_area, static_cast<double>(blocked));
    EXPECT_DOUBLE_EQ(density.getFreeArea(),
                     static_cast<double>(region.area() - instance - blocked));
  };
  // Sites 30 to 70 of rows 5 to 7: the macro is instance area and its halo,
  // 2 sites by 3 rows, blocked, though neither is on a row, which leaves 18
  // sites a row free.
  expect_areas(
      Rect(siteX(30), rowY(5), siteX(70), rowY(8)), area(macro_), 6 * site);
  // Sites 0 to 30 of rows 0 to 2: the same, on the rows.
  expect_areas(
      Rect(siteX(0), rowY(0), siteX(30), rowY(3)), area(macro_), 6 * site);
  // Sites 70 to 90 of row 8: the cell is instance area, its halo blocked.
  expect_areas(
      Rect(siteX(70), rowY(8), siteX(90), rowY(9)), area(cell_), 2 * site);
  // Sites 70 to 90 of row 0: the soft halo blocks nothing.
  expect_areas(
      Rect(siteX(70), rowY(0), siteX(90), rowY(1)), area(wide_cell_), 0);
  expectBinsMatchTheDb(grid);
}

TEST_F(PlacementDensityGridTest, BlockedAreaFollowsTheDb)
{
  dbInst* macro = place(macro_, siteX(40), rowY(5));
  macro->setHalo(kSiteWidth, kRowHeight, kSiteWidth, kRowHeight, false);
  PlacementDensityGrid grid(block_, core().ll(), kBinWidth, kBinHeight);
  const auto blocked = [&] { return grid.getDensity(core()).blocked_area; };
  // The halo: 22 sites by 5 rows, less the macro's 20 by 3.
  const int64_t site = int64_t{kSiteWidth} * kRowHeight;
  const int64_t halo = ((22 * 5) - (20 * 3)) * site;
  EXPECT_DOUBLE_EQ(blocked(), static_cast<double>(halo));

  // A blockage over 10 sites of rows 0 and 1, and row 9 gone and back.
  odb::dbBlockage* blockage
      = odb::dbBlockage::create(block_, siteX(0), rowY(0), siteX(10), rowY(2));
  EXPECT_DOUBLE_EQ(blocked(), static_cast<double>(halo + (20 * site)));
  cutRow(9, 0, kRowSites);
  EXPECT_DOUBLE_EQ(blocked(), static_cast<double>(halo + (120 * site)));
  addRow(9, 0, kRowSites);
  EXPECT_DOUBLE_EQ(blocked(), static_cast<double>(halo + (20 * site)));
  expectBinsMatchTheDb(grid);

  // The halo follows the macro as it moves and grows, and goes with it when
  // unplaced.
  macro->setLocation(siteX(10), rowY(3));
  expectBinsMatchTheDb(grid);
  macro->swapMaster(makeMaster("wide_macro", dbMasterType::BLOCK, 30, 3));
  expectBinsMatchTheDb(grid);
  macro->setPlacementStatus(dbPlacementStatus::UNPLACED);
  EXPECT_DOUBLE_EQ(blocked(), static_cast<double>(20 * site));

  // A macro over the blockage takes its area as instance area, until it is
  // swapped for a cell.
  dbInst* bare = place(macro_, siteX(0), rowY(0));
  EXPECT_DOUBLE_EQ(blocked(), 0);
  bare->swapMaster(cell_);
  EXPECT_DOUBLE_EQ(blocked(), static_cast<double>(20 * site));
  expectBinsMatchTheDb(grid);
  odb::dbBlockage::destroy(blockage);
  EXPECT_DOUBLE_EQ(blocked(), 0);
  expectBinsMatchTheDb(grid);
}

}  // namespace
}  // namespace rsz
