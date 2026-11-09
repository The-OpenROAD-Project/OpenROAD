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
  EXPECT_EQ(inv_info.numInputs(), 1u);
  EXPECT_EQ(inv_info.numOutputs(), 1u);
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
  EXPECT_EQ(dff_info.numInputs(), 2u);
  EXPECT_EQ(dff_info.numOutputs(), 2u);
  EXPECT_EQ(dff_info.findInput("D"), 0);
  EXPECT_EQ(dff_info.findInput("CLK"), 1);
  EXPECT_EQ(dff_info.findOutput("Q"), 0);
  EXPECT_EQ(dff_info.findOutput("QN"), 1);
  EXPECT_GT(table.memoryBytes(), 0u);
}

TEST_F(LibCellTableTest, ClassifiesCellKinds)
{
  sta::PortDirection* in = sta::PortDirection::input();
  sta::PortDirection* out = sta::PortDirection::output();
  sta::PortDirection* internal = sta::PortDirection::internal();

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

  // Statetable cell whose output is a state node (e.g. a synchronizer).
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
  EXPECT_EQ(LibCellInfo(sync).kind(), LibCellKind::kRegister);
  EXPECT_EQ(LibCellInfo(opaque).kind(), LibCellKind::kBlackBox);
  EXPECT_EQ(LibCellInfo(gated).kind(), LibCellKind::kBlackBox);
  EXPECT_EQ(LibCellInfo(tri).kind(), LibCellKind::kBlackBox);
  EXPECT_EQ(LibCellInfo(icg).kind(), LibCellKind::kClockGate);
  EXPECT_TRUE(LibCellInfo(icg).isSequential());
  EXPECT_EQ(LibCellInfo(sink).kind(), LibCellKind::kBlackBox);
  EXPECT_STREQ(libCellKindName(LibCellKind::kClockGate), "clock_gate");
  EXPECT_STREQ(libCellKindName(LibCellKind::kBlackBox), "black_box");
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
  ASSERT_EQ(info.numInputs(), 1u);
  EXPECT_EQ(info.input(0), a);
  ASSERT_EQ(info.numOutputs(), 1u);
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
  ASSERT_EQ(info.numInputs(), 4u);
  EXPECT_EQ(std::string_view(info.input(0)->name()), "D[0]");
  EXPECT_EQ(std::string_view(info.input(1)->name()), "D[1]");
  EXPECT_EQ(std::string_view(info.input(2)->name()), "D[2]");
  EXPECT_EQ(std::string_view(info.input(3)->name()), "D[3]");

  ASSERT_EQ(info.numOutputs(), 2u);
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
  sta::PortDirection* in = sta::PortDirection::input();
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
  sta::PortDirection* in = sta::PortDirection::input();
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
  sta::PortDirection* in = sta::PortDirection::input();
  sta::LibertyCell* bus = lib_.cell("BUS_AND");
  sta::BusDcl* bus2 = lib_.library().makeBusDcl("bus2", 1, 0);
  lib_.builder().makeBusPort(bus, "D", 1, 0, bus2)->setDirection(in);
  lib_.port(bus, "Y", sta::PortDirection::output());
  const LibCellInfo bus_info(bus);
  EXPECT_TRUE(bus_info.hasBusPins());
  EXPECT_FALSE(bus_info.hasBidirectionalPins());
  EXPECT_FALSE(bus_info.hasTristateOutputs());
  EXPECT_EQ(bus_info.numInputs(), 2u);
  expectUnimplemented(bus_info, "bus or bundle pins");
}

// Bundle pin (members D0, D1), as the Liberty reader builds it: not
// supported yet either.
TEST_F(LibCellTableTest, RejectsBundlePins)
{
  sta::PortDirection* in = sta::PortDirection::input();
  sta::LibertyCell* bundle = lib_.cell("BUNDLE_AND");
  auto* members = new sta::ConcretePortSeq;  // owned by the bundle port
  members->push_back(lib_.port(bundle, "D0", in));
  members->push_back(lib_.port(bundle, "D1", in));
  lib_.builder().makeBundlePort(bundle, "D", members)->setDirection(in);
  lib_.port(bundle, "Y", sta::PortDirection::output());
  const LibCellInfo bundle_info(bundle);
  EXPECT_TRUE(bundle_info.hasBusPins());
  EXPECT_FALSE(bundle_info.hasBidirectionalPins());
  expectUnimplemented(bundle_info, "bus or bundle pins");
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
  EXPECT_EQ(pad_info.numInputs(), 1u);
  EXPECT_EQ(pad_info.numOutputs(), 1u);
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
  EXPECT_EQ(nodir_info.numInputs(), 1u);
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
  EXPECT_EQ(tbuf_info.numOutputs(), 1u);
  expectUnimplemented(tbuf_info, "tristate output");
}

// An output pin with a three_state enable counts as tristate too.
TEST_F(LibCellTableTest, RejectsOutputsWithThreeStateEnable)
{
  sta::PortDirection* in = sta::PortDirection::input();
  sta::LibertyCell* tinv = lib_.cell("TINV");
  lib_.port(tinv, "A", in);
  sta::LibertyPort* tinv_en = lib_.port(tinv, "EN", in);
  lib_.port(tinv, "ZN", sta::PortDirection::output())
      ->setTristateEnable(sta::FuncExpr::makePort(tinv_en));
  const LibCellInfo tinv_info(tinv);
  EXPECT_EQ(tinv_info.kind(), LibCellKind::kCombinational);
  EXPECT_TRUE(tinv_info.hasTristateOutputs());
  expectUnimplemented(tinv_info, "tristate output");
}

// A sequential cell with a tristate output is a black box: supported.
TEST_F(LibCellTableTest, SupportsSequentialTristateCellsAsBlackBoxes)
{
  sta::PortDirection* in = sta::PortDirection::input();
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

}  // namespace
}  // namespace syn::test
