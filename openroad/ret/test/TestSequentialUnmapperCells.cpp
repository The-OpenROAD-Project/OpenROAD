// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// One register per test: each liberty shape the unmapper handles, on
// hand-made libraries with deliberate traps (an inverting-storage flop that
// is smaller than the plain one, a two-output tie cell, a negedge flop that
// is smaller than the posedge one).

#include <string>
#include <utility>
#include <vector>

#include "SequentialUnmapper.h"
#include "SequentialUnmapperFixture.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "sta/Liberty.hh"
#include "sta/Sequential.hh"
#include "tst/fixture.h"

namespace {

class SequentialUnmapperCellTest : public ret::SequentialUnmapperFixture
{
};

TEST_F(SequentialUnmapperCellTest, FixtureLoads)
{
  ASSERT_NO_FATAL_FAILURE(setUpChipForTest({kBasic}));
  EXPECT_NE(getDb()->findMaster("DFF_P"), nullptr);
  EXPECT_NE(getDb()->findMaster("INV"), nullptr);
  EXPECT_NE(getDb()->findMaster("DFF_PN0"), nullptr);
  sta::LibertyCell* dff = getSta()->getDbNetwork()->findLibertyCell("DFF_P");
  ASSERT_NE(dff, nullptr) << "liberty did not load or names mismatch the LEF";
  EXPECT_TRUE(dff->isSequential());
}

// The base cell must be the plain posedge flop, not the smaller
// inverting-storage DFF_P_ND, not the smaller DFF_P_2CK with its second
// clock pin, not the smaller DFF_P_SL with an input a new register would
// leave open, not the smaller DFF_P_NA without timing arcs, and not the
// smaller negedge DFF_N.
TEST_F(SequentialUnmapperCellTest, PicksPlainPosedgeBase)
{
  ASSERT_NO_FATAL_FAILURE(setUpChipForTest({kBasic, kNegedge}));
  ret::SequentialUnmapper unmapper(getDb(), getSta(), getLogger());
  ASSERT_TRUE(unmapper.prepare()) << "no base register or no inverter found";
  ASSERT_NE(unmapper.getBaseSequentialMaster(), nullptr);
  EXPECT_EQ(unmapper.getBaseSequentialMaster()->getName(),
            std::string("DFF_P"));
  ASSERT_NE(unmapper.getNegedgeSequentialMaster(), nullptr);
  EXPECT_EQ(unmapper.getNegedgeSequentialMaster()->getName(),
            std::string("DFF_N"));
}

// A flop with a second pin marked as a clock is left alone: the unmapper
// tells the clock pin of a cell from its other pins by the mark.
TEST_F(SequentialUnmapperCellTest, SkipsSecondClockPin)
{
  ASSERT_NO_FATAL_FAILURE(setUpChipForTest({kBasic}));
  makeInputPorts({"d1", "g1", "clk"});
  tst::InstOptions op;
  op.iterms = {{"d1", "D"}, {"g1", "CK2"}, {"clk", "CK"}, {"q1", "Q"}};
  makeInst(block_, getDb()->findMaster("DFF_P_2CK"), "ff1", op);
  makeOutputPort("q1");

  ret::SequentialUnmapper unmapper(getDb(), getSta(), getLogger());
  ASSERT_TRUE(unmapper.prepare());
  unmapper.run();
  expectSane();
  EXPECT_EQ(countMaster("DFF_P_2CK"), 1);
  EXPECT_EQ(countMaster("DFF_P"), 0);
  EXPECT_NE(block_->findInst("ff1"), nullptr);
}

// A flop with a tri-state output is left alone: the base cell always drives.
TEST_F(SequentialUnmapperCellTest, SkipsTristateOutput)
{
  ASSERT_NO_FATAL_FAILURE(setUpChipForTest({kBasic}));
  makeInputPorts({"d1", "oe1", "clk"});
  tst::InstOptions op;
  op.iterms = {{"d1", "D"}, {"oe1", "OE"}, {"clk", "CK"}, {"q1", "Q"}};
  makeInst(block_, getDb()->findMaster("DFF_P_TQ"), "ff1", op);
  makeOutputPort("q1");

  ret::SequentialUnmapper unmapper(getDb(), getSta(), getLogger());
  ASSERT_TRUE(unmapper.prepare());
  unmapper.run();
  expectSane();
  EXPECT_EQ(countMaster("DFF_P_TQ"), 1);
  EXPECT_EQ(countMaster("DFF_P"), 0);
  EXPECT_NE(block_->findInst("ff1"), nullptr);
}

// A cell whose LEF pins are not its liberty pins (an extra LEF input, a
// missing pin, another direction) is never the cell of a new instance: the
// enable flop unmaps to the cells of the basic library, not to the cheaper
// ones of kPinMismatch.
TEST_F(SequentialUnmapperCellTest, SkipsCellsWithOtherLefPins)
{
  ASSERT_NO_FATAL_FAILURE(setUpChipForTest({kBasic, kPinMismatch}));
  makeInputPorts({"d1", "e1", "clk"});
  tst::InstOptions op;
  op.iterms = {{"d1", "D"}, {"e1", "EN"}, {"clk", "CK"}, {"q1", "Q"}};
  makeInst(block_, getDb()->findMaster("DFFE_PP"), "ff1", op);
  makeOutputPort("q1");
  const sta::Sequential* seq = sequentialOf("DFFE_PP");
  ASSERT_NE(seq, nullptr);
  const LeafNets leaf
      = {{"D", block_->findNet("d1")}, {"EN", block_->findNet("e1")}};

  ret::SequentialUnmapper unmapper(getDb(), getSta(), getLogger());
  ASSERT_TRUE(unmapper.prepare());
  EXPECT_EQ(unmapper.getBaseSequentialMaster()->getName(),
            std::string("DFF_P"));
  EXPECT_EQ(unmapper.getTieMaster(true)->getName(), std::string("TIE1"));
  unmapper.run();
  expectSane();
  EXPECT_EQ(countMaster("INV"), 1);
  EXPECT_EQ(countMaster("AND2"), 2);
  EXPECT_EQ(countMaster("OR2"), 1);

  odb::dbInst* base = getInstByMasterName("DFF_P");
  ASSERT_NE(base, nullptr);
  expectSameFunction(base->findITerm("D")->getNet(),
                     seq,
                     leaf,
                     base->findITerm("Q")->getNet());
}

// DFF_P is already the base cell: rebuilt as-is, loads and port follow.
TEST_F(SequentialUnmapperCellTest, PlainFlop)
{
  ASSERT_NO_FATAL_FAILURE(setUpChipForTest({kBasic}));
  makeInputPorts({"d1", "clk"});
  tst::InstOptions op;
  op.iterms = {{"d1", "D"}, {"clk", "CK"}, {"q1", "Q"}};
  ASSERT_NE(makeInst(block_, getDb()->findMaster("DFF_P"), "ff1", op), nullptr);
  makeOutputPort("q1");
  const sta::Sequential* seq = sequentialOf("DFF_P");
  ASSERT_NE(seq, nullptr);
  const LeafNets leaf = {{"D", block_->findNet("d1")}};

  ret::SequentialUnmapper unmapper(getDb(), getSta(), getLogger());
  ASSERT_TRUE(unmapper.prepare());
  unmapper.run();
  expectSane();

  EXPECT_EQ(countMaster("DFF_P"), 1);
  EXPECT_EQ(countMaster("INV"), 0);
  EXPECT_EQ(block_->findInst("ff1"), nullptr)
      << "original instance must be destroyed";

  odb::dbInst* base = getInstByMasterName("DFF_P");
  ASSERT_NE(base, nullptr);
  EXPECT_EQ(base->findITerm("D")->getNet(), block_->findNet("d1"));
  EXPECT_EQ(base->findITerm("CK")->getNet(), block_->findNet("clk"));

  odb::dbBTerm* port = block_->findBTerm("q1");
  ASSERT_NE(port, nullptr);
  ASSERT_NE(port->getNet(), nullptr) << "port lost its net";
  EXPECT_EQ(driverOf(port->getNet()), base);
  EXPECT_EQ(port->getNet(), base->findITerm("Q")->getNet());
  EXPECT_EQ(block_->findNet("q1"), nullptr)
      << "old output net should be destroyed";

  odb::dbNet* state = base->findITerm("Q")->getNet();
  expectSameFunction(base->findITerm("D")->getNet(), seq, leaf, state);
}

// DFF_P_QN stores D and publishes the inverted state: one inverter on the
// output side (port <- INV <- base.Q), and the D cone is plain D.
TEST_F(SequentialUnmapperCellTest, InvertedOutput)
{
  ASSERT_NO_FATAL_FAILURE(setUpChipForTest({kBasic}));
  makeInputPorts({"d1", "clk"});
  tst::InstOptions op;
  op.iterms = {{"d1", "D"}, {"clk", "CK"}, {"qn1", "QN"}};
  makeInst(block_, getDb()->findMaster("DFF_P_QN"), "ff1", op);
  makeOutputPort("qn1");
  const sta::Sequential* seq = sequentialOf("DFF_P_QN");
  ASSERT_NE(seq, nullptr);
  const LeafNets leaf = {{"D", block_->findNet("d1")}};

  ret::SequentialUnmapper unmapper(getDb(), getSta(), getLogger());
  ASSERT_TRUE(unmapper.prepare());
  unmapper.run();
  expectSane();
  EXPECT_EQ(countMaster("INV"), 1);

  odb::dbInst* base = getInstByMasterName("DFF_P");
  ASSERT_NE(base, nullptr);
  odb::dbNet* state = base->findITerm("Q")->getNet();
  expectSameFunction(base->findITerm("D")->getNet(), seq, leaf, state);

  odb::dbBTerm* port = block_->findBTerm("qn1");
  ASSERT_NE(port, nullptr);
  odb::dbInst* inv = driverOf(port->getNet());
  ASSERT_TRUE(isMaster(inv, "INV"))
      << "port is not driven by an INV: "
      << (inv != nullptr ? inv->getName() : "none");
  EXPECT_EQ(inputNets(inv).at(0), state)
      << "output INV must read the base Q net";
}

// DFF_P_ND stores !D and reads the state directly: one inverter on the
// INPUT side, and the port reads base.Q straight.
TEST_F(SequentialUnmapperCellTest, InvertedData)
{
  ASSERT_NO_FATAL_FAILURE(setUpChipForTest({kBasic}));
  makeInputPorts({"d1", "clk"});
  tst::InstOptions op;
  op.iterms = {{"d1", "D"}, {"clk", "CK"}, {"qn1", "QN"}};
  makeInst(block_, getDb()->findMaster("DFF_P_ND"), "ff1", op);
  makeOutputPort("qn1");
  const sta::Sequential* seq = sequentialOf("DFF_P_ND");
  ASSERT_NE(seq, nullptr);
  const LeafNets leaf = {{"D", block_->findNet("d1")}};

  ret::SequentialUnmapper unmapper(getDb(), getSta(), getLogger());
  ASSERT_TRUE(unmapper.prepare());
  unmapper.run();
  expectSane();
  EXPECT_EQ(countMaster("INV"), 1);

  odb::dbInst* base = getInstByMasterName("DFF_P");
  ASSERT_NE(base, nullptr);
  odb::dbNet* state = base->findITerm("Q")->getNet();
  expectSameFunction(base->findITerm("D")->getNet(), seq, leaf, state);

  odb::dbBTerm* port = block_->findBTerm("qn1");
  ASSERT_NE(port, nullptr);
  EXPECT_EQ(port->getNet(), state)
      << "port must read base.Q directly, no output INV";
}

// DFFE_PP: next_state "(D * EN) + (IQ * !EN)" becomes INV + 2 AND2 + OR2;
// the truth table proves the hold path at every EN=0 row.
TEST_F(SequentialUnmapperCellTest, EnableFlop)
{
  ASSERT_NO_FATAL_FAILURE(setUpChipForTest({kBasic}));
  makeInputPorts({"d1", "e1", "clk"});
  tst::InstOptions op;
  op.iterms = {{"d1", "D"}, {"e1", "EN"}, {"clk", "CK"}, {"q1", "Q"}};
  makeInst(block_, getDb()->findMaster("DFFE_PP"), "ff1", op);
  makeOutputPort("q1");
  const sta::Sequential* seq = sequentialOf("DFFE_PP");
  ASSERT_NE(seq, nullptr);
  const LeafNets leaf
      = {{"D", block_->findNet("d1")}, {"EN", block_->findNet("e1")}};

  ret::SequentialUnmapper unmapper(getDb(), getSta(), getLogger());
  ASSERT_TRUE(unmapper.prepare());
  unmapper.run();
  expectSane();
  EXPECT_EQ(countMaster("OR2"), 1);
  EXPECT_EQ(countMaster("INV"), 1);
  EXPECT_EQ(countMaster("AND2"), 2);

  odb::dbInst* base = getInstByMasterName("DFF_P");
  ASSERT_NE(base, nullptr);
  odb::dbNet* state = base->findITerm("Q")->getNet();
  expectSameFunction(base->findITerm("D")->getNet(), seq, leaf, state);
}

// ---- negative-edge cells --------------------------------------------------

// A plain negedge flop stays on the negedge base cell.
TEST_F(SequentialUnmapperCellTest, NegedgePlainFlop)
{
  ASSERT_NO_FATAL_FAILURE(setUpChipForTest({kBasic, kNegedge}));
  makeInputPorts({"d1", "clk"});
  tst::InstOptions op;
  op.iterms = {{"d1", "D"}, {"clk", "CK"}, {"q1", "Q"}};
  makeInst(block_, getDb()->findMaster("DFF_N"), "ff1", op);
  makeOutputPort("q1");

  ret::SequentialUnmapper unmapper(getDb(), getSta(), getLogger());
  ASSERT_TRUE(unmapper.prepare());
  unmapper.run();
  expectSane();

  EXPECT_EQ(countMaster("DFF_N"), 1);
  EXPECT_EQ(countMaster("DFF_P"), 0);
}

TEST_F(SequentialUnmapperCellTest, NegedgeEnableFlop)
{
  ASSERT_NO_FATAL_FAILURE(setUpChipForTest({kBasic, kNegedge}));
  makeInputPorts({"d1", "e1", "clk"});
  tst::InstOptions op;
  op.iterms = {{"d1", "D"}, {"e1", "EN"}, {"clk", "CK"}, {"q1", "Q"}};
  makeInst(block_, getDb()->findMaster("DFFE_NP"), "ff1", op);
  makeOutputPort("q1");
  const sta::Sequential* seq = sequentialOf("DFFE_NP");
  ASSERT_NE(seq, nullptr);
  const LeafNets leaf
      = {{"D", block_->findNet("d1")}, {"EN", block_->findNet("e1")}};

  ret::SequentialUnmapper unmapper(getDb(), getSta(), getLogger());
  ASSERT_TRUE(unmapper.prepare());
  unmapper.run();
  expectSane();

  EXPECT_EQ(countMaster("DFF_N"), 1);
  EXPECT_EQ(countMaster("AND2"), 2);
  EXPECT_EQ(countMaster("OR2"), 1);
  EXPECT_EQ(countMaster("INV"), 1);

  odb::dbInst* base = getInstByMasterName("DFF_N");
  ASSERT_NE(base, nullptr);
  odb::dbNet* state = base->findITerm("Q")->getNet();
  expectSameFunction(base->findITerm("D")->getNet(), seq, leaf, state);
}

// Negedge flop with inverted data and inverted output: two inverters.
TEST_F(SequentialUnmapperCellTest, NegedgeInvertedDataAndOutput)
{
  ASSERT_NO_FATAL_FAILURE(setUpChipForTest({kBasic, kNegedge}));
  makeInputPorts({"d1", "clk"});
  tst::InstOptions op;
  op.iterms = {{"d1", "D"}, {"q1", "QN"}, {"clk", "CK"}};
  makeInst(block_, getDb()->findMaster("DFF_N_ND_QN"), "ff1", op);
  makeOutputPort("q1");
  const sta::Sequential* seq = sequentialOf("DFF_N_ND_QN");
  ASSERT_NE(seq, nullptr);
  const LeafNets leaf = {{"D", block_->findNet("d1")}};

  ret::SequentialUnmapper unmapper(getDb(), getSta(), getLogger());
  ASSERT_TRUE(unmapper.prepare());
  unmapper.run();
  expectSane();

  ASSERT_EQ(countMaster("DFF_N"), 1);
  ASSERT_EQ(countMaster("INV"), 2);

  odb::dbInst* base = getInstByMasterName("DFF_N");
  ASSERT_NE(base, nullptr);
  odb::dbNet* state = base->findITerm("Q")->getNet();
  expectSameFunction(base->findITerm("D")->getNet(), seq, leaf, state);
}

// Negedge enable flop with inverted data: next_state
// "!((D + !EN) * (!IQ + EN))".
TEST_F(SequentialUnmapperCellTest, NegedgeEnableInvertedData)
{
  ASSERT_NO_FATAL_FAILURE(setUpChipForTest({kBasic, kNegedge}));
  makeInputPorts({"d1", "e1", "clk"});
  tst::InstOptions op;
  op.iterms = {{"d1", "D"}, {"e1", "EN"}, {"clk", "CK"}, {"q1", "Q"}};
  makeInst(block_, getDb()->findMaster("DFFE_NP_ND"), "ff1", op);
  makeOutputPort("q1");
  const sta::Sequential* seq = sequentialOf("DFFE_NP_ND");
  ASSERT_NE(seq, nullptr);
  const LeafNets leaf
      = {{"D", block_->findNet("d1")}, {"EN", block_->findNet("e1")}};

  ret::SequentialUnmapper unmapper(getDb(), getSta(), getLogger());
  ASSERT_TRUE(unmapper.prepare());
  unmapper.run();
  expectSane();

  ASSERT_EQ(countMaster("DFF_N"), 1);
  ASSERT_EQ(countMaster("INV"), 3);
  ASSERT_EQ(countMaster("AND2"), 1);
  ASSERT_EQ(countMaster("OR2"), 2);

  odb::dbInst* base = getInstByMasterName("DFF_N");
  ASSERT_NE(base, nullptr);
  odb::dbNet* state = base->findITerm("Q")->getNet();
  expectSameFunction(base->findITerm("D")->getNet(), seq, leaf, state);
}

// XOR in next_state: "IQ ^ (EN * (D ^ IQ))" expands to AND/OR/INV only.
TEST_F(SequentialUnmapperCellTest, XorNextState)
{
  ASSERT_NO_FATAL_FAILURE(setUpChipForTest({kBasic, kNegedge}));
  makeInputPorts({"d1", "e1", "clk"});
  tst::InstOptions op;
  op.iterms = {{"d1", "D"}, {"e1", "EN"}, {"clk", "CK"}, {"q1", "QN"}};
  makeInst(block_, getDb()->findMaster("DFFE_NP_QN"), "ff1", op);
  makeOutputPort("q1");
  const sta::Sequential* seq = sequentialOf("DFFE_NP_QN");
  ASSERT_NE(seq, nullptr);
  const LeafNets leaf
      = {{"D", block_->findNet("d1")}, {"EN", block_->findNet("e1")}};

  ret::SequentialUnmapper unmapper(getDb(), getSta(), getLogger());
  ASSERT_TRUE(unmapper.prepare());
  unmapper.run();
  expectSane();

  ASSERT_EQ(countMaster("DFF_N"), 1);
  ASSERT_EQ(countMaster("INV"), 5);
  ASSERT_EQ(countMaster("AND2"), 5);
  ASSERT_EQ(countMaster("OR2"), 2);

  odb::dbInst* base = getInstByMasterName("DFF_N");
  ASSERT_NE(base, nullptr);
  odb::dbNet* state = base->findITerm("Q")->getNet();
  expectSameFunction(base->findITerm("D")->getNet(), seq, leaf, state);
}

// ---- cells that are left alone -------------------------------------------

TEST_F(SequentialUnmapperCellTest, SkipsAsyncReset)
{
  ASSERT_NO_FATAL_FAILURE(setUpChipForTest({kBasic}));
  makeInputPorts({"d1", "r1", "clk"});
  tst::InstOptions op;
  op.iterms = {{"d1", "D"}, {"r1", "RN"}, {"clk", "CK"}, {"q1", "Q"}};
  makeInst(block_, getDb()->findMaster("DFF_PN0"), "ff1", op);
  makeOutputPort("q1");

  ret::SequentialUnmapper unmapper(getDb(), getSta(), getLogger());
  ASSERT_TRUE(unmapper.prepare());
  unmapper.run();
  expectSane();
  EXPECT_EQ(countMaster("DFF_PN0"), 1);
  EXPECT_EQ(countMaster("DFF_P"), 0);
  EXPECT_NE(block_->findInst("ff1"), nullptr);
}

// ---- asynchronous controls tied inactive -----------------------------------

// A reset held high by a tie cell never fires: the register is unmapped
// like a plain one, and the tie cell that only it read goes with it.
TEST_F(SequentialUnmapperCellTest, UnmapsTiedAsyncReset)
{
  ASSERT_NO_FATAL_FAILURE(setUpChipForTest({kBasic}));
  makeInputPorts({"d1", "clk"});
  tst::InstOptions tie_op;
  tie_op.iterms = {{"hi", "Y"}};
  makeInst(block_, getDb()->findMaster("TIE1"), "t1", tie_op);
  tst::InstOptions op;
  op.iterms = {{"d1", "D"}, {"hi", "RN"}, {"clk", "CK"}, {"q1", "Q"}};
  makeInst(block_, getDb()->findMaster("DFF_PN0"), "ff1", op);
  makeOutputPort("q1");

  ret::SequentialUnmapper unmapper(getDb(), getSta(), getLogger());
  ASSERT_TRUE(unmapper.prepare());
  unmapper.run();
  expectSane();
  EXPECT_EQ(countMaster("DFF_PN0"), 0);
  EXPECT_EQ(countMaster("DFF_P"), 1);
  EXPECT_EQ(countMaster("TIE1"), 0) << "the idle tie cell must be removed";
  EXPECT_EQ(block_->findNet("hi"), nullptr);
}

// The base cell is the register that costs the least area once its
// asynchronous controls are tied: DFF_PN0 (5.0 + one tie), not the smaller
// DFFSR_PNN (4.5 + two ties), not the synchronous-reset SDFF_PP0 or the
// enable DFFE_PP, not DFF_PN0_HC with its heavier D pin, and not DFF_PD0 or
// DFF_PC0 (3.0), whose reset reads the data or the clock pin too.
TEST_F(SequentialUnmapperCellTest, PicksTiedBase)
{
  ASSERT_NO_FATAL_FAILURE(setUpChipForTest({kAsyncOnly}));
  ret::SequentialUnmapper unmapper(getDb(), getSta(), getLogger());
  ASSERT_TRUE(unmapper.prepare());
  ASSERT_NE(unmapper.getBaseSequentialMaster(), nullptr);
  EXPECT_EQ(unmapper.getBaseSequentialMaster()->getName(),
            std::string("DFF_PN0"));
  ASSERT_NE(unmapper.getNegedgeSequentialMaster(), nullptr);
  EXPECT_EQ(unmapper.getNegedgeSequentialMaster()->getName(),
            std::string("DFF_NP0"));
}

// An enable flop unmaps to DFF_PN0 with its active-low reset tied high.
TEST_F(SequentialUnmapperCellTest, TiedBaseGetsTieHigh)
{
  ASSERT_NO_FATAL_FAILURE(setUpChipForTest({kAsyncOnly}));
  makeInputPorts({"d1", "e1", "clk"});
  tst::InstOptions op;
  op.iterms = {{"d1", "D"}, {"e1", "EN"}, {"clk", "CK"}, {"q1", "Q"}};
  makeInst(block_, getDb()->findMaster("DFFE_PP"), "ff1", op);
  makeOutputPort("q1");
  const sta::Sequential* seq = sequentialOf("DFFE_PP");
  ASSERT_NE(seq, nullptr);
  const LeafNets leaf
      = {{"D", block_->findNet("d1")}, {"EN", block_->findNet("e1")}};

  ret::SequentialUnmapper unmapper(getDb(), getSta(), getLogger());
  ASSERT_TRUE(unmapper.prepare());
  unmapper.run();
  expectSane();
  EXPECT_EQ(countMaster("DFFE_PP"), 0);
  EXPECT_EQ(countMaster("TIE1"), 1);
  EXPECT_EQ(countMaster("TIE0"), 0);

  odb::dbInst* base = getInstByMasterName("DFF_PN0");
  ASSERT_NE(base, nullptr);
  EXPECT_TRUE(isMaster(driverOf(base->findITerm("RN")->getNet()), "TIE1"))
      << "the reset of the base cell must be tied high";
  odb::dbNet* state = base->findITerm("Q")->getNet();
  expectSameFunction(base->findITerm("D")->getNet(), seq, leaf, state);
}

// The negedge base cell DFF_NP0 has an active-high reset: tied low.  The
// register unmapped here already is one, so its old tie cell is replaced.
TEST_F(SequentialUnmapperCellTest, TiedBaseGetsTieLow)
{
  ASSERT_NO_FATAL_FAILURE(setUpChipForTest({kAsyncOnly}));
  makeInputPorts({"d1", "clk"});
  tst::InstOptions tie_op;
  tie_op.iterms = {{"lo", "Y"}};
  makeInst(block_, getDb()->findMaster("TIE0"), "t0", tie_op);
  tst::InstOptions op;
  op.iterms = {{"d1", "D"}, {"lo", "R"}, {"clk", "CK"}, {"q1", "Q"}};
  makeInst(block_, getDb()->findMaster("DFF_NP0"), "ff1", op);
  makeOutputPort("q1");

  ret::SequentialUnmapper unmapper(getDb(), getSta(), getLogger());
  ASSERT_TRUE(unmapper.prepare());
  unmapper.run();
  expectSane();
  EXPECT_EQ(block_->findInst("ff1"), nullptr);
  EXPECT_EQ(block_->findInst("t0"), nullptr);
  EXPECT_EQ(countMaster("TIE0"), 1);
  EXPECT_EQ(countMaster("TIE1"), 0);

  odb::dbInst* base = getInstByMasterName("DFF_NP0");
  ASSERT_NE(base, nullptr);
  EXPECT_TRUE(isMaster(driverOf(base->findITerm("R")->getNet()), "TIE0"))
      << "the reset of the base cell must be tied low";
  EXPECT_EQ(base->findITerm("D")->getNet(), block_->findNet("d1"));
}

// ---- tie cells -------------------------------------------------------------

// DFF_P_TIE0: next_state "(D + 0)" needs a tie-low; the sized library has
// TIE0x2 listed before the smaller TIE0x1, and the smaller one must win.
TEST_F(SequentialUnmapperCellTest, SmallestTieCell)
{
  ASSERT_NO_FATAL_FAILURE(setUpChipForTest({kTieSized}));
  makeInputPorts({"d1", "clk"});
  tst::InstOptions op;
  op.iterms = {{"d1", "D"}, {"clk", "CK"}, {"q1", "Q"}};
  makeInst(block_, getDb()->findMaster("DFF_P_TIE0"), "ff1", op);
  makeOutputPort("q1");
  const sta::Sequential* seq = sequentialOf("DFF_P_TIE0");
  ASSERT_NE(seq, nullptr);
  const LeafNets leaf = {{"D", block_->findNet("d1")}};

  ret::SequentialUnmapper unmapper(getDb(), getSta(), getLogger());
  ASSERT_TRUE(unmapper.prepare());
  unmapper.run();
  expectSane();

  EXPECT_EQ(countMaster("DFF_P"), 1);
  EXPECT_EQ(countMaster("TIE0x1"), 1);
  EXPECT_EQ(countMaster("TIE0x2"), 0);
  EXPECT_EQ(countMaster("OR2"), 1);

  odb::dbInst* base = getInstByMasterName("DFF_P");
  ASSERT_NE(base, nullptr);
  odb::dbNet* state = base->findITerm("Q")->getNet();
  expectSameFunction(base->findITerm("D")->getNet(), seq, leaf, state);
}

// On a two-output tie cell (MULTIE: HI and LO pins) the zero leaf must be
// wired to the LO pin.
TEST_F(SequentialUnmapperCellTest, TwoOutputTieLow)
{
  ASSERT_NO_FATAL_FAILURE(setUpChipForTest({kTieMulti}));
  makeInputPorts({"d1", "clk"});
  tst::InstOptions op;
  op.iterms = {{"d1", "D"}, {"clk", "CK"}, {"q1", "Q"}};
  makeInst(block_, getDb()->findMaster("DFF_P_TIE0"), "ff1", op);
  makeOutputPort("q1");
  const sta::Sequential* seq = sequentialOf("DFF_P_TIE0");
  ASSERT_NE(seq, nullptr);
  const LeafNets leaf = {{"D", block_->findNet("d1")}};

  ret::SequentialUnmapper unmapper(getDb(), getSta(), getLogger());
  ASSERT_TRUE(unmapper.prepare());
  unmapper.run();
  expectSane();

  EXPECT_EQ(countMaster("MULTIE"), 1);  // MULTIE.area < TIE0.area
  EXPECT_EQ(countMaster("TIE0"), 0);

  odb::dbInst* tie = getInstByMasterName("MULTIE");
  ASSERT_NE(tie, nullptr);
  odb::dbITerm* lo = tie->findITerm("LO");
  odb::dbITerm* hi = tie->findITerm("HI");
  ASSERT_NE(lo, nullptr);
  ASSERT_NE(hi, nullptr);
  EXPECT_NE(lo->getNet(), nullptr) << "zero leaf must come from the LO pin";
  EXPECT_EQ(hi->getNet(), nullptr) << "HI pin must stay unconnected";

  odb::dbInst* base = getInstByMasterName("DFF_P");
  ASSERT_NE(base, nullptr);
  odb::dbNet* state = base->findITerm("Q")->getNet();
  expectSameFunction(base->findITerm("D")->getNet(), seq, leaf, state);
}

// The one leaf must be wired to the HI pin.
TEST_F(SequentialUnmapperCellTest, TwoOutputTieHigh)
{
  ASSERT_NO_FATAL_FAILURE(setUpChipForTest({kTieMulti}));
  makeInputPorts({"d1", "clk"});
  tst::InstOptions op;
  op.iterms = {{"d1", "D"}, {"clk", "CK"}, {"q1", "Q"}};
  makeInst(block_, getDb()->findMaster("DFF_P_TIE1"), "ff1", op);
  makeOutputPort("q1");
  const sta::Sequential* seq = sequentialOf("DFF_P_TIE1");
  ASSERT_NE(seq, nullptr);
  const LeafNets leaf = {{"D", block_->findNet("d1")}};

  ret::SequentialUnmapper unmapper(getDb(), getSta(), getLogger());
  ASSERT_TRUE(unmapper.prepare());
  unmapper.run();
  expectSane();

  EXPECT_EQ(countMaster("MULTIE"), 1);
  EXPECT_EQ(countMaster("TIE1"), 0);

  odb::dbInst* tie = getInstByMasterName("MULTIE");
  ASSERT_NE(tie, nullptr);
  odb::dbITerm* lo = tie->findITerm("LO");
  odb::dbITerm* hi = tie->findITerm("HI");
  ASSERT_NE(lo, nullptr);
  ASSERT_NE(hi, nullptr);
  EXPECT_EQ(lo->getNet(), nullptr) << "LO pin must stay unconnected";
  EXPECT_NE(hi->getNet(), nullptr) << "one leaf must come from the HI pin";

  odb::dbInst* base = getInstByMasterName("DFF_P");
  ASSERT_NE(base, nullptr);
  odb::dbNet* state = base->findITerm("Q")->getNet();
  expectSameFunction(base->findITerm("D")->getNet(), seq, leaf, state);
}

}  // namespace
