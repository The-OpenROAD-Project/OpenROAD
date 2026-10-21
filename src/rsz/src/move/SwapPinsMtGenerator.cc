// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "SwapPinsMtGenerator.hh"

#include <algorithm>
#include <cstddef>
#include <map>
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "MoveCandidate.hh"
#include "MoveCommitter.hh"
#include "MoveGenerator.hh"
#include "OptimizerTypes.hh"
#include "SwapPinsMtCandidate.hh"
#include "db_sta/dbSta.hh"
#include "rsz/Resizer.hh"
#include "sta/ArcDelayCalc.hh"
#include "sta/Delay.hh"
#include "sta/FuncExpr.hh"
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
using PortStimulus
    = std::unordered_map<const sta::LibertyPort*, std::vector<bool>>;

// === Functional equivalence by Boolean simulation ==========================

bool simulateExpr(utl::Logger* logger,
                  sta::FuncExpr* expr,
                  PortStimulus& port_stimulus,
                  const size_t table_index)
{
  using Op = sta::FuncExpr::Op;
  switch (expr->op()) {
    case Op::not_:
      return !simulateExpr(logger, expr->left(), port_stimulus, table_index);
    case Op::and_:
      return simulateExpr(logger, expr->left(), port_stimulus, table_index)
             && simulateExpr(logger, expr->right(), port_stimulus, table_index);
    case Op::or_:
      return simulateExpr(logger, expr->left(), port_stimulus, table_index)
             || simulateExpr(logger, expr->right(), port_stimulus, table_index);
    case Op::xor_:
      return simulateExpr(logger, expr->left(), port_stimulus, table_index)
             ^ simulateExpr(logger, expr->right(), port_stimulus, table_index);
    case Op::one:
      return true;
    case Op::zero:
      return false;
    case Op::port:
      return port_stimulus[expr->port()][table_index];
  }

  logger->error(
      utl::RSZ, kMsgUnknownFuncExprOp, "unrecognized expr op from OpenSTA");
  return false;
}

std::vector<bool> simulateExpr(utl::Logger* logger,
                               sta::FuncExpr* expr,
                               PortStimulus& port_stimulus)
{
  const size_t table_length = 0x1 << port_stimulus.size();
  std::vector<bool> result(table_length, false);
  for (size_t i = 0; i < table_length; ++i) {
    result[i] = simulateExpr(logger, expr, port_stimulus, i);
  }
  return result;
}

bool isPortEqiv(utl::Logger* logger,
                const sta::FuncExpr* expr,
                const sta::LibertyCell* cell,
                const sta::LibertyPort* port_a,
                const sta::LibertyPort* port_b)
{
  if (port_a->libertyCell() != cell || port_b->libertyCell() != cell) {
    return false;
  }

  sta::LibertyCellPortIterator port_iter(cell);
  PortStimulus port_stimulus;
  size_t input_port_count = 0;
  while (port_iter.hasNext()) {
    sta::LibertyPort* port = port_iter.next();
    if (port->direction()->isInput()) {
      ++input_port_count;
      port_stimulus[port] = {};
    }
  }
  if (input_port_count > 16) {
    return false;
  }

  size_t var_index = 0;
  for (auto& [port, stimulus] : port_stimulus) {
    const size_t truth_table_length = 0x1 << input_port_count;
    stimulus.resize(truth_table_length, false);
    for (size_t i = 0; i < truth_table_length; ++i) {
      stimulus[i] = static_cast<bool>((i >> var_index) & 0x1);
    }
    ++var_index;
  }

  std::vector<bool> result_no_swap
      = simulateExpr(logger, const_cast<sta::FuncExpr*>(expr), port_stimulus);
  std::swap(port_stimulus.at(port_a), port_stimulus.at(port_b));
  std::vector<bool> result_with_swap
      = simulateExpr(logger, const_cast<sta::FuncExpr*>(expr), port_stimulus);
  return result_no_swap == result_with_swap;
}

// The input ports of cell that are interchangeable with input_port on every
// output function, in port id order.
LibertyPortVec equivCellPins(utl::Logger* logger,
                             const sta::LibertyCell* cell,
                             sta::LibertyPort* input_port)
{
  LibertyPortVec ports;
  if (cell->isSequential() || cell->isIsolationCell()) {
    return ports;
  }

  sta::LibertyCellPortIterator port_iter(cell);
  int outputs = 0;
  int inputs = 0;
  while (port_iter.hasNext()) {
    sta::LibertyPort* port = port_iter.next();
    sta::PortDirection* direction = port->direction();
    if (direction->isOutput()) {
      ++outputs;
    } else if (direction->isInput()) {
      ++inputs;
    } else if (port->isPwrGnd()) {
      continue;
    } else {
      return ports;
    }
  }

  if (outputs < 1 || inputs < 2) {
    return ports;
  }

  sta::LibertyCellPortIterator port_iter2(cell);
  std::unordered_set<sta::LibertyPort*> seen_ports;
  while (port_iter2.hasNext()) {
    sta::LibertyPort* candidate_port = port_iter2.next();
    if (!candidate_port->direction()->isInput()) {
      continue;
    }

    sta::LibertyCellPortIterator output_port_iter(cell);
    std::optional<bool> is_equivalent;
    while (output_port_iter.hasNext()) {
      sta::LibertyPort* output_candidate_port = output_port_iter.next();
      sta::FuncExpr* output_expr = output_candidate_port->function();
      if (!output_candidate_port->direction()->isOutput()
          || output_expr == nullptr || input_port == candidate_port) {
        continue;
      }

      const bool is_equivalent_result
          = isPortEqiv(logger, output_expr, cell, input_port, candidate_port);
      if (!is_equivalent.has_value()) {
        is_equivalent = is_equivalent_result;
      } else {
        is_equivalent = is_equivalent.value() && is_equivalent_result;
      }
    }

    if (is_equivalent.has_value() && is_equivalent.value()
        && !seen_ports.contains(candidate_port)) {
      seen_ports.insert(candidate_port);
      ports.push_back(candidate_port);
    }
  }

  if (!seen_ports.empty()) {
    std::ranges::sort(ports, {}, [](auto* port) { return port->id(); });
  }
  return ports;
}

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
    cached = cache.emplace(input_port, equivCellPins(logger, cell, input_port))
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
