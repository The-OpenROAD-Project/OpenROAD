#include <fstream>
#include <iterator>
#include <set>
#include <string>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbObject.h"
#include "sta/VerilogWriter.hh"
#include "tst/IntegratedFixture.h"

namespace odb {

class TestInsertBuffer : public tst::IntegratedFixture
{
 public:
  TestInsertBuffer()
      : tst::IntegratedFixture(tst::IntegratedFixture::Technology::Nangate45,
                               "_main/src/rsz/test/")
  {
  }

 protected:
  void SetUp() override
  {
    // IntegratedFixture handles library loading and basic setup.
    odb::dbChip* chip = odb::dbChip::create(db_.get(), db_->getTech());
    block_ = odb::dbBlock::create(chip, "top");
    sta_->postReadDef(block_);
  }
};

//
// insertBufferBeforeLoad() Case1
//
// This test case constructs a hierarchical netlist. The top-level module
// contains a constant driver (drvr_inst), two buffer loads (load0_inst,
// load2_inst), a top-level output port (load_output), and a hierarchical
// instance (mi0).
//
// The single top-level net "net" connects the driver to all these loads.
// The connection to mi0 propagates down through the hierarchy (mi0 -> mi1)
// to eventually drive another buffer load (load1_inst) inside the MOD1 module.
//
// [Pre ECO]
//
//                      +-----------+
//                      | LOGIC0_X1 |
//                      | drvr_inst |-----.
//                      +-----------+     |
//                            .Z          | (net "net")
//      +-------------------+-------------+------------------+
//      |                   |             |                  |
//      | .A                | .A          |                  |
// +----v------+       +----v------+      |             +----v-------+
// |  BUF_X1   |       |  BUF_X1   |      |             | Top Output |
// | load0_inst|       | load2_inst|      |             | load_output|
// +-----------+       +-----------+      |             +------------+
//                                        | .A
//                           +------------v----------------------+
//                           | MOD0 mi0   |                      |
//                           |            | .A                   |
//                           |   +--------v------------------+   |
//                           |   | MOD1   |                  |   |
//                           |   | mi1    | .A               |   |
//                           |   |    +---v---------------+  |   |
//                           |   |    |  BUF_X1           |  |   |
//                           |   |    | mi0/mi1/load1_inst|  |   |
//                           |   |    +-------------------+  |   |
//
// The test then proceeds to insert buffers one by one before each load:
// 1. Before `load0_inst` (in the top module).
// 2. Before `load1_inst` (inside the `mi0/mi1` hierarchy).
// 3. Before `load2_inst` (in the top module).
// 4. Before the top-level port `load_output`.
//
// After each insertion, it verifies the correctness of the resulting netlist
// by writing it out to a Verilog file and comparing it against an expected
// output.
//
// [Post ECO]
//                      +-----------+
//                      | LOGIC0_X1 |
//                      | drvr_inst |------.
//                      +-----------+      |
//                            .Z           | (net "net")
//      +---------------------+------------+-----------------+
// (NEW)| .A           (NEW)  | .A         | .A       (NEW)  | .A
// +----v------+         +----v------+     |            +----v------+
// |  BUF_X4   |         |  BUF_X4   |     |            |  BUF_X4   |
// |    buf    |         |   buf_1   |     |            |   buf_2   |
// +-----------+         +-----------+     |            +-----------+
//      | .Z                  | .Z         |                 | .Z
//      | (net_load)          |            |                 | (load_output)
//      |                     |(net_load_1)|                 |
// +----v------+         +----v------+     |          +----v-------+
// |  BUF_X1   |         |  BUF_X1   |     |          | Top Output |
// | load0_inst|         | load2_inst|     |          | load_output|
// +-----------+         +-----------+     |          +------------+
//                                         |
//                                         | .A
//                            +------------v----------------------+
//                            | MOD0 mi0   |                      |
//                            |            | .A                   |
//                            |   +--------v------------------+   |
//                            |   | MOD1   |                  |   |
//                            |   | mi1    | .A   (NEW)       |   |
//                            |   |    +---v---------------+  |   |
//                            |   |    |  BUF_X4           |  |   |
//                            |   |    | mi0/mi1/buf       |  |   |
//                            |   |    +----------+--------+  |   |
//                            |   |        |                  |   |
//                            |   |    +---v---------------+  |   |
//                            |   |    |  BUF_X1           |  |   |
//                            |   |    | mi0/mi1/load1_inst|  |   |
//                            |   |    +-------------------+  |   |
//
TEST_F(TestInsertBuffer, BeforeLoad_Case1)
{
  int num_warning = 0;
  dbMaster* buffer_master = db_->findMaster("BUF_X4");
  ASSERT_TRUE(buffer_master);

  // Create masters
  dbMaster* buf_master = db_->findMaster("BUF_X1");
  ASSERT_TRUE(buf_master);

  dbModule* mod1 = dbModule::create(block_, "MOD1");
  ASSERT_TRUE(mod1);
  dbModBTerm::create(mod1, "A");

  dbModule* mod0 = dbModule::create(block_, "MOD0");
  ASSERT_TRUE(mod0);
  dbModBTerm::create(mod0, "A");

  // TIELO master cell for Nangate45
  dbMaster* tielo_master = db_->findMaster("LOGIC0_X1");

  // Create instances, changing drvr_inst's master to TIELO
  dbInst* drvr_inst = dbInst::create(block_, tielo_master, "drvr_inst");
  ASSERT_TRUE(drvr_inst);

  dbInst* load0_inst = dbInst::create(block_, buf_master, "load0_inst");
  ASSERT_TRUE(load0_inst);

  dbInst* load2_inst = dbInst::create(block_, buf_master, "load2_inst");
  ASSERT_TRUE(load2_inst);

  dbModInst* mi1 = dbModInst::create(mod0, mod1, "mi1");
  ASSERT_TRUE(mi1);

  dbModInst* mi0 = dbModInst::create(block_->getTopModule(), mod0, "mi0");
  ASSERT_TRUE(mi0);

  dbInst* load1_inst
      = dbInst::create(block_, buf_master, "load1_inst", false, mod1);
  ASSERT_TRUE(load1_inst);

  // Create nets and connect pins
  dbNet* net = dbNet::create(block_, "net");
  ASSERT_TRUE(net);

  dbModNet* top_mod_net = dbModNet::create(block_->getTopModule(), "net");
  ASSERT_TRUE(top_mod_net);

  dbITerm* drvr_z = drvr_inst->findITerm("Z");
  ASSERT_TRUE(drvr_z);
  drvr_z->connect(net, top_mod_net);

  dbITerm* load0_a = load0_inst->findITerm("A");
  ASSERT_TRUE(load0_a);
  load0_a->connect(net, top_mod_net);

  dbITerm* load2_a = load2_inst->findITerm("A");
  ASSERT_TRUE(load2_a);
  load2_a->connect(net, top_mod_net);

  dbBTerm* load_output_bterm = dbBTerm::create(net, "load_output");
  ASSERT_TRUE(load_output_bterm);
  load_output_bterm->setIoType(dbIoType::OUTPUT);
  load_output_bterm->connect(net, top_mod_net);

  // Hierarchical connections
  // Inside MOD1
  dbModNet* mod1_net_a = dbModNet::create(mod1, "A");
  ASSERT_TRUE(mod1_net_a);
  dbITerm* load1_a = load1_inst->findITerm("A");
  ASSERT_TRUE(load1_a);
  mod1->findModBTerm("A")->connect(mod1_net_a);
  load1_a->connect(net, mod1_net_a);

  // Inside MOD0
  dbModNet* mod0_net_a = dbModNet::create(mod0, "A");
  ASSERT_TRUE(mod0_net_a);
  dbModITerm::create(mi1, "A", mod1->findModBTerm("A"));
  mi1->findModITerm("A")->connect(mod0_net_a);
  mod0->findModBTerm("A")->connect(mod0_net_a);

  // Create ModITerm for mi0
  dbModITerm::create(mi0, "A", mod0->findModBTerm("A"));
  mi0->findModITerm("A")->connect(top_mod_net);
  num_warning = db_network_->checkAxioms();
  EXPECT_EQ(num_warning, 0);

  // Write verilog and check the content
  const std::string verilog_file_0 = "test_insert_buffer_pre.v";
  sta::writeVerilog(verilog_file_0.c_str(), true, false, {}, sta_->network());

  std::ifstream file_0(verilog_file_0);
  std::string content_0((std::istreambuf_iterator<char>(file_0)),
                        std::istreambuf_iterator<char>());

  const std::string expected_verilog_0 = R"(module top (load_output);
 output load_output;

 wire net;

 LOGIC0_X1 drvr_inst (.Z(net));
 BUF_X1 load0_inst (.A(net));
 BUF_X1 load2_inst (.A(net));
 MOD0 mi0 (.A(net));
 assign load_output = net;
endmodule
module MOD0 (A);
 input A;


 MOD1 mi1 (.A(A));
endmodule
module MOD1 (A);
 input A;


 BUF_X1 load1_inst (.A(A));
endmodule
)";

