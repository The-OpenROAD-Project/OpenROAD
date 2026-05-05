// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include "RepairTargetCollector.hh"

namespace rsz {

class Resizer;

struct RepairSetupContext
{
  explicit RepairSetupContext(Resizer& resizer) : target_collector(&resizer) {}

  // Shared across setup phase policies so WNS/TNS endpoint state and final
  // tracker reports observe one repair_setup run, while each policy keeps its
  // own move generator list.
  RepairTargetCollector target_collector;
  bool fallback{false};
  float min_viol{0.0};
  float max_viol{0.0};
  int max_repairs_per_pass{1};
  int max_end_repairs{1};
  int overall_no_progress_count{0};
  double initial_design_area{0.0};
};

}  // namespace rsz
