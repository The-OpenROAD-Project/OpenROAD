// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "SetupLegacyBase.hh"
#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "sta/NetworkClass.hh"

namespace rsz {

class SetupCritVtSwapPolicy : public SetupLegacyBase
{
 public:
  using SetupLegacyBase::SetupLegacyBase;

  const char* name() const override { return "SetupCritVtSwapPolicy"; }
  void iterate() override;

 private:
  bool swapVTCritCells(int& num_viols);
  sta::Pin* outputPin(sta::Instance* inst);
  // Vertices a path of the selected path group can pass through, found
  // forward from the group's startpoints; nullopt when no group is selected.
  std::optional<std::unordered_set<sta::Vertex*>> findGroupFanout();
  void traverseFaninCone(
      sta::Vertex* endpoint,
      const std::unordered_set<sta::Vertex*>* group_fanout,
      std::vector<std::pair<sta::Instance*, float>>& crit_insts,
      std::unordered_set<sta::Instance*>& crit_inst_set,
      std::unordered_set<sta::Vertex*>& visited,
      std::unordered_set<sta::Instance*>& notSwappable,
      std::unordered_map<sta::Instance*, sta::LibertyCell*>& capture_cells);
  sta::Slack getInstanceSlack(sta::Instance* inst);
  // Worst setup slack over the timing-checked data inputs of a register,
  // INF for other cells.  A capture-only register's outputs show none of it.
  sta::Slack captureSlack(sta::Instance* inst);
  // Largest Liberty setup-margin decrease at a violating data input of
  // register inst when swapped to cell; 0 or less when the swap raises any
  // setup margin or pushes a hold check below zero.
  float captureSetupGain(sta::Instance* inst, sta::LibertyCell* cell);
  // The VT equivalent of register inst with the largest captureSetupGain,
  // null when none gains.  The crit sweep otherwise picks VT by drive
  // strength, and a faster VT can have the larger setup time.
  sta::LibertyCell* bestCaptureVtCell(sta::Instance* inst);
};

}  // namespace rsz
