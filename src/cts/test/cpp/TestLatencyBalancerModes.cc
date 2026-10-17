// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <algorithm>
#include <limits>
#include <optional>
#include <string>
#include <vector>

#include "CtsOptions.h"
#include "cts/TritonCTS.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "est/EstimateParasitics.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "sta/Clock.hh"
#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "sta/Mode.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "sta/Scene.hh"
#include "sta/Sdc.hh"
#include "sta/SdcClass.hh"
#include "sta/StringUtil.hh"
#include "sta/Transition.hh"
#include "sta/Units.hh"
#include "tst/IntegratedFixture.h"

namespace cts {

// clk drives flops d_* directly and flops g_* through clock gate cg.
class LatencyBalancerModesTest : public tst::IntegratedFixture
{
 protected:
  LatencyBalancerModesTest()
      : tst::IntegratedFixture(tst::Technology::kNangate45,
                               "_main/src/cts/test/")
  {
  }

  void SetUp() override
  {
    odb::dbTech* tech = db_->getTech();
    odb::dbChip* chip = odb::dbChip::create(db_.get(), tech);
    block_ = odb::dbBlock::create(chip, "top");
    block_->setDefUnits(tech->getDbUnitsPerMicron());
    block_->setDieArea(odb::Rect(0, 0, 800000, 800000));
    sta_->postReadDef(block_);

    makeBTerm(block_,
              "clk",
              {.sig_type = odb::dbSigType::CLOCK,
               .bpins = {{"metal6",
                          odb::Rect(400000, 799000, 401000, 800000),
                          odb::dbPlacementStatus::FIRM}}});
    makeBTerm(block_,
              "en",
              {.bpins = {{"metal6",
                          odb::Rect(0, 400000, 1000, 401000),
                          odb::dbPlacementStatus::FIRM}}});
    makeInst(block_,
             db_->findMaster("AND2_X1"),
             "cg",
             {.location = {400000, 400000},
              .status = odb::dbPlacementStatus::PLACED,
              .iterms = {{"clk", "A1"}, {"en", "A2"}, {"gclk", "ZN"}}});
    odb::dbMaster* ff = db_->findMaster("DFF_X1");
    for (int i = 0; i < kFlopsPerBranch; i++) {
      const int x = 50000 + i * 90000;
      makeInst(block_,
               ff,
               flopName("d", i).c_str(),
               {.location = {x, 600000},
                .status = odb::dbPlacementStatus::PLACED,
                .iterms = {{"clk", "CK"}}});
      makeInst(block_,
               ff,
               flopName("g", i).c_str(),
               {.location = {x, 200000},
                .status = odb::dbPlacementStatus::PLACED,
                .iterms = {{"gclk", "CK"}}});
    }
    block_->findNet("clk")->setSigType(odb::dbSigType::CLOCK);
    block_->findNet("gclk")->setSigType(odb::dbSigType::CLOCK);
  }

  static std::string flopName(const char* branch, int index)
  {
    return std::string(branch) + "_" + std::to_string(index);
  }

  float ns(float value) const
  {
    return sta_->units()->timeUnit()->userToSta(value);
  }

  // Clock clk in a new mode, plus a scene of the same name.
  sta::Clock* makeModeClock(const char* mode_name, float period, float rise)
  {
    sta_->setCmdMode(mode_name);
    sta::Mode* mode = sta_->findMode(mode_name);
    sta::PinSet pins(db_network_);
    pins.insert(db_network_->dbToSta(block_->findBTerm("clk")));
    const sta::FloatSeq waveform{ns(rise), ns(rise + period / 2)};
    sta_->makeClock("clk", pins, false, ns(period), waveform, "", mode);
    const sta::StringSeq libs{db_network_->defaultLibertyLibrary()->name()};
    sta_->makeScene(mode_name, mode_name, libs, libs, "", "");
    return mode->sdc()->findClock("clk");
  }

  void setSourceLatency(const char* mode_name, sta::Clock* clk, float latency)
  {
    sta_->setClockInsertion(clk,
                            nullptr,
                            sta::RiseFallBoth::riseFall(),
                            sta::MinMaxAll::all(),
                            sta::EarlyLateAll::all(),
                            ns(latency),
                            sta_->findMode(mode_name)->sdc());
  }

  // Turns the clock gate off, so the mode does not clock the g_* flops.
  void gateOff(const char* mode_name)
  {
    sta_->setCaseAnalysis(db_network_->dbToSta(block_->findBTerm("en")),
                          sta::LogicValue::zero,
                          sta_->findMode(mode_name));
  }

