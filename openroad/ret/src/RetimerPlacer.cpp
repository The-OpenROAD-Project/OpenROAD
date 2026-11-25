// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "RetimerPlacer.h"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <optional>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "RegisterTiming.h"
#include "RegisterUtils.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "est/EstimateParasitics.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "rsz/Resizer.hh"
#include "sta/Delay.hh"
#include "sta/EquivCells.hh"
#include "sta/Graph.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "sta/NetworkClass.hh"

namespace ret {

namespace {

bool hasPlacedCell(odb::dbBlock* block)
{
  for (odb::dbInst* inst : block->getInsts()) {
    if (inst->isPlaced()) {
      return true;
    }
  }
  return false;
}

}  // namespace

StaSlack staSlack(sta::dbSta* sta, est::IncrementalParasiticsGuard& guard)
{
  guard.update();
  return {
      .worst = sta::delayAsFloat(sta->worstSlack(sta::MinMax::max())),
      .total = sta::delayAsFloat(sta->totalNegativeSlack(sta::MinMax::max())),
  };
}

RetimerPlacer::RetimerPlacer(sta::dbSta* sta,
                             est::EstimateParasitics* estimate_parasitics,
                             rsz::Resizer* resizer)
    : sta_(sta), estimate_parasitics_(estimate_parasitics), resizer_(resizer)
{
}

void RetimerPlacer::prepare()
{
  // The resizer reads the core it keeps a cell in.
  resizer_->initBlock();
}

std::vector<odb::dbMaster*> RetimerPlacer::registerCells(odb::dbMaster* master)
{
  std::vector<odb::dbMaster*> masters = {master};
  const std::vector<odb::dbMaster*>& others = equivalentMasters(master);
  masters.insert(masters.end(), others.begin(), others.end());
  return masters;
}

std::optional<odb::Rect> RetimerPlacer::neighborBox(
    odb::dbInst* inst,
    const int max_input_neighbors) const
{
  const bool sequential = inst->getMaster()->isSequential();
  odb::dbITerm* data = sequential ? registerDataPin(sta_, inst) : nullptr;
  odb::Rect box;
  box.mergeInit();
  bool any = false;
  for (odb::dbITerm* iterm : inst->getITerms()) {
    odb::dbNet* net = iterm->getNet();
    if (net == nullptr || net->getSigType().isSupply()
        || iterm->getSigType() == odb::dbSigType::CLOCK) {
      continue;
    }
    const bool input = iterm->getIoType() == odb::dbIoType::INPUT;
    if (input && data != nullptr && iterm != data) {
      continue;
    }
    std::vector<odb::dbInst*> neighbors;
    for (odb::dbITerm* other : net->getITerms()) {
      odb::dbInst* cell = other->getInst();
      if (cell != inst && cell->isPlaced()) {
        neighbors.push_back(cell);
      }
    }
    if (input && !sequential
        && std::cmp_greater(neighbors.size(), max_input_neighbors)) {
      continue;
    }
    for (odb::dbInst* cell : neighbors) {
      box.merge(cell->getBBox()->getBox());
      any = true;
    }
  }
  if (!any) {
    return std::nullopt;
  }
  return box;
}

void RetimerPlacer::moveTo(odb::dbInst* inst, const int x, const int y)
{
  odb::dbMaster* master = inst->getMaster();
  resizer_->setLocation(inst,
                        {x - static_cast<int>(master->getWidth()) / 2,
                         y - static_cast<int>(master->getHeight()) / 2});
  for (odb::dbITerm* iterm : inst->getITerms()) {
    odb::dbNet* net = iterm->getNet();
    if (net != nullptr && !net->getSigType().isSupply()) {
      estimate_parasitics_->parasiticsInvalid(net);
    }
  }
}

const std::vector<odb::dbMaster*>& RetimerPlacer::equivalentMasters(
    odb::dbMaster* master)
{
  const auto found = equivalent_masters_.find(master);
  if (found != equivalent_masters_.end()) {
    return found->second;
  }
  std::vector<odb::dbMaster*>& masters = equivalent_masters_[master];
  sta::dbNetwork* network = sta_->getDbNetwork();
  sta::LibertyCell* cell = network->libertyCell(network->dbToSta(master));
  if (cell == nullptr) {
    return masters;
  }
  std::vector<std::pair<float, odb::dbMaster*>> by_area;
  std::unordered_set<odb::dbMaster*> seen = {master};
  for (sta::LibertyCell* other : libertyCells(sta_)) {
    if (other == cell || other->dontUse() || resizer_->dontUse(other)
        || !sta::equivCells(cell, other)) {
      continue;
    }
    odb::dbMaster* other_master = network->staToDb(other);
    if (other_master != nullptr && other_master->getSite() == master->getSite()
        && seen.insert(other_master).second) {
      by_area.emplace_back(other->area(), other_master);
    }
  }
  std::ranges::stable_sort(
      by_area, [](const auto& a, const auto& b) { return a.first < b.first; });
  for (const auto& [area, other_master] : by_area) {
    masters.push_back(other_master);
  }
  return masters;
}

RetimerPlacer::TableInputs RetimerPlacer::tableInputs(odb::dbInst* inst) const
{
  sta::dbNetwork* network = sta_->getDbNetwork();
  sta::GraphDelayCalc* delay_calc = sta_->graphDelayCalc();
  odb::dbITerm* data = registerDataPin(sta_, inst);
  TableInputs inputs;
  for (odb::dbITerm* iterm : inst->getITerms()) {
    if (iterm->getNet() == nullptr || iterm->getSigType().isSupply()) {
      continue;
    }
    sta::Pin* pin = network->dbToSta(iterm);
    const sta::LibertyPort* port = network->libertyPort(pin);
    if ((port != nullptr && port->isRegClk())
        || iterm->getSigType() == odb::dbSigType::CLOCK) {
      inputs.clock_slew = clockPinSlew(sta_, pin);
    } else if (iterm == data) {
      inputs.data_slew = pinSlew(sta_, pin);
    } else if (iterm->getIoType() == odb::dbIoType::OUTPUT) {
      inputs.load = std::max(
          inputs.load,
          delay_calc->loadCap(pin, sta_->cmdScene(), sta::MinMax::max()));
    }
  }
  return inputs;
}

std::vector<odb::dbMaster*> RetimerPlacer::sizeCandidates(odb::dbInst* inst)
{
  constexpr size_t kCandidates = 3;
  const std::vector<odb::dbMaster*>& all = equivalentMasters(inst->getMaster());
  if (all.size() <= kCandidates) {
    return all;
  }
  sta::dbNetwork* network = sta_->getDbNetwork();
  const TableInputs inputs = tableInputs(inst);
  std::vector<std::pair<float, odb::dbMaster*>> ranked;
  for (odb::dbMaster* master : all) {
    sta::LibertyCell* cell = network->libertyCell(network->dbToSta(master));
    const float launch
        = registerClockToQ(sta_, cell, nullptr, inputs.clock_slew, inputs.load);
    if (launch < 0.0f) {
      continue;
    }
    const float setup
        = registerSetupTime(sta_, cell, inputs.clock_slew, inputs.data_slew);
    ranked.emplace_back(launch + std::max(setup, 0.0f), master);
  }
  // Equal delays keep the order they came in, smallest area first.
  std::ranges::stable_sort(
      ranked, [](const auto& a, const auto& b) { return a.first < b.first; });
  std::vector<odb::dbMaster*> fastest;
  for (size_t i = 0; i < ranked.size() && i < kCandidates; ++i) {
    fastest.push_back(ranked[i].second);
  }
  return fastest;
}

std::vector<RetimerPlacer::CellChoice> RetimerPlacer::cellChoices(
    const std::vector<odb::dbInst*>& registers)
{
  std::vector<CellChoice> choices;
  for (odb::dbInst* inst : registers) {
    const PinSlack base = pinSlack(inst);
    if (base.worst >= 0.0f) {
      continue;
    }
    choices.push_back({
        .inst = inst,
        .original = inst->getMaster(),
        .candidates = sizeCandidates(inst),
        .best = base,
        .best_master = inst->getMaster(),
        .swapped = false,
    });
  }
  return choices;
}

void RetimerPlacer::tryRank(std::vector<CellChoice>& choices,
                            const size_t rank,
                            est::IncrementalParasiticsGuard& guard)
{
  for (CellChoice& choice : choices) {
    choice.swapped = rank < choice.candidates.size()
                     && choice.inst->swapMaster(choice.candidates[rank]);
  }
  guard.update();
  for (CellChoice& choice : choices) {
    if (!choice.swapped) {
      continue;
    }
    const PinSlack slack = pinSlack(choice.inst);
    if (betterSlack(slack, choice.best)) {
      choice.best = slack;
      choice.best_master = choice.candidates[rank];
    }
  }
  for (CellChoice& choice : choices) {
    if (choice.swapped) {
      choice.inst->swapMaster(choice.original);
    }
  }
}

int RetimerPlacer::swapToBest(std::vector<CellChoice>& choices)
{
  int changed = 0;
  for (CellChoice& choice : choices) {
    if (choice.best_master != choice.original
        && choice.inst->swapMaster(choice.best_master)) {
      ++changed;
    }
  }
  return changed;
}

int RetimerPlacer::chooseCells(const std::vector<odb::dbInst*>& registers,
                               est::IncrementalParasiticsGuard& guard)
{
  const StaSlack before = staSlack(sta_, guard);
  std::vector<CellChoice> choices = cellChoices(registers);
  size_t ranks = 0;
  for (const CellChoice& choice : choices) {
    ranks = std::max(ranks, choice.candidates.size());
  }
  for (size_t rank = 0; rank < ranks; ++rank) {
    tryRank(choices, rank, guard);
  }
  const int changed = swapToBest(choices);
  if (changed == 0) {
    return 0;
  }
  const StaSlack after = staSlack(sta_, guard);
  const bool worse = after.worst < before.worst - kStaSlackEps
                     || (after.worst < before.worst + kStaSlackEps
                         && after.total < before.total - kStaSlackEps);
  if (worse) {
    for (CellChoice& choice : choices) {
      if (choice.inst->getMaster() != choice.original) {
        choice.inst->swapMaster(choice.original);
      }
    }
    return 0;
  }
  return changed;
}

RetimerPlacer::PinSlack RetimerPlacer::pinSlack(odb::dbInst* inst) const
{
  sta::Graph* graph = sta_->ensureGraph();
  sta::dbNetwork* network = sta_->getDbNetwork();
  PinSlack pin_slack{.worst = sta::INF, .sum = 0.0f};
  for (odb::dbITerm* iterm : inst->getITerms()) {
    odb::dbNet* net = iterm->getNet();
    if (net == nullptr || net->getSigType().isSupply()
        || iterm->getSigType() == odb::dbSigType::CLOCK) {
      continue;
    }
    const sta::Pin* pin = network->dbToSta(iterm);
    sta::Vertex* vertex = iterm->getIoType() == odb::dbIoType::OUTPUT
                              ? graph->pinDrvrVertex(pin)
                              : graph->pinLoadVertex(pin);
    if (vertex == nullptr) {
      continue;
    }
    const float slack
        = sta::delayAsFloat(sta_->slack(vertex, sta::MinMax::max()));
    if (slack < sta::INF / 2) {
      pin_slack.worst = std::min(pin_slack.worst, slack);
      pin_slack.sum += slack;
    }
  }
  return pin_slack;
}

bool RetimerPlacer::betterSlack(const PinSlack& a, const PinSlack& b)
{
  if (a.worst > b.worst + kStaSlackEps) {
    return true;
  }
  return a.worst > b.worst - kStaSlackEps && a.sum > b.sum + kStaSlackEps;
}

PlaceStats RetimerPlacer::placeNewCells(
    odb::dbBlock* block,
    const std::unordered_set<std::string>& skip,
    est::IncrementalParasiticsGuard& guard)
{
  PlaceStats stats;
  if (!hasPlacedCell(block)) {
    return stats;
  }
  std::vector<odb::dbInst*> new_registers;
  // A cell with only new cells around it waits for a later pass.  A gate is
  // first placed without its wide input nets, such as an enable.
  constexpr int kMaxInputNeighbors = 8;
  constexpr int kNoBound = std::numeric_limits<int>::max();
  for (const int bound : {kMaxInputNeighbors, kNoBound}) {
    bool moved = true;
    while (moved) {
      moved = false;
      for (odb::dbInst* inst : block->getInsts()) {
        if (inst->isPlaced() || skip.contains(inst->getName())) {
          continue;
        }
        const std::optional<odb::Rect> box = neighborBox(inst, bound);
        if (!box) {
          continue;
        }
        moveTo(inst, box->xCenter(), box->yCenter());
        if (inst->getMaster()->isSequential()) {
          new_registers.push_back(inst);
        }
        ++stats.placed;
        moved = true;
      }
    }
  }
  if (!new_registers.empty()) {
    stats.resized = chooseCells(new_registers, guard);
  }
  return stats;
}

}  // namespace ret
