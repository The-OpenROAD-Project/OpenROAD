// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include <cstdio>
#include <string>

#include "db_sta/dbNetwork.hh"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "tst/fixture.h"
#include "utl/Logger.h"

namespace odb {

class TestSwapMasterUnusedPort : public tst::Fixture
{
 protected:
  void SetUp() override
  {
    const std::string nangate_lef
        = getFilePath("_main/test/Nangate45/Nangate45.lef");
    const std::string nangate_lib
        = getFilePath("_main/test/Nangate45/Nangate45_typ.lib");

    lib_ = loadTechAndLib("Nangate45", "Nangate45", nangate_lef);
    readLiberty(nangate_lib);

    odb::dbChip* chip = odb::dbChip::create(db_.get(), db_->getTech());
    block_ = odb::dbBlock::create(chip, "top");
    block_->setDieArea(odb::Rect(0, 0, 1000, 1000));

    // register proper callbacks for timer like read_def
    sta_->postReadDef(block_);

    // Enable hierarchy
    auto* db_network = sta_->getDbNetwork();
    db_network->setHierarchy();
    db_network->setBlock(block_);
  }

  odb::dbLib* lib_ = nullptr;
  odb::dbChip* chip_ = nullptr;
  odb::dbBlock* block_ = nullptr;
};

// Test Case: Swap Master with Unused Ports
//
// This test verifies that internal nets are correctly created and connected
// even when the corresponding module ports are UNCONNECTED (unused) at the
// instance level.
//
// Scenario:
// 1. Define 'module_A' (Original Master)
// 2. Define 'module_B' (New Master)
//    - Both have an input port 'in1' that is internally connected to logic.
// 3. Instantiate 'module_A' as 'inst_1'.
//    - LEAVE 'in1' UNCONNECTED at the top level.
// 4. Swap 'inst_1' master to 'module_B'.
// 5. Verify that inside 'inst_1' (now module_B), the internal net for 'in1'
//    exists and connects the port to the internal instance.
//      because it's connected to a port, but boundary stitching doesn't happen
//      because the port is unused.
//
// Test Case Scenario:
//
// 1. Initial State (inst_1 : module_A)
//    Top Block
//    +--------------------------------+
//    |                                |
//    |   inst_1 (module_A)            |
//    |   +------------------------+   |
//    |   | in1 (Unused Port)      |   |
//    |   |    |                   |   |
//    |   |    v                   |   |
//    |   | [BUF] (buf_a)          |   |
//    |   |    |                   |   |
//    |   +----|-------------------+   |
//    |        |                       |
//    +--------|-----------------------+
//             ^
//             |
//      (No Connection at Top)
//
// 2. After Swap (inst_1 : module_B)
//    - Expected: Internal Net (in1 -> buf_b) must exist even if in1 is unused.
TEST_F(TestSwapMasterUnusedPort, SwapMasterUnusedPortInternalConn)
{
  // Enable sanity checker for swapMaster
  logger_.setDebugLevel(utl::ODB, "replace_design_check_sanity", 1);

  auto* buf_master = lib_->findMaster("BUF_X1");
  ASSERT_NE(buf_master, nullptr);

  // 1. Create module_A (Original)
  auto* module_a = odb::dbModule::create(block_, "module_A");
  auto* modbterm_a_in1 = odb::dbModBTerm::create(module_a, "in1");
  modbterm_a_in1->setIoType(odb::dbIoType::INPUT);

  // Create internal logic for A: in1 -> BUF -> (internal net)
  auto* buf_a
      = odb::dbInst::create(block_, buf_master, "buf_a", false, module_a);
  auto* net_in1_a = odb::dbModNet::create(module_a, "net_in1");
  modbterm_a_in1->connect(net_in1_a);

  // Explicitly create flat net for module_A internal connection
  auto* flat_net_in1_a = odb::dbNet::create(block_, "module_A_net_in1");
  buf_a->findITerm("A")->connect(flat_net_in1_a);

  // Sync ModNet with FlatNet connections
  net_in1_a->connectTermsOf(flat_net_in1_a);

  // 2. Create module_B (New Master)
  auto* module_b = odb::dbModule::create(block_, "module_B");
  auto* modbterm_b_in1 = odb::dbModBTerm::create(module_b, "in1");
  modbterm_b_in1->setIoType(odb::dbIoType::INPUT);

  // Create internal logic for B: in1 -> BUF -> (internal net)
  // Logic is same/similar, just different master name to trigger swap logic
  auto* buf_b
      = odb::dbInst::create(block_, buf_master, "buf_b", false, module_b);
  auto* net_in1_b = odb::dbModNet::create(module_b, "net_in1");
  modbterm_b_in1->connect(net_in1_b);

  // Explicitly create flat net for module_B internal connection
  auto* flat_net_in1_b = odb::dbNet::create(block_, "module_B_net_in1");
  buf_b->findITerm("A")->connect(flat_net_in1_b);

  // Sync ModNet with FlatNet connections
  net_in1_b->connectTermsOf(flat_net_in1_b);

  // Force isInternalTo() to return false by connecting to a Top Block BTerm.
  // This simulates the condition where a net is considered "External"
  // (connected to port) causing copyModuleInsts to skip it in the buggy
  // version.
  auto* dummy_bterm
      = odb::dbBTerm::create(flat_net_in1_b, "dummy_bterm_force_external");
  dummy_bterm->setIoType(odb::dbIoType::OUTPUT);

  // 3. Instantiate module_A as inst_1
  auto* top_module = block_->getTopModule();
  auto* inst_1 = odb::dbModInst::create(top_module, module_a, "inst_1");

  // CRITICAL: We do NOT connect inst_1/in1 to anything at the top level.
  // It is an "Unused Port".
  // Only create the ITerm, but don't connect it to a top-level ModNet.
  auto* moditerm_in1 = odb::dbModITerm::create(inst_1, "in1");
  moditerm_in1->setChildModBTerm(modbterm_a_in1);
  modbterm_a_in1->setParentModITerm(moditerm_in1);

  // 4. Swap Master to module_B
  auto* new_inst_1 = inst_1->swapMaster(module_b);
  ASSERT_NE(new_inst_1, nullptr);

  // 5. Verification
  auto* new_master
      = new_inst_1->getMaster();  // This is the uniquified module_B

  // Find the internal instance in the new module
  // Name should be "inst_1/buf_b" (assuming flattening naming convention)
  auto* new_buf_b = new_master->findDbInst("inst_1/buf_b");
  ASSERT_NE(new_buf_b, nullptr);

  // Check the 'A' pin of the buffer. It should be connected to the internal net
  // for 'in1'.
  auto* iterm_A = new_buf_b->findITerm("A");
  ASSERT_NE(iterm_A, nullptr);

  auto* connected_net = iterm_A->getNet();
  EXPECT_NE(connected_net, nullptr)
      << "Internal pin 'A' of buffer is floating! Internal net for unused port "
         "'in1' was likely not created.";
}

}  // namespace odb