  EXPECT_EQ(content_0, expected_verilog_0);

  buffer_master = db_->findMaster("BUF_X4");
  ASSERT_TRUE(buffer_master);

  //-----------------------------------------------------------------
  // Insert buffer #1
  //-----------------------------------------------------------------
  dbInst* new_buffer1 = net->insertBufferBeforeLoad(load0_a, buffer_master);
  ASSERT_TRUE(new_buffer1);
  num_warning = db_network_->checkAxioms();
  EXPECT_EQ(num_warning, 0);

  // Verify connections
  EXPECT_EQ(load0_a->getNet()->getName(), std::string("net_load"));
  EXPECT_EQ(new_buffer1->findITerm("A")->getNet(), net);
  EXPECT_EQ(new_buffer1->findITerm("Z")->getNet(), load0_a->getNet());

  // Write verilog and check the content
  const std::string verilog_file_1 = "test_insert_buffer_1.v";
  sta::writeVerilog(verilog_file_1.c_str(), true, false, {}, sta_->network());

  std::ifstream file_1(verilog_file_1);
  std::string content_1((std::istreambuf_iterator<char>(file_1)),
                        std::istreambuf_iterator<char>());

  const std::string expected_verilog_1 = R"(module top (load_output);
 output load_output;

 wire net;
 wire net_load;

 BUF_X4 buf (.A(net),
    .Z(net_load));
 LOGIC0_X1 drvr_inst (.Z(net));
 BUF_X1 load0_inst (.A(net_load));
 BUF_X1 load2_inst (.A(net));
 MOD0 mi0 (.A(net));
 assign load_output = net;
endmodule
module MOD0 (A);
 input A;


 MOD1 mi1 (.A(A));
endmodule
module MOD1 (A);
 input A;


 BUF_X1 load1_inst (.A(A));
endmodule
)";

  EXPECT_EQ(content_1, expected_verilog_1);

  //-----------------------------------------------------------------
  // Insert buffer #2
  //-----------------------------------------------------------------
  dbInst* new_buffer2 = net->insertBufferBeforeLoad(load1_a, buffer_master);
  ASSERT_TRUE(new_buffer2);
  num_warning = db_network_->checkAxioms();
  EXPECT_EQ(num_warning, 0);

  // Verify connections for buffer #2
  EXPECT_EQ(load1_a->getNet()->getName(), std::string("mi0/mi1/net_load"));
  EXPECT_EQ(new_buffer2->findITerm("A")->getNet(), net);
  EXPECT_EQ(new_buffer2->findITerm("Z")->getNet(), load1_a->getNet());

  // Write verilog and check the content after inserting buffer #2
  const std::string verilog_file_2 = "test_insert_buffer_2.v";
  sta::writeVerilog(verilog_file_2.c_str(), true, false, {}, sta_->network());

  std::ifstream file_2(verilog_file_2);
  std::string content_2((std::istreambuf_iterator<char>(file_2)),
                        std::istreambuf_iterator<char>());

  const std::string expected_verilog_2 = R"(module top (load_output);
 output load_output;

 wire net;
 wire net_load;

 BUF_X4 buf (.A(net),
    .Z(net_load));
 LOGIC0_X1 drvr_inst (.Z(net));
 BUF_X1 load0_inst (.A(net_load));
 BUF_X1 load2_inst (.A(net));
 MOD0 mi0 (.A(net));
 assign load_output = net;
endmodule
module MOD0 (A);
 input A;


 MOD1 mi1 (.A(A));
endmodule
module MOD1 (A);
 input A;

 wire net_load;

 BUF_X4 buf (.A(A),
    .Z(net_load));
 BUF_X1 load1_inst (.A(net_load));
endmodule
)";
  EXPECT_EQ(content_2, expected_verilog_2);

  //-----------------------------------------------------------------
  // Insert buffer #3
  //-----------------------------------------------------------------
  dbInst* new_buffer3 = net->insertBufferBeforeLoad(load2_a, buffer_master);
  ASSERT_TRUE(new_buffer3);
  num_warning = db_network_->checkAxioms();
  EXPECT_EQ(num_warning, 0);

  // Verify connections for buffer #3
  EXPECT_EQ(load2_a->getNet()->getName(), std::string("net_load_1"));
  EXPECT_EQ(new_buffer3->findITerm("A")->getNet(), net);
  EXPECT_EQ(new_buffer3->findITerm("Z")->getNet(), load2_a->getNet());

  // Write verilog and check the content after inserting buffer #3
  const std::string verilog_file_3 = "test_insert_buffer_3.v";
  sta::writeVerilog(verilog_file_3.c_str(), true, false, {}, sta_->network());

  std::ifstream file_3(verilog_file_3);
  std::string content_3((std::istreambuf_iterator<char>(file_3)),
                        std::istreambuf_iterator<char>());

  const std::string expected_verilog_3 = R"(module top (load_output);
 output load_output;

 wire net;
 wire net_load;
 wire net_load_1;

 BUF_X4 buf (.A(net),
    .Z(net_load));
 BUF_X4 buf_1 (.A(net),
    .Z(net_load_1));
 LOGIC0_X1 drvr_inst (.Z(net));
 BUF_X1 load0_inst (.A(net_load));
 BUF_X1 load2_inst (.A(net_load_1));
 MOD0 mi0 (.A(net));
 assign load_output = net;
endmodule
module MOD0 (A);
 input A;


 MOD1 mi1 (.A(A));
endmodule
module MOD1 (A);
 input A;

 wire net_load;

 BUF_X4 buf (.A(A),
    .Z(net_load));
 BUF_X1 load1_inst (.A(net_load));
