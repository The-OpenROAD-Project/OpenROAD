// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2026, The OpenROAD Authors

#include <string>

#include "Test3DBloxCheckerFixture.h"
#include "gtest/gtest.h"
#include "odb/3dblox.h"
#include "odb/db.h"
#include "odb/dbWireCodec.h"

namespace odb {

namespace {

class BumpsFixture : public CheckerFixture
{
 protected:
  BumpsFixture()
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

TEST_F(BumpsFixture, bumpAlignmentSuccess)
{
  auto chip1_r1_fr = chip1_->findChipRegion("r1_fr");
  createBump(chip1_, chip1_r1_fr, "bump1", 500, 500);

  auto inst1 = dbChipInst::create(top_chip_, chip1_, "inst1");
  inst1->setLoc(Point3D(0, 0, 0));
  inst1->setOrient(dbOrientType3D(dbOrientType::R0, false));

  check();

  auto markers = getMarkers(CheckerFixture::bump_alignment_category);
  EXPECT_EQ(markers.size(), 0);
}

TEST_F(BumpsFixture, bumpAlignmentFailure)
{
  auto chip1_r1_fr = chip1_->findChipRegion("r1_fr");
  // Region is (0,0) to (2000, 2000)
  // Placing bump outside at (-100, -100)
  createBump(chip1_, chip1_r1_fr, "bump1", -100, -100);

  auto inst1 = dbChipInst::create(top_chip_, chip1_, "inst1");
  inst1->setLoc(Point3D(0, 0, 0));
  inst1->setOrient(dbOrientType3D(dbOrientType::R0, false));

  check();

  auto markers = getMarkers(CheckerFixture::bump_alignment_category);
  EXPECT_EQ(markers.size(), 1);
  if (!markers.empty()) {
    EXPECT_STREQ(markers[0]->getComment().c_str(),
                 "Bump is outside its parent region r1_fr");
  }
}

}  // namespace

}  // namespace odb
