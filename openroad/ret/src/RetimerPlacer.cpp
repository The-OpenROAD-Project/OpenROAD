// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "RetimerPlacer.h"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <unordered_set>
#include <utility>
#include <vector>

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
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "sta/NetworkClass.hh"

namespace ret {

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

std::optional<odb::Rect> RetimerPlacer::neighborBox(odb::dbInst* inst) const
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
    for (odb::dbITerm* other : net->getITerms()) {
      odb::dbInst* cell = other->getInst();
      if (cell != inst && cell->isPlaced()) {
        box.merge(cell->getBBox()->getBox());
        any = true;
      }
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
  sta::LibertyCell* cell = libertyCell(sta_, master);
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
        .candidates = equivalentMasters(inst->getMaster()),
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
  guard.update();
  std::vector<CellChoice> choices = cellChoices(registers);
  size_t ranks = 0;
  for (const CellChoice& choice : choices) {
    ranks = std::max(ranks, choice.candidates.size());
  }
  for (size_t rank = 0; rank < ranks; ++rank) {
    tryRank(choices, rank, guard);
  }
  return swapToBest(choices);
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

PlaceStats RetimerPlacer::placeNewCells(const std::vector<odb::dbInst*>& cells,
                                        est::IncrementalParasiticsGuard& guard)
{
  PlaceStats stats;
  std::vector<odb::dbInst*> new_registers;
  // A cell with only new cells around it waits for a later pass.
  bool moved = true;
  while (moved) {
    moved = false;
    for (odb::dbInst* inst : cells) {
      if (inst->isPlaced()) {
        continue;
      }
      const std::optional<odb::Rect> box = neighborBox(inst);
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
  if (!new_registers.empty()) {
    stats.resized = chooseCells(new_registers, guard);
  }
  return stats;
}

}  // namespace ret
