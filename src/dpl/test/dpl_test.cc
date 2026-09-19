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
    EXPECT_NE(master, nullptr);
    return makeInst(
        block_, master, inst_name, {.location = location, .status = status});
  }

  // Run the check and return the logger error id it raised, or "" if none.
  std::string runCheck(const bool fixed_only)
  {
    try {
      dp_.checkPlacement(/*verbose=*/false, "", fixed_only);
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

  EXPECT_EQ(runCheck(/*fixed_only=*/true), "");
  EXPECT_EQ(block_->findMarkerCategory("DPL"), nullptr);

  // The full check still reports the movable cell.
  EXPECT_EQ(runCheck(/*fixed_only=*/false), "DPL-0033");
  EXPECT_EQ(failureCount("Site_alignment_failures"), 1);
}

TEST_F(CheckPlacementTest, FixedOnlyReportsMisalignedTapcell)
{
  odb::dbInst* tap = makeCell("TAPCELL_X1",
                              "tap",
                              {kCoreX + 100, kCoreY},
                              odb::dbPlacementStatus::FIRM);

  EXPECT_EQ(runCheck(/*fixed_only=*/true), "DPL-0040");

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

  EXPECT_EQ(runCheck(/*fixed_only=*/true), "DPL-0040");
  EXPECT_EQ(failureCount("In_rows_failures"), 1);
}

TEST_F(CheckPlacementTest, FixedOnlyReportsMacroOverlappingTapcell)
{
  makeCell("TAPCELL_X1", "tap", {kCoreX, kCoreY}, odb::dbPlacementStatus::FIRM);
  makeCell("BLOCK1", "macro", {kCoreX, kCoreY}, odb::dbPlacementStatus::FIRM);

  EXPECT_EQ(runCheck(/*fixed_only=*/true), "DPL-0040");
  EXPECT_EQ(failureCount("Overlap_failures"), 1);
}

TEST_F(CheckPlacementTest, FixedOnlyChecksPlacedButUnfixedMacro)
{
  makeCell("TAPCELL_X1", "tap", {kCoreX, kCoreY}, odb::dbPlacementStatus::FIRM);
  // A placed macro that was never marked fixed is still part of the
  // floorplan and must be checked.
  makeCell("BLOCK1", "macro", {kCoreX, kCoreY}, odb::dbPlacementStatus::PLACED);

  EXPECT_EQ(runCheck(/*fixed_only=*/true), "DPL-0040");
  EXPECT_EQ(failureCount("Overlap_failures"), 1);
}

TEST_F(CheckPlacementTest, FixedOnlyErrorsOnUnplacedMacro)
{
  makeCell("BLOCK1", "macro", {kCoreX, kCoreY}, odb::dbPlacementStatus::NONE);

  EXPECT_EQ(runCheck(/*fixed_only=*/true), "DPL-0405");
}

}  // namespace dpl
