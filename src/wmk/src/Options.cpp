// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
#include "Options.h"

#include <cmath>
#include <limits>
#include <string_view>

#include "sta/MinMax.hh"
#include "utl/Logger.h"
#include "wmk/Watermark.h"

namespace wmk {
namespace {

void checkRange(std::string_view name,
                double value,
                double lower,
                double upper,
                utl::Logger* logger,
                double scale = 1.0)
{
  const double scaled = value * scale;
  if (!std::isfinite(scaled) || scaled < lower || scaled > upper) {
    logger->error(utl::WMK,
                  113,
                  "{} must be finite and in [{:.6g}, {:.6g}]; got {:.6g}.",
                  name,
                  lower / scale,
                  upper / scale,
                  value);
  }
}

void checkDistance(std::string_view name,
                   double um,
                   int dbu,
                   utl::Logger* logger)
{
  // All placement distances must fit in signed OpenDB coordinates. Validate
  // the scaled value used by the cast, avoiding a rounded-up um bound.
  checkRange(name, um, 0.0, std::numeric_limits<int>::max(), logger, dbu);
}

void checkTime(std::string_view name, double ns, utl::Logger* logger)
{
  // Keep conversions and subsequent sums/differences away from STA's
  // unconstrained sentinel as well as floating-point overflow.
  checkRange(name, ns, 0.0, static_cast<double>(sta::INF) * 1e9 / 4, logger);
}

}  // namespace

void validateOptions(const PlacementOptions& opts, int dbu, utl::Logger* logger)
{
  const int max_count = std::numeric_limits<int>::max();
  checkRange("grid_nx", opts.grid_nx, 1, max_count, logger);
  checkRange("grid_ny", opts.grid_ny, 1, max_count, logger);
  checkRange("pairs_per_tile", opts.pairs_per_tile, 0, max_count, logger);
  checkRange("min_pairs_total", opts.min_pairs_total, 0, max_count, logger);
  checkDistance("pair_dist_um", opts.pair_dist_um, dbu, logger);
  checkDistance("hpwl_eps_um", opts.hpwl_eps_um, dbu, logger);
  checkDistance("max_disp_um", opts.max_disp_um, dbu, logger);
  checkTime("slack_threshold_ns", opts.slack_threshold_ns, logger);
  checkTime("guard_degrade_ns", opts.guard_degrade_ns, logger);
}

void validateOptions(const CtsOptions& opts, int dbu, utl::Logger* logger)
{
  checkRange(
      "num_pairs", opts.num_pairs, 0, std::numeric_limits<int>::max(), logger);
  checkDistance("sibling_dist_um", opts.sibling_dist_um, dbu, logger);
  checkTime("skew_margin_ns", opts.skew_margin_ns, logger);
  checkRange("slew_headroom_frac", opts.slew_headroom_frac, 0, 1, logger);
  checkRange("cap_headroom_frac", opts.cap_headroom_frac, 0, 1, logger);
}
}  // namespace wmk
