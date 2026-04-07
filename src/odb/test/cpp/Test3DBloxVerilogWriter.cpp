// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2026, The OpenROAD Authors
//
// Tests for VerilogWriter path-length filtering:
//   - bumps with path.size() == 1 (direct child) must appear in output
//   - bumps with path.size() > 1 (nested child) must be silently skipped

#include <fstream>
#include <iterator>
#include <string>

#include "gtest/gtest.h"
#include "odb/3dblox.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "tst/fixture.h"

namespace odb {
namespace {

// Helper to read an entire file into a string.
static std::string readFileContent(const std::string& path)
{
  std::ifstream file(path);
  return std::string(std::istreambuf_iterator<char>(file),
                     std::istreambuf_iterator<char>());
}

class VerilogWriterFixture : public tst::Fixture
{
 protected:
  VerilogWriterFixture()
  {
    tech_ = dbTech::create(db_.get(), "tech");
    dbTechLayer::create(tech_, "layer1", dbTechLayerType::ROUTING);

    // HIER top chip (the module being written).
    top_chip_
        = dbChip::create(db_.get(), nullptr, "TopChip", dbChip::ChipType::HIER);

    // DIE chip used as the master for instances.
    chip1_ = dbChip::create(db_.get(), tech_, "Chip1", dbChip::ChipType::DIE);
    chip1_->setWidth(2000);
    chip1_->setHeight(2000);
    chip1_->setThickness(500);
    dbBlock::create(chip1_, "block1");

    // A second DIE chip, only needed to form multi-element path vectors.
    chip2_ = dbChip::create(db_.get(), tech_, "Chip2", dbChip::ChipType::DIE);
    chip2_->setWidth(1500);
    chip2_->setHeight(1500);
    chip2_->setThickness(500);
    dbBlock::create(chip2_, "block2");

    // Front region on chip1 where bumps will be placed.
    dbChipRegion* r1_fr = dbChipRegion::create(
        chip1_, "r1_fr", dbChipRegion::Side::FRONT, nullptr);
    r1_fr->setBox(Rect(0, 0, 2000, 2000));

    // Shared bump master for physical bump cells.
    lib_ = dbLib::create(db_.get(), "bump_lib", tech_, ',');
    bump_master_ = dbMaster::create(lib_, "bump_pad");
    bump_master_->setWidth(100);
    bump_master_->setHeight(100);
    bump_master_->setType(dbMasterType::CORE);
    dbMTerm::create(bump_master_, "pad", dbIoType::INOUT, dbSigType::SIGNAL);
    bump_master_->setFrozen();
  }

  // Create a bump on a chip region and attach a named BTerm (Verilog port).
  // Must be called BEFORE creating any dbChipInst that uses this chip,
  // so the bump inst is propagated into the region inst at create time.
  dbChipBump* createBump(dbChip* chip,
                         dbChipRegion* region,
                         const char* bump_name,
                         int x,
                         int y)
  {
    dbBlock* block = chip->getBlock();
    dbTechLayer* layer = tech_->findLayer("layer1");

    // Physical placement cell for the bump.
    dbInst* inst = dbInst::create(block, bump_master_, bump_name);
    inst->setOrigin(x, y);
    inst->setPlacementStatus(dbPlacementStatus::PLACED);

    // Chip-level bump object.
    dbChipBump* chip_bump = dbChipBump::create(region, inst);

    // Net + BTerm — the BTerm name becomes the Verilog port name.
    const std::string net_name = std::string(bump_name) + "_net";
    dbNet* net = dbNet::create(block, net_name.c_str());
    dbBTerm* bterm = dbBTerm::create(net, bump_name);
    bterm->setIoType(dbIoType::INOUT);

    // BPin geometry required for placement checks.
    dbBPin* bpin = dbBPin::create(bterm);
    bpin->setPlacementStatus(dbPlacementStatus::PLACED);
    dbBox::create(bpin, layer, x, y, x + 100, y + 100);

    chip_bump->setNet(net);
    chip_bump->setBTerm(bterm);
    return chip_bump;
  }

  // Write the top chip's Verilog to a fixed temp file and return the path.
  std::string writeVerilog()
  {
    const std::string filename = "/tmp/odb_test_verilog_writer.v";
    ThreeDBlox writer(&logger_, db_.get());
    writer.writeVerilog(filename, top_chip_);
    return filename;
  }

  dbTech* tech_;
  dbChip* top_chip_;
  dbChip* chip1_;
  dbChip* chip2_;
  dbLib* lib_;
  dbMaster* bump_master_;
};

// A bump registered with path.size() == 1 (direct child of the HIER chip)
// must produce a port connection in the Verilog output.
TEST_F(VerilogWriterFixture, test_direct_child_written)
{
  // Create bump BEFORE the chip inst so it is propagated into the region inst.
  dbChipRegion* r1_fr = chip1_->findChipRegion("r1_fr");
  createBump(chip1_, r1_fr, "port_a", 100, 100);

  dbChipInst* inst1 = dbChipInst::create(top_chip_, chip1_, "inst1");
  inst1->setLoc(Point3D(0, 0, 0));
  inst1->setOrient(dbOrientType3D(dbOrientType::R0, false));

  // Retrieve the bump inst auto-created on the region inst.
  dbChipRegionInst* ri1 = inst1->findChipRegionInst("r1_fr");
  ASSERT_NE(ri1, nullptr);
  dbChipBumpInst* bump_inst = *ri1->getChipBumpInsts().begin();
  ASSERT_NE(bump_inst, nullptr);

  // Wire the bump to a net via a direct (path length 1) path.
  dbChipNet* chip_net = dbChipNet::create(top_chip_, "net_signal");
  chip_net->addBumpInst(bump_inst, {inst1});

  const std::string filename = writeVerilog();
  const std::string content = readFileContent(filename);

  // The port connection must appear in the output.
  EXPECT_NE(content.find(".port_a(net_signal)"), std::string::npos);
}

// A bump registered with path.size() > 1 (nested child) must be silently
// skipped; no port connection should appear in the Verilog output.
TEST_F(VerilogWriterFixture, test_nested_child_skipped)
{
  // Create bump BEFORE the chip inst so it is propagated into the region inst.
  dbChipRegion* r1_fr = chip1_->findChipRegion("r1_fr");
  createBump(chip1_, r1_fr, "port_b", 200, 200);

  dbChipInst* inst1 = dbChipInst::create(top_chip_, chip1_, "inst1");
  inst1->setLoc(Point3D(0, 0, 0));
  inst1->setOrient(dbOrientType3D(dbOrientType::R0, false));

  // inst2 is used only to form a path vector of length 2.
  dbChipInst* inst2 = dbChipInst::create(top_chip_, chip2_, "inst2");
  inst2->setLoc(Point3D(0, 0, 500));
  inst2->setOrient(dbOrientType3D(dbOrientType::R0, false));

  dbChipRegionInst* ri1 = inst1->findChipRegionInst("r1_fr");
  ASSERT_NE(ri1, nullptr);
  dbChipBumpInst* bump_inst = *ri1->getChipBumpInsts().begin();
  ASSERT_NE(bump_inst, nullptr);

  // Path of length 2 — nested child; writer must skip this bump.
  dbChipNet* chip_net = dbChipNet::create(top_chip_, "net_deep");
  chip_net->addBumpInst(bump_inst, {inst1, inst2});

  const std::string filename = writeVerilog();
  const std::string content = readFileContent(filename);

  // No port connection should appear for the skipped bump.
  EXPECT_EQ(content.find(".port_b"), std::string::npos);
}

}  // namespace
}  // namespace odb
