// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "PowerVtSwapGenerator.hh"

#include <algorithm>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

#include "MoveCandidate.hh"
#include "MoveGenerator.hh"
#include "OptimizerTypes.hh"
#include "PowerSwapCandidate.hh"
#include "RepairSetupProfiler.hh"
#include "db_sta/dbSta.hh"
#include "rsz/Resizer.hh"
#include "sta/Delay.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/Transition.hh"

namespace rsz {

bool isRejected(const RejectedSwaps& rejected,
                const sta::Instance* inst,
                const sta::LibertyCell* cell)
{
  const auto cells = rejected.find(inst);
  return cells != rejected.end()
         && std::ranges::find(cells->second, cell) != cells->second.end();
}

std::vector<sta::LibertyCell*> higherVtCells(Resizer& resizer,
                                             sta::LibertyCell* cell)
{
  const sta::LibertyCellSeq cells = resizer.getVTEquivCells(cell);
  const auto it = std::ranges::find(cells, cell);
  if (it == cells.end()) {
    return {};
  }
  return {cells.begin(), it};
}

SlackFloors::SlackFloors(const float margin, const float tolerance)
    : margin_(margin),
      tolerance_(tolerance),
      per_endpoint_(tolerance > 0.0f),
      lowest_(margin - tolerance)
{
}

SlackFloors SlackFloors::uniform(const float floor)
{
  SlackFloors floors(floor, 0.0f);
  floors.per_endpoint_ = true;
  return floors;
}

void SlackFloors::recordEndpoint(const sta::Pin* pin, const float start_slack)
{
  if (!tolerant()) {
    return;
  }
  const float floor = std::min(start_slack, margin_) - tolerance_;
  endpoints_[pin] = floor;
  lowest_ = std::min(lowest_, floor);
}

float SlackFloors::endpoint(const sta::Pin* pin) const
{
  const auto floor = endpoints_.find(pin);
  return floor != endpoints_.end() ? floor->second : margin_ - tolerance_;
}

float SlackFloors::driver(const sta::Pin* pin) const
{
  const auto floor = drivers_.find(pin);
  return floor != drivers_.end() ? floor->second : margin_ - tolerance_;
}

PowerVtSwapGenerator::PowerVtSwapGenerator(const GeneratorContext& context,
                                           PowerModel& power_model,
                                           const RejectedSwaps& rejected,
                                           const SlackFloors& floors,
                                           const VtBudgetCounts& vt_budget,
                                           const float slack_ratio)
    : VtSwapMtGenerator(context),
      power_model_(power_model),
      rejected_(rejected),
      floors_(floors),
      vt_budget_(vt_budget),
      slack_ratio_(slack_ratio)
{
}

bool PowerVtSwapGenerator::isApplicable(const Target& target) const
{
  return VtSwapMtGenerator::isApplicable(target)
         && !higherVtCells(
                 resizer_,
                 resizer_.network()->libertyCell(target.inst(resizer_)))
                 .empty();
}

void PowerVtSwapGenerator::prepareInputs(Target& target) const
{
  VtSwapMtGenerator::prepareInputs(target);
  slew_headroom_[target.driver_pin] = slewHeadroom(target);
  if (floors_.tolerant()) {
    float& side_floor = target.estimator_context->side_slack_floor;
    side_floor = std::min(side_floor, floors_.lowest());
  }
  if (!vt_budget_.lowering()) {
    power_model_.prepare(target.inst(resizer_));
  }
  sta::LibertyCell* current_cell
      = resizer_.network()->libertyCell(target.inst(resizer_));
  resizer_.cellLeakage(current_cell);
  for (sta::LibertyCell* cell : higherVtCells(resizer_, current_cell)) {
    resizer_.cellLeakage(cell);
  }
}

std::vector<std::unique_ptr<MoveCandidate>> PowerVtSwapGenerator::generate(
    const Target& target)
{
  std::vector<std::unique_ptr<MoveCandidate>> candidates;
  sta::Instance* inst = target.inst(resizer_);
  sta::LibertyCell* current_cell = resizer_.network()->libertyCell(inst);
  const float slack
      = sta::delayAsFloat(target.slack) - floors_.driver(target.driver_pin);
  const auto headroom = slew_headroom_.find(target.driver_pin);
  const SlewHeadroom slew_headroom
      = headroom != slew_headroom_.end() ? headroom->second : SlewHeadroom{};
  for (sta::LibertyCell* candidate_cell :
       higherVtCells(resizer_, current_cell)) {
    if (isRejected(rejected_, inst, candidate_cell)
        || !vt_budget_.admits(current_cell, candidate_cell)) {
      continue;
    }
    if (!resizer_.replacementFitsInputCapBudgets(inputCapBudgets(target),
                                                 candidate_cell)) {
      RepairSetupProfiler::event(
          "max_cap_rejects", 1, moveName(MoveType::kVtSwap));
      continue;
    }
    if (vt_budget_.lowering()) {
      candidates.push_back(
          std::make_unique<PowerSwapCandidate>(resizer_,
                                               target,
                                               inst,
                                               candidate_cell,
                                               0.0f,
                                               slack,
                                               slew_headroom,
                                               slack_ratio_,
                                               /*cheapest_first=*/true));
      continue;
    }
    const float gain = power_model_.gain(inst, candidate_cell);
    if (gain > 0.0f) {
      candidates.push_back(std::make_unique<PowerSwapCandidate>(
          resizer_, target, inst, candidate_cell, gain, slack, slew_headroom));
    }
  }
  return candidates;
}

SlewHeadroom PowerVtSwapGenerator::slewHeadroom(const Target& target) const
{
  sta::Network* network = resizer_.network();
  sta::dbSta* sta = resizer_.sta();
  SlewHeadroom headroom;
  netSlewHeadroom(target.driver_pin, headroom.output_margin);
  std::unique_ptr<sta::InstancePinIterator> pin_iter(
      network->pinIterator(target.inst(resizer_)));
  while (pin_iter->hasNext()) {
    const sta::Pin* pin = pin_iter->next();
    const sta::LibertyPort* port = network->libertyPort(pin);
    sta::PinSet* drivers
        = network->isLoad(pin) ? network->drivers(pin) : nullptr;
    if (port == nullptr || drivers == nullptr || drivers->empty()) {
      continue;
    }
    const sta::Pin* driver = *drivers->begin();
    float margin = 0.0f;
    headroom.fanins.push_back(
        {.port = port->name(),
         .headroom = netSlewHeadroom(driver, margin),
         .load_cap = sta->graphDelayCalc()->loadCap(
             driver, sta->cmdScene(), sta::MinMax::max())});
  }
  return headroom;
}

float PowerVtSwapGenerator::netSlewHeadroom(const sta::Pin* driver_pin,
                                            float& margin) const
{
  float headroom = sta::INF;
  margin = sta::INF;
  std::unique_ptr<sta::PinConnectedPinIterator> pin_iter(
      resizer_.network()->connectedPinIterator(driver_pin));
  while (pin_iter->hasNext()) {
    const sta::Pin* pin = pin_iter->next();
    sta::Slew slew = 0.0;
    float limit = 0.0f;
    float slack = 0.0f;
    const sta::RiseFall* rf = nullptr;
    const sta::Scene* scene = nullptr;
    resizer_.sta()->checkSlew(pin,
                              resizer_.sta()->scenes(),
                              sta::MinMax::max(),
                              false,
                              slew,
                              limit,
                              slack,
                              rf,
                              scene);
    const float slew_value = sta::delayAsFloat(slew);
    if (scene != nullptr && limit > 0.0f && slew_value > 0.0f) {
      headroom = std::min(headroom, limit / slew_value);
      margin = std::min(margin, limit - slew_value);
    }
  }
  return headroom;
}

}  // namespace rsz
