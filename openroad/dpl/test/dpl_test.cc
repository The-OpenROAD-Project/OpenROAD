// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <string>

#include "dpl/Opendp.h"
#include "gtest/gtest.h"
#include "infrastructure/Coordinates.h"
#include "infrastructure/Grid.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "odb/lefin.h"
#include "tst/fixture.h"
#include "tst/nangate45_fixture.h"
#include "utl/Logger.h"

namespace dpl {

class OpendpTest : public tst::Fixture
{
 protected:
  void SetUp() override
  {
    lib_ = loadTechAndLib(
        "tech", "isPlacedTestLibName", "sky130hd/sky130_fd_sc_hd_merged.lef");

    chip_ = odb::dbChip::create(db_.get(), db_->getTech());
    block_ = odb::dbBlock::create(chip_, "top");
    block_->setDefUnits(lib_->getTech()->getLefUnits());
    block_->setDieArea(odb::Rect(0, 0, 1000, 1000));
  }

  odb::dbLib* lib_;
  odb::dbChip* chip_;
  odb::dbBlock* block_;
};

namespace {

class FixedMacroFootprintTest
    : public tst::Nangate45Fixture,
      public ::testing::WithParamInterface<odb::dbOrientType>
{
 protected:
  void SetUp() override
  {
    site_ = lib_->findSite("FreePDK45_38x28_10R_NP_162NW_34O");
    ASSERT_NE(site_, nullptr);
    site_width_ = site_->getWidth();
    row_height_ = site_->getHeight();
    core_x_ = 10 * site_width_;
    core_y_ = row_height_;
    block_->setDieArea(odb::Rect(0, 0, 120 * site_width_, 12 * row_height_));
    for (int row = 0; row < 10; ++row) {
      const std::string name = "ROW_" + std::to_string(row);
      odb::dbRow::create(
          block_,
          name.c_str(),
          site_,
          core_x_,
          core_y_ + row * row_height_,
          row % 2 == 0 ? odb::dbOrientType::R0 : odb::dbOrientType::MX,
          odb::dbRowDir::HORIZONTAL,
          100,
          site_width_);
    }
    block_->setCoreArea(block_->computeCoreArea());

    auto* master = odb::dbMaster::create(lib_, "macro");
    master->setType(odb::dbMasterType::BLOCK);
    master->setWidth(20 * site_width_);
    master->setHeight(2 * row_height_);
    master->setFrozen();
    macro_ = odb::dbInst::create(block_, master, "macro");
    macro_->setOrient(GetParam());
    macro_->setLocation(core_x_ + 10 * site_width_, core_y_ + 2 * row_height_);
    macro_->setPlacementStatus(odb::dbPlacementStatus::FIRM);
  }

  odb::dbInst* makeCell(const odb::Point& location)
  {
    auto* cell = makeInst(
        block_,
        lib_->findMaster("INV_X1"),
        "cell",
        {.location = location, .status = odb::dbPlacementStatus::PLACED});
    cell->setOrient(odb::dbOrientType::R0);
    return cell;
  }

  void legalize()
  {
    const odb::Point macro_location = macro_->getLocation();
    ASSERT_NO_THROW(dp_.detailedPlacement(0, 0));
    ASSERT_NO_THROW(dp_.checkPlacement(false));
    EXPECT_EQ(macro_->getLocation(), macro_location);
    EXPECT_EQ(macro_->getOrient(), GetParam());
  }

  odb::dbSite* site_ = nullptr;
  odb::dbInst* macro_ = nullptr;
  int site_width_ = 0;
  int row_height_ = 0;
  int core_x_ = 0;
  int core_y_ = 0;
  Opendp dp_{db_.get(), &logger_};
};

TEST_P(FixedMacroFootprintTest, LegalCellBesideMacroStaysInPlace)
{
  const odb::Rect bbox = macro_->getBBox()->getBox();
  const int first_site
      = (bbox.xMax() - core_x_ + site_width_ - 1) / site_width_;
  auto* cell = makeCell(
      {core_x_ + first_site * site_width_, core_y_ + 2 * row_height_});
  const odb::Point initial_location = cell->getLocation();
  ASSERT_FALSE(bbox.overlaps(cell->getBBox()->getBox()));

  legalize();

  EXPECT_EQ(cell->getLocation(), initial_location);
  EXPECT_FALSE(bbox.overlaps(cell->getBBox()->getBox()));
}

TEST_P(FixedMacroFootprintTest, CellAboveUnrotatedFootprintIsLegalized)
{
  const odb::Rect bbox = macro_->getBBox()->getBox();
  auto* cell = makeCell({bbox.xMin() + site_width_, core_y_ + 4 * row_height_});

  legalize();

  EXPECT_FALSE(bbox.overlaps(cell->getBBox()->getBox()));
}

TEST_P(FixedMacroFootprintTest, CellAtRightEdgeDoesNotOverlapMacro)
{
  const odb::Rect bbox = macro_->getBBox()->getBox();
  const int last_site = (bbox.xMax() - core_x_ - 1) / site_width_;
  auto* cell = makeCell(
      {core_x_ + last_site * site_width_, core_y_ + 2 * row_height_});
  ASSERT_TRUE(bbox.overlaps(cell->getBBox()->getBox()));

  legalize();

  EXPECT_FALSE(bbox.overlaps(cell->getBBox()->getBox()));
}

TEST_P(FixedMacroFootprintTest, MacroCrossingCoreEdgeKeepsItsBlockageInPlace)
{
  block_->setDieArea(odb::Rect(0, 0, 140 * site_width_, 12 * row_height_));
  macro_->setPlacementStatus(odb::dbPlacementStatus::PLACED);
  macro_->setLocation(core_x_ + 95 * site_width_, core_y_ + 2 * row_height_);
  macro_->setPlacementStatus(odb::dbPlacementStatus::FIRM);
  auto* cell
      = makeCell({core_x_ + 90 * site_width_, core_y_ + 2 * row_height_});
  const odb::Point initial_location = cell->getLocation();
  ASSERT_FALSE(macro_->getBBox()->getBox().overlaps(cell->getBBox()->getBox()));

  legalize();

  EXPECT_EQ(cell->getLocation(), initial_location);
}

INSTANTIATE_TEST_SUITE_P(
    Orientations,
    FixedMacroFootprintTest,
    ::testing::Values(odb::dbOrientType::R0,
                      odb::dbOrientType::R90,
                      odb::dbOrientType::R180,
                      odb::dbOrientType::R270,
                      odb::dbOrientType::MX,
                      odb::dbOrientType::MY,
                      odb::dbOrientType::MXR90,
                      odb::dbOrientType::MYR90),
    [](const ::testing::TestParamInfo<odb::dbOrientType>& info) {
      return info.param.getString();
    });

class GcdPlacementTest : public tst::Fixture
{
};

TEST_F(GcdPlacementTest, RotatedMacroKeepsPlacementLegal)
{
  auto* lib
      = loadTechAndLib("ng45", "ng45", "_main/test/Nangate45/Nangate45.lef");
  auto* block = readDef("_main/openroad/dpl/test/gcd_replace.def", {lib});
  block->setCoreArea(block->computeCoreArea());
  const odb::Rect core = block->getCoreArea();
  auto* site = lib->findSite("FreePDK45_38x28_10R_NP_162NW_34O");
  ASSERT_NE(site, nullptr);

  auto* master = odb::dbMaster::create(lib, "macro");
  master->setType(odb::dbMasterType::BLOCK);
  master->setWidth(20 * site->getWidth());
  master->setHeight(2 * site->getHeight());
  master->setFrozen();
  auto* macro = odb::dbInst::create(block, master, "macro");
  macro->setOrient(odb::dbOrientType::R90);
  const odb::Point location(core.xMin() + 200 * site->getWidth(),
                            core.yMin() + 20 * site->getHeight());
  macro->setLocation(location.x(), location.y());
  macro->setPlacementStatus(odb::dbPlacementStatus::FIRM);

  Opendp dp(db_.get(), &logger_);
  ASSERT_NO_THROW(dp.detailedPlacement(0, 0));
  EXPECT_NO_THROW(dp.checkPlacement(false));
  EXPECT_EQ(macro->getLocation(), location);
  EXPECT_EQ(macro->getOrient(), odb::dbOrientType::R90);
}

}  // namespace

// Four Nangate45 rows of 32 sites starting at (kCoreX, kCoreY), plus the
// BLOCK1 macro from extra.lef.  Row 2 is fragmented around a hole spanning
// sites [kHoleStart, kHoleEnd), as rows are cut around macros.  Coordinates
// are in DBU (2000 per micron).
class CheckPlacementTest : public tst::Nangate45Fixture
{
 protected:
  static constexpr int kSiteWidth = 380;
  static constexpr int kRowHeight = 2800;
  static constexpr int kCoreX = 3800;
  static constexpr int kCoreY = 2800;
  static constexpr int kNumSites = 32;
  static constexpr int kNumRows = 4;
  static constexpr int kHoleRow = 2;
  static constexpr int kHoleStart = 12;
  static constexpr int kHoleEnd = 20;

  void SetUp() override
  {
    ASSERT_TRUE(updateLib(lib_, "_main/openroad/dpl/test/extra.lef"));
    block_->setDieArea(odb::Rect(0, 0, 20000, 20000));
    site_ = lib_->findSite("FreePDK45_38x28_10R_NP_162NW_34O");
    ASSERT_NE(site_, nullptr);
    for (int row = 0; row < kNumRows; ++row) {
      const odb::dbOrientType orient
          = (row % 2 == 0) ? odb::dbOrientType::MX : odb::dbOrientType::R0;
      const int y = kCoreY + row * kRowHeight;
      const std::string name = "ROW_" + std::to_string(row);
      if (row != kHoleRow) {
        makeRow(name, 0, kNumSites, y, orient);
        continue;
      }
      makeRow(name + "_left", 0, kHoleStart, y, orient);
      makeRow(name + "_right", kHoleEnd, kNumSites, y, orient);
    }
    block_->setCoreArea(block_->computeCoreArea());
  }

  void makeRow(const std::string& name,
               const int first_site,
               const int end_site,
               const int y,
               const odb::dbOrientType orient)
  {
    odb::dbRow::create(block_,
                       name.c_str(),
                       site_,
                       kCoreX + first_site * kSiteWidth,
                       y,
                       orient,
                       odb::dbRowDir::HORIZONTAL,
                       end_site - first_site,
                       kSiteWidth);
  }

  odb::dbInst* makeCell(const char* master_name,
                        const char* inst_name,
                        const odb::Point& location,
                        const odb::dbPlacementStatus status)
  {
    odb::dbMaster* master = lib_->findMaster(master_name);
    if (master == nullptr) {
      // ASSERT_NE needs a void function, so report and return instead.
      ADD_FAILURE() << "Master " << master_name << " not found";
      return nullptr;
    }
    return makeInst(block_,
                    master,
                    inst_name,
                    {.location = location, .status = status, .iterms = {}});
  }

  // Run the check and return the logger error id it raised, or "" if none.
  std::string runCheck(const bool check_fixed = true,
                       const bool check_placeable = true)
  {
    try {
      dp_.checkPlacement(/*verbose=*/false, "", check_fixed, check_placeable);
    } catch (const std::runtime_error& error) {
      return error.what();
    }
    return "";
  }

  odb::dbMarkerCategory* findFailures(const char* category_name)
  {
    odb::dbMarkerCategory* dpl_category = block_->findMarkerCategory("DPL");
    if (dpl_category == nullptr) {
      return nullptr;
    }
    return dpl_category->findMarkerCategory(category_name);
  }

  // Number of markers saved under the given DPL failure category.
  int failureCount(const char* category_name)
  {
    odb::dbMarkerCategory* failures = findFailures(category_name);
    return failures == nullptr ? 0 : failures->getMarkerCount();
  }

