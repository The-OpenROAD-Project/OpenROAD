// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// run() mechanics: several registers in one pass, the candidate filter,
// and loads on a multi-fanout output.

#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "SequentialUnmapperFixture.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "ret/SequentialUnmapper.h"
#include "sta/Sequential.hh"
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

  tst::InstOptions op1;
  op1.iterms = {{"d1", "D"}, {"e1", "EN"}, {"clk", "CK"}, {"q1", "Q"}};
  makeInst(block_, getDb()->findMaster("DFFE_PP"), "ff1", op1);
  makeOutputPort("q1");

  tst::InstOptions op2;
  op2.iterms = {{"d2", "D"}, {"e2", "EN"}, {"clk", "CK"}, {"q2", "Q"}};
  makeInst(block_, getDb()->findMaster("DFFE_NP"), "ff2", op2);
  makeOutputPort("q2");

  const sta::Sequential* seq_pp = sequentialOf("DFFE_PP");
  const sta::Sequential* seq_np = sequentialOf("DFFE_NP");
  ASSERT_NE(seq_pp, nullptr);
  ASSERT_NE(seq_np, nullptr);
  const LeafNets leaf_pp
      = {{"D", block_->findNet("d1")}, {"EN", block_->findNet("e1")}};
  const LeafNets leaf_np
      = {{"D", block_->findNet("d2")}, {"EN", block_->findNet("e2")}};

  ret::SequentialUnmapper unmapper(getDb(), getSta(), getLogger());
  ASSERT_TRUE(unmapper.prepare());
  unmapper.run();
  expectSane();

  EXPECT_EQ(countMaster("DFF_P"), 1);
  EXPECT_EQ(countMaster("DFF_N"), 1);
  EXPECT_EQ(countMaster("AND2"), 4);
  EXPECT_EQ(countMaster("OR2"), 2);
  EXPECT_EQ(countMaster("INV"), 2);

  odb::dbInst* base_pp = getInstByMasterName("DFF_P");
  odb::dbInst* base_np = getInstByMasterName("DFF_N");
  ASSERT_NE(base_pp, nullptr);
  ASSERT_NE(base_np, nullptr);

  odb::dbNet* state_pp = base_pp->findITerm("Q")->getNet();
  odb::dbNet* state_np = base_np->findITerm("Q")->getNet();
  expectSameFunction(
      base_pp->findITerm("D")->getNet(), seq_pp, leaf_pp, state_pp);
  expectSameFunction(
      base_np->findITerm("D")->getNet(), seq_np, leaf_np, state_np);

  odb::dbBTerm* port1 = block_->findBTerm("q1");
  odb::dbBTerm* port2 = block_->findBTerm("q2");
  ASSERT_NE(port1, nullptr);
  ASSERT_NE(port2, nullptr);
  EXPECT_EQ(port1->getNet(), state_pp);
  EXPECT_EQ(port2->getNet(), state_np);
}

// run(&candidates) must touch only the listed registers.
TEST_F(SequentialUnmapperRunTest, CandidateFilter)
{
  ASSERT_NO_FATAL_FAILURE(setUpChipForTest({kBasic}));
  makeInputPorts({"d1", "d2", "clk"});

  tst::InstOptions op1;
  op1.iterms = {{"d1", "D"}, {"clk", "CK"}, {"qn1", "QN"}};
  odb::dbInst* ff1
      = makeInst(block_, getDb()->findMaster("DFF_P_QN"), "ff1", op1);
  makeOutputPort("qn1");

  tst::InstOptions op2;
  op2.iterms = {{"d2", "D"}, {"clk", "CK"}, {"qn2", "QN"}};
  odb::dbInst* ff2
      = makeInst(block_, getDb()->findMaster("DFF_P_QN"), "ff2", op2);
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

  tst::InstOptions op1;
  op1.iterms = {{"d1", "D"}, {"e1", "EN"}, {"clk", "CK"}, {"q1", "Q"}};
  makeInst(block_, getDb()->findMaster("DFFE_PP"), "ff1", op1);
  makeOutputPort("q1");

  tst::InstOptions op2;
  op2.iterms = {{"d2", "D"}, {"e1", "EN"}, {"clk", "CK"}, {"q2", "Q"}};
  makeInst(block_, getDb()->findMaster("DFFE_PP"), "ff2", op2);
  makeOutputPort("q2");

  const sta::Sequential* seq = sequentialOf("DFFE_PP");
  ASSERT_NE(seq, nullptr);
  const LeafNets leaf1
      = {{"D", block_->findNet("d1")}, {"EN", block_->findNet("e1")}};
  const LeafNets leaf2
      = {{"D", block_->findNet("d2")}, {"EN", block_->findNet("e1")}};

  ret::SequentialUnmapper unmapper(getDb(), getSta(), getLogger());
  ASSERT_TRUE(unmapper.prepare());
  unmapper.run();
  expectSane();

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
  expectSameFunction(
      base1->findITerm("D")->getNet(), seq, leaf1, port1->getNet());
  expectSameFunction(
      base2->findITerm("D")->getNet(), seq, leaf2, port2->getNet());
}

// One flop whose output fans out to two gates and a port: every load moves
// to the rebuilt net and the unrelated input of the AND2 is untouched.
TEST_F(SequentialUnmapperRunTest, FanoutPreserved)
{
  ASSERT_NO_FATAL_FAILURE(setUpChipForTest({kBasic}));
  makeInputPorts({"d1", "x1", "clk"});

  tst::InstOptions op;
  op.iterms = {{"d1", "D"}, {"clk", "CK"}, {"qn1", "QN"}};
  makeInst(block_, getDb()->findMaster("DFF_P_QN"), "ff1", op);

  tst::InstOptions oi;
  oi.iterms = {{"qn1", "A"}, {"o_inv", "Y"}};
  odb::dbInst* load_inv
      = makeInst(block_, getDb()->findMaster("INV"), "load_inv", oi);
  tst::InstOptions oa;
  oa.iterms = {{"qn1", "A"}, {"x1", "B"}, {"o_and", "Y"}};
  odb::dbInst* load_and
      = makeInst(block_, getDb()->findMaster("AND2"), "load_and", oa);
  makeOutputPort("qn1");
  makeOutputPort("o_inv");
  makeOutputPort("o_and");
  ASSERT_NE(load_inv, nullptr);
  ASSERT_NE(load_and, nullptr);

  ret::SequentialUnmapper unmapper(getDb(), getSta(), getLogger());
  ASSERT_TRUE(unmapper.prepare());
  unmapper.run();
  expectSane();

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
  for (odb::dbITerm* it : out->getITerms()) {
    if (it->getIoType() == odb::dbIoType::INPUT) {
      loads_on_out++;
    }
  }
  EXPECT_EQ(loads_on_out, 2);
}

}  // namespace
