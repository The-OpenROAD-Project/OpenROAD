// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include <cstdio>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>

#include "db_sta/dbSta.hh"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "sta/NetworkClass.hh"
#include "sta/VerilogWriter.hh"
#include "tst/IntegratedFixture.h"
#include "utl/Logger.h"

namespace rsz {
class BufRemTest3 : public tst::IntegratedFixture
{
 protected:
  BufRemTest3()
      : tst::IntegratedFixture(tst::IntegratedFixture::Technology::kNangate45,
                               "_main/src/rsz/test/")
  {
    if (debug_) {
      logger_.setDebugLevel(utl::ODB, "DB_EDIT", 2);
      logger_.setDebugLevel(utl::RSZ, "remove_buffer", 3);
    }
  }

  bool debug_ = false;  // Set to true to generate debug output
};

TEST_F(BufRemTest3, RemoveBufferCase9)
{
  std::string test_name = "TestBufferRemoval3_9";
  readVerilogAndSetup(test_name + ".v");

  // Netlist before buffer removal:
  //  (undriven input) -> buf1 -> out

  // Dump pre ECO state
  if (debug_) {
    dumpVerilogAndOdb(test_name + "_pre_eco");
  }

  odb::dbDatabase::beginEco(block_);

  // Pre sanity check
  sta_->updateTiming(true);
  // Do not call checkAxioms() because there is an undriven buffer.

  //----------------------------------------------------
  // Remove buffer
  //----------------------------------------------------
  auto insts = std::make_unique<sta::InstanceSeq>();
  odb::dbInst* buf_inst = block_->findInst("buf1");
  ASSERT_NE(buf_inst, nullptr);
  sta::Instance* sta_buf = db_network_->dbToSta(buf_inst);
  insts->emplace_back(sta_buf);
  resizer_.removeBuffers(*insts);

  // Post sanity check
  sta_->updateTiming(true);
  // Do not call checkAxioms() because there is an undriven buffer.

  // Write verilog and check the content after buffer removal
  const std::string after_vlog_path = test_name + "_after.v";
  sta::writeVerilog(after_vlog_path.c_str(), false, {}, sta_->network());

  std::ifstream file_after(after_vlog_path);
  std::string content_after((std::istreambuf_iterator<char>(file_after)),
                            std::istreambuf_iterator<char>());

  // Netlist after buffer removal:
  // in -> mod_inst/mod_in -> assign -> mod_inst/mod_out -> out
  const std::string expected_after_vlog = R"(module top (clk,
    in,
    out);
 input clk;
 input in;
 output out;


endmodule
)";

  EXPECT_EQ(content_after, expected_after_vlog);

  odb::dbDatabase::undoEco(block_);

  // Dump undo ECO state
  if (debug_) {
    dumpVerilogAndOdb(test_name + "_undo_eco");
  }

  // Clean up
  removeFile(after_vlog_path);
}

TEST_F(BufRemTest3, RemoveBufferCase8)
{
  std::string test_name = "TestBufferRemoval3_8";
  readVerilogAndSetup(test_name + ".v");

  // Netlist before buffer removal:
  //  (undriven input) -> buf1 -> out

  // Dump pre ECO state
  if (debug_) {
    dumpVerilogAndOdb(test_name + "_pre_eco");
  }

  odb::dbDatabase::beginEco(block_);

  // Pre sanity check
  sta_->updateTiming(true);
  // Do not call checkAxioms() because there is an undriven buffer.

  //----------------------------------------------------
  // Remove buffer
  //----------------------------------------------------
  auto insts = std::make_unique<sta::InstanceSeq>();
  odb::dbInst* buf_inst = block_->findInst("buf1");
  ASSERT_NE(buf_inst, nullptr);
  sta::Instance* sta_buf = db_network_->dbToSta(buf_inst);
  insts->emplace_back(sta_buf);
  resizer_.removeBuffers(*insts);

  // Post sanity check
  sta_->updateTiming(true);
  // Do not call checkAxioms() because there is an undriven buffer.

  // Write verilog and check the content after buffer removal
  const std::string after_vlog_path = test_name + "_after.v";
  sta::writeVerilog(after_vlog_path.c_str(), false, {}, sta_->network());

  std::ifstream file_after(after_vlog_path);
  std::string content_after((std::istreambuf_iterator<char>(file_after)),
                            std::istreambuf_iterator<char>());

  // Netlist after buffer removal:
  // in -> mod_inst/mod_in -> assign -> mod_inst/mod_out -> out
  const std::string expected_after_vlog = R"(module top (clk,
    in,
    out);
 input clk;
 input in;
 output out;


 BUF_X1 buf1 (.Z(out));
