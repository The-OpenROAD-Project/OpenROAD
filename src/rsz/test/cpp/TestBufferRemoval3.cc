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
#include "tst/fixture.h"
#include "utl/CallBackHandler.h"
#include "utl/Logger.h"

namespace rsz {

class BufRemTest3 : public tst::Fixture
{
 protected:
  void readVerilogAndSetup(const std::string& verilog_file);

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
    static const std::string prefix("_main/src/rsz/test/");

    readLiberty(getFilePath(prefix + "Nangate45/Nangate45_typ.lib"));
    lib_ = loadTechAndLib("Nangate45",
                          "Nangate45",
                          getFilePath(prefix + "Nangate45/Nangate45.lef"));

    db_->setLogger(&logger_);
    db_network_ = sta_->getDbNetwork();
    db_network_->setHierarchy();
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

void BufRemTest3::readVerilogAndSetup(const std::string& verilog_file)
{
  static const std::string prefix("_main/src/rsz/test/");

  ord::dbVerilogNetwork verilog_network(sta_.get());
  sta::VerilogReader verilog_reader(&verilog_network);
  verilog_reader.read(getFilePath(prefix + "cpp/" + verilog_file).c_str());

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

TEST_F(BufRemTest3, RemoveBuf)
{
  readVerilogAndSetup("TestBufferRemoval3.v");

  odb::dbModNet* modnet = block_->findModNet("mem/A0");
  ASSERT_NE(modnet, nullptr);

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

  // Write verilog and check the content after buffer removal
  const std::string after_vlog_path = "TestBufferRemoval3_after.v";
  sta::writeVerilog(after_vlog_path.c_str(), true, false, {}, sta_->network());

  std::ifstream file_after(after_vlog_path);
  std::string content_after((std::istreambuf_iterator<char>(file_after)),
                            std::istreambuf_iterator<char>());

  const std::string expected_after_vlog = R"(module top (clk,
    in1,
    out1,
    out2);
 input clk;
 input in1;
 output out1;
 output out2;

 wire net1;

 BUF_X1 drvr (.A(in1),
    .Z(net1));
 BUF_X4 load (.A(net1),
    .Z(out1));
 MEM mem (.Z1(out2),
    .A1(net1),
    .A0(net1));
endmodule
module MEM (Z1,
    A1,
    A0);
 output Z1;
 input A1;
 input A0;


 BUF_X1 load0 (.A(A0));
 BUF_X1 load1 (.A(A1),
    .Z(Z1));
endmodule
)";

  EXPECT_EQ(content_after, expected_after_vlog);

  // Clean up
  std::remove(after_vlog_path.c_str());
}

TEST_F(BufRemTest3, RemoveBuf1)
{
  readVerilogAndSetup("TestBufferRemoval3_1.v");

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

  // Write verilog and check the content after buffer removal
  const std::string after_vlog_path = "TestBufferRemoval3_1_after.v";
  sta::writeVerilog(after_vlog_path.c_str(), true, false, {}, sta_->network());

  std::ifstream file_after(after_vlog_path);
  std::string content_after((std::istreambuf_iterator<char>(file_after)),
                            std::istreambuf_iterator<char>());

  const std::string expected_after_vlog = R"(module top (clk,
    in1,
    out1,
    out2);
 input clk;
 input in1;
 output out1;
 output out2;

 wire net1;

 BUF_X1 drvr (.A(in1),
    .Z(net1));
 BUF_X4 load (.A(net1),
    .Z(out1));
 SUBMOD sub_inst (.in(net1),
    .out(out2));
endmodule
module SUBMOD (in,
    out);
 input in;
 output out;


 BUF_X1 load0 (.A(in),
    .Z(out));
endmodule
)";

  EXPECT_EQ(content_after, expected_after_vlog);

  // Clean up
  std::remove(after_vlog_path.c_str());
}

TEST_F(BufRemTest3, RemoveBuf2)
{
  readVerilogAndSetup("TestBufferRemoval3_2.v");

  odb::dbInst* buf_inst = block_->findInst("sub_inst/buf");
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

  // Write verilog and check the content after buffer removal
  const std::string after_vlog_path = "TestBufferRemoval3_2_after.v";
  sta::writeVerilog(after_vlog_path.c_str(), true, false, {}, sta_->network());

  std::ifstream file_after(after_vlog_path);
  std::string content_after((std::istreambuf_iterator<char>(file_after)),
                            std::istreambuf_iterator<char>());

  const std::string expected_after_vlog = R"(module top (clk,
    in1,
    out1);
 input clk;
 input in1;
 output out1;

 wire sub_out;

 BUF_X4 load (.A(sub_out),
    .Z(out1));
 SUBMOD sub_inst (.in(in1),
    .out(sub_out));
endmodule
module SUBMOD (in,
    out);
 input in;
 output out;


 BUF_X1 load0 (.A(in),
    .Z(out));
endmodule
)";

  EXPECT_EQ(content_after, expected_after_vlog);

  // Clean up
  std::remove(after_vlog_path.c_str());
}

TEST_F(BufRemTest3, RemoveBuf3)
{
  readVerilogAndSetup("TestBufferRemoval3_3.v");

  // Netlist before buffer removal:
  // DFF_X1/Q -> buf0 -> mod_inst/buf1 -> buf2 -> out1

  odb::dbInst* buf_inst = block_->findInst("mod_inst/buf1");
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

  // Write verilog and check the content after buffer removal
  const std::string after_vlog_path = "TestBufferRemoval3_3_after.v";
  sta::writeVerilog(after_vlog_path.c_str(), true, false, {}, sta_->network());

  std::ifstream file_after(after_vlog_path);
  std::string content_after((std::istreambuf_iterator<char>(file_after)),
                            std::istreambuf_iterator<char>());

  // Netlist after buffer removal:
  // DFF_X1/Q -> buf0 -> buf2 -> out1
  const std::string expected_after_vlog = R"(module top (clk,
    in1,
    out1);
 input clk;
 input in1;
 output out1;

 wire buf1_out;
 wire buf0_out;
 wire dff_q;

 BUF_X1 buf0 (.A(dff_q),
    .Z(buf0_out));
 BUF_X1 buf2 (.A(buf1_out),
    .Z(out1));
 DFF_X1 dff_inst (.D(in1),
    .CK(clk),
    .Q(dff_q));
 MOD mod_inst (.in(buf0_out),
    .out(buf1_out));
endmodule
module MOD (in,
    out);
 input in;
 output out;


 assign out = in;
endmodule
)";

  EXPECT_EQ(content_after, expected_after_vlog);

  // Clean up
  std::remove(after_vlog_path.c_str());
}

}  // namespace rsz
