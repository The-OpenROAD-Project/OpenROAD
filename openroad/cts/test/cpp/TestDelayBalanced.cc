// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <algorithm>
#include <exception>
#include <functional>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "CtsOptions.h"
#include "cts/TritonCTS.h"
#include "est/EstimateParasitics.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "sta/Clock.hh"
#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "sta/MinMax.hh"
#include "sta/Path.hh"
#include "sta/Sdc.hh"
#include "sta/SdcClass.hh"
#include "sta/Transition.hh"
#include "tst/IntegratedFixture.h"

namespace cts {

// clock_gate_sinks.v with its registers unevenly spread: the 48 flops on clk
// crowded into one corner and 24 more over the rest of the die, a blockage
// between them, and CLKGATE_X1 gates icg_close (4 close flops), icg_spread
// (4 flops 100 um apart), icg_many (40 close flops), icg_one (1 flop), and
// icg_buf (a buffer driving 2 flops).
class DelayBalancedTest : public tst::IntegratedFixture
{
 protected:
  static constexpr double kLatency = 500e-12;
  static constexpr double kSlewLimit = 80e-12;
  static constexpr int kFarFlops = 24;
  static constexpr double kPortLoad = 100e-15;

  DelayBalancedTest()
      : tst::IntegratedFixture(tst::Technology::kNangate45,
                               "_main/openroad/cts/test/")
  {
  }

  void SetUp() override { build(kLatency); }

  // A latency of 0 sets none. edit runs on the placed design before CTS.
  void build(double latency,
             double slew_limit = kSlewLimit,
             const std::function<void(TritonCTS&)>& edit = {})
  {
    readVerilogAndSetup(
        "clock_gate_sinks.v", /*init_default_sdc=*/false, hierarchy_);
    block_->setDieArea(odb::Rect(0, 0, um(400), um(400)));
    block_->setCoreArea(odb::Rect(um(10), um(10), um(390), um(390)));
    makeRows();
    for (int i = 0; i < 48; i++) {
      place("ff" + std::to_string(i), 20 + (i % 8) * 4, 20 + (i / 8) * 3);
    }
    odb::dbMaster* dff = db_->findMaster("DFF_X1");
    for (int i = 0; i < kFarFlops; i++) {
      const std::string name = "far" + std::to_string(i);
      odb::dbInst* ff = odb::dbInst::create(block_, dff, name.c_str());
      ff->findITerm("CK")->connect(block_->findNet("clk"));
      ff->findITerm("D")->connect(block_->findNet("d"));
      place(name, 110 + (i % 6) * 50, 40 + (i / 6) * 70);
    }
    place("icg_close", 100, 300);
    for (int j = 0; j < 4; j++) {
      place("close" + std::to_string(j), 102 + j * 2, 302);
    }
    place("icg_spread", 200, 300);
    for (int j = 0; j < 4; j++) {
      place("spread" + std::to_string(j), 30 + j * 100, 360);
    }
    place("icg_many", 300, 300);
    for (int j = 0; j < 40; j++) {
      place("many" + std::to_string(j), 302 + (j % 8) * 4, 303 + (j / 8) * 3);
    }
    place("icg_one", 340, 100);
    place("one0", 342, 102);
    place("icg_buf", 60, 300);
    place("cbuf", 62, 302);
    place("buffered0", 64, 304);
    place("buffered1", 66, 304);
    odb::dbBlockage::create(block_, um(150), um(100), um(250), um(200));

    // Nangate45.rc metal2
    ep_.setHWireSignalRC(nullptr, sta_->cmdScene(), 3.574e6, 8.081e-11);
    ep_.setVWireSignalRC(nullptr, sta_->cmdScene(), 3.574e6, 8.081e-11);
    ep_.setHWireClkRC(nullptr, sta_->cmdScene(), 3.574e6, 8.081e-11);
    ep_.setVWireClkRC(nullptr, sta_->cmdScene(), 3.574e6, 8.081e-11);
    initStaDefaultSdc();
    sta::Clock* clk = sta_->cmdSdc()->findClock("clk");
    if (latency > 0.0) {
      sta_->setClockLatency(clk,
                            nullptr,
                            sta::RiseFallBoth::riseFall(),
                            sta::MinMaxAll::all(),
                            latency,
                            sta_->cmdSdc());
    }
    sta_->setSlewLimit(clk,
                       sta::RiseFallBoth::riseFall(),
                       sta::PathClkOrData::clk,
                       sta::MinMax::max(),
                       slew_limit,
                       sta_->cmdSdc());

    TritonCTS cts(
        &logger_, db_.get(), db_network_, sta_.get(), &stt_, &resizer_, &ep_);
    cts.setBufferList("CLKBUF_X1 CLKBUF_X2 CLKBUF_X3");
    cts.setRootBuffer("CLKBUF_X3");
    cts.setSinkBuffer("");
    cts.getParms()->setSinkClustering(true);
    cts.getParms()->setDelayBalanced(true);
    cts.getParms()->setApplyNDR(CtsOptions::NdrStrategy::NONE);
    if (edit) {
      edit(cts);
    }
    cts.runTritonCts();
    ep_.estimateParasitics(est::ParasiticsSrc::kPlacement);
  }

