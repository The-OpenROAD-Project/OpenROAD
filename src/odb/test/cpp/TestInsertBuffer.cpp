#include <filesystem>
#include <fstream>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "sta/Liberty.hh"
#include "sta/VerilogWriter.hh"
#include "tst/fixture.h"

namespace odb {

class TestInsertBuffer : public tst::Fixture
{
 protected:
  void SetUp() override
  {
    library_ = readLiberty(
        "tools/OpenROAD/src/dbSta/test/Nangate45/Nangate45_typ.lib");
    loadTechAndLib("tech",
                   "Nangate45.lef",
                   "tools/OpenROAD/src/dbSta/test/Nangate45/Nangate45.lef");

    dbChip* chip = dbChip::create(db_.get(), db_->getTech());
    block_ = dbBlock::create(chip, "top");
    // Turn on hierarchy
    db_network_ = sta_->getDbNetwork();
    db_network_->setHierarchy();
    sta_->postReadDef(block_);
  }

  sta::LibertyLibrary* library_;
  sta::dbNetwork* db_network_ = nullptr;
  dbBlock* block_ = nullptr;
};

TEST_F(TestInsertBuffer, BeforeLoad_Case1)
{
  auto sort_lines = [](const std::string& s) {
    std::vector<std::string> lines;
    std::string line;
    std::istringstream iss(s);
    while (std::getline(iss, line)) {
      lines.push_back(line);
    }
    std::sort(lines.begin(), lines.end());
    std::ostringstream oss;
    for (const auto& l : lines) {
      oss << l << "\n";
    }
    return oss.str();
  };

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

  dbITerm* drvr_z = drvr_inst->findITerm("Z");
  ASSERT_TRUE(drvr_z);
  drvr_z->connect(net);

  dbITerm* load0_a = load0_inst->findITerm("A");
  ASSERT_TRUE(load0_a);
  load0_a->connect(net);

  dbITerm* load2_a = load2_inst->findITerm("A");
  ASSERT_TRUE(load2_a);
  load2_a->connect(net);

  dbBTerm* load_output_bterm = dbBTerm::create(net, "load_output");
  ASSERT_TRUE(load_output_bterm);
  load_output_bterm->setIoType(dbIoType::OUTPUT);
  load_output_bterm->connect(net);

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
  const std::string verilog_file_0 = "test_insert_buffer_pre.v";
  sta::writeVerilog(verilog_file_0.c_str(), true, false, {}, sta_->network());

  std::ifstream file_0(verilog_file_0);
  std::string content_0((std::istreambuf_iterator<char>(file_0)),
                        std::istreambuf_iterator<char>());

  const std::string expected_verilog_0
      = "module top (load_output);\n"
        " output load_output;\n\n"
        " wire net;\n\n"
        " LOGIC0_X1 drvr_inst (.Z(net));\n"
        " BUF_X1 load0_inst (.A(net));\n"
        " BUF_X1 load2_inst (.A(net));\n"
        " MOD0 mi0 (.A(net));\n"
        " assign load_output = net;\n"
        "endmodule\n"
        "module MOD0 (A);\n"
        " input A;\n\n\n"
        " MOD1 mi1 (.A(A));\n"
        "endmodule\n"
        "module MOD1 (A);\n"
        " input A;\n\n\n"
        " BUF_X1 load1_inst (.A(A));\n"
        "endmodule\n";

  EXPECT_EQ(sort_lines(content_0), sort_lines(expected_verilog_0));

  buffer_master = db_->findMaster("BUF_X4");
  ASSERT_TRUE(buffer_master);

  // Insert buffer #1
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

  const std::string expected_verilog_1
      = "module top (load_output);\n"
        " output load_output;\n\n"
        " wire net_load;\n"
        " wire net;\n\n"
        " assign load_output = net;\n"
        " BUF_X4 buf (.A(net),\n"
        "    .Z(net_load));\n"
        " LOGIC0_X1 drvr_inst (.Z(net));\n"
        " BUF_X1 load0_inst (.A(net_load));\n"
        " BUF_X1 load2_inst (.A(net));\n"
        " MOD0 mi0 (.A(net));\n"
        "endmodule\n"
        "module MOD0 (A);\n"
        " input A;\n\n\n"
        " MOD1 mi1 (.A(A));\n"
        "endmodule\n"
        "module MOD1 (A);\n"
        " input A;\n\n\n"
        " BUF_X1 load1_inst (.A(A));\n"
        "endmodule\n";

  EXPECT_EQ(sort_lines(content_1), sort_lines(expected_verilog_1));

  // Insert buffer #2
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

  const std::string expected_verilog_2
      = "module top (load_output);\n"
        " output load_output;\n\n"
        " wire net_load;\n"
        " wire net;\n\n"
        " assign load_output = net;\n"
        " BUF_X4 buf (.A(net),\n"
        "    .Z(net_load));\n"
        " LOGIC0_X1 drvr_inst (.Z(net));\n"
        " BUF_X1 load0_inst (.A(net_load));\n"
        " BUF_X1 load2_inst (.A(net));\n"
        " MOD0 mi0 (.A(net));\n"
        "endmodule\n"
        "module MOD0 (A);\n"
        " input A;\n\n\n"
        " MOD1 mi1 (.A(A));\n"
        "endmodule\n"
        "module MOD1 (A);\n"
        " input A;\n\n"
        " wire net_load;\n\n"
        " BUF_X4 buf (.A(A),\n"
        "    .Z(net_load));\n"
        " BUF_X1 load1_inst (.A(net_load));\n"
        "endmodule\n";
  EXPECT_EQ(sort_lines(content_2), sort_lines(expected_verilog_2));

  // Insert buffer #3
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

  const std::string expected_verilog_3
      = "module top (load_output);\n"
        " output load_output;\n\n"
        " wire net_load_1;\n"
        " wire net_load;\n"
        " wire net;\n\n"
        " assign load_output = net;\n"
        " BUF_X4 buf_1 (.A(net),\n"
        "    .Z(net_load_1));\n"
        " BUF_X4 buf (.A(net),\n"
        "    .Z(net_load));\n"
        " LOGIC0_X1 drvr_inst (.Z(net));\n"
        " BUF_X1 load0_inst (.A(net_load));\n"
        " BUF_X1 load2_inst (.A(net_load_1));\n"
        " MOD0 mi0 (.A(net));\n"
        "endmodule\n"
        "module MOD0 (A);\n"
        " input A;\n\n\n"
        " MOD1 mi1 (.A(A));\n"
        "endmodule\n"
        "module MOD1 (A);\n"
        " input A;\n\n"
        " wire net_load;\n\n"
        " BUF_X4 buf (.A(A),\n"
        "    .Z(net_load));\n"
        " BUF_X1 load1_inst (.A(net_load));\n"
        "endmodule\n";

  EXPECT_EQ(sort_lines(content_3), sort_lines(expected_verilog_3));

  // Insert buffer #4
  dbInst* new_buffer4
      = net->insertBufferBeforeLoad(load_output_bterm, buffer_master);
  ASSERT_TRUE(new_buffer4);
  num_warning = db_network_->checkAxioms();
  EXPECT_EQ(num_warning, 0);

  // Verify connections for buffer #4
  EXPECT_EQ(load_output_bterm->getNet()->getName(), std::string("net_load_2"));
  EXPECT_EQ(new_buffer4->findITerm("A")->getNet(), net);
  EXPECT_EQ(new_buffer4->findITerm("Z")->getNet(), load_output_bterm->getNet());

  // Write verilog and check the content after inserting buffer #4
  const std::string verilog_file_4 = "test_insert_buffer_4.v";
  sta::writeVerilog(verilog_file_4.c_str(), true, false, {}, sta_->network());

  std::ifstream file_4(verilog_file_4);
  std::string content_4((std::istreambuf_iterator<char>(file_4)),
                        std::istreambuf_iterator<char>());

  const std::string expected_verilog_4
      = "module top (load_output);\n"
        " output load_output;\n\n"
        " wire net_load_2;\n"
        " wire net_load_1;\n"
        " wire net_load;\n"
        " wire net;\n\n"
        " assign load_output = net_load_2;\n"
        " BUF_X4 buf_2 (.A(net),\n"
        "    .Z(net_load_2));\n"
        " BUF_X4 buf_1 (.A(net),\n"
        "    .Z(net_load_1));\n"
        " BUF_X4 buf (.A(net),\n"
        "    .Z(net_load));\n"
        " LOGIC0_X1 drvr_inst (.Z(net));\n"
        " BUF_X1 load0_inst (.A(net_load));\n"
        " BUF_X1 load2_inst (.A(net_load_1));\n"
        " MOD0 mi0 (.A(net));\n"
        "endmodule\n"
        "module MOD0 (A);\n"
        " input A;\n\n\n"
        " MOD1 mi1 (.A(A));\n"
        "endmodule\n"
        "module MOD1 (A);\n"
        " input A;\n\n"
        " wire net_load;\n\n"
        " BUF_X4 buf (.A(A),\n"
        "    .Z(net_load));\n"
        " BUF_X1 load1_inst (.A(net_load));\n"
        "endmodule\n";

  EXPECT_EQ(sort_lines(content_4), sort_lines(expected_verilog_4));

  // Clean up
  std::filesystem::remove(verilog_file_0);
  std::filesystem::remove(verilog_file_1);
  std::filesystem::remove(verilog_file_2);
  std::filesystem::remove(verilog_file_3);
  std::filesystem::remove(verilog_file_4);
}

TEST_F(TestInsertBuffer, AfterDriver_Case1)
{
  auto sort_lines = [](const std::string& s) {
    std::vector<std::string> lines;
    std::string line;
    std::istringstream iss(s);
    while (std::getline(iss, line)) {
      lines.push_back(line);
    }
    std::sort(lines.begin(), lines.end());
    std::ostringstream oss;
    for (const auto& l : lines) {
      oss << l << "\n";
    }
    return oss.str();
  };

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

  const std::string expected_verilog_0
      = "module top (in);\n"
        " input in;\n\n"
        " wire net;\n\n"
        " BUF_X1 drvr_inst (.A(in),\n"
        "    .Z(net));\n"
        " BUF_X1 load0_inst (.A(net));\n"
        " BUF_X1 load2_inst (.A(net));\n"
        " MOD0 mi0 (.A(net));\n"
        "endmodule\n"
        "module MOD0 (A);\n"
        " input A;\n\n\n"
        " MOD1 mi1 (.A(A));\n"
        "endmodule\n"
        "module MOD1 (A);\n"
        " input A;\n\n\n"
        " BUF_X1 load1_inst (.A(A));\n"
        "endmodule\n";

  EXPECT_EQ(sort_lines(content_0), sort_lines(expected_verilog_0));

  buffer_master = db_->findMaster("BUF_X4");
  ASSERT_TRUE(buffer_master);

  // Insert buffer
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

  const std::string expected_verilog_1
      = "module top (in);\n"
        " input in;\n\n"
        " wire net_drvr;\n"
        " wire net;\n\n"
        " BUF_X4 buf (.A(net_drvr),\n"
        "    .Z(net));\n"
        " BUF_X1 drvr_inst (.A(in),\n"
        "    .Z(net_drvr));\n"
        " BUF_X1 load0_inst (.A(net));\n"
        " BUF_X1 load2_inst (.A(net));\n"
        " MOD0 mi0 (.A(net));\n"
        "endmodule\n"
        "module MOD0 (A);\n"
        " input A;\n\n\n"
        " MOD1 mi1 (.A(A));\n"
        "endmodule\n"
        "module MOD1 (A);\n"
        " input A;\n\n\n"
        " BUF_X1 load1_inst (.A(A));\n"
        "endmodule\n";

  EXPECT_EQ(sort_lines(content_1), sort_lines(expected_verilog_1));

  // Clean up
  std::filesystem::remove(verilog_file_0);
  std::filesystem::remove(verilog_file_1);
}

TEST_F(TestInsertBuffer, AfterDriver_Case2)
{
  auto sort_lines = [](const std::string& s) {
    std::vector<std::string> lines;
    std::string line;
    std::istringstream iss(s);
    while (std::getline(iss, line)) {
      lines.push_back(line);
    }
    std::sort(lines.begin(), lines.end());
    std::ostringstream oss;
    for (const auto& l : lines) {
      oss << l << "\n";
    }
    return oss.str();
  };

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

  const std::string expected_verilog_0
      = "module top (X);\n"
        " input X;\n\n\n"
        " BUF_X1 load0_inst (.A(X));\n"
        " BUF_X1 load2_inst (.A(X));\n"
        " MOD0 mi0 (.A(X));\n"
        "endmodule\n"
        "module MOD0 (A);\n"
        " input A;\n\n\n"
        " MOD1 mi1 (.A(A));\n"
        "endmodule\n"
        "module MOD1 (A);\n"
        " input A;\n\n\n"
        " BUF_X1 load1_inst (.A(A));\n"
        "endmodule\n";

  EXPECT_EQ(sort_lines(content_0), sort_lines(expected_verilog_0));

  // Insert buffer
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

  const std::string expected_verilog_1
      = "module top (X);\n"
        " input X;\n\n"
        " wire net;\n\n"
        " BUF_X4 buf (.A(X),\n"
        "    .Z(net));\n"
        " BUF_X1 load0_inst (.A(net));\n"
        " BUF_X1 load2_inst (.A(net));\n"
        " MOD0 mi0 (.A(net));\n"
        "endmodule\n"
        "module MOD0 (A);\n"
        " input A;\n\n\n"
        " MOD1 mi1 (.A(A));\n"
        "endmodule\n"
        "module MOD1 (A);\n"
        " input A;\n\n\n"
        " BUF_X1 load1_inst (.A(A));\n"
        "endmodule\n";

  EXPECT_EQ(sort_lines(content_1), sort_lines(expected_verilog_1));

  // Clean up
  std::filesystem::remove(verilog_file_0);
  std::filesystem::remove(verilog_file_1);
}

}  // namespace odb