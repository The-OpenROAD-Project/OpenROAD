// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include <filesystem>
#include <fstream>
#include <ios>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "odb/db.h"
#include "tst/IntegratedFixture.h"

namespace odb {
namespace {

class TestWriteReadDbHier : public tst::IntegratedFixture
{
 protected:
  TestWriteReadDbHier()
      : tst::IntegratedFixture(tst::IntegratedFixture::Technology::Nangate45,
                               "_main/src/odb/test/")
  {
  }

  void SetUp() override
  {
    // IntegratedFixture handles library loading and basic setup.
    odb::dbChip* chip = odb::dbChip::create(db_.get(), db_->getTech());
    block_ = odb::dbBlock::create(chip, "top");
    sta_->postReadDef(block_);
  }

  void SetUpTmpPath(const std::string& name)
  {
    std::filesystem::create_directory("results");
    tmp_path_ = "results/" + name;
  }

  // Writes a temporal DB and then tries to read the contents back to check if
  // the serialization is working as expected
  dbDatabase* writeReadDb()
  {
    writeDb();
    return readDb();
  }

  void writeDb()
  {
    std::ofstream write;
    write.exceptions(std::ifstream::failbit | std::ifstream::badbit
                     | std::ios::eofbit);
    write.open(tmp_path_, std::ios::binary);
    db_->write(write);
  }

  dbDatabase* readDb()
  {
    dbDatabase* db = dbDatabase::create();
    std::ifstream read;
    read.exceptions(std::ifstream::failbit | std::ifstream::badbit
                    | std::ios::eofbit);
    read.open(tmp_path_, std::ios::binary);
    db->read(read);
    return db;
  }