  // The error CTS stops with, or "" when it completes.
  std::string buildError(double slew_limit,
                         const std::function<void(TritonCTS&)>& edit = {})
  {
    try {
      build(kLatency, slew_limit, edit);
    } catch (const std::exception& e) {
      return e.what();
    }
    return "";
  }

  int um(double microns) const { return block_->micronsToDbu(microns); }

  // Puts a buffer between the clock's port and its net, the port far from
  // the buffer, and a loaded output port on the net far from both.
  void addRootBufferAndPorts()
  {
    odb::dbNet* clk = block_->findNet("clk");
    odb::dbNet* root = odb::dbNet::create(block_, "clk_root");
    std::vector<odb::dbITerm*> loads;
    for (odb::dbITerm* load : clk->getITerms()) {
      loads.push_back(load);
    }
    for (odb::dbITerm* load : loads) {
      load->disconnect();
      load->connect(root);
    }
    odb::dbInst* buffer
        = odb::dbInst::create(block_, db_->findMaster("CLKBUF_X3"), "root_buf");
    buffer->findITerm("A")->connect(clk);
    buffer->findITerm("Z")->connect(root);
    place("root_buf", 200, 20);
    odb::dbBTerm* out = odb::dbBTerm::create(root, "clk_out");
    out->setIoType(odb::dbIoType::OUTPUT);
    placePort(clk->get1stBTerm(), 380, 380);
    placePort(out, 20, 380);
    sta_->setPortExtPinCap(db_network_->port(db_network_->dbToSta(out)),
                           sta::RiseFallBoth::riseFall(),
                           sta::MinMaxAll::all(),
                           kPortLoad,
                           sta_->cmdSdc());
  }

  void placePort(odb::dbBTerm* port, double x, double y)
  {
    odb::dbBPin* pin = odb::dbBPin::create(port);
    odb::dbBox::create(pin,
                       block_->getTech()->findRoutingLayer(6),
                       um(x),
                       um(y),
                       um(x + 0.5),
                       um(y + 0.5));
    pin->setPlacementStatus(odb::dbPlacementStatus::FIRM);
  }

  void makeRows()
  {
    odb::dbSite* site = db_->findMaster("DFF_X1")->getSite();
    const odb::Rect core = block_->getCoreArea();
    for (int row = 0; row < core.dy() / site->getHeight(); row++) {
      odb::dbRow::create(
          block_,
          ("row" + std::to_string(row)).c_str(),
          site,
          core.xMin(),
          core.yMin() + row * site->getHeight(),
          row % 2 == 0 ? odb::dbOrientType::R0 : odb::dbOrientType::MX,
          odb::dbRowDir::HORIZONTAL,
          core.dx() / site->getWidth(),
          site->getWidth());
    }
  }

