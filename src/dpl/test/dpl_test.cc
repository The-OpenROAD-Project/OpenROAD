// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include <cstdlib>
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

}  // namespace dpl