endmodule
)";

  EXPECT_EQ(content_3, expected_verilog_3);

  //-----------------------------------------------------------------
  // Insert buffer #4
  //-----------------------------------------------------------------
  dbInst* new_buffer4
      = net->insertBufferBeforeLoad(load_output_bterm, buffer_master);
  ASSERT_TRUE(new_buffer4);
  num_warning = db_network_->checkAxioms();
  EXPECT_EQ(num_warning, 0);

  // Verify connections for buffer #4
  EXPECT_EQ(load_output_bterm->getNet()->getName(), std::string("load_output"));
  EXPECT_EQ(new_buffer4->findITerm("A")->getNet(), net);
  EXPECT_EQ(new_buffer4->findITerm("Z")->getNet(), load_output_bterm->getNet());

  // Write verilog and check the content after inserting buffer #4
  const std::string verilog_file_4 = "test_insert_buffer_4.v";
  sta::writeVerilog(verilog_file_4.c_str(), true, false, {}, sta_->network());

  std::ifstream file_4(verilog_file_4);
  std::string content_4((std::istreambuf_iterator<char>(file_4)),
                        std::istreambuf_iterator<char>());

  const std::string expected_verilog_4 = R"(module top (load_output);
 output load_output;

 wire net;
 wire net_load;
 wire net_load_1;

 BUF_X4 buf (.A(net),
    .Z(net_load));
 BUF_X4 buf_1 (.A(net),
    .Z(net_load_1));
 BUF_X4 buf_2 (.A(net),
    .Z(load_output));
 LOGIC0_X1 drvr_inst (.Z(net));
 BUF_X1 load0_inst (.A(net_load));
 BUF_X1 load2_inst (.A(net_load_1));
 MOD0 mi0 (.A(net));
endmodule
module MOD0 (A);
 input A;


 MOD1 mi1 (.A(A));
endmodule
module MOD1 (A);
 input A;

 wire net_load;

 BUF_X4 buf (.A(A),
    .Z(net_load));
 BUF_X1 load1_inst (.A(net_load));
endmodule
)";

  EXPECT_EQ(content_4, expected_verilog_4);

  // Clean up
  removeFile(verilog_file_0);
  removeFile(verilog_file_1);
  removeFile(verilog_file_2);
  removeFile(verilog_file_3);
  removeFile(verilog_file_4);
}

// Netlist:
//   drvr (BUF)  --> n1 --> buf0 (BUF) --> n2 --> load (BUF)
//
// Load pins for insertion = {buf0/A}
// Expected result after insertion:
//   drvr (BUF) --> n1 --> buf_new --> net --> buf0 --> n2 --> load (BUF)
TEST_F(TestInsertBuffer, BeforeLoads_Case1)
{
  // Create masters
  dbMaster* buf_master = db_->findMaster("BUF_X1");
  ASSERT_TRUE(buf_master);
  dbMaster* buffer_master = db_->findMaster("BUF_X4");
  ASSERT_TRUE(buffer_master);

  // Create instances
  dbInst* drvr_inst = dbInst::create(block_, buf_master, "drvr");
  ASSERT_TRUE(drvr_inst);
  dbInst* buf0_inst = dbInst::create(block_, buf_master, "buf0");
  ASSERT_TRUE(buf0_inst);
  dbInst* load_inst = dbInst::create(block_, buf_master, "load");
  ASSERT_TRUE(load_inst);

  // Create nets and connect
  dbNet* n1 = dbNet::create(block_, "n1");
  ASSERT_TRUE(n1);
  dbNet* n2 = dbNet::create(block_, "n2");
  ASSERT_TRUE(n2);

  drvr_inst->findITerm("Z")->connect(n1);
  buf0_inst->findITerm("A")->connect(n1);
  buf0_inst->findITerm("Z")->connect(n2);
  load_inst->findITerm("A")->connect(n2);

  // Find load pin for buffer insertion
  dbITerm* buf0_a = buf0_inst->findITerm("A");
  std::set<dbObject*> loads;
  loads.insert(buf0_a);

  // Insert buffer
  dbInst* new_buf = n1->insertBufferBeforeLoads(loads, buffer_master);
  ASSERT_TRUE(new_buf);

  // Write verilog
  const std::string verilog_file = "results/BeforeLoads_Case1.v";
  sta::writeVerilog(verilog_file.c_str(), false, false, {}, sta_->network());
}

// Netlist:
//   in (Port) --> n1 --> buf0 (BUF) --> n2 --> out (Port)
//
// Insert buffer 1: Load pins = {buf0/A}
// Insert buffer 2: Load pins = {out (Port)}
//
// Expected result after buffer 1 (before buf0/A):
//   in (Port) --> [buf1] --> buf0 -->  out (Port)
//
// Expected result after buffer 2 (before out Port):
//   in (Port) --> [buf1] --> buf0 --> [buf2] --> out (Port)
TEST_F(TestInsertBuffer, BeforeLoads_Case2)
{
  // Create masters
  dbMaster* buf_master = db_->findMaster("BUF_X1");
  ASSERT_TRUE(buf_master);
  dbMaster* buffer_master = db_->findMaster("BUF_X4");
  ASSERT_TRUE(buffer_master);

  // Create instances
  dbInst* buf0_inst = dbInst::create(block_, buf_master, "buf0");
  ASSERT_TRUE(buf0_inst);

  // Create nets and ports
  dbNet* n1 = dbNet::create(block_, "n1");
  ASSERT_TRUE(n1);
  dbNet* n2 = dbNet::create(block_, "n2");
  ASSERT_TRUE(n2);
  dbBTerm* in_port = dbBTerm::create(n1, "in");
  ASSERT_TRUE(in_port);
  in_port->setIoType(dbIoType::INPUT);
  dbBTerm* out_port = dbBTerm::create(n2, "out");
  ASSERT_TRUE(out_port);
  out_port->setIoType(dbIoType::OUTPUT);

  // Connect
  buf0_inst->findITerm("A")->connect(n1);
  buf0_inst->findITerm("Z")->connect(n2);

  // Insert buffer 1
  dbITerm* buf0_a = buf0_inst->findITerm("A");
  std::set<dbObject*> loads1;
  loads1.insert(buf0_a);
  dbInst* new_buf1 = n1->insertBufferBeforeLoads(loads1, buffer_master);
  ASSERT_TRUE(new_buf1);

  // Insert buffer 2
  std::set<dbObject*> loads2;
  loads2.insert(out_port);
  dbInst* new_buf2 = n2->insertBufferBeforeLoads(loads2, buffer_master);
  ASSERT_TRUE(new_buf2);

  // Write verilog
  const std::string verilog_file = "results/BeforeLoads_Case2.v";
  sta::writeVerilog(verilog_file.c_str(), false, false, {}, sta_->network());
}

