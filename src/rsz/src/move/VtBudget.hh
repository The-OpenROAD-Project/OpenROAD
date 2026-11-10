// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <cstddef>
#include <memory>
#include <unordered_set>
#include <vector>

#include "rsz/Resizer.hh"

namespace sta {
class LibertyCell;
}  // namespace sta

namespace rsz {

struct OptimizerRunConfig;

// The logic cells on each VtBudget's cells, and which swaps keep the budgets.
// A swap may not raise a group past its limit, nor one already past it.  In a
// run that lowers the budgets, a swap must also take its instance out of a
// group past its limit.
class VtBudgetCounts
{
 public:
  VtBudgetCounts(const std::vector<VtBudget>& budgets, bool lowering);

  bool empty() const { return counts_.empty(); }
  bool lowering() const { return lowering_; }
  // Counts the logic cells of the design.  An instance that repair_power
  // cannot move out of a group raises its limit to their number.
  void count(Resizer& resizer);
  bool met() const;
  // The instances over the limits, summed over the budgets.
  int excess() const;
  // Every swap without budgets.
  bool admits(const sta::LibertyCell* from, const sta::LibertyCell* to) const;
  void move(const sta::LibertyCell* from, const sta::LibertyCell* to);

  size_t size() const { return counts_.size(); }
  int count(const size_t group) const { return counts_[group]; }
  int total() const { return total_; }
  float maxPercent(size_t group) const;
  // The budget's limit, before a raise for the instances that cannot move.
  int limit(size_t group) const;
  // The instances that cannot move out of the group.
  int locked(const size_t group) const { return locked_[group]; }

 private:
  bool in(size_t group, const sta::LibertyCell* cell) const;
  int effectiveLimit(size_t group) const;

  struct Group
  {
    std::unordered_set<const sta::LibertyCell*> cells;
    float max_percent{100.0f};
  };
  // Shared by the copies a batch selection makes.
  std::shared_ptr<const std::vector<Group>> groups_;
  bool lowering_;
  int total_{0};
  std::vector<int> counts_;
  std::vector<int> locked_;
};

// The share of each budget, and a warning for each one over its limit after
// the run.
void reportVtBudgets(Resizer& resizer,
                     const VtBudgetCounts& counts,
                     bool after_run);

// Brings config's VT budgets down with repair_power runs, cheapest swaps
// first: one within the setup contract, then one that lets every endpoint fall
// to the setup WS, which grows TNS, and lowers that floor while it stalls,
// which lowers WS.
void lowerVtBudgets(Resizer& resizer, OptimizerRunConfig config);

}  // namespace rsz
