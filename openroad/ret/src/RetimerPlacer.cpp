// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "RetimerPlacer.h"

#include <algorithm>
#include <cstddef>
#include <memory>
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
#include "sta/Scene.hh"
#include "utl/Logger.h"

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
                             rsz::Resizer* resizer,
                             utl::Logger* logger)
    : sta_(sta),
      estimate_parasitics_(estimate_parasitics),
      resizer_(resizer),
      logger_(logger)
{
}

void RetimerPlacer::prepare()
{
  // The resizer reads the core it keeps a cell in.
  resizer_->initBlock();
}

bool RetimerPlacer::choosesCells() const
{
  return logger_->debugCheck(utl::RET, "place_size", 1);
}

std::vector<odb::dbMaster*> RetimerPlacer::registerCells(odb::dbMaster* master)
{
  std::vector<odb::dbMaster*> masters = {master};
  if (choosesCells()) {
    const std::vector<odb::dbMaster*>& others = equivalentMasters(master);
    masters.insert(masters.end(), others.begin(), others.end());
  }
  return masters;
}

bool RetimerPlacer::neighborBox(odb::dbInst* inst, odb::Rect& box) const
{
  odb::dbITerm* data = inst->getMaster()->isSequential()
                           ? registerDataPin(sta_, inst)
                           : nullptr;
  box.mergeInit();
  bool any = false;
  for (odb::dbITerm* iterm : inst->getITerms()) {
    odb::dbNet* net = iterm->getNet();
    if (net == nullptr || net->getSigType().isSupply()
        || iterm->getSigType() == odb::dbSigType::CLOCK) {
      continue;
    }
    if (iterm->getIoType() == odb::dbIoType::INPUT && data != nullptr
        && iterm != data) {
      continue;
    }
    for (odb::dbITerm* other : net->getITerms()) {
      odb::dbInst* cell = other->getInst();
      if (cell != inst && cell->isPlaced()) {
        box.merge(cell->getBBox()->getBox());
        any = true;
      }
    }
  }
  return any;
}

void RetimerPlacer::moveTo(odb::dbInst* inst, int x, int y)
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
  auto found = equivalent_masters_.find(master);
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
  std::unique_ptr<sta::LibertyLibraryIterator> lib_iter(
      network->libertyLibraryIterator());
  while (lib_iter->hasNext()) {
    sta::LibertyCellIterator cell_iter(lib_iter->next());
    while (cell_iter.hasNext()) {
      sta::LibertyCell* other = cell_iter.next();
      if (other == cell || other->dontUse() || resizer_->dontUse(other)
          || !sta::equivCells(cell, other)) {
        continue;
      }
      odb::dbMaster* other_master = network->staToDb(other);
      if (other_master != nullptr
          && other_master->getSite() == master->getSite()
          && seen.insert(other_master).second) {
        by_area.emplace_back(other->area(), other_master);
      }
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

std::vector<odb::dbMaster*> RetimerPlacer::fastestMasters(
    odb::dbInst* inst,
    const std::vector<odb::dbMaster*>& masters,
    size_t count)
{
  if (sta_->cmdScene() == nullptr) {
    return masters;
  }
  sta::dbNetwork* network = sta_->getDbNetwork();
  const TableInputs inputs = tableInputs(inst);
  std::vector<std::pair<float, odb::dbMaster*>> ranked;
  for (odb::dbMaster* master : masters) {
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
  for (size_t i = 0; i < ranked.size() && i < count; ++i) {
    fastest.push_back(ranked[i].second);
  }
  return fastest;
}

std::vector<odb::dbMaster*> RetimerPlacer::sizeCandidates(odb::dbInst* inst)
{
  constexpr size_t kCandidates = 3;
  const std::vector<odb::dbMaster*>& all = equivalentMasters(inst->getMaster());
  if (all.size() <= kCandidates) {
    return all;
  }
  return fastestMasters(inst, all, kCandidates);
}

std::vector<RetimerPlacer::CellChoice> RetimerPlacer::cellChoices(
    const std::vector<odb::dbInst*>& registers,
    est::IncrementalParasiticsGuard& guard)
{
  std::vector<CellChoice> choices;
  for (odb::dbInst* inst : registers) {
    const Reward base = rewardAt(inst, guard);
    // A register with slack to spare keeps the small cell.
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
                            size_t rank,
                            est::IncrementalParasiticsGuard& guard)
{
  for (CellChoice& choice : choices) {
    choice.swapped = rank < choice.candidates.size()
                     && choice.candidates[rank] != choice.original
                     && choice.inst->swapMaster(choice.candidates[rank]);
  }
  for (CellChoice& choice : choices) {
    if (!choice.swapped) {
      continue;
    }
    const Reward reward = rewardAt(choice.inst, guard);
    if (betterReward(reward, choice.best)) {
      choice.best = reward;
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

void RetimerPlacer::restoreCells(std::vector<CellChoice>& choices)
{
  for (CellChoice& choice : choices) {
    if (choice.inst->getMaster() != choice.original) {
      choice.inst->swapMaster(choice.original);
    }
  }
}

// The registers of neighboring paths read each other's trial cells; the
// choice is kept only if STA does not call the result worse.
int RetimerPlacer::chooseCells(const std::vector<odb::dbInst*>& registers,
                               est::IncrementalParasiticsGuard& guard)
{
  const StaSlack before = staSlack(sta_, guard);
  std::vector<CellChoice> choices = cellChoices(registers, guard);
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
    restoreCells(choices);
    return 0;
  }
  return changed;
}

RetimerPlacer::Reward RetimerPlacer::rewardAt(
    odb::dbInst* inst,
    est::IncrementalParasiticsGuard& guard)
{
  guard.update();
  sta::Graph* graph = sta_->ensureGraph();
  sta::dbNetwork* network = sta_->getDbNetwork();
  Reward reward{.worst = sta::INF, .sum = 0.0f};
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
      reward.worst = std::min(reward.worst, slack);
      reward.sum += slack;
    }
  }
  return reward;
}

bool RetimerPlacer::betterReward(const Reward& a, const Reward& b)
{
  if (a.worst > b.worst + kStaSlackEps) {
    return true;
  }
  return a.worst > b.worst - kStaSlackEps && a.sum > b.sum + kStaSlackEps;
}

PlaceStats RetimerPlacer::placeNewCells(
    odb::dbBlock* block,
    const std::unordered_set<odb::dbInst*>& skip,
    est::IncrementalParasiticsGuard& guard)
{
  PlaceStats stats;
  if (!hasPlacedCell(block)) {
    return stats;
  }
  std::vector<odb::dbInst*> new_registers;
  // A cell with only new cells around it waits for a later pass.
  bool moved = true;
  while (moved) {
    moved = false;
    for (odb::dbInst* inst : block->getInsts()) {
      odb::Rect box;
      if (inst->isPlaced() || skip.contains(inst) || !neighborBox(inst, box)) {
        continue;
      }
      moveTo(inst, box.xCenter(), box.yCenter());
      if (inst->getMaster()->isSequential()) {
        new_registers.push_back(inst);
      }
      ++stats.placed;
      moved = true;
    }
  }
  if (!new_registers.empty() && choosesCells()) {
    stats.resized = chooseCells(new_registers, guard);
  }
  return stats;
}

}  // namespace ret