  odb::dbSite* site_ = nullptr;
  Opendp dp_{db_.get(), &logger_};
};

TEST_F(CheckPlacementTest, FixedOnlyIgnoresMovableCells)
{
  // Legal fixed tapcell in the first site of the first row.
  makeCell("TAPCELL_X1", "tap", {kCoreX, kCoreY}, odb::dbPlacementStatus::FIRM);
  // Legal fixed macro further up in the core.
  makeCell("BLOCK1",
           "macro",
           {kCoreX + 10 * kSiteWidth, kCoreY + kRowHeight},
           odb::dbPlacementStatus::FIRM);
  // Movable cell at a stale, misaligned location overlapping the tapcell,
  // as left behind by a floorplan-stage database.
  makeCell("BUF_X1",
           "buf",
           {kCoreX + 100, kCoreY + 100},
           odb::dbPlacementStatus::PLACED);

  EXPECT_EQ(runCheck(/*check_fixed=*/true, /*check_placeable=*/false), "");
  EXPECT_EQ(block_->findMarkerCategory("DPL"), nullptr);

  // The full check still reports the movable cell.
  EXPECT_EQ(runCheck(), "DPL-0033");
  EXPECT_EQ(failureCount("Site_alignment_failures"), 1);
}

TEST_F(CheckPlacementTest, FixedOnlyReportsMisalignedTapcell)
{
  odb::dbInst* tap = makeCell("TAPCELL_X1",
                              "tap",
                              {kCoreX + 100, kCoreY},
                              odb::dbPlacementStatus::FIRM);

  EXPECT_EQ(runCheck(/*check_fixed=*/true, /*check_placeable=*/false),
            "DPL-0041");

  odb::dbMarkerCategory* failures = findFailures("Site_alignment_failures");
  ASSERT_NE(failures, nullptr);
  ASSERT_EQ(failures->getMarkerCount(), 1);
  odb::dbMarker* marker = *failures->getMarkers().begin();
  EXPECT_TRUE(marker->getSources().contains(tap));
}

TEST_F(CheckPlacementTest, FixedOnlyReportsTapcellInRowHole)
{
  // Site aligned and on a row coordinate, but inside the hole of row 2.
  makeCell(
      "TAPCELL_X1",
      "tap",
      {kCoreX + (kHoleStart + 2) * kSiteWidth, kCoreY + kHoleRow * kRowHeight},
      odb::dbPlacementStatus::FIRM);

  EXPECT_EQ(runCheck(/*check_fixed=*/true, /*check_placeable=*/false),
            "DPL-0041");
  EXPECT_EQ(failureCount("In_rows_failures"), 1);
}

TEST_F(CheckPlacementTest, FixedOnlyReportsMacroOverlappingTapcell)
{
  makeCell("TAPCELL_X1", "tap", {kCoreX, kCoreY}, odb::dbPlacementStatus::FIRM);
  makeCell("BLOCK1", "macro", {kCoreX, kCoreY}, odb::dbPlacementStatus::FIRM);

  EXPECT_EQ(runCheck(/*check_fixed=*/true, /*check_placeable=*/false),
            "DPL-0041");
  EXPECT_EQ(failureCount("Overlap_failures"), 1);
}

TEST_F(CheckPlacementTest, FixedOnlyChecksPlacedButUnfixedMacro)
{
  makeCell("TAPCELL_X1", "tap", {kCoreX, kCoreY}, odb::dbPlacementStatus::FIRM);
  // A placed macro that was never marked fixed is still part of the
  // floorplan and must be checked.
  makeCell("BLOCK1", "macro", {kCoreX, kCoreY}, odb::dbPlacementStatus::PLACED);

  EXPECT_EQ(runCheck(/*check_fixed=*/true, /*check_placeable=*/false),
            "DPL-0041");
  EXPECT_EQ(failureCount("Overlap_failures"), 1);
}

TEST_F(CheckPlacementTest, FixedOnlyErrorsOnUnplacedMacro)
{
  makeCell("BLOCK1", "macro", {kCoreX, kCoreY}, odb::dbPlacementStatus::NONE);

  EXPECT_EQ(runCheck(/*check_fixed=*/true, /*check_placeable=*/false),
            "DPL-0405");
}

TEST_F(CheckPlacementTest, FixedOnlyReportsBlockedPinAccess)
{
  odb::dbInst* buf = makeCell(
      "BUF_X1", "buf", {kCoreX, kCoreY}, odb::dbPlacementStatus::FIRM);
  ASSERT_NE(buf, nullptr);
  odb::dbNet* power = odb::dbNet::create(block_, "VDD");
  power->setSigType(odb::dbSigType::POWER);
  power->setSpecial();
  buf->findITerm("VDD")->connect(power);
  odb::dbSWire* wire = odb::dbSWire::create(power, odb::dbWireType::ROUTED);
  odb::dbTechLayer* metal2 = db_->getTech()->findLayer("metal2");
  ASSERT_NE(metal2, nullptr);
  ASSERT_NE(odb::dbSBox::create(wire,
                                metal2,
                                kCoreX + kSiteWidth,
                                kCoreY,
                                kCoreX + 2 * kSiteWidth,
                                kCoreY + kRowHeight,
                                odb::dbWireShapeType::STRIPE),
            nullptr);

  // Unconnected signal pins and supply connections do not need via access.
  EXPECT_EQ(runCheck(/*check_fixed=*/true, /*check_placeable=*/false), "");
  EXPECT_EQ(runCheck(), "");

  // The signal pin is on metal1, so the metal2 stripe blocks its via access.
  odb::dbNet* signal = odb::dbNet::create(block_, "input");
  buf->findITerm("A")->connect(signal);
  EXPECT_EQ(runCheck(/*check_fixed=*/true, /*check_placeable=*/false),
            "DPL-0041");
  odb::dbMarkerCategory* failures = findFailures("Blocked_layers_failures");
  ASSERT_NE(failures, nullptr);
  ASSERT_EQ(failures->getMarkerCount(), 1);
  odb::dbMarker* marker = *failures->getMarkers().begin();
  EXPECT_TRUE(marker->getSources().contains(buf));

  EXPECT_EQ(runCheck(), "DPL-0033");
  EXPECT_EQ(failureCount("Blocked_layers_failures"), 1);
}

TEST_F(CheckPlacementTest, PlaceableIgnoresMisalignedTapcell)
{
  makeCell("TAPCELL_X1",
           "tap",
           {kCoreX + 100, kCoreY},
           odb::dbPlacementStatus::FIRM);
  makeCell("BUF_X1",
           "buf",
           {kCoreX + 10 * kSiteWidth, kCoreY},
           odb::dbPlacementStatus::PLACED);

  EXPECT_EQ(runCheck(/*check_fixed=*/false, /*check_placeable=*/true), "");
  EXPECT_EQ(block_->findMarkerCategory("DPL"), nullptr);
}

TEST_F(CheckPlacementTest, PlaceableReportsOverlapWithFixedCell)
{
  makeCell("TAPCELL_X1", "tap", {kCoreX, kCoreY}, odb::dbPlacementStatus::FIRM);
  // Sorts before the tapcell, so it is visited first by the check loop.
  odb::dbInst* buf = makeCell(
      "BUF_X1", "buf", {kCoreX, kCoreY}, odb::dbPlacementStatus::PLACED);

  EXPECT_EQ(runCheck(/*check_fixed=*/false, /*check_placeable=*/true),
            "DPL-0033");

  odb::dbMarkerCategory* failures = findFailures("Overlap_failures");
  ASSERT_NE(failures, nullptr);
  ASSERT_EQ(failures->getMarkerCount(), 1);
  odb::dbMarker* marker = *failures->getMarkers().begin();
  EXPECT_TRUE(marker->getSources().contains(buf));
}

TEST_F(CheckPlacementTest, PlaceableReportsOverlapWithMisalignedFixedCell)
{
  // The misaligned tapcell itself is not checked, but still blocks the buffer.
  makeCell("TAPCELL_X1",
           "tap",
           {kCoreX + 100, kCoreY},
           odb::dbPlacementStatus::FIRM);
  makeCell("BUF_X1", "buf", {kCoreX, kCoreY}, odb::dbPlacementStatus::PLACED);

  EXPECT_EQ(runCheck(/*check_fixed=*/false, /*check_placeable=*/true),
            "DPL-0033");
  EXPECT_EQ(failureCount("Overlap_failures"), 1);
  EXPECT_EQ(failureCount("Site_alignment_failures"), 0);
}

TEST_F(CheckPlacementTest, DetailedPlacementAvoidsMisalignedFixedMacroOverlap)
{
  // BLOCK1 is 4000x4000 DBU (10.53 sites x 1.43 rows). Placed off-grid at
  // (+350, +2000) from core origin, it physically spans columns [0, 12) and
  // rows [0, 3) even though gridRoundX/Y snaps to (1, 1) and gridHeight is 2.
  makeCell("BLOCK1",
           "macro",
           {kCoreX + 350, kCoreY + 2000},
           odb::dbPlacementStatus::FIRM);
  makeCell(
      "BUF_X1", "buf_bl", {kCoreX, kCoreY}, odb::dbPlacementStatus::PLACED);
  makeCell("BUF_X1",
           "buf_top",
           {kCoreX, kCoreY + 2 * kRowHeight},
           odb::dbPlacementStatus::PLACED);
  makeCell("BUF_X1",
           "buf_right",
           {kCoreX + 11 * kSiteWidth, kCoreY + kRowHeight},
           odb::dbPlacementStatus::PLACED);

  dp_.detailedPlacement(/*max_displacement_x=*/0, /*max_displacement_y=*/0);

  EXPECT_EQ(runCheck(/*check_fixed=*/false, /*check_placeable=*/true), "");
  EXPECT_EQ(failureCount("Overlap_failures"), 0);
}

TEST_F(CheckPlacementTest, LegalCellPosAfterDetailedPlacementLeavesMacro)
{
  // detailed_placement frees its pixel grid on return. The resizer then
  // calls legalCellPos for a cell it moved, without placing again, and the
  // cell must still be moved off a macro, which legalCellPos finds in the
  // pixels.
  // The macro is away from the core's edges, so the cell has room beside it.
  odb::dbInst* macro = makeCell("BLOCK1",
                                "macro",
                                {kCoreX + 10 * kSiteWidth, kCoreY},
                                odb::dbPlacementStatus::FIRM);
  odb::dbInst* buf = makeCell("BUF_X1",
                              "buf",
                              {kCoreX + 24 * kSiteWidth, kCoreY},
                              odb::dbPlacementStatus::PLACED);
  ASSERT_NE(macro, nullptr);
  ASSERT_NE(buf, nullptr);

  dp_.detailedPlacement(/*max_displacement_x=*/0, /*max_displacement_y=*/0);

  const odb::Rect macro_box = macro->getBBox()->getBox();
  // One site in from the macro's left edge, one row up: the left edge is
  // the nearest way off it.
  buf->setLocation(kCoreX + 11 * kSiteWidth, kCoreY + kRowHeight);
  ASSERT_TRUE(buf->getBBox()->getBox().overlaps(macro_box));

  dp_.legalCellPos(buf);

  EXPECT_FALSE(buf->getBBox()->getBox().overlaps(macro_box));
}

TEST_F(CheckPlacementTest, LegalCellPosAfterFixedOnlyCheckLeavesMacro)
{
  // A fixed-only check_placement clears the whole grid on return, rows and
  // pixels, along with the network it made without the movable cells. The
  // resizer can still call legalCellPos for a cell it moved, and the cell
  // must be moved off a macro, which legalCellPos finds in the pixels.
  odb::dbInst* macro = makeCell("BLOCK1",
                                "macro",
                                {kCoreX + 10 * kSiteWidth, kCoreY},
                                odb::dbPlacementStatus::FIRM);
  odb::dbInst* buf = makeCell("BUF_X1",
                              "buf",
                              {kCoreX + 24 * kSiteWidth, kCoreY},
                              odb::dbPlacementStatus::PLACED);
  ASSERT_NE(macro, nullptr);
  ASSERT_NE(buf, nullptr);

  ASSERT_EQ(runCheck(/*check_fixed=*/true, /*check_placeable=*/false), "");

  const odb::Rect macro_box = macro->getBBox()->getBox();
  buf->setLocation(kCoreX + 11 * kSiteWidth, kCoreY + kRowHeight);
  ASSERT_TRUE(buf->getBBox()->getBox().overlaps(macro_box));

  dp_.legalCellPos(buf);

  EXPECT_FALSE(buf->getBBox()->getBox().overlaps(macro_box));
}

TEST_F(CheckPlacementTest, LegalCellPosRebuildsReleasedGridOnce)
{
  // detailed_placement frees the pixels on return. The first legalCellPos
  // after it makes them again; the ones after that use them as they are.
  odb::dbInst* macro = makeCell("BLOCK1",
                                "macro",
                                {kCoreX + 10 * kSiteWidth, kCoreY},
                                odb::dbPlacementStatus::FIRM);
  odb::dbInst* buf = makeCell("BUF_X1",
                              "buf",
                              {kCoreX + 24 * kSiteWidth, kCoreY},
                              odb::dbPlacementStatus::PLACED);
  ASSERT_NE(macro, nullptr);
  ASSERT_NE(buf, nullptr);

  dp_.detailedPlacement(/*max_displacement_x=*/0, /*max_displacement_y=*/0);
  ASSERT_TRUE(dp_.grid_->pixelsReleased());

  const odb::Rect macro_box = macro->getBBox()->getBox();
  buf->setLocation(kCoreX + 11 * kSiteWidth, kCoreY + kRowHeight);
  dp_.legalCellPos(buf);
  EXPECT_FALSE(buf->getBBox()->getBox().overlaps(macro_box));
  ASSERT_TRUE(dp_.grid_->hasPixels());
  EXPECT_FALSE(dp_.grid_->pixelsReleased());

  // Making the grid again resets every pixel, so a mark left in one
  // survives the second call only if that call kept the grid.
  Pixel* pixel = dp_.grid_->gridPixel(GridX{0}, GridY{0});
  ASSERT_NE(pixel, nullptr);
  pixel->util = 42.0;

  buf->setLocation(kCoreX + 12 * kSiteWidth, kCoreY + kRowHeight);
  dp_.legalCellPos(buf);
  EXPECT_FALSE(buf->getBBox()->getBox().overlaps(macro_box));
  EXPECT_EQ(dp_.grid_->gridPixel(GridX{0}, GridY{0})->util, 42.0);
}

// Higher routing layers isolate supply-via checks from legacy pin blockages.
class FixedSupplyViaTest : public tst::Nangate45Fixture
{
 protected:
  static constexpr int kSiteWidth = 380;
  static constexpr int kRowHeight = 2800;
  static constexpr int kCellX = 10 * kSiteWidth;

