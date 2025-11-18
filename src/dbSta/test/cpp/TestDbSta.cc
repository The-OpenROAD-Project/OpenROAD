// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include <cassert>
#include <cstdio>
#include <fstream>
#include <memory>
#include <string>

#include "ant/AntennaChecker.hh"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbReadVerilog.hh"
#include "db_sta/dbSta.hh"
#include "dpl/Opendp.h"
#include "est/EstimateParasitics.h"
#include "grt/GlobalRouter.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "rsz/Resizer.hh"
#include "sta/Clock.hh"
#include "sta/Graph.hh"
#include "sta/NetworkClass.hh"
#include "sta/Sdc.hh"
#include "sta/SdcClass.hh"
#include "sta/Sta.hh"
#include "sta/Units.hh"
#include "sta/VerilogWriter.hh"
#include "stt/SteinerTreeBuilder.h"
#include "tst/fixture.h"
#include "utl/CallBackHandler.h"
#include "utl/Logger.h"

namespace rsz {

class TestDbSta : public tst::Fixture
{
 protected:
  void readVerilogAndSetup(const std::string& verilog_file);
  void dumpVerilogAndOdb(const std::string& name) const;

  TestDbSta()
      : stt_(db_.get(), &logger_),
        callback_handler_(&logger_),
        dp_(db_.get(), &logger_),
        ant_(db_.get(), &logger_),
        grt_(&logger_,
             &callback_handler_,
             &stt_,
             db_.get(),
             sta_.get(),
             &ant_,
             &dp_),
        ep_(&logger_, &callback_handler_, db_.get(), sta_.get(), &stt_, &grt_),
        resizer_(&logger_, db_.get(), sta_.get(), &stt_, &grt_, &dp_, &ep_)
  {
    readLiberty(getFilePath(prefix_ + "Nangate45/Nangate45_typ.lib"));
    lib_ = loadTechAndLib("Nangate45",
                          "Nangate45",
                          getFilePath(prefix_ + "Nangate45/Nangate45.lef"));

    db_->setLogger(&logger_);
    db_network_ = sta_->getDbNetwork();
    db_network_->setHierarchy();

    if (debug_) {
      logger_.setDebugLevel(utl::ODB, "DB_ECO", 3);
    }
  }

  odb::dbLib* lib_;
  odb::dbBlock* block_;
  sta::dbNetwork* db_network_;

  stt::SteinerTreeBuilder stt_;
  utl::CallBackHandler callback_handler_;
  dpl::Opendp dp_;
  ant::AntennaChecker ant_;
  grt::GlobalRouter grt_;
  est::EstimateParasitics ep_;
  rsz::Resizer resizer_;
  bool debug_ = false;  // Set to true to generate debug output

  const std::string prefix_ = "_main/src/dbSta/test/";
};

void TestDbSta::readVerilogAndSetup(const std::string& verilog_file)
{
  ord::dbVerilogNetwork verilog_network(sta_.get());
  sta::VerilogReader verilog_reader(&verilog_network);
  verilog_reader.read(getFilePath(prefix_ + "cpp/" + verilog_file).c_str());

  ord::dbLinkDesign(
      "top", &verilog_network, db_.get(), &logger_, true /*hierarchy = */);

  sta_->postReadDb(db_.get());

  block_ = db_->getChip()->getBlock();
  block_->setDefUnits(lib_->getTech()->getLefUnits());
  block_->setDieArea(odb::Rect(0, 0, 1000, 1000));
  sta_->postReadDef(block_);

  // Timing setup
  sta::Cell* top_cell = db_network_->cell(db_network_->topInstance());
  sta::Port* clk_port = db_network_->findPort(top_cell, "clk");
  sta::Pin* clk_pin
      = db_network_->findPin(db_network_->topInstance(), clk_port);

  sta::PinSet* pinset = new sta::PinSet(db_network_);
  pinset->insert(clk_pin);
  sta::PinSet* clk_pins = new sta::PinSet;
  clk_pins->insert(db_network_->dbToSta(block_->findBTerm("clk")));

  // 0.5ns
  double period = sta_->units()->timeUnit()->userToSta(0.5);
  sta::FloatSeq* waveform = new sta::FloatSeq;
  waveform->push_back(0);
  waveform->push_back(period / 2.0);

  sta_->makeClock("clk",
                  pinset,
                  /*add_to_pins=*/false,
                  /*period=*/period,
                  waveform,
                  /*comment=*/nullptr);

  sta::Sdc* sdc = sta_->sdc();
  const sta::RiseFallBoth* rf = sta::RiseFallBoth::riseFall();
  sta::Clock* clk = sdc->findClock("clk");
  const sta::RiseFall* clk_rf = sta::RiseFall::rise();

  for (odb::dbBTerm* term : block_->getBTerms()) {
    sta::Pin* pin = db_network_->dbToSta(term);
    if (pin == nullptr) {
      continue;
    }
    if (sdc->isClock(pin)) {
      continue;
    }

    odb::dbIoType io_type = term->getIoType();
    if (io_type == odb::dbIoType::INPUT) {
      sta_->setInputDelay(pin,
                          rf,
                          clk,
                          clk_rf,
                          nullptr,
                          false,
                          false,
                          sta::MinMaxAll::all(),
                          true,
                          0.0);
    } else if (io_type == odb::dbIoType::OUTPUT) {
      sta_->setOutputDelay(pin,
                           rf,
                           clk,
                           clk_rf,
                           nullptr,
                           false,
                           false,
                           sta::MinMaxAll::all(),
                           true,
                           0.0);
    }
  }

  sta_->ensureGraph();
  sta_->ensureLevelized();

  resizer_.initBlock();
  ep_.estimateWireParasitics();
}

void TestDbSta::dumpVerilogAndOdb(const std::string& name) const
{
  // Write verilog
  std::string vlog_file = name + ".v";
  sta::writeVerilog(vlog_file.c_str(), true, false, {}, sta_->network());

  // Dump ODB content
  std::ofstream orig_odb_file(name + "_odb.txt");
  block_->debugPrintContent(orig_odb_file);
  orig_odb_file.close();
}

TEST_F(TestDbSta, TestIsConnected)
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
}

}  // namespace rsz