  // Wire RC of metal3 (signal) and metal5 (clock) for every scene.
  void setWireRC()
  {
    odb::dbTech* tech = db_->getTech();
    for (const sta::Scene* scene : sta_->scenes()) {
      ep_.setHWireSignalRC(tech, scene, 3.574e6, 7.516e-11);
      ep_.setVWireSignalRC(tech, scene, 3.574e6, 7.516e-11);
      ep_.setHWireClkRC(tech, scene, 1.502e6, 8.394e-11);
      ep_.setVWireClkRC(tech, scene, 1.502e6, 8.394e-11);
    }
  }

  // Runs CTS with the options clock_tree_synthesis applies by default.
  void runCts()
  {
    setWireRC();
    TritonCTS cts(
        &logger_, db_.get(), db_network_, sta_.get(), &stt_, &resizer_, &ep_);
    CtsOptions* options = cts.getParms();
    options->setSinkClustering(false);
    cts.setBufferList("CLKBUF_X3");
    options->setWireSegmentUnit(20);
    cts.setRootBuffer("CLKBUF_X3");
    cts.setSinkBuffer("");
    options->enableDummyLoad(true);
    options->setRepairClockNets(false);
    options->enableInsertionDelay(true);
    cts.runTritonCts();

    ep_.estimateParasitics(est::ParasiticsSrc::kPlacement);
    sta_->updateTiming(false);
  }

  int delayBufferCount() const
  {
    int count = 0;
    for (odb::dbInst* inst : block_->getInsts()) {
      if (inst->getName().starts_with("delaybuf_")) {
        count++;
      }
    }
    return count;
  }

  // Latest rising clock arrival at a flop, if the scene clocks it.
  std::optional<float> clockArrival(const std::string& flop,
                                    const sta::Scene* scene) const
  {
    odb::dbITerm* ck = block_->findInst(flop.c_str())->findITerm("CK");
    sta::Vertex* vertex
        = sta_->graph()->pinLoadVertex(db_network_->dbToSta(ck));
    std::optional<float> arrival;
    sta::VertexPathIterator iter(
        vertex, scene, sta::MinMax::max(), sta::RiseFall::rise(), sta_.get());
    while (iter.hasNext()) {
      const sta::Path* path = iter.next();
      const sta::ClockEdge* edge = path->clkEdge(sta_.get());
      if (edge != nullptr && edge->transition() == sta::RiseFall::rise()) {
        arrival = std::max(arrival.value_or(-std::numeric_limits<float>::max()),
                           sta::delayAsFloat(path->arrival()));
      }
    }
    return arrival;
  }

  // Clock arrival spread over the flops the scene clocks.
  float clockSkew(const char* scene_name) const
  {
    const sta::Scene* scene = sta_->findScene(scene_name);
    float earliest = std::numeric_limits<float>::max();
    float latest = -std::numeric_limits<float>::max();
    for (const char* branch : {"d", "g"}) {
      for (int i = 0; i < kFlopsPerBranch; i++) {
        const std::optional<float> arrival
            = clockArrival(flopName(branch, i), scene);
        if (arrival) {
          earliest = std::min(earliest, *arrival);
          latest = std::max(latest, *arrival);
        }
      }
    }
    return latest - earliest;
  }

  static constexpr int kFlopsPerBranch = 8;
  // Unbalanced, the gated branch is about 57ps late.
  static constexpr float kBalancedSkew = 20e-12;
};

// func's source latency must not make the test-only g_* flops look early.
TEST_F(LatencyBalancerModesTest, SourceLatencyOfOneModeKeepsTreeBalanced)
{
  sta::Clock* func_clk = makeModeClock("func", 10.0, 0.0);
  setSourceLatency("func", func_clk, 1.0);
  gateOff("func");
  makeModeClock("test", 20.0, 0.0);
  sta_->setCmdMode("func");

  runCts();

  EXPECT_GT(delayBufferCount(), 0);
  EXPECT_LT(clockSkew("test"), kBalancedSkew)
      << delayBufferCount() << " delay buffers inserted";
}

// Same, with func's clock edge at 2ns instead of a source latency.
TEST_F(LatencyBalancerModesTest, ClockEdgeOfOneModeKeepsTreeBalanced)
{
  makeModeClock("func", 10.0, 2.0);
  gateOff("func");
  makeModeClock("test", 20.0, 0.0);
  sta_->setCmdMode("func");

  runCts();

  EXPECT_GT(delayBufferCount(), 0);
  EXPECT_LT(clockSkew("test"), kBalancedSkew)
      << delayBufferCount() << " delay buffers inserted";
}

// Flops that no mode clocks get no delay buffers.
TEST_F(LatencyBalancerModesTest, UnclockedFlopsAreNotPadded)
{
  makeModeClock("func", 10.0, 0.0);
  gateOff("func");

  runCts();

  EXPECT_EQ(delayBufferCount(), 0);
}

}  // namespace cts