  dbBlock* block_;
  std::string tmp_path_;
  std::vector<dbInst*> instances_;
};

// Construct hierarchical netlist and write/read it back.
// The hierarchical netlist should be read back successfully.
TEST_F(TestWriteReadDbHier, WriteReadOdb)
{
  SetUpTmpPath("WriteReadOdb");
  db_->setHierarchy(true);

  // Create masters
  dbMaster* buf_master = db_->findMaster("BUF_X1");
  ASSERT_TRUE(buf_master);

  // Create modules & module instances
  dbModule* mod0 = dbModule::create(block_, "MOD0");
  ASSERT_TRUE(mod0);
  dbModInst* mi0 = dbModInst::create(block_->getTopModule(), mod0, "mi0");
  ASSERT_TRUE(mi0);

  dbModule* mod1 = dbModule::create(block_, "MOD1");
  ASSERT_TRUE(mod1);
  dbModInst* mi1 = dbModInst::create(mod0, mod1, "mi1");
  ASSERT_TRUE(mi1);

  dbModBTerm::create(mod1, "A");
  dbModBTerm::create(mod0, "A");

  // Create instances
  dbInst* drvr_inst = dbInst::create(block_, buf_master, "drvr_inst");
  ASSERT_TRUE(drvr_inst);

  dbInst* load0_inst = dbInst::create(block_, buf_master, "load0_inst");
  ASSERT_TRUE(load0_inst);

  dbInst* load2_inst = dbInst::create(block_, buf_master, "load2_inst");
  ASSERT_TRUE(load2_inst);

  dbInst* load1_inst
      = dbInst::create(block_, buf_master, "mi0/mi1/load1_inst", false, mod1);
  ASSERT_TRUE(load1_inst);

  // Create nets and connect pins
  dbNet* net = dbNet::create(block_, "net");
  ASSERT_TRUE(net);

  dbNet* in_net = dbNet::create(block_, "in");
  ASSERT_TRUE(in_net);
  dbBTerm* in_bterm = dbBTerm::create(in_net, "in");
  ASSERT_TRUE(in_bterm);
  in_bterm->setIoType(dbIoType::INPUT);
  in_bterm->connect(in_net);

  dbITerm* drvr_a = drvr_inst->findITerm("A");
  ASSERT_TRUE(drvr_a);
  drvr_a->connect(in_net);

  dbITerm* drvr_z = drvr_inst->findITerm("Z");
  ASSERT_TRUE(drvr_z);
  drvr_z->connect(net);

  dbITerm* load0_a = load0_inst->findITerm("A");
  ASSERT_TRUE(load0_a);
  load0_a->connect(net);

  dbITerm* load2_a = load2_inst->findITerm("A");
  ASSERT_TRUE(load2_a);
  load2_a->connect(net);

  // Hierarchical connections
  // Inside MOD1
  dbModNet* mod1_net_a = dbModNet::create(mod1, "A");
  ASSERT_TRUE(mod1_net_a);
  dbITerm* load1_a = load1_inst->findITerm("A");
  ASSERT_TRUE(load1_a);
  mod1->findModBTerm("A")->connect(mod1_net_a);
  load1_a->connect(net, mod1_net_a);

  // Inside MOD0
  dbModNet* mod0_net_a = dbModNet::create(mod0, "A");
  ASSERT_TRUE(mod0_net_a);
  dbModITerm::create(mi1, "A", mod1->findModBTerm("A"));
  mi1->findModITerm("A")->connect(mod0_net_a);
  mod0->findModBTerm("A")->connect(mod0_net_a);

  // Connect top-level net to hierarchical instance through a modnet
  dbModNet* top_mod_net = dbModNet::create(block_->getTopModule(), "net");
  ASSERT_TRUE(top_mod_net);
  drvr_z->connect(top_mod_net);
  load0_a->connect(top_mod_net);
  load2_a->connect(top_mod_net);

  // Create ModITerm for mi0
  dbModITerm::create(mi0, "A", mod0->findModBTerm("A"));
  mi0->findModITerm("A")->connect(top_mod_net);

  //--------------------------------------------------------------
  // Write ODB and read back
  //--------------------------------------------------------------
  dbDatabase* db2 = writeReadDb();
  ASSERT_TRUE(db2->hasHierarchy());
  dbBlock* block2 = db2->getChip()->getBlock();

  dbInst* db2_load1_inst = block2->findInst("mi0/mi1/load1_inst");
  ASSERT_NE(db2_load1_inst, nullptr);
  dbModInst* db2_mi0 = block2->findModInst("mi0");
  ASSERT_NE(db2_mi0, nullptr);
  dbModInst* db2_mi1 = block2->findModInst("mi0/mi1");
  ASSERT_NE(db2_mi1, nullptr);
}

// Construct hierarchical netlist with multiple blocks and write/read it back.
// The hierarchical netlist should be read back successfully.
TEST_F(TestWriteReadDbHier, WriteReadOdbMultiBlocks)
{
  SetUpTmpPath("WriteReadOdbMultiBlocks");
  db_->setHierarchy(true);

  // Create masters
  dbMaster* buf_master = db_->findMaster("BUF_X1");
  ASSERT_TRUE(buf_master);

  // Create child block
  dbBlock* child_block = dbBlock::create(block_, "CHILD_BLOCK");
  ASSERT_TRUE(child_block);

  // Create module in child block
  dbModule* mod0 = dbModule::create(child_block, "MOD0");
  ASSERT_TRUE(mod0);

  dbModBTerm* mod0_a = dbModBTerm::create(mod0, "A");
  ASSERT_TRUE(mod0_a);

  // Create instance in child block
  dbInst* child_inst
      = dbInst::create(child_block, buf_master, "child_inst", false, mod0);
  ASSERT_TRUE(child_inst);

  //--------------------------------------------------------------
  // Write ODB and read back
  //--------------------------------------------------------------
  dbDatabase* db2 = writeReadDb();
  ASSERT_TRUE(db2->hasHierarchy());
  dbBlock* top_block2 = db2->getChip()->getBlock();

  // Find child block
  dbBlock* child_block2 = top_block2->findChild("CHILD_BLOCK");
  ASSERT_NE(child_block2, nullptr);

  // Find module in child block
  dbModule* mod0_2 = child_block2->findModule("MOD0");
  ASSERT_NE(mod0_2, nullptr);

  // Verify ModBTerm hash is populated
  dbModBTerm* mod0_a_2 = mod0_2->findModBTerm("A");
  ASSERT_NE(mod0_a_2, nullptr);
  EXPECT_STREQ(mod0_a_2->getName(), "A");

  // Verify Inst hash is populated
  dbInst* child_inst_2 = mod0_2->findDbInst("child_inst");
  ASSERT_NE(child_inst_2, nullptr);
  EXPECT_EQ(child_inst_2->getName(), "child_inst");
}

}  // namespace
}  // namespace odb
