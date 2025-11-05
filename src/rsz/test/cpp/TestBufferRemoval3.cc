// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

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
#include "sta/Corner.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/PortDirection.hh"
#include "sta/Sdc.hh"
#include "sta/Search.hh"
#include "sta/Sta.hh"
#include "sta/Units.hh"
#include "sta/VerilogWriter.hh"
#include "stt/SteinerTreeBuilder.h"
#include "utl/CallBackHandler.h"
#include "utl/Logger.h"

namespace rsz {

class BufRemTest3 : public tst::Fixture
{
 protected:
  BufRemTest3()
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
    const std::string prefix("_main/src/rsz/test/");
    readLiberty(prefix + "Nangate45/Nangate45_typ.lib");
    lib_ = loadTechAndLib("ng45", "ng45", "_main/test/Nangate45/Nangate45.lef");

    db_->setLogger(&logger_);
    db_network_ = sta_->getDbNetwork();
    db_network_->setHierarchy();

    ord::dbVerilogNetwork verilog_network(sta_.get());
    sta::VerilogReader verilog_reader(&verilog_network);
    verilog_reader.read(getFilePath("cpp/TestBufferRemoval3.v").c_str());

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

    sta_->setInputDelay(db_network_->dbToSta(block_->findBTerm("in1")),
                        rf,
                        clk,
                        clk_rf,
                        nullptr,
                        false,
                        false,
                        sta::MinMaxAll::all(),
                        true,
                        0.0);
    sta_->setOutputDelay(db_network_->dbToSta(block_->findBTerm("out1")),
                         rf,
                         clk,
                         clk_rf,
                         nullptr,
                         false,
                         false,
                         sta::MinMaxAll::all(),
                         true,
                         0.0);
    sta_->setOutputDelay(db_network_->dbToSta(block_->findBTerm("out2")),
                         rf,
                         clk,
                         clk_rf,
                         nullptr,
                         false,
                         false,
                         sta::MinMaxAll::all(),
                         true,
                         0.0);

    sta_->ensureGraph();
    sta_->ensureLevelized();

    resizer_.initBlock();
    ep_.estimateWireParasitics();
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
};

TEST_F(BufRemTest3, RemoveBuf)
{
  odb::dbInst* buf_inst = block_->findInst("buf");
  ASSERT_NE(buf_inst, nullptr);
  sta::Instance* sta_buf = db_network_->dbToSta(buf_inst);

  // Pre sanity check
  sta_->updateTiming(true);
  db_network_->checkAxioms();

  //----------------------------------------------------
  // Remove buffer
  //----------------------------------------------------
  auto insts = std::make_unique<sta::InstanceSeq>();
  insts->emplace_back(sta_buf);
  resizer_.removeBuffers(*insts);

  // Post sanity check
  sta_->updateTiming(true);
  db_network_->checkAxioms();
}

}  // namespace rsz