  void SetUp() override
  {
    odb::dbTech* tech = db_->getTech();
    routing_ = odb::dbTechLayer::create(
        tech, "zero_metal", odb::dbTechLayerType::ROUTING);
    cut_
        = odb::dbTechLayer::create(tech, "zero_cut", odb::dbTechLayerType::CUT);
    odb::dbTechLayer* top = odb::dbTechLayer::create(
        tech, "zero_top", odb::dbTechLayerType::ROUTING);
    routing_->setSpacing(0);
    cut_->setSpacing(0);
    top->setSpacing(0);
    via_ = odb::dbTechVia::create(tech, "zero_via");
    odb::dbBox::create(via_, routing_, -50, -50, 50, 50);
    odb::dbBox::create(via_, cut_, -50, -50, 50, 50);
    odb::dbBox::create(via_, top, -50, -50, 50, 50);

    site_ = lib_->findSite("FreePDK45_38x28_10R_NP_162NW_34O");
    ASSERT_NE(site_, nullptr);
    makeRowAndSupply();
  }

  void makeRowAndSupply()
  {
    block_->setDefUnits(lib_->getTech()->getLefUnits());
    block_->setDieArea(odb::Rect(0, 0, 32 * kSiteWidth, kRowHeight));
    makeRow();
    supply_ = odb::dbNet::create(block_, "VDD");
    supply_->setSigType(odb::dbSigType::POWER);
    supply_->setSpecial();
  }

  void makeRow(const odb::dbOrientType orient = odb::dbOrientType::R0)
  {
    odb::dbRow::create(block_,
                       "ROW_0",
                       site_,
                       0,
                       0,
                       orient,
                       odb::dbRowDir::HORIZONTAL,
                       32,
                       kSiteWidth);
    block_->setCoreArea(block_->computeCoreArea());
  }

  void makeCell(odb::dbTechLayer* layer,
                const bool terminal = false,
                const odb::dbSigType sig_type = odb::dbSigType::POWER,
                const odb::dbOrientType orient = odb::dbOrientType::R0)
  {
    odb::dbMaster* master = odb::dbMaster::create(lib_, "VIA_CELL");
    master->setType(odb::dbMasterType::CORE);
    master->setWidth(kSiteWidth);
    master->setHeight(kRowHeight);
    master->setSite(site_);
    if (orient == odb::dbOrientType::MX) {
      master->setSymmetryX();
      site_->setSymmetryX();
      odb::dbRow::destroy(*block_->getRows().begin());
      makeRow(orient);
    }
    if (terminal) {
      odb::dbMTerm* term
          = odb::dbMTerm::create(master, "P", odb::dbIoType::INOUT, sig_type);
      odb::dbMPin* pin = odb::dbMPin::create(term);
      odb::dbBox::create(pin, layer, 50, 100, 150, 200);
    } else {
      odb::dbBox::create(master, layer, 50, 100, 150, 200);
    }
    master->setFrozen();
    inst_ = makeInst(block_,
                     master,
                     "cell",
                     {.location = {kCellX, 0},
                      .status = odb::dbPlacementStatus::PLACED,
                      .iterms = {}});
    inst_->setLocationOrient(orient);
  }

  void addVia(const int offset_x = 100, const int offset_y = 150)
  {
    odb::dbSWire* wire = odb::dbSWire::create(supply_, odb::dbWireType::ROUTED);
    ASSERT_NE(odb::dbSBox::create(wire,
                                  via_,
                                  kCellX + offset_x,
                                  offset_y,
                                  odb::dbWireShapeType::STRIPE),
              nullptr);
  }

  void legalizePdn()
  {
    dp_.detailedPlacement(0,
                          0,
                          "",
                          /*incremental=*/false,
                          /*use_diamond_legalizer=*/true,
                          -1,
                          -1,
                          -1.0,
                          /*disable_window_extension=*/false,
                          /*quiet=*/false,
                          /*pdn_aware=*/true);
    EXPECT_NO_THROW(dp_.checkPlacement(false));
  }

  void expectOneSiteMove(const odb::dbInst* inst, const int initial_x)
  {
    const odb::Point location = inst->getLocation();
    EXPECT_EQ(std::abs(location.x() - initial_x), kSiteWidth);
    EXPECT_EQ(location.y(), 0);
  }

  odb::dbSite* site_ = nullptr;
  odb::dbTechLayer* routing_ = nullptr;
  odb::dbTechLayer* cut_ = nullptr;
  odb::dbTechVia* via_ = nullptr;
  odb::dbNet* supply_ = nullptr;
  odb::dbInst* inst_ = nullptr;
  Opendp dp_{db_.get(), &logger_};
};

TEST_F(FixedSupplyViaTest, ZeroSpacingCutOverlap)
{
  makeCell(cut_);
  addVia();
  legalizePdn();
  expectOneSiteMove(inst_, kCellX);
}

TEST_F(FixedSupplyViaTest, ZeroSpacingRoutingOverlap)
{
  makeCell(routing_);
  addVia();
  legalizePdn();
  expectOneSiteMove(inst_, kCellX);
}

TEST_F(FixedSupplyViaTest, ZeroSpacingDifferentNetPinOverlap)
{
  makeCell(routing_, /*terminal=*/true);
  odb::dbNet* other = odb::dbNet::create(block_, "OTHER");
  other->setSigType(odb::dbSigType::POWER);
  inst_->findITerm("P")->connect(other);
  addVia();
  logger_.redirectStringBegin();
  legalizePdn();
  const std::string log = logger_.redirectStringEnd();
  EXPECT_EQ(log.find("DPL-0502"), std::string::npos);
  expectOneSiteMove(inst_, kCellX);
}

TEST_F(FixedSupplyViaTest, ZeroSpacingSameNetPinOverlap)
{
  makeCell(routing_, /*terminal=*/true);
  inst_->findITerm("P")->connect(supply_);
  addVia();
  logger_.redirectStringBegin();
  legalizePdn();
  const std::string log = logger_.redirectStringEnd();
  EXPECT_EQ(log.find("DPL-0502"), std::string::npos);
  EXPECT_EQ(inst_->getLocation(), odb::Point(kCellX, 0));
}

TEST_F(FixedSupplyViaTest, UnconnectedSupplyPinWarnsOnce)
{
  makeCell(routing_, /*terminal=*/true);
  const int second_x = kCellX + 3 * kSiteWidth;
  odb::dbInst* second = makeInst(block_,
                                 inst_->getMaster(),
                                 "second_cell",
                                 {.location = {second_x, 0},
                                  .status = odb::dbPlacementStatus::PLACED,
                                  .iterms = {}});
  addVia();
  addVia(/*offset_x=*/3 * kSiteWidth + 100);
  logger_.redirectStringBegin();
  legalizePdn();
  const std::string log = logger_.redirectStringEnd();
  const size_t warning = log.find("DPL-0502");
  ASSERT_NE(warning, std::string::npos);
  EXPECT_EQ(log.find("DPL-0502", warning + 1), std::string::npos);
  EXPECT_NE(log.find("Unconnected supply pin"), std::string::npos);
  EXPECT_NE(log.find("/P prevents same-net PDN exemptions"), std::string::npos);
  expectOneSiteMove(inst_, kCellX);
  expectOneSiteMove(second, second_x);
  EXPECT_EQ(inst_->findITerm("P")->getNet(), nullptr);
  EXPECT_EQ(second->findITerm("P")->getNet(), nullptr);
}

TEST_F(FixedSupplyViaTest, UnconnectedSignalPinDoesNotWarn)
{
  makeCell(routing_, /*terminal=*/true, odb::dbSigType::SIGNAL);
  addVia();
  logger_.redirectStringBegin();
  legalizePdn();
  const std::string log = logger_.redirectStringEnd();
  EXPECT_EQ(log.find("DPL-0502"), std::string::npos);
  expectOneSiteMove(inst_, kCellX);
}

TEST_F(FixedSupplyViaTest, ZeroSpacingCutHorizontalBoundaryTouch)
{
  makeCell(cut_);
  addVia(/*offset_x=*/200);
  legalizePdn();
  EXPECT_EQ(inst_->getLocation(), odb::Point(kCellX, 0));
}

TEST_F(FixedSupplyViaTest, ZeroSpacingCutVerticalBoundaryTouch)
{
  makeCell(cut_);
  addVia(/*offset_x=*/100, /*offset_y=*/250);
  legalizePdn();
  EXPECT_EQ(inst_->getLocation(), odb::Point(kCellX, 0));
}

TEST_F(FixedSupplyViaTest, ZeroSpacingCutOneDbuOverlap)
{
  makeCell(cut_);
  addVia(/*offset_x=*/199);
  legalizePdn();
  expectOneSiteMove(inst_, kCellX);
}

TEST_F(FixedSupplyViaTest, ZeroSpacingRoutingHorizontalBoundaryTouch)
{
  makeCell(routing_);
  addVia(/*offset_x=*/200);
  legalizePdn();
  EXPECT_EQ(inst_->getLocation(), odb::Point(kCellX, 0));
}

TEST_F(FixedSupplyViaTest, ZeroSpacingRoutingVerticalBoundaryTouch)
{
  makeCell(routing_);
  addVia(/*offset_x=*/100, /*offset_y=*/250);
  legalizePdn();
  EXPECT_EQ(inst_->getLocation(), odb::Point(kCellX, 0));
}

TEST_F(FixedSupplyViaTest, RoutingVerticalGapBelowSpacing)
{
  routing_->setSpacing(130);
  makeCell(routing_);
  // Via yMin 329 leaves a 129 DBU gap above the OBS yMax 200.
  addVia(/*offset_x=*/100, /*offset_y=*/379);
  legalizePdn();
  expectOneSiteMove(inst_, kCellX);
}

TEST_F(FixedSupplyViaTest, RoutingVerticalGapAtSpacing)
{
  routing_->setSpacing(130);
  makeCell(routing_);
  // A 130 DBU gap is legal and permits the vertically safe shortcut.
  addVia(/*offset_x=*/100, /*offset_y=*/380);
  legalizePdn();
  EXPECT_EQ(inst_->getLocation(), odb::Point(kCellX, 0));
}

TEST_F(FixedSupplyViaTest, MxAsymmetricRoutingOverlap)
{
  makeCell(routing_,
           /*terminal=*/false,
           odb::dbSigType::POWER,
           odb::dbOrientType::MX);
  // MX maps the OBS y=100..200 to y=2600..2700.
  addVia(/*offset_x=*/100, /*offset_y=*/2650);
  legalizePdn();
  expectOneSiteMove(inst_, kCellX);
  EXPECT_EQ(inst_->getOrient(), odb::dbOrientType::MX);
}

TEST_F(FixedSupplyViaTest, MxAsymmetricRoutingUntransformedViaClear)
{
  makeCell(routing_,
           /*terminal=*/false,
           odb::dbSigType::POWER,
           odb::dbOrientType::MX);
  addVia(/*offset_x=*/100, /*offset_y=*/150);
  legalizePdn();
  EXPECT_EQ(inst_->getLocation(), odb::Point(kCellX, 0));
  EXPECT_EQ(inst_->getOrient(), odb::dbOrientType::MX);
}

TEST_F(FixedSupplyViaTest, RecreatedBlockStartsWithPdnDisabled)
{
  cut_->setSpacing(20);
  makeCell(cut_);
  addVia();
  legalizePdn();
  ASSERT_NE(inst_->getLocation(), odb::Point(kCellX, 0));

  odb::dbMaster* master = inst_->getMaster();
  odb::dbBlock* old_block = block_;
  odb::dbBlock::destroy(block_);
  block_ = odb::dbBlock::create(chip_, "replacement");
  // Exercise ODB slot reuse without an intervening placement command.
  ASSERT_EQ(block_, old_block);
  makeRowAndSupply();
  inst_ = makeInst(block_,
                   master,
                   "cell",
                   {.location = {kCellX, 0},
                    .status = odb::dbPlacementStatus::PLACED,
                    .iterms = {}});
  addVia();
  EXPECT_NO_THROW(dp_.checkPlacement(false));
}

TEST_F(FixedSupplyViaTest, PlacementWithoutPdnResetsMode)
{
  cut_->setSpacing(20);
  makeCell(cut_);
  addVia();
  legalizePdn();
  ASSERT_NE(inst_->getLocation(), odb::Point(kCellX, 0));

  inst_->setLocation(kCellX, 0);
  dp_.detailedPlacementQuiet();
  EXPECT_EQ(inst_->getLocation(), odb::Point(kCellX, 0));
  EXPECT_NO_THROW(dp_.checkPlacement(false));
}

TEST_F(FixedSupplyViaTest, QuietWrapperKeepsPdnDisabled)
{
  cut_->setSpacing(20);
  makeCell(cut_);
  addVia();
  logger_.redirectStringBegin();
  dp_.detailedPlacementQuiet();
  const std::string log = logger_.redirectStringEnd();
  EXPECT_NE(log.find("DPL-0501"), std::string::npos);
  EXPECT_EQ(log.find("Core area:"), std::string::npos);
  EXPECT_EQ(inst_->getLocation(), odb::Point(kCellX, 0));
  EXPECT_NO_THROW(dp_.checkPlacement(false));
}

class Lef58ImplantAndEdgeTest : public tst::Nangate45Fixture
{
 protected:
  static constexpr int kSiteWidth = 380;
  static constexpr int kRowHeight = 2800;
  static constexpr int kNumSites = 20;
  static constexpr int kNumRows = 4;

