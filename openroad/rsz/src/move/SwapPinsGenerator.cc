// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "SwapPinsGenerator.hh"

#include <memory>
#include <utility>
#include <vector>

#include "MoveCommitter.hh"
#include "MoveGenerator.hh"
#include "OptimizerTypes.hh"
#include "SwapPinsCandidate.hh"
#include "SwapPinsEquivalence.hh"
#include "db_sta/dbSta.hh"
#include "rsz/Resizer.hh"
#include "sta/ArcDelayCalc.hh"
#include "sta/Delay.hh"
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

  return buildCandidates(target);
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
  scene = target.activeScene(resizer_);
  min_max = target.minMax(resizer_);
  return drvr != nullptr;
}

std::vector<std::unique_ptr<MoveCandidate>> SwapPinsGenerator::buildCandidates(
    const Target& target) const
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

  const float load_cap
      = resizer_.sta()->graphDelayCalc()->loadCap(drvr_pin, scene, min_max);

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
  const sta::Path* drvr_path = target.driverPath(resizer_);
  const sta::TimingArc* in_arc = drvr_path != nullptr
                                     ? drvr_path->prevArc(resizer_.staState())
                                     : nullptr;
  input_port = in_arc != nullptr ? in_arc->from() : nullptr;
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
    equiv_ports_entry.first->second = equivalentSwapInputs(
        resizer_.logger(), drvr_port->libertyCell(), input_port);
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

}  // namespace rsz
