// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <cstdlib>
#include <functional>
#include <string>
#include <vector>

#include "CtsOptions.h"
#include "cts/TritonCTS.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "tst/IntegratedFixture.h"

namespace cts {

// clock_gate_sinks.v: 48 flops on clk, plus CLKGATE_X1 gates icg_close
// (4 close flops), icg_spread (4 flops 100 um apart), icg_many (40 close
// flops), icg_one (1 flop) and icg_buf (a buffer driving 2 flops).
class ClockGateSinkTest : public tst::IntegratedFixture
{
 protected:
  ClockGateSinkTest()
      : tst::IntegratedFixture(tst::Technology::kNangate45,
                               "_main/src/cts/test/")
  {
  }

  void SetUp() override { build(); }

  // extra_flops more flops on clk push the tree past the sink count that
  // turns on sink clustering. edit runs on the placed design before CTS.
  void build(int extra_flops = 0,
             const std::function<void(TritonCTS&)>& edit = {})
  {
    readVerilogAndSetup("clock_gate_sinks.v", /*init_default_sdc=*/false);
    block_->setDieArea(odb::Rect(0, 0, um(400), um(400)));
    block_->setCoreArea(odb::Rect(um(10), um(10), um(390), um(390)));
    for (int i = 0; i < 48; i++) {
      place("ff" + std::to_string(i), 20 + (i % 8) * 40, 20 + (i / 8) * 40);
    }
    place("icg_close", 100, 300);
    for (int j = 0; j < 4; j++) {
      place("close" + std::to_string(j), 102 + j * 2, 302);
    }
    place("icg_spread", 200, 300);
    for (int j = 0; j < 4; j++) {
      place("spread" + std::to_string(j), 30 + j * 100, 360);
    }
    place("icg_many", 260, 240);
    for (int j = 0; j < 40; j++) {
      place("many" + std::to_string(j), 262 + (j % 8) * 2, 242 + (j / 8) * 2);
    }
    place("icg_one", 340, 100);
    place("one0", 342, 102);
    place("icg_buf", 60, 300);
    place("cbuf", 62, 302);
    place("buffered0", 64, 304);
    place("buffered1", 66, 304);
    odb::dbMaster* dff = db_->findMaster("DFF_X1");
    for (int i = 0; i < extra_flops; i++) {
      const std::string name = "xff" + std::to_string(i);
      odb::dbInst* ff = odb::dbInst::create(block_, dff, name.c_str());
      ff->findITerm("CK")->connect(block_->findNet("clk"));
      ff->findITerm("D")->connect(block_->findNet("d"));
      place(name, 20 + (i % 20) * 18, 30 + (i / 20) * 25);
    }

    // Nangate45.rc metal2
    ep_.setHWireSignalRC(nullptr, sta_->cmdScene(), 3.574e6, 8.081e-11);
    ep_.setVWireSignalRC(nullptr, sta_->cmdScene(), 3.574e6, 8.081e-11);
    ep_.setHWireClkRC(nullptr, sta_->cmdScene(), 3.574e6, 8.081e-11);
    ep_.setVWireClkRC(nullptr, sta_->cmdScene(), 3.574e6, 8.081e-11);
    initStaDefaultSdc();
    runCts(edit);
  }

  void runCts(const std::function<void(TritonCTS&)>& edit = {})
  {
    TritonCTS cts(
        &logger_, db_.get(), db_network_, sta_.get(), &stt_, &resizer_, &ep_);
    cts.setBufferList("CLKBUF_X3");
    cts.setRootBuffer("CLKBUF_X3");
    cts.setSinkBuffer("");
    cts.getParms()->setWireSegmentUnit(20);
    cts.getParms()->setSinkClustering(true);
    cts.getParms()->setSimpleSegmentsEnabled(true);
    cts.getParms()->setBufferDistance(um(100));
    if (edit) {
      edit(cts);
    }
    cts.runTritonCts();
  }

  int um(double microns) const { return block_->micronsToDbu(microns); }