endmodule
)";

  EXPECT_EQ(content_after, expected_after_vlog);

  odb::dbDatabase::undoEco(block_);

  // Dump undo ECO state
  if (debug_) {
    dumpVerilogAndOdb(test_name + "_undo_eco");
  }

  // Clean up
  removeFile(after_vlog_path);
}

TEST_F(BufRemTest3, RemoveBufferCase7)
{
  std::string test_name = "TestBufferRemoval3_7";
  readVerilogAndSetup(test_name + ".v");

  // Netlist before buffer removal:
  //  (undriven input) -> buf1 -> buf2 -> out

  // Dump pre ECO state
  if (debug_) {
    dumpVerilogAndOdb(test_name + "_pre_eco");
  }

  odb::dbDatabase::beginEco(block_);

  // Pre sanity check
  sta_->updateTiming(true);
  // Do not call checkAxioms() because there is an undriven buffer.

  //----------------------------------------------------
  // Remove buffer
  //----------------------------------------------------
  auto insts = std::make_unique<sta::InstanceSeq>();
  odb::dbInst* buf_inst = block_->findInst("buf1");
  ASSERT_NE(buf_inst, nullptr);
  sta::Instance* sta_buf = db_network_->dbToSta(buf_inst);
  insts->emplace_back(sta_buf);
  resizer_.removeBuffers(*insts);

  // Post sanity check
  sta_->updateTiming(true);
  // Do not call checkAxioms() because there is an undriven buffer.

  // Write verilog and check the content after buffer removal
  const std::string after_vlog_path = test_name + "_after.v";
  sta::writeVerilog(after_vlog_path.c_str(), false, {}, sta_->network());

  std::ifstream file_after(after_vlog_path);
  std::string content_after((std::istreambuf_iterator<char>(file_after)),
                            std::istreambuf_iterator<char>());

  // Netlist after buffer removal:
  // in -> mod_inst/mod_in -> assign -> mod_inst/mod_out -> out
  const std::string expected_after_vlog = R"(module top (clk,
    in,
    out);
 input clk;
 input in;
 output out;

 wire n1;

 BUF_X1 buf1 (.Z(n1));
 BUF_X1 buf2 (.A(n1),
    .Z(out));
endmodule
)";

  EXPECT_EQ(content_after, expected_after_vlog);

  odb::dbDatabase::undoEco(block_);

  // Dump undo ECO state
  if (debug_) {
    dumpVerilogAndOdb(test_name + "_undo_eco");
  }

  // Clean up
  removeFile(after_vlog_path);
}

TEST_F(BufRemTest3, RemoveBufferCase6)
{
  std::string test_name = "TestBufferRemoval3_6";
  readVerilogAndSetup(test_name + ".v");

  // Netlist before buffer removal:
  // in -> mod_inst/mod_in -> mod_inst/buf0 -> mod_inst/mod_out -> out

  // Dump pre ECO state
  if (debug_) {
    dumpVerilogAndOdb(test_name + "_pre_eco");
  }

  odb::dbDatabase::beginEco(block_);

  // Pre sanity check
  sta_->updateTiming(true);
  db_network_->checkAxioms();
  sta_->checkSanity();

  //----------------------------------------------------
  // Remove buffer
  //----------------------------------------------------
  auto insts = std::make_unique<sta::InstanceSeq>();
  resizer_.removeBuffers(*insts);

  // Post sanity check
  sta_->updateTiming(true);
  db_network_->checkAxioms();
  sta_->checkSanity();

  // Write verilog and check the content after buffer removal
  const std::string after_vlog_path = test_name + "_after.v";
  sta::writeVerilog(after_vlog_path.c_str(), false, {}, sta_->network());

  std::ifstream file_after(after_vlog_path);
  std::string content_after((std::istreambuf_iterator<char>(file_after)),
                            std::istreambuf_iterator<char>());

  // Netlist after buffer removal:
  // in -> mod_inst/mod_in -> assign -> mod_inst/mod_out -> out
  const std::string expected_after_vlog = R"(module top (clk,
    in,
    out);
 input clk;
 input in;
 output out;


 MOD mod_inst (.mod_in(in),
    .mod_out(out));