  void place(const std::string& name, double x, double y)
  {
    odb::dbInst* inst = block_->findInst(name.c_str());
    ASSERT_NE(inst, nullptr) << name;
    inst->setLocation(um(x), um(y));
    inst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
  }

  odb::dbInst* inst(const std::string& name)
  {
    return block_->findInst(name.c_str());
  }

  static bool isFlop(odb::dbInst* inst)
  {
    return inst->getMaster()->getName() == "DFF_X1";
  }

  // The latest arrival of the clock's rising edge at the pin.
  double clockArrival(odb::dbITerm* pin)
  {
    sta::Vertex* vertex
        = sta_->graph()->pinLoadVertex(db_network_->dbToSta(pin));
    sta::VertexPathIterator paths(vertex,
                                  sta_->cmdScene(),
                                  sta::MinMax::max(),
                                  sta::RiseFall::rise(),
                                  sta_.get());
    double arrival = -1.0;
    while (paths.hasNext()) {
      const sta::Path* path = paths.next();
      if (path->isClock(sta_.get())) {
        arrival = std::max<double>(arrival, sta::delayAsFloat(path->arrival()));
      }
    }
    return arrival;
  }

  // The earliest and the latest clock arrival at the registers.
  std::pair<double, double> registerArrivals()
  {
    sta_->ensureClkArrivals();
    double earliest = std::numeric_limits<double>::max();
    double latest = std::numeric_limits<double>::lowest();
    for (odb::dbInst* inst : block_->getInsts()) {
      if (isFlop(inst)) {
        const double arrival = clockArrival(inst->findITerm("CK"));
        EXPECT_GE(arrival, 0.0) << inst->getName();
        earliest = std::min(earliest, arrival);
        latest = std::max(latest, arrival);
      }
    }
    return {earliest, latest};
  }

  std::string clockDriver(const std::string& name)
  {
    return driver(inst(name)->findITerm("CK")->getNet());
  }

  // The instance that drives the net, or "" when a port does.
  static std::string driver(odb::dbNet* net)
  {
    odb::dbITerm* output = net->getFirstOutput();
    return output != nullptr ? output->getInst()->getName() : "";
  }

  // The pin the clock comes in by.
  static odb::dbITerm* clockInput(odb::dbInst* inst)
  {
    if (isTreeBuffer(inst)) {
      return inst->findITerm("A");
    }
    return inst->findITerm("CK");
  }

  bool hierarchy_ = true;

