// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <cstddef>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "helpers/TestLiberty.h"
#include "liberty/LibertyBuilder.hh"
#include "sta/ConcreteLibrary.hh"
#include "sta/FuncExpr.hh"
#include "sta/Liberty.hh"
#include "sta/LibertyClass.hh"
#include "sta/NetworkClass.hh"
#include "sta/PortDirection.hh"
#include "sta/Transition.hh"
#include "syn/lir/Ids.h"
#include "syn/lir/LibCellTable.h"

namespace syn::test {
namespace {

using ::syn::lir::LibCellId;
using ::syn::lir::LibCellInfo;
using ::syn::lir::LibCellKind;
using ::syn::lir::libCellKindName;
using ::syn::lir::LibCellTable;
using ::syn::lir::TableMode;

class LibCellTableTest : public ::testing::Test
{
 protected:
  TestLiberty lib_{"test_lib", "test.lib"};
};

TEST_F(LibCellTableTest, InternAndLookupCombinationalAndSequentialCells)
{
  sta::LibertyCell* inv = lib_.cell("INV_X1");
  sta::LibertyPort* inv_a = lib_.port(inv, "A", sta::PortDirection::input());
  sta::LibertyPort* inv_y = lib_.port(inv, "Y", sta::PortDirection::output());
  inv_y->setFunction(sta::FuncExpr::makeNot(sta::FuncExpr::makePort(inv_a)));
  sta::LibertyCell* dff = lib_.dff("DFF_X1");

  LibCellTable table;
  EXPECT_EQ(table.size(), 0u);
  EXPECT_TRUE(table.isReadOnly());
  EXPECT_FALSE(table.find(nullptr).isValid());
  EXPECT_FALSE(table.find(inv).isValid());

  table.setMode(TableMode::kSingleWrite);
  EXPECT_FALSE(table.isReadOnly());

  const LibCellId inv_id = table.intern(inv);
  EXPECT_EQ(inv_id, LibCellId(0));
  EXPECT_EQ(table.size(), 1u);
  EXPECT_EQ(table.intern(inv), inv_id);
  EXPECT_EQ(table.size(), 1u);
  EXPECT_EQ(table.find(inv), inv_id);
  EXPECT_EQ(table.cell(inv_id), inv);

  const LibCellId dff_id = table.intern(dff);
  EXPECT_EQ(dff_id, LibCellId(1));
  EXPECT_EQ(table.size(), 2u);
  EXPECT_EQ(table.find(dff), dff_id);
  EXPECT_EQ(table.cell(dff_id), dff);

  const LibCellInfo& inv_info = table.info(inv_id);
  EXPECT_EQ(inv_info.cell(), inv);
  EXPECT_EQ(inv_info.name(), "INV_X1");
  EXPECT_FALSE(inv_info.isSequential());
  EXPECT_EQ(inv_info.kind(), LibCellKind::kCombinational);
  EXPECT_EQ(inv_info.numInputs(), 1);
  EXPECT_EQ(inv_info.numOutputs(), 1);
  EXPECT_EQ(inv_info.input(0), inv_a);
  EXPECT_EQ(inv_info.output(0), inv_y);
  EXPECT_EQ(inv_info.findInput(inv_a), 0);
  EXPECT_EQ(inv_info.findInput("A"), 0);
  EXPECT_EQ(inv_info.findInput("Z"), -1);
  EXPECT_EQ(inv_info.findOutput(inv_y), 0);
  EXPECT_EQ(inv_info.findOutput("Y"), 0);
  EXPECT_EQ(inv_info.findOutput("A"), -1);

  const LibCellInfo& dff_info = table.info(dff_id);
  EXPECT_EQ(dff_info.cell(), dff);
  EXPECT_EQ(dff_info.name(), "DFF_X1");
  EXPECT_TRUE(dff_info.isSequential());
  EXPECT_EQ(dff_info.kind(), LibCellKind::kRegister);
  EXPECT_EQ(dff_info.numInputs(), 2);
  EXPECT_EQ(dff_info.numOutputs(), 2);
  EXPECT_EQ(dff_info.findInput("D"), 0);
  EXPECT_EQ(dff_info.findInput("CLK"), 1);
  EXPECT_EQ(dff_info.findOutput("Q"), 0);
  EXPECT_EQ(dff_info.findOutput("QN"), 1);
  EXPECT_GT(table.memoryBytes(), 0u);
}

TEST_F(LibCellTableTest, ClassifiesCellKinds)
{
  const sta::PortDirection* in = sta::PortDirection::input();
  const sta::PortDirection* out = sta::PortDirection::output();
  const sta::PortDirection* internal = sta::PortDirection::internal();

  // ff (IQ, IQN) with Q = IQ and QN = IQN, as the Liberty reader builds it.
  sta::LibertyCell* dff = lib_.cell("DFF");
  lib_.port(dff, "D", in);
  lib_.port(dff, "CK", in);
  sta::LibertyPort* dff_q = lib_.port(dff, "Q", out);
  sta::LibertyPort* dff_qn = lib_.port(dff, "QN", out);
  sta::LibertyPort* dff_iq = lib_.port(dff, "IQ", internal);
  sta::LibertyPort* dff_iqn = lib_.port(dff, "IQN", internal);
  TestLiberty::flop(dff, true, dff_iq, dff_iqn);
  dff_q->setFunction(sta::FuncExpr::makePort(dff_iq));
  dff_qn->setFunction(sta::FuncExpr::makePort(dff_iqn));

  // A latch is a register too: its output reads only the state.
  sta::LibertyCell* latch = lib_.cell("LATCH");
  lib_.port(latch, "D", in);
  lib_.port(latch, "G", in);
  sta::LibertyPort* latch_q = lib_.port(latch, "Q", out);
  sta::LibertyPort* latch_iq = lib_.port(latch, "IQ", internal);
  TestLiberty::flop(latch, false, latch_iq, nullptr);
  latch_q->setFunction(sta::FuncExpr::makePort(latch_iq));

  // Statetable cell whose output reads only state nodes (e.g. a
  // synchronizer): still a black box, since no ff or latch stands for it.
  sta::LibertyCell* sync = lib_.cell("SYNC2");
  sta::LibertyPort* sync_d = lib_.port(sync, "D", in);
  sta::LibertyPort* sync_ck = lib_.port(sync, "CK", in);
  sta::LibertyPort* sync_q = lib_.port(sync, "Q", out);
  sta::LibertyPort* sync_s1 = lib_.port(sync, "S1", internal);
  sta::LibertyPort* sync_s2 = lib_.port(sync, "S2", internal);
  TestLiberty::statetable(sync, {sync_ck, sync_d}, {sync_s1, sync_s2});
  sync_q->setFunction(sta::FuncExpr::makePort(sync_s2));

  // Statetable cell whose output has no function (state_function is not
  // parsed by OpenSTA): unknown, so a black box.
  sta::LibertyCell* opaque = lib_.cell("ST");
  sta::LibertyPort* opaque_a = lib_.port(opaque, "A", in);
  lib_.port(opaque, "Y", out);
  sta::LibertyPort* opaque_s = lib_.port(opaque, "S", internal);
  TestLiberty::statetable(opaque, {opaque_a}, {opaque_s});

  // Gated output Q = IQ & OE: depends on an input, so a black box.
  sta::LibertyCell* gated = lib_.cell("GDFF");
  lib_.port(gated, "D", in);
  lib_.port(gated, "CK", in);
  sta::LibertyPort* gated_oe = lib_.port(gated, "OE", in);
  sta::LibertyPort* gated_q = lib_.port(gated, "Q", out);
  sta::LibertyPort* gated_iq = lib_.port(gated, "IQ", internal);
  TestLiberty::flop(gated, true, gated_iq, nullptr);
  gated_q->setFunction(sta::FuncExpr::makeAnd(
      sta::FuncExpr::makePort(gated_iq), sta::FuncExpr::makePort(gated_oe)));

  // Tristate output: depends on its enable, so a black box.
  sta::LibertyCell* tri = lib_.cell("TDFF");
  lib_.port(tri, "D", in);
  lib_.port(tri, "CK", in);
  sta::LibertyPort* tri_oe = lib_.port(tri, "OE", in);
  sta::LibertyPort* tri_q = lib_.port(tri, "Q", sta::PortDirection::tristate());
  sta::LibertyPort* tri_iq = lib_.port(tri, "IQ", internal);
  TestLiberty::flop(tri, true, tri_iq, nullptr);
  tri_q->setFunction(sta::FuncExpr::makePort(tri_iq));
  tri_q->setTristateEnable(sta::FuncExpr::makePort(tri_oe));

  // Clock gate: a statetable cell, but recognized first.
  sta::LibertyCell* icg = lib_.cell("ICG");
  sta::LibertyPort* icg_ck = lib_.port(icg, "CK", in);
  sta::LibertyPort* icg_en = lib_.port(icg, "EN", in);
  lib_.port(icg, "GCLK", out);
  sta::LibertyPort* icg_iq = lib_.port(icg, "IQ", internal);
  TestLiberty::statetable(icg, {icg_ck, icg_en}, {icg_iq});
  icg->setClockGateType(sta::ClockGateType::latch_posedge);

  // A sequential cell without outputs has no owner: a black box.
  sta::LibertyCell* sink = lib_.cell("SINK");
  lib_.port(sink, "D", in);
  lib_.port(sink, "CK", in);
  sta::LibertyPort* sink_iq = lib_.port(sink, "IQ", internal);
  TestLiberty::flop(sink, true, sink_iq, nullptr);

  EXPECT_EQ(LibCellInfo(dff).kind(), LibCellKind::kRegister);
  EXPECT_EQ(LibCellInfo(latch).kind(), LibCellKind::kRegister);
  EXPECT_EQ(LibCellInfo(sync).kind(), LibCellKind::kBlackBox);
  EXPECT_TRUE(LibCellInfo(sync).isSequential());
  EXPECT_EQ(LibCellInfo(opaque).kind(), LibCellKind::kBlackBox);
  EXPECT_EQ(LibCellInfo(gated).kind(), LibCellKind::kBlackBox);
  EXPECT_EQ(LibCellInfo(tri).kind(), LibCellKind::kBlackBox);
  EXPECT_EQ(LibCellInfo(icg).kind(), LibCellKind::kClockGate);
  EXPECT_TRUE(LibCellInfo(icg).isSequential());
  EXPECT_EQ(LibCellInfo(sink).kind(), LibCellKind::kBlackBox);
  EXPECT_EQ(libCellKindName(LibCellKind::kClockGate), "clock_gate");
  EXPECT_EQ(libCellKindName(LibCellKind::kBlackBox), "black_box");
}

TEST_F(LibCellTableTest, ZeroOutputCellsAreBlackBoxes)
{
  const sta::PortDirection* in = sta::PortDirection::input();

  // Sink cell (e.g. antenna diode): 1 input, 0 outputs.
  sta::LibertyCell* sink = lib_.cell("ANTENNA_X1");
  lib_.port(sink, "A", in);
  const LibCellInfo sink_info(sink);
  EXPECT_EQ(sink_info.numInputs(), 1);
  EXPECT_EQ(sink_info.numOutputs(), 0);
  EXPECT_FALSE(sink_info.isSequential());
  EXPECT_EQ(sink_info.kind(), LibCellKind::kBlackBox);
  EXPECT_TRUE(sink_info.checkSupported().ok()) << sink_info.checkSupported();

  // Physical-only cell (e.g. filler or decap): 0 inputs, 0 outputs.
  sta::LibertyCell* filler = lib_.cell("FILLCELL_X1");
  const LibCellInfo filler_info(filler);
  EXPECT_EQ(filler_info.numInputs(), 0);
  EXPECT_EQ(filler_info.numOutputs(), 0);
  EXPECT_FALSE(filler_info.isSequential());
  EXPECT_EQ(filler_info.kind(), LibCellKind::kBlackBox);
  EXPECT_TRUE(filler_info.checkSupported().ok())
      << filler_info.checkSupported();

  // Clock-gate attribute on a cell with no outputs: still a black box, since
  // a kCell requires output 0.
  sta::LibertyCell* icg_sink = lib_.cell("ICG_SINK");
  lib_.port(icg_sink, "CK", in);
  lib_.port(icg_sink, "EN", in);
  icg_sink->setClockGateType(sta::ClockGateType::latch_posedge);
  const LibCellInfo icg_sink_info(icg_sink);
  EXPECT_EQ(icg_sink_info.numInputs(), 2);
  EXPECT_EQ(icg_sink_info.numOutputs(), 0);
  EXPECT_EQ(icg_sink_info.kind(), LibCellKind::kBlackBox);
  EXPECT_TRUE(icg_sink_info.checkSupported().ok())
      << icg_sink_info.checkSupported();
}

// Pads, macros, memories and interface-timing models are black boxes even
// when every output has a function (an input pad whose output is the pad).
TEST_F(LibCellTableTest, OpaqueBlocksAreBlackBoxes)
{
  sta::LibertyCell* plain = lib_.combinational("PLAIN", {"A"}, {"Y"});
  EXPECT_EQ(LibCellInfo(plain).kind(), LibCellKind::kCombinational);

  sta::LibertyCell* pad = lib_.combinational("PAD_IN", {"PAD"}, {"P2C"});
  pad->setIsPad(true);
  EXPECT_EQ(LibCellInfo(pad).kind(), LibCellKind::kBlackBox);

  sta::LibertyCell* macro = lib_.combinational("MACRO", {"A"}, {"Y"});
  macro->setIsMacro(true);
  EXPECT_EQ(LibCellInfo(macro).kind(), LibCellKind::kBlackBox);

  sta::LibertyCell* memory = lib_.combinational("MEMORY", {"A"}, {"Y"});
  memory->setIsMemory(true);
  EXPECT_EQ(LibCellInfo(memory).kind(), LibCellKind::kBlackBox);

  sta::LibertyCell* etm = lib_.combinational("ETM", {"A"}, {"Y"});
  etm->setInterfaceTiming(true);
  EXPECT_EQ(LibCellInfo(etm).kind(), LibCellKind::kBlackBox);

  // Clock-to-output arcs without an ff or latch: unmodeled state.
  sta::LibertyCell* inferred = lib_.combinational("INFERRED", {"CK"}, {"Q"});
  inferred->setHasInferedRegTimingArcs(true);
  EXPECT_EQ(LibCellInfo(inferred).kind(), LibCellKind::kBlackBox);
}

// A combinational cell is a kCell only if every output reads inputs only.
TEST_F(LibCellTableTest, CombinationalOutputsReadOnlyInputs)
{
  // Y2 = !Y1 reads another output.
  sta::LibertyCell* chained
      = lib_.combinational("CHAINED", {"A", "B"}, {"Y1", "Y2"});
  sta::LibertyPort* y1 = chained->findLibertyPort("Y1");
  TestLiberty::setFunction(chained->findLibertyPort("Y2"),
                           sta::FuncExpr::makeNot(sta::FuncExpr::makePort(y1)));
  EXPECT_EQ(LibCellInfo(chained).kind(), LibCellKind::kBlackBox);

  // Y = IQ reads an internal pin.
  sta::LibertyCell* reads_internal
      = lib_.combinational("READS_INTERNAL", {"A"}, {"Y"});
  sta::LibertyPort* internal_pin
      = lib_.port(reads_internal, "IQ", sta::PortDirection::internal());
  TestLiberty::setFunction(reads_internal->findLibertyPort("Y"),
                           sta::FuncExpr::makePort(internal_pin));
  EXPECT_EQ(LibCellInfo(reads_internal).kind(), LibCellKind::kBlackBox);

  // A constant reads nothing: a tie cell stays combinational.
  sta::LibertyCell* tie = lib_.combinational("TIEH", {}, {"Y"});
  TestLiberty::setFunction(tie->findLibertyPort("Y"), sta::FuncExpr::makeOne());
  EXPECT_EQ(LibCellInfo(tie).kind(), LibCellKind::kCombinational);
}

TEST_F(LibCellTableTest, FiltersPowerGroundAndInternalPorts)
{
  sta::LibertyCell* cell = lib_.cell("BUF_PG");
  sta::LibertyPort* vdd = lib_.port(cell, "VDD", sta::PortDirection::power());
  vdd->setPwrGndType(sta::PwrGndType::primary_power);
  sta::LibertyPort* vss = lib_.port(cell, "VSS", sta::PortDirection::ground());
  vss->setPwrGndType(sta::PwrGndType::primary_ground);
  sta::LibertyPort* iq = lib_.port(cell, "IQ", sta::PortDirection::internal());
  sta::LibertyPort* a = lib_.port(cell, "A", sta::PortDirection::input());
  sta::LibertyPort* y = lib_.port(cell, "Y", sta::PortDirection::output());

  const LibCellInfo info(cell);
  ASSERT_EQ(info.numInputs(), 1);
  EXPECT_EQ(info.input(0), a);
  ASSERT_EQ(info.numOutputs(), 1);
  EXPECT_EQ(info.output(0), y);
  EXPECT_EQ(info.findInput(vdd), -1);
  EXPECT_EQ(info.findInput(vss), -1);
  EXPECT_EQ(info.findOutput(iq), -1);
}

TEST_F(LibCellTableTest, ExpandsBusPortsLsbFirst)
{
  sta::LibertyCell* cell = lib_.cell("BUS_CELL");
  sta::BusDcl* bus_desc = lib_.library().makeBusDcl("bus4_desc", 3, 0);
  sta::BusDcl* bus_asc = lib_.library().makeBusDcl("bus2_asc", 0, 1);

  // Descending bus D[3:0] should be expanded LSB-first: D[0], D[1], D[2], D[3].
  sta::LibertyPort* d_bus
      = lib_.builder().makeBusPort(cell, "D", 3, 0, bus_desc);
  d_bus->setDirection(sta::PortDirection::input());

  // Ascending bus Q[0:1] should also be expanded LSB-first: Q[0], Q[1].
  sta::LibertyPort* q_bus
      = lib_.builder().makeBusPort(cell, "Q", 0, 1, bus_asc);
  q_bus->setDirection(sta::PortDirection::output());

  const LibCellInfo info(cell);
  ASSERT_EQ(info.numInputs(), 4);
  EXPECT_EQ(std::string_view(info.input(0)->name()), "D[0]");
  EXPECT_EQ(std::string_view(info.input(1)->name()), "D[1]");
  EXPECT_EQ(std::string_view(info.input(2)->name()), "D[2]");
  EXPECT_EQ(std::string_view(info.input(3)->name()), "D[3]");

  ASSERT_EQ(info.numOutputs(), 2);
  EXPECT_EQ(std::string_view(info.output(0)->name()), "Q[0]");
  EXPECT_EQ(std::string_view(info.output(1)->name()), "Q[1]");
}

// checkSupported() is kUnimplemented, naming the cell and `substr`.
void expectUnimplemented(const LibCellInfo& info, std::string_view substr)
{
  const absl::Status status = info.checkSupported();
  EXPECT_EQ(status.code(), absl::StatusCode::kUnimplemented) << status;
  EXPECT_THAT(status.message(), ::testing::HasSubstr(std::string(info.name())));
  EXPECT_THAT(status.message(), ::testing::HasSubstr(std::string(substr)));
}

// Combinational tristate buffer TBUF(A, EN) -> Z.
sta::LibertyCell* makeTbuf(TestLiberty& lib)
{
  const sta::PortDirection* in = sta::PortDirection::input();
  sta::LibertyCell* tbuf = lib.cell("TBUF");
  sta::LibertyPort* tbuf_a = lib.port(tbuf, "A", in);
  sta::LibertyPort* tbuf_en = lib.port(tbuf, "EN", in);
  sta::LibertyPort* tbuf_z
      = lib.port(tbuf, "Z", sta::PortDirection::tristate());
  tbuf_z->setFunction(sta::FuncExpr::makePort(tbuf_a));
  tbuf_z->setTristateEnable(sta::FuncExpr::makePort(tbuf_en));
  return tbuf;
}

TEST_F(LibCellTableTest, SupportsOrdinaryCombinationalCells)
{
  const sta::PortDirection* in = sta::PortDirection::input();
  sta::LibertyCell* nand = lib_.cell("NAND2");
  lib_.port(nand, "A", in);
  lib_.port(nand, "B", in);
  lib_.port(nand, "Y", sta::PortDirection::output());
  const LibCellInfo nand_info(nand);
  EXPECT_FALSE(nand_info.hasBusPins());
  EXPECT_FALSE(nand_info.hasBidirectionalPins());
  EXPECT_FALSE(nand_info.hasUnknownDirectionPins());
  EXPECT_FALSE(nand_info.hasTristateOutputs());
  EXPECT_TRUE(nand_info.checkSupported().ok());
}

// Bus pin: expanded to bits, but not supported yet.
TEST_F(LibCellTableTest, RejectsBusPins)
{
  const sta::PortDirection* in = sta::PortDirection::input();
  sta::LibertyCell* bus = lib_.cell("BUS_AND");
  sta::BusDcl* bus2 = lib_.library().makeBusDcl("bus2", 1, 0);
  lib_.builder().makeBusPort(bus, "D", 1, 0, bus2)->setDirection(in);
  lib_.port(bus, "Y", sta::PortDirection::output());
  const LibCellInfo bus_info(bus);
  EXPECT_TRUE(bus_info.hasBusPins());
  EXPECT_FALSE(bus_info.hasBidirectionalPins());
  EXPECT_FALSE(bus_info.hasTristateOutputs());
  EXPECT_EQ(bus_info.numInputs(), 2);
  expectUnimplemented(bus_info, "bus or bundle pins");
}

// Bundle pin (members D0, D1), as the Liberty reader builds it: the members
// are top-level ports too, so each is listed once and the bundle itself not
// at all. Not supported yet either.
TEST_F(LibCellTableTest, RejectsBundlePins)
{
  const sta::PortDirection* in = sta::PortDirection::input();
  sta::LibertyCell* bundle = lib_.cell("BUNDLE_AND");
  auto* members = new sta::ConcretePortSeq;  // owned by the bundle port
  sta::LibertyPort* d0 = lib_.port(bundle, "D0", in);
  sta::LibertyPort* d1 = lib_.port(bundle, "D1", in);
  members->push_back(d0);
  members->push_back(d1);
  lib_.builder().makeBundlePort(bundle, "D", members)->setDirection(in);
  lib_.port(bundle, "Y", sta::PortDirection::output());
  const LibCellInfo bundle_info(bundle);
  EXPECT_TRUE(bundle_info.hasBusPins());
  EXPECT_FALSE(bundle_info.hasBidirectionalPins());
  ASSERT_EQ(bundle_info.numInputs(), 2);
  EXPECT_EQ(bundle_info.input(0), d0);
  EXPECT_EQ(bundle_info.input(1), d1);
  EXPECT_EQ(bundle_info.findInput("D"), -1);
  expectUnimplemented(bundle_info, "bus or bundle pins");
}

// Gives `cell` a test_cell, as the Liberty reader does; the cell owns it.
sta::TestCell* addTestCell(TestLiberty& lib, sta::LibertyCell* cell)
{
  auto* test_cell = new sta::TestCell(&lib.library(),
                                      absl::StrCat(cell->name(), "/test_cell"),
                                      lib.library().filename());
  cell->setTestCell(test_cell);
  return test_cell;
}

// A multi-bit flop with bundle pins whose ff_bank is only in its test_cell
// (MBFF2_SET_X1 below): bundle members listed once, and a black box, since
// the test cell's state is not tied to the cell's outputs.
//   cell (MBFF2_SET_X1) {
//     bundle (D) { members (D1, D2); direction : input; }
//     pin (CK) { direction : input; }  pin (SN) { direction : input; }
//     bundle (Q) { members (Q1, Q2); direction : output; }
//     test_cell () { ... ff_bank (IQ, IQN, 2) { ... } ... }
//   }
TEST_F(LibCellTableTest, BundleFlopWithStateOnlyInTestCellIsABlackBox)
{
  const sta::PortDirection* in = sta::PortDirection::input();
  const sta::PortDirection* out = sta::PortDirection::output();
  sta::LibertyCell* mbff = lib_.cell("MBFF2_SET_X1");
  auto* d_members = new sta::ConcretePortSeq;  // owned by the bundle port
  d_members->push_back(lib_.port(mbff, "D1", in));
  d_members->push_back(lib_.port(mbff, "D2", in));
  lib_.builder().makeBundlePort(mbff, "D", d_members)->setDirection(in);
  lib_.port(mbff, "CK", in);
  lib_.port(mbff, "SN", in);
  auto* q_members = new sta::ConcretePortSeq;
  q_members->push_back(lib_.port(mbff, "Q1", out));
  q_members->push_back(lib_.port(mbff, "Q2", out));
  lib_.builder().makeBundlePort(mbff, "Q", q_members)->setDirection(out);

  sta::TestCell* test = addTestCell(lib_, mbff);
  lib_.port(test, "D1", in);
  lib_.port(test, "CK", in);
  sta::LibertyPort* test_q = lib_.port(test, "Q1", out);
  sta::LibertyPort* test_iq
      = lib_.port(test, "IQ", sta::PortDirection::internal());
  TestLiberty::flop(test, true, test_iq, nullptr);
  test_q->setFunction(sta::FuncExpr::makePort(test_iq));

  const LibCellInfo info(mbff);
  ASSERT_EQ(info.numInputs(), 4);
  EXPECT_EQ(info.findInput("D1"), 0);
  EXPECT_EQ(info.findInput("D2"), 1);
  EXPECT_EQ(info.findInput("CK"), 2);
  EXPECT_EQ(info.findInput("SN"), 3);
  ASSERT_EQ(info.numOutputs(), 2);
  EXPECT_EQ(info.findOutput("Q1"), 0);
  EXPECT_EQ(info.findOutput("Q2"), 1);
  EXPECT_TRUE(info.isSequential());
  EXPECT_EQ(info.kind(), LibCellKind::kBlackBox);
  expectUnimplemented(info, "bus or bundle pins");
}

// A scan flop whose ff is only in its test_cell: sequential, and a black
// box for now (not a combinational kCell).
TEST_F(LibCellTableTest, FlopWithStateOnlyInTestCellIsABlackBox)
{
  const sta::PortDirection* in = sta::PortDirection::input();
  const sta::PortDirection* out = sta::PortDirection::output();
  sta::LibertyCell* sdff
      = lib_.combinational("SDFF", {"D", "SI", "SE", "CK"}, {"Q"});
  sta::TestCell* test = addTestCell(lib_, sdff);
  lib_.port(test, "D", in);
  lib_.port(test, "CK", in);
  sta::LibertyPort* test_q = lib_.port(test, "Q", out);
  sta::LibertyPort* test_iq
      = lib_.port(test, "IQ", sta::PortDirection::internal());
  TestLiberty::flop(test, true, test_iq, nullptr);
  test_q->setFunction(sta::FuncExpr::makePort(test_iq));

  const LibCellInfo info(sdff);
  EXPECT_TRUE(info.isSequential());
  EXPECT_EQ(info.kind(), LibCellKind::kBlackBox);
  EXPECT_EQ(info.numInputs(), 4);
  EXPECT_EQ(info.numOutputs(), 1);
  EXPECT_TRUE(info.checkSupported().ok()) << info.checkSupported();
}

// An integrated clock gate described by a statetable, as in Liberty:
//   cell (CLKGATE_POSEDGE_X1) {
//     clock_gating_integrated_cell : "latch_posedge_precontrol";
//     statetable ("CP TE E", "QD") { ... }
//     pin (QD) { direction : internal; internal_node : "QD"; }
//     pin (Q) { direction : output; state_function : "CP * QD"; }
//   }
// OpenSTA reads it as latch_posedge and does not read state_function, so Q
// has no function: the clock-gate check comes first and makes it kClockGate.
TEST_F(LibCellTableTest, StatetableClockGateIsAClockGate)
{
  const sta::PortDirection* in = sta::PortDirection::input();
  sta::LibertyCell* icg = lib_.cell("CLKGATE_POSEDGE_X1");
  sta::LibertyPort* cp = lib_.port(icg, "CP", in);
  sta::LibertyPort* e = lib_.port(icg, "E", in);
  sta::LibertyPort* te = lib_.port(icg, "TE", in);
  sta::LibertyPort* q = lib_.port(icg, "Q", sta::PortDirection::output());
  sta::LibertyPort* qd = lib_.port(icg, "QD", sta::PortDirection::internal());
  TestLiberty::statetable(icg, {cp, te, e}, {qd});
  icg->setClockGateType(sta::ClockGateType::latch_posedge);

  const LibCellInfo info(icg);
  EXPECT_EQ(info.kind(), LibCellKind::kClockGate);
  EXPECT_TRUE(info.isSequential());
  EXPECT_EQ(info.numInputs(), 3);
  ASSERT_EQ(info.numOutputs(), 1);
  EXPECT_EQ(info.output(0), q);
  EXPECT_TRUE(info.checkSupported().ok()) << info.checkSupported();
}

// Inout pin: left out of inputs() and outputs(), not supported yet.
TEST_F(LibCellTableTest, RejectsBidirectionalPins)
{
  sta::LibertyCell* pad = lib_.cell("PAD_IO");
  lib_.port(pad, "A", sta::PortDirection::input());
  lib_.port(pad, "PAD", sta::PortDirection::bidirect());
  lib_.port(pad, "Y", sta::PortDirection::output());
  const LibCellInfo pad_info(pad);
  EXPECT_FALSE(pad_info.hasBusPins());
  EXPECT_TRUE(pad_info.hasBidirectionalPins());
  EXPECT_FALSE(pad_info.hasUnknownDirectionPins());
  EXPECT_EQ(pad_info.numInputs(), 1);
  EXPECT_EQ(pad_info.numOutputs(), 1);
  EXPECT_EQ(pad_info.findInput("PAD"), -1);
  EXPECT_EQ(pad_info.findOutput("PAD"), -1);
  expectUnimplemented(pad_info, "bidirectional pins");
}

// Pin without a direction: left out of inputs() and outputs(), not
// supported yet.
TEST_F(LibCellTableTest, RejectsPinsWithoutDirection)
{
  sta::LibertyCell* nodir = lib_.cell("NODIR");
  lib_.port(nodir, "A", sta::PortDirection::input());
  lib_.port(nodir, "B", sta::PortDirection::unknown());
  lib_.port(nodir, "Y", sta::PortDirection::output());
  const LibCellInfo nodir_info(nodir);
  EXPECT_FALSE(nodir_info.hasBusPins());
  EXPECT_FALSE(nodir_info.hasBidirectionalPins());
  EXPECT_TRUE(nodir_info.hasUnknownDirectionPins());
  EXPECT_EQ(nodir_info.numInputs(), 1);
  EXPECT_EQ(nodir_info.findInput("B"), -1);
  expectUnimplemented(nodir_info, "pins without a direction");
}

// Combinational tristate buffer (TBUF): a kCell would hide the z.
TEST_F(LibCellTableTest, RejectsCombinationalTristateOutputs)
{
  const LibCellInfo tbuf_info(makeTbuf(lib_));
  EXPECT_EQ(tbuf_info.kind(), LibCellKind::kCombinational);
  EXPECT_FALSE(tbuf_info.hasBusPins());
  EXPECT_FALSE(tbuf_info.hasBidirectionalPins());
  EXPECT_TRUE(tbuf_info.hasTristateOutputs());
  EXPECT_EQ(tbuf_info.numOutputs(), 1);
  expectUnimplemented(tbuf_info, "tristate output");
}

// An output pin with a three_state enable counts as tristate too.
TEST_F(LibCellTableTest, RejectsOutputsWithThreeStateEnable)
{
  const sta::PortDirection* in = sta::PortDirection::input();
  sta::LibertyCell* tinv = lib_.cell("TINV");
  sta::LibertyPort* tinv_a = lib_.port(tinv, "A", in);
  sta::LibertyPort* tinv_en = lib_.port(tinv, "EN", in);
  sta::LibertyPort* tinv_zn
      = lib_.port(tinv, "ZN", sta::PortDirection::output());
  tinv_zn->setFunction(sta::FuncExpr::makeNot(sta::FuncExpr::makePort(tinv_a)));
  tinv_zn->setTristateEnable(sta::FuncExpr::makePort(tinv_en));
  const LibCellInfo tinv_info(tinv);
  EXPECT_EQ(tinv_info.kind(), LibCellKind::kCombinational);
  EXPECT_TRUE(tinv_info.hasTristateOutputs());
  expectUnimplemented(tinv_info, "tristate output");
}

// A sequential cell with a tristate output is a black box: supported.
TEST_F(LibCellTableTest, SupportsSequentialTristateCellsAsBlackBoxes)
{
  const sta::PortDirection* in = sta::PortDirection::input();
  sta::LibertyCell* tdff = lib_.cell("TDFF");
  lib_.port(tdff, "D", in);
  lib_.port(tdff, "CK", in);
  sta::LibertyPort* tdff_oe = lib_.port(tdff, "OE", in);
  sta::LibertyPort* tdff_q
      = lib_.port(tdff, "Q", sta::PortDirection::tristate());
  sta::LibertyPort* tdff_iq
      = lib_.port(tdff, "IQ", sta::PortDirection::internal());
  TestLiberty::flop(tdff, true, tdff_iq, nullptr);
  tdff_q->setFunction(sta::FuncExpr::makePort(tdff_iq));
  tdff_q->setTristateEnable(sta::FuncExpr::makePort(tdff_oe));
  const LibCellInfo tdff_info(tdff);
  EXPECT_EQ(tdff_info.kind(), LibCellKind::kBlackBox);
  EXPECT_TRUE(tdff_info.hasTristateOutputs());
  EXPECT_TRUE(tdff_info.checkSupported().ok()) << tdff_info.checkSupported();
}

// A cell without state whose outputs have no function (a hard macro, as in
// fake_macros.lib) is a black box, not a kCell of unknown function; a tie
// cell (constant function) stays combinational.
TEST_F(LibCellTableTest, CellsWithUnknownOutputsAreBlackBoxes)
{
  const sta::PortDirection* in = sta::PortDirection::input();
  const sta::PortDirection* out = sta::PortDirection::output();
  sta::LibertyCell* macro = lib_.cell("HM_1x1");
  lib_.port(macro, "A", in);
  lib_.port(macro, "Y", out);
  const LibCellInfo macro_info(macro);
  EXPECT_FALSE(macro_info.isSequential());
  EXPECT_EQ(macro_info.kind(), LibCellKind::kBlackBox);
  EXPECT_TRUE(macro_info.checkSupported().ok()) << macro_info.checkSupported();

  // One known output is not enough.
  sta::LibertyCell* half = lib_.combinational("HALF_KNOWN", {"A"}, {"Y"});
  lib_.port(half, "Z", out);
  EXPECT_EQ(LibCellInfo(half).kind(), LibCellKind::kBlackBox);

  sta::LibertyCell* tie = lib_.cell("LOGIC0");
  lib_.port(tie, "Z", out)->setFunction(sta::FuncExpr::makeZero());
  EXPECT_EQ(LibCellInfo(tie).kind(), LibCellKind::kCombinational);
}

// A register's outputs must read only its ff/latch state variables: an
// internal pin that is not one, or a constant output, makes a black box.
TEST_F(LibCellTableTest, RegisterOutputsReadOnlyTheState)
{
  const sta::PortDirection* in = sta::PortDirection::input();
  const sta::PortDirection* out = sta::PortDirection::output();
  const sta::PortDirection* internal = sta::PortDirection::internal();

  sta::LibertyCell* other = lib_.cell("DFF_OTHER_NODE");
  lib_.port(other, "D", in);
  lib_.port(other, "CK", in);
  sta::LibertyPort* other_q = lib_.port(other, "Q", out);
  sta::LibertyPort* other_iq = lib_.port(other, "IQ", internal);
  sta::LibertyPort* other_x = lib_.port(other, "X", internal);  // not state
  TestLiberty::flop(other, true, other_iq, nullptr);
  other_q->setFunction(sta::FuncExpr::makeAnd(
      sta::FuncExpr::makePort(other_iq), sta::FuncExpr::makePort(other_x)));
  EXPECT_EQ(LibCellInfo(other).kind(), LibCellKind::kBlackBox);

  sta::LibertyCell* constant = lib_.cell("DFF_TIE");
  lib_.port(constant, "D", in);
  lib_.port(constant, "CK", in);
  sta::LibertyPort* constant_q = lib_.port(constant, "Q", out);
  sta::LibertyPort* constant_lo = lib_.port(constant, "LO", out);
  sta::LibertyPort* constant_iq = lib_.port(constant, "IQ", internal);
  TestLiberty::flop(constant, true, constant_iq, nullptr);
  constant_q->setFunction(sta::FuncExpr::makePort(constant_iq));
  constant_lo->setFunction(sta::FuncExpr::makeZero());
  EXPECT_EQ(LibCellInfo(constant).kind(), LibCellKind::kBlackBox);
}

// The table interns unsupported cells too (a library may contain them).
TEST_F(LibCellTableTest, InternsUnsupportedCells)
{
  LibCellTable table;
  table.setMode(TableMode::kSingleWrite);
  const LibCellId tbuf_id = table.intern(makeTbuf(lib_));
  ASSERT_TRUE(tbuf_id.isValid());
  EXPECT_EQ(table.info(tbuf_id).checkSupported().code(),
            absl::StatusCode::kUnimplemented);
}

// Run under TSan. Every thread interns the same cells in its own order; all
// agree on the ids and each cell is stored once.
TEST_F(LibCellTableTest, ConcurrentInterningAgrees)
{
  constexpr int kThreads = 8;
  constexpr int kCells = 500;
  std::vector<sta::LibertyCell*> cells;
  cells.reserve(kCells);
  for (int i = 0; i < kCells; ++i) {
    // The builder is not thread-safe: the cells are made up front.
    cells.push_back(lib_.combinational(absl::StrCat("BUF_", i), {"A"}, {"Y"}));
  }
  LibCellTable table;
  table.setMode(TableMode::kParallelWrite);
  std::vector<std::vector<LibCellId>> ids(kThreads);
  std::vector<std::thread> threads;
  threads.reserve(kThreads);
  for (int t = 0; t < kThreads; ++t) {
    threads.emplace_back([&, t] {
      for (int k = 0; k < kCells; ++k) {
        const int i = (k * 7919 + t * 104729) % kCells;
        ids[t].push_back(table.intern(cells[i]));
        // Reads interleave with the other threads' writes.
        ASSERT_EQ(table.cell(ids[t].back()), cells[i]);
      }
    });
  }
  for (std::thread& thread : threads) {
    thread.join();
  }
  table.setMode(TableMode::kReadOnly);
  EXPECT_EQ(table.size(), static_cast<size_t>(kCells));
  for (int t = 0; t < kThreads; ++t) {
    for (int k = 0; k < kCells; ++k) {
      const int i = (k * 7919 + t * 104729) % kCells;
      ASSERT_EQ(table.find(cells[i]), ids[t][k]);
    }
  }
}

TEST_F(LibCellTableTest, DetectsPowerSensitiveCells)
{
  struct ExpectedLayout
  {
    sta::LibertyCell* cell;
    std::vector<sta::LibertyPort*> inputs;
    std::vector<sta::LibertyPort*> outputs;
    LibCellKind kind;
    bool b0;
    bool b1;
    bool b2;
    bool b3;
  };
  static_assert(sizeof(LibCellInfo) == sizeof(ExpectedLayout));

  // Ordinary cell with primary power and ground rails is not power-sensitive.
  sta::LibertyCell* plain = lib_.combinational("PLAIN_BUF", {"A"}, {"Y"});
  sta::LibertyPort* vdd = lib_.port(plain, "VDD", sta::PortDirection::power());
  vdd->setPwrGndType(sta::PwrGndType::primary_power);
  sta::LibertyPort* vss = lib_.port(plain, "VSS", sta::PortDirection::ground());
  vss->setPwrGndType(sta::PwrGndType::primary_ground);
  const LibCellInfo plain_info(plain);
  EXPECT_FALSE(plain_info.isPowerSensitive());
  EXPECT_FALSE(plain_info.isModeledRegister());

  // Cell-level flags: isolation, level-shifter, always-on.
  sta::LibertyCell* iso = lib_.combinational("ISO_BUF", {"A"}, {"Y"});
  iso->setIsIsolationCell(true);
  EXPECT_TRUE(LibCellInfo(iso).isPowerSensitive());

  sta::LibertyCell* lvl = lib_.combinational("LVL_BUF", {"A"}, {"Y"});
  lvl->setIsLevelShifter(true);
  EXPECT_TRUE(LibCellInfo(lvl).isPowerSensitive());

  // Same name, kind and ports: checksum() does not depend on power flags.
  TestLiberty other_lib{"other_lib", "other.lib"};
  sta::LibertyCell* same_aon
      = other_lib.combinational("PLAIN_BUF", {"A"}, {"Y"});
  same_aon->setAlwaysOn(true);
  const LibCellInfo same_aon_info(same_aon);
  EXPECT_TRUE(same_aon_info.isPowerSensitive());
  EXPECT_EQ(same_aon_info.checksum(), plain_info.checksum());

  // Port-level flags on scalar ports.
  sta::LibertyCell* iso_data
      = lib_.combinational("ISO_DATA", {"A", "EN"}, {"Y"});
  iso_data->findLibertyPort("A")->setIsolationCellData(true);
  EXPECT_TRUE(LibCellInfo(iso_data).isPowerSensitive());

  sta::LibertyCell* iso_en = lib_.combinational("ISO_EN", {"A", "EN"}, {"Y"});
  iso_en->findLibertyPort("EN")->setIsolationCellEnable(true);
  EXPECT_TRUE(LibCellInfo(iso_en).isPowerSensitive());

  sta::LibertyCell* lvl_data = lib_.combinational("LVL_DATA", {"A"}, {"Y"});
  lvl_data->findLibertyPort("A")->setLevelShifterData(true);
  EXPECT_TRUE(LibCellInfo(lvl_data).isPowerSensitive());

  sta::LibertyCell* psw = lib_.combinational("PSW", {"SLEEP"}, {"VDDOUT"});
  psw->findLibertyPort("VDDOUT")->setIsSwitch(true);
  EXPECT_TRUE(LibCellInfo(psw).isPowerSensitive());

  // Port-level flag on an expanded bus bit member.
  sta::LibertyCell* bus_iso = lib_.cell("BUS_ISO");
  sta::BusDcl* bus2 = lib_.library().makeBusDcl("bus2_pwr", 1, 0);
  sta::LibertyPort* bus_in
      = lib_.builder().makeBusPort(bus_iso, "A", 1, 0, bus2);
  bus_in->setDirection(sta::PortDirection::input());
  sta::LibertyPortMemberIterator members(bus_in);
  ASSERT_TRUE(members.hasNext());
  members.next()->setIsolationCellData(true);
  lib_.port(bus_iso, "Y", sta::PortDirection::output());
  EXPECT_TRUE(LibCellInfo(bus_iso).isPowerSensitive());

  // Backup power and backup ground pg_pins mark the cell power-sensitive.
  sta::LibertyCell* bkpwr = lib_.combinational("BKPWR_BUF", {"A"}, {"Y"});
  sta::LibertyPort* vpb = lib_.port(bkpwr, "VPB", sta::PortDirection::power());
  vpb->setPwrGndType(sta::PwrGndType::backup_power);
  EXPECT_TRUE(LibCellInfo(bkpwr).isPowerSensitive());

  sta::LibertyCell* bkgnd = lib_.combinational("BKGND_BUF", {"A"}, {"Y"});
  sta::LibertyPort* vnb = lib_.port(bkgnd, "VNB", sta::PortDirection::ground());
  vnb->setPwrGndType(sta::PwrGndType::backup_ground);
  EXPECT_TRUE(LibCellInfo(bkgnd).isPowerSensitive());

  // Wells are not power rails of interest: most cells have them.
  sta::LibertyCell* wells = lib_.combinational("WELL_BUF", {"A"}, {"Y"});
  lib_.port(wells, "VPB", sta::PortDirection::power())
      ->setPwrGndType(sta::PwrGndType::nwell);
  lib_.port(wells, "VNB", sta::PortDirection::ground())
      ->setPwrGndType(sta::PwrGndType::pwell);
  EXPECT_FALSE(LibCellInfo(wells).isPowerSensitive());

  // Power switches: coarse-grain cell type, an internal switch pin, or an
  // internal (switched) rail, each on a buffered sleep chain SLEEP_OUT =
  // SLEEP that would otherwise look like a plain buffer.
  sta::LibertyCell* coarse
      = lib_.combinational("HEADER_COARSE", {"SLEEP"}, {"SLEEP_OUT"});
  coarse->setSwitchCellType(sta::SwitchCellType::coarse_grain);
  EXPECT_TRUE(LibCellInfo(coarse).isPowerSensitive());

  sta::LibertyCell* fine
      = lib_.combinational("HEADER_FINE", {"SLEEP"}, {"SLEEP_OUT"});
  fine->setSwitchCellType(sta::SwitchCellType::fine_grain);  // the default
  EXPECT_FALSE(LibCellInfo(fine).isPowerSensitive());

  sta::LibertyCell* sw_pin
      = lib_.combinational("HEADER_SW_PIN", {"SLEEP"}, {"SLEEP_OUT"});
  lib_.port(sw_pin, "VVDD", sta::PortDirection::internal())->setIsSwitch(true);
  const LibCellInfo sw_pin_info(sw_pin);
  EXPECT_TRUE(sw_pin_info.isPowerSensitive());
  EXPECT_EQ(sw_pin_info.kind(), LibCellKind::kCombinational);

  sta::LibertyCell* vvdd
      = lib_.combinational("HEADER_VVDD", {"SLEEP"}, {"SLEEP_OUT"});
  lib_.port(vvdd, "VVDD", sta::PortDirection::power())
      ->setPwrGndType(sta::PwrGndType::internal_power);
  EXPECT_TRUE(LibCellInfo(vvdd).isPowerSensitive());

  sta::LibertyCell* vvss
      = lib_.combinational("FOOTER_VVSS", {"SLEEP"}, {"SLEEP_OUT"});
  lib_.port(vvss, "VVSS", sta::PortDirection::ground())
      ->setPwrGndType(sta::PwrGndType::internal_ground);
  EXPECT_TRUE(LibCellInfo(vvss).isPowerSensitive());
}

TEST_F(LibCellTableTest, ClassifiesModeledAndUnmodeledRegisters)
{
  const sta::PortDirection* in = sta::PortDirection::input();
  const sta::PortDirection* out = sta::PortDirection::output();
  const sta::PortDirection* internal = sta::PortDirection::internal();

  // Ordinary flip-flop and async set/reset flip-flop: modeled.
  sta::LibertyCell* dff = lib_.cell("DFF_PLAIN");
  sta::LibertyPort* dff_d = lib_.port(dff, "D", in);
  sta::LibertyPort* dff_ck = lib_.port(dff, "CK", in);
  sta::LibertyPort* dff_q = lib_.port(dff, "Q", out);
  sta::LibertyPort* dff_iq = lib_.port(dff, "IQ", internal);
  dff->makeSequential(1,
                      /*is_register=*/true,
                      sta::FuncExpr::makePort(dff_ck),
                      sta::FuncExpr::makePort(dff_d),
                      nullptr,
                      nullptr,
                      sta::LogicValue::unknown,
                      sta::LogicValue::unknown,
                      dff_iq,
                      nullptr);
  dff_q->setFunction(sta::FuncExpr::makePort(dff_iq));
  const LibCellInfo dff_info(dff);
  EXPECT_EQ(dff_info.kind(), LibCellKind::kRegister);
  EXPECT_TRUE(dff_info.isModeledRegister());

  // A sequential group without a next_state function (TestLiberty::dff) is a
  // kRegister, not modeled.
  const LibCellInfo no_fn_info(lib_.dff("DFF_NO_NEXT"));
  EXPECT_EQ(no_fn_info.kind(), LibCellKind::kRegister);
  EXPECT_FALSE(no_fn_info.isModeledRegister());

  const LibCellInfo dffrs_info(
      lib_.dffrs("DFFRS_PLAIN", sta::LogicValue::zero, sta::LogicValue::one));
  EXPECT_EQ(dffrs_info.kind(), LibCellKind::kRegister);
  EXPECT_TRUE(dffrs_info.isModeledRegister());

  // Scan flip-flop whose scan mux is part of next_state: modeled.
  sta::LibertyCell* sdff = lib_.cell("SDFF_PLAIN");
  sta::LibertyPort* sd_d = lib_.port(sdff, "D", in);
  sta::LibertyPort* sd_si = lib_.port(sdff, "SI", in);
  sta::LibertyPort* sd_se = lib_.port(sdff, "SE", in);
  sta::LibertyPort* sd_ck = lib_.port(sdff, "CK", in);
  sta::LibertyPort* sd_q = lib_.port(sdff, "Q", out);
  sta::LibertyPort* sd_iq = lib_.port(sdff, "IQ", internal);
  sta::FuncExpr* mux = sta::FuncExpr::makeOr(
      sta::FuncExpr::makeAnd(sta::FuncExpr::makePort(sd_se),
                             sta::FuncExpr::makePort(sd_si)),
      sta::FuncExpr::makeAnd(
          sta::FuncExpr::makeNot(sta::FuncExpr::makePort(sd_se)),
          sta::FuncExpr::makePort(sd_d)));
  sdff->makeSequential(1,
                       /*is_register=*/true,
                       sta::FuncExpr::makePort(sd_ck),
                       mux,
                       nullptr,
                       nullptr,
                       sta::LogicValue::unknown,
                       sta::LogicValue::unknown,
                       sd_iq,
                       nullptr);
  sd_q->setFunction(sta::FuncExpr::makePort(sd_iq));
  const LibCellInfo sdff_info(sdff);
  EXPECT_EQ(sdff_info.kind(), LibCellKind::kRegister);
  EXPECT_TRUE(sdff_info.isModeledRegister());

  // 1-FF retention flop with unmodeled SAVE/NRESTORE inputs: kRegister, not
  // modeled.
  sta::LibertyCell* ret1 = lib_.cell("RET_DFF_1");
  sta::LibertyPort* r1_d = lib_.port(ret1, "D", in);
  sta::LibertyPort* r1_ck = lib_.port(ret1, "CK", in);
  lib_.port(ret1, "SAVE", in);
  lib_.port(ret1, "NRESTORE", in);
  sta::LibertyPort* r1_q = lib_.port(ret1, "Q", out);
  sta::LibertyPort* r1_iq = lib_.port(ret1, "IQ", internal);
  ret1->makeSequential(1,
                       /*is_register=*/true,
                       sta::FuncExpr::makePort(r1_ck),
                       sta::FuncExpr::makePort(r1_d),
                       nullptr,
                       nullptr,
                       sta::LogicValue::unknown,
                       sta::LogicValue::unknown,
                       r1_iq,
                       nullptr);
  r1_q->setFunction(sta::FuncExpr::makePort(r1_iq));
  const LibCellInfo ret1_info(ret1);
  EXPECT_EQ(ret1_info.kind(), LibCellKind::kRegister);
  EXPECT_FALSE(ret1_info.isModeledRegister());

  // 2-sequential-group retention cell (balloon latch + main FF): kRegister,
  // not modeled.
  sta::LibertyCell* ret2 = lib_.cell("RET_DFF_2");
  sta::LibertyPort* r2_d = lib_.port(ret2, "D", in);
  sta::LibertyPort* r2_ck = lib_.port(ret2, "CK", in);
  sta::LibertyPort* r2_save = lib_.port(ret2, "SAVE", in);
  sta::LibertyPort* r2_nrestore = lib_.port(ret2, "NRESTORE", in);
  sta::LibertyPort* r2_q = lib_.port(ret2, "Q", out);
  sta::LibertyPort* r2_iq = lib_.port(ret2, "IQ", internal);
  sta::LibertyPort* r2_ret = lib_.port(ret2, "RET", internal);
  ret2->makeSequential(1,
                       /*is_register=*/false,
                       sta::FuncExpr::makePort(r2_save),
                       sta::FuncExpr::makePort(r2_iq),
                       nullptr,
                       nullptr,
                       sta::LogicValue::unknown,
                       sta::LogicValue::unknown,
                       r2_ret,
                       nullptr);
  ret2->makeSequential(
      1,
      /*is_register=*/true,
      sta::FuncExpr::makePort(r2_ck),
      sta::FuncExpr::makePort(r2_d),
      sta::FuncExpr::makeAnd(
          sta::FuncExpr::makeNot(sta::FuncExpr::makePort(r2_nrestore)),
          sta::FuncExpr::makeNot(sta::FuncExpr::makePort(r2_ret))),
      sta::FuncExpr::makeAnd(
          sta::FuncExpr::makeNot(sta::FuncExpr::makePort(r2_nrestore)),
          sta::FuncExpr::makePort(r2_ret)),
      sta::LogicValue::zero,
      sta::LogicValue::zero,
      r2_iq,
      nullptr);
  r2_q->setFunction(sta::FuncExpr::makePort(r2_iq));
  const LibCellInfo ret2_info(ret2);
  EXPECT_EQ(ret2_info.kind(), LibCellKind::kRegister);
  EXPECT_FALSE(ret2_info.isModeledRegister());

  // Power-sensitive isolation latch: a modeled kRegister, flagged
  // power-sensitive so that passes can keep off it.
  sta::LibertyCell* iso_lat = lib_.cell("ISO_LATCH");
  sta::LibertyPort* il_d = lib_.port(iso_lat, "D", in);
  sta::LibertyPort* il_sb = lib_.port(iso_lat, "SLEEP_B", in);
  il_sb->setIsolationCellEnable(true);
  sta::LibertyPort* il_q = lib_.port(iso_lat, "Q", out);
  sta::LibertyPort* il_iq = lib_.port(iso_lat, "IQ", internal);
  iso_lat->makeSequential(1,
                          /*is_register=*/false,
                          sta::FuncExpr::makePort(il_sb),
                          sta::FuncExpr::makePort(il_d),
                          nullptr,
                          nullptr,
                          sta::LogicValue::unknown,
                          sta::LogicValue::unknown,
                          il_iq,
                          nullptr);
  il_q->setFunction(sta::FuncExpr::makePort(il_iq));
  const LibCellInfo iso_lat_info(iso_lat);
  EXPECT_EQ(iso_lat_info.kind(), LibCellKind::kRegister);
  EXPECT_TRUE(iso_lat_info.isPowerSensitive());
  EXPECT_TRUE(iso_lat_info.isModeledRegister());

  // 1-bit bus-pin register (unsupported pin shape): kRegister, not modeled.
  sta::LibertyCell* bus_dff = lib_.cell("BUS_DFF");
  sta::BusDcl* bus1 = lib_.library().makeBusDcl("bus1_reg", 0, 0);
  sta::LibertyPort* bus_d
      = lib_.builder().makeBusPort(bus_dff, "D", 0, 0, bus1);
  bus_d->setDirection(in);
  sta::LibertyPortMemberIterator d_members(bus_d);
  ASSERT_TRUE(d_members.hasNext());
  sta::LibertyPort* d0 = d_members.next();
  sta::LibertyPort* bd_ck = lib_.port(bus_dff, "CK", in);
  sta::LibertyPort* bd_q = lib_.port(bus_dff, "Q", out);
  sta::LibertyPort* bd_iq = lib_.port(bus_dff, "IQ", internal);
  bus_dff->makeSequential(1,
                          /*is_register=*/true,
                          sta::FuncExpr::makePort(bd_ck),
                          sta::FuncExpr::makePort(d0),
                          nullptr,
                          nullptr,
                          sta::LogicValue::unknown,
                          sta::LogicValue::unknown,
                          bd_iq,
                          nullptr);
  bd_q->setFunction(sta::FuncExpr::makePort(bd_iq));
  const LibCellInfo bus_dff_info(bus_dff);
  EXPECT_EQ(bus_dff_info.kind(), LibCellKind::kRegister);
  EXPECT_TRUE(bus_dff_info.hasBusPins());
  EXPECT_FALSE(bus_dff_info.isModeledRegister());

  // Sequential group missing primary state variable (output == nullptr):
  // kRegister, not modeled.
  sta::LibertyCell* iqn_only = lib_.cell("IQN_ONLY_DFF");
  sta::LibertyPort* io_d = lib_.port(iqn_only, "D", in);
  sta::LibertyPort* io_ck = lib_.port(iqn_only, "CK", in);
  sta::LibertyPort* io_qn = lib_.port(iqn_only, "QN", out);
  sta::LibertyPort* io_iqn = lib_.port(iqn_only, "IQN", internal);
  iqn_only->makeSequential(1,
                           /*is_register=*/true,
                           sta::FuncExpr::makePort(io_ck),
                           sta::FuncExpr::makePort(io_d),
                           nullptr,
                           nullptr,
                           sta::LogicValue::unknown,
                           sta::LogicValue::unknown,
                           nullptr,
                           io_iqn);
  io_qn->setFunction(sta::FuncExpr::makePort(io_iqn));
  const LibCellInfo iqn_only_info(iqn_only);
  EXPECT_EQ(iqn_only_info.kind(), LibCellKind::kRegister);
  EXPECT_FALSE(iqn_only_info.isModeledRegister());

  // Register with a tristate-direction output (even without three_state expr):
  // not modeled.
  sta::LibertyCell* tri_dir = lib_.cell("TRI_DIR_DFF");
  sta::LibertyPort* td_d = lib_.port(tri_dir, "D", in);
  sta::LibertyPort* td_ck = lib_.port(tri_dir, "CK", in);
  sta::LibertyPort* td_q
      = lib_.port(tri_dir, "Q", sta::PortDirection::tristate());
  sta::LibertyPort* td_iq = lib_.port(tri_dir, "IQ", internal);
  tri_dir->makeSequential(1,
                          /*is_register=*/true,
                          sta::FuncExpr::makePort(td_ck),
                          sta::FuncExpr::makePort(td_d),
                          nullptr,
                          nullptr,
                          sta::LogicValue::unknown,
                          sta::LogicValue::unknown,
                          td_iq,
                          nullptr);
  td_q->setFunction(sta::FuncExpr::makePort(td_iq));
  const LibCellInfo tri_dir_info(tri_dir);
  EXPECT_EQ(tri_dir_info.kind(), LibCellKind::kBlackBox);
  EXPECT_TRUE(tri_dir_info.hasTristateOutputs());
  EXPECT_FALSE(tri_dir_info.isModeledRegister());

  // Pulse latch: the enable expects a pulse. Still a register (its state
  // cuts loops), but not modeled, so the mapper never picks it for a level
  // latch.
  sta::LibertyCell* pulse = lib_.cell("PULSE_LATCH");
  sta::LibertyPort* pl_d = lib_.port(pulse, "D", in);
  sta::LibertyPort* pl_ck = lib_.port(pulse, "CK", in);
  pl_ck->setPulseClk(sta::RiseFall::rise(), sta::RiseFall::rise());
  sta::LibertyPort* pl_q = lib_.port(pulse, "Q", out);
  sta::LibertyPort* pl_iq = lib_.port(pulse, "IQ", internal);
  pulse->makeSequential(1,
                        /*is_register=*/false,
                        sta::FuncExpr::makePort(pl_ck),
                        sta::FuncExpr::makePort(pl_d),
                        nullptr,
                        nullptr,
                        sta::LogicValue::unknown,
                        sta::LogicValue::unknown,
                        pl_iq,
                        nullptr);
  pl_q->setFunction(sta::FuncExpr::makePort(pl_iq));
  const LibCellInfo pulse_info(pulse);
  EXPECT_EQ(pulse_info.kind(), LibCellKind::kRegister);
  EXPECT_FALSE(pulse_info.isModeledRegister());

  // Toggle flop: next_state = T ^ IQ may read the state, so it is modeled.
  sta::LibertyCell* tff = lib_.cell("TFF");
  sta::LibertyPort* tff_t = lib_.port(tff, "T", in);
  sta::LibertyPort* tff_ck = lib_.port(tff, "CK", in);
  sta::LibertyPort* tff_q = lib_.port(tff, "Q", out);
  sta::LibertyPort* tff_iq = lib_.port(tff, "IQ", internal);
  tff->makeSequential(1,
                      /*is_register=*/true,
                      sta::FuncExpr::makePort(tff_ck),
                      sta::FuncExpr::makeXor(sta::FuncExpr::makePort(tff_t),
                                             sta::FuncExpr::makePort(tff_iq)),
                      nullptr,
                      nullptr,
                      sta::LogicValue::unknown,
                      sta::LogicValue::unknown,
                      tff_iq,
                      nullptr);
  tff_q->setFunction(sta::FuncExpr::makePort(tff_iq));
  const LibCellInfo tff_info(tff);
  EXPECT_EQ(tff_info.kind(), LibCellKind::kRegister);
  EXPECT_TRUE(tff_info.isModeledRegister());

  // Self-clearing flop: clear = RN & IQ reads the state, so it is not
  // modeled.
  sta::LibertyCell* self_clr = lib_.cell("SELF_CLEAR_DFF");
  sta::LibertyPort* sc_d = lib_.port(self_clr, "D", in);
  sta::LibertyPort* sc_ck = lib_.port(self_clr, "CK", in);
  sta::LibertyPort* sc_rn = lib_.port(self_clr, "RN", in);
  sta::LibertyPort* sc_q = lib_.port(self_clr, "Q", out);
  sta::LibertyPort* sc_iq = lib_.port(self_clr, "IQ", internal);
  self_clr->makeSequential(
      1,
      /*is_register=*/true,
      sta::FuncExpr::makePort(sc_ck),
      sta::FuncExpr::makePort(sc_d),
      sta::FuncExpr::makeAnd(sta::FuncExpr::makePort(sc_rn),
                             sta::FuncExpr::makePort(sc_iq)),
      nullptr,
      sta::LogicValue::unknown,
      sta::LogicValue::unknown,
      sc_iq,
      nullptr);
  sc_q->setFunction(sta::FuncExpr::makePort(sc_iq));
  const LibCellInfo self_clr_info(self_clr);
  EXPECT_EQ(self_clr_info.kind(), LibCellKind::kRegister);
  EXPECT_FALSE(self_clr_info.isModeledRegister());
}

}  // namespace
}  // namespace syn::test
