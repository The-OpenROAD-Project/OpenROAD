// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "rsz/RepairDesignOptions.hh"

#include "utl/Logger.h"

namespace rsz {

const utl::Schema<RepairDesignOptions>& RepairDesignOptions::schema()
{
  using Options = RepairDesignOptions;
  static const utl::Schema<Options> schema
      = utl::Schema<Options>(utl::RSZ, 420)
            .field("max_wire_length",
                   &Options::max_wire_length,
                   421,
                   "Maximum wire length; 0 picks the length that minimizes "
                   "the wire delay for the set_wire_rc values.",
                   utl::NonNegative{},
                   utl::Distance{})
            .field("slew_margin",
                   &Options::slew_margin,
                   422,
                   "Slew margin in percent, to over-repair for the "
                   "difference between estimated and routed parasitics.",
                   utl::Percent{})
            .field("cap_margin",
                   &Options::cap_margin,
                   423,
                   "Capacitance margin in percent.",
                   utl::Percent{})
            .field("max_utilization",
                   &Options::max_utilization,
                   424,
                   "Percentage of the core area that may be used.",
                   utl::Percent{})
            .field("pre_placement",
                   &Options::pre_placement,
                   "Run an initial sizing and buffering round before "
                   "placement.")
            .field("match_cell_footprint",
                   &Options::match_cell_footprint,
                   "Obey the Liberty cell footprint when swapping gates.")
            .field("reroute",
                   &Options::reroute,
                   "Resistance-aware rerouting of nets with slew violations "
                   "after global routing (experimental).")
            .field("verbose", &Options::verbose, "Report the repair progress.");
  return schema;
}

}  // namespace rsz
