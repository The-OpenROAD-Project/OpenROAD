// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include <cstdio>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>

#include "ant/AntennaChecker.hh"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "dpl/Opendp.h"
#include "est/EstimateParasitics.h"
#include "grt/GlobalRouter.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "rsz/Resizer.hh"
#include "sta/Liberty.hh"
#include "sta/NetworkClass.hh"
#include "sta/VerilogWriter.hh"
#include "stt/SteinerTreeBuilder.h"
#include "tst/nangate45_fixture.h"
#include "utl/CallBackHandler.h"

namespace rsz {

class BufRemTest2 : public tst::Nangate45Fixture
{
 protected:
  BufRemTest2()
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
    library_ = readLiberty(prefix + "Nangate45/Nangate45_typ.lib");
    db_network_ = sta_->getDbNetwork();
    db_network_->setHierarchy();
    db_network_->setBlock(block_);
    block_->setDieArea(odb::Rect(0, 0, 1000, 1000));
    // register proper callbacks for timer like read_def
    sta_->postReadDef(block_);

    // Create top level ports
    makeBTerm(block_, "in1");
    makeBTerm(block_, "out1", {.io_type = odb::dbIoType::OUTPUT, .bpins = {}});
    makeBTerm(block_, "out2", {.io_type = odb::dbIoType::OUTPUT, .bpins = {}});

    // Create top level nets
    odb::dbNet* net1 = odb::dbNet::create(block_, "net1");
    odb::dbNet* net2 = odb::dbNet::create(block_, "net2");
    odb::dbNet* in1_net = block_->findNet("in1");
    odb::dbNet* out1_net = block_->findNet("out1");
    odb::dbNet* out2_net = block_->findNet("out2");

    // Create top level instances
    odb::dbMaster* buf_x1_master = db_->findMaster("BUF_X1");
    odb::dbMaster* buf_x2_master = db_->findMaster("BUF_X2");
    odb::dbMaster* buf_x4_master = db_->findMaster("BUF_X4");

    odb::dbInst* drvr = odb::dbInst::create(block_, buf_x1_master, "drvr");
    odb::dbInst* buf = odb::dbInst::create(block_, buf_x2_master, "buf");
    odb::dbInst* load = odb::dbInst::create(block_, buf_x4_master, "load");

    // Create MEM module
    odb::dbModule* mem_module = odb::dbModule::create(block_, "MEM");
    odb::dbModBTerm* mem_A0_port = odb::dbModBTerm::create(mem_module, "A0");
    mem_A0_port->setIoType(odb::dbIoType::INPUT);
    odb::dbModBTerm* mem_A1_port = odb::dbModBTerm::create(mem_module, "A1");
    mem_A1_port->setIoType(odb::dbIoType::INPUT);
    odb::dbModBTerm* mem_Z1_port = odb::dbModBTerm::create(mem_module, "Z1");
    mem_Z1_port->setIoType(odb::dbIoType::OUTPUT);

    // Create MEM instances
    odb::dbInst* load0 = odb::dbInst::create(
        block_, buf_x1_master, "load0", false, mem_module);
    odb::dbInst* load1 = odb::dbInst::create(
        block_, buf_x1_master, "load1", false, mem_module);

    // Wire up inside MEM
    odb::dbModNet* mem_modnet_A0 = odb::dbModNet::create(mem_module, "A0");
    mem_A0_port->connect(mem_modnet_A0);
    load0->findITerm("A")->connect(net1, mem_modnet_A0);

    odb::dbModNet* mem_modnet_A1 = odb::dbModNet::create(mem_module, "A1");
    mem_A1_port->connect(mem_modnet_A1);
    load1->findITerm("A")->connect(net2, mem_modnet_A1);

    odb::dbModNet* mem_modnet_Z1 = odb::dbModNet::create(mem_module, "Z1");
    mem_Z1_port->connect(mem_modnet_Z1);
    load1->findITerm("Z")->connect(out2_net, mem_modnet_Z1);

    // Create instance of MEM
    odb::dbModInst* mem_inst
        = odb::dbModInst::create(block_->getTopModule(), mem_module, "mem");
    odb::dbModITerm* mem_iterm_A0
        = odb::dbModITerm::create(mem_inst, "A0", mem_A0_port);
    odb::dbModITerm* mem_iterm_A1
        = odb::dbModITerm::create(mem_inst, "A1", mem_A1_port);
    odb::dbModITerm* mem_iterm_Z1
        = odb::dbModITerm::create(mem_inst, "Z1", mem_Z1_port);

    // Connect top level flat nets
    drvr->findITerm("A")->connect(in1_net);
    drvr->findITerm("Z")->connect(net1);

    buf->findITerm("A")->connect(net1);
    buf->findITerm("Z")->connect(net2);

    load->findITerm("A")->connect(net1);
    load->findITerm("Z")->connect(out1_net);

    // Hierarchical connections
    odb::dbModule* top_module = block_->getTopModule();
    odb::dbModNet* top_modnet_net1 = odb::dbModNet::create(top_module, "net1");
    drvr->findITerm("Z")->connect(top_modnet_net1);
    buf->findITerm("A")->connect(top_modnet_net1);
    load->findITerm("A")->connect(top_modnet_net1);
    mem_iterm_A0->connect(top_modnet_net1);

