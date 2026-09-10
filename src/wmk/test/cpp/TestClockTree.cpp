// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <string>
#include <vector>

#include "ClockTree.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "sta/Liberty.hh"
#include "tst/nangate45_fixture.h"

namespace wmk {
namespace {

class TestClockTree : public tst::Nangate45Fixture
{
 protected:
  void SetUp() override
  {
    readLiberty("_main/test/Nangate45/Nangate45_typ.lib");
    sta_->postReadDb(db_.get());
    network_ = sta_->getDbNetwork();
    root_ = odb::dbNet::create(block_, "clk");
    odb::dbBTerm::create(root_, "clk")->setIoType(odb::dbIoType::INPUT);
  }

  odb::dbInst* cell(const char* master, const char* name)
  {
    return odb::dbInst::create(block_, lib_->findMaster(master), name);
  }

  odb::dbInst* buffer(const char* master, const char* name, odb::dbNet* input)
  {
    auto* inst = cell(master, name);
    inst->findITerm("A")->connect(input);
    auto* net
        = odb::dbNet::create(block_, (std::string(name) + "_out").c_str());
    net->setSigType(odb::dbSigType::CLOCK);
    inst->findITerm(std::string(master).starts_with("INV") ? "ZN" : "Z")
        ->connect(net);
    return inst;
  }

  sta::dbNetwork* network_ = nullptr;
  odb::dbNet* root_ = nullptr;
};

TEST_F(TestClockTree, EquivalentBufferChains)
{
  auto* a = buffer("CLKBUF_X3", "a", root_);
  auto* trunk = buffer("BUF_X1", "trunk", root_);
  auto* b = buffer("CLKBUF_X3", "b", singleOutputNet(trunk));
  ASSERT_TRUE(clockBranch(a, network_));
  EXPECT_EQ(clockBranch(a, network_), clockBranch(b, network_));
}

TEST_F(TestClockTree, PreservesInversionParity)
{
  auto* a = buffer("CLKBUF_X3", "a", root_);
  auto* inv = buffer("INV_X1", "inv", root_);
  auto* b = buffer("CLKBUF_X3", "b", singleOutputNet(inv));
  auto* c = buffer("INV_X1", "c", singleOutputNet(b));
  ASSERT_TRUE(clockBranch(b, network_));
  EXPECT_NE(clockBranch(a, network_), clockBranch(b, network_));
  EXPECT_EQ(clockBranch(a, network_), clockBranch(c, network_));
}

TEST_F(TestClockTree, ClockGateIsABoundary)
{
  auto* gate = cell("CLKGATE_X1", "gate");
  gate->findITerm("CK")->connect(root_);
  auto* gated = odb::dbNet::create(block_, "gated");
  gate->findITerm("GCK")->connect(gated);
  auto* a = buffer("CLKBUF_X3", "a", root_);
  auto* b = buffer("CLKBUF_X3", "b", gated);
  auto* c = buffer("CLKBUF_X3", "c", gated);
  const auto branch = clockBranch(b, network_);
  ASSERT_TRUE(branch);
  EXPECT_NE(clockBranch(a, network_), branch);
  EXPECT_EQ(branch, clockBranch(c, network_));
  const ClockBranch expected{.source = gated, .inverted = false};
  EXPECT_EQ(branch, expected);
}

TEST_F(TestClockTree, MuxIsABoundaryEvenWithIdenticalInputs)
{
  auto* mux = cell("MUX2_X1", "mux");
  mux->findITerm("A")->connect(root_);
  mux->findITerm("B")->connect(root_);
  auto* muxed = odb::dbNet::create(block_, "muxed");
  mux->findITerm("Z")->connect(muxed);
  auto* a = buffer("CLKBUF_X3", "a", root_);
  auto* b = buffer("CLKBUF_X3", "b", muxed);
  ASSERT_TRUE(clockBranch(b, network_));
  EXPECT_NE(clockBranch(a, network_), clockBranch(b, network_));
}

TEST_F(TestClockTree, RejectsMultipleDrivers)
{
  auto* a = buffer("CLKBUF_X3", "a", root_);
  auto* b = buffer("CLKBUF_X3", "b", root_);
  b->findITerm("Z")->connect(singleOutputNet(a));
  EXPECT_FALSE(clockBranch(a, network_));
}

TEST_F(TestClockTree, RejectsFloatingNetsAndCycles)
{
  auto* floating = odb::dbNet::create(block_, "floating");
  auto* a = buffer("CLKBUF_X3", "a", floating);
  EXPECT_FALSE(clockBranch(a, network_));
  auto* b = buffer("CLKBUF_X3", "b", singleOutputNet(a));
  a->findITerm("A")->connect(singleOutputNet(b));
  EXPECT_FALSE(clockBranch(a, network_));
}

TEST_F(TestClockTree, UsesLibertyClockPins)
{
  auto* ff = cell("DFF_X1", "ff");
  auto* gate = cell("CLKGATE_X1", "gate");
  // Neither the LEF CLOCK flag nor a conventional pin name is sufficient.
  ff->findITerm("D")->getMTerm()->setSigType(odb::dbSigType::CLOCK);
  ff->findITerm("CK")->getMTerm()->setSigType(odb::dbSigType::SIGNAL);
  EXPECT_TRUE(isSequentialClockSink(ff->findITerm("CK"), network_));
  EXPECT_FALSE(isSequentialClockSink(ff->findITerm("D"), network_));
  EXPECT_FALSE(isSequentialClockSink(gate->findITerm("CK"), network_));
  // STA's classification also works for an unconventional clock-pin name.
  network_->libertyPort(network_->dbToSta(ff->findITerm("D")))
      ->setIsRegClk(true);
  EXPECT_TRUE(isSequentialClockSink(ff->findITerm("D"), network_));
}

TEST_F(TestClockTree, FanoutExcludesClockGateInputs)
{
  auto* a = buffer("CLKBUF_X3", "a", root_);
  auto* ff = cell("DFF_X1", "ff");
  auto* gate = cell("CLKGATE_X1", "gate");
  ff->findITerm("CK")->connect(singleOutputNet(a));
  gate->findITerm("CK")->connect(singleOutputNet(a));
  EXPECT_EQ(seqFanout(a, network_), 1);
  EXPECT_EQ(findLeafClockBuffers(block_, network_),
            std::vector<odb::dbInst*>{a});
}

TEST_F(TestClockTree, RejectsSequentialAndLogicCarriers)
{
  auto* ff = cell("DFF_X1", "ff");
  auto* gate = cell("CLKGATE_X1", "gate");
  EXPECT_FALSE(seqFanout(ff, network_));
  EXPECT_FALSE(seqFanout(gate, network_));
  EXPECT_FALSE(clockBranch(ff, network_));
  EXPECT_FALSE(clockBranch(gate, network_));
}

TEST_F(TestClockTree, UnknownFanoutCannotEstablishParity)
{
  auto* a = buffer("CLKBUF_X3", "a", root_);
  auto* master = odb::dbMaster::create(lib_, "UNKNOWN");
  odb::dbMTerm::create(
      master, "CLK", odb::dbIoType::INPUT, odb::dbSigType::CLOCK);
  master->setFrozen();
  auto* unknown = cell("UNKNOWN", "unknown");
  unknown->findITerm("CLK")->connect(singleOutputNet(a));
  EXPECT_FALSE(seqFanout(a, network_));
  EXPECT_TRUE(findLeafClockBuffers(block_, network_).empty());
}

}  // namespace
}  // namespace wmk
