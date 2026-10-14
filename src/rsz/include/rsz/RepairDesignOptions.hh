// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <optional>

#include "utl/Validate.h"

namespace rsz {

// The options of repair_design.  Standalone from the command, or embedded in
// another tool's options (global_placement runs repair_design to find the
// slacks for timing-driven placement).
struct RepairDesignOptions
{
  // Maximum wire length in meters; 0 picks the length that minimizes the
  // wire delay for the set_wire_rc values.
  double max_wire_length = 0;
  // Margins in percent.
  double slew_margin = 0;
  double cap_margin = 0;
  // Fraction of the core area that may be used, in percent.  Unset leaves
  // the resizer's current limit.
  std::optional<double> max_utilization = std::nullopt;
  // Run an initial sizing and buffering round before placement.
  bool pre_placement = false;
  bool match_cell_footprint = false;
  // Resistance-aware rerouting of nets with slew violations (post-GRT).
  bool reroute = false;
  bool verbose = false;

  static const utl::Schema<RepairDesignOptions>& schema();
};

}  // namespace rsz
