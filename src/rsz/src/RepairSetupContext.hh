// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include "RepairTargetCollector.hh"

namespace rsz {

class Resizer;

struct RepairSetupContext
{
  explicit RepairSetupContext(Resizer& resizer);

  // Shared across setup phase policies so WNS/TNS endpoint state and final
  // tracker reports observe one repair_setup run, while each policy keeps its
  // own move generator list.
  RepairTargetCollector target_collector;
  float min_viol{0.0};
  float max_viol{0.0};
  int max_repairs_per_pass{1};
  int max_end_repairs{1};
  int overall_no_progress_count{0};
  double initial_design_area{0.0};

  // Shared optimizer progress for phase pipelines.
  bool phase_pipeline_active{false};
  int phase_index{0};
  int iteration{0};
  int violation_count{0};
  float initial_tns{0.0f};
  float previous_tns{0.0f};
  bool progress_header_printed{false};
  // Design area for the progress table, and the committer's netlist edit
  // count it was computed at; -1 means not yet computed.
  double progress_design_area{0.0};
  int progress_area_at_edit{-1};

  // Legacy-derived setup phases share one preamble per repair_setup run.
  bool legacy_preamble_done{false};
  bool legacy_preamble_has_violations{true};
};

}  // namespace rsz