endmodule
module MOD (mod_in,
    mod_out);
 input mod_in;
 output mod_out;


 assign mod_out = mod_in;
endmodule
)";

  EXPECT_EQ(content_after, expected_after_vlog);

  odb::dbDatabase::undoEco(block_);

  // Dump undo ECO state
  if (debug_) {
    dumpVerilogAndOdb(test_name + "_undo_eco");
  }

  // Clean up
  removeFile(after_vlog_path);
}

TEST_F(BufRemTest3, RemoveBufferCase5)
{
  std::string test_name = "TestBufferRemoval3_5";
  readVerilogAndSetup(test_name + ".v");

  // Netlist before buffer removal:
  // in -> buf0 -> out

  // Dump pre ECO state
  if (debug_) {
    dumpVerilogAndOdb(test_name + "_pre_eco");
  }

  odb::dbDatabase::beginEco(block_);

  // Pre sanity check
  sta_->updateTiming(true);
  db_network_->checkAxioms();
  sta_->checkSanity();

  //----------------------------------------------------
  // Remove buffer
  //----------------------------------------------------
  auto insts = std::make_unique<sta::InstanceSeq>();
  resizer_.removeBuffers(*insts);

  // removeBuffers will do nothing since buf0 is connecting two ports.

  // Post sanity check
  sta_->updateTiming(true);
  db_network_->checkAxioms();
  sta_->checkSanity();

  // Write verilog and check the content after buffer removal
  const std::string after_vlog_path = test_name + "_after.v";
  sta::writeVerilog(after_vlog_path.c_str(), false, {}, sta_->network());

  std::ifstream file_after(after_vlog_path);
  std::string content_after((std::istreambuf_iterator<char>(file_after)),
                            std::istreambuf_iterator<char>());

  // Netlist after buffer removal:
  // in -> out
  const std::string expected_after_vlog = R"(module top (clk,
    in,
    out);
 input clk;
 input in;
 output out;


 assign out = in;
endmodule
)";

  EXPECT_EQ(content_after, expected_after_vlog);

  odb::dbDatabase::undoEco(block_);

  // Dump undo ECO state
  if (debug_) {
    dumpVerilogAndOdb(test_name + "_undo_eco");
  }

  // Clean up
  removeFile(after_vlog_path);
}

