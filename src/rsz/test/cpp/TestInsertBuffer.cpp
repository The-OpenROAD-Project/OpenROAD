// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include <set>
#include <string>
#include <vector>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbObject.h"
#include "tst/IntegratedFixture.h"
#include "utl/Logger.h"

namespace odb {

class TestInsertBuffer : public tst::IntegratedFixture
{
 public:
  TestInsertBuffer()
      : tst::IntegratedFixture(tst::IntegratedFixture::Technology::Nangate45,
                               "_main/src/rsz/test/")
  {
    if (debug_) {
      logger_.setDebugLevel(utl::ODB, "DB_EDIT", 3);
      logger_.setDebugLevel(utl::ODB, "insert_buffer", 3);
    }
  }

 protected:
  void SetUp() override
  {
    // IntegratedFixture handles library loading and basic setup.
    odb::dbChip* chip = odb::dbChip::create(db_.get(), db_->getTech());
    block_ = odb::dbBlock::create(chip, "top");
    sta_->postReadDef(block_);
  }

  bool debug_ = false;  // Set to true to generate debug output
};

TEST_F(TestInsertBuffer, AfterDriver_Case1)
{
  // Get the test name dynamically from the gtest framework.
  const auto* test_info = testing::UnitTest::GetInstance()->current_test_info();
  const std::string test_name
      = std::string(test_info->test_suite_name()) + "_" + test_info->name();

  int num_warning = 0;
  dbMaster* buffer_master = db_->findMaster("BUF_X4");
  ASSERT_TRUE(buffer_master);

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
      = dbInst::create(block_, buf_master, "load1_inst", false, mod1);
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
  num_warning = db_network_->checkAxioms();
  num_warning += sta_->checkSanity();
  EXPECT_EQ(num_warning, 0);

  // Write verilog and check the content
  writeAndCompareVerilogOutputFile(test_name, test_name + "_pre.v");

  //-----------------------------------------------------------------
  // Insert buffer
  //-----------------------------------------------------------------
  buffer_master = db_->findMaster("BUF_X4");
  ASSERT_TRUE(buffer_master);
  dbInst* new_buffer = net->insertBufferAfterDriver(drvr_z, buffer_master);
  ASSERT_TRUE(new_buffer);
  num_warning = db_network_->checkAxioms();
  num_warning += sta_->checkSanity();
  EXPECT_EQ(num_warning, 0);

  // Verify connections
  EXPECT_EQ(drvr_z->getNet()->getName(), std::string("net1"));
  EXPECT_EQ(new_buffer->findITerm("A")->getNet(), drvr_z->getNet());
  EXPECT_EQ(new_buffer->findITerm("Z")->getNet(), net);

  // Write verilog and check the content
  writeAndCompareVerilogOutputFile(test_name, test_name + "_post.v");
}

TEST_F(TestInsertBuffer, AfterDriver_Case2)
{
  // Get the test name dynamically from the gtest framework.
  const auto* test_info = testing::UnitTest::GetInstance()->current_test_info();
  const std::string test_name
      = std::string(test_info->test_suite_name()) + "_" + test_info->name();

  int num_warning = 0;
  dbMaster* buffer_master = db_->findMaster("BUF_X4");
  ASSERT_TRUE(buffer_master);

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

  dbModBTerm::create(mod0, "A");
  dbModBTerm::create(mod1, "A");

  // Create instances
  dbInst* load0_inst = dbInst::create(block_, buf_master, "load0_inst");
  ASSERT_TRUE(load0_inst);

  dbInst* load2_inst = dbInst::create(block_, buf_master, "load2_inst");
  ASSERT_TRUE(load2_inst);

  dbInst* load1_inst
      = dbInst::create(block_, buf_master, "load1_inst", false, mod1);
  ASSERT_TRUE(load1_inst);

  // Create nets and connect pins
  dbNet* net = dbNet::create(block_, "X");
  ASSERT_TRUE(net);

  dbBTerm* drvr_bterm = dbBTerm::create(net, "X");
  ASSERT_TRUE(drvr_bterm);
  drvr_bterm->setIoType(dbIoType::INPUT);
  drvr_bterm->connect(net);

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
  dbModNet* top_mod_net = dbModNet::create(block_->getTopModule(), "X");
  ASSERT_TRUE(top_mod_net);
  drvr_bterm->connect(top_mod_net);
  load0_a->connect(top_mod_net);
  load2_a->connect(top_mod_net);
  // Create ModITerm for mi0
  dbModITerm::create(mi0, "A", mod0->findModBTerm("A"));
  mi0->findModITerm("A")->connect(top_mod_net);
  num_warning = db_network_->checkAxioms();
  num_warning += sta_->checkSanity();
  EXPECT_EQ(num_warning, 0);

  buffer_master = db_->findMaster("BUF_X4");
  ASSERT_TRUE(buffer_master);

  // Write verilog and check the content
  writeAndCompareVerilogOutputFile(test_name, test_name + "_pre.v");

  //-----------------------------------------------------------------
  // Insert buffer
  //-----------------------------------------------------------------
  dbInst* new_buffer = net->insertBufferAfterDriver(drvr_bterm, buffer_master);
  ASSERT_TRUE(new_buffer);
  num_warning = db_network_->checkAxioms();
  num_warning += sta_->checkSanity();
  EXPECT_EQ(num_warning, 0);

  // Verify connections
  EXPECT_EQ(drvr_bterm->getNet()->getName(), std::string("X"));
  EXPECT_EQ(new_buffer->findITerm("A")->getNet(), drvr_bterm->getNet());
  EXPECT_EQ(new_buffer->findITerm("Z")->getNet()->getName(),
            std::string("net1"));

  // Write verilog and check the content
  writeAndCompareVerilogOutputFile(test_name, test_name + "_post.v");
}

TEST_F(TestInsertBuffer, AfterDriver_Case3)
{
  // Get the test name dynamically from the gtest framework.
  const auto* test_info = testing::UnitTest::GetInstance()->current_test_info();
  const std::string test_name
      = std::string(test_info->test_suite_name()) + "_" + test_info->name();

  int num_warning = 0;
  readVerilogAndSetup(test_name + "_pre.v");

  // Get ODB objects
  dbITerm* drvr_iterm = block_->findITerm("h0/drvr/Z");
  ASSERT_TRUE(drvr_iterm);
  dbNet* net = drvr_iterm->getNet();
  ASSERT_TRUE(net);

  dbMaster* buffer_master = db_->findMaster("BUF_X4");
  ASSERT_TRUE(buffer_master);

  // Pre sanity check
  sta_->updateTiming(true);
  num_warning = db_network_->checkAxioms();
  num_warning += sta_->checkSanity();
  EXPECT_EQ(num_warning, 0);  // 'n1' is dangling

  //-----------------------------------------------------------------
  // Insert buffer
  //-----------------------------------------------------------------
  dbInst* new_buffer = net->insertBufferAfterDriver(drvr_iterm, buffer_master);
  ASSERT_TRUE(new_buffer);

  // Post sanity check - this was failing with ORD-2030 before the fix
  num_warning = db_network_->checkAxioms();
  num_warning += sta_->checkSanity();
  EXPECT_EQ(num_warning, 0);

  // Write verilog and check the content
  writeAndCompareVerilogOutputFile(test_name, test_name + "_post.v");
}

//
// insertBufferBeforeLoad() Case1
//
// This test case constructs a hierarchical netlist. The top-level module
// contains a constant driver (drvr_inst), two buffer loads (load0_inst,
// load2_inst), a top-level output port (load_output), and a hierarchical
// instance (mi0).
//
// The single top-level net "net" connects the driver to all these loads.
// The connection to mi0 propagates down through the hierarchy (mi0 -> mi1)
// to eventually drive another buffer load (load1_inst) inside the MOD1 module.
//
// [Pre ECO]
//
//                      +-----------+
//                      | LOGIC0_X1 |
//                      | drvr_inst |-----.
//                      +-----------+     |
//                            .Z          | (net "net")
//      +-------------------+-------------+------------------+
//      |                   |             |                  |
//      | .A                | .A          |                  |
// +----v------+       +----v------+      |             +----v-------+
// |  BUF_X1   |       |  BUF_X1   |      |             | Top Output |
// | load0_inst|       | load2_inst|      |             | load_output|
// +-----------+       +-----------+      |             +------------+
//                                        | .A
//                           +------------v----------------------+
//                           | MOD0 mi0   |                      |
//                           |            | .A                   |
//                           |   +--------v------------------+   |
//                           |   | MOD1   |                  |   |
//                           |   | mi1    | .A               |   |
//                           |   |    +---v---------------+  |   |
//                           |   |    |  BUF_X1           |  |   |
//                           |   |    | mi0/mi1/load1_inst|  |   |
//                           |   |    +-------------------+  |   |
//
// The test then proceeds to insert buffers one by one before each load:
// 1. Before `load0_inst` (in the top module).
// 2. Before `load1_inst` (inside the `mi0/mi1` hierarchy).
// 3. Before `load2_inst` (in the top module).
// 4. Before the top-level port `load_output`.
//
// After each insertion, it verifies the correctness of the resulting netlist
// by writing it out to a Verilog file and comparing it against an expected
// output.
//
// [Post ECO]
//                      +-----------+
//                      | LOGIC0_X1 |
//                      | drvr_inst |------.
//                      +-----------+      |
//                            .Z           | (net "net")
//      +---------------------+------------+-----------------+
// (NEW)| .A           (NEW)  | .A         | .A       (NEW)  | .A
// +----v------+         +----v------+     |            +----v------+
// |  BUF_X4   |         |  BUF_X4   |     |            |  BUF_X4   |
// |    buf    |         |   buf_1   |     |            |   buf_2   |
// +-----------+         +-----------+     |            +-----------+
//      | .Z                  | .Z         |                 | .Z
//      | (net_load)          |            |                 | (load_output)
//      |                     |(net_load_1)|                 |
// +----v------+         +----v------+     |          +----v-------+
// |  BUF_X1   |         |  BUF_X1   |     |          | Top Output |
// | load0_inst|         | load2_inst|     |          | load_output|
// +-----------+         +-----------+     |          +------------+
//                                         |
//                                         | .A
//                            +------------v----------------------+
//                            | MOD0 mi0   |                      |
//                            |            | .A                   |
//                            |   +--------v------------------+   |
//                            |   | MOD1   |                  |   |
//                            |   | mi1    | .A   (NEW)       |   |
//                            |   |    +---v---------------+  |   |
//                            |   |    |  BUF_X4           |  |   |
//                            |   |    | mi0/mi1/buf       |  |   |
//                            |   |    +----------+--------+  |   |
//                            |   |        |                  |   |
//                            |   |    +---v---------------+  |   |
//                            |   |    |  BUF_X1           |  |   |
//                            |   |    | mi0/mi1/load1_inst|  |   |
//                            |   |    +-------------------+  |   |
//
TEST_F(TestInsertBuffer, BeforeLoad_Case1)
{
  // Get the test name dynamically from the gtest framework.
  const auto* test_info = testing::UnitTest::GetInstance()->current_test_info();
  const std::string test_name
      = std::string(test_info->test_suite_name()) + "_" + test_info->name();

  int num_warning = 0;
  dbMaster* buffer_master = db_->findMaster("BUF_X4");
  ASSERT_TRUE(buffer_master);

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

  // TIELO master cell for Nangate45
  dbMaster* tielo_master = db_->findMaster("LOGIC0_X1");

  // Create instances, changing drvr_inst's master to TIELO
  dbInst* drvr_inst = dbInst::create(block_, tielo_master, "drvr_inst");
  ASSERT_TRUE(drvr_inst);

  dbInst* load0_inst = dbInst::create(block_, buf_master, "load0_inst");
  ASSERT_TRUE(load0_inst);

  dbInst* load2_inst = dbInst::create(block_, buf_master, "load2_inst");
  ASSERT_TRUE(load2_inst);

  dbInst* load1_inst
      = dbInst::create(block_, buf_master, "load1_inst", false, mod1);
  ASSERT_TRUE(load1_inst);

  // Create nets and connect pins
  dbNet* net = dbNet::create(block_, "net");
  ASSERT_TRUE(net);

  dbModNet* top_mod_net = dbModNet::create(block_->getTopModule(), "net");
  ASSERT_TRUE(top_mod_net);

  dbITerm* drvr_z = drvr_inst->findITerm("Z");
  ASSERT_TRUE(drvr_z);
  drvr_z->connect(net, top_mod_net);

  dbITerm* load0_a = load0_inst->findITerm("A");
  ASSERT_TRUE(load0_a);
  load0_a->connect(net, top_mod_net);

  dbITerm* load2_a = load2_inst->findITerm("A");
  ASSERT_TRUE(load2_a);
  load2_a->connect(net, top_mod_net);

  dbBTerm* load_output_bterm = dbBTerm::create(net, "load_output");
  ASSERT_TRUE(load_output_bterm);
  load_output_bterm->setIoType(dbIoType::OUTPUT);
  load_output_bterm->connect(net, top_mod_net);

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

  // Create ModITerm for mi0
  dbModITerm::create(mi0, "A", mod0->findModBTerm("A"));
  mi0->findModITerm("A")->connect(top_mod_net);
  num_warning = db_network_->checkAxioms();
  num_warning += sta_->checkSanity();
  EXPECT_EQ(num_warning, 0);

  // Write verilog and check the content
  writeAndCompareVerilogOutputFile(test_name, test_name + "_pre.v");

  //-----------------------------------------------------------------
  // Insert buffer #1
  //-----------------------------------------------------------------
  buffer_master = db_->findMaster("BUF_X4");
  ASSERT_TRUE(buffer_master);
  dbInst* new_buffer1 = net->insertBufferBeforeLoad(load0_a, buffer_master);
  ASSERT_TRUE(new_buffer1);
  num_warning = db_network_->checkAxioms();
  num_warning += sta_->checkSanity();
  EXPECT_EQ(num_warning, 0);

  // Verify connections
  EXPECT_EQ(new_buffer1->findITerm("A")->getNet(), net);
  EXPECT_EQ(new_buffer1->findITerm("Z")->getNet(), load0_a->getNet());

  // Write verilog and check the content
  writeAndCompareVerilogOutputFile(test_name, test_name + "_step1.v");

  //-----------------------------------------------------------------
  // Insert buffer #2
  //-----------------------------------------------------------------
  dbInst* new_buffer2 = net->insertBufferBeforeLoad(load1_a, buffer_master);
  ASSERT_TRUE(new_buffer2);
  num_warning = db_network_->checkAxioms();
  num_warning += sta_->checkSanity();
  EXPECT_EQ(num_warning, 0);

  // Verify connections for buffer #2
  EXPECT_EQ(new_buffer2->findITerm("A")->getNet(), net);
  EXPECT_EQ(new_buffer2->findITerm("Z")->getNet(), load1_a->getNet());

  // Write verilog and check the content after inserting buffer #2
  writeAndCompareVerilogOutputFile(test_name, test_name + "_step2.v");

  //-----------------------------------------------------------------
  // Insert buffer #3
  //-----------------------------------------------------------------
  dbInst* new_buffer3 = net->insertBufferBeforeLoad(load2_a, buffer_master);
  ASSERT_TRUE(new_buffer3);
  num_warning = db_network_->checkAxioms();
  num_warning += sta_->checkSanity();
  EXPECT_EQ(num_warning, 0);

  // Verify connections for buffer #3
  EXPECT_EQ(load2_a->getNet()->getName(), std::string("net3"));
  EXPECT_EQ(new_buffer3->findITerm("A")->getNet(), net);
  EXPECT_EQ(new_buffer3->findITerm("Z")->getNet(), load2_a->getNet());

  // Write verilog and check the content after inserting buffer #3
  writeAndCompareVerilogOutputFile(test_name, test_name + "_step3.v");

  //-----------------------------------------------------------------
  // Insert buffer #4
  //-----------------------------------------------------------------
  dbInst* new_buffer4
      = net->insertBufferBeforeLoad(load_output_bterm, buffer_master);
  ASSERT_TRUE(new_buffer4);
  num_warning = db_network_->checkAxioms();
  num_warning += sta_->checkSanity();
  EXPECT_EQ(num_warning, 0);

  // Verify connections for buffer #4
  EXPECT_EQ(new_buffer4->findITerm("A")->getNet(), net);
  EXPECT_EQ(new_buffer4->findITerm("Z")->getNet(), load_output_bterm->getNet());

  // Write verilog and check the content after inserting buffer #4
  writeAndCompareVerilogOutputFile(test_name, test_name + "_post.v");
}

// Netlist:
//   drvr (BUF)  --> n1 --> buf0 (BUF) --> n2 --> load (BUF)
//
// Load pins for insertion = {buf0/A}
// Expected result after insertion:
//   drvr (BUF) --> n1 --> buf_new --> net --> buf0 --> n2 --> load (BUF)
TEST_F(TestInsertBuffer, BeforeLoads_Case1)
{
  // Get the test name dynamically from the gtest framework.
  const auto* test_info = testing::UnitTest::GetInstance()->current_test_info();
  const std::string test_name
      = std::string(test_info->test_suite_name()) + "_" + test_info->name();

  // Create masters
  dbMaster* buf_master = db_->findMaster("BUF_X1");
  ASSERT_TRUE(buf_master);
  dbMaster* buffer_master = db_->findMaster("BUF_X4");
  ASSERT_TRUE(buffer_master);

  // Create instances
  dbInst* drvr_inst = dbInst::create(block_, buf_master, "drvr");
  ASSERT_TRUE(drvr_inst);
  dbInst* buf0_inst = dbInst::create(block_, buf_master, "buf0");
  ASSERT_TRUE(buf0_inst);
  dbInst* load_inst = dbInst::create(block_, buf_master, "load");
  ASSERT_TRUE(load_inst);

  // Create nets and connect
  dbNet* n1 = dbNet::create(block_, "n1");
  ASSERT_TRUE(n1);
  dbNet* n2 = dbNet::create(block_, "n2");
  ASSERT_TRUE(n2);

  drvr_inst->findITerm("Z")->connect(n1);
  buf0_inst->findITerm("A")->connect(n1);
  buf0_inst->findITerm("Z")->connect(n2);
  load_inst->findITerm("A")->connect(n2);

  // Find load pin for buffer insertion
  dbITerm* buf0_a = buf0_inst->findITerm("A");
  std::set<dbObject*> loads;
  loads.insert(buf0_a);

  // Insert buffer
  dbInst* new_buf = n1->insertBufferBeforeLoads(loads, buffer_master);
  ASSERT_TRUE(new_buf);

  // Write verilog and check the content
  writeAndCompareVerilogOutputFile(test_name, test_name + "_post.v");
}

// Netlist:
//   in (Port) --> n1 --> buf0 (BUF) --> n2 --> out (Port)
//
// Insert buffer 1: Load pins = {buf0/A}
// Insert buffer 2: Load pins = {out (Port)}
//
// Expected result after buffer 1 (before buf0/A):
//   in (Port) --> [buf1] --> buf0 -->  out (Port)
//
// Expected result after buffer 2 (before out Port):
//   in (Port) --> [buf1] --> buf0 --> [buf2] --> out (Port)
TEST_F(TestInsertBuffer, BeforeLoads_Case2)
{
  // Get the test name dynamically from the gtest framework.
  const auto* test_info = testing::UnitTest::GetInstance()->current_test_info();
  const std::string test_name
      = std::string(test_info->test_suite_name()) + "_" + test_info->name();

  // Create masters
  dbMaster* buf_master = db_->findMaster("BUF_X1");
  ASSERT_TRUE(buf_master);
  dbMaster* buffer_master = db_->findMaster("BUF_X4");
  ASSERT_TRUE(buffer_master);

  // Create instances
  dbInst* buf0_inst = dbInst::create(block_, buf_master, "buf0");
  ASSERT_TRUE(buf0_inst);

  // Create nets and ports
  dbNet* n1 = dbNet::create(block_, "in");
  ASSERT_TRUE(n1);
  dbNet* n2 = dbNet::create(block_, "out");
  ASSERT_TRUE(n2);
  dbBTerm* in_port = dbBTerm::create(n1, "in");
  ASSERT_TRUE(in_port);
  in_port->setIoType(dbIoType::INPUT);
  dbBTerm* out_port = dbBTerm::create(n2, "out");
  ASSERT_TRUE(out_port);
  out_port->setIoType(dbIoType::OUTPUT);

  // Connect
  buf0_inst->findITerm("A")->connect(n1);
  buf0_inst->findITerm("Z")->connect(n2);

  // Insert buffer 1
  dbITerm* buf0_a = buf0_inst->findITerm("A");
  std::set<dbObject*> loads1;
  loads1.insert(buf0_a);
  dbInst* new_buf1 = n1->insertBufferBeforeLoads(loads1, buffer_master);
  ASSERT_TRUE(new_buf1);

  // Insert buffer 2
  std::set<dbObject*> loads2;
  loads2.insert(out_port);
  dbInst* new_buf2 = n2->insertBufferBeforeLoads(loads2, buffer_master);
  ASSERT_TRUE(new_buf2);

  // Write verilog and check the content
  writeAndCompareVerilogOutputFile(test_name, test_name + "_post.v");
}

// Netlist:
//   in (Port) --> n1 --> mod0/buf0 (BUF) --> n2 --> out (Port)
//   where mod0 is an instance of submodule MOD0
//
// Insert buffer 1: Load pins = {mod0/buf0/A}
// Insert buffer 2: Load pins = {out (Port)}
//
// Expected result after buffer 1 (before mod0/buf0/A):
//   in --> n1 --> [new_buf1] --> mod0/buf0 --> out
//
// Expected result after buffer 2 (before out Port):
//   in --> n1 --> [new_buf1] --> mod0/buf0 --> [new_buf2] --> out
TEST_F(TestInsertBuffer, BeforeLoads_Case3)
{
  // Get the test name dynamically from the gtest framework.
  const auto* test_info = testing::UnitTest::GetInstance()->current_test_info();
  const std::string test_name
      = std::string(test_info->test_suite_name()) + "_" + test_info->name();

  // Create masters
  dbMaster* buf_master = db_->findMaster("BUF_X1");
  ASSERT_TRUE(buf_master);
  dbMaster* buffer_master = db_->findMaster("BUF_X4");
  ASSERT_TRUE(buffer_master);

  // Create submodule MOD0
  dbModule* mod0_module = dbModule::create(block_, "MOD0");
  ASSERT_TRUE(mod0_module);
  dbModInst* mod0_inst
      = dbModInst::create(block_->getTopModule(), mod0_module, "mod0");
  ASSERT_TRUE(mod0_inst);

  dbInst* buf0_inst
      = dbInst::create(block_, buf_master, "buf0", false, mod0_module);
  ASSERT_TRUE(buf0_inst);
  dbModBTerm* mod0_in = dbModBTerm::create(mod0_module, "in");
  mod0_in->setIoType(dbIoType::INPUT);
  dbModBTerm* mod0_out = dbModBTerm::create(mod0_module, "out");
  mod0_out->setIoType(dbIoType::OUTPUT);

  // Connect inside MOD0
  dbModNet* mod_n1 = dbModNet::create(mod0_module, "n1");
  mod0_in->connect(mod_n1);
  buf0_inst->findITerm("A")->connect(mod_n1);

  dbModNet* mod_n2 = dbModNet::create(mod0_module, "n2");
  buf0_inst->findITerm("Z")->connect(mod_n2);
  mod0_out->connect(mod_n2);

  // Create top-level logic
  dbNet* n1 = dbNet::create(block_, "n1");
  ASSERT_TRUE(n1);
  dbBTerm* in_port = dbBTerm::create(n1, "in");
  ASSERT_TRUE(in_port);
  in_port->setIoType(dbIoType::INPUT);

  dbNet* n2 = dbNet::create(block_, "n2");
  ASSERT_TRUE(n2);
  dbBTerm* out_port = dbBTerm::create(n2, "out");
  ASSERT_TRUE(out_port);
  out_port->setIoType(dbIoType::OUTPUT);

  // Make hierarchical connections
  dbModNet* top_n1 = dbModNet::create(block_->getTopModule(), "n1");
  in_port->connect(top_n1);
  dbModITerm* mod0_in_iterm = dbModITerm::create(mod0_inst, "in", mod0_in);
  mod0_in_iterm->connect(top_n1);

  dbModNet* top_n2 = dbModNet::create(block_->getTopModule(), "n2");
  out_port->connect(top_n2);
  dbModITerm* mod0_out_iterm = dbModITerm::create(mod0_inst, "out", mod0_out);
  mod0_out_iterm->connect(top_n2);

  // Physical connection for flat nets
  mod0_module->findDbInst("buf0")->findITerm("A")->connect(n1);
  mod0_module->findDbInst("buf0")->findITerm("Z")->connect(n2);

  // Insert buffer 1
  dbITerm* buf0_a = mod0_module->findDbInst("buf0")->findITerm("A");
  ASSERT_TRUE(buf0_a);
  std::set<dbObject*> loads1;
  loads1.insert(buf0_a);
  dbInst* new_buf1 = buf0_a->getNet()->insertBufferBeforeLoads(
      loads1, buffer_master, nullptr, "new_buf1");
  ASSERT_TRUE(new_buf1);

  // Insert buffer 2
  std::set<dbObject*> loads2;
  loads2.insert(out_port);
  dbInst* new_buf2 = out_port->getNet()->insertBufferBeforeLoads(
      loads2, buffer_master, nullptr, "new_buf2");
  ASSERT_TRUE(new_buf2);

  // Write verilog and check the content
  writeAndCompareVerilogOutputFile(test_name, test_name + "_post.v");
}

// Netlist:
//   drvr0 (BUF) --> n1 --+--> load0 (BUF)
//                        +--> load1 (BUF)
//                        +--> load2 (BUF)
//
//   No hierarchical modules in this netlist.
//
// Insert buffer 1: Load pins = {load0/A, load1/A}
// Insert buffer 2: Load pins = {new0/A, load2/A}
//
// Expected result after buffer 1:
//   drvr0 --> n1 --+--> load2
//                  |
//                  +--> [new0] --(net_A)--+--> load0
//                                         +--> load1
//
// Expected result after buffer 2 (Buffers both load2 and new0):
//   drvr0 --> n1 --> [new1] --(net_B)--+--> load2
//                                      |                    +--> load0
//                                      +--> new0 --(net_A)--+
//                                                           +--> load1
TEST_F(TestInsertBuffer, BeforeLoads_Case4)
{
  // Get the test name dynamically from the gtest framework.
  const auto* test_info = testing::UnitTest::GetInstance()->current_test_info();
  const std::string test_name
      = std::string(test_info->test_suite_name()) + "_" + test_info->name();

  // Create masters
  dbMaster* buf_master = db_->findMaster("BUF_X1");
  ASSERT_TRUE(buf_master);
  dbMaster* buffer_master = db_->findMaster("BUF_X4");
  ASSERT_TRUE(buffer_master);

  // Create instances
  dbInst* drvr0 = dbInst::create(block_, buf_master, "drvr0");
  ASSERT_TRUE(drvr0);
  dbInst* load0 = dbInst::create(block_, buf_master, "load0");
  ASSERT_TRUE(load0);
  dbInst* load1 = dbInst::create(block_, buf_master, "load1");
  ASSERT_TRUE(load1);
  dbInst* load2 = dbInst::create(block_, buf_master, "load2");
  ASSERT_TRUE(load2);

  // Create net and connect
  dbNet* n1 = dbNet::create(block_, "n1");
  ASSERT_TRUE(n1);
  drvr0->findITerm("Z")->connect(n1);
  load0->findITerm("A")->connect(n1);
  load1->findITerm("A")->connect(n1);
  load2->findITerm("A")->connect(n1);

  // Insert buffer 1
  dbITerm* load0_a = load0->findITerm("A");
  dbITerm* load1_a = load1->findITerm("A");
  std::set<dbObject*> loads1;
  loads1.insert(load0_a);
  loads1.insert(load1_a);
  dbInst* new0
      = n1->insertBufferBeforeLoads(loads1, buffer_master, nullptr, "new0");
  ASSERT_TRUE(new0);

  // Insert buffer 2
  dbITerm* new0_a = new0->findITerm("A");
  dbITerm* load2_a = load2->findITerm("A");
  std::set<dbObject*> loads2;
  loads2.insert(new0_a);
  loads2.insert(load2_a);
  dbInst* new1
      = n1->insertBufferBeforeLoads(loads2, buffer_master, nullptr, "new1");
  ASSERT_TRUE(new1);

  // Write verilog and check the content
  writeAndCompareVerilogOutputFile(test_name, test_name + "_post.v");
}

// Netlist:
//   Top module contains four submodules: MOD0, MOD1, MOD2, MOD3.
//   There are four cells (drvr0, load0, load1, load2) in total.
//   drvr0 drives load0, load1, and load2, with each cell located
//   inside a different MOD* submodule (mod0/drvr0, mod1/load0,
//   mod2/load1, mod3/load2).
//
//   Hierarchy:
//     top
//     +-- mod0 (MOD0) --> drvr0 (BUF)
//     +-- mod1 (MOD1) --> load0 (BUF)
//     +-- mod2 (MOD2) --> load1 (BUF)
//     +-- mod3 (MOD3) --> load2 (BUF)
//
//   Connections:
//     mod0/drvr0/Z --> n1
//     n1 --> mod1/load0/A
//     n1 --> mod2/load1/A
//     n1 --> mod3/load2/A
//
// Insert buffer 1: Load pins = {mod1/load0/A, mod2/load1/A}
// Insert buffer 2: Load pins = {new0/A, mod3/load2/A}
TEST_F(TestInsertBuffer, BeforeLoads_Case5)
{
  // Get the test name dynamically from the gtest framework.
  const auto* test_info = testing::UnitTest::GetInstance()->current_test_info();
  const std::string test_name
      = std::string(test_info->test_suite_name()) + "_" + test_info->name();

  // Create masters
  dbMaster* buf_master = db_->findMaster("BUF_X1");
  ASSERT_TRUE(buf_master);
  dbMaster* buffer_master = db_->findMaster("BUF_X4");
  ASSERT_TRUE(buffer_master);

  // Create submodules and instances within them
  dbModule* mod0 = dbModule::create(block_, "MOD0");
  dbModInst* mi0 = dbModInst::create(block_->getTopModule(), mod0, "mod0");
  dbInst::create(block_, buf_master, "drvr0", false, mod0);
  dbModBTerm::create(mod0, "Z")->setIoType(dbIoType::OUTPUT);
  mod0->findModBTerm("Z")->connect(dbModNet::create(mod0, "Z"));
  mod0->findDbInst("drvr0")->findITerm("Z")->connect(mod0->getModNet("Z"));

  dbModule* mod1 = dbModule::create(block_, "MOD1");
  dbModInst* mi1 = dbModInst::create(block_->getTopModule(), mod1, "mod1");
  dbInst::create(block_, buf_master, "load0", false, mod1);
  dbModBTerm::create(mod1, "A")->setIoType(dbIoType::INPUT);
  mod1->findModBTerm("A")->connect(dbModNet::create(mod1, "A"));
  mod1->findDbInst("load0")->findITerm("A")->connect(mod1->getModNet("A"));

  dbModule* mod2 = dbModule::create(block_, "MOD2");
  dbModInst* mi2 = dbModInst::create(block_->getTopModule(), mod2, "mod2");
  dbInst::create(block_, buf_master, "load1", false, mod2);
  dbModBTerm::create(mod2, "A")->setIoType(dbIoType::INPUT);
  mod2->findModBTerm("A")->connect(dbModNet::create(mod2, "A"));
  mod2->findDbInst("load1")->findITerm("A")->connect(mod2->getModNet("A"));

  dbModule* mod3 = dbModule::create(block_, "MOD3");
  dbModInst* mi3 = dbModInst::create(block_->getTopModule(), mod3, "mod3");
  dbInst::create(block_, buf_master, "load2", false, mod3);
  dbModBTerm::create(mod3, "A")->setIoType(dbIoType::INPUT);
  mod3->findModBTerm("A")->connect(dbModNet::create(mod3, "A"));
  mod3->findDbInst("load2")->findITerm("A")->connect(mod3->getModNet("A"));

  // Connect them hierarchically
  dbNet* n1 = dbNet::create(block_, "n1");
  ASSERT_TRUE(n1);

  dbModNet* top_n1 = dbModNet::create(block_->getTopModule(), "n1");
  dbModITerm::create(mi0, "Z", mod0->findModBTerm("Z"))->connect(top_n1);
  dbModITerm::create(mi1, "A", mod1->findModBTerm("A"))->connect(top_n1);
  dbModITerm::create(mi2, "A", mod2->findModBTerm("A"))->connect(top_n1);
  dbModITerm::create(mi3, "A", mod3->findModBTerm("A"))->connect(top_n1);

  // Physical connections
  mod0->findDbInst("drvr0")->findITerm("Z")->connect(n1);
  mod1->findDbInst("load0")->findITerm("A")->connect(n1);
  mod2->findDbInst("load1")->findITerm("A")->connect(n1);
  mod3->findDbInst("load2")->findITerm("A")->connect(n1);

  // Insert buffer 1
  dbITerm* load0_a = mod1->findDbInst("load0")->findITerm("A");
  ASSERT_TRUE(load0_a);
  dbITerm* load1_a = mod2->findDbInst("load1")->findITerm("A");
  ASSERT_TRUE(load1_a);

  std::set<dbObject*> loads1;
  loads1.insert(load0_a);
  loads1.insert(load1_a);
  dbInst* new0
      = n1->insertBufferBeforeLoads(loads1, buffer_master, nullptr, "new0");
  ASSERT_TRUE(new0);

  // Insert buffer 2
  dbITerm* new0_a = new0->findITerm("A");
  ASSERT_TRUE(new0_a);
  dbITerm* load2_a = mod3->findDbInst("load2")->findITerm("A");
  ASSERT_TRUE(load2_a);

  std::set<dbObject*> loads2;
  loads2.insert(new0_a);
  loads2.insert(load2_a);
  dbInst* new1
      = n1->insertBufferBeforeLoads(loads2, buffer_master, nullptr, "new1");
  ASSERT_TRUE(new1);

  // Write verilog and check the content
  writeAndCompareVerilogOutputFile(test_name, test_name + "_post.v");
}

// Netlist Structure (Reflecting the user provided tree):
//
// [Top Module]
//  |
//  +-- load0 (BUF)
//  |
//  +-- u_h2 (ModH2)
//  |    |
//  |    +-- u_h0 (ModH0) -> drvr (BUF) [Driver]
//  |    |
//  |    +-- u_h1 (ModH1) -> load5 (BUF) [TARGET]
//  |
//  +-- u_h3 (ModH3)
//  |    |
//  |    +-- load1 (BUF)
//  |    +-- load2 (BUF) [TARGET]
//  |
//  +-- u_h4 (ModH4)
//       |
//       +-- load4 (BUF)
//       +-- u_h5 (ModH5) -> load3 (BUF) [TARGET]
//
// Operation:
//   Insert buffer for targets: { load5, load2, load3 }
//   LCA: Top Module
//
// Expected Result:
//   1. New buffer 'new_buf' placed in Top.
//   2. Logical connections (Port Punching):
//      - To load5: Top -> u_h2 -> u_h1 (New Ports created)
//      - To load2: Top -> u_h3 (New Port created)
//      - To load3: Top -> u_h4 -> u_h5 (New Ports created)
//   3. Non-targets (load0, load1, load4) remain on original net.
TEST_F(TestInsertBuffer, BeforeLoads_Case6)
{
  // Get the test name dynamically from the gtest framework.
  const auto* test_info = testing::UnitTest::GetInstance()->current_test_info();
  const std::string test_name
      = std::string(test_info->test_suite_name()) + "_" + test_info->name();

  int num_warning = 0;
  readVerilogAndSetup(test_name + ".v");

  // Get ODB objects
  dbInst* drvr = block_->findInst("u_h2/u_h0/drvr");
  ASSERT_NE(drvr, nullptr);
  dbInst* load0 = block_->findInst("load0");
  ASSERT_NE(load0, nullptr);
  dbInst* load1 = block_->findInst("u_h3/load1");
  ASSERT_NE(load1, nullptr);
  dbInst* load2 = block_->findInst("u_h3/load2");
  ASSERT_NE(load2, nullptr);
  dbInst* load3 = block_->findInst("u_h4/u_h5/load3");
  ASSERT_NE(load3, nullptr);
  dbInst* load4 = block_->findInst("u_h4/load4");
  ASSERT_NE(load4, nullptr);
  dbInst* load5 = block_->findInst("u_h2/u_h1/load5");
  ASSERT_NE(load5, nullptr);
  dbITerm* load5_a = load5->findITerm("A");
  ASSERT_NE(load5_a, nullptr);
  dbITerm* load2_a = load2->findITerm("A");
  ASSERT_NE(load2_a, nullptr);
  dbITerm* load3_a = load3->findITerm("A");
  ASSERT_NE(load3_a, nullptr);
  dbNet* target_net = load5_a->getNet();
  ASSERT_NE(target_net, nullptr);
  dbMaster* buffer_master = db_->findMaster("BUF_X1");
  ASSERT_NE(buffer_master, nullptr);
  dbModInst* u_h0 = block_->findModInst("u_h2/u_h0");
  ASSERT_NE(u_h0, nullptr);
  dbModInst* u_h1 = block_->findModInst("u_h2/u_h1");
  ASSERT_NE(u_h1, nullptr);
  dbModInst* u_h2 = block_->findModInst("u_h2");
  ASSERT_NE(u_h2, nullptr);
  dbModInst* u_h3 = block_->findModInst("u_h3");
  ASSERT_NE(u_h3, nullptr);
  dbModInst* u_h4 = block_->findModInst("u_h4");
  ASSERT_NE(u_h4, nullptr);
  dbModInst* u_h5 = block_->findModInst("u_h4/u_h5");
  ASSERT_NE(u_h5, nullptr);
  dbModule* mod_h0 = block_->findModule("ModH0");
  ASSERT_NE(mod_h0, nullptr);
  dbModule* mod_h1 = block_->findModule("ModH1");
  ASSERT_NE(mod_h1, nullptr);
  dbModule* mod_h2 = block_->findModule("ModH2");
  ASSERT_NE(mod_h2, nullptr);
  dbModule* mod_h3 = block_->findModule("ModH3");
  ASSERT_NE(mod_h3, nullptr);
  dbModule* mod_h4 = block_->findModule("ModH4");
  ASSERT_NE(mod_h4, nullptr);
  dbModule* mod_h5 = block_->findModule("ModH5");
  ASSERT_NE(mod_h5, nullptr);
  dbModNet* modnet_h4_in = block_->findModNet("u_h4/in");
  ASSERT_NE(modnet_h4_in, nullptr);

  // odb::dbDatabase::beginEco(block_);

  // Pre sanity check
  sta_->updateTiming(true);
  num_warning = db_network_->checkAxioms();
  num_warning += sta_->checkSanity();
  EXPECT_EQ(num_warning, 0);

  //----------------------------------------------------
  // Insert buffer
  // - Targets: load5 (in H2/H1), load2 (in H3), load3 (in H4/H5)
  //----------------------------------------------------
  std::set<dbObject*> targets;
  targets.insert(load5_a);
  targets.insert(load2_a);
  targets.insert(load3_a);
  dbInst* new_buf = target_net->insertBufferBeforeLoads(
      targets, buffer_master, nullptr, "new_buf");
  ASSERT_TRUE(new_buf);

  //----------------------------------------------------
  // Verify Results
  //----------------------------------------------------

  // Buffer Location: Top Module (LCA)
  EXPECT_EQ(new_buf->getModule(), block_->getTopModule());

  // Net Separation
  dbNet* buf_out_net = new_buf->findITerm("Z")->getNet();
  ASSERT_TRUE(buf_out_net);
  EXPECT_NE(buf_out_net, target_net);

  // Target Loads moved to new net
  EXPECT_EQ(load5->findITerm("A")->getNet(), buf_out_net);
  EXPECT_EQ(load2->findITerm("A")->getNet(), buf_out_net);
  EXPECT_EQ(load3->findITerm("A")->getNet(), buf_out_net);

  // Non-Target Loads remain on old net
  EXPECT_EQ(load0->findITerm("A")->getNet(), target_net);
  EXPECT_EQ(load1->findITerm("A")->getNet(), target_net);
  EXPECT_EQ(load4->findITerm("A")->getNet(), target_net);

  // Port Punching Verification (Logical)
  dbModNet* top_out_mod_net
      = block_->getTopModule()->getModNet(buf_out_net->getConstName());
  ASSERT_NE(top_out_mod_net, nullptr);
  ASSERT_TRUE(top_out_mod_net);

  // Check if new ports were punched into H2, H3, H4
  bool punched_h2 = false;
  bool punched_h3 = false;
  bool punched_h4 = false;

  for (dbModITerm* iterm : top_out_mod_net->getModITerms()) {
    if (iterm->getParent() == u_h2) {
      punched_h2 = true;
    }
    if (iterm->getParent() == u_h3) {
      punched_h3 = true;
    }
    if (iterm->getParent() == u_h4) {
      punched_h4 = true;
    }
  }

  EXPECT_TRUE(punched_h2);  // Top -> H2 -> H1 -> load5
  EXPECT_TRUE(punched_h3);  // Top -> H3 -> load2
  EXPECT_TRUE(punched_h4);  // Top -> H4 -> H5 -> load3

  // Deeper check for H4 -> H5 punching
  // We need to find the net inside H4 that connects to the new port
  // This is tricky without knowing exact name, but we can search H4's nets
  bool punched_h5_in_h4 = false;
  for (dbModNet* net : mod_h4->getModNets()) {
    // If this net connects to u_h5 AND to the boundary (ModBTerm), it's likely
    // the punched net
    bool connects_boundary = !net->getModBTerms().empty();
    bool connects_h5 = false;
    for (dbModITerm* iterm : net->getModITerms()) {
      if (iterm->getParent() == u_h5) {
        connects_h5 = true;
      }
    }
    if (connects_boundary && connects_h5 && net != modnet_h4_in) {
      punched_h5_in_h4 = true;
      break;
    }
  }
  EXPECT_TRUE(punched_h5_in_h4);

  // Write verilog and check the content
  writeAndCompareVerilogOutputFile(test_name, test_name + "_post.v");
}

// Netlist Structure:
//
// [Top Module]
//  |
//  +-- u_h0 (ModH0) -> drvr (LOGIC0_X1) [Driver]
//  |
//  +-- load0 (BUF_X1)
//  +-- load1 (BUF_X1)
//  +-- load4 (BUF_X1) [TARGET]
//  |
//  +-- u_h1 (ModH1) -> load2 (BUF_X1) [TARGET]
//  |
//  +-- u_h2 (ModH2)
//       |
//       +-- u_h3 (ModH3) -> load3 (BUF_X1) [TARGET]
//
// Operation:
//   Insert buffer for targets: { load4, load2, load3 }
//   LCA: Top Module
//
// Expected Result:
//   1. New buffer 'new_buf' placed in Top.
//   2. Logical connections (Port Punching):
//      - To load4: Top -> load4 (Direct connection)
//      - To load2: Top -> u_h1 (New Port created)
//      - To load3: Top -> u_h2 -> u_h3 (New Ports created)
//   3. Non-targets (load0, load1) remain on original net.
TEST_F(TestInsertBuffer, BeforeLoads_Case7)
{
  // Get the test name dynamically from the gtest framework.
  const auto* test_info = testing::UnitTest::GetInstance()->current_test_info();
  const std::string test_name
      = std::string(test_info->test_suite_name()) + "_" + test_info->name();

  int num_warning = 0;
  readVerilogAndSetup(test_name + ".v");

  // Get ODB objects
  dbInst* drvr = block_->findInst("u_h0/drvr");
  ASSERT_NE(drvr, nullptr);
  dbInst* load0 = block_->findInst("load0");
  ASSERT_NE(load0, nullptr);
  dbInst* load1 = block_->findInst("load1");
  ASSERT_NE(load1, nullptr);
  dbInst* load4 = block_->findInst("load4");
  ASSERT_NE(load4, nullptr);
  dbInst* load2 = block_->findInst("u_h1/load2");
  ASSERT_NE(load2, nullptr);
  dbInst* load3 = block_->findInst("u_h2/u_h3/load3");
  ASSERT_NE(load3, nullptr);

  dbITerm* load4_a = load4->findITerm("A");
  ASSERT_NE(load4_a, nullptr);
  dbITerm* load2_a = load2->findITerm("A");
  ASSERT_NE(load2_a, nullptr);
  dbITerm* load3_a = load3->findITerm("A");
  ASSERT_NE(load3_a, nullptr);

  dbNet* target_net = load4_a->getNet();
  ASSERT_NE(target_net, nullptr);

  dbMaster* buffer_master = db_->findMaster("BUF_X1");
  ASSERT_NE(buffer_master, nullptr);

  dbModInst* u_h1 = block_->findModInst("u_h1");
  ASSERT_NE(u_h1, nullptr);
  dbModInst* u_h2 = block_->findModInst("u_h2");
  ASSERT_NE(u_h2, nullptr);
  dbModInst* u_h3 = block_->findModInst("u_h2/u_h3");
  ASSERT_NE(u_h3, nullptr);

  dbModule* mod_h2 = block_->findModule("ModH2");
  ASSERT_NE(mod_h2, nullptr);
  dbModNet* modnet_h2_in = block_->findModNet("u_h2/in");
  ASSERT_NE(modnet_h2_in, nullptr);

  // Pre sanity check
  sta_->updateTiming(true);
  num_warning = db_network_->checkAxioms();
  num_warning += sta_->checkSanity();
  EXPECT_EQ(num_warning, 0);

  //----------------------------------------------------
  // Insert buffer
  // - Targets: load4 (Top), load2 (H1), load3 (H2/H3)
  //----------------------------------------------------
  std::set<dbObject*> targets;
  targets.insert(load4_a);
  targets.insert(load2_a);
  targets.insert(load3_a);
  dbInst* new_buf = target_net->insertBufferBeforeLoads(
      targets, buffer_master, nullptr, "new_buf");
  ASSERT_TRUE(new_buf);

  //----------------------------------------------------
  // Verify Results
  //----------------------------------------------------

  // Buffer Location: Top Module (LCA)
  EXPECT_EQ(new_buf->getModule(), block_->getTopModule());

  // Net Separation
  dbNet* buf_out_net = new_buf->findITerm("Z")->getNet();
  ASSERT_TRUE(buf_out_net);
  EXPECT_NE(buf_out_net, target_net);

  // Target Loads moved to new net
  EXPECT_EQ(load4->findITerm("A")->getNet(), buf_out_net);
  EXPECT_EQ(load2->findITerm("A")->getNet(), buf_out_net);
  EXPECT_EQ(load3->findITerm("A")->getNet(), buf_out_net);

  // Non-Target Loads remain on old net
  EXPECT_EQ(load0->findITerm("A")->getNet(), target_net);
  EXPECT_EQ(load1->findITerm("A")->getNet(), target_net);

  // Port Punching Verification (Logical)
  dbModNet* top_out_mod_net
      = block_->getTopModule()->getModNet(buf_out_net->getConstName());
  ASSERT_NE(top_out_mod_net, nullptr);
  ASSERT_TRUE(top_out_mod_net);

  // Check if new ports were punched into H1, H2
  bool punched_h1 = false;
  bool punched_h2 = false;

  for (dbModITerm* iterm : top_out_mod_net->getModITerms()) {
    if (iterm->getParent() == u_h1) {
      punched_h1 = true;
    }
    if (iterm->getParent() == u_h2) {
      punched_h2 = true;
    }
  }

  EXPECT_TRUE(punched_h1);  // Top -> H1 -> load2
  EXPECT_TRUE(punched_h2);  // Top -> H2 -> H3 -> load3

  // Deeper check for H2 -> H3 punching
  bool punched_h3_in_h2 = false;
  for (dbModNet* net : mod_h2->getModNets()) {
    // If this net connects to u_h3 AND to the boundary (ModBTerm), it's likely
    // the punched net
    bool connects_boundary = !net->getModBTerms().empty();
    bool connects_h3 = false;
    for (dbModITerm* iterm : net->getModITerms()) {
      if (iterm->getParent() == u_h3) {
        connects_h3 = true;
      }
    }
    if (connects_boundary && connects_h3 && net != modnet_h2_in) {
      punched_h3_in_h2 = true;
      break;
    }
  }
  EXPECT_FALSE(punched_h3_in_h2);

  // Write verilog and check the content
  writeAndCompareVerilogOutputFile(test_name, test_name + "_post.v");
}

//
// Case 8: Insert buffer on a net connected to an Input dbBTerm.
// Net: in_port -> load1
// Target: load1
// Result: in_port -> buffer -> load1
//
TEST_F(TestInsertBuffer, BeforeLoads_Case8)
{
  // Get the test name dynamically from the gtest framework.
  const auto* test_info = testing::UnitTest::GetInstance()->current_test_info();
  const std::string test_name
      = std::string(test_info->test_suite_name()) + "_" + test_info->name();

  // Masters
  dbMaster* buf_master = db_->findMaster("BUF_X1");
  ASSERT_TRUE(buf_master);
  dbMaster* buffer_master = db_->findMaster("BUF_X4");
  ASSERT_TRUE(buffer_master);

  // Instance
  dbInst* load1 = dbInst::create(block_, buf_master, "load1");
  ASSERT_TRUE(load1);

  // Net + Port
  dbNet* n1 = dbNet::create(block_, "n1");
  dbBTerm* in_port = dbBTerm::create(n1, "in");
  in_port->setIoType(dbIoType::INPUT);

  // Connect
  dbITerm* load1_a = load1->findITerm("A");
  load1_a->connect(n1);

  // Insert Buffer
  std::set<dbObject*> targets;
  targets.insert(load1_a);
  dbInst* new_buf
      = n1->insertBufferBeforeLoads(targets, buffer_master, nullptr, "new_buf");
  ASSERT_TRUE(new_buf);

  // Verify
  // buffer input should be connected to original net (driven by input port)
  EXPECT_EQ(new_buf->findITerm("A")->getNet(), n1);
  // buffer output should drive a new net
  dbNet* out_net = new_buf->findITerm("Z")->getNet();
  ASSERT_TRUE(out_net);
  EXPECT_NE(out_net, n1);
  // load1 should be on new net
  EXPECT_EQ(load1_a->getNet(), out_net);

  // Write verilog and check the content
  writeAndCompareVerilogOutputFile(test_name, test_name + "_post.v");
}

//
// Case 9: Insert buffer on a net connected to an Output dbBTerm.
// Net: driver -> out_port
// Target: out_port
// Result: driver -> buffer -> out_port
//
TEST_F(TestInsertBuffer, BeforeLoads_Case9)
{
  // Get the test name dynamically from the gtest framework.
  const auto* test_info = testing::UnitTest::GetInstance()->current_test_info();
  const std::string test_name
      = std::string(test_info->test_suite_name()) + "_" + test_info->name();

  // Masters
  dbMaster* buf_master = db_->findMaster("BUF_X1");
  ASSERT_TRUE(buf_master);
  dbMaster* buffer_master = db_->findMaster("BUF_X4");
  ASSERT_TRUE(buffer_master);

  // Instance
  dbInst* drvr = dbInst::create(block_, buf_master, "drvr");
  ASSERT_TRUE(drvr);

  // Net + Port
  dbNet* n1 = dbNet::create(block_, "n1");
  dbBTerm* out_port = dbBTerm::create(n1, "out");
  out_port->setIoType(dbIoType::OUTPUT);

  // Connect
  dbITerm* drvr_z = drvr->findITerm("Z");
  drvr_z->connect(n1);

  // Insert Buffer
  std::set<dbObject*> targets;
  targets.insert(out_port);
  dbInst* new_buf
      = n1->insertBufferBeforeLoads(targets, buffer_master, nullptr, "new_buf");
  ASSERT_TRUE(new_buf);

  // Verify
  // buffer input connected to n1 (driven by drvr)
  EXPECT_EQ(new_buf->findITerm("A")->getNet(), n1);
  // buffer output connected to a new net?
  // No, typically insertBufferBeforeLoads splits the net.
  // If target is BTerm, the BTerm moves to new net.
  dbNet* out_net = new_buf->findITerm("Z")->getNet();
  ASSERT_TRUE(out_net);
  EXPECT_NE(out_net, n1);
  EXPECT_EQ(out_port->getNet(), out_net);

  // Write verilog and check the content
  writeAndCompareVerilogOutputFile(test_name, test_name + "_post.v");
}

//
// Case 10: Insert buffer on a feedthrough net connecting Input dbBTerm to
// Output dbBTerm.
// Netist: in_port -> out_port
// Target: out_port
// Result: in_port -> new_buf -> out_port
//
TEST_F(TestInsertBuffer, BeforeLoads_Case10)
{
  // Get the test name dynamically from the gtest framework.
  const auto* test_info = testing::UnitTest::GetInstance()->current_test_info();
  const std::string test_name
      = std::string(test_info->test_suite_name()) + "_" + test_info->name();

  // Masters
  dbMaster* buffer_master = db_->findMaster("BUF_X4");
  ASSERT_TRUE(buffer_master);

  // Net + Ports
  dbNet* n1 = dbNet::create(block_, "n1");
  dbBTerm* in_port = dbBTerm::create(n1, "in");
  in_port->setIoType(dbIoType::INPUT);
  dbBTerm* out_port = dbBTerm::create(n1, "out");
  out_port->setIoType(dbIoType::OUTPUT);

  // Insert Buffer
  std::set<dbObject*> targets;
  targets.insert(out_port);
  dbInst* new_buf
      = n1->insertBufferBeforeLoads(targets, buffer_master, nullptr, "new_buf");
  ASSERT_TRUE(new_buf);

  // Verify
  // buffer input connected to n1 (driven by in_port)
  EXPECT_EQ(new_buf->findITerm("A")->getNet(), n1);
  // buffer output connected to new net
  dbNet* out_net = new_buf->findITerm("Z")->getNet();
  ASSERT_TRUE(out_net);
  EXPECT_NE(out_net, n1);
  EXPECT_EQ(out_port->getNet(), out_net);

  // Write verilog and check the content
  writeAndCompareVerilogOutputFile(test_name, test_name + "_post.v");
}

//
// Case 11: Insert on a net having both dbBTerm (Input) and dbITerm.
// Net: in (port) -> load1, load2
// Target: load1
// Result: in -> load2
//         in-> new_buf -> Load1
//
TEST_F(TestInsertBuffer, BeforeLoads_Case11)
{
  // Get the test name dynamically from the gtest framework.
  const auto* test_info = testing::UnitTest::GetInstance()->current_test_info();
  const std::string test_name
      = std::string(test_info->test_suite_name()) + "_" + test_info->name();

  // Masters
  dbMaster* buf_master = db_->findMaster("BUF_X1");
  ASSERT_TRUE(buf_master);
  dbMaster* buffer_master = db_->findMaster("BUF_X4");
  ASSERT_TRUE(buffer_master);

  // Instances
  dbInst* load1 = dbInst::create(block_, buf_master, "load1");
  dbInst* load2 = dbInst::create(block_, buf_master, "load2");

  // Net + Port
  dbNet* n1 = dbNet::create(block_, "n1");
  dbBTerm* in_port = dbBTerm::create(n1, "in");
  in_port->setIoType(dbIoType::INPUT);

  // Connect
  dbITerm* load1_a = load1->findITerm("A");
  dbITerm* load2_a = load2->findITerm("A");
  load1_a->connect(n1);
  load2_a->connect(n1);

  // Insert Buffer for load1 only
  std::set<dbObject*> targets;
  targets.insert(load1_a);
  dbInst* new_buf
      = n1->insertBufferBeforeLoads(targets, buffer_master, nullptr, "new_buf");
  ASSERT_TRUE(new_buf);

  // Verify
  // n1 should still connect in_port and load2
  EXPECT_EQ(in_port->getNet(), n1);
  EXPECT_EQ(load2_a->getNet(), n1);
  // buffer input on n1
  EXPECT_EQ(new_buf->findITerm("A")->getNet(), n1);
  // buffer output on new net
  dbNet* out_net = new_buf->findITerm("Z")->getNet();
  EXPECT_NE(out_net, n1);
  // load1 on new net
  EXPECT_EQ(load1_a->getNet(), out_net);

  // Write verilog and check the content
  writeAndCompareVerilogOutputFile(test_name, test_name + "_post.v");
}

//
// Case 12: Insert on a net having dbBTerm (Output), dbITerm (Load).
// Net: driver -> load1, out_port
// Target: load1, out_port
// Result: driver -> buffer -> load1, out_port
//
TEST_F(TestInsertBuffer, BeforeLoads_Case12)
{
  // Get the test name dynamically from the gtest framework.
  const auto* test_info = testing::UnitTest::GetInstance()->current_test_info();
  const std::string test_name
      = std::string(test_info->test_suite_name()) + "_" + test_info->name();

  // Masters
  dbMaster* buf_master = db_->findMaster("BUF_X1");
  ASSERT_TRUE(buf_master);
  dbMaster* buffer_master = db_->findMaster("BUF_X4");
  ASSERT_TRUE(buffer_master);

  // Instances
  dbInst* drvr = dbInst::create(block_, buf_master, "drvr");
  dbInst* load1 = dbInst::create(block_, buf_master, "load1");

  // Net + Port
  dbNet* n1 = dbNet::create(block_, "n1");
  dbBTerm* out_port = dbBTerm::create(n1, "out");
  out_port->setIoType(dbIoType::OUTPUT);

  // Connect
  drvr->findITerm("Z")->connect(n1);
  dbITerm* load1_a = load1->findITerm("A");
  load1_a->connect(n1);

  // Insert Buffer for both
  std::set<dbObject*> targets;
  targets.insert(load1_a);
  targets.insert(out_port);
  dbInst* new_buf
      = n1->insertBufferBeforeLoads(targets, buffer_master, nullptr, "new_buf");
  ASSERT_TRUE(new_buf);

  // Verify
  // buffer input on n1 (driven by drvr)
  EXPECT_EQ(new_buf->findITerm("A")->getNet(), n1);
  // new net drives targets
  dbNet* out_net = new_buf->findITerm("Z")->getNet();
  EXPECT_NE(out_net, n1);
  EXPECT_EQ(load1_a->getNet(), out_net);
  EXPECT_EQ(out_port->getNet(), out_net);

  // Write verilog and check the content
  writeAndCompareVerilogOutputFile(test_name, test_name + "_post.v");
}

//
// Case 13: Mixed Hierarchy (dbBTerm, dbITerm, dbModITerm implicitly via
// hierarchy) We use a simpler hierarchy construction than Case 6/7.
// Structure:
//   Top:
//     - drvr -> n1
//     - load1 (on n1)
//     - SubModule u1 (on n1 via port A) -> load2
//     - out_port (on n1)
// Targets: load1, load2, out_port.
//
TEST_F(TestInsertBuffer, BeforeLoads_Case13)
{
  // Get the test name dynamically from the gtest framework.
  const auto* test_info = testing::UnitTest::GetInstance()->current_test_info();
  const std::string test_name
      = std::string(test_info->test_suite_name()) + "_" + test_info->name();
  readVerilogAndSetup(test_name + ".v");

  // Find objects
  dbInst* load1 = block_->findInst("load1");
  ASSERT_NE(load1, nullptr);
  dbInst* load2 = block_->findInst("u1/load2");
  ASSERT_NE(load2, nullptr);
  dbModInst* u1 = block_->findModInst("u1");
  ASSERT_NE(u1, nullptr);
  dbBTerm* out_port = block_->findBTerm("out");
  ASSERT_NE(out_port, nullptr);
  dbITerm* load1_a = load1->findITerm("A");
  ASSERT_NE(load1_a, nullptr);
  dbITerm* load2_a = load2->findITerm("A");
  ASSERT_NE(load2_a, nullptr);
  dbNet* n1 = load1_a->getNet();
  ASSERT_NE(n1, nullptr);

  // Targets
  std::set<dbObject*> targets;
  targets.insert(load1_a);   // Leaf on top
  targets.insert(out_port);  // Output Port
  targets.insert(load2_a);   // Hierarchical Leaf

  // Master
  dbMaster* buffer_master = db_->findMaster("BUF_X4");
  ASSERT_TRUE(buffer_master);

  // Insert Buffer
  dbInst* new_buf
      = n1->insertBufferBeforeLoads(targets, buffer_master, nullptr, "new_buf");
  ASSERT_TRUE(new_buf);

  // Verify
  dbNet* out_net = new_buf->findITerm("Z")->getNet();
  EXPECT_NE(out_net, n1);

  EXPECT_EQ(load1_a->getNet(), out_net);
  EXPECT_EQ(out_port->getNet(), out_net);
  EXPECT_EQ(load2_a->getNet(), out_net);

  // Verify port reuse for load2
  // - MOD1/A should be reused.
  dbModNet* top_out_mod_net
      = block_->getTopModule()->getModNet(out_net->getConstName());
  ASSERT_TRUE(top_out_mod_net);

  bool port_reuse = false;
  for (dbModITerm* iterm : top_out_mod_net->getModITerms()) {
    if (iterm->getParent() == u1 && std::string(iterm->getName()) == "A") {
      port_reuse = true;
    }
  }
  EXPECT_TRUE(port_reuse);

  // Write verilog and check the content
  writeAndCompareVerilogOutputFile(test_name, test_name + "_post.v");
}

// Case 14: Insert Buffer for partial loads on different nets
TEST_F(TestInsertBuffer, BeforeLoads_Case14)
{
  // Get the test name dynamically from the gtest framework.
  const auto* test_info = testing::UnitTest::GetInstance()->current_test_info();
  const std::string test_name
      = std::string(test_info->test_suite_name()) + "_" + test_info->name();

  // 1. Setup
  dbMaster* buf_master = db_->findMaster("BUF_X1");
  dbMaster* load_master = db_->findMaster("BUF_X1");
  dbMaster* drvr_master = db_->findMaster("LOGIC0_X1");

  // Create instances
  dbInst* drvr_inst = dbInst::create(block_, drvr_master, "drvr_inst");
  dbInst* load1_inst = dbInst::create(block_, load_master, "load1_inst");
  dbInst* load2_inst = dbInst::create(block_, load_master, "load2_inst");

  // Create nets
  dbNet* drvr_net = dbNet::create(block_, "drvr_net");
  dbNet* other_net = dbNet::create(block_, "other_net");

  // Connect
  drvr_inst->findITerm("Z")->connect(drvr_net);
  load1_inst->findITerm("A")->connect(drvr_net);

  // Connect load2 to other_net
  load2_inst->findITerm("A")->connect(other_net);

  // Write verilog and check the content
  writeAndCompareVerilogOutputFile(test_name, test_name + "_pre.v");

  // 2. Insert Buffer
  std::set<dbObject*> loads;
  loads.insert(load1_inst->findITerm("A"));
  loads.insert(load2_inst->findITerm("A"));

  // Call with loads_on_diff_nets = true
  dbInst* buf_inst
      = drvr_net->insertBufferBeforeLoads(loads,
                                          buf_master,
                                          nullptr,
                                          "buf",
                                          nullptr,
                                          dbNameUniquifyType::IF_NEEDED,
                                          true);

  // 3. Verify
  ASSERT_NE(buf_inst, nullptr);

  // Buffer input should be connected to drvr_net
  EXPECT_EQ(buf_inst->findITerm("A")->getNet(), drvr_net);

  // Buffer output should drive a new net
  dbNet* buf_out_net = buf_inst->findITerm("Z")->getNet();
  ASSERT_NE(buf_out_net, nullptr);
  EXPECT_NE(buf_out_net, drvr_net);
  EXPECT_NE(buf_out_net, other_net);

  // Both loads should be connected to buf_out_net
  EXPECT_EQ(load1_inst->findITerm("A")->getNet(), buf_out_net);
  EXPECT_EQ(load2_inst->findITerm("A")->getNet(), buf_out_net);

  // Write verilog and check the content
  writeAndCompareVerilogOutputFile(test_name, test_name + "_post.v");
}

TEST_F(TestInsertBuffer, BeforeLoads_Case15)
{
  // Get the test name dynamically from the gtest framework.
  const auto* test_info = testing::UnitTest::GetInstance()->current_test_info();
  const std::string test_name
      = std::string(test_info->test_suite_name()) + "_" + test_info->name();

  int num_warning = 0;
  readVerilogAndSetup(test_name + ".v");

  // Get ODB objects
  dbInst* drvr = block_->findInst("drvr");
  ASSERT_NE(drvr, nullptr);
  dbInst* load1 = block_->findInst("load1");
  ASSERT_NE(load1, nullptr);
  dbInst* u1_load2 = block_->findInst("u1/load2");
  ASSERT_NE(u1_load2, nullptr);
  dbInst* u1_load3 = block_->findInst("u1/load3");
  ASSERT_NE(u1_load3, nullptr);
  dbInst* u1_non_target = block_->findInst("u1/non_target");
  ASSERT_NE(u1_non_target, nullptr);
  dbITerm* load1_a = load1->findITerm("A");
  ASSERT_NE(load1_a, nullptr);
  dbITerm* u1_load2_a = u1_load2->findITerm("A");
  ASSERT_NE(u1_load2_a, nullptr);
  dbITerm* u1_load3_a = u1_load3->findITerm("A");
  ASSERT_NE(u1_load3_a, nullptr);
  dbITerm* u1_non_target_a = u1_non_target->findITerm("A");
  ASSERT_NE(u1_non_target_a, nullptr);
  dbNet* target_net = load1_a->getNet();
  ASSERT_NE(target_net, nullptr);
  dbMaster* buffer_master = db_->findMaster("BUF_X1");
  ASSERT_NE(buffer_master, nullptr);
  dbModInst* u1 = block_->findModInst("u1");
  ASSERT_NE(u1, nullptr);
  dbModule* mod1 = block_->findModule("MOD1");
  ASSERT_NE(mod1, nullptr);
  dbModNet* modnet_a = block_->findModNet("u1/A");
  ASSERT_NE(modnet_a, nullptr);

  // odb::dbDatabase::beginEco(block_);

  // Pre sanity check
  sta_->updateTiming(true);
  num_warning = db_network_->checkAxioms();
  num_warning += sta_->checkSanity();
  EXPECT_EQ(num_warning, 0);

  //----------------------------------------------------
  // Insert buffer
  // - Targets: load1, u1/load2, u1/load3
  //----------------------------------------------------
  std::set<dbObject*> targets;
  targets.insert(load1_a);
  targets.insert(u1_load2_a);
  targets.insert(u1_load3_a);
  dbInst* new_buf = target_net->insertBufferBeforeLoads(
      targets, buffer_master, nullptr, "new_buf");
  ASSERT_TRUE(new_buf);

  //----------------------------------------------------
  // Verify Results
  //----------------------------------------------------

  // Post sanity check
  num_warning = db_network_->checkAxioms();
  num_warning += sta_->checkSanity();
  EXPECT_EQ(num_warning, 0);

  // Buffer Location: Top Module (LCA)
  EXPECT_EQ(new_buf->getModule(), block_->getTopModule());

  // Net Separation
  dbNet* buf_out_net = new_buf->findITerm("Z")->getNet();
  ASSERT_TRUE(buf_out_net);
  EXPECT_NE(buf_out_net, target_net);

  // Target Loads moved to new net
  EXPECT_EQ(load1_a->getNet(), buf_out_net);
  EXPECT_EQ(u1_load2_a->getNet(), buf_out_net);
  EXPECT_EQ(u1_load3_a->getNet(), buf_out_net);

  // Non-Target Loads remain on old net
  EXPECT_EQ(u1_non_target_a->getNet(), target_net);

  //----------------------------------------------------
  // Verify Concrete Port Registration
  //----------------------------------------------------

  // Check existing instance ports are concrete
  for (dbITerm* iterm : drvr->getITerms()) {
    sta::Pin* pin = db_network_->dbToSta(iterm);
    ASSERT_NE(pin, nullptr);
    sta::Port* port = db_network_->port(pin);
    ASSERT_NE(port, nullptr);
    EXPECT_TRUE(db_network_->isConcretePort(port))
        << "Port of existing instance drvr should be concrete: "
        << db_network_->name(port);
  }

  for (dbITerm* iterm : load1->getITerms()) {
    sta::Pin* pin = db_network_->dbToSta(iterm);
    ASSERT_NE(pin, nullptr);
    sta::Port* port = db_network_->port(pin);
    ASSERT_NE(port, nullptr);
    EXPECT_TRUE(db_network_->isConcretePort(port))
        << "Port of existing instance load1 should be concrete: "
        << db_network_->name(port);
  }

  for (dbITerm* iterm : u1_load2->getITerms()) {
    sta::Pin* pin = db_network_->dbToSta(iterm);
    ASSERT_NE(pin, nullptr);
    sta::Port* port = db_network_->port(pin);
    ASSERT_NE(port, nullptr);
    EXPECT_TRUE(db_network_->isConcretePort(port))
        << "Port of existing instance u1/load2 should be concrete: "
        << db_network_->name(port);
  }

  for (dbITerm* iterm : u1_load3->getITerms()) {
    sta::Pin* pin = db_network_->dbToSta(iterm);
    ASSERT_NE(pin, nullptr);
    sta::Port* port = db_network_->port(pin);
    ASSERT_NE(port, nullptr);
    EXPECT_TRUE(db_network_->isConcretePort(port))
        << "Port of existing instance u1/load3 should be concrete: "
        << db_network_->name(port);
  }

  for (dbITerm* iterm : u1_non_target->getITerms()) {
    sta::Pin* pin = db_network_->dbToSta(iterm);
    ASSERT_NE(pin, nullptr);
    sta::Port* port = db_network_->port(pin);
    ASSERT_NE(port, nullptr);
    EXPECT_TRUE(db_network_->isConcretePort(port))
        << "Port of existing instance u1/non_target should be concrete: "
        << db_network_->name(port);
  }

  // Check newly inserted buffer ports are concrete
  // - All dbMTerm of dbMaster objects are registered as concrete ports
  //   when a library is loaded.
  // - insertBuffer*() don't have to perform any operation regarding
  //   concrete_port registration.
  for (dbITerm* iterm : new_buf->getITerms()) {
    sta::Pin* pin = db_network_->dbToSta(iterm);
    ASSERT_NE(pin, nullptr);
    sta::Port* port = db_network_->port(pin);
    ASSERT_NE(port, nullptr);
    EXPECT_TRUE(db_network_->isConcretePort(port))
        << "Port of newly inserted buffer should be concrete: "
        << db_network_->name(port);
  }

  // Write verilog and check the content
  writeAndCompareVerilogOutputFile(test_name, test_name + "_post.v");
}

TEST_F(TestInsertBuffer, BeforeLoads_Case16)
{
  // Get the test name dynamically from the gtest framework.
  const auto* test_info = testing::UnitTest::GetInstance()->current_test_info();
  const std::string test_name
      = std::string(test_info->test_suite_name()) + "_" + test_info->name();

  int num_warning = 0;
  readVerilogAndSetup(test_name + "_pre.v");

  // Get ODB objects
  dbInst* load0 = block_->findInst("load0");
  ASSERT_NE(load0, nullptr);
  dbInst* h0_load1 = block_->findInst("h0/load1");
  ASSERT_NE(h0_load1, nullptr);

  dbITerm* load0_a = load0->findITerm("A");
  ASSERT_NE(load0_a, nullptr);
  dbITerm* h0_load1_a = h0_load1->findITerm("A");
  ASSERT_NE(h0_load1_a, nullptr);

  dbNet* target_net = load0_a->getNet();
  ASSERT_NE(target_net, nullptr);

  dbMaster* buffer_master = db_->findMaster("BUF_X1");
  ASSERT_NE(buffer_master, nullptr);

  // Pre sanity check
  sta_->updateTiming(true);
  num_warning = db_network_->checkAxioms();
  num_warning += sta_->checkSanity();
  EXPECT_EQ(num_warning, 0);

  //----------------------------------------------------
  // Insert buffer
  // - Targets: load0, h0/load1
  //----------------------------------------------------
  std::set<dbObject*> targets;
  targets.insert(load0_a);
  targets.insert(h0_load1_a);

  dbInst* new_buf = target_net->insertBufferBeforeLoads(
      targets, buffer_master, nullptr, "new_buf");
  ASSERT_TRUE(new_buf);

  //----------------------------------------------------
  // Verify Results
  //----------------------------------------------------

  // Post sanity check
  num_warning = db_network_->checkAxioms();
  num_warning += sta_->checkSanity();
  EXPECT_EQ(num_warning, 0);

  // Write verilog and check the content
  writeAndCompareVerilogOutputFile(test_name, test_name + "_post.v");
}

TEST_F(TestInsertBuffer, BeforeLoads_Case17)
{
  // Get the test name dynamically from the gtest framework.
  const auto* test_info = testing::UnitTest::GetInstance()->current_test_info();
  const std::string test_name
      = std::string(test_info->test_suite_name()) + "_" + test_info->name();

  int num_warning = 0;
  readVerilogAndSetup(test_name + "_pre.v");

  // Get ODB objects
  dbInst* drvr = block_->findInst("drvr");
  ASSERT_NE(drvr, nullptr);
  dbInst* buf = block_->findInst("buf");
  ASSERT_NE(buf, nullptr);
  dbInst* load0 = block_->findInst("load0");
  ASSERT_NE(load0, nullptr);
  dbInst* h0_load1 = block_->findInst("h0/load1");
  ASSERT_NE(h0_load1, nullptr);
  dbInst* non_target0 = block_->findInst("non_target0");
  ASSERT_NE(non_target0, nullptr);

  dbITerm* drvr_z = drvr->findITerm("Z");
  ASSERT_NE(drvr_z, nullptr);
  dbITerm* buf_z = buf->findITerm("Z");
  ASSERT_NE(buf_z, nullptr);
  dbITerm* load0_a = load0->findITerm("A");
  ASSERT_NE(load0_a, nullptr);
  dbITerm* h0_load1_a = h0_load1->findITerm("A");
  ASSERT_NE(h0_load1_a, nullptr);
  dbITerm* non_target0_a = non_target0->findITerm("A");
  ASSERT_NE(non_target0_a, nullptr);

  dbNet* target_net = load0_a->getNet();
  ASSERT_NE(target_net, nullptr);

  dbMaster* buffer_master = db_->findMaster("BUF_X4");
  ASSERT_NE(buffer_master, nullptr);

  // Pre sanity check
  sta_->updateTiming(true);
  num_warning = db_network_->checkAxioms();
  num_warning += sta_->checkSanity();
  EXPECT_EQ(num_warning, 0);

  //----------------------------------------------------
  // Insert buffer
  // - Targets: load0, h0/load1
  // - Note that the two loads are on different dbNets.
  //----------------------------------------------------
  std::set<dbObject*> targets;
  targets.insert(load0_a);
  targets.insert(h0_load1_a);

  dbInst* new_buf
      = target_net->insertBufferBeforeLoads(targets,
                                            buffer_master,
                                            nullptr,
                                            "new_buf",
                                            nullptr,
                                            odb::dbNameUniquifyType::ALWAYS,
                                            true);
  ASSERT_TRUE(new_buf);

  //----------------------------------------------------
  // Verify Results
  //----------------------------------------------------

  // buf/Z should have NO fanout load
  sta::Pin* buf_out_pin = db_network_->dbToSta(buf_z);
  ASSERT_FALSE(resizer_.hasFanout(buf_out_pin));

  // Post sanity check
  num_warning = db_network_->checkAxioms();
  num_warning += sta_->checkSanity();
  EXPECT_EQ(num_warning, 0);

  // Write verilog and check the content
  writeAndCompareVerilogOutputFile(test_name, test_name + "_post.v");
}

TEST_F(TestInsertBuffer, BeforeLoads_Case18)
{
  // Get the test name dynamically from the gtest framework.
  const auto* test_info = testing::UnitTest::GetInstance()->current_test_info();
  const std::string test_name
      = std::string(test_info->test_suite_name()) + "_" + test_info->name();

  int num_warning = 0;
  readVerilogAndSetup(test_name + "_pre.v");

  // Get ODB objects
  dbInst* drvr = block_->findInst("drvr");
  ASSERT_NE(drvr, nullptr);
  dbInst* buf = block_->findInst("buf");
  ASSERT_NE(buf, nullptr);
  dbInst* load0 = block_->findInst("load0");
  ASSERT_NE(load0, nullptr);
  dbInst* h0_load1 = block_->findInst("h0/load1");
  ASSERT_NE(h0_load1, nullptr);
  dbInst* non_target0 = block_->findInst("non_target0");
  ASSERT_NE(non_target0, nullptr);

  dbITerm* drvr_z = drvr->findITerm("Z");
  ASSERT_NE(drvr_z, nullptr);
  dbITerm* buf_z = buf->findITerm("Z");
  ASSERT_NE(buf_z, nullptr);
  dbITerm* load0_a = load0->findITerm("A");
  ASSERT_NE(load0_a, nullptr);
  dbITerm* h0_load1_a = h0_load1->findITerm("A");
  ASSERT_NE(h0_load1_a, nullptr);
  dbITerm* non_target0_a = non_target0->findITerm("A");
  ASSERT_NE(non_target0_a, nullptr);

  dbNet* target_net = load0_a->getNet();
  ASSERT_NE(target_net, nullptr);

  dbMaster* buffer_master = db_->findMaster("BUF_X4");
  ASSERT_NE(buffer_master, nullptr);

  // Pre sanity check
  sta_->updateTiming(true);
  num_warning = db_network_->checkAxioms();
  num_warning += sta_->checkSanity();
  EXPECT_EQ(num_warning, 0);

  //----------------------------------------------------
  // Insert buffer
  // - Targets: load0, h0/load1
  // - Note that the two loads are on different dbNets.
  //----------------------------------------------------
  std::set<dbObject*> targets;
  targets.insert(load0_a);
  targets.insert(h0_load1_a);

  dbInst* new_buf
      = target_net->insertBufferBeforeLoads(targets,
                                            buffer_master,
                                            nullptr,
                                            "new_buf",
                                            nullptr,
                                            odb::dbNameUniquifyType::ALWAYS,
                                            true);
  ASSERT_TRUE(new_buf);

  //----------------------------------------------------
  // Verify Results
  //----------------------------------------------------

  // buf/Z should have fanout load
  sta::Pin* buf_out_pin = db_network_->dbToSta(buf_z);
  ASSERT_TRUE(resizer_.hasFanout(buf_out_pin));

  // Post sanity check
  num_warning = db_network_->checkAxioms();
  num_warning += sta_->checkSanity();
  EXPECT_EQ(num_warning, 0);

  // Write verilog and check the content
  writeAndCompareVerilogOutputFile(test_name, test_name + "_post.v");
}

// Partial-load buffering with loads of bus ports in a fakeram hard-macro.
// New buffer will be inserted outside a submodule containing the fakeram,
// which results in a dbModITerm connection change.
// STA state should be consistent to the ODB state.
TEST_F(TestInsertBuffer, BeforeLoads_Case19)
{
  // Get the test name dynamically from the gtest framework.
  const auto* test_info = testing::UnitTest::GetInstance()->current_test_info();
  const std::string test_name
      = std::string(test_info->test_suite_name()) + "_" + test_info->name();

  int num_warning = 0;

  // Load fakeram
  loadLibaryLef(lib_->getTech(),
                "Nangate45_fakeram",
                getFilePath("_main/test/Nangate45/fakeram45_64x7.lef"));
  readLiberty(getFilePath("_main/test/Nangate45/fakeram45_64x7.lib"));

  // Read verilog
  readVerilogAndSetup(test_name + "_pre.v");

  // Get ODB objects
  dbInst* drvr = block_->findInst("drvr");
  ASSERT_NE(drvr, nullptr);
  dbInst* buf = block_->findInst("buf");
  ASSERT_NE(buf, nullptr);
  dbInst* load0 = block_->findInst("load0");
  ASSERT_NE(load0, nullptr);
  dbInst* non_target0 = block_->findInst("non_target0");
  ASSERT_NE(non_target0, nullptr);
  dbInst* h0_mem = block_->findInst("h0/mem");
  ASSERT_NE(h0_mem, nullptr);

  dbITerm* drvr_z = drvr->findITerm("Z");
  ASSERT_NE(drvr_z, nullptr);
  dbITerm* buf_z = buf->findITerm("Z");
  ASSERT_NE(buf_z, nullptr);
  dbITerm* load0_a = load0->findITerm("A");
  ASSERT_NE(load0_a, nullptr);
  dbITerm* non_target0_a = non_target0->findITerm("A");
  ASSERT_NE(non_target0_a, nullptr);
  dbITerm* h0_mem_w_mask_in0 = h0_mem->findITerm("w_mask_in[0]");
  ASSERT_NE(h0_mem_w_mask_in0, nullptr);
  dbITerm* h0_mem_w_mask_in1 = h0_mem->findITerm("w_mask_in[1]");
  ASSERT_NE(h0_mem_w_mask_in1, nullptr);
  dbITerm* h0_mem_w_mask_in2 = h0_mem->findITerm("w_mask_in[2]");
  ASSERT_NE(h0_mem_w_mask_in2, nullptr);

  dbNet* target_net = load0_a->getNet();
  ASSERT_NE(target_net, nullptr);

  dbMaster* buffer_master = db_->findMaster("BUF_X4");
  ASSERT_NE(buffer_master, nullptr);

  // Pre sanity check
  sta_->updateTiming(true);
  num_warning = db_network_->checkAxioms();
  num_warning += sta_->checkSanity();
  EXPECT_EQ(num_warning, 0);

  // buf/Z should have fanout load
  sta::Pin* buf_out_pin = db_network_->dbToSta(buf_z);
  ASSERT_TRUE(resizer_.hasFanout(buf_out_pin));

  //----------------------------------------------------
  // Insert buffer
  // - Targets: load0, h0/mem/w_mask_in[2:0]
  // - Note that the two loads are on different dbNets.
  //----------------------------------------------------
  std::set<dbObject*> targets;
  targets.insert(load0_a);
  targets.insert(h0_mem_w_mask_in0);
  targets.insert(h0_mem_w_mask_in1);
  targets.insert(h0_mem_w_mask_in2);

  dbInst* new_buf
      = target_net->insertBufferBeforeLoads(targets,
                                            buffer_master,
                                            nullptr,
                                            "new_buf",
                                            nullptr,
                                            odb::dbNameUniquifyType::ALWAYS,
                                            true);
  ASSERT_TRUE(new_buf);

  //----------------------------------------------------
  // Verify Results
  //----------------------------------------------------

  // buf/Z should have NO fanout load
  ASSERT_FALSE(resizer_.hasFanout(buf_out_pin));

  // Post sanity check
  num_warning = db_network_->checkAxioms();
  num_warning += sta_->checkSanity();
  EXPECT_EQ(num_warning, 0);

  // Write verilog and check the content
  writeAndCompareVerilogOutputFile(test_name, test_name + "_post.v");
}

// Partial-load buffering with three loads in three different hierarchies.
TEST_F(TestInsertBuffer, BeforeLoads_Case20)
{
  // Get the test name dynamically from the gtest framework.
  const auto* test_info = testing::UnitTest::GetInstance()->current_test_info();
  const std::string test_name
      = std::string(test_info->test_suite_name()) + "_" + test_info->name();

  int num_warning = 0;

  // Load fakeram
  loadLibaryLef(lib_->getTech(),
                "Nangate45_fakeram",
                getFilePath("_main/test/Nangate45/fakeram45_64x7.lef"));
  readLiberty(getFilePath("_main/test/Nangate45/fakeram45_64x7.lib"));

  // Read verilog
  readVerilogAndSetup(test_name + "_pre.v");

  // Get ODB objects
  dbInst* drvr = block_->findInst("drvr");
  ASSERT_NE(drvr, nullptr);
  dbInst* h0_load0 = block_->findInst("h0/load0");
  ASSERT_NE(h0_load0, nullptr);
  dbInst* h0_h1_load1 = block_->findInst("h0/h1/load1");
  ASSERT_NE(h0_h1_load1, nullptr);
  dbInst* h0_h1_nontarget0 = block_->findInst("h0/h1/nontarget0");
  ASSERT_NE(h0_h1_nontarget0, nullptr);
  dbInst* h2_mem = block_->findInst("h2/mem");
  ASSERT_NE(h2_mem, nullptr);

  dbITerm* drvr_z = drvr->findITerm("Z");
  ASSERT_NE(drvr_z, nullptr);
  dbITerm* h0_load0_a = h0_load0->findITerm("A");
  ASSERT_NE(h0_load0_a, nullptr);
  dbITerm* h0_h1_load1_a = h0_h1_load1->findITerm("A");
  ASSERT_NE(h0_h1_load1_a, nullptr);
  dbITerm* h0_h1_nontarget0_a = h0_h1_nontarget0->findITerm("A");
  ASSERT_NE(h0_h1_nontarget0_a, nullptr);
  dbITerm* h2_mem_we_in = h2_mem->findITerm("we_in");
  ASSERT_NE(h2_mem_we_in, nullptr);

  dbNet* target_net = drvr_z->getNet();
  ASSERT_NE(target_net, nullptr);

  dbMaster* buffer_master = db_->findMaster("BUF_X4");
  ASSERT_NE(buffer_master, nullptr);

  // Pre sanity check
  sta_->updateTiming(true);
  num_warning = db_network_->checkAxioms();
  num_warning += sta_->checkSanity();
  EXPECT_EQ(num_warning, 0);

  //----------------------------------------------------
  // Insert buffer
  // - Targets: load0, h0/mem/w_mask_in[2:0]
  // - Note that the two loads are on different dbNets.
  //----------------------------------------------------
  std::set<dbObject*> targets;
  targets.insert(h0_load0_a);
  targets.insert(h0_h1_load1_a);
  targets.insert(h2_mem_we_in);

  dbInst* new_buf
      = target_net->insertBufferBeforeLoads(targets,
                                            buffer_master,
                                            nullptr,
                                            "new_buf",
                                            nullptr,
                                            odb::dbNameUniquifyType::ALWAYS,
                                            true);
  ASSERT_TRUE(new_buf);

  //----------------------------------------------------
  // Verify Results
  //----------------------------------------------------

  // Post sanity check
  num_warning = db_network_->checkAxioms();
  num_warning += sta_->checkSanity();
  EXPECT_EQ(num_warning, 0);

  // Write verilog and check the content
  writeAndCompareVerilogOutputFile(test_name, test_name + "_post.v");
}

// Partial-load buffering on loads connected with both input & output
// dbModBTerms.
TEST_F(TestInsertBuffer, BeforeLoads_Case21)
{
  // Get the test name dynamically from the gtest framework.
  const auto* test_info = testing::UnitTest::GetInstance()->current_test_info();
  const std::string test_name
      = std::string(test_info->test_suite_name()) + "_" + test_info->name();

  int num_warning = 0;

  // Read verilog
  readVerilogAndSetup(test_name + "_pre.v");

  // Get ODB objects
  dbInst* drvr = block_->findInst("h0/drvr");
  ASSERT_NE(drvr, nullptr);
  dbInst* h1_h2_load0 = block_->findInst("h1/h2/load0");
  ASSERT_NE(h1_h2_load0, nullptr);
  dbInst* h3_load1 = block_->findInst("h3/load1");
  ASSERT_NE(h3_load1, nullptr);
  dbInst* h3_load2 = block_->findInst("h3/load2");
  ASSERT_NE(h3_load2, nullptr);

  dbITerm* drvr_z = drvr->findITerm("Z");
  ASSERT_NE(drvr_z, nullptr);
  dbITerm* h1_h2_load0_a = h1_h2_load0->findITerm("A");
  ASSERT_NE(h1_h2_load0_a, nullptr);
  dbITerm* h3_load1_a = h3_load1->findITerm("A");
  ASSERT_NE(h3_load1_a, nullptr);
  dbITerm* h3_load2_a = h3_load2->findITerm("A");
  ASSERT_NE(h3_load2_a, nullptr);

  dbNet* target_net = drvr_z->getNet();
  ASSERT_NE(target_net, nullptr);

  dbMaster* buffer_master = db_->findMaster("BUF_X4");
  ASSERT_NE(buffer_master, nullptr);

  // Pre sanity check
  sta_->updateTiming(true);
  num_warning = db_network_->checkAxioms();
  num_warning += sta_->checkSanity();
  EXPECT_EQ(num_warning, 0);

  //----------------------------------------------------
  // Insert buffer
  // - Targets: h1/h2/load0, h3/load1, h3/load2
  // - Note that the two loads are on different dbNets.
  //----------------------------------------------------
  std::set<dbObject*> targets;
  targets.insert(h1_h2_load0_a);
  targets.insert(h3_load1_a);
  targets.insert(h3_load2_a);

  dbInst* new_buf
      = target_net->insertBufferBeforeLoads(targets,
                                            buffer_master,
                                            nullptr,
                                            "new_buf",
                                            nullptr,
                                            odb::dbNameUniquifyType::ALWAYS,
                                            true);
  ASSERT_TRUE(new_buf);

  //----------------------------------------------------
  // Verify Results
  //----------------------------------------------------

  // Post sanity check
  num_warning = db_network_->checkAxioms();
  num_warning += sta_->checkSanity();
  EXPECT_EQ(num_warning, 0);

  // Write verilog and check the content
  writeAndCompareVerilogOutputFile(test_name, test_name + "_post.v");
}

TEST_F(TestInsertBuffer, BeforeLoads_Case22)
{
  // Reproduction of ORD-2030: Flat net logical inconsistency
  const auto* test_info = testing::UnitTest::GetInstance()->current_test_info();
  const std::string test_name
      = std::string(test_info->test_suite_name()) + "_" + test_info->name();

  int num_warning = 0;

  // Read verilog
  readVerilogAndSetup(test_name + "_pre.v");

  dbMaster* buffer_master = db_->findMaster("BUF_X4");
  ASSERT_TRUE(buffer_master);

  // Get ODB objects
  dbInst* drvr = block_->findInst("drvr");
  ASSERT_NE(drvr, nullptr);
  dbInst* wb_inst_load_internal = block_->findInst("wb_inst/load_internal");
  ASSERT_NE(wb_inst_load_internal, nullptr);
  dbInst* exec_inst_load_exec = block_->findInst("exec_inst/load_exec");
  ASSERT_NE(exec_inst_load_exec, nullptr);

  dbITerm* drvr_z = drvr->findITerm("Z");
  ASSERT_NE(drvr_z, nullptr);
  dbITerm* load_internal_a = wb_inst_load_internal->findITerm("A");
  ASSERT_NE(load_internal_a, nullptr);
  dbITerm* load_exec_a = exec_inst_load_exec->findITerm("A");
  ASSERT_NE(load_exec_a, nullptr);

  dbNet* net_orig = drvr_z->getNet();
  ASSERT_NE(net_orig, nullptr);

  // Pre sanity check
  sta_->updateTiming(true);
  num_warning = db_network_->checkAxioms();
  num_warning += sta_->checkSanity();
  EXPECT_EQ(num_warning, 0);

  //----------------------------------------------------
  // Insert buffer
  // - Targets: load_internal_a, load_exec_a
  //----------------------------------------------------
  std::set<dbObject*> loads;
  loads.insert(load_internal_a);
  loads.insert(load_exec_a);

  dbInst* new_buf
      = net_orig->insertBufferBeforeLoads(loads,
                                          buffer_master,
                                          nullptr,
                                          "new_buf",
                                          nullptr,
                                          odb::dbNameUniquifyType::ALWAYS,
                                          true);
  ASSERT_TRUE(new_buf);

  //----------------------------------------------------
  // Verify Results
  //----------------------------------------------------

  // Post sanity check
  num_warning = db_network_->checkAxioms();
  num_warning += sta_->checkSanity();
  EXPECT_EQ(num_warning, 0);

  // Write verilog and check the content
  writeAndCompareVerilogOutputFile(test_name, test_name + "_post.v");
}

TEST_F(TestInsertBuffer, BeforeLoads_Case23)
{
  // Reproduction of ORD-2030: Flat net logical inconsistency
  // Net renaming across hierarchy + name collision in sub-module
  const auto* test_info = testing::UnitTest::GetInstance()->current_test_info();
  const std::string test_name
      = std::string(test_info->test_suite_name()) + "_" + test_info->name();

  int num_warning = 0;

  // Read verilog
  readVerilogAndSetup(test_name + "_pre.v");

  dbMaster* buffer_master = db_->findMaster("BUF_X4");
  ASSERT_TRUE(buffer_master);

  // Get ODB objects
  dbInst* drvr = block_->findInst("drvr");
  ASSERT_NE(drvr, nullptr);
  dbInst* load_inst = block_->findInst("inst1/load_inst");
  ASSERT_NE(load_inst, nullptr);

  dbITerm* drvr_q = drvr->findITerm("Q");
  ASSERT_NE(drvr_q, nullptr);
  dbITerm* load_a = load_inst->findITerm("A");
  ASSERT_NE(load_a, nullptr);

  dbNet* net_orig = drvr_q->getNet();
  ASSERT_NE(net_orig, nullptr);
  // Verify net name is "n1"
  EXPECT_EQ(std::string(net_orig->getConstName()), "n1");

  // Pre sanity check
  sta_->updateTiming(true);
  num_warning = db_network_->checkAxioms();
  num_warning += sta_->checkSanity();
  EXPECT_EQ(num_warning, 0);

  //----------------------------------------------------
  // Insert buffer
  // - Targets: load_a
  //----------------------------------------------------
  std::set<dbObject*> loads;
  loads.insert(load_a);

  dbInst* new_buf
      = net_orig->insertBufferBeforeLoads(loads,
                                          buffer_master,
                                          nullptr,
                                          "new_buf",
                                          nullptr,
                                          odb::dbNameUniquifyType::ALWAYS,
                                          true);
  ASSERT_TRUE(new_buf);

  //----------------------------------------------------
  // Verify Results
  //----------------------------------------------------

  // Post sanity check
  num_warning = db_network_->checkAxioms();
  num_warning += sta_->checkSanity();
  EXPECT_EQ(num_warning, 0);

  // Write verilog and check the content
  writeAndCompareVerilogOutputFile(test_name, test_name + "_post.v");
}

TEST_F(TestInsertBuffer, BeforeLoads_Case24)
{
  // Reproduction of ORD-2030: Flat net logical inconsistency
  // Multiple ModNets in target module scenario:
  //
  // Hierarchy:
  // - top (target module where buffer will be placed)
  //   - H0 (child module with feedthrough: in -> internal_buf -> out)
  //   - H1 (another child module with internal_load)
  //   - drvr (driver in top)
  //
  // Signal flow:
  // - drvr/Z drives flat net 'n1' (modnet 'n1' in top)
  // - n1 connects to h0/in (creates modnet 'in' inside H0)
  // - h0/out (feedthrough) connects to 'w1' (modnet 'w1' in top)
  // - w1 connects to h1/in (load path through different modnet)
  //
  // The key issue:
  // - Both 'n1' and 'w1' are modnets in 'top' module for the same logical net
  // - When buffering loads inside H0 and H1, the algorithm must select
  //   the driver's modnet ('n1'), not the load's modnet ('w1')

  const auto* test_info = testing::UnitTest::GetInstance()->current_test_info();
  const std::string test_name
      = std::string(test_info->test_suite_name()) + "_" + test_info->name();

  int num_warning = 0;

  // Read verilog
  readVerilogAndSetup(test_name + "_pre.v");

  dbMaster* buffer_master = db_->findMaster("BUF_X1");
  ASSERT_TRUE(buffer_master);

  // Get ODB objects
  dbInst* drvr = block_->findInst("drvr");
  ASSERT_NE(drvr, nullptr);
  dbInst* h0_internal_buf = block_->findInst("h0/internal_buf");
  ASSERT_NE(h0_internal_buf, nullptr);
  dbInst* h1_internal_load = block_->findInst("h1/internal_load");
  ASSERT_NE(h1_internal_load, nullptr);

  dbITerm* drvr_z = drvr->findITerm("Z");
  ASSERT_NE(drvr_z, nullptr);
  dbITerm* h0_internal_buf_a = h0_internal_buf->findITerm("A");
  ASSERT_NE(h0_internal_buf_a, nullptr);
  dbITerm* h1_internal_load_a = h1_internal_load->findITerm("A");
  ASSERT_NE(h1_internal_load_a, nullptr);

  dbNet* target_net = drvr_z->getNet();
  ASSERT_NE(target_net, nullptr);

  // dbNet "w1" makes "n1" dangling.
  // - "w1" and "n1" indicates the same logical net.
  // - "n1" is created first and has all the connections at first.
  // - "w1" is created later and takes over all the connections.
  // - "n1" has no connection at the end.
  EXPECT_EQ(std::string(target_net->getConstName()), "n1");

  dbModNet* modnet_n1 = block_->findModNet("n1");
  ASSERT_NE(modnet_n1, nullptr);

  dbModNet* modnet_w1 = block_->findModNet("w1");
  ASSERT_NE(modnet_w1, nullptr);

  dbModNet* modnet_h0_in = block_->findModNet("h0/in");
  ASSERT_NE(modnet_h0_in, nullptr);

  dbModNet* modnet_h1_in = block_->findModNet("h1/in");
  ASSERT_NE(modnet_h1_in, nullptr);

  // Check feedthrough path
  dbModBTerm* modbterm_h0_out = block_->findModBTerm("h0/out");
  ASSERT_NE(modbterm_h0_out, nullptr);
  dbModITerm* moditerm_h0_out = block_->findModITerm("h0/out");
  ASSERT_NE(moditerm_h0_out, nullptr);
  ASSERT_EQ(modbterm_h0_out->getModNet(), modnet_h0_in);

  dbModNet* modnet_h0_out = block_->findModNet("h0/out");
  ASSERT_EQ(modnet_h0_out, nullptr);

  // Verify we have multiple modnets in target module for this flat net
  std::set<dbModNet*> related_modnets;
  target_net->findRelatedModNets(related_modnets);
  std::set<dbModNet*> modnets_in_top;
  dbModule* top_module = block_->getTopModule();
  for (dbModNet* modnet : related_modnets) {
    if (modnet->getParent() == top_module) {
      modnets_in_top.insert(modnet);
    }
  }
  // Should have at least 2 modnets in top: 'n1' and 'w1'
  EXPECT_GE(modnets_in_top.size(), 2);

  // Pre sanity check
  sta_->updateTiming(true);
  num_warning = db_network_->checkAxioms();
  num_warning += sta_->checkSanity();
  EXPECT_EQ(num_warning, 0);

  //----------------------------------------------------
  // Insert buffer
  // - Targets: h0/internal_buf/A (inside H0, connected via h0/in modnet)
  //            AND h1/internal_load/A (inside H1, connected via h1/in modnet)
  // - Both are on the same logical net but via different modnets in 'top'
  // - The fix ensures buffer input connects to driver's modnet ('n1'),
  //   not the load's modnet ('w1')
  //----------------------------------------------------
  std::set<dbObject*> loads;
  loads.insert(h0_internal_buf_a);
  loads.insert(h1_internal_load_a);

  dbInst* new_buf
      = target_net->insertBufferBeforeLoads(loads,
                                            buffer_master,
                                            nullptr,
                                            "new_buf",
                                            nullptr,
                                            odb::dbNameUniquifyType::ALWAYS,
                                            true);
  ASSERT_NE(new_buf, nullptr);

  //----------------------------------------------------
  // Verify Results
  //----------------------------------------------------

  // Verify buffer input is connected to driver's modnet ('n1'), not 'w1'
  dbITerm* buf_input = new_buf->findITerm("A");
  ASSERT_NE(buf_input, nullptr);
  dbModNet* buf_input_modnet = buf_input->getModNet();
  ASSERT_NE(buf_input_modnet, nullptr);
  // Buffer input should be connected to 'n1' modnet (driver's modnet)
  EXPECT_EQ(std::string(buf_input_modnet->getConstName()), "n1");

  // Post sanity check - this was failing with ORD-2030 before the fix
  num_warning = db_network_->checkAxioms();
  num_warning += sta_->checkSanity();
  EXPECT_EQ(num_warning, 0);

  // Write verilog and check the content
  writeAndCompareVerilogOutputFile(test_name, test_name + "_post.v");

  // dbNet 'w1' loses dbModNet 'w1' because the new buffer cuts off the
  // connection
  dbModNet* w1_mn = block_->findModNet("w1");
  ASSERT_NE(w1_mn, nullptr);
}

TEST_F(TestInsertBuffer, BeforeLoads_Case25)
{
  // This case has a redundant port punching because the new buffer is placed at
  // the LCA (least common ancestor) module.
  // TODO: Enhance the algorithm to avoid port punching.
  const auto* test_info = testing::UnitTest::GetInstance()->current_test_info();
  const std::string test_name
      = std::string(test_info->test_suite_name()) + "_" + test_info->name();

  int num_warning = 0;

  // Read verilog
  readVerilogAndSetup(test_name + "_pre.v");

  dbMaster* buffer_master = db_->findMaster("BUF_X1");
  ASSERT_TRUE(buffer_master);

  // Get ODB objects
  dbInst* drvr = block_->findInst("h0/drvr");
  ASSERT_NE(drvr, nullptr);
  dbInst* h0_load0 = block_->findInst("h0/load0");
  ASSERT_NE(h0_load0, nullptr);
  dbInst* load1 = block_->findInst("load1");
  ASSERT_NE(load1, nullptr);

  dbITerm* drvr_z = drvr->findITerm("Z");
  ASSERT_NE(drvr_z, nullptr);
  dbITerm* h0_load0_a = h0_load0->findITerm("A");
  ASSERT_NE(h0_load0_a, nullptr);
  dbITerm* load1_a = load1->findITerm("A");
  ASSERT_NE(load1_a, nullptr);

  dbNet* target_net = drvr_z->getNet();
  ASSERT_NE(target_net, nullptr);

  // Pre sanity check
  sta_->updateTiming(true);
  num_warning = db_network_->checkAxioms();
  num_warning += sta_->checkSanity();
  EXPECT_EQ(num_warning, 0);

  //----------------------------------------------------
  // Insert buffer
  //----------------------------------------------------
  std::set<dbObject*> loads;
  loads.insert(h0_load0_a);
  loads.insert(load1_a);

  dbInst* new_buf
      = target_net->insertBufferBeforeLoads(loads,
                                            buffer_master,
                                            nullptr,
                                            "new_buf",
                                            nullptr,
                                            odb::dbNameUniquifyType::ALWAYS,
                                            true);
  ASSERT_NE(new_buf, nullptr);

  //----------------------------------------------------
  // Verify Results
  //----------------------------------------------------

  // Post sanity check
  num_warning = db_network_->checkAxioms();
  num_warning += sta_->checkSanity();
  EXPECT_EQ(num_warning, 0);

  // Write verilog and check the content
  writeAndCompareVerilogOutputFile(test_name, test_name + "_post.v");
}

TEST_F(TestInsertBuffer, BeforeLoads_Case26)
{
  // This case has a redundant port punching because the new buffer is placed at
  // the LCA (least common ancestor) module.
  // TODO: Enhance the algorithm to avoid port punching.
  const auto* test_info = testing::UnitTest::GetInstance()->current_test_info();
  const std::string test_name
      = std::string(test_info->test_suite_name()) + "_" + test_info->name();

  int num_warning = 0;

  // Read verilog
  readVerilogAndSetup(test_name + "_pre.v");

  dbMaster* buffer_master = db_->findMaster("BUF_X1");
  ASSERT_TRUE(buffer_master);

  // Get ODB objects
  dbInst* h0_drvr = block_->findInst("h0/drvr");
  ASSERT_NE(h0_drvr, nullptr);
  dbInst* h1_load0 = block_->findInst("h1/load0");
  ASSERT_NE(h1_load0, nullptr);
  dbInst* h1_load1 = block_->findInst("h1/load1");
  ASSERT_NE(h1_load1, nullptr);

  dbITerm* h0_drvr_z = h0_drvr->findITerm("Z");
  ASSERT_NE(h0_drvr_z, nullptr);
  dbITerm* h1_load0_a = h1_load0->findITerm("A");
  ASSERT_NE(h1_load0_a, nullptr);
  dbITerm* h1_load1_a = h1_load1->findITerm("A");
  ASSERT_NE(h1_load1_a, nullptr);

  dbNet* target_net = h0_drvr_z->getNet();
  ASSERT_NE(target_net, nullptr);

  // Pre sanity check
  sta_->updateTiming(true);
  num_warning = db_network_->checkAxioms();
  num_warning += sta_->checkSanity();
  EXPECT_EQ(num_warning, 0);

  //----------------------------------------------------
  // Insert buffer
  //----------------------------------------------------
  std::set<dbObject*> loads;
  loads.insert(h1_load0_a);
  loads.insert(h1_load1_a);

  dbInst* new_buf
      = target_net->insertBufferBeforeLoads(loads,
                                            buffer_master,
                                            nullptr,
                                            "new_buf",
                                            nullptr,
                                            odb::dbNameUniquifyType::ALWAYS,
                                            true);
  ASSERT_NE(new_buf, nullptr);

  //----------------------------------------------------
  // Verify Results
  //----------------------------------------------------

  // Post sanity check
  num_warning = db_network_->checkAxioms();
  num_warning += sta_->checkSanity();
  EXPECT_EQ(num_warning, 0);

  // Write verilog and check the content
  writeAndCompareVerilogOutputFile(test_name, test_name + "_post.v");
}

TEST_F(TestInsertBuffer, BeforeLoads_Case27)
{
  // Get the test name dynamically from the gtest framework.
  const auto* test_info = testing::UnitTest::GetInstance()->current_test_info();
  const std::string test_name
      = std::string(test_info->test_suite_name()) + "_" + test_info->name();

  int num_warning = 0;

  // Read verilog
  readVerilogAndSetup(test_name + "_pre.v");

  // Get ODB objects
  dbMaster* buffer_master = db_->findMaster("BUF_X4");
  ASSERT_TRUE(buffer_master);
  dbInst* load1 = block_->findInst("h1/load1");
  ASSERT_NE(load1, nullptr);
  dbITerm* load1_a = load1->findITerm("A");
  ASSERT_NE(load1_a, nullptr);
  dbNet* target_net = block_->findNet("in");
  ASSERT_NE(target_net, nullptr);
  dbModule* mod_h1 = block_->findModule("H1");
  ASSERT_NE(mod_h1, nullptr);

  // Pre sanity check
  sta_->updateTiming(true);
  num_warning = db_network_->checkAxioms();
  num_warning += sta_->checkSanity();
  EXPECT_EQ(num_warning, 0);

  // Insert buffer before load1/A
  std::set<dbObject*> targets;
  targets.insert(load1_a);

  dbInst* new_buf = target_net->insertBufferBeforeLoads(
      targets, buffer_master, nullptr, "new_buf");
  ASSERT_TRUE(new_buf);

  //----------------------------------------------------
  // Verify Results
  //----------------------------------------------------
  EXPECT_EQ(new_buf->getModule(), mod_h1);
  EXPECT_EQ(new_buf->findITerm("A")->getNet(), target_net);

  // Post sanity check
  num_warning = db_network_->checkAxioms();
  num_warning += sta_->checkSanity();
  EXPECT_EQ(num_warning, 0);

  writeAndCompareVerilogOutputFile(test_name, test_name + "_post.v");
}

// Test case for hierarchical name collision with internal signal during port
// punching. When buffer is placed at top module (LCA of load1 in H1 and load4
// in top), the buffer output needs to punch a port into H1. The port name
// "_019_" should be avoided because H1 has an internal net with that name.
TEST_F(TestInsertBuffer, BeforeLoads_Case28)
{
  // Get the test name dynamically from the gtest framework.
  const auto* test_info = testing::UnitTest::GetInstance()->current_test_info();
  const std::string test_name
      = std::string(test_info->test_suite_name()) + "_" + test_info->name();

  int num_warning = 0;

  // Read verilog
  readVerilogAndSetup(test_name + "_pre.v");

  // Get ODB objects
  dbMaster* buffer_master = db_->findMaster("BUF_X4");
  ASSERT_TRUE(buffer_master);

  // Target net is "_019_" in top module (driven by drvr)
  dbNet* target_net = block_->findNet("_019_");
  ASSERT_NE(target_net, nullptr);

  // Top module
  dbModule* top_mod = block_->getTopModule();
  ASSERT_NE(top_mod, nullptr);

  // H1 module and its loads
  dbModule* mod_h1 = block_->findModule("H1");
  ASSERT_NE(mod_h1, nullptr);
  dbInst* load1 = block_->findInst("h1/load1");
  ASSERT_NE(load1, nullptr);
  dbITerm* load1_a = load1->findITerm("A");
  ASSERT_NE(load1_a, nullptr);

  // load4 in top module
  dbInst* load4 = block_->findInst("load4");
  ASSERT_NE(load4, nullptr);
  dbITerm* load4_a = load4->findITerm("A");
  ASSERT_NE(load4_a, nullptr);

  // Verify that "h1/_019_" internal net exists within H1 module
  // This is the XOR gate output, NOT connected to the port
  dbNet* internal_net = block_->findNet("h1/_019_");
  ASSERT_NE(internal_net, nullptr);
  EXPECT_TRUE(internal_net->isInternalTo(mod_h1));

  // Pre sanity check
  sta_->updateTiming(true);
  num_warning = db_network_->checkAxioms();
  num_warning += sta_->checkSanity();
  EXPECT_EQ(num_warning, 0);

  // Insert buffer before load1/A (in H1) AND load4/A (in top)
  // LCA is top module, so buffer will be placed in top.
  // Buffer output will need to punch a port into H1 to reach load1.
  // Since H1 already has internal net "h1/_019_", the punched port name
  // should be renamed to avoid collision (e.g., "_019__0")
  std::set<dbObject*> targets;
  targets.insert(load1_a);
  targets.insert(load4_a);

  dbInst* new_buf = target_net->insertBufferBeforeLoads(
      targets, buffer_master, nullptr, "new_buf");
  ASSERT_TRUE(new_buf);

  //----------------------------------------------------
  // Verify Results
  //----------------------------------------------------
  // Buffer should be placed in TOP module (LCA of load1 and load4)
  EXPECT_EQ(new_buf->getModule(), top_mod);

  // Verify that "h1/_019_" internal net still exists and is unchanged
  dbNet* internal_net_after = block_->findNet("h1/_019_");
  ASSERT_NE(internal_net_after, nullptr);
  EXPECT_EQ(internal_net, internal_net_after);

  // Verify that a new port "_019__0" is created in H1 to avoid collision
  dbModBTerm* punched_port = mod_h1->findModBTerm("_019__0");
  ASSERT_NE(punched_port, nullptr);
  EXPECT_EQ(punched_port->getIoType(), dbIoType::INPUT);

  // Post sanity check
  num_warning = db_network_->checkAxioms();
  num_warning += sta_->checkSanity();
  EXPECT_EQ(num_warning, 0);

  writeAndCompareVerilogOutputFile(test_name, test_name + "_post.v");
}

// Test case for ORD-2030 ERROR by insertBufferBeforeLoads()
TEST_F(TestInsertBuffer, BeforeLoads_Case29)
{
  // Get the test name dynamically from the gtest framework.
  const auto* test_info = testing::UnitTest::GetInstance()->current_test_info();
  const std::string test_name
      = std::string(test_info->test_suite_name()) + "_" + test_info->name();

  int num_warning = 0;

  // Read verilog
  readVerilogAndSetup(test_name + "_pre.v");

  // Get ODB objects
  dbMaster* buffer_master = db_->findMaster("BUF_X4");
  ASSERT_TRUE(buffer_master);

  // Target net
  dbNet* target_net = block_->findNet("n1");
  ASSERT_NE(target_net, nullptr);

  // Top module
  dbModule* top_mod = block_->getTopModule();
  ASSERT_NE(top_mod, nullptr);

  // load0 in top module
  dbInst* load0 = block_->findInst("load0");
  ASSERT_NE(load0, nullptr);
  dbITerm* load0_a = load0->findITerm("A");
  ASSERT_NE(load0_a, nullptr);

  // H1 module and its loads
  dbModule* mod_h0 = block_->findModule("H0");
  ASSERT_NE(mod_h0, nullptr);
  dbInst* load1 = block_->findInst("h0/load1");
  ASSERT_NE(load1, nullptr);
  dbITerm* load1_a = load1->findITerm("A");
  ASSERT_NE(load1_a, nullptr);

  dbModNet* modnet_n1 = block_->findModNet("n1");
  ASSERT_NE(modnet_n1, nullptr);

  // Pre sanity check
  sta_->updateTiming(true);
  num_warning = db_network_->checkAxioms();
  num_warning += sta_->checkSanity();
  EXPECT_EQ(num_warning, 0);

  // Insert buffer before load1/A (in H1) AND load0/A (in top)
  std::set<dbObject*> targets;
  targets.insert(load0_a);
  targets.insert(load1_a);

  dbInst* new_buf = target_net->insertBufferBeforeLoads(
      targets, buffer_master, nullptr, "new_buf");
  ASSERT_TRUE(new_buf);

  //----------------------------------------------------
  // Verify Results
  //----------------------------------------------------
  // Buffer should be placed in TOP module (LCA of load0 and load1)
  EXPECT_EQ(new_buf->getModule(), top_mod);

  // TODO: Dangling modnet "n1" is not removed
  modnet_n1 = block_->findModNet("n1");
  ASSERT_NE(modnet_n1, nullptr);

  // Post sanity check
  num_warning = db_network_->checkAxioms();
  num_warning += sta_->checkSanity();
  EXPECT_EQ(num_warning, 0);

  writeAndCompareVerilogOutputFile(test_name, test_name + "_post.v");
}

// Reproduction of ORD-2030 bug using Verilog input
// Fixed by preventing reuse of ModNets connected to Output/Inout ports.
TEST_F(TestInsertBuffer, BeforeLoads_Case30)
{
  // Get the test name dynamically from the gtest framework.
  const auto* test_info = testing::UnitTest::GetInstance()->current_test_info();
  const std::string test_name
      = std::string(test_info->test_suite_name()) + "_" + test_info->name();

  int num_warning = 0;

  // Read verilog
  readVerilogAndSetup(test_name + "_pre.v");

  block_ = db_->getChip()->getBlock();
  ASSERT_TRUE(block_);

  // Find Flat Net
  dbNet* flat_net = block_->findNet("ic_debug_addr_2");
  ASSERT_TRUE(flat_net);

  // Find Buffer Master
  dbMaster* buffer_master = db_->findMaster("BUF_X1");
  ASSERT_TRUE(buffer_master);

  // Collect Load Pins
  std::vector<dbObject*> load_pins;

  // Top loads
  std::vector<std::string> top_loads = {"u_2884",
                                        "u_2885",
                                        "u_2924",
                                        "u_2961",
                                        "u_2999",
                                        "u_3034",
                                        "u_3041",
                                        "u_3048",
                                        "u_3055"};
  for (const auto& name : top_loads) {
    dbInst* inst = block_->findInst(name.c_str());
    ASSERT_TRUE(inst);
    dbITerm* pin = inst->findITerm("A");
    ASSERT_TRUE(pin);
    load_pins.push_back(pin);
  }

  // Internal loads (swerv/ifu/...)
  std::vector<std::string> internal_loads
      = {"u_11044", "u_11045", "u_11046", "u_11047"};
  for (const auto& name : internal_loads) {
    std::string full_name = "swerv_inst/ifu/" + name;
    dbInst* inst = block_->findInst(full_name.c_str());
    ASSERT_TRUE(inst) << "Internal load " << full_name << " not found";
    dbITerm* pin = inst->findITerm("A");
    ASSERT_TRUE(pin);
    load_pins.push_back(pin);
  }

  // Run insert_buffer
  odb::dbInst* buf_inst
      = flat_net->insertBufferBeforeLoads(load_pins, buffer_master, nullptr);

  // Verify
  ASSERT_TRUE(buf_inst);

  sta_->updateTiming(true);
  num_warning = db_network_->checkAxioms();
  num_warning += sta_->checkSanity();

  // Expect 1 warning for 'swerv_inst/dec_tlu/zero_' has no driver (from 1'b0
  // connection)
  EXPECT_LE(num_warning, 1);

  // Write verilog and check the content
  writeAndCompareVerilogOutputFile(test_name, test_name + "_post.v");
}

// Test case for hierarchical buffer insertion w/ loads_on_diff_nets=true
TEST_F(TestInsertBuffer, BeforeLoads_Case31)
{
  const auto* test_info = testing::UnitTest::GetInstance()->current_test_info();
  const std::string test_name
      = std::string(test_info->test_suite_name()) + "_" + test_info->name();

  readVerilogAndSetup(test_name + "_pre.v");

  int num_warning = 0;

  // Find buffer master
  dbMaster* buf_master = db_->findMaster("BUF_X4");
  ASSERT_NE(buf_master, nullptr);

  // Find the target net to insert a buffer
  dbNet* target_net = block_->findNet("net5869");
  ASSERT_NE(target_net, nullptr);

  // Find the load pins
  dbITerm* load0_a = block_->findITerm("swerv/ifu/_11393_/A");
  ASSERT_NE(load0_a, nullptr);
  dbITerm* load1_a = block_->findITerm("swerv/ifu/_11394_/A");
  ASSERT_NE(load1_a, nullptr);

  // Pre sanity check
  sta_->updateTiming(true);
  num_warning = db_network_->checkAxioms();
  num_warning += sta_->checkSanity();
  EXPECT_EQ(num_warning, 0);

  // Prepare load set - only loads on net848 (hierarchical loads in swerv/ifu)
  std::set<dbObject*> load_pins;
  load_pins.insert(load0_a);
  load_pins.insert(load1_a);

  // Insert buffer before the hierarchical loads
  dbInst* buf_inst
      = target_net->insertBufferBeforeLoads(load_pins,
                                            buf_master,
                                            nullptr,
                                            "new_buf",
                                            nullptr,
                                            dbNameUniquifyType::IF_NEEDED,
                                            true);
  ASSERT_NE(buf_inst, nullptr);

  // Post sanity check
  num_warning = db_network_->checkAxioms();
  num_warning += sta_->checkSanity();
  EXPECT_EQ(num_warning, 0);

  // Write verilog and check the content
  writeAndCompareVerilogOutputFile(test_name, test_name + "_post.v");
}

// Test case for hierarchical buffer insertion w/ loads_on_diff_nets=true
// Reproduction of a crash when a load pin is not connected to any net.
TEST_F(TestInsertBuffer, BeforeLoads_Case32)
{
  const auto* test_info = testing::UnitTest::GetInstance()->current_test_info();
  const std::string test_name
      = std::string(test_info->test_suite_name()) + "_" + test_info->name();

  readVerilogAndSetup(test_name + "_pre.v");

  int num_warning = 0;

  // Find buffer master
  dbMaster* buf_master = db_->findMaster("BUF_X4");
  ASSERT_NE(buf_master, nullptr);

  // Find the target net to insert a buffer
  dbNet* target_net = block_->findNet("net5869");
  ASSERT_NE(target_net, nullptr);

  // Find the load pins
  dbITerm* load0_a = block_->findITerm("swerv/ifu/_11393_/A");
  ASSERT_NE(load0_a, nullptr);
  dbITerm* load1_a = block_->findITerm("swerv/_11394_/A");
  ASSERT_NE(load1_a, nullptr);

  // Pre sanity check
  sta_->updateTiming(true);
  // "swerv/ifu/_11393_/A" is not connected intentionally for test.
  // Thus, checkAxioms() should not be called.
  // num_warning = db_network_->checkAxioms();
  num_warning = sta_->checkSanity();
  EXPECT_EQ(num_warning, 0);

  // Prepare load set
  std::set<dbObject*> load_pins;
  load_pins.insert(load0_a);
  load_pins.insert(load1_a);

  // Insert buffer before the hierarchical loads
  dbInst* buf_inst
      = target_net->insertBufferBeforeLoads(load_pins,
                                            buf_master,
                                            nullptr,
                                            "new_buf",
                                            nullptr,
                                            dbNameUniquifyType::IF_NEEDED,
                                            true);
  ASSERT_NE(buf_inst, nullptr);

  // Post sanity check
  num_warning = db_network_->checkAxioms();
  num_warning += sta_->checkSanity();
  EXPECT_EQ(num_warning, 0);

  // Write verilog and check the content
  writeAndCompareVerilogOutputFile(test_name, test_name + "_post.v");
}

}  // namespace odb
