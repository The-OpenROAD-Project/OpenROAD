// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "SwapPinsMtGenerator.hh"

#include <map>
#include <memory>
#include <utility>
#include <vector>

#include "MoveCandidate.hh"
#include "MoveCommitter.hh"
#include "MoveGenerator.hh"
#include "OptimizerTypes.hh"
#include "SwapPinsEquivalence.hh"
#include "SwapPinsMtCandidate.hh"
#include "db_sta/dbSta.hh"
#include "rsz/Resizer.hh"
#include "sta/ArcDelayCalc.hh"
#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/Liberty.hh"
#include "sta/LibertyClass.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "sta/PortDirection.hh"
#include "sta/TimingArc.hh"
#include "utl/Logger.h"

namespace rsz {

namespace {

using LibertyPortVec = std::vector<sta::LibertyPort*>;

// === Driver and swap-port selection =========================================

// The input port the path enters the target gate through, or null.
sta::LibertyPort* pathInputPort(Resizer& resizer, const Target& target)
{
  const sta::Path* drvr_path = target.driverPath(resizer);
  const sta::TimingArc* in_arc
      = drvr_path != nullptr ? drvr_path->prevArc(resizer.staState()) : nullptr;
  sta::LibertyPort* input_port = in_arc != nullptr ? in_arc->from() : nullptr;
  return input_port != nullptr && !input_port->direction()->isOutput()
             ? input_port
             : nullptr;
}

// The inputs of cell interchangeable with input_port, simulated once per run
// and kept in the policy's cache.
LibertyPortVec equivalentInputs(utl::Logger* logger,
                                EquivPinCache& cache,
                                const sta::LibertyCell* cell,
                                sta::LibertyPort* input_port)
{
  EquivPinCache::iterator cached = cache.find(input_port);
  if (cached == cache.end()) {
    cached = cache
                 .emplace(input_port,
                          equivalentSwapInputs(logger, cell, input_port))
                 .first;
  }
  return cached->second;
}

}  // namespace

SwapPinsMtGenerator::SwapPinsMtGenerator(const GeneratorContext& context)
    : MoveGenerator(context)
{
}

bool SwapPinsMtGenerator::isApplicable(const Target& target) const
{
  // The path enters the gate through an input, the gate has inputs to swap
  // (a buffer or inverter has one), and it may change but has not swapped
  // yet.
  if (!MoveGenerator::isApplicable(target) || target.path_index <= 0) {
    return false;
  }

  sta::Network* network = resizer_.network();
  const sta::LibertyPort* drvr_port = network->libertyPort(target.driver_pin);
  const sta::LibertyCell* cell
      = drvr_port != nullptr ? drvr_port->libertyCell() : nullptr;
  sta::Instance* drvr = network->instance(target.driver_pin);
  return cell != nullptr && !cell->isBuffer() && !cell->isInverter()
         && drvr != nullptr && !resizer_.dontTouch(drvr)
         && !committer_.hasMoves(MoveType::kSwapPins, drvr);
}

void SwapPinsMtGenerator::prepareInputs(Target& target) const
{
  Resizer& resizer = resizer_;
  sta::Pin* drvr_pin = target.driver_pin;
  sta::Instance* drvr = resizer.network()->instance(drvr_pin);
  sta::LibertyPort* drvr_port = resizer.network()->libertyPort(drvr_pin);
  const sta::Scene* scene = target.activeScene(resizer);
  const sta::MinMax* min_max = target.minMax(resizer);

  sta::LibertyPort* input_port = pathInputPort(resizer, target);
  if (input_port == nullptr) {
    return;
  }

  const LibertyPortVec equiv_ports = equivalentInputs(resizer.logger(),
                                                      *equiv_pin_cache_,
                                                      drvr_port->libertyCell(),
                                                      input_port);
  if (equiv_ports.empty()) {
    return;
  }

  // The arc delays under the gate's current input slews and load.
  const float load_cap
      = resizer.sta()->graphDelayCalc()->loadCap(drvr_pin, scene, min_max);
  sta::ArcDelay input_delay = 0.0;
  std::map<sta::LibertyPort*, sta::ArcDelay> port_delays;
  resizer.annotateInputSlews(drvr, scene, min_max);
  resizer.swapPinDelays(input_port,
                        drvr_port,
                        load_cap,
                        scene,
                        min_max,
                        input_delay,
                        port_delays);
  resizer.resetInputSlews();
  if (input_delay == -sta::INF) {
    return;
  }

  // The equivalent inputs that can take the critical signal, each with its
  // delay and, for the side model, the signal it displaces.
  const bool side_model
      = policy_config_.fanin_side_loss && target.estimator_context.has_value();
  std::vector<SwapPinsMtInput::Equivalent> equivalents;
  for (sta::LibertyPort* port : equiv_ports) {
    if (!port_delays.contains(port) || !port->direction()->isInput()
        || sta::LibertyPort::equiv(input_port, port)
        || sta::LibertyPort::equiv(drvr_port, port)) {
      continue;
    }

    SwapPinsMtInput::Equivalent equivalent{.port = port,
                                           .delay = port_delays[port]};
    if (side_model) {
      sta::Pin* pin = resizer.network()->findPin(drvr, port);
      sta::Vertex* vertex = pin != nullptr
                                ? resizer.sta()->graph()->pinLoadVertex(pin)
                                : nullptr;
      if (vertex != nullptr) {
        const sta::Slack slack = resizer.sta()->slack(vertex, min_max);
        if (!sta::delayInf(slack, resizer.sta())) {
          equivalent.pin = pin;
          equivalent.slack = sta::delayAsFloat(slack);
        }
      }
    }
    equivalents.push_back(equivalent);
  }

  target.swap_pins_input = std::make_shared<const SwapPinsMtInput>(
      SwapPinsMtInput{.input_port = input_port,
                      .input_delay = input_delay,
                      .equivalents = std::move(equivalents)});
}

std::vector<std::unique_ptr<MoveCandidate>> SwapPinsMtGenerator::generate(
    const Target& target)
{
  std::vector<std::unique_ptr<MoveCandidate>> candidates;
  if (target.swap_pins_input == nullptr) {
    return candidates;
  }

  // The critical signal moves to the equivalent input with the fastest arc,
  // if that beats its own; the first of equal delays wins.
  const SwapPinsMtInput& input = *target.swap_pins_input;
  const SwapPinsMtInput::Equivalent* swap = nullptr;
  float best_delay = input.input_delay;
  for (const SwapPinsMtInput::Equivalent& equivalent : input.equivalents) {
    if (equivalent.delay < best_delay) {
      swap = &equivalent;
      best_delay = equivalent.delay;
    }
  }
  if (swap == nullptr) {
    return candidates;
  }

  candidates.push_back(std::make_unique<SwapPinsMtCandidate>(
      resizer_, target, target.swap_pins_input, *swap));
  return candidates;
}

}  // namespace rsz
