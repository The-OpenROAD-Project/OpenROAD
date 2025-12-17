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
      : tst::IntegratedFixture(tst::IntegratedFixture::Technology::Nangate45,
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
  sta::writeVerilog(after_vlog_path.c_str(), true, false, {}, sta_->network());

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
  sta::writeVerilog(after_vlog_path.c_str(), true, false, {}, sta_->network());

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
  sta::writeVerilog(after_vlog_path.c_str(), true, false, {}, sta_->network());

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
  sta::writeVerilog(after_vlog_path.c_str(), true, false, {}, sta_->network());

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
  sta::writeVerilog(after_vlog_path.c_str(), true, false, {}, sta_->network());

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
  sta::writeVerilog(after_vlog_path.c_str(), true, false, {}, sta_->network());

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

  odb::dbDatabase::undoEco(block_);

  // Dump undo ECO state
  if (debug_) {
    dumpVerilogAndOdb(test_name + "_undo_eco");
  }

  // Clean up
  removeFile(after_vlog_path);
}

}  // namespace rsz