  void SetUp() override
  {
    tech_ = db_->getTech();
    site_ = lib_->findSite("FreePDK45_38x28_10R_NP_162NW_34O");
    ASSERT_NE(site_, nullptr);

    block_->setDefUnits(tech_->getLefUnits());
    block_->setDieArea(
        odb::Rect(0, 0, kNumSites * kSiteWidth, kNumRows * kRowHeight));
    for (int r = 0; r < kNumRows; ++r) {
      const auto orient
          = (r % 2 == 0) ? odb::dbOrientType::R0 : odb::dbOrientType::MX;
      odb::dbRow::create(block_,
                         ("ROW_" + std::to_string(r)).c_str(),
                         site_,
                         0,
                         r * kRowHeight,
                         orient,
                         odb::dbRowDir::HORIZONTAL,
                         kNumSites,
                         kSiteWidth);
    }
    block_->setCoreArea(block_->computeCoreArea());

    imp1_ = odb::dbTechLayer::create(
        tech_, "IMP1", odb::dbTechLayerType::IMPLANT);
    imp2_ = odb::dbTechLayer::create(
        tech_, "IMP2", odb::dbTechLayerType::IMPLANT);
    ms_od_ = odb::dbTechLayer::create(
        tech_, "OD_LAYER", odb::dbTechLayerType::MASTERSLICE);
  }

  odb::dbMaster* createCustomMaster(
      const char* name,
      const int sites_wide,
      const std::vector<std::pair<odb::dbTechLayer*, odb::Rect>>& obs_rects,
      const odb::dbMasterType type = odb::dbMasterType::CORE,
      const int rows_tall = 1)
  {
    odb::dbMaster* m = odb::dbMaster::create(lib_, name);
    m->setType(type);
    m->setSite(site_);
    m->setWidth(sites_wide * kSiteWidth);
    m->setHeight(rows_tall * kRowHeight);
    m->setSymmetryX();
    m->setSymmetryY();
    for (const auto& [layer, rect] : obs_rects) {
      odb::dbBox::create(
          m, layer, rect.xMin(), rect.yMin(), rect.xMax(), rect.yMax());
    }
    m->setFrozen();
    return m;
  }

  odb::dbInst* placeCell(odb::dbMaster* master,
                         const char* name,
                         const int gx,
                         const int gy,
                         const odb::dbOrientType orient = odb::dbOrientType::R0,
                         const odb::dbPlacementStatus status
                         = odb::dbPlacementStatus::PLACED)
  {
    odb::dbInst* inst = odb::dbInst::create(block_, master, name);
    inst->setOrient(orient);
    inst->setLocation(gx * kSiteWidth, gy * kRowHeight);
    inst->setPlacementStatus(status);
    return inst;
  }

  std::string runCheck()
  {
    try {
      dp_.checkPlacement(/*verbose=*/false);
    } catch (const std::runtime_error& error) {
      return error.what();
    }
    return "";
  }

  int failureCount(const char* category_name)
  {
    odb::dbMarkerCategory* dpl_cat = block_->findMarkerCategory("DPL");
    if (dpl_cat == nullptr) {
      return 0;
    }
    odb::dbMarkerCategory* sub = dpl_cat->findMarkerCategory(category_name);
    return sub == nullptr ? 0 : sub->getMarkerCount();
  }

