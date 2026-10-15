// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <array>
#include <limits>
#include <optional>
#include <string>

#include "utl/Validate.h"

namespace grt {

// The options of global_route.  Standalone from the command, or embedded in
// another tool's options (global_placement routes for congestion when it is
// not using RUDY, and for the congestion heatmap).
struct GlobalRouteOptions
{
  // Finish with congestion left instead of failing.
  bool allow_congestion = false;
  // Rip-up and reroute iterations; CUGR saturates around 5.
  int congestion_iterations = 50;
  int congestion_report_iter_step = 0;
  // Congestion report for the DRC viewer; empty for none.
  std::string congestion_report_file = "";
  // Origin of the routing grid in DBU.
  std::array<int, 2> grid_origin{0, 0};
  // Share of the nets with the worst slack that get routing preference;
  // unset keeps the router's current setting.
  std::optional<float> critical_nets_percentage = std::nullopt;
  // Share of the nets routed resistance-aware; unset keeps the router's
  // default.
  std::optional<float> res_aware_nets_percentage = std::nullopt;
  bool resistance_aware = false;
  int snapshot_batched_width = 0;
  // Nets with more pins are not routed.
  int skip_large_fanout_nets = std::numeric_limits<int>::max();
  bool infinite_capacity = false;
  bool verbose = false;
  // Route with CUGR instead of FastRoute; unset keeps the selected engine.
  std::optional<bool> use_cugr = std::nullopt;
  // Write the routes to the database as guides.
  bool save_guides = true;

  static const utl::Schema<GlobalRouteOptions>& schema();
};

}  // namespace grt