// Netlist:
//   in (Port) --> n1 --> mod0/buf0 (BUF) --> n2 --> out (Port)
//   where mod0 is an instance of submodule MOD0
//
// Insert buffer 1: Load pins = {mod0/buf0/A}
// Insert buffer 2: Load pins = {out (Port)}
//
// Expected result after buffer 1 (before mod0/buf0/A):
//   in --> n1 --> [new_buf1] --> mod0/buf0 --> out
//
// Expected result after buffer 2 (before out Port):
//   in --> n1 --> [new_buf1] --> mod0/buf0 --> [new_buf2] --> out
TEST_F(TestInsertBuffer, BeforeLoads_Case3)
{
  // Create masters
  dbMaster* buf_master = db_->findMaster("BUF_X1");
  ASSERT_TRUE(buf_master);
  dbMaster* buffer_master = db_->findMaster("BUF_X4");
  ASSERT_TRUE(buffer_master);

  // Create submodule MOD0
  dbModule* mod0_module = dbModule::create(block_, "MOD0");
  ASSERT_TRUE(mod0_module);
  dbInst* buf0_inst
      = dbInst::create(block_, buf_master, "buf0", false, mod0_module);
  ASSERT_TRUE(buf0_inst);
  dbModBTerm* mod0_in = dbModBTerm::create(mod0_module, "in");
  mod0_in->setIoType(dbIoType::INPUT);
  dbModBTerm* mod0_out = dbModBTerm::create(mod0_module, "out");
  mod0_out->setIoType(dbIoType::OUTPUT);

  // Connect inside MOD0
  dbModNet* mod_n1 = dbModNet::create(mod0_module, "n1");
  mod0_in->connect(mod_n1);
  buf0_inst->findITerm("A")->connect(mod_n1);

  dbModNet* mod_n2 = dbModNet::create(mod0_module, "n2");
  buf0_inst->findITerm("Z")->connect(mod_n2);
  mod0_out->connect(mod_n2);

  // Create top-level logic
  dbModInst* mod0_inst
      = dbModInst::create(block_->getTopModule(), mod0_module, "mod0");
  ASSERT_TRUE(mod0_inst);

  dbNet* n1 = dbNet::create(block_, "n1");
  ASSERT_TRUE(n1);
  dbBTerm* in_port = dbBTerm::create(n1, "in");
  ASSERT_TRUE(in_port);
  in_port->setIoType(dbIoType::INPUT);

  dbNet* n2 = dbNet::create(block_, "n2");
  ASSERT_TRUE(n2);
  dbBTerm* out_port = dbBTerm::create(n2, "out");
  ASSERT_TRUE(out_port);
  out_port->setIoType(dbIoType::OUTPUT);

  // Make hierarchical connections
  dbModNet* top_n1 = dbModNet::create(block_->getTopModule(), "n1");
  in_port->connect(top_n1);
  dbModITerm* mod0_in_iterm = dbModITerm::create(mod0_inst, "in", mod0_in);
  mod0_in_iterm->connect(top_n1);

  dbModNet* top_n2 = dbModNet::create(block_->getTopModule(), "n2");
  out_port->connect(top_n2);
  dbModITerm* mod0_out_iterm = dbModITerm::create(mod0_inst, "out", mod0_out);
  mod0_out_iterm->connect(top_n2);

  // Physical connection for flat nets
  mod0_module->findDbInst("buf0")->findITerm("A")->connect(n1);
  mod0_module->findDbInst("buf0")->findITerm("Z")->connect(n2);

  // Insert buffer 1
  dbITerm* buf0_a = mod0_module->findDbInst("buf0")->findITerm("A");
  ASSERT_TRUE(buf0_a);
  std::set<dbObject*> loads1;
  loads1.insert(buf0_a);
  dbInst* new_buf1 = buf0_a->getNet()->insertBufferBeforeLoads(
      loads1, buffer_master, nullptr, "new_buf1");
  ASSERT_TRUE(new_buf1);

  // Insert buffer 2
  std::set<dbObject*> loads2;
  loads2.insert(out_port);
  dbInst* new_buf2 = out_port->getNet()->insertBufferBeforeLoads(
      loads2, buffer_master, nullptr, "new_buf2");
  ASSERT_TRUE(new_buf2);

  // Write verilog
  const std::string verilog_file = "results/BeforeLoads_Case3.v";
  sta::writeVerilog(verilog_file.c_str(), false, false, {}, sta_->network());
}

// Netlist:
//   drvr0 (BUF) --> n1 --+--> load0 (BUF)
//                        +--> load1 (BUF)
//                        +--> load2 (BUF)
//
//   No hierarchical modules in this netlist.
//
// Insert buffer 1: Load pins = {load0/A, load1/A}
// Insert buffer 2: Load pins = {new0/A, load2/A}
//
// Expected result after buffer 1:
//   drvr0 --> n1 --+--> load2
//                  |
//                  +--> [new0] --(net_A)--+--> load0
//                                         +--> load1
//
// Expected result after buffer 2 (Buffers both load2 and new0):
//   drvr0 --> n1 --> [new1] --(net_B)--+--> load2
//                                      |                    +--> load0
//                                      +--> new0 --(net_A)--+
//                                                           +--> load1
TEST_F(TestInsertBuffer, BeforeLoads_Case4)
{
  // Create masters
  dbMaster* buf_master = db_->findMaster("BUF_X1");
  ASSERT_TRUE(buf_master);
  dbMaster* buffer_master = db_->findMaster("BUF_X4");
  ASSERT_TRUE(buffer_master);

  // Create instances
  dbInst* drvr0 = dbInst::create(block_, buf_master, "drvr0");
  ASSERT_TRUE(drvr0);
  dbInst* load0 = dbInst::create(block_, buf_master, "load0");
  ASSERT_TRUE(load0);
  dbInst* load1 = dbInst::create(block_, buf_master, "load1");
  ASSERT_TRUE(load1);
  dbInst* load2 = dbInst::create(block_, buf_master, "load2");
  ASSERT_TRUE(load2);

  // Create net and connect
  dbNet* n1 = dbNet::create(block_, "n1");
  ASSERT_TRUE(n1);
  drvr0->findITerm("Z")->connect(n1);
  load0->findITerm("A")->connect(n1);
  load1->findITerm("A")->connect(n1);
  load2->findITerm("A")->connect(n1);

  // Insert buffer 1
  dbITerm* load0_a = load0->findITerm("A");
  dbITerm* load1_a = load1->findITerm("A");
  std::set<dbObject*> loads1;
  loads1.insert(load0_a);
  loads1.insert(load1_a);
  dbInst* new0
      = n1->insertBufferBeforeLoads(loads1, buffer_master, nullptr, "new0");
  ASSERT_TRUE(new0);

  // Insert buffer 2
  dbITerm* new0_a = new0->findITerm("A");
  dbITerm* load2_a = load2->findITerm("A");
  std::set<dbObject*> loads2;
  loads2.insert(new0_a);
  loads2.insert(load2_a);
  dbInst* new1
      = n1->insertBufferBeforeLoads(loads2, buffer_master, nullptr, "new1");
  ASSERT_TRUE(new1);

  // Write verilog
  const std::string verilog_file = "results/BeforeLoads_Case4.v";
  sta::writeVerilog(verilog_file.c_str(), false, false, {}, sta_->network());
}

