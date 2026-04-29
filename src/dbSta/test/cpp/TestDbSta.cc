// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include <cassert>
#include <cstdio>
#include <string>

#include "db_sta/dbNetwork.hh"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "sta/Graph.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "sta/SdcClass.hh"
#include "sta/Sta.hh"
#include "tst/IntegratedFixture.h"

namespace sta {

class TestDbSta : public tst::IntegratedFixture
{
 protected:
  TestDbSta()
      : tst::IntegratedFixture(tst::IntegratedFixture::Technology::kNangate45,
                               "_main/src/dbSta/test/")
  {
  }
};

TEST_F(TestDbSta, TestHierarchyConnectivity)
{
  std::string test_name = "TestDbSta_0";
  readVerilogAndSetup(test_name + ".v");

  Pin* sta_hier_pin = db_network_->findPin("sub_inst/mod_in");
  ASSERT_NE(sta_hier_pin, nullptr);

  odb::dbModNet* modnet = block_->findModNet("sub_inst/mod_in");
  ASSERT_NE(modnet, nullptr);

  odb::dbNet* dbnet = block_->findNet("net2");
  ASSERT_NE(dbnet, nullptr);

  Net* sta_modnet = db_network_->dbToSta(modnet);
  Net* sta_net = db_network_->dbToSta(dbnet);

  odb::dbInst* buf_inst = block_->findInst("buf");
  ASSERT_NE(buf_inst, nullptr);

  // Sanity check
  db_network_->checkAxioms();

  // Check connectivity b/w Net* and Pin*
  bool bool_return;
  bool_return = db_network_->isConnected(sta_modnet, sta_hier_pin);
  ASSERT_TRUE(bool_return);

  bool_return = db_network_->isConnected(sta_net, sta_hier_pin);
  ASSERT_TRUE(bool_return);

  // Check connectivity b/w Net* and Net*
  bool_return = db_network_->isConnected(sta_modnet, sta_net);
  ASSERT_TRUE(bool_return);

  bool_return = db_network_->isConnected(sta_net, sta_modnet);
  ASSERT_TRUE(bool_return);

  // Check Network::highestNetAbove(Net* net)
  odb::dbNet* dbnet_out2 = block_->findNet("out2");
  ASSERT_NE(dbnet_out2, nullptr);
  Net* sta_dbnet_out2 = db_network_->dbToSta(dbnet_out2);
  ASSERT_NE(sta_dbnet_out2, nullptr);
  Net* sta_highest_net = db_network_->highestNetAbove(sta_dbnet_out2);
  ASSERT_EQ(sta_highest_net, sta_dbnet_out2);

  odb::dbModNet* modnet_mod_out = block_->findModNet("sub_inst/mod_out");
  ASSERT_NE(modnet_mod_out, nullptr);
  Net* sta_modnet_mod_out = db_network_->dbToSta(modnet_mod_out);
  ASSERT_NE(sta_modnet_mod_out, nullptr);
  odb::dbModNet* modnet_out2 = block_->findModNet("out2");
  ASSERT_NE(modnet_out2, nullptr);
  Net* sta_modnet_out2 = db_network_->dbToSta(modnet_out2);
  ASSERT_NE(sta_modnet_out2, nullptr);
  Net* sta_highest_modnet_out
      = db_network_->highestNetAbove(sta_modnet_mod_out);
  ASSERT_EQ(sta_highest_modnet_out, sta_modnet_out2);

  // Check get_ports -of_object Net*
  NetTermIterator* term_iter = db_network_->termIterator(sta_dbnet_out2);
  while (term_iter->hasNext()) {
    Term* term = term_iter->next();
    Pin* pin = db_network_->pin(term);
    Port* port = db_network_->port(pin);
    ASSERT_EQ(db_network_->name(port), block_->findBTerm("out2")->getName());
  }

  // Check dbBTerm::getITerm()
  odb::dbBTerm* bterm_clk = block_->findBTerm("in1");
  ASSERT_NE(bterm_clk, nullptr);
  // There is no related dbITerm for a dbBTerm
  ASSERT_EQ(bterm_clk->getITerm(), nullptr);
}

// Regression for #10210 (stale Path* dereference in rsz).
//
// Topology (TestDbSta_StalePrevPath.v):
//   clk -> b1(BUF) -> inv1(INV) -> nd1(NAND2) -> out1
//                                   nd1/A2 <- in2
//
// Flow:
//   1. Capture drvr_path at nd1/ZN and snapshot prevPath() pointer + pin name
//   2. Delete upstream b1 + updateTiming -> free
//   3. Add a fresh BUF + clock + updateTiming -> recycle
//   4. Assert the captured Path's prev slot has been recycled: pin()
//      decodes to data that belongs to a different instance than nd1's
//      real input.
TEST_F(TestDbSta, StalePrevPath)
{
  const auto* test_info = testing::UnitTest::GetInstance()->current_test_info();
  const std::string test_name
      = std::string(test_info->test_suite_name()) + "_" + test_info->name();
  readVerilogAndSetup(test_name + ".v");
  sta_->updateTiming(true);

  Network* network = sta_->network();

  Instance* nd1 = db_network_->dbToSta(block_->findInst("nd1"));
  Path* drvr_path = sta_->vertexWorstArrivalPath(
      sta_->ensureGraph()->pinDrvrVertex(network->findPin(nd1, "ZN")),
      MinMax::max());
  ASSERT_NE(drvr_path, nullptr);
  ASSERT_EQ(network->pathName(drvr_path->pin(sta_.get())), "nd1/ZN");
  const Path* pre_addr = drvr_path->prevPath();
  ASSERT_NE(pre_addr, nullptr);
  const std::string pre_pin_name = network->pathName(pre_addr->pin(sta_.get()));

  // 2. Free upstream Path[] slots.
  sta_->deleteInstance(db_network_->dbToSta(block_->findInst("b1")));
  sta_->updateTiming(true);

  // 3. Recycle freed slots via a single fresh BUF driven by a new clock.
  odb::dbNet* in3_net = odb::dbNet::create(block_, "in3");
  odb::dbBTerm* new_bt = odb::dbBTerm::create(in3_net, "in3");
  new_bt->setIoType(odb::dbIoType::INPUT);
  odb::dbNet* nfan_net = odb::dbNet::create(block_, "nfan");
  odb::dbInst* bnew
      = odb::dbInst::create(block_, db_->findMaster("BUF_X1"), "bnew");
  bnew->findITerm("A")->connect(in3_net);
  bnew->findITerm("Z")->connect(nfan_net);

  PinSet clk2_pins(db_network_);
  clk2_pins.insert(db_network_->dbToSta(new_bt));
  FloatSeq clk2_waveform = {0.0f, 0.1f};
  sta_->makeClock(
      "clk2", clk2_pins, false, 0.2f, clk2_waveform, "", sta_->cmdMode());
  sta_->updateTiming(true);

  // 4. Staleness evidence. Pointer address is same but pin name has changed.
  const Path* post_addr = drvr_path->prevPath();
  const std::string post_pin_name
      = post_addr ? network->pathName(post_addr->pin(sta_.get()))
                  : std::string("<null>");

  EXPECT_EQ(pre_addr, post_addr)
      << "stale-pointer signature: prev_path_ address unchanged";
  EXPECT_NE(pre_pin_name, post_pin_name)
      << "but slot content should differ after free+reuse. before="
      << pre_pin_name << " after=" << post_pin_name;
}

}  // namespace sta
