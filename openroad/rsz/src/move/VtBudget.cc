// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "VtBudget.hh"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

#include "Optimizer.hh"
#include "OptimizerTypes.hh"
#include "PowerVtSwapGenerator.hh"
#include "db_sta/dbSta.hh"
#include "rsz/Resizer.hh"
#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "sta/GraphClass.hh"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "sta/Mode.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "sta/Units.hh"
#include "utl/Logger.h"

namespace rsz {

VtBudgetCounts::VtBudgetCounts(const std::vector<VtBudget>& budgets,
                               const bool lowering)
    : lowering_(lowering),
      counts_(budgets.size(), 0),
      locked_(budgets.size(), 0)
{
  auto groups = std::make_shared<std::vector<Group>>();
  for (const VtBudget& budget : budgets) {
    groups->push_back({
        .cells = {budget.cells.begin(), budget.cells.end()},
        .max_percent = budget.max_percent,
    });
  }
  groups_ = std::move(groups);
}

bool VtBudgetCounts::in(const size_t group, const sta::LibertyCell* cell) const
{
  return (*groups_)[group].cells.contains(cell);
}

void VtBudgetCounts::count(Resizer& resizer)
{
  if (empty()) {
    return;
  }
  sta::Network* network = resizer.network();
  sta::dbSta* sta = resizer.sta();
  for (sta::Mode* mode : sta->modes()) {
    sta->ensureClkNetwork(mode);
  }
  const auto is_clock_in_any_mode = [&](const sta::Pin* pin) {
    return std::ranges::any_of(
        sta->modes(), [&](sta::Mode* mode) { return sta->isClock(pin, mode); });
  };
  // repair_power swaps the cells of non-clock drivers with a timing path.
  sta::Graph* graph = sta->graph();
  const sta::MinMax* max = sta::MinMax::max();
  const auto movable = [&](const sta::Instance* inst) {
    if (!resizer.isEditableLogicStdCell(inst)) {
      return false;
    }
    bool has_data_driver = false;
    std::unique_ptr<sta::InstancePinIterator> pin_iter(
        network->pinIterator(inst));
    while (pin_iter->hasNext()) {
      const sta::Pin* pin = pin_iter->next();
      if (!network->isDriver(pin)) {
        continue;
      }
      if (is_clock_in_any_mode(pin)) {
        return false;
      }
      if (graph != nullptr) {
        sta::Vertex* vertex = graph->pinDrvrVertex(pin);
        if (vertex == nullptr) {
          continue;
        }
        const sta::Path* path = sta->vertexWorstSlackPath(vertex, max);
        if (path == nullptr) {
          path = sta->vertexWorstArrivalPath(vertex, max);
        }
        if (path == nullptr || path->prevArc(sta) == nullptr) {
          continue;
        }
      }
      has_data_driver = true;
    }
    return has_data_driver;
  };

  total_ = 0;
  std::ranges::fill(counts_, 0);
  std::ranges::fill(locked_, 0);
  std::unique_ptr<sta::LeafInstanceIterator> inst_iter(
      network->leafInstanceIterator());
  while (inst_iter->hasNext()) {
    sta::Instance* inst = inst_iter->next();
    sta::LibertyCell* cell = network->libertyCell(inst);
    if (cell == nullptr || !resizer.isLogicStdCell(inst)) {
      continue;
    }
    ++total_;
    for (size_t group = 0; group < counts_.size(); ++group) {
      if (!in(group, cell)) {
        continue;
      }
      ++counts_[group];
      const bool can_leave
          = movable(inst)
            && std::ranges::any_of(
                higherVtCells(resizer, cell),
                [&](const sta::LibertyCell* to) { return !in(group, to); });
      if (!can_leave) {
        ++locked_[group];
      }
    }
  }
}

float VtBudgetCounts::maxPercent(const size_t group) const
{
  return (*groups_)[group].max_percent;
}

int VtBudgetCounts::limit(const size_t group) const
{
  return static_cast<int>(
      std::floor(maxPercent(group) / 100.0 * total_ + 1e-6));
}

int VtBudgetCounts::effectiveLimit(const size_t group) const
{
  return std::max(limit(group), locked_[group]);
}

bool VtBudgetCounts::met() const
{
  return excess() == 0;
}

int VtBudgetCounts::excess() const
{
  int excess = 0;
  for (size_t group = 0; group < counts_.size(); ++group) {
    excess += std::max(counts_[group] - effectiveLimit(group), 0);
  }
  return excess;
}

bool VtBudgetCounts::admits(const sta::LibertyCell* from,
                            const sta::LibertyCell* to) const
{
  bool lowers = false;
  for (size_t group = 0; group < counts_.size(); ++group) {
    const bool was = in(group, from);
    const bool is = in(group, to);
    if (is && !was && counts_[group] >= effectiveLimit(group)) {
      return false;
    }
    if (was && !is && counts_[group] > effectiveLimit(group)) {
      lowers = true;
    }
  }
  return !lowering_ || lowers;
}

void VtBudgetCounts::move(const sta::LibertyCell* from,
                          const sta::LibertyCell* to)
{
  for (size_t group = 0; group < counts_.size(); ++group) {
    counts_[group] += static_cast<int>(in(group, to)) - in(group, from);
  }
}

void reportVtBudgets(Resizer& resizer,
                     const VtBudgetCounts& counts,
                     const bool after_run)
{
  utl::Logger* logger = resizer.logger();
  for (size_t group = 0; group < counts.size(); ++group) {
    const int count = counts.count(group);
    const float share
        = counts.total() > 0 ? 100.0f * count / counts.total() : 0.0f;
    if (!after_run) {
      logger->info(utl::RSZ,
                   3265,
                   "VT budget {}: {} of {} logic cells ({:.2f}%), limit {}%.",
                   group + 1,
                   count,
                   counts.total(),
                   share,
                   counts.maxPercent(group));
      if (counts.locked(group) > counts.limit(group)) {
        logger->warn(utl::RSZ,
                     3268,
                     "VT budget {}: {} of its cells cannot change VT, more "
                     "than its limit of {}.",
                     group + 1,
                     counts.locked(group),
                     counts.limit(group));
      }
    } else if (count > counts.limit(group)) {
      logger->warn(utl::RSZ,
                   3269,
                   "VT budget {} not met: {} of {} logic cells ({:.2f}%), "
                   "limit {}%.",
                   group + 1,
                   count,
                   counts.total(),
                   share,
                   counts.maxPercent(group));
    } else {
      logger->info(utl::RSZ,
                   3266,
                   "VT budget {} met: {} of {} logic cells ({:.2f}%).",
                   group + 1,
                   count,
                   counts.total(),
                   share);
    }
  }
}

bool lowerVtBudgets(Resizer& resizer, OptimizerRunConfig config)
{
  sta::dbSta* sta = resizer.sta();
  const sta::MinMax* max = sta::MinMax::max();
  const sta::Slack ws_before = sta->worstSlack(max);
  const sta::Slack tns_before = sta->totalNegativeSlack(max);
  VtBudgetCounts counts(config.vt_budgets, false);
  const auto run = [&] {
    Optimizer optimizer(&resizer);
    optimizer.configure(config);
    optimizer.run();
    counts.count(resizer);
    return counts.met();
  };

  config.vt_budget_run = true;
  bool relaxed_setup = false;
  if (!run()) {
    // Past the contract, one floor for every endpoint replaces the tolerance.
    relaxed_setup = true;
    config.setup_tolerance = 0.0;
    config.vt_budget_ws = sta::delayAsFloat(sta->worstSlack(max));
    run();
  }

  const sta::Unit* time_unit = sta->units()->timeUnit();
  resizer.logger()->info(
      utl::RSZ,
      3270,
      "VT budget runs: setup WS {} -> {}{}, TNS {} -> {}{}.",
      time_unit->asString(sta::delayAsFloat(ws_before)),
      time_unit->asString(sta::delayAsFloat(sta->worstSlack(max))),
      time_unit->scaleAbbrevSuffix(),
      time_unit->asString(sta::delayAsFloat(tns_before)),
      time_unit->asString(sta::delayAsFloat(sta->totalNegativeSlack(max))),
      time_unit->scaleAbbrevSuffix());
  return relaxed_setup;
}

}  // namespace rsz
