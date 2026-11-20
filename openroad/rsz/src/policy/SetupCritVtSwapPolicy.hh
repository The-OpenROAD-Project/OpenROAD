// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <optional>
#include <unordered_set>
#include <utility>
#include <vector>

#include "SetupLegacyBase.hh"
#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "sta/NetworkClass.hh"

namespace rsz {

class PathGroupFilter;

class SetupCritVtSwapPolicy : public SetupLegacyBase
{
 public:
  using SetupLegacyBase::SetupLegacyBase;

  const char* name() const override { return "SetupCritVtSwapPolicy"; }
  void iterate() override;

 private:
  bool swapVTCritCells(int& num_viols);
  sta::Pin* outputPin(sta::Instance* inst);
  bool drivesClock(sta::Instance* inst);
  // Vertices a path of the selected path group can pass through, found
  // forward from the group's startpoints; nullopt when no group is selected.
  std::optional<std::unordered_set<sta::Vertex*>> findGroupFanout();
  void traverseFaninCone(
      sta::Vertex* endpoint,
      const std::unordered_set<sta::Vertex*>* group_fanout,
      std::vector<std::pair<sta::Instance*, float>>& crit_insts,
      std::unordered_set<sta::Instance*>& crit_inst_set,
      std::unordered_set<sta::Vertex*>& visited,
      std::unordered_set<sta::Instance*>& notSwappable);
  // The cells on the group's worst path to `endpoint`, for a group the SDC
  // defines: its fanin cone also feeds the endpoint's paths outside it.
  void collectGroupPathInsts(
      sta::Vertex* endpoint,
      const PathGroupFilter& path_group_filter,
      std::vector<std::pair<sta::Instance*, float>>& crit_insts,
      std::unordered_set<sta::Instance*>& crit_inst_set,
      std::unordered_set<sta::Instance*>& notSwappable);
  sta::Slack getInstanceSlack(sta::Instance* inst);
};

}  // namespace rsz
