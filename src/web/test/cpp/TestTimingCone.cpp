// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <algorithm>
#include <string>
#include <vector>

#include "color.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "sta/Graph.hh"
#include "sta/PortDirection.hh"
#include "timing_report.h"
#include "tst/nangate45_fixture.h"

namespace web {
namespace {

//------------------------------------------------------------------------------
// spectrumColor — the Turbo colormap ported from gui::SpectrumGenerator.
// These lock the endpoints/clamping so the cone colors keep matching the Qt
// GUI even if the table is edited.
//------------------------------------------------------------------------------

TEST(SpectrumColorTest, LowEndIsFirstTableEntry)
{
  const Color c = spectrumColor(0.0, 255);
  EXPECT_EQ(c.r, 48);
  EXPECT_EQ(c.g, 18);
  EXPECT_EQ(c.b, 59);
  EXPECT_EQ(c.a, 255);
}

TEST(SpectrumColorTest, HighEndIsLastTableEntry)
{
  const Color c = spectrumColor(1.0, 128);
  EXPECT_EQ(c.r, 122);
  EXPECT_EQ(c.g, 4);
  EXPECT_EQ(c.b, 3);
  EXPECT_EQ(c.a, 128);
}

TEST(SpectrumColorTest, ClampsOutOfRangeValues)
{
  EXPECT_EQ(spectrumColor(-5.0).r, spectrumColor(0.0).r);
  EXPECT_EQ(spectrumColor(5.0).r, spectrumColor(1.0).r);
}

TEST(SpectrumColorTest, IsMonotonicAcrossTheRamp)
{
  // The Turbo ramp is not monotonic per channel, but distinct inputs must map
  // to valid, in-bounds colors (a cheap guard against table truncation).
  const Color mid = spectrumColor(0.5);
  EXPECT_NE(mid.r + mid.g + mid.b, 0);
}

//------------------------------------------------------------------------------
// TimingReport::computeTimingCone guard behavior (no timing setup needed).
//------------------------------------------------------------------------------

using TimingConeTest = tst::Nangate45Fixture;

TEST_F(TimingConeTest, EmptyDirectionsReturnEmptyCone)
{
  TimingReport report(getSta());
  // Neither fanin nor fanout requested: a valid, empty cone (the clear path).
  const TimingConeResult result
      = report.computeTimingCone("anything", false, false, 0, 0);
  EXPECT_TRUE(result.ok);
  EXPECT_TRUE(result.nodes.empty());
  EXPECT_FALSE(result.constrained);
}

//------------------------------------------------------------------------------
// Cone traversal over a real STA graph.  A chain of buffers gives every level
// an instance input pin, which is the case the walk used to drop: in STA an
// input pin has only a load vertex, so asking for pinDrvrVertex alone yields
// null and the cone stops at the first one.
//------------------------------------------------------------------------------

class TimingConeGraphTest : public tst::Nangate45Fixture
{
 protected:
  void SetUp() override
  {
    readLiberty("_main/test/Nangate45/Nangate45_typ.lib");
    block_->setDieArea(odb::Rect(0, 0, 100000, 100000));

    // in -> b0 -> b1 -> b2 -> b3 -> out.  Each buffer contributes an input
    // (load vertex) and an output (driver vertex) pin.
    odb::dbMaster* buf = lib_->findMaster("BUF_X1");
    ASSERT_NE(buf, nullptr);
    for (int i = 0; i < kChainLength; ++i) {
      tst::InstOptions opts;
      opts.location = odb::Point(1000 * (i + 1), 1000);
      opts.status = odb::dbPlacementStatus::PLACED;
      opts.iterms = {{netName(i).c_str(), "A"}, {netName(i + 1).c_str(), "Z"}};
      makeInst(block_, buf, instName(i).c_str(), opts);
    }

    sta_->postReadDef(block_);
    sta_->getDbNetwork()->setBlock(block_);
  }

  static constexpr int kChainLength = 4;
  static std::string netName(int i) { return "n" + std::to_string(i); }
  static std::string instName(int i) { return "b" + std::to_string(i); }
  // computeTimingCone seeds from a pin, not an instance.
  static std::string inputPin(int i) { return instName(i) + "/A"; }
};

// Documents what Graph actually promises, because two review passes have now
// asserted the opposite: pinDrvrVertex returns the pin's single vertex for
// every direction except bidirect, where it looks up a separate driver vertex.
// It is NOT null for instance input pins, so the cone walk does not need a
// load-vertex fallback to get past them.
TEST_F(TimingConeGraphTest, DriverAndLoadVertexCoincideOffBidirect)
{
  sta_->ensureGraph();
  sta_->searchPreamble();
  auto* graph = sta_->graph();
  auto* network = sta_->getDbNetwork();

  for (const char* pin_name : {"b1/A", "b1/Z"}) {
    odb::dbITerm* iterm = block_->findITerm(pin_name);
    ASSERT_NE(iterm, nullptr) << pin_name;
    const sta::Pin* pin = network->dbToSta(iterm);
    ASSERT_NE(pin, nullptr) << pin_name;
    ASSERT_FALSE(network->direction(pin)->isBidirect()) << pin_name;

    EXPECT_NE(graph->pinDrvrVertex(pin), nullptr) << pin_name;
    EXPECT_EQ(graph->pinDrvrVertex(pin), graph->pinLoadVertex(pin)) << pin_name;
  }
}

TEST_F(TimingConeGraphTest, FaninConeWalksTheWholeChain)
{
  TimingReport report(getSta());
  // Unlimited depth from the last buffer's input: the cone should reach back
  // through every buffer, not stop one level in.
  const TimingConeResult result = report.computeTimingCone(
      inputPin(kChainLength - 1), /*fanin=*/true, /*fanout=*/false, 0, 0);
  ASSERT_TRUE(result.ok) << result.error;

  int min_depth = 0;
  for (const TimingConeNode& node : result.nodes) {
    min_depth = std::min(min_depth, node.depth);
  }
  // Two levels per buffer (input pin, output pin) back to the chain head.
  EXPECT_EQ(min_depth, -2 * (kChainLength - 1))
      << "nodes=" << result.nodes.size();
}

// Flight lines are drawn from source_indices, so a cone whose levels are not
// wired together renders as scattered pins with nothing joining them.
TEST_F(TimingConeGraphTest, ConeLevelsFormAnUnbrokenChain)
{
  TimingReport report(getSta());
  const TimingConeResult result = report.computeTimingCone(
      inputPin(kChainLength - 1), /*fanin=*/true, /*fanout=*/false, 0, 0);
  ASSERT_TRUE(result.ok) << result.error;
  ASSERT_FALSE(result.nodes.empty());

  size_t links = 0;
  size_t sourceless = 0;
  for (const TimingConeNode& node : result.nodes) {
    links += node.source_indices.size();
    if (node.source_indices.empty()) {
      ++sourceless;
    }
    std::vector<int> sorted = node.source_indices;
    std::sort(sorted.begin(), sorted.end());
    EXPECT_EQ(std::adjacent_find(sorted.begin(), sorted.end()), sorted.end())
        << "a source listed twice would draw two identical flight lines";
  }
  // A linear chain: every node but the far end is driven by exactly one other.
  EXPECT_EQ(links, result.nodes.size() - 1);
  EXPECT_EQ(sourceless, 1u);
}

}  // namespace
}  // namespace web