// Netlist:
//   Top module contains four submodules: MOD0, MOD1, MOD2, MOD3.
//   There are four cells (drvr0, load0, load1, load2) in total.
//   drvr0 drives load0, load1, and load2, with each cell located
//   inside a different MOD* submodule (mod0/drvr0, mod1/load0,
//   mod2/load1, mod3/load2).
//
//   Hierarchy:
//     top
//     +-- mod0 (MOD0) --> drvr0 (BUF)
//     +-- mod1 (MOD1) --> load0 (BUF)
//     +-- mod2 (MOD2) --> load1 (BUF)
//     +-- mod3 (MOD3) --> load2 (BUF)
//
//   Connections:
//     mod0/drvr0/Z --> n1
//     n1 --> mod1/load0/A
//     n1 --> mod2/load1/A
//     n1 --> mod3/load2/A
//
// Insert buffer 1: Load pins = {mod1/load0/A, mod2/load1/A}
// Insert buffer 2: Load pins = {new0/A, mod3/load2/A}
TEST_F(TestInsertBuffer, BeforeLoads_Case5)
{
  // Create masters
  dbMaster* buf_master = db_->findMaster("BUF_X1");
  ASSERT_TRUE(buf_master);
  dbMaster* buffer_master = db_->findMaster("BUF_X4");
  ASSERT_TRUE(buffer_master);

  // Create submodules and instances within them
  dbModule* mod0 = dbModule::create(block_, "MOD0");
  dbInst::create(block_, buf_master, "drvr0", false, mod0);
  dbModBTerm::create(mod0, "Z")->setIoType(dbIoType::OUTPUT);
  mod0->findModBTerm("Z")->connect(dbModNet::create(mod0, "Z_net"));
  mod0->findDbInst("drvr0")->findITerm("Z")->connect(mod0->getModNet("Z_net"));

  dbModule* mod1 = dbModule::create(block_, "MOD1");
  dbInst::create(block_, buf_master, "load0", false, mod1);
  dbModBTerm::create(mod1, "A")->setIoType(dbIoType::INPUT);
  mod1->findModBTerm("A")->connect(dbModNet::create(mod1, "A_net"));
  mod1->findDbInst("load0")->findITerm("A")->connect(mod1->getModNet("A_net"));

  dbModule* mod2 = dbModule::create(block_, "MOD2");
  dbInst::create(block_, buf_master, "load1", false, mod2);
  dbModBTerm::create(mod2, "A")->setIoType(dbIoType::INPUT);
  mod2->findModBTerm("A")->connect(dbModNet::create(mod2, "A_net"));
  mod2->findDbInst("load1")->findITerm("A")->connect(mod2->getModNet("A_net"));

  dbModule* mod3 = dbModule::create(block_, "MOD3");
  dbInst::create(block_, buf_master, "load2", false, mod3);
  dbModBTerm::create(mod3, "A")->setIoType(dbIoType::INPUT);
  mod3->findModBTerm("A")->connect(dbModNet::create(mod3, "A_net"));
  mod3->findDbInst("load2")->findITerm("A")->connect(mod3->getModNet("A_net"));

  // Create top-level module instances
  dbModInst* mi0 = dbModInst::create(block_->getTopModule(), mod0, "mod0");
  dbModInst* mi1 = dbModInst::create(block_->getTopModule(), mod1, "mod1");
  dbModInst* mi2 = dbModInst::create(block_->getTopModule(), mod2, "mod2");
  dbModInst* mi3 = dbModInst::create(block_->getTopModule(), mod3, "mod3");

  // Connect them hierarchically
  dbNet* n1 = dbNet::create(block_, "n1");
  ASSERT_TRUE(n1);

  dbModNet* top_n1 = dbModNet::create(block_->getTopModule(), "n1");
  dbModITerm::create(mi0, "Z", mod0->findModBTerm("Z"))->connect(top_n1);
  dbModITerm::create(mi1, "A", mod1->findModBTerm("A"))->connect(top_n1);
  dbModITerm::create(mi2, "A", mod2->findModBTerm("A"))->connect(top_n1);
  dbModITerm::create(mi3, "A", mod3->findModBTerm("A"))->connect(top_n1);

  // Physical connections
  mod0->findDbInst("drvr0")->findITerm("Z")->connect(n1);
  mod1->findDbInst("load0")->findITerm("A")->connect(n1);
  mod2->findDbInst("load1")->findITerm("A")->connect(n1);
  mod3->findDbInst("load2")->findITerm("A")->connect(n1);

  // Insert buffer 1
  dbITerm* load0_a = mod1->findDbInst("load0")->findITerm("A");
  ASSERT_TRUE(load0_a);
  dbITerm* load1_a = mod2->findDbInst("load1")->findITerm("A");
  ASSERT_TRUE(load1_a);

  std::set<dbObject*> loads1;
  loads1.insert(load0_a);
  loads1.insert(load1_a);
  dbInst* new0
      = n1->insertBufferBeforeLoads(loads1, buffer_master, nullptr, "new0");
  ASSERT_TRUE(new0);

  // Insert buffer 2
  dbITerm* new0_a = new0->findITerm("A");
  ASSERT_TRUE(new0_a);
  dbITerm* load2_a = mod3->findDbInst("load2")->findITerm("A");
  ASSERT_TRUE(load2_a);

  std::set<dbObject*> loads2;
  loads2.insert(new0_a);
  loads2.insert(load2_a);
  dbInst* new1
      = n1->insertBufferBeforeLoads(loads2, buffer_master, nullptr, "new1");
  ASSERT_TRUE(new1);

  // Write verilog
  const std::string verilog_file = "results/BeforeLoads_Case5.v";
  sta::writeVerilog(verilog_file.c_str(), false, false, {}, sta_->network());
}

// Netlist Structure (Reflecting the user provided tree):
//
// [Top Module]
//  |
//  +-- load0 (BUF)
//  |
//  +-- u_h2 (ModH2)
//  |    |
//  |    +-- u_h0 (ModH0) -> drvr (BUF) [Driver]
//  |    |
//  |    +-- u_h1 (ModH1) -> load5 (BUF) [TARGET]
//  |
//  +-- u_h3 (ModH3)
//  |    |
//  |    +-- load1 (BUF)
//  |    +-- load2 (BUF) [TARGET]
//  |
//  +-- u_h4 (ModH4)
//       |
//       +-- load4 (BUF)
//       +-- u_h5 (ModH5) -> load3 (BUF) [TARGET]
//
// Operation:
//   Insert buffer for targets: { load5, load2, load3 }
//   LCA: Top Module
//
// Expected Result:
//   1. New buffer 'new_buf' placed in Top.
//   2. Logical connections (Port Punching):
//      - To load5: Top -> u_h2 -> u_h1 (New Ports created)
//      - To load2: Top -> u_h3 (New Port created)
//      - To load3: Top -> u_h4 -> u_h5 (New Ports created)
//   3. Non-targets (load0, load1, load4) remain on original net.
TEST_F(TestInsertBuffer, BeforeLoads_Case6)
{
  std::string test_name = "TestInsertBuffer_BeforeLoads6";
  readVerilogAndSetup(test_name + ".v");

  // Get ODB objects
  dbInst* drvr = block_->findInst("u_h2/u_h0/drvr");
  ASSERT_NE(drvr, nullptr);
  dbInst* load0 = block_->findInst("load0");
  ASSERT_NE(load0, nullptr);
  dbInst* load1 = block_->findInst("u_h3/load1");
  ASSERT_NE(load1, nullptr);
  dbInst* load2 = block_->findInst("u_h3/load2");
  ASSERT_NE(load2, nullptr);
  dbInst* load3 = block_->findInst("u_h4/u_h5/load3");
  ASSERT_NE(load3, nullptr);
  dbInst* load4 = block_->findInst("u_h4/load4");
  ASSERT_NE(load4, nullptr);
  dbInst* load5 = block_->findInst("u_h2/u_h1/load5");
  ASSERT_NE(load5, nullptr);
  dbITerm* load5_a = load5->findITerm("A");
  ASSERT_NE(load5_a, nullptr);
  dbITerm* load2_a = load2->findITerm("A");
  ASSERT_NE(load2_a, nullptr);
  dbITerm* load3_a = load3->findITerm("A");
  ASSERT_NE(load3_a, nullptr);
  dbNet* target_net = load5_a->getNet();
  ASSERT_NE(target_net, nullptr);
  dbMaster* buffer_master = db_->findMaster("BUF_X1");
  ASSERT_NE(buffer_master, nullptr);
  dbModInst* u_h0 = block_->findModInst("u_h2/u_h0");
  ASSERT_NE(u_h0, nullptr);
  dbModInst* u_h1 = block_->findModInst("u_h2/u_h1");
  ASSERT_NE(u_h1, nullptr);
  dbModInst* u_h2 = block_->findModInst("u_h2");
  ASSERT_NE(u_h2, nullptr);
  dbModInst* u_h3 = block_->findModInst("u_h3");
  ASSERT_NE(u_h3, nullptr);
  dbModInst* u_h4 = block_->findModInst("u_h4");
  ASSERT_NE(u_h4, nullptr);
  dbModInst* u_h5 = block_->findModInst("u_h4/u_h5");
  ASSERT_NE(u_h5, nullptr);
  dbModule* mod_h0 = block_->findModule("ModH0");
  ASSERT_NE(mod_h0, nullptr);
  dbModule* mod_h1 = block_->findModule("ModH1");
  ASSERT_NE(mod_h1, nullptr);
  dbModule* mod_h2 = block_->findModule("ModH2");
  ASSERT_NE(mod_h2, nullptr);
  dbModule* mod_h3 = block_->findModule("ModH3");
  ASSERT_NE(mod_h3, nullptr);
  dbModule* mod_h4 = block_->findModule("ModH4");
  ASSERT_NE(mod_h4, nullptr);
  dbModule* mod_h5 = block_->findModule("ModH5");
  ASSERT_NE(mod_h5, nullptr);
  dbModNet* modnet_h4_in = block_->findModNet("u_h4/in");
  ASSERT_NE(modnet_h4_in, nullptr);

  // odb::dbDatabase::beginEco(block_);

  // Pre sanity check
  sta_->updateTiming(true);
  db_network_->checkAxioms();

  //----------------------------------------------------
  // Insert buffer
  // - Targets: load5 (in H2/H1), load2 (in H3), load3 (in H4/H5)
  //----------------------------------------------------
  std::set<dbObject*> targets;
  targets.insert(load5_a);
  targets.insert(load2_a);
  targets.insert(load3_a);
  dbInst* new_buf = target_net->insertBufferBeforeLoads(
      targets, buffer_master, nullptr, "new_buf");
  ASSERT_TRUE(new_buf);

  // jk: dbg. Write verilog
  {
    const std::string verilog_file = "results/BeforeLoads_Case6.v";
    sta::writeVerilog(verilog_file.c_str(), false, false, {}, sta_->network());
  }

  //----------------------------------------------------
  // Verify Results
  //----------------------------------------------------

  // Buffer Location: Top Module (LCA)
  EXPECT_EQ(new_buf->getModule(), block_->getTopModule());

  // Net Separation
  dbNet* buf_out_net = new_buf->findITerm("Z")->getNet();
  ASSERT_TRUE(buf_out_net);
  EXPECT_NE(buf_out_net, target_net);

  // Target Loads moved to new net
  EXPECT_EQ(load5->findITerm("A")->getNet(), buf_out_net);
  EXPECT_EQ(load2->findITerm("A")->getNet(), buf_out_net);
  EXPECT_EQ(load3->findITerm("A")->getNet(), buf_out_net);

  // Non-Target Loads remain on old net
  EXPECT_EQ(load0->findITerm("A")->getNet(), target_net);
  EXPECT_EQ(load1->findITerm("A")->getNet(), target_net);
  EXPECT_EQ(load4->findITerm("A")->getNet(), target_net);

  // Port Punching Verification (Logical)
  dbModNet* top_out_mod_net
      = block_->getTopModule()->getModNet(buf_out_net->getConstName());
  ASSERT_NE(top_out_mod_net, nullptr);
  ASSERT_TRUE(top_out_mod_net);

  // Check if new ports were punched into H2, H3, H4
  bool punched_h2 = false;
  bool punched_h3 = false;
  bool punched_h4 = false;

  for (dbModITerm* iterm : top_out_mod_net->getModITerms()) {
    if (iterm->getParent() == u_h2) {
      punched_h2 = true;
    }
    if (iterm->getParent() == u_h3) {
      punched_h3 = true;
    }
    if (iterm->getParent() == u_h4) {
      punched_h4 = true;
    }
  }

  EXPECT_TRUE(punched_h2);  // Top -> H2 -> H1 -> load5
  EXPECT_TRUE(punched_h3);  // Top -> H3 -> load2
  EXPECT_TRUE(punched_h4);  // Top -> H4 -> H5 -> load3

  // Deeper check for H4 -> H5 punching
  // We need to find the net inside H4 that connects to the new port
  // This is tricky without knowing exact name, but we can search H4's nets
  bool punched_h5_in_h4 = false;
  for (dbModNet* net : mod_h4->getModNets()) {
    // If this net connects to u_h5 AND to the boundary (ModBTerm), it's likely
    // the punched net
    bool connects_boundary = !net->getModBTerms().empty();
    bool connects_h5 = false;
    for (dbModITerm* iterm : net->getModITerms()) {
      if (iterm->getParent() == u_h5) {
        connects_h5 = true;
      }
    }
    if (connects_boundary && connects_h5 && net != modnet_h4_in) {
      punched_h5_in_h4 = true;
      break;
    }
  }
  EXPECT_TRUE(punched_h5_in_h4);

  // Write verilog
  const std::string verilog_file = "results/BeforeLoads_Case6.v";
  sta::writeVerilog(verilog_file.c_str(), false, false, {}, sta_->network());
}