TEST_F(BufRemTest3, RemoveBufferCase4)
{
  std::string test_name = "TestBufferRemoval3_4";
  readVerilogAndSetup(test_name + ".v");

  // Netlist before buffer removal:
  // DFF_X1/Q -> buf_top1 -> load_top1 -> out1
  // DFF_X1/Q -> buf_top1 -> load_top2 -> out2
  // DFF_X1/Q -> buf_top1 -> mod3_inst/load_mod3_1 -> out5
  // DFF_X1/Q -> buf_top2 -> load_top3 -> out3
  // DFF_X1/Q -> buf_top2 -> mod3_inst/load_mod3_2 -> out6
  // DFF_X1/Q -> mod2_inst/buf_mod2 -> load_top4 -> out4
  // DFF_X1/Q -> mod2_inst/buf_mod2 -> mod3_inst/load_mod3_3

  // Pre sanity check
  sta_->updateTiming(true);
  db_network_->checkAxioms();
  sta_->checkSanity();

  // Dump pre ECO state
  if (debug_) {
    dumpVerilogAndOdb(test_name + "_pre_eco");
  }

  odb::dbDatabase::beginEco(block_);

  //----------------------------------------------------
  // Remove buffer
  //----------------------------------------------------
  auto insts = std::make_unique<sta::InstanceSeq>();
  resizer_.removeBuffers(*insts);

  // Post sanity check
  sta_->updateTiming(true);
  db_network_->checkAxioms();
  sta_->checkSanity();

  // Write verilog and check the content after buffer removal
  const std::string after_vlog_path = test_name + "_after.v";
  sta::writeVerilog(after_vlog_path.c_str(), false, {}, sta_->network());

  std::ifstream file_after(after_vlog_path);
  std::string content_after((std::istreambuf_iterator<char>(file_after)),
                            std::istreambuf_iterator<char>());

  // Netlist after buffer removal:
  // DFF_X1/Q -> out1, out2, out3, out4, out5, out6
  const std::string expected_after_vlog = R"(module top (clk,
    in1,
    out1,
    out2,
    out3,
    out4,
    out5,
    out6);
 input clk;
 input in1;
 output out1;
 output out2;
 output out3;
 output out4;
 output out5;
 output out6;


 MOD1 mod1_inst (.clk_in(clk),
    .d_in(in1),
    .q_out(out1));
 MOD2 mod2_inst (.in(out1),
    .out(out4));
 MOD3 mod3_inst (.in1(out1),
    .in2(out1),
    .in3(out4),
    .out1(out5),
    .out2(out6));
 assign out2 = out1;
 assign out3 = out1;
endmodule
module MOD1 (clk_in,
    d_in,
    q_out);
 input clk_in;
 input d_in;
 output q_out;


 DFF_X1 drvr (.D(d_in),
    .CK(clk_in),
    .Q(q_out));
endmodule
module MOD2 (in,
    out);
 input in;
 output out;


 assign out = in;
endmodule
module MOD3 (in1,
    in2,
    in3,
    out1,
    out2);
 input in1;
 input in2;
 input in3;
 output out1;
 output out2;


 assign out2 = in2;
 assign out1 = in1;
endmodule
)";

  EXPECT_EQ(content_after, expected_after_vlog);

  odb::dbDatabase::undoEco(block_);

  // Dump undo ECO state
  if (debug_) {
    dumpVerilogAndOdb(test_name + "_undo_eco");
  }

  // Clean up
  removeFile(after_vlog_path);
}

TEST_F(BufRemTest3, RemoveBufferCase3)
{
  std::string test_name = "TestBufferRemoval3_3";
  readVerilogAndSetup(test_name + ".v");

  // Netlist before buffer removal:
  // DFF_X1/Q -> buf0 -> mod_inst/buf1 -> buf2 -> out1

  odb::dbInst* buf_inst = block_->findInst("mod_inst/buf1");
  ASSERT_NE(buf_inst, nullptr);
  sta::Instance* sta_buf = db_network_->dbToSta(buf_inst);

  // Dump pre ECO state
  if (debug_) {
    dumpVerilogAndOdb(test_name + "_pre_eco");
  }

  odb::dbDatabase::beginEco(block_);

  // Pre sanity check
  sta_->updateTiming(true);
  db_network_->checkAxioms();
  sta_->checkSanity();

  //----------------------------------------------------
  // Remove buffer
  //----------------------------------------------------
  auto insts = std::make_unique<sta::InstanceSeq>();
  insts->emplace_back(sta_buf);
  resizer_.removeBuffers(*insts);

  // Post sanity check
  sta_->updateTiming(true);
  db_network_->checkAxioms();
  sta_->checkSanity();

  // Write verilog and check the content after buffer removal
  const std::string after_vlog_path = test_name + "_after.v";
  sta::writeVerilog(after_vlog_path.c_str(), false, {}, sta_->network());

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

  odb::dbDatabase::undoEco(block_);

  // Dump undo ECO state
  if (debug_) {
    dumpVerilogAndOdb(test_name + "_undo_eco");
  }

  // Clean up
  removeFile(after_vlog_path);
}

