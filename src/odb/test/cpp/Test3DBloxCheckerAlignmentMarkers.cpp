// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2026, The OpenROAD Authors

#include <string>

#include "Test3DBloxCheckerFixture.h"
#include "gtest/gtest.h"
#include "odb/3dblox.h"
#include "odb/db.h"
#include "odb/dbTypes.h"

namespace odb {

namespace {

class AlignmentMarkersFixture : public CheckerFixture
{
 protected:
  AlignmentMarkersFixture()
  {
    dbTechLayer::create(tech_, "layer1", dbTechLayerType::ROUTING);

    dbBlock::create(chip1_, "block1");
    dbBlock::create(chip2_, "block2");

    lib_ = dbLib::create(db_.get(), "align_lib", tech_, ',');
    marker_master_ = dbMaster::create(lib_, "ALIGN_MARK");
    marker_master_->setWidth(100);
    marker_master_->setHeight(100);
    marker_master_->setType(dbMasterType::CORE);
    dbMTerm::create(marker_master_, "pin", dbIoType::INOUT, dbSigType::SIGNAL);
    marker_master_->setFrozen();
    marker_master_->setAlignmentMarker(true);
  }

  void placeMarker(dbChip* chip, const char* name, int x, int y)
  {
    dbBlock* block = chip->getBlock();
    dbInst* inst = dbInst::create(block, marker_master_, name);
    inst->setOrigin(x, y);
    inst->setPlacementStatus(dbPlacementStatus::PLACED);
  }

  // chip1 at z=[0,500], chip2 stacked on top at z=[500,1000], bonded via a
  // dbChipConn between chip1's front region and chip2's back region. The
  // alignment-marker check is connection-aware, so the connection is required
  // for markers on the two chips to be compared.
  void stackChips()
  {
    auto inst1 = dbChipInst::create(top_chip_, chip1_, "inst1");
    inst1->setLoc(Point3D(0, 0, 0));
    inst1->setOrient(dbOrientType3D(dbOrientType::R0, false));

    auto inst2 = dbChipInst::create(top_chip_, chip2_, "inst2");
    inst2->setLoc(Point3D(0, 0, 500));
    inst2->setOrient(dbOrientType3D(dbOrientType::R0, false));

    auto* ri1 = inst1->findChipRegionInst("r1_fr");
    auto* ri2 = inst2->findChipRegionInst("r2_bk");
    auto* conn
        = dbChipConn::create("c1", top_chip_, {inst1}, ri1, {inst2}, ri2);
    conn->setThickness(0);
  }

  dbLib* lib_;
  dbMaster* marker_master_;

  static constexpr const char* alignment_markers_category = "Alignment Markers";
};

TEST_F(AlignmentMarkersFixture, exactAlignmentZeroTolerance)
{
  placeMarker(chip1_, "m1", 500, 500);
  placeMarker(chip2_, "m1", 500, 500);
  stackChips();
  top_chip_->setAlignmentMarkerTolerance(0);

  check();

  EXPECT_EQ(getMarkers(alignment_markers_category).size(), 0);
}

TEST_F(AlignmentMarkersFixture, withinToleranceNoViolation)
{
  placeMarker(chip1_, "m1", 500, 500);
  placeMarker(chip2_, "m1", 520, 510);  // offset ~22 units
  stackChips();
  top_chip_->setAlignmentMarkerTolerance(50);

  check();

  EXPECT_EQ(getMarkers(alignment_markers_category).size(), 0);
}

TEST_F(AlignmentMarkersFixture, beyondToleranceViolation)
{
  placeMarker(chip1_, "m1", 500, 500);
  placeMarker(chip2_, "m1", 700, 700);  // offset ~283 units
  stackChips();
  top_chip_->setAlignmentMarkerTolerance(50);

  check();

  // Neither marker has a valid counterpart -> two violations.
  EXPECT_EQ(getMarkers(alignment_markers_category).size(), 2);
}

TEST_F(AlignmentMarkersFixture, orphanMarker)
{
  placeMarker(chip1_, "m1", 500, 500);
  placeMarker(chip1_, "m2", 800, 800);
  placeMarker(chip2_, "m1", 500, 500);
  stackChips();
  top_chip_->setAlignmentMarkerTolerance(10);

  check();

  // m2 on chip1 has no counterpart -> one violation.
  EXPECT_EQ(getMarkers(alignment_markers_category).size(), 1);
}

TEST_F(AlignmentMarkersFixture, zeroToleranceRejectsNonExactMatch)
{
  placeMarker(chip1_, "m1", 500, 500);
  placeMarker(chip2_, "m1", 501, 500);  // off by 1 unit
  stackChips();
  top_chip_->setAlignmentMarkerTolerance(0);

  check();

  EXPECT_EQ(getMarkers(alignment_markers_category).size(), 2);
}

TEST_F(AlignmentMarkersFixture, noMarkersPassIsNoOp)
{
  // Don't flag the master as an alignment marker.
  marker_master_->setAlignmentMarker(false);
  placeMarker(chip1_, "m1", 500, 500);
  placeMarker(chip2_, "m1", 999, 999);
  stackChips();
  top_chip_->setAlignmentMarkerTolerance(10);

  check();

  EXPECT_EQ(getMarkers(alignment_markers_category).size(), 0);
}

TEST_F(AlignmentMarkersFixture, noConnectionNoCheck)
{
  // Two chip insts placed on top of each other but with no dbChipConn between
  // them. The check is connection-aware, so mismatched markers must not be
  // flagged.
  placeMarker(chip1_, "m1", 500, 500);
  placeMarker(chip2_, "m1", 900, 900);

  auto inst1 = dbChipInst::create(top_chip_, chip1_, "inst1");
  inst1->setLoc(Point3D(0, 0, 0));
  inst1->setOrient(dbOrientType3D(dbOrientType::R0, false));

  auto inst2 = dbChipInst::create(top_chip_, chip2_, "inst2");
  inst2->setLoc(Point3D(0, 0, 500));
  inst2->setOrient(dbOrientType3D(dbOrientType::R0, false));

  top_chip_->setAlignmentMarkerTolerance(10);

  check();

  EXPECT_EQ(getMarkers(alignment_markers_category).size(), 0);
}

TEST_F(AlignmentMarkersFixture, masterFlagRoundTrip)
{
  dbMaster* m = dbMaster::create(lib_, "another");
  EXPECT_FALSE(m->isAlignmentMarker());
  m->setAlignmentMarker(true);
  EXPECT_TRUE(m->isAlignmentMarker());
  m->setAlignmentMarker(false);
  EXPECT_FALSE(m->isAlignmentMarker());
}

}  // namespace

}  // namespace odb