// Netlist Structure:
//
// [Top Module]
//  |
//  +-- u_h0 (ModH0) -> drvr (LOGIC0_X1) [Driver]
//  |
//  +-- load0 (BUF_X1)
//  +-- load1 (BUF_X1)
//  +-- load4 (BUF_X1) [TARGET]
//  |
//  +-- u_h1 (ModH1) -> load2 (BUF_X1) [TARGET]
//  |
//  +-- u_h2 (ModH2)
//       |
//       +-- u_h3 (ModH3) -> load3 (BUF_X1) [TARGET]
//
// Operation:
//   Insert buffer for targets: { load4, load2, load3 }
//   LCA: Top Module
//
// Expected Result:
//   1. New buffer 'new_buf' placed in Top.
//   2. Logical connections (Port Punching):
//      - To load4: Top -> load4 (Direct connection)
//      - To load2: Top -> u_h1 (New Port created)
//      - To load3: Top -> u_h2 -> u_h3 (New Ports created)
//   3. Non-targets (load0, load1) remain on original net.
TEST_F(TestInsertBuffer, BeforeLoads_Case7)
{
  std::string test_name = "TestInsertBuffer_BeforeLoads7";
  readVerilogAndSetup(test_name + ".v");

  // Get ODB objects
  dbInst* drvr = block_->findInst("u_h0/drvr");
  ASSERT_NE(drvr, nullptr);
  dbInst* load0 = block_->findInst("load0");
  ASSERT_NE(load0, nullptr);
  dbInst* load1 = block_->findInst("load1");
  ASSERT_NE(load1, nullptr);
  dbInst* load4 = block_->findInst("load4");
  ASSERT_NE(load4, nullptr);
  dbInst* load2 = block_->findInst("u_h1/load2");
  ASSERT_NE(load2, nullptr);
  dbInst* load3 = block_->findInst("u_h2/u_h3/load3");
  ASSERT_NE(load3, nullptr);

  dbITerm* load4_a = load4->findITerm("A");
  ASSERT_NE(load4_a, nullptr);
  dbITerm* load2_a = load2->findITerm("A");
  ASSERT_NE(load2_a, nullptr);
  dbITerm* load3_a = load3->findITerm("A");
  ASSERT_NE(load3_a, nullptr);

  dbNet* target_net = load4_a->getNet();
  ASSERT_NE(target_net, nullptr);

  dbMaster* buffer_master = db_->findMaster("BUF_X1");
  ASSERT_NE(buffer_master, nullptr);

  dbModInst* u_h1 = block_->findModInst("u_h1");
  ASSERT_NE(u_h1, nullptr);
  dbModInst* u_h2 = block_->findModInst("u_h2");
  ASSERT_NE(u_h2, nullptr);
  dbModInst* u_h3 = block_->findModInst("u_h2/u_h3");
  ASSERT_NE(u_h3, nullptr);

  dbModule* mod_h2 = block_->findModule("ModH2");
  ASSERT_NE(mod_h2, nullptr);
  dbModNet* modnet_h2_in = block_->findModNet("u_h2/in");
  ASSERT_NE(modnet_h2_in, nullptr);

  // Pre sanity check
  sta_->updateTiming(true);
  db_network_->checkAxioms();

  //----------------------------------------------------
  // Insert buffer
  // - Targets: load4 (Top), load2 (H1), load3 (H2/H3)
  //----------------------------------------------------
  std::set<dbObject*> targets;
  targets.insert(load4_a);
  targets.insert(load2_a);
  targets.insert(load3_a);
  dbInst* new_buf = target_net->insertBufferBeforeLoads(
      targets, buffer_master, nullptr, "new_buf");
  ASSERT_TRUE(new_buf);

  // Write verilog
  {
    const std::string verilog_file = "results/BeforeLoads_Case7.v";
    sta::writeVerilog(verilog_file.c_str(), false, false, {}, sta_->network());
  }

  //----------------------------------------------------
  // Verify Results
  //----------------------------------------------------

  // Buffer Location: Top Module (LCA)
  EXPECT_EQ(new_buf->getModule(), block_->getTopModule());

  // Net Separation
  dbNet* buf_out_net = new_buf->findITerm("Z")->getNet();
  ASSERT_TRUE(buf_out_net);
  EXPECT_NE(buf_out_net, target_net);

  // Target Loads moved to new net
  EXPECT_EQ(load4->findITerm("A")->getNet(), buf_out_net);
  EXPECT_EQ(load2->findITerm("A")->getNet(), buf_out_net);
  EXPECT_EQ(load3->findITerm("A")->getNet(), buf_out_net);

  // Non-Target Loads remain on old net
  EXPECT_EQ(load0->findITerm("A")->getNet(), target_net);
  EXPECT_EQ(load1->findITerm("A")->getNet(), target_net);

  // Port Punching Verification (Logical)
  dbModNet* top_out_mod_net
      = block_->getTopModule()->getModNet(buf_out_net->getConstName());
  ASSERT_NE(top_out_mod_net, nullptr);
  ASSERT_TRUE(top_out_mod_net);

  // Check if new ports were punched into H1, H2
  bool punched_h1 = false;
  bool punched_h2 = false;

  for (dbModITerm* iterm : top_out_mod_net->getModITerms()) {
    if (iterm->getParent() == u_h1) {
      punched_h1 = true;
    }
    if (iterm->getParent() == u_h2) {
      punched_h2 = true;
    }
  }

  EXPECT_TRUE(punched_h1);  // Top -> H1 -> load2
  EXPECT_TRUE(punched_h2);  // Top -> H2 -> H3 -> load3

  // Deeper check for H2 -> H3 punching
  bool punched_h3_in_h2 = false;
  for (dbModNet* net : mod_h2->getModNets()) {
    // If this net connects to u_h3 AND to the boundary (ModBTerm), it's likely
    // the punched net
    bool connects_boundary = !net->getModBTerms().empty();
    bool connects_h3 = false;
    for (dbModITerm* iterm : net->getModITerms()) {
      if (iterm->getParent() == u_h3) {
        connects_h3 = true;
      }
    }
    if (connects_boundary && connects_h3 && net != modnet_h2_in) {
      punched_h3_in_h2 = true;
      break;
    }
  }
  EXPECT_FALSE(punched_h3_in_h2);
}

