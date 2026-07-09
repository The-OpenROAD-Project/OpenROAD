// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <vector>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "sta/Delay.hh"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "sta/NetworkClass.hh"
#include "utl/Logger.h"

namespace est {
class EstimateParasitics;
}

namespace rsz {

class Resizer;

// Leakage-power recovery: spend POSITIVE timing slack to reduce static
// (leakage) power by swapping logic cells to their lowest-leakage,
// same-footprint Vt variant -- but only where doing so does not degrade
// timing (WNS/TNS) or push the swapped cell below the requested slack
// margin.
//
// This reuses the resizer's existing cell-swap (replaceCell) and the STA
// timing engine for the slack checks; it does NOT reimplement timing.
// set_dont_touch / set_dont_use are respected.
class OptimizePower : public sta::dbStaState
{
 public:
  explicit OptimizePower(Resizer* resizer);

  // Returns true if any cell was swapped (design changed).
  // slack_margin: only cells whose worst slack stays >= slack_margin after
  // the swap are accepted (and only cells with slack > slack_margin are
  // considered). leakage-only is the only mode implemented in this slice.
  bool optimizePower(float slack_margin, bool verbose);

 private:
  void init();

  // Worst (min) setup slack over all output-pin vertices of inst.
  sta::Slack instanceWorstSlack(sta::Instance* inst) const;

  // The lowest-leakage same-footprint Vt variant of inst's cell, or nullptr
  // if none exists / not swappable / guarded.
  sta::LibertyCell* lowestLeakageVariant(sta::Instance* inst,
                                         sta::LibertyCell* curr_cell) const;

  utl::Logger* logger_ = nullptr;
  sta::dbNetwork* db_network_ = nullptr;
  Resizer* resizer_;
  est::EstimateParasitics* estimate_parasitics_ = nullptr;
  const sta::MinMax* max_ = sta::MinMax::max();

  int swap_count_ = 0;
};

}  // namespace rsz
