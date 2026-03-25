// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2026, The OpenROAD Authors

#include <string>
#include <vector>

#include "Test3DBloxCheckerFixture.h"
#include "gtest/gtest.h"
#include "odb/3dblox.h"
#include "odb/db.h"
#include "odb/geom.h"

namespace odb {
namespace {

class CheckerLogicalConnFixture : public CheckerFixture
{
 protected:
  CheckerLogicalConnFixture()
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
  dbChipBump* createBump(dbChip* chip,
                         dbChipRegion* region,
                         const char* bump_name,
                         int x,
                         int y)
  {
    dbBlock* block = chip->getBlock();
    dbTechLayer* layer = tech_->findLayer("layer1");

    // Physical cell in the block
    dbInst* inst = dbInst::create(block, bump_master_, bump_name);
    inst->setOrigin(x, y);
    inst->setPlacementStatus(dbPlacementStatus::PLACED);

    // Chip-level bump association
    dbChipBump* chip_bump = dbChipBump::create(region, inst);

    // Net + BTerm for wire-graph connectivity
    std::string net_name = std::string(bump_name) + "_net";
    dbNet* net = block->findNet(net_name.c_str());
    if (!net) {
      net = dbNet::create(block, net_name.c_str());
    }
    dbBTerm* bterm = dbBTerm::create(net, bump_name);
    bterm->setIoType(dbIoType::INOUT);

    // BPin with box at the bump location
    dbBPin* bpin = dbBPin::create(bterm);
    bpin->setPlacementStatus(dbPlacementStatus::PLACED);
    dbBox::create(bpin, layer, x, y, x + 100, y + 100);

    chip_bump->setNet(net);
    chip_bump->setBTerm(bterm);

    return chip_bump;
  }

  void check()
  {
    utl::Logger logger;
    db_->constructUnfoldedModel();
    ThreeDBlox three_dblox(&logger, db_.get());
    three_dblox.check();
  }

  dbLib* lib_;
  dbMaster* bump_master_;
};

TEST_F(CheckerLogicalConnFixture, test_logical_connectivity_matching_nets)
{
  auto* r1_fr = chip1_->findChipRegion("r1_fr");
  auto* r2_bk = chip2_->findChipRegion("r2_bk");

  createBump(chip1_, r1_fr, "bump1", 100, 100);
  createBump(chip2_, r2_bk, "bump2", 100, 100);

  auto inst1 = dbChipInst::create(top_chip_, chip1_, "inst1");
  inst1->setLoc(Point3D(0, 0, 0));
  inst1->setOrient(dbOrientType3D(dbOrientType::R0, false));

  auto inst2 = dbChipInst::create(top_chip_, chip2_, "inst2");
  inst2->setLoc(Point3D(0, 0, 500));  // Stacked on top
  inst2->setOrient(dbOrientType3D(dbOrientType::R0, false));

  auto* ri1 = inst1->findChipRegionInst("r1_fr");
  auto* ri2 = inst2->findChipRegionInst("r2_bk");

  auto* conn1 = dbChipConn::create("c1", top_chip_, {inst1}, ri1, {inst2}, ri2);
  conn1->setThickness(0);

  auto inst1_bump1 = *ri1->getChipBumpInsts().begin();
  auto inst2_bump2 = *ri2->getChipBumpInsts().begin();

  auto* chip_net = dbChipNet::create(top_chip_, "net1");
  chip_net->addBumpInst(inst1_bump1, {inst1});
  chip_net->addBumpInst(inst2_bump2, {inst2});

  check();

  auto markers = getMarkers(logical_connectivity_category);
  EXPECT_TRUE(markers.empty());
}

TEST_F(CheckerLogicalConnFixture, test_logical_connectivity_mismatching_nets)
{
  auto* r1_fr = chip1_->findChipRegion("r1_fr");
  auto* r2_bk = chip2_->findChipRegion("r2_bk");

  createBump(chip1_, r1_fr, "bump1", 200, 200);
  createBump(chip2_, r2_bk, "bump2", 200, 200);

  auto inst1 = dbChipInst::create(top_chip_, chip1_, "inst1");
  inst1->setLoc(Point3D(0, 0, 0));
  inst1->setOrient(dbOrientType3D(dbOrientType::R0, false));

  auto inst2 = dbChipInst::create(top_chip_, chip2_, "inst2");
  inst2->setLoc(Point3D(0, 0, 500));  // Stacked on top
  inst2->setOrient(dbOrientType3D(dbOrientType::R0, false));

  auto* ri1 = inst1->findChipRegionInst("r1_fr");
  auto* ri2 = inst2->findChipRegionInst("r2_bk");

  auto* conn1 = dbChipConn::create("c1", top_chip_, {inst1}, ri1, {inst2}, ri2);
  conn1->setThickness(0);

  auto inst1_bump1 = *ri1->getChipBumpInsts().begin();
  auto inst2_bump2 = *ri2->getChipBumpInsts().begin();

  auto* chip_net1 = dbChipNet::create(top_chip_, "net1");
  chip_net1->addBumpInst(inst1_bump1, {inst1});

  auto* chip_net2 = dbChipNet::create(top_chip_, "net2");
  chip_net2->addBumpInst(inst2_bump2, {inst2});

  check();

  auto markers = getMarkers(logical_connectivity_category);
  EXPECT_EQ(markers.size(), 1);
  if (!markers.empty()) {
    auto sources = markers[0]->getSources();
    EXPECT_EQ(sources.size(), 2);

    // Check message correctness conceptually
    EXPECT_NE(markers[0]->getComment().find("logical connectivity mismatch"),
              std::string::npos);
  }
}

}  // namespace
}  // namespace odb
