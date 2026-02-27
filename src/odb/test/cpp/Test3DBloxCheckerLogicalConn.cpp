#include <gtest/gtest.h>

#include <string>

#include "odb/3dblox.h"
#include "odb/db.h"
#include "tst/fixture.h"

namespace odb {

class LogicalConnFixture : public tst::Fixture
{
 protected:
  void SetUp() override
  {
    blox_ = new ThreeDBlox(&logger_, db_.get(), sta_.get());
  }

  void TearDown() override { delete blox_; }

  ThreeDBlox* blox_;
};

TEST_F(LogicalConnFixture, TestMissingNetCreation)
{
  std::string dbv_file
      = getFilePath("src/odb/test/data/3dblox_logical_test.3dbv");

  // Run readDbv
  blox_->readDbv(dbv_file);

  // Check if chip "top" was created
  auto* created_chip = db_->findChip("top");
  ASSERT_NE(created_chip, nullptr);

  // Check if nets "A" and "Z" were created in the chip
  bool foundA = false;
  bool foundZ = false;
  for (auto* net : created_chip->getChipNets()) {
    if (net->getName() == "A") {
      foundA = true;
    }
    if (net->getName() == "Z") {
      foundZ = true;
    }
  }
  EXPECT_TRUE(foundA);
  EXPECT_TRUE(foundZ);
}

TEST_F(LogicalConnFixture, TestCheckerNoViolations)
{
  auto* tech = dbTech::create(db_.get(), "tech");
  // master chip name matches module name in Verilog "top"
  auto* master = dbChip::create(db_.get(), tech, "top", dbChip::ChipType::DIE);
  dbChipNet::create(master, "A");
  dbChipNet::create(master, "Z");

  std::string verilog_file
      = getFilePath("src/odb/test/data/3dblox_logical_check_pass.v");
  odb::dbStringProperty::create(master, "verilog_file", verilog_file.c_str());

  // top chip containing instance of master
  auto* design_top = dbChip::create(
      db_.get(), nullptr, "design_top", dbChip::ChipType::HIER);
  dbChipInst::create(design_top, master, "inst1");
  db_->setTopChip(design_top);

  // Run Checker
  blox_->check();

  // Verify no markers in "Logical Alignment" or "Design Alignment"
  auto* top_cat = design_top->findMarkerCategory("3DBlox");
  if (top_cat) {
    auto* design_cat = top_cat->findMarkerCategory("Design Alignment");
    if (design_cat) {
      EXPECT_EQ(design_cat->getMarkers().size(), 0);
    }
    auto* align_cat = top_cat->findMarkerCategory("Logical Alignment");
    if (align_cat) {
      EXPECT_EQ(align_cat->getMarkers().size(), 0);
    }
  }
}

TEST_F(LogicalConnFixture, TestCheckerMissingModule)
{
  auto* tech = dbTech::create(db_.get(), "tech");
  // master chip name "top" but Verilog has "other_module"
  auto* master = dbChip::create(db_.get(), tech, "top", dbChip::ChipType::DIE);

  std::string verilog_file
      = getFilePath("src/odb/test/data/3dblox_logical_check_fail_mod.v");
  odb::dbStringProperty::create(master, "verilog_file", verilog_file.c_str());

  auto* design_top = dbChip::create(
      db_.get(), nullptr, "design_top", dbChip::ChipType::HIER);
  dbChipInst::create(design_top, master, "inst1");
  db_->setTopChip(design_top);

  blox_->check();

  // Verify "Design Alignment" marker
  auto* top_cat = design_top->findMarkerCategory("3DBlox");
  ASSERT_NE(top_cat, nullptr);
  auto* design_cat = top_cat->findMarkerCategory("Design Alignment");
  ASSERT_NE(design_cat, nullptr);
  EXPECT_GT(design_cat->getMarkers().size(), 0);
}

TEST_F(LogicalConnFixture, TestCheckerMissingNetInVerilog)
{
  auto* tech = dbTech::create(db_.get(), "tech");
  // master chip name "top" matches module name in Verilog
  auto* master = dbChip::create(db_.get(), tech, "top", dbChip::ChipType::DIE);

  // Create a bump connected to a net "EXTRA" that is NOT in Verilog
  auto* region
      = dbChipRegion::create(master, "r1", dbChipRegion::Side::FRONT, nullptr);
  auto* block = dbBlock::create(master, "block");
  auto* lib = dbLib::create(db_.get(), "lib", tech);
  auto* bump_master = dbMaster::create(lib, "bump_cell");
  bump_master->setFrozen();
  auto* inst = dbInst::create(block, bump_master, "bump_inst");
  auto* bump = dbChipBump::create(region, inst);
  auto* net = dbNet::create(block, "EXTRA");
  bump->setNet(net);

  std::string verilog_file
      = getFilePath("src/odb/test/data/3dblox_logical_check_fail_net.v");
  odb::dbStringProperty::create(master, "verilog_file", verilog_file.c_str());

  auto* design_top = dbChip::create(
      db_.get(), nullptr, "design_top", dbChip::ChipType::HIER);
  dbChipInst::create(design_top, master, "inst1");
  db_->setTopChip(design_top);

  blox_->check();

  // Verify "Logical Alignment" marker
  auto* top_cat = design_top->findMarkerCategory("3DBlox");
  ASSERT_NE(top_cat, nullptr);
  auto* align_cat = top_cat->findMarkerCategory("Logical Alignment");
  ASSERT_NE(align_cat, nullptr);
  EXPECT_GT(align_cat->getMarkers().size(), 0);
}

}  // namespace odb