  odb::dbTech* tech_{nullptr};
  odb::dbSite* site_{nullptr};
  odb::dbTechLayer* imp1_{nullptr};
  odb::dbTechLayer* imp2_{nullptr};
  odb::dbTechLayer* ms_od_{nullptr};
  Opendp dp_{db_.get(), &logger_};
};

TEST_F(Lef58ImplantAndEdgeTest, CellEdgeSpacingExceptAbuttedAndFacingDirs)
{
  auto* rule = odb::dbCellEdgeSpacing::create(tech_);
  rule->setFirstEdgeType("ETYPE_A");
  rule->setSecondEdgeType("ETYPE_A");
  rule->setSpacing(2 * kSiteWidth);
  rule->setExceptAbutted(true);

  odb::dbMaster* m = odb::dbMaster::create(lib_, "EDGE_CELL");
  m->setType(odb::dbMasterType::CORE);
  m->setSite(site_);
  m->setWidth(kSiteWidth);
  m->setHeight(kRowHeight);
  m->setSymmetryX();
  m->setSymmetryY();
  auto* e_right = odb::dbMasterEdgeType::create(m);
  e_right->setEdgeDir(odb::dbMasterEdgeType::RIGHT);
  e_right->setEdgeType("ETYPE_A");
  auto* e_left = odb::dbMasterEdgeType::create(m);
  e_left->setEdgeDir(odb::dbMasterEdgeType::LEFT);
  e_left->setEdgeType("ETYPE_A");
  m->setFrozen();

  // Abutted (dist == 0) is legal with EXCEPTABUTTED
  odb::dbInst* c1 = placeCell(m, "c1", 2, 0);
  odb::dbInst* c2 = placeCell(m, "c2", 3, 0);
  EXPECT_EQ(runCheck(), "");

  // 1-site gap (dist == kSiteWidth < 2 * kSiteWidth) violates rule
  c2->setLocation(4 * kSiteWidth, 0);
  EXPECT_EQ(runCheck(), "DPL-0033");
  EXPECT_EQ(failureCount("Cell_edge_spacing_failures"), 1);

  // 2-site gap (dist == 2 * kSiteWidth) is legal
  c2->setLocation(5 * kSiteWidth, 0);
  EXPECT_EQ(runCheck(), "");
  (void) c1;
}

TEST_F(Lef58ImplantAndEdgeTest, CellEdgeSpacingRangeAndCoreSpacer)
{
  auto* rule = odb::dbCellEdgeSpacing::create(tech_);
  rule->setFirstEdgeType("TOP_SEG");
  rule->setSecondEdgeType("BOT_SEG");
  rule->setSpacing(100);

  // Master with TOP edge restricted to x=[kSiteWidth, 2*kSiteWidth]
  odb::dbMaster* m_top = odb::dbMaster::create(lib_, "M_TOP_RANGE");
  m_top->setType(odb::dbMasterType::CORE);
  m_top->setSite(site_);
  m_top->setWidth(2 * kSiteWidth);
  m_top->setHeight(kRowHeight);
  auto* et = odb::dbMasterEdgeType::create(m_top);
  et->setEdgeDir(odb::dbMasterEdgeType::TOP);
  et->setEdgeType("TOP_SEG");
  et->setRangeBegin(kSiteWidth);
  et->setRangeEnd(2 * kSiteWidth);
  m_top->setFrozen();

  // CORE_SPACER master with explicit BOTTOM edge on x=[0, kSiteWidth]
  odb::dbMaster* m_fill = odb::dbMaster::create(lib_, "FILL_BOT_EDGE");
  m_fill->setType(odb::dbMasterType::CORE_SPACER);
  m_fill->setSite(site_);
  m_fill->setWidth(kSiteWidth);
  m_fill->setHeight(kRowHeight);
  auto* eb = odb::dbMasterEdgeType::create(m_fill);
  eb->setEdgeDir(odb::dbMasterEdgeType::BOTTOM);
  eb->setEdgeType("BOT_SEG");
  m_fill->setFrozen();

  // Place m_top at gx=2..4 on row 0 (TOP_SEG is at x=[3*kSiteWidth,
  // 4*kSiteWidth]). Place m_fill at gx=2..3 on row 1 (BOT_SEG is at
  // x=[2*kSiteWidth, 3*kSiteWidth]). Because range_begin=kSiteWidth, they only
  // touch at a point (no PRL overlap).
  placeCell(m_top, "u_top", 2, 0, odb::dbOrientType::R0);
  odb::dbInst* u_fill
      = placeCell(m_fill, "u_fill", 2, 1, odb::dbOrientType::R0);
  EXPECT_EQ(runCheck(), "");

  // Move u_fill to gx=3..4 on row 1 so BOT_SEG directly faces TOP_SEG (dist=0 <
  // 100).
  u_fill->setLocation(3 * kSiteWidth, 1 * kRowHeight);
  EXPECT_EQ(runCheck(), "DPL-0033");
  EXPECT_EQ(failureCount("Cell_edge_spacing_failures"), 1);
}

TEST_F(Lef58ImplantAndEdgeTest, ImplantBaseWidthPreFillerVsPostFiller)
{
  imp1_->setMinWidth(2 * kSiteWidth);

  odb::dbMaster* m_imp1 = createCustomMaster(
      "CELL_IMP1", 1, {{imp1_, odb::Rect(0, 0, kSiteWidth, kRowHeight)}});
  odb::dbMaster* m_imp2 = createCustomMaster(
      "CELL_IMP2", 1, {{imp2_, odb::Rect(0, 0, kSiteWidth, kRowHeight)}});

  // Place a 1-site IMP1 cell with empty sites next to it:
  // strict checkPlacement flags the raw 1-site IMP1 island (< 2 sites),
  // whereas detailedPlacement allows it while adjacent sites are empty.
  odb::dbInst* c1 = placeCell(m_imp1, "c1", 4, 0);
  EXPECT_EQ(runCheck(), "DPL-0033");
  EXPECT_EQ(failureCount("Implant_DRC_failures"), 1);

  dp_.detailedPlacement(0, 0);
  EXPECT_EQ(c1->getLocation(), odb::Point(4 * kSiteWidth, 0));

  // Placing a second 1-site IMP1 cell adjacent at gx=5 merges IMP1 to 2 sites,
  // satisfying checkPlacement.
  odb::dbInst* c2 = placeCell(m_imp1, "c2", 5, 0);
  EXPECT_EQ(runCheck(), "");

  // If c1 is pinched between two fixed IMP2 cells at gx=3 and gx=5 (and c2 is
  // moved away), detailedPlacement must move c1 out of the pinched slot.
  c2->setLocation(10 * kSiteWidth, 0);
  placeCell(m_imp1, "c3", 11, 0);
  placeCell(m_imp2,
            "fix_l",
            3,
            0,
            odb::dbOrientType::R0,
            odb::dbPlacementStatus::FIRM);
  placeCell(m_imp2,
            "fix_r",
            5,
            0,
            odb::dbOrientType::R0,
            odb::dbPlacementStatus::FIRM);
  dp_.detailedPlacement(0, 0);
  EXPECT_NE(c1->getLocation(), odb::Point(4 * kSiteWidth, 0));
}

TEST_F(Lef58ImplantAndEdgeTest, ImplantBaseSpacingSameAndInterLayer)
{
  imp1_->setSpacing(2 * kSiteWidth);
  auto* inter_rule = odb::dbTechLayerSpacingRule::create(imp2_);
  inter_rule->setSpacing(kSiteWidth);
  inter_rule->setCutLayer4Spacing(imp1_);

  odb::dbMaster* m1 = createCustomMaster(
      "M_IMP1", 1, {{imp1_, odb::Rect(0, 0, kSiteWidth, kRowHeight)}});
  odb::dbMaster* m2 = createCustomMaster(
      "M_IMP2", 1, {{imp2_, odb::Rect(0, 0, kSiteWidth, kRowHeight)}});

  // Same-layer spacing violation (1-site gap < 2 sites)
  placeCell(m1, "a1", 2, 0);
  odb::dbInst* a2 = placeCell(m1, "a2", 4, 0);
  EXPECT_EQ(runCheck(), "DPL-0033");
  EXPECT_EQ(failureCount("Implant_DRC_failures"), 2);

  // Move a2 to 2-site gap (gx=5): legal
  a2->setLocation(5 * kSiteWidth, 0);
  EXPECT_EQ(runCheck(), "");

  // Inter-layer abutment (dist=0 < kSiteWidth) with IMP2 violates V54 rule
  odb::dbInst* b1 = placeCell(m2, "b1", 6, 0);
  EXPECT_EQ(runCheck(), "DPL-0033");

  // Move b1 to 1-site gap (gx=7): legal
  b1->setLocation(7 * kSiteWidth, 0);
  EXPECT_EQ(runCheck(), "");

  // GAA half-row dual-implant flipped rows (R0 / MX) where row_height <
  // imp1_->getSpacing() (like N2P 0.130 um < 0.144 um): vertical separation
  // across the opposite-polarity stripe is legal; horizontal same-row gap
  // < min_spc still fails.
  odb::dbInst::destroy(block_->findInst("a1"));
  odb::dbInst::destroy(a2);
  odb::dbInst::destroy(b1);
  inter_rule->setSpacing(0);
  imp1_->setSpacing(kRowHeight + kSiteWidth);
  odb::dbMaster* m_dual = createCustomMaster(
      "M_GAA_DUAL",
      1,
      {{imp1_, odb::Rect(0, 0, kSiteWidth, kRowHeight / 2)},
       {imp2_, odb::Rect(0, kRowHeight / 2, kSiteWidth, kRowHeight)}});
  placeCell(m_dual, "d_r0", 2, 0, odb::dbOrientType::R0);
  placeCell(m_dual, "d_r1", 2, 1, odb::dbOrientType::MX);
  EXPECT_EQ(runCheck(), "");

  placeCell(m_dual, "d_r0_gap", 4, 0, odb::dbOrientType::R0);
  EXPECT_EQ(runCheck(), "DPL-0033");
}

TEST_F(Lef58ImplantAndEdgeTest,
       ImplantNonImplantSealingPmosSnapMultiSiteAndAttribution)
{
  imp1_->setWidth(4 * kSiteWidth);
  imp1_->setSpacing(2 * kSiteWidth);

  odb::dbMaster* m_pmos = createCustomMaster(
      "M_PMOS_TOP",
      1,
      {{imp1_, odb::Rect(0, kRowHeight / 2, kSiteWidth, kRowHeight)}});
  odb::dbMaster* m_imp1_2h = createCustomMaster(
      "M_IMP1_2H",
      3,
      {{imp1_, odb::Rect(0, 0, 3 * kSiteWidth, 2 * kRowHeight)}},
      odb::dbMasterType::CORE,
      /*rows_tall=*/2);
  odb::dbMaster* m_no_imp = createCustomMaster("M_NO_IMP", 1, {});
  odb::dbMaster* m_imp2 = createCustomMaster(
      "M_IMP2_BLK", 1, {{imp2_, odb::Rect(0, 0, kSiteWidth, kRowHeight)}});

  odb::dbMaster* m_imp1_1h = createCustomMaster(
      "M_IMP1_1H", 1, {{imp1_, odb::Rect(0, 0, kSiteWidth, kRowHeight)}});

  // 1H cells on rows 0 and 1 abutting a 3-site 2H cell merge across each
  // overlapping Y-band to 4 sites wide (>= 4 * kSiteWidth).
  placeCell(m_imp1_1h,
            "p_1h",
            0,
            0,
            odb::dbOrientType::R0,
            odb::dbPlacementStatus::FIRM);
  placeCell(m_imp1_2h,
            "p_2h",
            1,
            0,
            odb::dbOrientType::R0,
            odb::dbPlacementStatus::FIRM);
  placeCell(m_imp1_1h,
            "p_r1_full",
            0,
            1,
            odb::dbOrientType::R0,
            odb::dbPlacementStatus::FIRM);
  EXPECT_EQ(runCheck(), "");

  // Interior half-row implant (dy = kRowHeight / 2 = 1400 < min_w = 1520) that
  // satisfies horizontal width (4 * kSiteWidth = 1520) and is bounded above and
  // below by IMP2 rows must pass checkPlacement (base IMPLANT WIDTH measures
  // horizontal run along the row, matching GAA half-row cells like N2P/A14).
  for (int gx = 12; gx < 16; ++gx) {
    placeCell(m_imp2,
              ("r0_b_" + std::to_string(gx)).c_str(),
              gx,
              0,
              odb::dbOrientType::R0,
              odb::dbPlacementStatus::FIRM);
    placeCell(m_pmos,
              ("r1_p_" + std::to_string(gx)).c_str(),
              gx,
              1,
              odb::dbOrientType::MX,
              odb::dbPlacementStatus::FIRM);
    placeCell(m_imp2,
              ("r2_t_" + std::to_string(gx)).c_str(),
              gx,
              2,
              odb::dbOrientType::R0,
              odb::dbPlacementStatus::FIRM);
  }
  EXPECT_EQ(runCheck(), "");

  // Multi-site extension check: a 1-site cell at gx=7 bounded by FIRM IMP2
  // cells at gx=5 and gx=9 only has 3 total sites available (< 4 * kSiteWidth),
  // even when row 1 above is completely empty (verifies gridSnapDownY checks
  // row 0 rather than row 1).
  placeCell(m_imp2,
            "blk_l",
            5,
            0,
            odb::dbOrientType::R0,
            odb::dbPlacementStatus::FIRM);
  placeCell(m_imp2,
            "blk_r",
            9,
            0,
            odb::dbOrientType::R0,
            odb::dbPlacementStatus::FIRM);
  odb::dbInst* pinched = placeCell(m_pmos, "pinched", 7, 0);
  dp_.detailedPlacement(0, 0);
  EXPECT_NE(pinched->getLocation(), odb::Point(7 * kSiteWidth, 0));
  odb::dbInst::destroy(pinched);
  odb::dbInst::destroy(block_->findInst("blk_l"));
  odb::dbInst::destroy(block_->findInst("blk_r"));

  // Remote sealer check (1 empty site away): a 2-site IMP1 island at gx=6..7
  // bounded on the left (gx=5) by FIRM IMP2 needs 2 empty sites (gx=8..9) to
  // reach 4 * kSiteWidth; placing M_NO_IMP at gx=9 leaves only 3 total sites
  // (gx=6..8), so DPL must move M_NO_IMP away from gx=9.
  placeCell(m_imp2,
            "rem_l",
            5,
            0,
            odb::dbOrientType::R0,
            odb::dbPlacementStatus::FIRM);
  placeCell(m_imp1_1h,
            "rem_6",
            6,
            0,
            odb::dbOrientType::R0,
            odb::dbPlacementStatus::FIRM);
  placeCell(m_imp1_1h,
            "rem_7",
            7,
            0,
            odb::dbOrientType::R0,
            odb::dbPlacementStatus::FIRM);
  odb::dbInst* rem_sealer = placeCell(m_no_imp, "rem_sealer", 9, 0);
  dp_.detailedPlacement(0, 0);
  EXPECT_NE(rem_sealer->getLocation(), odb::Point(9 * kSiteWidth, 0));
  for (const char* nm : {"rem_l", "rem_6", "rem_7", "rem_sealer"}) {
    odb::dbInst::destroy(block_->findInst(nm));
  }

  // Mandatory min-width contention across a shared 1-site gap: with both IMP1
  // and IMP2 requiring >= 3 * kSiteWidth, a left-locked 2-site IMP1 island at
  // gx=5..6 and a right-locked 2-site IMP2 island at gx=8..9 would both have to
  // claim the single empty site gx=7, which no filler can satisfy.
  imp1_->setWidth(3 * kSiteWidth);
  imp2_->setWidth(3 * kSiteWidth);
  placeCell(m_no_imp,
            "cnt_l",
            4,
            0,
            odb::dbOrientType::R0,
            odb::dbPlacementStatus::FIRM);
  placeCell(m_imp1_1h,
            "cnt_i5",
            5,
            0,
            odb::dbOrientType::R0,
            odb::dbPlacementStatus::FIRM);
  placeCell(m_imp1_1h,
            "cnt_i6",
            6,
            0,
            odb::dbOrientType::R0,
            odb::dbPlacementStatus::FIRM);
  odb::dbInst* cnt_m8 = placeCell(m_imp2, "cnt_m8", 8, 0);
  placeCell(m_imp2,
            "cnt_m9",
            9,
            0,
            odb::dbOrientType::R0,
            odb::dbPlacementStatus::FIRM);
  placeCell(m_no_imp,
            "cnt_r",
            10,
            0,
            odb::dbOrientType::R0,
            odb::dbPlacementStatus::FIRM);
  dp_.detailedPlacement(0, 0);
  EXPECT_NE(cnt_m8->getLocation(), odb::Point(8 * kSiteWidth, 0));
  for (const char* nm :
       {"cnt_l", "cnt_i5", "cnt_i6", "cnt_m8", "cnt_m9", "cnt_r"}) {
    odb::dbInst::destroy(block_->findInst(nm));
  }
  imp1_->setWidth(4 * kSiteWidth);
  imp2_->setWidth(0);

  // Non-implant cell sealing check: place a 3-site IMP1 island at gx=6..8
  // bounded on the left (gx=5) by FIRM IMP2; placing a movable M_NO_IMP cell at
  // gx=9 would seal the 3-site IMP1 island (< 4 * kSiteWidth), so DPL must move
  // M_NO_IMP away from gx=9!
  placeCell(m_imp2,
            "seal_l",
            5,
            0,
            odb::dbOrientType::R0,
            odb::dbPlacementStatus::FIRM);
  placeCell(m_imp1_1h,
            "i6",
            6,
            0,
            odb::dbOrientType::R0,
            odb::dbPlacementStatus::FIRM);
  placeCell(m_imp1_1h,
            "i7",
            7,
            0,
            odb::dbOrientType::R0,
            odb::dbPlacementStatus::FIRM);
  placeCell(m_imp1_1h,
            "i8",
            8,
            0,
            odb::dbOrientType::R0,
            odb::dbPlacementStatus::FIRM);
  odb::dbInst* sealer = placeCell(m_no_imp, "sealer", 9, 0);
  dp_.detailedPlacement(0, 0);
  EXPECT_NE(sealer->getLocation(), odb::Point(9 * kSiteWidth, 0));
}

TEST_F(Lef58ImplantAndEdgeTest, Lef58ImplantWidthZeroPrlAndLayerMaxWidth)
{
  auto* r_zprl = odb::dbTechLayerImplantWidthRule::create(imp1_);
  r_zprl->setMinWidth(2 * kSiteWidth);
  r_zprl->setZeroPrl(true);

  auto* r_adj = odb::dbTechLayerImplantWidthRule::create(imp1_);
  r_adj->setMinWidth(2 * kSiteWidth);
  r_adj->setLayerAny(true);
  r_adj->setMaxWidth(kSiteWidth);
  r_adj->setExceptCornerTouch(true);

  // Group-scoped rules should be skipped without false positives
  auto* r_grp = odb::dbTechLayerImplantWidthRule::create(imp1_);
  r_grp->setMinWidth(10 * kSiteWidth);
  r_grp->setCheckImplantGroup("IG_SPECIAL");

  odb::dbTechLayer* imp3
      = odb::dbTechLayer::create(tech_, "IMP3", odb::dbTechLayerType::IMPLANT);

  odb::dbMaster* m1_1 = createCustomMaster(
      "M1_W1", 1, {{imp1_, odb::Rect(0, 0, kSiteWidth, kRowHeight)}});
  odb::dbMaster* m2_1 = createCustomMaster(
      "M2_W1", 1, {{imp2_, odb::Rect(0, 0, kSiteWidth, kRowHeight)}});
  odb::dbMaster* m3_1 = createCustomMaster(
      "M3_W1", 1, {{imp3, odb::Rect(0, 0, kSiteWidth, kRowHeight)}});

  // Two 1-site IMP1 cells touching only at a diagonal corner violate ZEROPRL
  odb::dbInst* u1 = placeCell(m1_1, "u1", 2, 0);
  odb::dbInst* u2 = placeCell(m1_1, "u2", 3, 1, odb::dbOrientType::MX);
  EXPECT_EQ(runCheck(), "DPL-0033");

  // Extending both rows to 2 sites wide satisfies ZEROPRL
  odb::dbInst* u1_ext = placeCell(m1_1, "u1_ext", 1, 0);
  odb::dbInst* u2_ext = placeCell(m1_1, "u2_ext", 4, 1, odb::dbOrientType::MX);
  EXPECT_EQ(runCheck(), "");

  // Remove u1_ext and u2, u2_ext; place 1-site IMP2 (gx=3) and 1-site IMP3
  // (gx=4) beside 1-site IMP1 (gx=2): LAYER ANY merges per layer so IMP2 stays
  // width kSiteWidth <= maxWidth and triggers the violation!
  odb::dbInst::destroy(u1_ext);
  odb::dbInst::destroy(u2);
  odb::dbInst::destroy(u2_ext);
  odb::dbInst* v2 = placeCell(m2_1, "v2", 3, 0);
  odb::dbInst* v3 = placeCell(m3_1, "v3", 4, 0);
  EXPECT_EQ(runCheck(), "DPL-0033");
  odb::dbInst::destroy(v3);

  // Moving v2 to diagonal corner (gx=3, row=1) passes because of
  // EXCEPTCORNERTOUCH
  v2->setOrient(odb::dbOrientType::MX);
  v2->setLocation(3 * kSiteWidth, 1 * kRowHeight);
  EXPECT_EQ(runCheck(), "");

  // N2P-style ZEROPRL + EXCEPTCORNERTOUCH + MAXWIDTH + LENGTH (same-layer &
  // LAYER ANY): must not fall through to Case 3 on wide L-shaped 1-site
  // overhangs (> MAXWIDTH), while still catching a <= MAXWIDTH 1-site neck!
  odb::dbInst::destroy(u1);
  odb::dbInst::destroy(v2);
  odb::dbTechLayerImplantWidthRule::destroy(r_zprl);
  odb::dbTechLayerImplantWidthRule::destroy(r_adj);

  for (const bool any_layer : {false, true}) {
    auto* r_n2p = odb::dbTechLayerImplantWidthRule::create(imp1_);
    r_n2p->setMinWidth(2 * kSiteWidth);
    r_n2p->setZeroPrl(true);
    r_n2p->setMaxWidth(3 * kSiteWidth);
    r_n2p->setExceptCornerTouch(true);
    r_n2p->setLength(kRowHeight + 200);
    r_n2p->setLayerAny(any_layer);
  }

  // Wide (> MAXWIDTH = 3 sites) 4-site rows offset by 1 site (3-site overlap,
  // 1-site overhang on each side) must pass checkPlacement!
  for (int gx = 2; gx < 6; ++gx) {
    placeCell(m1_1,
              ("w_r0_" + std::to_string(gx)).c_str(),
              gx,
              0,
              odb::dbOrientType::R0);
    placeCell(m1_1,
              ("w_r1_" + std::to_string(gx)).c_str(),
              gx + 1,
              1,
              odb::dbOrientType::MX);
  }
  EXPECT_EQ(runCheck(), "");

  // Narrow (2-site <= MAXWIDTH) IMP1 on row 0 overlapping by 1 site (< min_w)
  // with 2-site IMP2 on row 1 violates LAYER ANY ZEROPRL neck!
  odb::dbInst* n0 = placeCell(m1_1, "n_r0_0", 10, 0, odb::dbOrientType::R0);
  odb::dbInst* n1 = placeCell(m1_1, "n_r0_1", 11, 0, odb::dbOrientType::R0);
  odb::dbInst* n2 = placeCell(m2_1, "n_r1_0", 11, 1, odb::dbOrientType::MX);
  odb::dbInst* n3 = placeCell(m2_1, "n_r1_1", 12, 1, odb::dbOrientType::MX);
  EXPECT_EQ(runCheck(), "DPL-0033");
  for (odb::dbInst* inst : {n0, n1, n2, n3}) {
    odb::dbInst::destroy(inst);
  }

  // Pre-filler empty-site directionality (with IMP1 filler master and base
  // WIDTH = 3 sites): Row 0 has a locked 3-site IMP1 island [9..12]
  // (<= MAXWIDTH) between FIRM IMP3 blockers at gx=8 and gx=12; Row 1 has a
  // 2-site IMP1 island [11..13] bounded on its outer right (gx=13) by FIRM
  // IMP3, forcing mandatory 3-site expansion inward across empty gx=10 to
  // [10..13] (2-site overlap [10..12] >= min_w -> legal!).
  odb::dbMaster* m_elvt = createCustomMaster("M_ELVT_NO_IMP", 1, {});
  odb::dbMaster* m1_3 = createCustomMaster(
      "M1_W3", 3, {{imp1_, odb::Rect(0, 0, 3 * kSiteWidth, kRowHeight)}});
  imp1_->setWidth(3 * kSiteWidth);
  createCustomMaster("FILL_IMP1",
                     1,
                     {{imp1_, odb::Rect(0, 0, kSiteWidth, kRowHeight)}},
                     odb::dbMasterType::CORE_SPACER);
  placeCell(m_elvt,
            "blk_l",
            8,
            0,
            odb::dbOrientType::R0,
            odb::dbPlacementStatus::FIRM);
  placeCell(
      m1_1, "lk_0", 9, 0, odb::dbOrientType::R0, odb::dbPlacementStatus::FIRM);
  placeCell(
      m1_1, "lk_1", 10, 0, odb::dbOrientType::R0, odb::dbPlacementStatus::FIRM);
  placeCell(
      m1_1, "lk_2", 11, 0, odb::dbOrientType::R0, odb::dbPlacementStatus::FIRM);
  placeCell(m_elvt,
            "blk_r",
            12,
            0,
            odb::dbOrientType::R0,
            odb::dbPlacementStatus::FIRM);
  placeCell(m1_1,
            "r1_in",
            11,
            1,
            odb::dbOrientType::MX,
            odb::dbPlacementStatus::FIRM);
  placeCell(m1_1,
            "r1_out",
            12,
            1,
            odb::dbOrientType::MX,
            odb::dbPlacementStatus::FIRM);
  odb::dbInst* r1_outer_blk = placeCell(m_elvt,
                                        "r1_outer_blk",
                                        13,
                                        1,
                                        odb::dbOrientType::MX,
                                        odb::dbPlacementStatus::FIRM);
  EXPECT_EQ(runCheck(), "");

  // Removing the outer blocker at gx=13 leaves [11..13] unforced on gx=10,
  // exposing a 1-site [11..12] ZEROPRL throat vulnerable to wide non-IMP1
  // filler packing -> must fail!
  odb::dbInst::destroy(r1_outer_blk);
  EXPECT_EQ(runCheck(), "DPL-0033");
  for (const char* nm :
       {"blk_l", "lk_0", "lk_1", "lk_2", "blk_r", "r1_in", "r1_out"}) {
    odb::dbInst::destroy(block_->findInst(nm));
  }

  // Unprotected 1-site ZEROPRL throat where BOTH rows are 3 sites wide
  // (<= MAXWIDTH) and outer sites are empty: must fail checkPlacement and
  // move the movable 3-site cell to a clean alignment.
  placeCell(m1_3,
            "ot_r0",
            10,
            0,
            odb::dbOrientType::R0,
            odb::dbPlacementStatus::FIRM);
  odb::dbInst* ot_r1 = placeCell(m1_3, "ot_r1", 12, 1, odb::dbOrientType::MX);
  EXPECT_EQ(runCheck(), "DPL-0033");
  dp_.detailedPlacement(0, 0);
  EXPECT_NE(ot_r1->getLocation(), odb::Point(12 * kSiteWidth, 1 * kRowHeight));
  EXPECT_EQ(runCheck(), "");
  odb::dbInst::destroy(block_->findInst("ot_r0"));
  odb::dbInst::destroy(ot_r1);

  // Minimal base-WIDTH completion ZEROPRL check: with base WIDTH = 3 sites, a
  // 2-site IMP1 cell on row 1 (gx=10..11) facing a 3-site IMP1 island on row 0
  // (gx=12..14):
  //  - Right side blocked at gx=12 and left side open -> only minimal 3-site
  //    completion is [9..12] (ovl_x == 0, EXCEPTCORNERTOUCH) -> legal!
  //  - Both sides open (e.g. gx<10 empty so wide non-IMP1 fillers can seal
  //    gx=10, and gx=12 empty) -> right minimal completion [10..13] creates a
  //    1-site overlap at gx=12 -> rejected, and DPL moves the 2-site cell!
  odb::dbMaster* m1_2 = createCustomMaster(
      "M1_W2", 2, {{imp1_, odb::Rect(0, 0, 2 * kSiteWidth, kRowHeight)}});
  placeCell(m_elvt,
            "fe_r0_l",
            11,
            0,
            odb::dbOrientType::R0,
            odb::dbPlacementStatus::FIRM);
  placeCell(m1_1,
            "fe_r0_0",
            12,
            0,
            odb::dbOrientType::R0,
            odb::dbPlacementStatus::FIRM);
  placeCell(m1_1,
            "fe_r0_1",
            13,
            0,
            odb::dbOrientType::R0,
            odb::dbPlacementStatus::FIRM);
  placeCell(m1_1,
            "fe_r0_2",
            14,
            0,
            odb::dbOrientType::R0,
            odb::dbPlacementStatus::FIRM);
  placeCell(m_elvt,
            "fe_r0_r",
            15,
            0,
            odb::dbOrientType::R0,
            odb::dbPlacementStatus::FIRM);
  odb::dbInst* fe_r1 = placeCell(m1_2, "fe_r1", 10, 1, odb::dbOrientType::MX);
  odb::dbInst* fe_r1_r = placeCell(m_elvt,
                                   "fe_r1_r",
                                   12,
                                   1,
                                   odb::dbOrientType::MX,
                                   odb::dbPlacementStatus::FIRM);
  EXPECT_EQ(runCheck(), "");
  odb::dbInst::destroy(fe_r1_r);
  EXPECT_EQ(runCheck(), "DPL-0033");
  dp_.detailedPlacement(0, 0);
  EXPECT_NE(fe_r1->getLocation(), odb::Point(10 * kSiteWidth, 1 * kRowHeight));
  EXPECT_EQ(runCheck(), "");
  imp1_->setWidth(0);
}

TEST_F(Lef58ImplantAndEdgeTest, Lef58ImplantWidthLengthWithinLayerAndCellHeight)
{
  auto* rule = odb::dbTechLayerImplantWidthRule::create(imp1_);
  rule->setMinWidth(kRowHeight);
  rule->setLength(2 * kSiteWidth);
  rule->addWithinLayer(ms_od_);
  rule->setCellHeight(kRowHeight);

  odb::dbMaster* m_with_od
      = createCustomMaster("M_WITH_OD",
                           1,
                           {{imp1_, odb::Rect(0, 0, kSiteWidth, kRowHeight)},
                            {ms_od_, odb::Rect(0, 0, kSiteWidth, kRowHeight)}});
  odb::dbMaster* m_no_od = createCustomMaster(
      "M_NO_OD", 1, {{imp1_, odb::Rect(0, 0, kSiteWidth, kRowHeight)}});

  // Without OD_LAYER (WITHINLAYER), 1-site IMP1 is not checked by this rule
  odb::dbInst* c_no_od = placeCell(m_no_od, "c_no_od", 1, 0);
  EXPECT_EQ(runCheck(), "");
  odb::dbInst::destroy(c_no_od);

  // With OD_LAYER, a single 1-site cell has height == kRowHeight and length ==
  // kSiteWidth < 2 * kSiteWidth -> violation
  placeCell(m_with_od, "c1", 4, 0);
  EXPECT_EQ(runCheck(), "DPL-0033");

  // L-shaped component: wide 3-site bar on row 1 attached to 1-site stub c1 on
  // row 0 STILL violates LENGTH on the 1-site stub (r.dx() < min_len)!
  placeCell(m_with_od, "r1_a", 4, 1, odb::dbOrientType::MX);
  placeCell(m_with_od, "r1_b", 5, 1, odb::dbOrientType::MX);
  placeCell(m_with_od, "r1_c", 6, 1, odb::dbOrientType::MX);
  EXPECT_EQ(runCheck(), "DPL-0033");

  // Adding a second adjacent cell on row 0 extends stub length to 2 *
  // kSiteWidth -> legal
  placeCell(m_with_od, "c2", 5, 0);
  EXPECT_EQ(runCheck(), "");
}
TEST_F(Lef58ImplantAndEdgeTest, Lef58ImplantWidthLayerLengthSelectsShortShapes)
{
  // WIDTH 2 sites LAYER IMP2 LENGTH 2 rows: LENGTH keeps its ZEROPRL meaning
  // (the vertical height of the implant shape the rule applies to), so only
  // IMP1 shapes shorter than 2 rows must be >= 2 sites wide where they abut
  // IMP2.
  auto* rule = odb::dbTechLayerImplantWidthRule::create(imp1_);
  rule->setMinWidth(2 * kSiteWidth);
  rule->setSecondLayer(imp2_);
  rule->setLength(2 * kRowHeight);

  odb::dbMaster* m1 = createCustomMaster(
      "M1_1H", 1, {{imp1_, odb::Rect(0, 0, kSiteWidth, kRowHeight)}});
  odb::dbMaster* m1_2h = createCustomMaster(
      "M1_2H",
      1,
      {{imp1_, odb::Rect(0, 0, kSiteWidth, 2 * kRowHeight)}},
      odb::dbMasterType::CORE,
      /*rows_tall=*/2);
  odb::dbMaster* m2 = createCustomMaster(
      "M2_1H", 1, {{imp2_, odb::Rect(0, 0, kSiteWidth, kRowHeight)}});

  // 1-row IMP1 (height kRowHeight < LENGTH) 1 site wide abutting IMP2 ->
  // violation
  odb::dbInst* a = placeCell(m1, "a", 2, 0);
  placeCell(m2, "b", 3, 0);
  EXPECT_EQ(runCheck(), "DPL-0033");

  // 2-row IMP1 (height 2 * kRowHeight >= LENGTH) is outside the rule's
  // selection, so its 1-site width next to IMP2 is legal
  odb::dbInst::destroy(a);
  placeCell(m1_2h, "t", 2, 0);
  EXPECT_EQ(runCheck(), "");
}
TEST_F(Lef58ImplantAndEdgeTest, Lef58ImplantSpacingIntersectLayersAnyOf)
{
  // INTERSECTLAYERS OD_LAYER IMP3: the spacing applies when the gap is
  // crossed by a shape on ANY one of the listed layers; all of them need not
  // be present.
  odb::dbTechLayer* imp3
      = odb::dbTechLayer::create(tech_, "IMP3", odb::dbTechLayerType::IMPLANT);
  auto* rule = odb::dbTechLayerImplantSpacingRule::create(imp1_);
  rule->setMinSpacing(2 * kSiteWidth);
  rule->setSecondLayer(imp2_);
  rule->setHorizontal(true);
  rule->addIntersectLayer(ms_od_);
  rule->addIntersectLayer(imp3);

  odb::dbMaster* m1 = createCustomMaster(
      "M1", 1, {{imp1_, odb::Rect(0, 0, kSiteWidth, kRowHeight)}});
  odb::dbMaster* m2 = createCustomMaster(
      "M2", 1, {{imp2_, odb::Rect(0, 0, kSiteWidth, kRowHeight)}});
  odb::dbMaster* m_od = createCustomMaster(
      "M_OD", 1, {{ms_od_, odb::Rect(0, 0, kSiteWidth, kRowHeight)}});
  odb::dbMaster* m_i3 = createCustomMaster(
      "M_I3", 1, {{imp3, odb::Rect(0, 0, kSiteWidth, kRowHeight)}});

  // Empty 1-site gap: no listed layer crosses it -> rule does not apply
  placeCell(m1, "i1", 2, 0);
  placeCell(m2, "i2", 4, 0);
  EXPECT_EQ(runCheck(), "");

  // OD alone in the gap is enough -> violation
  odb::dbInst* od = placeCell(m_od, "od", 3, 0);
  EXPECT_EQ(runCheck(), "DPL-0033");

  // IMP3 alone in the gap is also enough -> violation
  odb::dbInst::destroy(od);
  placeCell(m_i3, "i3", 3, 0);
  EXPECT_EQ(runCheck(), "DPL-0033");
}
TEST_F(Lef58ImplantAndEdgeTest,
       Lef58ImplantSpacingHorizontalVerticalPrlAndIntersect)
{
  auto* r_h = odb::dbTechLayerImplantSpacingRule::create(imp1_);
  r_h->setMinSpacing(2 * kSiteWidth);
  r_h->setSecondLayer(imp2_);
  r_h->setHorizontal(true);
  r_h->setExceptAbutted(true);
  r_h->setLength(2 * kSiteWidth);
  r_h->addIntersectLayer(ms_od_);

  odb::dbMaster* m1
      = createCustomMaster("M1_OD",
                           1,
                           {{imp1_, odb::Rect(0, 0, kSiteWidth, kRowHeight)},
                            {ms_od_, odb::Rect(0, 0, kSiteWidth, kRowHeight)}});
  odb::dbMaster* m2
      = createCustomMaster("M2_OD",
                           1,
                           {{imp2_, odb::Rect(0, 0, kSiteWidth, kRowHeight)},
                            {ms_od_, odb::Rect(0, 0, kSiteWidth, kRowHeight)}});

  // Abutted (dx_gap == 0) is legal due to EXCEPTABUTTED
  odb::dbInst* i1 = placeCell(m1, "i1", 2, 0);
  odb::dbInst* i2 = placeCell(m2, "i2", 3, 0);
  EXPECT_EQ(runCheck(), "");

  // Vertical stack (IMP2 directly above IMP1 on row 1) has prl_x > 0, so it
  // must NOT falsely trigger a HORIZONTAL rule even without EXCEPTABUTTED!
  i2->setOrient(odb::dbOrientType::MX);
  i2->setLocation(2 * kSiteWidth, 1 * kRowHeight);
  EXPECT_EQ(runCheck(), "");

  // 1-site horizontal gap WITHOUT OD bridging the gap is legal (negative
  // INTERSECTLAYERS check - i1/i2's own OD must not match the gap!)
  i2->setOrient(odb::dbOrientType::R0);
  i2->setLocation(4 * kSiteWidth, 0);
  EXPECT_EQ(runCheck(), "");

  // Adding OD bridging the 1-site gap triggers HORIZONTAL spacing violation
  odb::dbMaster* m_od_bridge = createCustomMaster(
      "M_OD_BRIDGE", 1, {{ms_od_, odb::Rect(0, 0, kSiteWidth, kRowHeight)}});
  odb::dbInst* bridge = placeCell(m_od_bridge, "bridge", 3, 0);
  EXPECT_EQ(runCheck(), "DPL-0033");

  // 2-site gap (dx_gap == 2 * kSiteWidth) is legal
  i2->setLocation(5 * kSiteWidth, 0);
  EXPECT_EQ(runCheck(), "");
  odb::dbInst::destroy(bridge);
  odb::dbInst::destroy(i1);
  odb::dbInst::destroy(i2);

  // Test VERTICAL rule with negative PRL and EXCEPTCORNERTOUCH on IMP2
  auto* r_v = odb::dbTechLayerImplantSpacingRule::create(imp2_);
  r_v->setMinSpacing(kRowHeight / 2);
  r_v->setVertical(true);
  r_v->setPrl(-kSiteWidth);
  r_v->setPrlValid(true);
  r_v->setExceptCornerTouch(true);

  odb::dbMaster* m2_half = createCustomMaster(
      "M2_BOT_HALF",
      1,
      {{imp2_, odb::Rect(0, 0, kSiteWidth, 3 * kRowHeight / 4)}});
  // Diagonal corner touch (gx=2 row 0 full height vs gx=3 row 1 MX full height)
  // is exempt due to EXCEPTCORNERTOUCH:
  odb::dbInst* v1 = placeCell(m2, "v1", 2, 0);
  odb::dbInst* v2 = placeCell(m2, "v2", 3, 1, odb::dbOrientType::MX);
  EXPECT_EQ(runCheck(), "");
  odb::dbInst::destroy(v1);
  odb::dbInst::destroy(v2);

  // Vertically separated by kRowHeight/4 (< kRowHeight/2) with 1-site X offset
  // (prl_x == -kSiteWidth >= rule_prl) triggers VERTICAL negative-PRL rule!
  odb::dbInst* vn1 = placeCell(m2_half, "vn1", 2, 0);
  odb::dbInst* vn2 = placeCell(m2, "vn2", 4, 1, odb::dbOrientType::MX);
  // At gx=4 (dx_gap = kSiteWidth -> prl_x = -kSiteWidth >= -kSiteWidth,
  // dy_gap = kRowHeight/4 < kRowHeight/2) -> violation!
  EXPECT_EQ(runCheck(), "DPL-0033");
  // Moving vn2 to gx=5 makes prl_x = -2 * kSiteWidth < -kSiteWidth -> legal!
  vn2->setLocation(5 * kSiteWidth, 1 * kRowHeight);
  EXPECT_EQ(runCheck(), "");
  (void) vn1;
}
TEST_F(Lef58ImplantAndEdgeTest, Lef58CoreEdgeLengthAndExemptions)
{
  auto* rule = odb::dbTechLayerCoreEdgeLengthRule::create(imp1_);
  rule->setMinLength(3 * kSiteWidth);
  rule->setMaxLength(6 * kSiteWidth);
  rule->addExceptLayer(ms_od_);
  // Entry 0 (exact_adjacent_length = true): core edge == kSiteWidth is exempt
  // ONLY when an adjacent vertical edge has exact length 2 * kRowHeight.
  rule->addExceptAdjacentLength(kSiteWidth, 2 * kRowHeight, /*exact=*/true);
  // Entry 1 (exact_adjacent_length = false): core edge == 2 * kSiteWidth is
  // exempt when an adjacent vertical edge has length >= 2 * kRowHeight.
  rule->addExceptAdjacentLength(
      2 * kSiteWidth, 2 * kRowHeight, /*exact=*/false);

  odb::dbMaster* m1 = createCustomMaster(
      "M_CORE1", 1, {{imp1_, odb::Rect(0, 0, kSiteWidth, kRowHeight)}});
  odb::dbMaster* m1_exempt
      = createCustomMaster("M_EXEMPT",
                           1,
                           {{imp1_, odb::Rect(0, 0, kSiteWidth, kRowHeight)},
                            {ms_od_, odb::Rect(0, 0, kSiteWidth, kRowHeight)}});
  odb::dbMaster* m_od_only = createCustomMaster(
      "M_OD_ONLY", 1, {{ms_od_, odb::Rect(0, 0, kSiteWidth, kRowHeight)}});

  // 1-site IMP1 has horizontal edges kSiteWidth < 3 * kSiteWidth and vertical
  // edges kRowHeight != 2 * kRowHeight -> violation!
  odb::dbInst* c1 = placeCell(m1, "c1", 2, 0);
  EXPECT_EQ(runCheck(), "DPL-0033");

  // EXCEPTLAYER touching only at an endpoint (gx=3, row=0) or outside y_edge
  // (gx=2, row=1) must NOT exempt c1!
  odb::dbInst* od_side = placeCell(m_od_only, "od_side", 3, 0);
  odb::dbInst* od_above
      = placeCell(m_od_only, "od_above", 2, 1, odb::dbOrientType::MX);
  EXPECT_EQ(runCheck(), "DPL-0033");
  odb::dbInst::destroy(od_side);
  odb::dbInst::destroy(od_above);

  // EXACTADJACENTLENGTH = true: stacking 2 rows tall (adj = 2 * kRowHeight)
  // makes the kSiteWidth core edges exempt -> passes!
  odb::dbInst* c1_r1 = placeCell(m1, "c1_r1", 2, 1, odb::dbOrientType::MX);
  EXPECT_EQ(runCheck(), "");

  // Stacking 3 rows tall (adj = 3 * kRowHeight != 2 * kRowHeight) FAILS exact
  // match on kSiteWidth!
  odb::dbInst* c1_r2 = placeCell(m1, "c1_r2", 2, 2, odb::dbOrientType::R0);
  EXPECT_EQ(runCheck(), "DPL-0033");

  // EXACTADJACENTLENGTH = false: widening all 3 rows to 2 * kSiteWidth gives
  // core edge == 2 * kSiteWidth and adj == 3 * kRowHeight >= 2 * kRowHeight ->
  // passes threshold exemption!
  odb::dbInst* c2_r0 = placeCell(m1, "c2_r0", 3, 0, odb::dbOrientType::R0);
  odb::dbInst* c2_r1 = placeCell(m1, "c2_r1", 3, 1, odb::dbOrientType::MX);
  odb::dbInst* c2_r2 = placeCell(m1, "c2_r2", 3, 2, odb::dbOrientType::R0);
  EXPECT_EQ(runCheck(), "");

  odb::dbInst::destroy(c1);
  odb::dbInst::destroy(c1_r1);
  odb::dbInst::destroy(c1_r2);
  odb::dbInst::destroy(c2_r0);
  odb::dbInst::destroy(c2_r1);
  odb::dbInst::destroy(c2_r2);

  // Single 1-site cell covered on its interior by EXCEPTLAYER (OD_LAYER) is
  // exempt
  placeCell(m1_exempt, "c_ex", 2, 0);
  EXPECT_EQ(runCheck(), "");
}
TEST_F(Lef58ImplantAndEdgeTest, Lef58CoreEdgeLengthPreFillerExteriorGap)
{
  auto* rule = odb::dbTechLayerCoreEdgeLengthRule::create(imp1_);
  rule->setMinLength(3 * kSiteWidth);

  odb::dbMaster* m3 = createCustomMaster(
      "M_CORE3", 3, {{imp1_, odb::Rect(0, 0, 3 * kSiteWidth, kRowHeight)}});
  odb::dbMaster* m7 = createCustomMaster(
      "M_CORE7", 7, {{imp1_, odb::Rect(0, 0, 7 * kSiteWidth, kRowHeight)}});

  // Row 0: two 3-site cells at gx=1..4 and gx=5..8 leave a 1-site gap at
  // gx=4. Row 1: one 7-site cell at gx=1..8 covers both. The merged island
  // exposes a 1-site (< 3-site) bottom edge of the row-1 cell over the gap.
  placeCell(m3, "a", 1, 0);
  placeCell(m3, "c", 5, 0);
  placeCell(m7, "b", 1, 1, odb::dbOrientType::MX);

  // Without an IMP1 filler master the gap can never close -> violation
  EXPECT_EQ(runCheck(), "DPL-0033");

  // With an IMP1 filler master the empty site outside the edge is fillable,
  // so the edge is not final before filler insertion -> legal
  createCustomMaster("FILL_IMP1",
                     1,
                     {{imp1_, odb::Rect(0, 0, kSiteWidth, kRowHeight)}},
                     odb::dbMasterType::CORE_SPACER);
  EXPECT_EQ(runCheck(), "");

  // A non-IMP1 cell in the gap blocks the filler -> violation again
  odb::dbMaster* m_other = createCustomMaster(
      "M_GAP_IMP2", 1, {{imp2_, odb::Rect(0, 0, kSiteWidth, kRowHeight)}});
  placeCell(m_other, "g", 4, 0);
  EXPECT_EQ(runCheck(), "DPL-0033");
}
TEST_F(Lef58ImplantAndEdgeTest, Lef58CornerSpacingAlignedAndGeneral)
{
  // Stricter CHECKIMPLANTGROUPONLY rule must be skipped (not enforced globally)
  auto* grp_rule = odb::dbTechLayerCornerSpacingRule::create(imp1_);
  grp_rule->setCheckImplantGroupOnly(true);
  grp_rule->setAlignedOnly(true);
  grp_rule->addSpacing(0, 10 * kRowHeight);

  auto* rule = odb::dbTechLayerCornerSpacingRule::create(imp1_);
  rule->setAlignedOnly(true);
  rule->addSpacing(0, 2 * kRowHeight);

  odb::dbMaster* m1 = createCustomMaster(
      "M_CORNER", 1, {{imp1_, odb::Rect(0, 0, kSiteWidth, kRowHeight)}});

  // Create two vertically aligned diagonal corner touches at x = 3 *
  // kSiteWidth: Corner 1 between (gx=2, gy=0) and (gx=3, gy=1) at
  // (3*kSiteWidth, kRowHeight) Corner 2 between (gx=2, gy=2) and (gx=3, gy=1)
  // at (3*kSiteWidth, 2*kRowHeight) Vertical distance between the two aligned
  // corners is kRowHeight < 2*kRowHeight!
  odb::dbInst* r0 = placeCell(m1, "r0", 2, 0, odb::dbOrientType::R0);
  odb::dbInst* r1 = placeCell(m1, "r1", 3, 1, odb::dbOrientType::MX);
  odb::dbInst* r2 = placeCell(m1, "r2", 2, 2, odb::dbOrientType::R0);
  EXPECT_EQ(runCheck(), "DPL-0033");

  // Shifting r2 to gx=4 makes the second corner at (4*kSiteWidth,
  // 2*kRowHeight), which is NOT axis-aligned with (3*kSiteWidth, kRowHeight) ->
  // passes ALIGNEDONLY!
  r2->setLocation(4 * kSiteWidth, 2 * kRowHeight);
  EXPECT_EQ(runCheck(), "");

  // General (aligned_only == false) Euclidean rule on IMP1 with width_table
  // lower-bound threshold == kSiteWidth and spacing == 2 * kRowHeight catches
  // those two non-aligned corners (dx=kSiteWidth, dy=kRowHeight, dist <
  // 2*kRowHeight)!
  auto* euc_rule = odb::dbTechLayerCornerSpacingRule::create(imp1_);
  euc_rule->setAlignedOnly(false);
  euc_rule->addSpacing(kSiteWidth, 2 * kRowHeight);
  EXPECT_EQ(runCheck(), "DPL-0033");

  // Moving r2 far away removes the second corner touch -> legal
  odb::dbInst::destroy(r2);
  EXPECT_EQ(runCheck(), "");
  odb::dbInst::destroy(r0);
  odb::dbInst::destroy(r1);

  // Concave corners of ONE merged U-shaped polygon are not corner-touch
  // junctions and must not be measured (LEF 5.8 CORNERSPACING measures the
  // distance between corners of two shapes; CONCAVECORNER is a separate form):
  // Row 0: gx=2, 3, 4 (3-site base)
  // Row 1: gx=2 and gx=4 (leaving a 1-site U-notch at gx=3 whose two concave
  //        corners are only kSiteWidth apart) -> legal
  odb::dbInst* u_b0 = placeCell(m1, "u_b0", 2, 0, odb::dbOrientType::R0);
  odb::dbInst* u_b1 = placeCell(m1, "u_b1", 3, 0, odb::dbOrientType::R0);
  odb::dbInst* u_b2 = placeCell(m1, "u_b2", 4, 0, odb::dbOrientType::R0);
  odb::dbInst* u_l = placeCell(m1, "u_l", 2, 1, odb::dbOrientType::MX);
  odb::dbInst* u_r = placeCell(m1, "u_r", 4, 1, odb::dbOrientType::MX);
  EXPECT_EQ(runCheck(), "");
  for (odb::dbInst* inst : {u_b0, u_b1, u_b2, u_l, u_r}) {
    odb::dbInst::destroy(inst);
  }

  // Pre-filler relief: with an IMP1 filler master available, a junction is
  // not final while one of its two empty quadrants can still take an IMP1
  // filler (the filler edge-connects the two shapes). Re-create the two
  // aligned junctions from above -> legal before fillers.
  createCustomMaster("FILL_IMP1",
                     1,
                     {{imp1_, odb::Rect(0, 0, kSiteWidth, kRowHeight)}},
                     odb::dbMasterType::CORE_SPACER);
  placeCell(m1, "p0", 2, 0, odb::dbOrientType::R0);
  placeCell(m1, "p1", 3, 1, odb::dbOrientType::MX);
  placeCell(m1, "p2", 2, 2, odb::dbOrientType::R0);
  EXPECT_EQ(runCheck(), "");

  // Occupying every empty quadrant with non-IMP1 cells leaves no room for an
  // IMP1 filler, so both junctions are final again -> violation.
  odb::dbMaster* m_other = createCustomMaster(
      "M_CORNER_IMP2", 1, {{imp2_, odb::Rect(0, 0, kSiteWidth, kRowHeight)}});
  placeCell(m_other, "q0", 3, 0, odb::dbOrientType::R0);
  placeCell(m_other, "q1", 2, 1, odb::dbOrientType::MX);
  placeCell(m_other, "q2", 3, 2, odb::dbOrientType::R0);
  EXPECT_EQ(runCheck(), "DPL-0033");
}
}  // namespace dpl
