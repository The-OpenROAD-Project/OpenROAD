// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// run() mechanics: several registers in one pass, the candidate filter,
// and loads on a multi-fanout output.

#include <string>
#include <unordered_set>

#include "SequentialUnmapper.h"
#include "SequentialUnmapperFixture.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "tst/fixture.h"

namespace {

class SequentialUnmapperRunTest : public ret::SequentialUnmapperFixture
{
};

// Two enable flops of different clock edges in one run().
//
//   d1 ──┐                                   d1 ──┐
//        ├─►[DFFE_PP ff1]──► q1                   ├─►[mux]──►[DFF_P]──► q1
//   e1 ──┘                                   e1 ──┘
//   d2 ──┐                                   d2 ──┐
//        ├─►[DFFE_NP ff2]──► q2                   ├─►[mux]──►[DFF_N]──► q2
//   e2 ──┘                                   e2 ──┘
TEST_F(SequentialUnmapperRunTest, MixedEdgesOneRun)
{
  ASSERT_NO_FATAL_FAILURE(setUpChipForTest({kBasic, kNegedge}));
  makeInputPorts({"d1", "e1", "d2", "e2", "clk"});
  makeCell("DFFE_PP",
           "ff1",
           {{"d1", "D"}, {"e1", "EN"}, {"clk", "CK"}, {"q1", "Q"}});
  makeOutputPort("q1");
  makeCell("DFFE_NP",
           "ff2",
           {{"d2", "D"}, {"e2", "EN"}, {"clk", "CK"}, {"q2", "Q"}});
  makeOutputPort("q2");

  ASSERT_NO_FATAL_FAILURE(unmapAll(2));
  EXPECT_EQ(countMaster("DFF_P"), 1);
  EXPECT_EQ(countMaster("DFF_N"), 1);
  EXPECT_EQ(countMaster("AND2"), 4);
  EXPECT_EQ(countMaster("OR2"), 2);
  EXPECT_EQ(countMaster("INV"), 2);

  odb::dbInst* base_pp = getInstByMasterName("DFF_P");
  odb::dbInst* base_np = getInstByMasterName("DFF_N");
  ASSERT_NE(base_pp, nullptr);
  ASSERT_NE(base_np, nullptr);
  expectSameFunction(base_pp, "DFFE_PP", {{"D", "d1"}, {"EN", "e1"}});
  expectSameFunction(base_np, "DFFE_NP", {{"D", "d2"}, {"EN", "e2"}});

  odb::dbBTerm* port1 = block_->findBTerm("q1");
  odb::dbBTerm* port2 = block_->findBTerm("q2");
  ASSERT_NE(port1, nullptr);
  ASSERT_NE(port2, nullptr);
  EXPECT_EQ(port1->getNet(), base_pp->findITerm("Q")->getNet());
  EXPECT_EQ(port2->getNet(), base_np->findITerm("Q")->getNet());
}

// run(&candidates) must touch only the listed registers.
TEST_F(SequentialUnmapperRunTest, CandidateFilter)
{
  ASSERT_NO_FATAL_FAILURE(setUpChipForTest({kBasic}));
  makeInputPorts({"d1", "d2", "clk"});
  odb::dbInst* ff1 = makeCell(
      "DFF_P_QN", "ff1", {{"d1", "D"}, {"clk", "CK"}, {"qn1", "QN"}});
  makeOutputPort("qn1");
  odb::dbInst* ff2 = makeCell(
      "DFF_P_QN", "ff2", {{"d2", "D"}, {"clk", "CK"}, {"qn2", "QN"}});
  makeOutputPort("qn2");
  ASSERT_NE(ff1, nullptr);
  ASSERT_NE(ff2, nullptr);

  ret::SequentialUnmapper unmapper(getDb(), getSta(), getLogger());
  ASSERT_TRUE(unmapper.prepare());
  const std::unordered_set<odb::dbInst*> candidates = {ff1};
  unmapper.run(&candidates);
  expectSane();

  EXPECT_EQ(countMaster("DFF_P"), 1);
  EXPECT_EQ(countMaster("INV"), 1);
  EXPECT_EQ(block_->findInst("ff1"), nullptr) << "ff1 should be rebuilt";

  odb::dbInst* ff2_after = block_->findInst("ff2");
  ASSERT_NE(ff2_after, nullptr) << "ff2 was not a candidate and must survive";
  EXPECT_EQ(ff2_after, ff2);
  EXPECT_EQ(ff2_after->getMaster()->getName(), std::string("DFF_P_QN"));
  EXPECT_EQ(countMaster("DFF_P_QN"), 1);
  odb::dbNet* qn2 = block_->findNet("qn2");
  ASSERT_NE(qn2, nullptr) << "ff2's output net must not be destroyed";
  EXPECT_EQ(driverOf(qn2), ff2);
}

// Two flops of the same cell in one run(), sharing the enable net; every
// new instance name must be unique.
TEST_F(SequentialUnmapperRunTest, SharedEnableTwoFlops)
{
  ASSERT_NO_FATAL_FAILURE(setUpChipForTest({kBasic}));
  makeInputPorts({"d1", "d2", "e1", "clk"});
  makeCell("DFFE_PP",
           "ff1",
           {{"d1", "D"}, {"e1", "EN"}, {"clk", "CK"}, {"q1", "Q"}});
  makeOutputPort("q1");
  makeCell("DFFE_PP",
           "ff2",
           {{"d2", "D"}, {"e1", "EN"}, {"clk", "CK"}, {"q2", "Q"}});
  makeOutputPort("q2");

  ASSERT_NO_FATAL_FAILURE(unmapAll(2));
  EXPECT_EQ(countMaster("DFF_P"), 2);
  EXPECT_EQ(countMaster("AND2"), 4);
  EXPECT_EQ(countMaster("OR2"), 2);
  EXPECT_EQ(countMaster("INV"), 2);

  std::unordered_set<std::string> names;
  for (odb::dbInst* inst : block_->getInsts()) {
    EXPECT_TRUE(names.insert(inst->getName()).second)
        << "duplicate instance name " << inst->getName();
  }

  odb::dbBTerm* port1 = block_->findBTerm("q1");
  odb::dbBTerm* port2 = block_->findBTerm("q2");
  ASSERT_NE(port1, nullptr);
  ASSERT_NE(port2, nullptr);
  odb::dbInst* base1 = driverOf(port1->getNet());
  odb::dbInst* base2 = driverOf(port2->getNet());
  ASSERT_TRUE(isMaster(base1, "DFF_P"));
  ASSERT_TRUE(isMaster(base2, "DFF_P"));
  EXPECT_NE(base1, base2);
  expectSameFunction(base1, "DFFE_PP", {{"D", "d1"}, {"EN", "e1"}});
  expectSameFunction(base2, "DFFE_PP", {{"D", "d2"}, {"EN", "e1"}});
}

// One flop whose output fans out to two gates and a port: every load moves
// to the rebuilt net and the unrelated input of the AND2 is untouched.
TEST_F(SequentialUnmapperRunTest, FanoutPreserved)
{
  ASSERT_NO_FATAL_FAILURE(setUpChipForTest({kBasic}));
  makeInputPorts({"d1", "x1", "clk"});
  makeCell("DFF_P_QN", "ff1", {{"d1", "D"}, {"clk", "CK"}, {"qn1", "QN"}});
  odb::dbInst* load_inv
      = makeCell("INV", "load_inv", {{"qn1", "A"}, {"o_inv", "Y"}});
  odb::dbInst* load_and = makeCell(
      "AND2", "load_and", {{"qn1", "A"}, {"x1", "B"}, {"o_and", "Y"}});
  makeOutputPort("qn1");
  makeOutputPort("o_inv");
  makeOutputPort("o_and");
  ASSERT_NE(load_inv, nullptr);
  ASSERT_NE(load_and, nullptr);

  ASSERT_NO_FATAL_FAILURE(unmapAll(1));
  EXPECT_EQ(countMaster("DFF_P"), 1);
  EXPECT_EQ(countMaster("INV"), 2);
  EXPECT_EQ(block_->findNet("qn1"), nullptr)
      << "old output net must be destroyed";

  odb::dbBTerm* port = block_->findBTerm("qn1");
  ASSERT_NE(port, nullptr);
  odb::dbNet* out = port->getNet();
  ASSERT_NE(out, nullptr);
  odb::dbInst* out_inv = driverOf(out);
  ASSERT_TRUE(isMaster(out_inv, "INV"));
  odb::dbInst* base = driverOf(inputNets(out_inv).at(0));
  ASSERT_TRUE(isMaster(base, "DFF_P"));

  EXPECT_EQ(load_inv->findITerm("A")->getNet(), out) << "INV load not moved";
  EXPECT_EQ(load_and->findITerm("A")->getNet(), out) << "AND2 load not moved";
  EXPECT_EQ(load_and->findITerm("B")->getNet(), block_->findNet("x1"))
      << "unrelated input of the AND2 must be untouched";
  int loads_on_out = 0;
  for (odb::dbITerm* iterm : out->getITerms()) {
    if (iterm->getIoType() == odb::dbIoType::INPUT) {
      loads_on_out++;
    }
  }
  EXPECT_EQ(loads_on_out, 2);
}

// A register whose next state reads an unconnected input is left alone.
TEST_F(SequentialUnmapperRunTest, UnconnectedInputIsSkipped)
{
  ASSERT_NO_FATAL_FAILURE(setUpChipForTest({kBasic}));
  makeInputPorts({"d1", "clk"});
  odb::dbInst* ff
      = makeCell("DFFE_PP", "ff1", {{"d1", "D"}, {"clk", "CK"}, {"q1", "Q"}});
  makeOutputPort("q1");
  ASSERT_NE(ff, nullptr);

  // Not unmapAll: the netlist checks reject the open input.
  ret::SequentialUnmapper unmapper(getDb(), getSta(), getLogger());
  ASSERT_TRUE(unmapper.prepare());
  EXPECT_EQ(unmapper.run(), 0);
  EXPECT_EQ(block_->findInst("ff1"), ff);
  EXPECT_EQ(countMaster("DFFE_PP"), 1);
  EXPECT_EQ(countMaster("DFF_P"), 0);
  EXPECT_EQ(countMaster("AND2"), 0);
}

// In a hierarchical netlist the cells that replace a register are created
// in the register's module: ff1 is in `child` (instance u1), ff2 in the top
// module, and each one's base flop and gates stay where it was.
TEST_F(SequentialUnmapperRunTest, CellsStayInTheRegistersModule)
{
  ASSERT_NO_FATAL_FAILURE(setUpChipForTest({kBasic}));
  getDb()->setHierarchy(true);
  odb::dbModule* top = block_->getTopModule();
  odb::dbModule* child = odb::dbModule::create(block_, "child");
  ASSERT_NE(child, nullptr);
  ASSERT_NE(odb::dbModInst::create(top, child, "u1"), nullptr);
  makeInputPorts({"d1", "e1", "d2", "e2", "clk"});

  tst::InstOptions in_child;
  in_child.parent_module = child;
  in_child.iterms = {{"d1", "D"}, {"e1", "EN"}, {"clk", "CK"}, {"q1", "Q"}};
  makeInst(block_, getDb()->findMaster("DFFE_PP"), "u1/ff1", in_child);
  makeOutputPort("q1");

  makeCell("DFFE_PP",
           "ff2",
           {{"d2", "D"}, {"e2", "EN"}, {"clk", "CK"}, {"q2", "Q"}});
  makeOutputPort("q2");

  ASSERT_NO_FATAL_FAILURE(unmapAll(2));
  EXPECT_EQ(countMaster("DFFE_PP"), 0);
  EXPECT_EQ(countMaster("DFF_P"), 2);
  int in_child_count = 0;
  int in_top_count = 0;
  for (odb::dbInst* inst : block_->getInsts()) {
    if (inst->getModule() == child) {
      in_child_count++;
      EXPECT_TRUE(inst->getName().starts_with("u1/"))
          << inst->getName() << " is in `child` without its instance path";
    } else {
      in_top_count++;
      EXPECT_EQ(inst->getModule(), top);
    }
  }
  // One enable flop becomes DFF_P + INV + 2 AND2 + OR2.
  EXPECT_EQ(in_child_count, 5);
  EXPECT_EQ(in_top_count, 5);

  odb::dbBTerm* port = block_->findBTerm("q1");
  ASSERT_NE(port, nullptr);
  odb::dbInst* base = driverOf(port->getNet());
  ASSERT_TRUE(isMaster(base, "DFF_P"));
  EXPECT_EQ(base->getModule(), child);
  EXPECT_EQ(base->findITerm("CK")->getNet(), block_->findNet("clk"));
}

// A flop with Q and QN both loaded: the Q loads read the base flop, the QN
// loads an inverter of it, and both old nets are deleted.
TEST_F(SequentialUnmapperRunTest, BothOutputsLoaded)
{
  ASSERT_NO_FATAL_FAILURE(setUpChipForTest({kBasic, kTwoOutputs}));
  makeInputPorts({"d1", "clk"});
  makeCell("DFF_P_QQN",
           "ff1",
           {{"d1", "D"}, {"clk", "CK"}, {"q1", "Q"}, {"qn1", "QN"}});
  odb::dbInst* load_q = makeCell("INV", "load_q", {{"q1", "A"}, {"o_q", "Y"}});
  odb::dbInst* load_qn
      = makeCell("INV", "load_qn", {{"qn1", "A"}, {"o_qn", "Y"}});
  makeOutputPort("o_q");
  makeOutputPort("o_qn");

  ASSERT_NO_FATAL_FAILURE(unmapAll(1));
  EXPECT_EQ(countMaster("DFF_P_QQN"), 0);
  EXPECT_EQ(countMaster("DFF_P"), 1);
  EXPECT_EQ(countMaster("INV"), 3);
  EXPECT_EQ(block_->findNet("q1"), nullptr);
  EXPECT_EQ(block_->findNet("qn1"), nullptr);

  odb::dbInst* base = driverOf(load_q->findITerm("A")->getNet());
  ASSERT_TRUE(isMaster(base, "DFF_P"));
  odb::dbInst* inv = driverOf(load_qn->findITerm("A")->getNet());
  ASSERT_TRUE(isMaster(inv, "INV"));
  EXPECT_EQ(driverOf(inputNets(inv).at(0)), base);
}

// An inverted output with no load gets no inverter, and its net goes with
// the register.
TEST_F(SequentialUnmapperRunTest, UnloadedOutputGetsNoInverter)
{
  ASSERT_NO_FATAL_FAILURE(setUpChipForTest({kBasic, kTwoOutputs}));
  makeInputPorts({"d1", "clk"});
  makeCell("DFF_P_QQN",
           "ff1",
           {{"d1", "D"}, {"clk", "CK"}, {"q1", "Q"}, {"qn1", "QN"}});
  makeCell("INV", "load_q", {{"q1", "A"}, {"o_q", "Y"}});
  makeOutputPort("o_q");

  ASSERT_NO_FATAL_FAILURE(unmapAll(1));
  EXPECT_EQ(countMaster("DFF_P"), 1);
  EXPECT_EQ(countMaster("INV"), 1);
  EXPECT_EQ(block_->findNet("qn1"), nullptr);
}

// A register with an output that does not read the state is left alone.
TEST_F(SequentialUnmapperRunTest, OutputNotReadingStateIsSkipped)
{
  ASSERT_NO_FATAL_FAILURE(setUpChipForTest({kBasic, kTwoOutputs}));
  makeInputPorts({"d1", "clk"});
  odb::dbInst* ff
      = makeCell("DFF_P_ODD",
                 "ff1",
                 {{"d1", "D"}, {"clk", "CK"}, {"q1", "Q"}, {"y1", "Y"}});
  makeOutputPort("q1");
  makeOutputPort("y1");

  ASSERT_NO_FATAL_FAILURE(unmapAll(0));
  EXPECT_EQ(block_->findInst("ff1"), ff);
  EXPECT_EQ(countMaster("DFF_P"), 0);
  EXPECT_EQ(block_->findNet("y1"), ff->findITerm("Y")->getNet());
}

}  // namespace
