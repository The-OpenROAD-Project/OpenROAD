// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// The part of TritonCTS that builds delay-balanced trees.

#include <algorithm>
#include <cstddef>
#include <memory>
#include <ranges>
#include <string>
#include <unordered_set>
#include <vector>

#include "Clock.h"
#include "ClockDelayModel.h"
#include "CtsOptions.h"
#include "DelayBalancedTreeBuilder.h"
#include "TreeBuilder.h"
#include "cts/TritonCTS.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "est/EstimateParasitics.h"
#include "odb/db.h"
#include "odb/dbObject.h"
#include "odb/dbTypes.h"
#include "sta/Clock.hh"
#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "sta/Mode.hh"
#include "sta/Sdc.hh"
#include "sta/Transition.hh"
#include "utl/Logger.h"

namespace cts {

using utl::CTS;

// Most cells the clock is followed through from one pin.
static constexpr int kMaxClockDepth = 64;

void TritonCTS::buildDelayBalancedTrees()
{
  if (options_->applyUsefulSkew()) {
    logger_->warn(CTS, 307, "-useful_skew is ignored with -delay_balanced.");
  }
  // The slews above the trees are those of the nets as placed.
  estimate_parasitics_->estimateParasitics(est::ParasiticsSrc::kPlacement);
  delay_model_ = std::make_unique<ClockDelayModel>(options_,
                                                   db_,
                                                   openSta_,
                                                   network_,
                                                   estimate_parasitics_,
                                                   resizer_,
                                                   logger_);
  // With -clk_nets the nets of the clock network have not been looked up.
  if (staClockNets_.empty()) {
    for (sta::Mode* mode : openSta_->modes()) {
      const odb::PtrSet<odb::dbNet> nets = openSta_->findClkNets(mode);
      staClockNets_.insert(nets.begin(), nets.end());
    }
  }
  const auto as_delay_builder = [](TreeBuilder* builder) {
    return static_cast<DelayBalancedTreeBuilder*>(builder);
  };
  for (auto& builder : builders_) {
    Clock& clock = builder->getClock();
    delay_builders_[clock.getNetObj()] = as_delay_builder(builder.get());
    odb::dbObject* driver = clock.getDriverPin();
    if (driver == nullptr || driver->getObjectType() != odb::dbITermObj) {
      continue;
    }
    odb::dbITerm* input;
    const int arc = clockArc(static_cast<odb::dbITerm*>(driver), input);
    if (arc >= 0) {
      as_delay_builder(builder.get())->setDriverCell(arc, input);
    }
  }
  std::unordered_set<DelayBalancedTreeBuilder*> built;
  for (auto& builder : std::ranges::reverse_view(builders_)) {
    buildDelayBalancedTree(as_delay_builder(builder.get()), built);
  }
}

void TritonCTS::buildDelayBalancedTree(
    DelayBalancedTreeBuilder* builder,
    std::unordered_set<DelayBalancedTreeBuilder*>& built)
{
  if (!built.insert(builder).second) {
    return;
  }
  std::vector<DelayBalancedTreeBuilder*> behind;
  builder->getClock().forEachSink([&](ClockInst& sink) {
    buildersBehind(sink.getDbInputPin(), behind, 0);
  });
  for (DelayBalancedTreeBuilder* other : behind) {
    buildDelayBalancedTree(other, built);
  }
  builder->setTechChar(*techChar_);
  builder->setDb(db_);
  builder->setLogger(logger_);
  builder->initBlockages();
  builder->setDelayModel(delay_model_.get());
  setSinkModels(builder);
  if (builder->getParent() == nullptr) {
    setClockSource(builder);
    setLatencyTarget(builder);
  }
  for (odb::dbBTerm* port : builder->getClock().getNetObj()->getBTerms()) {
    int x, y;
    if (port->getIoType() != odb::dbIoType::INPUT
        && port->getFirstPinLocation(x, y)) {
      builder->addPortLoad({
          .x = x,
          .y = y,
          .cap = delay_model_->portCap(port),
          .layer = ClockDelayModel::pinLayer(port),
      });
    }
  }
  builder->allowPortBuffer(!network_->hasHierarchy());
  builder->run();
}

void TritonCTS::setSinkModels(DelayBalancedTreeBuilder* builder)
{
  const sta::Sdc* sdc = openSta_->cmdMode()->sdc();
  const sta::Clock* clock = sdc->findClock(builder->getClock().getSdcName());
  float target = 0.0;
  bool has_target = false;
  if (clock != nullptr) {
    sdc->clockLatency(
        clock, sta::RiseFall::rise(), sta::MinMax::max(), target, has_target);
  }
  int ignored = 0;
  builder->getClock().forEachSink([&](ClockInst& sink) {
    odb::dbITerm* pin = sink.getDbInputPin();
    ClockPinModel model;
    if (clockPinModel(pin, model, 0)) {
      builder->setPinModel(pin, model);
      return;
    }
    if (options_->insertionDelayEnabled()) {
      const sta::LibertyPort* port
          = network_->libertyPort(network_->dbToSta(pin));
      model.after = port ? port->clkTreeDelay(
                               0.0, sta::RiseFall::rise(), sta::MinMax::max())
                         : 0.0;
    }
    // A pin with a latency of its own is reached that much before the
    // clock's latency.
    float own = 0.0;
    bool has_own = false;
    if (pin != nullptr) {
      sdc->clockLatency(clock,
                        network_->dbToSta(pin),
                        sta::RiseFall::rise(),
                        sta::MinMax::max(),
                        own,
                        has_own);
    }
    if (has_own && !has_target) {
      ignored++;
    } else if (has_own && own < target) {
      model.after += target - own;
    }
    if (model.after > 0.0) {
      builder->setPinModel(pin, model);
    }
  });
  if (ignored > 0) {
    logger_->warn(CTS,
                  309,
                  "{} clock pins on net {} have a latency of their own, which "
                  "-delay_balanced follows only when clock {} has a latency.",
                  ignored,
                  builder->getClock().getName(),
                  builder->getClock().getSdcName());
  }
}

void TritonCTS::setClockSource(DelayBalancedTreeBuilder* builder)
{
  const auto timer_slew
      = [&](sta::Vertex* vertex, const sta::Scene* scene = nullptr) {
          sta::Scene* at = scene != nullptr ? openSta_->findScene(scene->name())
                                            : openSta_->cmdScene();
          return sta::delayAsFloat(openSta_->slew(
              vertex, sta::RiseFallBoth::rise(), {at}, sta::MinMax::max()));
        };
  openSta_->ensureGraph();
  odb::dbITerm* input = builder->getDriverInput();
  odb::dbNet* net = builder->getClock().getNetObj();
  odb::dbBTerm* source = nullptr;
  if (input != nullptr) {
    sta::Vertex* vertex
        = openSta_->graph()->pinLoadVertex(network_->dbToSta(input));
    if (vertex != nullptr) {
      builder->setDriverSlew(timer_slew(vertex));
    }
    // The clock's port, when this driver is all it drives.
    odb::dbNet* above = input->getNet();
    if (above->getFirstOutput() == nullptr && above->getITermCount() == 1
        && above->getBTermCount() == 1) {
      source = above->get1stBTerm();
    }
  } else if (net->getFirstOutput() == nullptr) {
    for (odb::dbBTerm* port : net->getBTerms()) {
      if (port->getIoType() == odb::dbIoType::INPUT) {
        source = port;
      }
    }
  }
  sta::Vertex* vertex
      = source != nullptr
            ? openSta_->graph()->pinDrvrVertex(network_->dbToSta(source))
            : nullptr;
  int x, y;
  if (vertex != nullptr && input == nullptr) {
    builder->setDriverSlew(timer_slew(vertex));
  } else if (vertex != nullptr && source->getFirstPinLocation(x, y)) {
    builder->setSourcePort(
        {
            .x = x,
            .y = y,
            .cap = 0.0,
            .layer = ClockDelayModel::pinLayer(source),
        },
        timer_slew(vertex),
        timer_slew(vertex, delay_model_->slowScene()));
  }
}

void TritonCTS::setLatencyTarget(DelayBalancedTreeBuilder* builder)
{
  const sta::Sdc* sdc = openSta_->cmdMode()->sdc();
  const sta::Clock* clock = sdc->findClock(builder->getClock().getSdcName());
  if (clock == nullptr) {
    return;
  }
  float latency;
  bool exists;
  sdc->clockLatency(
      clock, sta::RiseFall::rise(), sta::MinMax::max(), latency, exists);
  if (!exists || latency <= 0.0) {
    return;
  }
  odb::dbObject* driver = builder->getClock().getDriverPin();
  odb::dbITerm* input = nullptr;
  if (driver != nullptr && driver->getObjectType() == odb::dbITermObj
      && clockArc(static_cast<odb::dbITerm*>(driver), input) >= 0) {
    latency -= clockArrival(input, 0);
  }
  builder->setLatency(latency);
}

NetWire TritonCTS::placedWire(odb::dbITerm* output,
                              std::vector<odb::dbITerm*>& loads)
{
  int x, y;
  computeITermPosition(output, x, y);
  const NetPin driver{
      .x = x,
      .y = y,
      .cap = 0.0,
      .layer = ClockDelayModel::pinLayer(output->getMTerm()),
  };
  std::vector<NetPin> pins;
  bool leaf = false;
  for (odb::dbITerm* load : output->getNet()->getITerms()) {
    if (load == output || !load->isInputSignal()) {
      continue;
    }
    computeITermPosition(load, x, y);
    pins.push_back({
        .x = x,
        .y = y,
        .cap = delay_model_->pinCap(load),
        .layer = ClockDelayModel::pinLayer(load->getMTerm()),
    });
    loads.push_back(load);
    leaf = leaf || delay_model_->isRegisterClock(load);
  }
  if (loads.empty()) {
    return {};
  }
  return delay_model_->wire(driver, pins, leaf);
}

bool TritonCTS::clockPinModel(odb::dbITerm* pin,
                              ClockPinModel& model,
                              const int depth)
{
  if (isSink(pin) || depth > kMaxClockDepth) {
    return false;
  }
  odb::dbInst* inst = pin->getInst();
  odb::dbITerm* output = getSingleOutput(inst, pin);
  sta::LibertyCell* cell = network_->libertyCell(network_->dbToSta(inst));
  if (output == nullptr || output->getNet() == nullptr || cell == nullptr) {
    return false;
  }
  const int arc = delay_model_->addCell(
      cell,
      cell->findLibertyPort(pin->getMTerm()->getConstName()),
      cell->findLibertyPort(output->getMTerm()->getConstName()));
  if (arc < 0) {
    return false;
  }
  const auto it = delay_builders_.find(output->getNet());
  if (it != delay_builders_.end()) {
    model = it->second->getDriverModel();
    model.cell = arc;
    return model.registers > 0.0;
  }

  // The net has no tree of its own: its loads are what the pin drives.
  std::vector<odb::dbITerm*> loads;
  const NetWire wire = placedWire(output, loads);
  if (loads.empty()) {
    return false;
  }
  const StageTiming stage = delay_model_->drive(
      ClockDelayModel::kBalance, arc, delay_model_->nominalSlew(), wire);
  double sum = 0.0;
  double weight = 0.0;
  for (size_t i = 0; i < loads.size(); i++) {
    ClockPinModel behind;
    if (!clockPinModel(loads[i], behind, depth + 1)) {
      behind = ClockPinModel();
    }
    double delay = stage.wire_delay[i];
    if (behind.cell >= 0) {
      delay += delay_model_->delayAt(
          behind.cell, behind.delay, stage.load_slew[i], behind.load);
    }
    sum += behind.registers * (delay + behind.after);
    weight += behind.registers;
  }
  model.cell = arc;
  model.delay = stage.delay;
  model.load = stage.load;
  model.after = sum / weight;
  model.registers = weight;
  return true;
}

int TritonCTS::clockArc(odb::dbITerm* output, odb::dbITerm*& input)
{
  odb::dbInst* inst = output->getInst();
  sta::LibertyCell* cell = network_->libertyCell(network_->dbToSta(inst));
  if (cell == nullptr) {
    return -1;
  }
  for (odb::dbITerm* pin : inst->getITerms()) {
    if (!pin->isInputSignal() || pin->getNet() == nullptr
        || !staClockNets_.contains(pin->getNet())) {
      continue;
    }
    const int arc = delay_model_->addCell(
        cell,
        cell->findLibertyPort(pin->getMTerm()->getConstName()),
        cell->findLibertyPort(output->getMTerm()->getConstName()));
    if (arc >= 0) {
      input = pin;
      return arc;
    }
  }
  return -1;
}

double TritonCTS::clockArrival(odb::dbITerm* pin, const int depth)
{
  odb::dbITerm* output = pin->getNet()->getFirstOutput();
  odb::dbITerm* input = nullptr;
  if (output == nullptr || depth > kMaxClockDepth) {
    return 0.0;
  }
  const int arc = clockArc(output, input);
  if (arc < 0) {
    return 0.0;
  }
  std::vector<odb::dbITerm*> loads;
  const NetWire wire = placedWire(output, loads);
  const auto at = std::ranges::find(loads, pin);
  const size_t index = at != loads.end() ? at - loads.begin() : 0;
  const StageTiming stage = delay_model_->drive(
      ClockDelayModel::kBalance, arc, delay_model_->nominalSlew(), wire);
  return clockArrival(input, depth + 1) + stage.delay + stage.wire_delay[index];
}

void TritonCTS::buildersBehind(odb::dbITerm* pin,
                               std::vector<DelayBalancedTreeBuilder*>& behind,
                               const int depth)
{
  if (pin == nullptr || isSink(pin) || depth > kMaxClockDepth) {
    return;
  }
  odb::dbITerm* output = getSingleOutput(pin->getInst(), pin);
  if (output == nullptr || output->getNet() == nullptr) {
    return;
  }
  const auto it = delay_builders_.find(output->getNet());
  if (it != delay_builders_.end()) {
    behind.push_back(it->second);
    return;
  }
  for (odb::dbITerm* load : output->getNet()->getITerms()) {
    if (load != output && load->isInputSignal()) {
      buildersBehind(load, behind, depth + 1);
    }
  }
}

void TritonCTS::connectRootLoads(TreeBuilder* builder)
{
  Clock& clockNet = builder->getClock();
  odb::dbNet* topClockNet = clockNet.getNetObj();
  // With buffers for the net's ports, the driver moves to a net of its own
  // with the tree and the first of them, and the last drives the ports' net.
  odb::dbNet* rootNet = topClockNet;
  if (ClockInst* portBuffer = clockNet.getPortBuffer()) {
    odb::dbITerm* topDriver = topClockNet->getFirstOutput();
    rootNet = odb::dbNet::create(block_,
                                 ("clknet_root_" + clockNet.getName()).c_str());
    rootNet->setSigType(odb::dbSigType::CLOCK);
    topDriver->disconnect();
    topDriver->connect(rootNet);
    getFirstInput(portBuffer->getDbInst())->connect(rootNet);
    clockNet.getPortDriver()->getDbInst()->getFirstOutput()->connect(
        topClockNet);
  }
  // Buffers from the clock's port stand between its net and the driver.
  if (ClockInst* sourceBuffer = clockNet.getSourceBuffer()) {
    odb::dbITerm* input
        = static_cast<DelayBalancedTreeBuilder*>(builder)->getDriverInput();
    odb::dbNet* above = input->getNet();
    odb::dbNet* last = odb::dbNet::create(
        block_, ("clknet_source_" + clockNet.getName()).c_str());
    last->setSigType(odb::dbSigType::CLOCK);
    input->disconnect();
    input->connect(last);
    getFirstInput(sourceBuffer->getDbInst())->connect(above);
    clockNet.getSourceDriver()->getDbInst()->getFirstOutput()->connect(last);
  }
  for (ClockInst* load : clockNet.getRootLoads()) {
    odb::dbITerm* inputPin = load->isClockBuffer()
                                 ? getFirstInput(load->getDbInst())
                                 : load->getDbInputPin();
    inputPin->connect(rootNet);
    odb::dbITerm* topDriver = rootNet->getFirstOutput();
    if (network_->hasHierarchy() && topDriver != nullptr) {
      network_->hierarchicalConnect(
          topDriver, inputPin, rootNet->getName().c_str());
    }
  }
}

}  // namespace cts