TEST_F(BufRemTest3, RemoveBufferCase2)
{
  std::string test_name = "TestBufferRemoval3_2";
  readVerilogAndSetup(test_name + ".v");

  odb::dbInst* buf_inst = block_->findInst("sub_inst/buf");
  ASSERT_NE(buf_inst, nullptr);
  sta::Instance* sta_buf = db_network_->dbToSta(buf_inst);

  // Dump pre ECO state
  if (debug_) {
    dumpVerilogAndOdb(test_name + "_pre_eco");
  }

  odb::dbDatabase::beginEco(block_);

  // Pre sanity check
  sta_->updateTiming(true);
  db_network_->checkAxioms();
  sta_->checkSanity();

  //----------------------------------------------------
  // Remove buffer
  //----------------------------------------------------
  auto insts = std::make_unique<sta::InstanceSeq>();
  insts->emplace_back(sta_buf);
  resizer_.removeBuffers(*insts);

  // Post sanity check
  sta_->updateTiming(true);
  db_network_->checkAxioms();
  sta_->checkSanity();

  // Write verilog and check the content after buffer removal
  const std::string after_vlog_path = test_name + "_after.v";
  sta::writeVerilog(after_vlog_path.c_str(), false, {}, sta_->network());

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

  odb::dbDatabase::undoEco(block_);

  // Dump undo ECO state
  if (debug_) {
    dumpVerilogAndOdb(test_name + "_undo_eco");
  }

  // Clean up
  removeFile(after_vlog_path);
}

TEST_F(BufRemTest3, RemoveBufferCase1)
{
  std::string test_name = "TestBufferRemoval3_1";
  readVerilogAndSetup(test_name + ".v");

  odb::dbInst* buf_inst = block_->findInst("buf");
  ASSERT_NE(buf_inst, nullptr);
  sta::Instance* sta_buf = db_network_->dbToSta(buf_inst);

  // Dump pre ECO state
  if (debug_) {
    dumpVerilogAndOdb(test_name + "_pre_eco");
  }

  odb::dbDatabase::beginEco(block_);

  // Pre sanity check
  sta_->updateTiming(true);
  db_network_->checkAxioms();
  sta_->checkSanity();

  //----------------------------------------------------
  // Remove buffer
  //----------------------------------------------------
  auto insts = std::make_unique<sta::InstanceSeq>();
  insts->emplace_back(sta_buf);
  resizer_.removeBuffers(*insts);

  // Post sanity check
  sta_->updateTiming(true);
  db_network_->checkAxioms();
  sta_->checkSanity();

  // Write verilog and check the content after buffer removal
  const std::string after_vlog_path = test_name + "_after.v";
  sta::writeVerilog(after_vlog_path.c_str(), false, {}, sta_->network());

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

  odb::dbDatabase::undoEco(block_);

  // Dump undo ECO state
  if (debug_) {
    dumpVerilogAndOdb(test_name + "_undo_eco");
  }

  // Clean up
  removeFile(after_vlog_path);
}

TEST_F(BufRemTest3, RemoveBufferCase0)
{
  std::string test_name = "TestBufferRemoval3_0";
  readVerilogAndSetup(test_name + ".v");

  odb::dbModNet* modnet = block_->findModNet("mem/A0");
  ASSERT_NE(modnet, nullptr);

  odb::dbInst* buf_inst = block_->findInst("buf");
  ASSERT_NE(buf_inst, nullptr);
  sta::Instance* sta_buf = db_network_->dbToSta(buf_inst);

  // Dump pre ECO state
  if (debug_) {
    dumpVerilogAndOdb(test_name + "_pre_eco");
  }

  odb::dbDatabase::beginEco(block_);

  // Pre sanity check
  sta_->updateTiming(true);
  db_network_->checkAxioms();
  sta_->checkSanity();

  //----------------------------------------------------
  // Remove buffer
  //----------------------------------------------------
  auto insts = std::make_unique<sta::InstanceSeq>();
  insts->emplace_back(sta_buf);
  resizer_.removeBuffers(*insts);

  // Post sanity check
  sta_->updateTiming(true);
  db_network_->checkAxioms();
  sta_->checkSanity();

  // Write verilog and check the content after buffer removal
  const std::string after_vlog_path = test_name + "_after.v";
  sta::writeVerilog(after_vlog_path.c_str(), false, {}, sta_->network());

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

  odb::dbDatabase::undoEco(block_);

  // Dump undo ECO state
  if (debug_) {
    dumpVerilogAndOdb(test_name + "_undo_eco");
  }

  // Clean up
  removeFile(after_vlog_path);
}