  void place(const std::string& name, double x, double y)
  {
    odb::dbInst* inst = block_->findInst(name.c_str());
    ASSERT_NE(inst, nullptr) << name;
    inst->setLocation(um(x), um(y));
    inst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
  }

  // The gate and the clones split from it.
  std::vector<odb::dbInst*> copiesOf(const std::string& gate)
  {
    std::vector<odb::dbInst*> copies;
    for (odb::dbInst* inst : block_->getInsts()) {
      const std::string name = inst->getName();
      if (name == gate || name.starts_with("splitgate_" + gate + "_")) {
        copies.push_back(inst);
      }
    }
    return copies;
  }

  static bool isFlop(odb::dbITerm* iterm)
  {
    return iterm->getInst()->getMaster()->getName() == "DFF_X1";
  }

  // A clock gate sink drives flops only and shares its clock net with flops.
  static bool isSinkOfFlopTree(odb::dbInst* gate)
  {
    bool flops_on_clock = false;
    for (odb::dbITerm* iterm : gate->findITerm("CK")->getNet()->getITerms()) {
      flops_on_clock |= isFlop(iterm);
    }
    for (odb::dbITerm* iterm : gate->findITerm("GCK")->getNet()->getITerms()) {
      if (iterm->isInputSignal() && !isFlop(iterm)) {
        return false;
      }
    }
    return flops_on_clock;
  }

  static int loads(odb::dbInst* gate)
  {
    return gate->findITerm("GCK")->getNet()->getITermCount() - 1;
  }

  std::string clockDriver(const std::string& inst, const char* pin)
  {
    odb::dbNet* net = block_->findInst(inst.c_str())->findITerm(pin)->getNet();
    return net->getFirstOutput()->getInst()->getName();
  }
};

class ClockGateSinkClusteredTest : public ClockGateSinkTest
{
 protected:
  void SetUp() override { build(200); }
};

// Each test builds its own variant of the design.
class ClockGateSinkVariantTest : public ClockGateSinkTest
{
 protected:
  void SetUp() override {}

  odb::dbInst* inst(const char* name) { return block_->findInst(name); }

