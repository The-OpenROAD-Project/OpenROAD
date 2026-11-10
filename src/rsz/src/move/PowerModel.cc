// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "PowerModel.hh"

#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "db_sta/dbSta.hh"
#include "rsz/Resizer.hh"
#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/InternalPower.hh"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/PortDirection.hh"
#include "sta/PowerClass.hh"
#include "sta/Scene.hh"
#include "sta/Sdc.hh"
#include "sta/TimingArc.hh"
#include "sta/Transition.hh"

namespace rsz {

PowerModel::PowerModel(Resizer& resizer) : resizer_(resizer)
{
}

void PowerModel::init()
{
  sta::dbSta* sta = resizer_.sta();
  sta::Network* network = resizer_.network();
  scene_ = sta->cmdScene();
  pvt_ = scene_->sdc()->operatingConditions(sta::MinMax::max());
  anchors_.clear();
  insts_.clear();
  std::unique_ptr<sta::LeafInstanceIterator> inst_iter(
      network->leafInstanceIterator());
  while (inst_iter->hasNext()) {
    const sta::Instance* inst = inst_iter->next();
    sta::LibertyCell* cell = network->libertyCell(inst);
    if (cell == nullptr) {
      continue;
    }
    const sta::PowerResult power = sta->power(inst, scene_);
    Anchor& anchor = anchors_[inst];
    anchor = {.cell = cell,
              .leakage = power.leakage(),
              .internal = power.internal(),
              .densities = {}};
    std::unique_ptr<sta::InstancePinIterator> pin_iter(
        network->pinIterator(inst));
    while (pin_iter->hasNext()) {
      const sta::Pin* pin = pin_iter->next();
      if (const sta::LibertyPort* port = network->libertyPort(pin)) {
        anchor.densities.emplace_back(port->name(),
                                      sta->activity(pin, scene_).density());
      }
    }
  }
}

void PowerModel::prepare(const sta::Instance* inst)
{
  const auto anchor = anchors_.find(inst);
  if (insts_.contains(inst) || anchor == anchors_.end()) {
    return;
  }
  sta::dbSta* sta = resizer_.sta();
  sta::Network* network = resizer_.network();
  const sta::MinMax* max = sta::MinMax::max();

  InstInput& input = insts_[inst];
  input.anchor = &anchor->second;
  input.cell = network->libertyCell(inst);
  // Fill the leakage cache gain() reads on the workers.
  resizer_.cellLeakage(input.anchor->cell);
  input.voltage = input.cell->libertyLibrary()->nominalVoltage();

  const sta::DcalcAPIndex slew_index = scene_->dcalcAnalysisPtIndex(max);
  for (const auto& [port_name, density] : anchor->second.densities) {
    const sta::Pin* pin = network->findPin(inst, port_name.c_str());
    if (pin == nullptr) {
      continue;
    }
    PinInput pin_input{.port = port_name,
                       .output = network->direction(pin)->isAnyOutput(),
                       .density = density,
                       .slews = {0.0f, 0.0f},
                       .load_cap = 0.0f};
    if (pin_input.output) {
      pin_input.load_cap = sta->graphDelayCalc()->loadCap(pin, scene_, max);
    } else if (sta::Vertex* vertex = resizer_.graph()->pinLoadVertex(pin)) {
      for (const sta::RiseFall* rf : sta::RiseFall::range()) {
        pin_input.slews[rf->index()]
            = sta::delayAsFloat(resizer_.graph()->slew(vertex, rf, slew_index));
      }
    }
    input.pins.push_back(std::move(pin_input));
  }
}

float PowerModel::gain(const sta::Instance* inst, sta::LibertyCell* cell) const
{
  const auto found = insts_.find(inst);
  if (found == insts_.end()) {
    return 0.0f;
  }
  const InstInput& input = found->second;
  const Anchor& anchor = *input.anchor;
  float gain = 0.0f;

  const std::optional<float> anchor_leakage = resizer_.cellLeakage(anchor.cell);
  const std::optional<float> leakage = resizer_.cellLeakage(input.cell);
  const std::optional<float> new_leakage = resizer_.cellLeakage(cell);
  if (anchor_leakage && leakage && new_leakage && *anchor_leakage > 0.0f) {
    gain += anchor.leakage * (*leakage - *new_leakage) / *anchor_leakage;
  }

  const float anchor_energy = energy(input, anchor.cell);
  if (anchor_energy > 0.0f) {
    gain += anchor.internal * (energy(input, input.cell) - energy(input, cell))
            / anchor_energy;
  }

  // The fanin nets switch less with smaller input pins.
  for (const PinInput& pin : input.pins) {
    const sta::LibertyPort* port = input.cell->findLibertyPort(pin.port);
    const sta::LibertyPort* new_port = cell->findLibertyPort(pin.port);
    if (!pin.output && port != nullptr && new_port != nullptr) {
      gain += 0.5f * input.voltage * input.voltage * pin.density
              * (port->capacitance() - new_port->capacitance());
    }
  }
  return gain;
}

float PowerModel::energy(const InstInput& inst, sta::LibertyCell* cell) const
{
  sta::LibertyCell* scene_cell = cell->sceneCell(scene_, sta::MinMax::max());
  float energy = 0.0f;
  for (const PinInput& pin : inst.pins) {
    const sta::LibertyPort* port = scene_cell->findLibertyPort(pin.port);
    if (port == nullptr) {
      continue;
    }
    // Table energy per transition, averaged over the when groups of each
    // related input, as power weights them by their duty.  Liberty order:
    // summed in port address order, the gain changes from run to run.
    std::vector<std::pair<const sta::LibertyPort*, std::pair<float, int>>>
        groups;
    for (const sta::InternalPower* power : scene_cell->internalPowers(port)) {
      const sta::LibertyPort* related = power->relatedPort();
      const PinInput* from = pin.output && related != nullptr
                                 ? findPin(inst, related->name())
                                 : &pin;
      if (from == nullptr) {
        continue;
      }
      // As STA power, the output edge comes from the opposite input edge
      // through a negative unate arc.
      bool negative_unate = false;
      if (pin.output && related != nullptr) {
        const sta::TimingArcSetSeq& arc_sets
            = scene_cell->timingArcSets(related, port);
        negative_unate
            = !arc_sets.empty()
              && arc_sets[0]->sense() == sta::TimingSense::negative_unate;
      }
      float table_energy = 0.0f;
      for (const sta::RiseFall* rf : sta::RiseFall::range()) {
        const sta::RiseFall* from_rf = negative_unate ? rf->opposite() : rf;
        table_energy += power->power(
            rf, pvt_, from->slews[from_rf->index()], pin.load_cap);
      }
      auto group = std::ranges::find(
          groups, related, [](const auto& entry) { return entry.first; });
      if (group == groups.end()) {
        group = groups.emplace(groups.end(), related, std::pair{0.0f, 0});
      }
      group->second.first += table_energy / 2.0f;
      group->second.second++;
    }

    // An output's transitions split over its related inputs by their
    // activity.
    float density_sum = 0.0f;
    if (pin.output) {
      for (const auto& [related, group] : groups) {
        const PinInput* from
            = related != nullptr ? findPin(inst, related->name()) : nullptr;
        density_sum += from != nullptr ? from->density : 0.0f;
      }
    }
    for (const auto& [related, group] : groups) {
      float weight = 1.0f;
      if (pin.output && related != nullptr && density_sum > 0.0f) {
        weight = findPin(inst, related->name())->density / density_sum;
      }
      energy += pin.density * weight * group.first / group.second;
    }
  }
  return energy;
}

const PowerModel::PinInput* PowerModel::findPin(const InstInput& inst,
                                                const std::string& port) const
{
  for (const PinInput& pin : inst.pins) {
    if (pin.port == port) {
      return &pin;
    }
  }
  return nullptr;
}

}  // namespace rsz
