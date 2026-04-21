// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "SwapPinsGenerator.hh"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "MoveCommitter.hh"
#include "MoveGenerator.hh"
#include "OptimizerTypes.hh"
#include "SwapPinsCandidate.hh"
#include "db_sta/dbSta.hh"
#include "rsz/Resizer.hh"
#include "sta/ArcDelayCalc.hh"
#include "sta/Delay.hh"
#include "sta/FuncExpr.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/Liberty.hh"
#include "sta/LibertyClass.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "sta/PortDirection.hh"
#include "utl/Logger.h"

namespace rsz {

using utl::RSZ;

SwapPinsGenerator::SwapPinsGenerator(const GeneratorContext& context)
    : MoveGenerator(context)
{
}

bool SwapPinsGenerator::isApplicable(const Target& target) const
{
  return MoveGenerator::isApplicable(target) && target.path_index > 0;
}

std::vector<std::unique_ptr<MoveCandidate>> SwapPinsGenerator::generate(
    const Target& target)
{
  if (!isApplicable(target)) {
    return {};
  }

  if (!target.isPrepared(kArcDelayStateCache)) {
    return buildCandidates(
        target, std::nullopt, -std::numeric_limits<double>::infinity());
  }

  const ArcDelayState& arc_delay = target.arc_delay.value();
  return buildCandidates(target, arc_delay.load_cap, arc_delay.current_delay);
}

bool SwapPinsGenerator::resolveDriverContext(const Target& target,
                                             sta::Pin*& drvr_pin,
                                             sta::Instance*& drvr,
                                             sta::LibertyPort*& drvr_port,
                                             const sta::Scene*& scene,
                                             const sta::MinMax*& min_max) const
{
  drvr_pin = target.driver_pin;
  if (drvr_pin == nullptr) {
    drvr_pin = target.endpoint_path->pin(resizer_.staState());
  }
  if (drvr_pin == nullptr) {
    return false;
  }

  drvr_port = resizer_.network()->libertyPort(drvr_pin);
  if (drvr_port == nullptr) {
    return false;
  }

  sta::LibertyCell* cell = drvr_port->libertyCell();
  if (cell == nullptr || cell->isBuffer() || cell->isInverter()) {
    return false;
  }

  drvr = resizer_.network()->instance(drvr_pin);
  scene = target.endpoint_path->scene(resizer_.sta());
  min_max = target.endpoint_path->minMax(resizer_.sta());
  return drvr != nullptr;
}

std::vector<std::unique_ptr<MoveCandidate>> SwapPinsGenerator::buildCandidates(
    const Target& target,
    const std::optional<float> load_cap_override,
    const float current_delay_override) const
{
  std::vector<std::unique_ptr<MoveCandidate>> candidates;
  sta::Pin* drvr_pin = nullptr;
  sta::Instance* drvr = nullptr;
  sta::LibertyPort* drvr_port = nullptr;
  const sta::Scene* scene = nullptr;
  const sta::MinMax* min_max = nullptr;
  if (!resolveDriverContext(
          target, drvr_pin, drvr, drvr_port, scene, min_max)) {
    return candidates;
  }
  if (resizer_.dontTouch(drvr)) {
    debugPrint(resizer_.logger(),
               RSZ,
               "swap_pins_move",
               2,
               "REJECT SwapPinsMove {}: {} is \"don't touch\"",
               resizer_.network()->pathName(drvr_pin),
               resizer_.network()->pathName(drvr));
    return candidates;
  }
  if (committer_.hasMoves(MoveType::kSwapPins, drvr)) {
    debugPrint(resizer_.logger(),
               RSZ,
               "swap_pins_move",
               2,
               "REJECT SwapPinsMove {}: Already swapped {}",
               resizer_.network()->pathName(drvr_pin),
               resizer_.network()->pathName(drvr));
    return candidates;
  }

  const float load_cap = load_cap_override.has_value()
                             ? *load_cap_override
                             : resizer_.sta()->graphDelayCalc()->loadCap(
                                   drvr_pin, scene, min_max);

  sta::LibertyPort* input_port = nullptr;
  if (!loadInputPort(target, input_port)) {
    return candidates;
  }

  sta::LibertyPort* swap_port = nullptr;
  float current_delay = 0.0f;
  float swap_delay = 0.0f;
  if (!selectSwapPort(drvr,
                      drvr_port,
                      input_port,
                      scene,
                      min_max,
                      load_cap,
                      swap_port,
                      current_delay,
                      swap_delay)) {
    return candidates;
  }

  current_delay
      = current_delay_override > -std::numeric_limits<float>::infinity()
            ? current_delay_override
            : current_delay;
  candidates.push_back(std::make_unique<SwapPinsCandidate>(resizer_,
                                                           target,
                                                           drvr,
                                                           drvr_port,
                                                           input_port,
                                                           swap_port,
                                                           current_delay,
                                                           swap_delay));
  return candidates;
}

bool SwapPinsGenerator::loadInputPort(const Target& target,
                                      sta::LibertyPort*& input_port) const
{
  const sta::Path* in_path = target.inputPath(resizer_);
  sta::Pin* input_pin
      = in_path != nullptr ? in_path->pin(resizer_.sta()) : nullptr;
  if (input_pin == nullptr) {
    return false;
  }

  input_port = resizer_.network()->libertyPort(input_pin);
  return input_port != nullptr && !input_port->direction()->isOutput();
}

bool SwapPinsGenerator::selectSwapPort(sta::Instance* drvr,
                                       sta::LibertyPort* drvr_port,
                                       sta::LibertyPort* input_port,
                                       const sta::Scene* scene,
                                       const sta::MinMax* min_max,
                                       const float load_cap,
                                       sta::LibertyPort*& swap_port,
                                       float& current_delay,
                                       float& swap_delay) const
{
  std::pair<EquivPinMap::iterator, bool> equiv_ports_entry
      = equiv_pin_map_.try_emplace(input_port);
  if (equiv_ports_entry.second) {
    equivCellPins(
        drvr_port->libertyCell(), input_port, equiv_ports_entry.first->second);
  }
  const LibertyPortVec& equiv_ports = equiv_ports_entry.first->second;
  if (equiv_ports.empty()) {
    return false;
  }

  swap_port = input_port;
  sta::ArcDelay current_arc_delay = 0.0;
  sta::ArcDelay swap_arc_delay = 0.0;
  resizer_.annotateInputSlews(drvr, scene, min_max);
  resizer_.findSwapPinCandidate(input_port,
                                drvr_port,
                                equiv_ports,
                                load_cap,
                                scene,
                                min_max,
                                &swap_port,
                                &current_arc_delay,
                                &swap_arc_delay);
  resizer_.resetInputSlews();
  current_delay = current_arc_delay;
  swap_delay = swap_arc_delay;
  return swap_port != nullptr
         && !sta::LibertyPort::equiv(swap_port, input_port);
}

void SwapPinsGenerator::equivCellPins(const sta::LibertyCell* cell,
                                      sta::LibertyPort* input_port,
                                      LibertyPortVec& ports) const
{
  if (cell->hasSequentials() || cell->isIsolationCell()) {
    ports.clear();
    return;
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
      ports.clear();
      return;
    }
  }

