// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include <cassert>
#include <cstdio>
#include <string>

#include "db_sta/dbNetwork.hh"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "sta/NetworkClass.hh"
#include "tst/IntegratedFixture.h"

namespace sta {

class TestDbSta : public tst::IntegratedFixture
{
 protected:
  TestDbSta()
      : tst::IntegratedFixture(tst::IntegratedFixture::Technology::Nangate45,
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

}  // namespace sta