// Regression test for feedthrough buffer removal
//
// Design: top has a register driving child_mod's data_i (internal wire),
//   and child_mod's data_o connects to a top-level output port.
//   Inside child_mod, data_i -> BUF_X1 -> data_o (feedthrough buffer).
//
// When remove_buffers removes the feedthrough buffer, the buffer
// removal logic detects the feedthrough and keeps the input ModNet
// as the survivor.  This ensures VerilogWriter emits
// "assign data_o = data_i;" correctly.
TEST_F(BufRemTest3, FeedthroughAssign)
{
  std::string test_name = "TestBufferRemoval3_feedthrough";
  readVerilogAndSetup(test_name + ".v", /*init_default_sdc=*/false);

  // Verify the feedthrough buffer exists before removal
  odb::dbModule* child_mod = block_->findModule("child_mod");
  ASSERT_NE(child_mod, nullptr);
  odb::dbInst* buf_inst = block_->findInst("u_child/u_ft");
  ASSERT_NE(buf_inst, nullptr) << "Feedthrough buffer u_child/u_ft not found";

  // Before remove_buffers: two separate ModNets
  odb::dbModBTerm* bt_in = child_mod->findModBTerm("data_i");
  odb::dbModBTerm* bt_out = child_mod->findModBTerm("data_o");
  ASSERT_NE(bt_in, nullptr);
  ASSERT_NE(bt_out, nullptr);
  EXPECT_NE(bt_in->getModNet(), bt_out->getModNet())
      << "ModNets should be separate before remove_buffers";

  // Run remove_buffers — the buffer removal logic detects the
  // feedthrough and forces the input ModNet to survive.
  resizer_.removeBuffers({});

  // After remove_buffers: buffer is gone
  EXPECT_EQ(block_->findInst("u_child/u_ft"), nullptr)
      << "Feedthrough buffer should be removed";

  // write_verilog should emit "assign data_o = data_i;" for the
  // feedthrough since port_name("data_o") != net_name("data_i").
  writeAndCompareVerilogOutputFile(test_name, test_name + "_post.v");
}