    odb::dbModNet* top_modnet_net2 = odb::dbModNet::create(top_module, "net2");
    buf->findITerm("Z")->connect(top_modnet_net2);
    mem_iterm_A1->connect(top_modnet_net2);

    odb::dbModNet* top_modnet_out2 = odb::dbModNet::create(top_module, "out2");
    mem_iterm_Z1->connect(top_modnet_out2);
    block_->findBTerm("out2")->connect(top_modnet_out2);

    sta_->postReadDef(block_);
  }

  stt::SteinerTreeBuilder stt_;
  utl::CallBackHandler callback_handler_;
  dpl::Opendp dp_;
  ant::AntennaChecker ant_;
  grt::GlobalRouter grt_;
  est::EstimateParasitics ep_;
  rsz::Resizer resizer_;

  sta::LibertyLibrary* library_{nullptr};
  sta::dbNetwork* db_network_{nullptr};
};

TEST_F(BufRemTest2, RemoveBuf)
{
  resizer_.initBlock();
  db_->setLogger(&logger_);

  // Write verilog and check the content before buffer removal
  const std::string before_vlog_path = "TestBufferRemoval2_before.v";
  sta::writeVerilog(before_vlog_path.c_str(), false, {}, sta_->network());

  std::ifstream file_before(before_vlog_path);
  std::string content_before((std::istreambuf_iterator<char>(file_before)),
                             std::istreambuf_iterator<char>());

  // Netlist before buffer removal:
  //
  // in1 ---- drvr(BUF_X1) --- net1 ----+---- buf(BUF_X2) ---- net2 ---- mem/A1
  //                                    |
  //                                    +------------------------------- mem/A0
  //                                    |
  //                                    +---- load(BUF_X4) ---- out1
  //
  // Inside mem (MEM module):
  //   mem/A0 ---- load0(BUF_X1)
  //   mem/A1 ---- load1(BUF_X1) ---- mem/Z1 ---- out2
  //
  const std::string expected_before_vlog = R"(module top (in1,
    out1,
    out2);
 input in1;
 output out1;
 output out2;

 wire net2;
 wire net1;

 BUF_X2 buf (.A(net1),
    .Z(net2));
 BUF_X1 drvr (.A(in1),
    .Z(net1));
 BUF_X4 load (.A(net1),
    .Z(out1));
 MEM mem (.Z1(out2),
    .A1(net2),
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

  EXPECT_EQ(content_before, expected_before_vlog);

  odb::dbInst* buf_inst = block_->findInst("buf");
  ASSERT_NE(buf_inst, nullptr);
  sta::Instance* sta_buf = db_network_->dbToSta(buf_inst);

  // Pre sanity check
  db_network_->checkAxioms();

  //----------------------------------------------------
  // Remove buffer
  //----------------------------------------------------
  auto insts = std::make_unique<sta::InstanceSeq>();
  insts->emplace_back(sta_buf);
  resizer_.removeBuffers(*insts);

  // Post sanity check
  db_network_->checkAxioms();

  // Write verilog and check the content after buffer removal
  const std::string after_vlog_path = "TestBufferRemoval2_after.v";
  sta::writeVerilog(after_vlog_path.c_str(), false, {}, sta_->network());

  std::ifstream file_after(after_vlog_path);
  std::string content_after((std::istreambuf_iterator<char>(file_after)),
                            std::istreambuf_iterator<char>());

  // Netlist after buffer removal:
  //
  // in1 ---- drvr(BUF_X1) --- net1 ----+---- mem/A1
  //                                    |
  //                                    +---- mem/A0
  //                                    |
  //                                    +---- load(BUF_X4) ---- out1
  //
  // Inside mem (MEM module):
  //   mem/A0 ---- load0(BUF_X1)
  //   mem/A1 ---- load1(BUF_X1) ---- mem/Z1 ---- out2
  const std::string expected_after_vlog = R"(module top (in1,
    out1,
    out2);
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

  EXPECT_EQ(block_->findInst("buf"), nullptr);
  EXPECT_EQ(block_->findNet("net2"), nullptr);

  odb::dbNet* net1 = block_->findNet("net1");
  ASSERT_NE(net1, nullptr);

  odb::dbModInst* mem_inst = block_->findModInst("mem");
  ASSERT_NE(mem_inst, nullptr);
  odb::dbModITerm* mem_A1_iterm = mem_inst->findModITerm("A1");
  ASSERT_NE(mem_A1_iterm, nullptr);

  odb::dbModNet* mod_net = mem_A1_iterm->getModNet();
  ASSERT_NE(mod_net, nullptr);

  bool found_drvr_Z = false;
  for (odb::dbITerm* iterm : mod_net->getITerms()) {
    if (iterm->getInst()->getName() == "drvr"
        && iterm->getMTerm()->getName() == "Z") {
      found_drvr_Z = true;
      break;
    }
  }
  EXPECT_TRUE(found_drvr_Z);

  // Clean up
  std::remove(before_vlog_path.c_str());
  std::remove(after_vlog_path.c_str());
}

}  // namespace rsz
