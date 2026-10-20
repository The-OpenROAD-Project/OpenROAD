// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include <memory>
#include <stdexcept>
#include <string>

#include "dpl/Opendp.h"
#include "gtest/gtest.h"
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
  auto* block = readDef("_main/src/dpl/test/gcd_replace.def", {lib});
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
    ASSERT_TRUE(updateLib(lib_, "_main/src/dpl/test/extra.lef"));
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

}  // namespace dpl