  if (outputs < 1 || inputs < 2) {
    return;
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
          = isPortEqiv(output_expr, cell, input_port, candidate_port);
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
}

bool SwapPinsGenerator::isPortEqiv(const sta::FuncExpr* expr,
                                   const sta::LibertyCell* cell,
                                   const sta::LibertyPort* port_a,
                                   const sta::LibertyPort* port_b) const
{
  if (port_a->libertyCell() != cell || port_b->libertyCell() != cell) {
    return false;
  }

  sta::LibertyCellPortIterator port_iter(cell);
  std::unordered_map<const sta::LibertyPort*, std::vector<bool>> port_stimulus;
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
      = simulateExpr(const_cast<sta::FuncExpr*>(expr), port_stimulus);
  std::swap(port_stimulus.at(port_a), port_stimulus.at(port_b));
  std::vector<bool> result_with_swap
      = simulateExpr(const_cast<sta::FuncExpr*>(expr), port_stimulus);
  return result_no_swap == result_with_swap;
}

bool SwapPinsGenerator::simulateExpr(
    sta::FuncExpr* expr,
    std::unordered_map<const sta::LibertyPort*, std::vector<bool>>&
        port_stimulus,
    const size_t table_index) const
{
  using Op = sta::FuncExpr::Op;
  switch (expr->op()) {
    case Op::not_:
      return !simulateExpr(expr->left(), port_stimulus, table_index);
    case Op::and_:
      return simulateExpr(expr->left(), port_stimulus, table_index)
             && simulateExpr(expr->right(), port_stimulus, table_index);
    case Op::or_:
      return simulateExpr(expr->left(), port_stimulus, table_index)
             || simulateExpr(expr->right(), port_stimulus, table_index);
    case Op::xor_:
      return simulateExpr(expr->left(), port_stimulus, table_index)
             ^ simulateExpr(expr->right(), port_stimulus, table_index);
    case Op::one:
      return true;
    case Op::zero:
      return false;
    case Op::port:
      return port_stimulus[expr->port()][table_index];
  }

  resizer_.logger()->error(utl::RSZ, 91, "unrecognized expr op from OpenSTA");
  return false;
}

std::vector<bool> SwapPinsGenerator::simulateExpr(
    sta::FuncExpr* expr,
    std::unordered_map<const sta::LibertyPort*, std::vector<bool>>&
        port_stimulus) const
{
  const size_t table_length = 0x1 << port_stimulus.size();
  std::vector<bool> result(table_length, false);
  for (size_t i = 0; i < table_length; ++i) {
    result[i] = simulateExpr(expr, port_stimulus, i);
  }
  return result;
}

}  // namespace rsz