  // The error CTS stops with, or "" when it completes.
  std::string buildError(const std::function<void(TritonCTS&)>& edit)
  {
    try {
      build(0, edit);
    } catch (const std::exception& e) {
      return e.what();
    }
    return "";
  }
};

TEST_F(ClockGateSinkTest, GateWithCloseFlopsIsSink)
{
  const std::vector<odb::dbInst*> copies = copiesOf("icg_close");
  ASSERT_EQ(copies.size(), 1);
  EXPECT_TRUE(isSinkOfFlopTree(copies[0]));
  EXPECT_EQ(loads(copies[0]), 4);
}

TEST_F(ClockGateSinkTest, GateWithSpreadFlopsIsSplitIntoSinks)
{
  const std::vector<odb::dbInst*> copies = copiesOf("icg_spread");
  ASSERT_EQ(copies.size(), 4);
  for (odb::dbInst* copy : copies) {
    EXPECT_TRUE(isSinkOfFlopTree(copy)) << copy->getName();
    EXPECT_EQ(loads(copy), 1) << copy->getName();
    EXPECT_EQ(copy->findITerm("E")->getNet()->getName(), "en");
  }
}

TEST_F(ClockGateSinkTest, GateWithTooManyFlopsIsSplitIntoSinks)
{
  const std::vector<odb::dbInst*> copies = copiesOf("icg_many");
  ASSERT_EQ(copies.size(), 2);
  for (odb::dbInst* copy : copies) {
    EXPECT_TRUE(isSinkOfFlopTree(copy)) << copy->getName();
    EXPECT_EQ(loads(copy), 20) << copy->getName();
  }
}

TEST_F(ClockGateSinkTest, GateDrivingABufferKeepsItsTree)
{
  const std::vector<odb::dbInst*> copies = copiesOf("icg_buf");
  ASSERT_EQ(copies.size(), 1);
  EXPECT_FALSE(isSinkOfFlopTree(copies[0]));
  EXPECT_EQ(copies[0]->getMaster()->getName(), "CLKGATE_X1");
}

TEST_F(ClockGateSinkTest, GateSinksAreResizedByLoad)
{
  EXPECT_EQ(copiesOf("icg_one")[0]->getMaster()->getName(), "CLKGATE_X1");
  for (odb::dbInst* copy : copiesOf("icg_many")) {
    EXPECT_NE(copy->getMaster()->getName(), "CLKGATE_X1") << copy->getName();
  }
}

TEST_F(ClockGateSinkClusteredTest, GateSinkSkipsLeafClustering)
{
  EXPECT_TRUE(clockDriver("xff0", "CK").starts_with("clkbuf_leaf_"));
  const std::vector<odb::dbInst*> copies = copiesOf("icg_close");
  ASSERT_EQ(copies.size(), 1);
  EXPECT_FALSE(clockDriver("icg_close", "CK").starts_with("clkbuf_leaf_"));
}

// Gate cloning leaves a gate with one flop where it is.
TEST_F(ClockGateSinkVariantTest, GateFarFromItsFlopMovesToIt)
{
  build(0, [this](TritonCTS&) { place("icg_one", 40, 200); });
  EXPECT_TRUE(isSinkOfFlopTree(inst("icg_one")));
  int x, y;
  inst("icg_one")->getLocation(x, y);
  EXPECT_LT(std::abs(x - um(342)) + std::abs(y - um(102)), um(50));
}

// Gate cloning moves a movable gate to its flops; a fixed one stays put.
TEST_F(ClockGateSinkVariantTest, FixedGateFarFromItsFlopsKeepsItsTree)
{
  build(0, [this](TritonCTS&) {
    place("icg_close", 300, 40);
    inst("icg_close")->setPlacementStatus(odb::dbPlacementStatus::FIRM);
  });
  EXPECT_TRUE(clockDriver("close0", "CK").starts_with("clkbuf_"));
}

// CTS refuses to rewire a dont_touch flop; the split must not try first.
TEST_F(ClockGateSinkVariantTest, DontTouchFlopIsNotSplitOff)
{
  EXPECT_EQ(
      buildError([this](TritonCTS&) { inst("spread0")->setDoNotTouch(true); }),
      "CTS-0137");
  EXPECT_EQ(copiesOf("icg_spread").size(), 1);
}

// CTS refuses a dont_touch gate on its clock net; resizing must not try first.
TEST_F(ClockGateSinkVariantTest, DontTouchGateSinkIsNotResized)
{
  // A cluster size above 40 makes icg_many a sink without a split.
  EXPECT_EQ(buildError([this](TritonCTS& cts) {
              cts.getParms()->setSinkClusteringSize(64);
              inst("icg_many")->setDoNotTouch(true);
            }),
            "CTS-0137");
  EXPECT_EQ(inst("icg_many")->getMaster()->getName(), "CLKGATE_X1");
}

TEST_F(ClockGateSinkTest, SecondRunKeepsGateSinks)
{
  runCts();
  EXPECT_EQ(copiesOf("icg_spread").size(), 4);
  EXPECT_EQ(copiesOf("icg_many").size(), 2);
  EXPECT_TRUE(isSinkOfFlopTree(copiesOf("icg_close")[0]));
}

TEST_F(ClockGateSinkVariantTest, SinkClusteringOffKeepsGates)
{
  build(0, [](TritonCTS& cts) { cts.getParms()->setSinkClustering(false); });
  EXPECT_EQ(copiesOf("icg_spread").size(), 1);
  EXPECT_EQ(copiesOf("icg_many").size(), 1);
}

TEST_F(ClockGateSinkVariantTest, GateDrivingAClockRootKeepsItsTree)
{
  build(0, [](TritonCTS& cts) { cts.setClockNets("clk gck_many"); });
  const std::vector<odb::dbInst*> copies = copiesOf("icg_many");
  ASSERT_EQ(copies.size(), 1);
  EXPECT_FALSE(isSinkOfFlopTree(copies[0]));
}

}  // namespace cts
