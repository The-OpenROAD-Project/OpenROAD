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
  // capture_only: the CAPTURE_VT_SWAP phase, which swaps only registers whose
  // failing checks are at their data inputs, to the VT that shortens them.
  SetupCritVtSwapPolicy(Resizer& resizer,
                        MoveCommitter& committer,
                        RepairSetupContext& setup_context,
                        const OptimizerRunConfig& config,
                        bool capture_only = false);

  const char* name() const override
  {
    return capture_only_ ? "SetupCaptureVtSwapPolicy" : "SetupCritVtSwapPolicy";
  }
  void iterate() override;

 private:
  const bool capture_only_;
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
      std::unordered_set<sta::Instance*>& notSwappable,
      std::unordered_map<sta::Instance*, sta::LibertyCell*>& capture_cells);
  sta::Slack getInstanceSlack(sta::Instance* inst);
  // Worst setup slack over the timing-checked data inputs of a register,
  // INF for other cells.  A capture-only register's outputs show none of it.
  sta::Slack captureSlack(sta::Instance* inst);
  // Setup TNS that swapping register inst to cell recovers at its data
  // inputs: the sum over its setup checks of the negative slack regained.
  // 0 when the swap pushes any setup or hold check into, or deeper into,
  // violation.
  float captureSetupGain(sta::Instance* inst, sta::LibertyCell* cell);
  // The VT equivalent of register inst with the largest captureSetupGain,
  // null when none gains.  The crit sweep otherwise picks VT by drive
  // strength, and a faster VT can have the larger setup time.
  sta::LibertyCell* bestCaptureVtCell(sta::Instance* inst);
};

}  // namespace rsz
