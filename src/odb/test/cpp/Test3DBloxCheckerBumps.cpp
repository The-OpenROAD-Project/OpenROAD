// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2026, The OpenROAD Authors

#include <string>

#include "Test3DBloxCheckerFixture.h"
#include "gtest/gtest.h"
#include "odb/3dblox.h"
#include "odb/db.h"
#include "odb/dbWireCodec.h"
#include "odb/geom.h"

namespace odb {

namespace {

class CoincidentBumpsFixture : public CheckerFixture
{
 protected:
  CoincidentBumpsFixture()
  {
    dbTechLayer::create(tech_, "layer1", dbTechLayerType::ROUTING);

    // DIE chips need blocks (for bump masters/cells)
    dbBlock::create(chip1_, "block1");
    dbBlock::create(chip2_, "block2");

    // Lib + bump master
    lib_ = dbLib::create(db_.get(), "bump_lib", tech_, ',');
    bump_master_ = dbMaster::create(lib_, "bump_pad");
    bump_master_->setWidth(100);
    bump_master_->setHeight(100);
    bump_master_->setType(dbMasterType::CORE);
    dbMTerm::create(bump_master_, "pin", dbIoType::INOUT, dbSigType::SIGNAL);
    bump_master_->setFrozen();
  }

  // Create a bump on a chip region.
  // - Creates dbInst at (x,y) in the chip's block
  // - Creates dbChipBump on the region
  // - Creates dbNet, dbBTerm, dbBPin with a box at the bump location
  // - Sets the bump's net and bterm
  // Returns the dbChipBump for further use.
  dbChipBump* createBump(dbChip* chip,
                         dbChipRegion* region,
                         const char* bump_name,
                         int x,
                         int y)
  {
    dbBlock* block = chip->getBlock();

    // Physical cell in the block
    dbInst* inst = dbInst::create(block, bump_master_, bump_name);
    inst->setOrigin(x, y);
    inst->setPlacementStatus(dbPlacementStatus::PLACED);

    // Chip-level bump association
    dbChipBump* chip_bump = dbChipBump::create(region, inst);

    return chip_bump;
  }
  dbLib* lib_;
  dbMaster* bump_master_;
};

// Same-region, same XY → violation
TEST_F(CoincidentBumpsFixture, test_same_region_same_xy_is_violation)
{
  // Two bumps stacked at (200, 200) on chip1's front region
  createBump(chip1_, chip1_->findChipRegion("r1_fr"), "bA", 200, 200);
  createBump(chip1_, chip1_->findChipRegion("r1_fr"), "bB", 200, 200);
  // A non-coincident bump to confirm it doesn't get caught
  createBump(chip1_, chip1_->findChipRegion("r1_fr"), "bC", 400, 400);

  auto inst1 = dbChipInst::create(top_chip_, chip1_, "inst1");
  inst1->setLoc(Point3D(0, 0, 0));
  inst1->setOrient(dbOrientType3D(dbOrientType::R0, false));

  check();
  auto markers = getMarkers(coincident_bumps_category);
  ASSERT_EQ(markers.size(), 1);
  EXPECT_TRUE(markers[0]->getComment().find("2 coincident bumps")
              != std::string::npos);
  EXPECT_TRUE(markers[0]->getComment().find("(200, 200)") != std::string::npos);
}

// Cross-chip, same XY → not a violation (that's how connections work)
TEST_F(CoincidentBumpsFixture, test_cross_chip_same_xy_no_violation)
{
  // chip1 FRONT and chip2 BACK at the same global XY — intentional, no marker
  createBump(chip1_, chip1_->findChipRegion("r1_fr"), "b1", 100, 100);
  createBump(chip2_, chip2_->findChipRegion("r2_bk"), "b2", 100, 100);

  auto inst1 = dbChipInst::create(top_chip_, chip1_, "inst1");
  inst1->setLoc(Point3D(0, 0, 0));
  inst1->setOrient(dbOrientType3D(dbOrientType::R0, false));

  auto inst2 = dbChipInst::create(top_chip_, chip2_, "inst2");
  inst2->setLoc(Point3D(0, 0, 500));
  inst2->setOrient(dbOrientType3D(dbOrientType::R0, false));

  check();
  EXPECT_TRUE(getMarkers(coincident_bumps_category).empty());
}

// Three bumps at the same XY on the same region → one marker, count=3
TEST_F(CoincidentBumpsFixture, test_three_way_coincidence)
{
  createBump(chip1_, chip1_->findChipRegion("r1_fr"), "b1", 300, 300);
  createBump(chip1_, chip1_->findChipRegion("r1_fr"), "b2", 300, 300);
  createBump(chip1_, chip1_->findChipRegion("r1_fr"), "b3", 300, 300);

  auto inst1 = dbChipInst::create(top_chip_, chip1_, "inst1");
  inst1->setLoc(Point3D(0, 0, 0));
  inst1->setOrient(dbOrientType3D(dbOrientType::R0, false));

  check();
  auto markers = getMarkers(coincident_bumps_category);
  ASSERT_EQ(markers.size(), 1);
  EXPECT_TRUE(markers[0]->getComment().find("3 coincident bumps")
              != std::string::npos);
}

}  // namespace

}  // namespace odb