// Regression test for a feedthrough buffer whose two sides sit at different
// hierarchy depths, where remove_buffers leaves a kept sub-module output port
// without any driver.
//
// Design:
//   top
//   +- u_wrap (wrap_mod)
//   |    +- u_blk (blk_mod)
//   |         +- u_drv    (drv_mod)  NOR2_X1 g_drv drives drv_o
//   |         +- u_ft_mod (ft_mod)   ft_i -> BUF_X1 u_ft -> ft_o
//   +- u_sink (sink_mod)             sink_i -> OR2_X1 g_sink .A1
//
// Unlike FeedthroughAssign, the buffer output leaves ft_mod, blk_mod and
// wrap_mod before it re-enters sink_mod, so the two flat nets around the
// buffer end up at very different depths:
//   input  side  u_wrap/u_blk/drv_to_ft
//   output side  mid                      (top level)
//
// removeBuffer() correctly keeps the input ModNet as the survivor for the
// feedthrough, but because the surviving flat net is the deeper one it then
// renames the surviving ModNet to the removed one's name, i.e. to the output
// port name "ft_o".  VerilogWriter now sees output port "ft_o" sitting on a
// net also called "ft_o" and emits nothing, so the "assign ft_o = ft_i;"
// bridge is lost and ft_mod comes out with an undriven output port.
//
// The flat dbNet merge itself is correct, which is why placement and routing
// never notice the problem and only LEC or gate level simulation fails.
//
// NOTE: this test fails on current master by design.  The golden file holds
// the netlist that a correct remove_buffers has to produce.
TEST_F(BufRemTest3, HierFeedthroughAcrossLevels)
{
  std::string test_name = "TestBufferRemoval3_hier_feedthrough";
  readVerilogAndSetup(test_name + ".v", /*init_default_sdc=*/false);

  odb::dbModule* ft_mod = block_->findModule("ft_mod");
  ASSERT_NE(ft_mod, nullptr);
  ASSERT_NE(block_->findInst("u_wrap/u_blk/u_ft_mod/u_ft"), nullptr)
      << "Feedthrough buffer u_ft not found";

  // The leaf driver and the leaf load at the two ends of the whole path.
  odb::dbInst* drv_inst = block_->findInst("u_wrap/u_blk/u_drv/g_drv");
  odb::dbInst* sink_inst = block_->findInst("u_sink/g_sink");
  ASSERT_NE(drv_inst, nullptr);
  ASSERT_NE(sink_inst, nullptr);
  odb::dbITerm* drv_iterm = drv_inst->findITerm("ZN");
  odb::dbITerm* sink_iterm = sink_inst->findITerm("A1");
  ASSERT_NE(drv_iterm, nullptr);
  ASSERT_NE(sink_iterm, nullptr);

  odb::dbModBTerm* bt_in = ft_mod->findModBTerm("ft_i");
  odb::dbModBTerm* bt_out = ft_mod->findModBTerm("ft_o");
  ASSERT_NE(bt_in, nullptr);
  ASSERT_NE(bt_out, nullptr);

  // Before removal the buffer separates the two flat nets as well as the two
  // boundary ModNets of ft_mod.  The depth asymmetry is what triggers the
  // survivor rename inside removeBuffer().
  EXPECT_NE(drv_iterm->getNet(), sink_iterm->getNet());
  EXPECT_NE(bt_in->getModNet(), bt_out->getModNet())
      << "ModNets should be separate before remove_buffers";
  EXPECT_EQ(drv_iterm->getNet()->getName(), "u_wrap/u_blk/drv_to_ft");
  EXPECT_EQ(sink_iterm->getNet()->getName(), "mid");

  if (debug_) {
    std::cout << "pre  ft_i modnet: " << bt_in->getModNet()->getName() << "\n";
    std::cout << "pre  ft_o modnet: " << bt_out->getModNet()->getName() << "\n";
  }

  resizer_.removeBuffers({});

  EXPECT_EQ(block_->findInst("u_wrap/u_blk/u_ft_mod/u_ft"), nullptr)
      << "Feedthrough buffer should be removed";

  // Flat view: the merge is correct, the leaf driver and the leaf load share
  // one dbNet.  This part already works today.
  EXPECT_EQ(drv_iterm->getNet(), sink_iterm->getNet())
      << "flat dbNets should be merged after buffer removal";

  // Hierarchical view: both boundary terminals stay bound to one ModNet, so
  // the database itself is consistent.  What breaks is the name that ModNet
  // is given, because VerilogWriter can only express the feedthrough when the
  // net name differs from the output port name.
  odb::dbModNet* in_modnet = bt_in->getModNet();
  odb::dbModNet* out_modnet = bt_out->getModNet();
  ASSERT_NE(in_modnet, nullptr) << "ft_i lost its dbModNet";
  ASSERT_NE(out_modnet, nullptr) << "ft_o lost its dbModNet";
  EXPECT_EQ(in_modnet, out_modnet)
      << "ft_mod boundary ModNets should be merged after buffer removal";
  if (debug_) {
    std::cout << "post ft_i modnet: " << in_modnet->getName() << "\n";
    std::cout << "post ft_o modnet: " << out_modnet->getName() << "\n";
  }
  EXPECT_NE(in_modnet->getName(), std::string("ft_o"))
      << "surviving ModNet must not take the output port name, otherwise "
         "write_verilog drops the feedthrough assign";

  // The emitted netlist must keep ft_mod's output port driven, which requires
  // "assign ft_o = ft_i;" inside ft_mod.
  writeAndCompareVerilogOutputFile(test_name, test_name + "_post.v");
}

