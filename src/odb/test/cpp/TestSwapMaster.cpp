// Copyright 2024 The OpenROAD Authors
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <cstdio>
#include <string>

#include "db_sta/dbNetwork.hh"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "tst/fixture.h"

namespace odb {

class TestSwapMasterFixture : public tst::Fixture
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

TEST_F(TestSwapMasterFixture, ComplexSwap)
{
  // 1. Setup: Get masters for various cells from Nangate45
  auto* inv_master = lib_->findMaster("INV_X1");
  auto* nand_master = lib_->findMaster("NAND2_X1");
  auto* buf_master = lib_->findMaster("BUF_X1");
  auto* nor_master = lib_->findMaster("NOR2_X1");

  ASSERT_NE(inv_master, nullptr);
  ASSERT_NE(nand_master, nullptr);
  ASSERT_NE(buf_master, nullptr);
  ASSERT_NE(nor_master, nullptr);

  // Top module netlist (before swap):
  //
  //             +--------------------------------+
  //             |                                |
  //   top_in1 --| in1                            |
  //             |            module_A            |
  //   top_in2 --| in2          (inst_1)     out1 |-- top_out1
  //             |                                |
  //           --| unconnected_in                 |
  //             |                                |
  //             |                           out2 |--
  //             |                                |
  //             +--------------------------------+

  // module_A netlist:
  //
  //          +-------------+
  //   in1 ---| INV(inv1_a) |---+
  //          +-------------+   |   +---------------+
  //                            +---| A1            |
  //                                | NAND2(nand1_a)|--- out1
  //                            +---| A2            |
  //          +-------------+   |   +---------------+
  //   in2 ---| INV(inv2_a) |---+
  //          +-------------+
  //
  //   unconnected_in ---
  //
  //                      --- out2 (unconnected)
  //
  // This module is logically equivalent to a 2-input OR gate.
  // out1 = NOT( (NOT in1) AND (NOT in2) ) = in1 OR in2

  // module_B netlist:
  //
  //          +------------+
  //   in1 ---| BUF(buf1_b)|---+
  //          +------------+   |   +-------------+  +-----------------+
  //                           +---| A1          |  |                 |
  //                               | NOR2(nor1_b)|--| A INV(inv_out_b)|--- out1
  //                           +---| A2          |  |                 |
  //          +------------+   |   +-------------+  +-----------------+
  //   in2 ---| BUF(buf2_b)|---+
  //          +------------+
  //
  //   unconnected_in ---
  //
  //                     --- out2
  //
  // This module is logically equivalent to a 2-input OR gate.
  // out1 = NOT( NOT( (in1) OR (in2) ) ) = in1 OR in2
  //
  // module_A and module_B have the same port signature and are logically
  // equivalent, but have different internal logic and cell types. This makes
  // them swappable at the module instance level.

  // Create module_A: inverters and a NAND gate
  auto* module_a = odb::dbModule::create(block_, "module_A");
  auto* modbterm_a_in1 = odb::dbModBTerm::create(module_a, "in1");
  modbterm_a_in1->setIoType(odb::dbIoType::INPUT);
  auto* modbterm_a_in2 = odb::dbModBTerm::create(module_a, "in2");
  modbterm_a_in2->setIoType(odb::dbIoType::INPUT);
  auto* modbterm_a_unconnected_in
      = odb::dbModBTerm::create(module_a, "unconnected_in");
  modbterm_a_unconnected_in->setIoType(odb::dbIoType::INPUT);
  auto* modbterm_a_out1 = odb::dbModBTerm::create(module_a, "out1");
  modbterm_a_out1->setIoType(odb::dbIoType::OUTPUT);
  auto* modbterm_a_out2 = odb::dbModBTerm::create(module_a, "out2");
  modbterm_a_out2->setIoType(odb::dbIoType::OUTPUT);

  auto* inv1_a
      = odb::dbInst::create(block_, inv_master, "inv1_a", false, module_a);
  auto* inv2_a
      = odb::dbInst::create(block_, inv_master, "inv2_a", false, module_a);
  auto* nand1_a
      = odb::dbInst::create(block_, nand_master, "nand1_a", false, module_a);

  auto* net_in1_a = odb::dbModNet::create(module_a, "net_in1");
  modbterm_a_in1->connect(net_in1_a);
  inv1_a->findITerm("A")->connect(net_in1_a);

  auto* net_in2_a = odb::dbModNet::create(module_a, "net_in2");
  modbterm_a_in2->connect(net_in2_a);
  inv2_a->findITerm("A")->connect(net_in2_a);

  auto* net_n1_a = odb::dbModNet::create(module_a, "net_n1");
  inv1_a->findITerm("ZN")->connect(net_n1_a);
  nand1_a->findITerm("A1")->connect(net_n1_a);

  auto* net_n2_a = odb::dbModNet::create(module_a, "net_n2");
  inv2_a->findITerm("ZN")->connect(net_n2_a);
  nand1_a->findITerm("A2")->connect(net_n2_a);

  auto* net_out1_a = odb::dbModNet::create(module_a, "net_out1");
  modbterm_a_out1->connect(net_out1_a);
  nand1_a->findITerm("ZN")->connect(net_out1_a);
  // unconnected_in and out2 are not connected internally in module_A

  // Create module_B: buffers and a NOR gate (compatible port signature)
  auto* module_b = odb::dbModule::create(block_, "module_B");
  auto* modbterm_b_in1 = odb::dbModBTerm::create(module_b, "in1");
  modbterm_b_in1->setIoType(odb::dbIoType::INPUT);
  auto* modbterm_b_in2 = odb::dbModBTerm::create(module_b, "in2");
  modbterm_b_in2->setIoType(odb::dbIoType::INPUT);
  auto* modbterm_b_unconnected_in
      = odb::dbModBTerm::create(module_b, "unconnected_in");
  modbterm_b_unconnected_in->setIoType(odb::dbIoType::INPUT);
  auto* modbterm_b_out1 = odb::dbModBTerm::create(module_b, "out1");
  modbterm_b_out1->setIoType(odb::dbIoType::OUTPUT);
  auto* modbterm_b_out2 = odb::dbModBTerm::create(module_b, "out2");
  modbterm_b_out2->setIoType(odb::dbIoType::OUTPUT);

  auto* buf1_b
      = odb::dbInst::create(block_, buf_master, "buf1_b", false, module_b);
  auto* buf2_b
      = odb::dbInst::create(block_, buf_master, "buf2_b", false, module_b);
  auto* nor1_b
      = odb::dbInst::create(block_, nor_master, "nor1_b", false, module_b);
  auto* inv_out_b
      = odb::dbInst::create(block_, inv_master, "inv_out_b", false, module_b);

  auto* net_in1_b = odb::dbModNet::create(module_b, "net_in1");
  modbterm_b_in1->connect(net_in1_b);
  buf1_b->findITerm("A")->connect(net_in1_b);

  auto* net_in2_b = odb::dbModNet::create(module_b, "net_in2");
  modbterm_b_in2->connect(net_in2_b);
  buf2_b->findITerm("A")->connect(net_in2_b);

  auto* net_n1_b = odb::dbModNet::create(module_b, "net_n1");
  buf1_b->findITerm("Z")->connect(net_n1_b);
  nor1_b->findITerm("A1")->connect(net_n1_b);

  auto* net_n2_b = odb::dbModNet::create(module_b, "net_n2");
  buf2_b->findITerm("Z")->connect(net_n2_b);
  nor1_b->findITerm("A2")->connect(net_n2_b);

  auto* net_nor_out_b = odb::dbModNet::create(module_b, "net_nor_out");
  nor1_b->findITerm("ZN")->connect(net_nor_out_b);
  inv_out_b->findITerm("A")->connect(net_nor_out_b);

  auto* net_out1_b = odb::dbModNet::create(module_b, "net_out1");
  modbterm_b_out1->connect(net_out1_b);
  inv_out_b->findITerm("ZN")->connect(net_out1_b);
  // unconnected_in and out2 are not connected internally in module_B

  // Instantiate module_A in the top module
  auto* top_module = block_->getTopModule();
  auto* inst_1 = odb::dbModInst::create(top_module, module_a, "inst_1");

  // Create ModITerms for the instance and connect them
  auto* moditerm_in1 = odb::dbModITerm::create(inst_1, "in1");
  moditerm_in1->setChildModBTerm(modbterm_a_in1);
  modbterm_a_in1->setParentModITerm(moditerm_in1);

  auto* moditerm_in2 = odb::dbModITerm::create(inst_1, "in2");
  moditerm_in2->setChildModBTerm(modbterm_a_in2);
  modbterm_a_in2->setParentModITerm(moditerm_in2);

  auto* moditerm_unconnected_in
      = odb::dbModITerm::create(inst_1, "unconnected_in");
  moditerm_unconnected_in->setChildModBTerm(modbterm_a_unconnected_in);
  modbterm_a_unconnected_in->setParentModITerm(moditerm_unconnected_in);

  auto* moditerm_out1 = odb::dbModITerm::create(inst_1, "out1");
  moditerm_out1->setChildModBTerm(modbterm_a_out1);
  modbterm_a_out1->setParentModITerm(moditerm_out1);

  auto* moditerm_out2 = odb::dbModITerm::create(inst_1, "out2");
  moditerm_out2->setChildModBTerm(modbterm_a_out2);
  modbterm_a_out2->setParentModITerm(moditerm_out2);

  // Create and connect top-level ports and nets
  auto* net_top_in1 = odb::dbNet::create(block_, "top_in1");
  auto* top_in1 = odb::dbBTerm::create(net_top_in1, "top_in1");
  top_in1->setIoType(odb::dbIoType::INPUT);

  auto* net_top_in2 = odb::dbNet::create(block_, "top_in2");
  auto* top_in2 = odb::dbBTerm::create(net_top_in2, "top_in2");
  top_in2->setIoType(odb::dbIoType::INPUT);

  auto* net_top_out1 = odb::dbNet::create(block_, "top_out1");
  auto* top_out1 = odb::dbBTerm::create(net_top_out1, "top_out1");
  top_out1->setIoType(odb::dbIoType::OUTPUT);

  auto* mod_net_in1 = odb::dbModNet::create(top_module, "mod_net_in1");
  top_in1->connect(mod_net_in1);
  moditerm_in1->connect(mod_net_in1);

  auto* mod_net_in2 = odb::dbModNet::create(top_module, "mod_net_in2");
  top_in2->connect(mod_net_in2);
  moditerm_in2->connect(mod_net_in2);

  auto* mod_net_out1 = odb::dbModNet::create(top_module, "mod_net_out1");
  top_out1->connect(mod_net_out1);
  moditerm_out1->connect(mod_net_out1);

  // inst_1/out2 and inst_1/unconnected_in are intentionally not connected at
  // the top level to increase test coverage.

  // Pre-swap checks
  ASSERT_EQ(inst_1->getMaster(), module_a);
  ASSERT_EQ(module_a->getInsts().size(), 3);
  ASSERT_NE(block_->findModule("module_A"), nullptr);

  // Dump module_A contents for easier debugging
  printf("--- Dumping module_A (%s) contents ---\n", module_a->getName());
  printf("Instances:\n");
  for (auto* inst : module_a->getInsts()) {
    printf("  - %s (Master: %s)\n",
           inst->getName().c_str(),
           inst->getMaster()->getName().c_str());
  }
  printf("Module Instances:\n");
  for (auto* mod_inst : module_a->getModInsts()) {
    printf("  - %s (Master: %s)\n",
           mod_inst->getName(),
           mod_inst->getMaster()->getName());
  }
  printf("Module Nets:\n");
  for (auto* mod_net : module_a->getModNets()) {
    printf("  - %s\n", mod_net->getName().c_str());
  }
  printf("--- End of dump ---\n");

  // 2. Action
  auto* new_inst_1 = inst_1->swapMaster(module_b);

  // 3. Verification
  ASSERT_NE(new_inst_1, nullptr);
  EXPECT_STREQ(new_inst_1->getName(), "inst_1");

  // Master checks
  auto* new_master = new_inst_1->getMaster();
  ASSERT_NE(new_master, nullptr);
  EXPECT_NE(new_master, module_b);  // Should be a unique copy
  EXPECT_EQ(new_master->getInsts().size(), 4);

  // Dump new_master contents for easier debugging
  printf("--- Dumping new_master (%s) contents ---\n", new_master->getName());
  printf("Instances:\n");
  for (auto* inst : new_master->getInsts()) {
    printf("  - %s (Master: %s)\n",
           inst->getName().c_str(),
           inst->getMaster()->getName().c_str());
  }
  printf("Module Instances:\n");
  for (auto* mod_inst : new_master->getModInsts()) {
    printf("  - %s (Master: %s)\n",
           mod_inst->getName(),
           mod_inst->getMaster()->getName());
  }
  printf("Module Nets:\n");
  for (auto* mod_net : new_master->getModNets()) {
    printf("  - %s\n", mod_net->getName().c_str());
  }
  printf("--- End of dump ---\n");

  // Check internal instances of the new master by name
  auto* new_buf1 = new_master->findDbInst("inst_1/buf1_b");
  ASSERT_NE(new_buf1, nullptr);
  EXPECT_EQ(new_buf1->getMaster(), buf_master);

  auto* new_buf2 = new_master->findDbInst("inst_1/buf2_b");
  ASSERT_NE(new_buf2, nullptr);
  EXPECT_EQ(new_buf2->getMaster(), buf_master);

  auto* new_nor1 = new_master->findDbInst("inst_1/nor1_b");
  ASSERT_NE(new_nor1, nullptr);
  EXPECT_EQ(new_nor1->getMaster(), nor_master);

  // Check backup module
  // - module_A still exists in the child block of the top block.
  // - swapMaster() moves the old master to a child block as a backup.
  ASSERT_NE(block_->findModule("module_A"), nullptr);

  // Port and connection checks
  auto* new_moditerm_in1 = new_inst_1->findModITerm("in1");
  ASSERT_NE(new_moditerm_in1, nullptr);
  EXPECT_EQ(new_moditerm_in1->getModNet(), mod_net_in1);

  auto* new_moditerm_out1 = new_inst_1->findModITerm("out1");
  ASSERT_NE(new_moditerm_out1, nullptr);
  EXPECT_EQ(new_moditerm_out1->getModNet(), mod_net_out1);

  // Check unconnected ports still exist
  auto* new_moditerm_out2 = new_inst_1->findModITerm("out2");
  ASSERT_NE(new_moditerm_out2, nullptr);
  EXPECT_EQ(new_moditerm_out2->getModNet(), nullptr);  // Was not connected

  auto* new_moditerm_unconnected_in
      = new_inst_1->findModITerm("unconnected_in");
  ASSERT_NE(new_moditerm_unconnected_in, nullptr);
  EXPECT_EQ(new_moditerm_unconnected_in->getModNet(),
            nullptr);  // Was not connected

  // Check internal connectivity of the new master
  auto* new_master_in1 = new_master->findModBTerm("in1");
  ASSERT_NE(new_master_in1, nullptr);
  auto* new_master_in1_net = new_master_in1->getModNet();
  ASSERT_NE(new_master_in1_net, nullptr);
  EXPECT_EQ(new_master_in1_net->getITerms().size(), 1);
  EXPECT_EQ((*new_master_in1_net->getITerms().begin())->getInst()->getMaster(),
            buf_master);

  auto* new_master_unconnected_in = new_master->findModBTerm("unconnected_in");
  ASSERT_NE(new_master_unconnected_in, nullptr);
  EXPECT_EQ(new_master_unconnected_in->getModNet(),
            nullptr);  // Was not connected internally
}

}  // namespace odb