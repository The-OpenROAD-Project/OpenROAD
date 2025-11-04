// SPDX-License-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "ant/AntennaChecker.hh"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "dpl/Opendp.h"
#include "est/EstimateParasitics.h"
#include "grt/GlobalRouter.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "rsz/Resizer.hh"
#include "sta/Corner.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/PortDirection.hh"
#include "sta/Search.hh"
#include "sta/Sta.hh"
#include "sta/VerilogWriter.hh"
#include "stt/SteinerTreeBuilder.h"
#include "tst/nangate45_fixture.h"
#include "utl/CallBackHandler.h"
#include "utl/Logger.h"

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
    makeBTerm(block_, "out1", {.io_type = odb::dbIoType::OUTPUT});

    // Create top level nets
    odb::dbNet* net1 = odb::dbNet::create(block_, "net1");
    odb::dbNet* net2 = odb::dbNet::create(block_, "net2");
    odb::dbNet* out2 = odb::dbNet::create(block_, "out2");

    // Create top level instances
    odb::dbMaster* buf_x1_master = db_->findMaster("BUF_X1");
    odb::dbMaster* buf_x2_master = db_->findMaster("BUF_X2");
    odb::dbMaster* buf_x4_master = db_->findMaster("BUF_X4");

    odb::dbInst* drvr = odb::dbInst::create(block_, buf_x1_master, "drvr");
    odb::dbInst* buf = odb::dbInst::create(block_, buf_x2_master, "buf");
    odb::dbInst* load = odb::dbInst::create(block_, buf_x4_master, "load");
    odb::dbInst* mem_load0
        = odb::dbInst::create(block_, buf_x1_master, "mem_load0");
    // This instance represents the load that mem.B would have presented on
    // net2.
    odb::dbInst* mem_load1
        = odb::dbInst::create(block_, buf_x1_master, "mem_load1");

    // Connect top level
    odb::dbNet* in1_net = block_->findNet("in1");
    drvr->findITerm("A")->connect(in1_net);
    drvr->findITerm("Z")->connect(net1);

    buf->findITerm("A")->connect(net1);
    buf->findITerm("Z")->connect(net2);

    load->findITerm("A")->connect(net1);
    load->findITerm("Z")->connect(block_->findNet("out1"));

    mem_load0->findITerm("A")->connect(net1);

    mem_load1->findITerm("A")->connect(net2);
    mem_load1->findITerm("Z")->connect(out2);

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

  auto sort_lines = [](const std::string& text) {
    std::vector<std::string> lines;
    std::istringstream iss(text);
    std::string line;
    while (std::getline(iss, line)) {
      // Trim whitespace from the line
      line.erase(0, line.find_first_not_of(" \t\r\n"));
      line.erase(line.find_last_not_of(" \t\r\n") + 1);
      if (!line.empty()) {
        lines.push_back(line);
      }
    }
    std::sort(lines.begin(), lines.end());
    std::stringstream oss;
    for (const auto& l : lines) {
      oss << l << "\n";
    }
    return oss.str();
  };

  // Write verilog and check the content before buffer removal
  const std::string before_vlog_path = "TestBufferRemoval2_before.v";
  sta::writeVerilog(before_vlog_path.c_str(), true, false, {}, sta_->network());

  std::ifstream file_before(before_vlog_path);
  std::string content_before((std::istreambuf_iterator<char>(file_before)),
                             std::istreambuf_iterator<char>());

  const std::string expected_before_vlog = R"(module top (in1,

        out1);

    input in1;

    output out1;

    wire net1;

    wire net2;

    wire out2;

    BUF_X2 buf (.A(net1),

        .Z(net2));

    BUF_X1 drvr (.A(in1),

        .Z(net1));

    BUF_X4 load (.A(net1),

        .Z(out1));

    BUF_X1 mem_load0 (.A(net1));

    BUF_X1 mem_load1 (.A(net2),

        .Z(out2));

    endmodule

    )";

  EXPECT_EQ(sort_lines(content_before), sort_lines(expected_before_vlog));

  odb::dbInst* buf_inst = block_->findInst("buf");
  ASSERT_NE(buf_inst, nullptr);
  sta::Instance* sta_buf = db_network_->dbToSta(buf_inst);

  auto insts = std::make_unique<sta::InstanceSeq>();
  insts->emplace_back(sta_buf);
  resizer_.removeBuffers(*insts);

  // Write verilog and check the content after buffer removal
  const std::string after_vlog_path = "TestBufferRemoval2_after.v";
  sta::writeVerilog(after_vlog_path.c_str(), true, false, {}, sta_->network());

  std::ifstream file_after(after_vlog_path);
  std::string content_after((std::istreambuf_iterator<char>(file_after)),
                            std::istreambuf_iterator<char>());

  const std::string expected_after_vlog = R"(module top (in1,

        out1);

    input in1;

    output out1;

    wire net1;

    wire out2;

    BUF_X1 drvr (.A(in1),

        .Z(net1));

    BUF_X4 load (.A(net1),

        .Z(out1));

    BUF_X1 mem_load0 (.A(net1));

    BUF_X1 mem_load1 (.A(net1),

        .Z(out2));

    endmodule

    )";

  EXPECT_EQ(sort_lines(content_after), sort_lines(expected_after_vlog));

  std::remove(before_vlog_path.c_str());
  std::remove(after_vlog_path.c_str());

  EXPECT_EQ(block_->findInst("buf"), nullptr);
  EXPECT_EQ(block_->findNet("net2"), nullptr);

  odb::dbNet* net1 = block_->findNet("net1");
  ASSERT_NE(net1, nullptr);

  odb::dbInst* mem_load1_inst = block_->findInst("mem_load1");
  ASSERT_NE(mem_load1_inst, nullptr);

  odb::dbITerm* mem_load1_iterm = mem_load1_inst->findITerm("A");
  ASSERT_NE(mem_load1_iterm, nullptr);

  EXPECT_EQ(mem_load1_iterm->getNet(), net1);
}

}  // namespace rsz