TEST_F(TestInsertBuffer, AfterDriver_Case1)
{
  int num_warning = 0;
  dbMaster* buffer_master = db_->findMaster("BUF_X4");
  ASSERT_TRUE(buffer_master);

  // Create masters
  dbMaster* buf_master = db_->findMaster("BUF_X1");
  ASSERT_TRUE(buf_master);

  dbModule* mod1 = dbModule::create(block_, "MOD1");
  ASSERT_TRUE(mod1);
  dbModBTerm::create(mod1, "A");

  dbModule* mod0 = dbModule::create(block_, "MOD0");
  ASSERT_TRUE(mod0);
  dbModBTerm::create(mod0, "A");

  // Create instances
  dbInst* drvr_inst = dbInst::create(block_, buf_master, "drvr_inst");
  ASSERT_TRUE(drvr_inst);

  dbInst* load0_inst = dbInst::create(block_, buf_master, "load0_inst");
  ASSERT_TRUE(load0_inst);

  dbInst* load2_inst = dbInst::create(block_, buf_master, "load2_inst");
  ASSERT_TRUE(load2_inst);

  dbModInst* mi1 = dbModInst::create(mod0, mod1, "mi1");
  ASSERT_TRUE(mi1);

  dbModInst* mi0 = dbModInst::create(block_->getTopModule(), mod0, "mi0");
  ASSERT_TRUE(mi0);

  dbInst* load1_inst
      = dbInst::create(block_, buf_master, "load1_inst", false, mod1);
  ASSERT_TRUE(load1_inst);

  // Create nets and connect pins
  dbNet* net = dbNet::create(block_, "net");
  ASSERT_TRUE(net);

  dbNet* in_net = dbNet::create(block_, "in");
  ASSERT_TRUE(in_net);
  dbBTerm* in_bterm = dbBTerm::create(in_net, "in");
  ASSERT_TRUE(in_bterm);
  in_bterm->setIoType(dbIoType::INPUT);
  in_bterm->connect(in_net);

  dbITerm* drvr_a = drvr_inst->findITerm("A");
  ASSERT_TRUE(drvr_a);
  drvr_a->connect(in_net);

  dbITerm* drvr_z = drvr_inst->findITerm("Z");
  ASSERT_TRUE(drvr_z);
  drvr_z->connect(net);

  dbITerm* load0_a = load0_inst->findITerm("A");
  ASSERT_TRUE(load0_a);
  load0_a->connect(net);

  dbITerm* load2_a = load2_inst->findITerm("A");
  ASSERT_TRUE(load2_a);
  load2_a->connect(net);

  // Hierarchical connections
  // Inside MOD1
  dbModNet* mod1_net_a = dbModNet::create(mod1, "A");
  ASSERT_TRUE(mod1_net_a);
  dbITerm* load1_a = load1_inst->findITerm("A");
  ASSERT_TRUE(load1_a);
  mod1->findModBTerm("A")->connect(mod1_net_a);
  load1_a->connect(net, mod1_net_a);

  // Inside MOD0
  dbModNet* mod0_net_a = dbModNet::create(mod0, "A");
  ASSERT_TRUE(mod0_net_a);
  dbModITerm::create(mi1, "A", mod1->findModBTerm("A"));
  mi1->findModITerm("A")->connect(mod0_net_a);
  mod0->findModBTerm("A")->connect(mod0_net_a);

  // Connect top-level net to hierarchical instance through a modnet
  dbModNet* top_mod_net = dbModNet::create(block_->getTopModule(), "net");
  ASSERT_TRUE(top_mod_net);
  drvr_z->connect(top_mod_net);
  load0_a->connect(top_mod_net);
  load2_a->connect(top_mod_net);

  // Create ModITerm for mi0
  dbModITerm::create(mi0, "A", mod0->findModBTerm("A"));
  mi0->findModITerm("A")->connect(top_mod_net);
  num_warning = db_network_->checkAxioms();
  EXPECT_EQ(num_warning, 0);

  // Write verilog and check the content
  const std::string verilog_file_0 = "test_insert_buffer_after_drvr_pre.v";
  sta::writeVerilog(verilog_file_0.c_str(), true, false, {}, sta_->network());

  std::ifstream file_0(verilog_file_0);
  std::string content_0((std::istreambuf_iterator<char>(file_0)),
                        std::istreambuf_iterator<char>());

  const std::string expected_verilog_0 = R"(module top (in);
 input in;

 wire net;

 BUF_X1 drvr_inst (.A(in),
    .Z(net));
 BUF_X1 load0_inst (.A(net));
 BUF_X1 load2_inst (.A(net));
 MOD0 mi0 (.A(net));
endmodule
module MOD0 (A);
 input A;


 MOD1 mi1 (.A(A));
endmodule
module MOD1 (A);
 input A;


 BUF_X1 load1_inst (.A(A));
endmodule
)";

  EXPECT_EQ(content_0, expected_verilog_0);

  buffer_master = db_->findMaster("BUF_X4");
  ASSERT_TRUE(buffer_master);

  //-----------------------------------------------------------------
  // Insert buffer
  //-----------------------------------------------------------------
  dbInst* new_buffer = net->insertBufferAfterDriver(drvr_z, buffer_master);
  ASSERT_TRUE(new_buffer);
  num_warning = db_network_->checkAxioms();
  EXPECT_EQ(num_warning, 0);

  // Verify connections
  EXPECT_EQ(drvr_z->getNet()->getName(), std::string("net_drvr"));
  EXPECT_EQ(new_buffer->findITerm("A")->getNet(), drvr_z->getNet());
  EXPECT_EQ(new_buffer->findITerm("Z")->getNet(), net);

  // Write verilog and check the content
  const std::string verilog_file_1 = "test_insert_buffer_after_drvr.v";
  sta::writeVerilog(verilog_file_1.c_str(), true, false, {}, sta_->network());

  std::ifstream file_1(verilog_file_1);
  std::string content_1((std::istreambuf_iterator<char>(file_1)),
                        std::istreambuf_iterator<char>());

  const std::string expected_verilog_1 = R"(module top (in);
 input in;

 wire net;
 wire net_drvr;

 BUF_X4 buf (.A(net_drvr),
    .Z(net));
 BUF_X1 drvr_inst (.A(in),
    .Z(net_drvr));
 BUF_X1 load0_inst (.A(net));
 BUF_X1 load2_inst (.A(net));
 MOD0 mi0 (.A(net));
