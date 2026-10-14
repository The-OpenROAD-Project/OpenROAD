// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "grt/GlobalRouteOptions.h"

#include "utl/Logger.h"

namespace grt {

const utl::Schema<GlobalRouteOptions>& GlobalRouteOptions::schema()
{
  using Options = GlobalRouteOptions;
  static const utl::Schema<Options> schema
      = utl::Schema<Options>(utl::GRT, 400)
            .field("allow_congestion",
                   &Options::allow_congestion,
                   "Finish with congestion left instead of failing.")
            .field("congestion_iterations",
                   &Options::congestion_iterations,
                   401,
                   "Rip-up and reroute iterations to remove congestion; "
                   "CUGR saturates around 5.",
                   utl::NonNegative{})
            .field("congestion_report_iter_step",
                   &Options::congestion_report_iter_step,
                   402,
                   "Iterations between congestion reports; 0 for none.",
                   utl::NonNegative{})
            .field("congestion_report_file",
                   &Options::congestion_report_file,
                   "Congestion report file for the DRC viewer; empty for "
                   "none.")
            .field("grid_origin",
                   &Options::grid_origin,
                   55,
                   "Origin {x y} of the routing grid in DBU.")
            .field("critical_nets_percentage",
                   &Options::critical_nets_percentage,
                   403,
                   "Share of the nets with the worst slack that get "
                   "preference during congestion iterations; unset keeps "
                   "the router's current setting.",
                   utl::Percent{})
            .field("res_aware_nets_percentage",
                   &Options::res_aware_nets_percentage,
                   404,
                   "Share of the nets routed resistance-aware; needs "
                   "resistance_aware. Unset keeps the router's default.",
                   utl::Percent{})
            .field("resistance_aware",
                   &Options::resistance_aware,
                   "Resistance-aware layer assignment and 3D routing "
                   "(experimental).")
            .field("snapshot_batched_width",
                   &Options::snapshot_batched_width,
                   405,
                   "Batches per wave of snapshot-batched routing; 0 for the "
                   "non-batched behavior (research use).",
                   utl::NonNegative{})
            .field("skip_large_fanout_nets",
                   &Options::skip_large_fanout_nets,
                   406,
                   "Nets with more pins than this are not routed.",
                   utl::NonNegative{})
            .field("infinite_capacity",
                   &Options::infinite_capacity,
                   "Unlimited gcell capacity, for testing.")
            .field("verbose", &Options::verbose, "Report the full routing log.")
            .field("use_cugr",
                   &Options::use_cugr,
                   "Route with CUGR instead of FastRoute; unset keeps the "
                   "engine selected earlier (not ready for production).")
            .field("save_guides",
                   &Options::save_guides,
                   "Write the routes to the database as guides.");
  return schema;
}

}  // namespace grt
