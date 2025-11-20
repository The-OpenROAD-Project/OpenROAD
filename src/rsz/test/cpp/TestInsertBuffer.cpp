#include <fstream>
#include <iterator>
#include <string>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "gtest/gtest.h"
#include "odb/db.h"
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

/*
 * insertBufferBeforeLoad() Case1
 *
 * This test case constructs a hierarchical netlist. The top-level module
 * contains a constant driver (drvr_inst), two buffer loads (load0_inst,
 * load2_inst), a top-level output port (load_output), and a hierarchical
 * instance (mi0).
 *
 * The single top-level net "net" connects the driver to all these loads.
 * The connection to mi0 propagates down through the hierarchy (mi0 -> mi1)
 * to eventually drive another buffer load (load1_inst) inside the MOD1 module.
 *
 * [Pre ECO]
 *
 *                      +-----------+
 *                      | LOGIC0_X1 |
 *                      | drvr_inst |-----.
 *                      +-----------+     |
 *                            .Z          | (net "net")
 *      +-------------------+-------------+------------------+
 *      |                   |             |                  |
 *      | .A                | .A          |                  |
 * +----v------+       +----v------+      |             +----v-------+
 * |  BUF_X1   |       |  BUF_X1   |      |             | Top Output |
 * | load0_inst|       | load2_inst|      |             | load_output|
 * +-----------+       +-----------+      |             +------------+
 *                                        | .A
 *                           +------------v----------------------+
 *                           | MOD0 mi0   |                      |
 *                           |            | .A                   |
 *                           |   +--------v------------------+   |
 *                           |   | MOD1   |                  |   |
 *                           |   | mi1    | .A               |   |
 *                           |   |    +---v---------------+  |   |
 *                           |   |    |  BUF_X1           |  |   |
 *                           |   |    | mi0/mi1/load1_inst|  |   |
 *                           |   |    +-------------------+  |   |
 *
 * The test then proceeds to insert buffers one by one before each load:
 * 1. Before `load0_inst` (in the top module).
 * 2. Before `load1_inst` (inside the `mi0/mi1` hierarchy).
 * 3. Before `load2_inst` (in the top module).
 * 4. Before the top-level port `load_output`.
 *
 * After each insertion, it verifies the correctness of the resulting netlist
 * by writing it out to a Verilog file and comparing it against an expected
 * output.
 *
 * [Post ECO]
 *                      +-----------+
 *                      | LOGIC0_X1 |
 *                      | drvr_inst |------.
 *                      +-----------+      |
 *                            .Z           | (net "net")
 *      +---------------------+------------+-----------------+
 * (NEW)| .A           (NEW)  | .A         | .A       (NEW)  | .A
 * +----v------+         +----v------+     |            +----v------+
 * |  BUF_X4   |         |  BUF_X4   |     |            |  BUF_X4   |
 * |    buf    |         |   buf_1   |     |            |   buf_2   |
 * +-----------+         +-----------+     |            +-----------+
 *      | .Z                  | .Z         |                 | .Z
 *      | (net_load)          |            |                 | (load_output)
 *      |                     |(net_load_1)|                 |
 * +----v------+         +----v------+     |          +----v-------+
 * |  BUF_X1   |         |  BUF_X1   |     |          | Top Output |
 * | load0_inst|         | load2_inst|     |          | load_output|
 * +-----------+         +-----------+     |          +------------+
 *                                         |
 *                                         | .A
 *                            +------------v----------------------+
 *                            | MOD0 mi0   |                      |
 *                            |            | .A                   |
 *                            |   +--------v------------------+   |
 *                            |   | MOD1   |                  |   |
 *                            |   | mi1    | .A   (NEW)       |   |
 *                            |   |    +---v---------------+  |   |
 *                            |   |    |  BUF_X4           |  |   |
 *                            |   |    | mi0/mi1/buf       |  |   |
 *                            |   |    +----------+--------+  |   |
 *                            |   |        |                  |   |
 *                            |   |    +---v---------------+  |   |
 *                            |   |    |  BUF_X1           |  |   |
 *                            |   |    | mi0/mi1/load1_inst|  |   |
 *                            |   |    +-------------------+  |   |
 */
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