// Insert a buffer across sibling hierarchies so the insertion API creates the
// sink input port and a deeper source-side flat net before buffer removal.
TEST_F(BufRemTest3, HierInputPortNamePreservedAfterBufferRemoval)
{
  const std::string test_name = "TestBufferRemoval3_hier_input_port";
  readVerilogAndSetup(test_name + ".v", /*init_default_sdc=*/false);

  odb::dbITerm* driver = block_->findITerm("source/driver/u_drv/ZN");
  odb::dbITerm* load0 = block_->findITerm("sink/load0/u_inv/A");
  odb::dbITerm* load1 = block_->findITerm("sink/load1/u_inv/A");
  ASSERT_NE(driver, nullptr);
  ASSERT_NE(load0, nullptr);
  ASSERT_NE(load1, nullptr);
  EXPECT_EQ(driver->getNet()->getName(), "signal");
  EXPECT_EQ(load0->getNet()->getName(), "seed");
  EXPECT_EQ(load1->getNet()->getName(), "seed");

  odb::dbMaster* buffer_master = db_->findMaster("BUF_X2");
  ASSERT_NE(buffer_master, nullptr);
  odb::PtrSet<odb::dbObject> loads;
  loads.insert(load0);
  loads.insert(load1);
  odb::dbInst* buffer
      = resizer_.insertBufferBeforeLoads(driver->getNet(),
                                         loads,
                                         buffer_master,
                                         nullptr,
                                         "u_buf",
                                         "load_net",
                                         odb::dbNameUniquifyType::IF_NEEDED,
                                         /*loads_on_diff_nets=*/true);
  ASSERT_NE(buffer, nullptr);
  EXPECT_EQ(buffer, block_->findInst("sink/u_buf"));

  odb::dbITerm* buffer_input = buffer->findITerm("A");
  odb::dbITerm* buffer_output = buffer->findITerm("Z");
  ASSERT_NE(buffer_input, nullptr);
  ASSERT_NE(buffer_output, nullptr);
  odb::dbNet* input_net = buffer_input->getNet();
  odb::dbNet* output_net = buffer_output->getNet();
  ASSERT_NE(input_net, nullptr);
  ASSERT_NE(output_net, nullptr);
  EXPECT_EQ(input_net->getName(), "source/driver/out");
  EXPECT_EQ(output_net->getName(), "sink/load_net");
  EXPECT_TRUE(input_net->isDeeperThan(output_net));

  odb::dbModule* sink = block_->findModule("Sink");
  ASSERT_NE(sink, nullptr);
  odb::dbModBTerm* input_port = sink->findModBTerm("net");
  ASSERT_NE(input_port, nullptr);
  odb::dbModNet* input_modnet = input_port->getModNet();
  odb::dbModNet* output_modnet = buffer_output->getModNet();
  ASSERT_NE(input_modnet, nullptr);
  ASSERT_NE(output_modnet, nullptr);
  EXPECT_NE(input_modnet, output_modnet);
  EXPECT_TRUE(output_modnet->getModBTerms().empty());
  EXPECT_FALSE(output_modnet->getModITerms().empty());
  EXPECT_EQ(input_modnet->getName(), "net");
  EXPECT_EQ(output_modnet->getName(), "load_net");

  sta::Instance* sta_buffer = db_network_->dbToSta(buffer);
  ASSERT_NE(sta_buffer, nullptr);
  sta::InstanceSeq buffers;
  buffers.push_back(sta_buffer);
  resizer_.removeBuffers(buffers);
  EXPECT_EQ(block_->findInst("sink/u_buf"), nullptr);

  sta_->updateTiming(true);
  EXPECT_EQ(db_network_->checkAxioms() + sta_->checkSanity(), 0);
  EXPECT_EQ(load0->getNet(), driver->getNet());
  EXPECT_EQ(load1->getNet(), driver->getNet());

  ASSERT_NE(input_port->getModNet(), nullptr);
  EXPECT_EQ(std::string(input_port->getModNet()->getName()), "net")
      << "the surviving input-port ModNet must retain the port name";

  writeAndCompareVerilogOutputFile(test_name, test_name + "_post.v");
}

}  // namespace rsz