endmodule
module MOD0 (A);
 input A;


 MOD1 mi1 (.A(A));
endmodule
module MOD1 (A);
 input A;


 BUF_X1 load1_inst (.A(A));
endmodule
)";

  EXPECT_EQ(content_1, expected_verilog_1);

  // Clean up
  removeFile(verilog_file_0);
  removeFile(verilog_file_1);
}

TEST_F(TestInsertBuffer, AfterDriver_Case2)
{
  int num_warning = 0;
  dbMaster* buffer_master = db_->findMaster("BUF_X4");
  ASSERT_TRUE(buffer_master);

  // Create masters
  dbMaster* buf_master = db_->findMaster("BUF_X1");
  ASSERT_TRUE(buf_master);

  dbModule* mod1 = dbModule::create(block_, "MOD1");
  ASSERT_TRUE(mod1);
  dbModBTerm::create(mod1, "A");

  dbModule* mod0 = dbModule::create(block_, "MOD0");
  ASSERT_TRUE(mod0);
  dbModBTerm::create(mod0, "A");

  // Create instances
  dbInst* load0_inst = dbInst::create(block_, buf_master, "load0_inst");
  ASSERT_TRUE(load0_inst);

  dbInst* load2_inst = dbInst::create(block_, buf_master, "load2_inst");
  ASSERT_TRUE(load2_inst);

  dbModInst* mi1 = dbModInst::create(mod0, mod1, "mi1");
  ASSERT_TRUE(mi1);

  dbModInst* mi0 = dbModInst::create(block_->getTopModule(), mod0, "mi0");
  ASSERT_TRUE(mi0);

  dbInst* load1_inst
      = dbInst::create(block_, buf_master, "load1_inst", false, mod1);
  ASSERT_TRUE(load1_inst);

  // Create nets and connect pins
  dbNet* net = dbNet::create(block_, "X");
  ASSERT_TRUE(net);

  dbBTerm* drvr_bterm = dbBTerm::create(net, "X");
  ASSERT_TRUE(drvr_bterm);
  drvr_bterm->setIoType(dbIoType::INPUT);
  drvr_bterm->connect(net);

  dbITerm* load0_a = load0_inst->findITerm("A");
  ASSERT_TRUE(load0_a);
  load0_a->connect(net);

  dbITerm* load2_a = load2_inst->findITerm("A");
  ASSERT_TRUE(load2_a);
  load2_a->connect(net);

  // Hierarchical connections
  // Inside MOD1
  dbModNet* mod1_net_a = dbModNet::create(mod1, "A");
  ASSERT_TRUE(mod1_net_a);
  dbITerm* load1_a = load1_inst->findITerm("A");
  ASSERT_TRUE(load1_a);
  mod1->findModBTerm("A")->connect(mod1_net_a);
  load1_a->connect(net, mod1_net_a);

  // Inside MOD0
  dbModNet* mod0_net_a = dbModNet::create(mod0, "A");
  ASSERT_TRUE(mod0_net_a);
  dbModITerm::create(mi1, "A", mod1->findModBTerm("A"));
  mi1->findModITerm("A")->connect(mod0_net_a);
  mod0->findModBTerm("A")->connect(mod0_net_a);

  // Connect top-level net to hierarchical instance through a modnet
  dbModNet* top_mod_net = dbModNet::create(block_->getTopModule(), "X");
  ASSERT_TRUE(top_mod_net);
  drvr_bterm->connect(top_mod_net);
  load0_a->connect(top_mod_net);
  load2_a->connect(top_mod_net);
  // Create ModITerm for mi0
  dbModITerm::create(mi0, "A", mod0->findModBTerm("A"));
  mi0->findModITerm("A")->connect(top_mod_net);
  num_warning = db_network_->checkAxioms();
  EXPECT_EQ(num_warning, 0);

  buffer_master = db_->findMaster("BUF_X4");
  ASSERT_TRUE(buffer_master);

  // Write verilog and check the content
  const std::string verilog_file_0 = "test_insert_buffer_after_drvr_port_pre.v";
  sta::writeVerilog(verilog_file_0.c_str(), true, false, {}, sta_->network());

  std::ifstream file_0(verilog_file_0);
  std::string content_0((std::istreambuf_iterator<char>(file_0)),
                        std::istreambuf_iterator<char>());

  const std::string expected_verilog_0 = R"(module top (X);
 input X;


 BUF_X1 load0_inst (.A(X));
 BUF_X1 load2_inst (.A(X));
 MOD0 mi0 (.A(X));
endmodule
module MOD0 (A);
 input A;


 MOD1 mi1 (.A(A));
endmodule
module MOD1 (A);
 input A;


 BUF_X1 load1_inst (.A(A));
endmodule
)";

  EXPECT_EQ(content_0, expected_verilog_0);

  //-----------------------------------------------------------------
  // Insert buffer
  //-----------------------------------------------------------------
  dbInst* new_buffer = net->insertBufferAfterDriver(drvr_bterm, buffer_master);
  ASSERT_TRUE(new_buffer);
  num_warning = db_network_->checkAxioms();
  EXPECT_EQ(num_warning, 0);

  // Verify connections
  EXPECT_EQ(drvr_bterm->getNet()->getName(), std::string("X"));
  EXPECT_EQ(new_buffer->findITerm("A")->getNet(), drvr_bterm->getNet());
  EXPECT_EQ(new_buffer->findITerm("Z")->getNet()->getName(),
            std::string("net"));

  // Write verilog and check the content
  const std::string verilog_file_1 = "test_insert_buffer_after_drvr_port.v";
  sta::writeVerilog(verilog_file_1.c_str(), true, false, {}, sta_->network());

  std::ifstream file_1(verilog_file_1);
  std::string content_1((std::istreambuf_iterator<char>(file_1)),
                        std::istreambuf_iterator<char>());

  const std::string expected_verilog_1 = R"(module top (X);
 input X;

 wire net;

 BUF_X4 buf (.A(X),
    .Z(net));
 BUF_X1 load0_inst (.A(net));
 BUF_X1 load2_inst (.A(net));
 MOD0 mi0 (.A(net));
endmodule
module MOD0 (A);
 input A;


 MOD1 mi1 (.A(A));
endmodule
module MOD1 (A);
 input A;


 BUF_X1 load1_inst (.A(A));
endmodule
)";

  EXPECT_EQ(content_1, expected_verilog_1);

  // Clean up
  removeFile(verilog_file_0);
  removeFile(verilog_file_1);
}

}  // namespace odb