  static bool isTreeBuffer(odb::dbInst* inst)
  {
    return inst->getName().starts_with("clkbuf_");
  }
};

// Each test builds its own variant of the design.
class DelayBalancedVariantTest : public DelayBalancedTest
{
 protected:
  void SetUp() override {}
};

TEST_F(DelayBalancedTest, RegistersGetTheClockAtTheLatency)
{
  const auto [earliest, latest] = registerArrivals();
  EXPECT_GT(earliest, kLatency - 15e-12);
  EXPECT_LT(latest, kLatency + 15e-12);
  EXPECT_LT(latest - earliest, 20e-12);
}

TEST_F(DelayBalancedVariantTest, RegistersGetTheClockEarlyWithoutALatency)
{
  build(0.0);
  const auto [earliest, latest] = registerArrivals();
  EXPECT_LT(latest, kLatency - 100e-12);
  EXPECT_LT(latest - earliest, 20e-12);
}

// The registers, the tree's buffers, and the clock gates, including the
// registers on a net that its gate was left to drive.
TEST_F(DelayBalancedTest, ClockPinsHoldTheSlewLimit)
{
  sta_->checkSlewsPreamble();
  int checked = 0;
  for (odb::dbInst* inst : block_->getInsts()) {
    odb::dbITerm* pin = clockInput(inst);
    if (pin == nullptr) {
      continue;
    }
    sta::Slew slew;
    float limit;
    float slack;
    const sta::RiseFall* rf;
    const sta::Scene* scene;
    sta_->checkSlew(db_network_->dbToSta(pin),
                    sta_->scenes(),
                    sta::MinMax::max(),
                    /*check_clks=*/true,
                    slew,
                    limit,
                    slack,
                    rf,
                    scene);
    EXPECT_LE(sta::delayAsFloat(slew), kSlewLimit) << pin->getName();
    checked++;
  }
  EXPECT_GT(checked, 99 + kFarFlops + 5);
}

// The stock flow clones, moves, and sizes clock gates.
TEST_F(DelayBalancedVariantTest, ClockGatesStayAsPlaced)
{
  const std::vector<std::string> gates
      = {"icg_close", "icg_spread", "icg_many", "icg_one", "icg_buf"};
  std::vector<odb::Point> placed;
  build(kLatency, kSlewLimit, [&](TritonCTS&) {
    for (const std::string& gate : gates) {
      placed.push_back(inst(gate)->getLocation());
    }
  });
  for (size_t i = 0; i < gates.size(); i++) {
    EXPECT_EQ(inst(gates[i])->getLocation(), placed[i]) << gates[i];
    EXPECT_EQ(inst(gates[i])->getMaster()->getName(), "CLKGATE_X1");
  }
  int gate_count = 0;
  for (odb::dbInst* inst : block_->getInsts()) {
    gate_count += inst->getMaster()->getName().starts_with("CLKGATE");
  }
  EXPECT_EQ(gate_count, gates.size());
}

TEST_F(DelayBalancedTest, GateThatDrivesItsRegistersKeepsItsNet)
{
  for (int j = 0; j < 4; j++) {
    EXPECT_EQ(clockDriver("close" + std::to_string(j)), "icg_close");
  }
}

TEST_F(DelayBalancedTest, GateWithFarRegistersGetsATree)
{
  for (int j = 0; j < 4; j++) {
    const std::string flop = "spread" + std::to_string(j);
    EXPECT_TRUE(clockDriver(flop).starts_with("clkbuf_")) << flop;
  }
  EXPECT_TRUE(clockDriver("icg_spread").starts_with("clkbuf_"));
}

TEST_F(DelayBalancedTest, BuffersStandClearOfTheBlockage)
{
  const odb::Rect blockage(um(150), um(100), um(250), um(200));
  int buffers = 0;
  for (odb::dbInst* inst : block_->getInsts()) {
    if (isTreeBuffer(inst)) {
      EXPECT_FALSE(inst->getBBox()->getBox().overlaps(blockage))
          << inst->getName();
      buffers++;
    }
  }
  EXPECT_GT(buffers, 0);
}

// The port's wire would load every register's clock, so buffers lead to it
// from the net's driver, and buffers lead from the clock's port to the driver.
TEST_F(DelayBalancedVariantTest, PortsOfTheClockNetGetTheirOwnBuffers)
{
  hierarchy_ = false;
  build(kLatency, 50e-12, [this](TritonCTS&) { addRootBufferAndPorts(); });
  odb::dbNet* port_net = block_->findBTerm("clk_out")->getNet();
  EXPECT_TRUE(driver(port_net).starts_with("clkbuf_port_"));
  EXPECT_EQ(port_net->getITermCount(), 1);
  EXPECT_TRUE(driver(inst("root_buf")->findITerm("A")->getNet())
                  .starts_with("clkbuf_source_"));
  const auto [earliest, latest] = registerArrivals();
  EXPECT_GT(earliest, kLatency - 15e-12);
  EXPECT_LT(latest, kLatency + 15e-12);
}

// A slow clock port drives only as far as the slew limit allows before the
// first buffer from it.
TEST_F(DelayBalancedVariantTest, ClockPortDrivesWithinTheSlewLimit)
{
  constexpr double kSlewLimit = 50e-12;
  hierarchy_ = false;
  build(kLatency, kSlewLimit, [this](TritonCTS&) {
    addRootBufferAndPorts();
    sta_->setInputSlew(
        db_network_->port(db_network_->dbToSta(block_->findBTerm("clk"))),
        sta::RiseFallBoth::riseFall(),
        sta::MinMaxAll::all(),
        0.98 * kSlewLimit,
        sta_->cmdSdc());
  });
  odb::dbNet* port_net = block_->findBTerm("clk")->getNet();
  ASSERT_EQ(port_net->getITermCount(), 1);
  odb::dbITerm* first = *port_net->getITerms().begin();
  EXPECT_TRUE(first->getInst()->getName().starts_with("clkbuf_source_"));
  sta_->ensureClkArrivals();
  sta::Vertex* vertex
      = sta_->graph()->pinLoadVertex(db_network_->dbToSta(first));
  const double slew
      = sta::delayAsFloat(sta_->slew(vertex,
                                     sta::RiseFallBoth::riseFall(),
                                     sta_->scenes(),
                                     sta::MinMax::max()));
  EXPECT_GT(slew, 0.9 * kSlewLimit);
  EXPECT_LE(slew, kSlewLimit);
}

// A register clock pin with a latency of its own gets the clock that much
// before the clock's latency.
TEST_F(DelayBalancedVariantTest, RegistersWithTheirOwnLatencyGetTheClockThen)
{
  constexpr double kOwn = 400e-12;
  const std::vector<std::string> own = {"ff0", "ff9", "far0", "far13"};
  const auto pin = [&](const std::string& name) {
    return db_network_->dbToSta(inst(name)->findITerm("CK"));
  };
  build(kLatency, kSlewLimit, [&](TritonCTS&) {
    for (const std::string& name : own) {
      sta_->setClockLatency(nullptr,
                            pin(name),
                            sta::RiseFallBoth::riseFall(),
                            sta::MinMaxAll::all(),
                            kOwn,
                            sta_->cmdSdc());
    }
  });
  for (const std::string& name : own) {
    sta_->removeClockLatency(nullptr, pin(name), sta_->cmdSdc());
  }
  sta_->ensureClkArrivals();
  for (const std::string& name : own) {
    EXPECT_NEAR(clockArrival(inst(name)->findITerm("CK")), kOwn, 15e-12)
        << name;
  }
  for (const std::string& name : {"ff1", "far1", "many4", "close0"}) {
    EXPECT_NEAR(clockArrival(inst(name)->findITerm("CK")), kLatency, 15e-12)
        << name;
  }
}

// Only the named net gets a tree; the gates on it still count for the delay
// behind them.
TEST_F(DelayBalancedVariantTest, GatesOnANamedClockNetCarryTheirDelay)
{
  build(kLatency, kSlewLimit, [](TritonCTS& cts) { cts.setClockNets("clk"); });
  sta_->ensureClkArrivals();
  EXPECT_EQ(clockDriver("close0"), "icg_close");
  EXPECT_NEAR(clockArrival(inst("close0")->findITerm("CK")),
              clockArrival(inst("ff0")->findITerm("CK")),
              20e-12);
}

TEST_F(DelayBalancedVariantTest, NonDefaultRulesAreNotApplied)
{
  build(kLatency, kSlewLimit, [](TritonCTS& cts) {
    cts.getParms()->setApplyNDR(CtsOptions::NdrStrategy::FULL);
  });
  for (odb::dbNet* net : block_->getNets()) {
    EXPECT_EQ(net->getNonDefaultRule(), nullptr) << net->getName();
  }
}

TEST_F(DelayBalancedVariantTest, BuffersThatCannotHoldTheSlewLimitAreAnError)
{
  EXPECT_EQ(buildError(1e-12), "CTS-0306");
}

TEST_F(DelayBalancedVariantTest, UnitDelayCalculatorIsAnError)
{
  EXPECT_EQ(buildError(kSlewLimit,
                       [this](TritonCTS&) { sta_->setArcDelayCalc("unit"); }),
            "CTS-0308");
}

}  // namespace cts
