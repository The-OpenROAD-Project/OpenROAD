// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <unordered_map>

#include "dpl/Opendp.h"
#include "odb/util.h"
#include "utl/Logger.h"

// My stuff.
#include "graphics/DplObserver.h"
#include "legalize_shift.h"
#include "optimization/detailed.h"
#include "optimization/detailed_manager.h"

namespace dpl {

////////////////////////////////////////////////////////////////
void Opendp::improvePlacement(const int seed,
                              const int max_displacement_x,
                              const int max_displacement_y)
{
  logger_->report("Detailed placement improvement.");

  odb::WireLengthEvaluator eval(db_->getChip()->getBlock());
  int64_t hpwlBefore_x = 0;
  int64_t hpwlBefore_y = 0;
  const int64_t hpwlBefore = eval.hpwl(hpwlBefore_x, hpwlBefore_y);

  if (hpwlBefore == 0) {
    logger_->report("Skipping detailed improvement since hpwl is zero.");
    return;
  }

  // Get needed information from DB.
  importDb();
  // TODO: adjustNodesOrient() but it's currently causing an unrelated CI
  // failure
  initGrid();

  const bool disallow_one_site_gaps = !odb::hasOneSiteMaster(db_);

  // A manager to track cells.
  DetailedMgr mgr(arch_.get(), network_.get(), grid_.get(), drc_engine_.get());
  mgr.setLogger(logger_);
  mgr.setGlobalSwapParams(global_swap_params_);
  mgr.setExtraDplEnabled(extra_dpl_enabled_);
  // Various settings.
  mgr.setSeed(seed);
  mgr.setMaxDisplacement(max_displacement_x, max_displacement_y);
  mgr.setDisallowOneSiteGaps(disallow_one_site_gaps);

  // Legalization.  Doesn't particularly do much.  It only
  // populates the data structures required for detailed
  // improvement.  If it errors or prints a warning when
  // given a legal placement, that likely means there is
  // a bug in my code somewhere.
  ShiftLegalizer lg;
  lg.legalize(mgr);
  setFixedGridCells();

  // Detailed improvement.  Runs through a number of different
  // optimizations aimed at wirelength improvement.  The last
  // call to the random improver can be set to consider things
  // like density, displacement, etc. in addition to wirelength.
  // Everything done through a script string.

  DetailedParams dtParams;
  dtParams.script = "";
  // Maximum independent set matching.
  dtParams.script += "mis -p 10 -t 0.005;";
  // Global swaps.
  dtParams.script += "gs -p 10 -t 0.005;";
  // Vertical swaps.
  dtParams.script += "vs -p 10 -t 0.005;";
  // Small reordering.
  dtParams.script += "ro -p 10 -t 0.005;";
  // Random moves and swaps with hpwl as a cost function.  Use
  // random moves and hpwl objective right now.
  dtParams.script += "default -p 5 -f 20 -gen rng -obj hpwl -cost (hpwl);";

  if (disallow_one_site_gaps) {
    dtParams.script += "disallow_one_site_gaps;";
  }

  if (debug_observer_) {
    logger_->report("Pause before improve placement.");
    debug_observer_->redrawAndPause();
  }

  // Snapshot positions before improvement for displacement tracking.
  odb::dbBlock* block = db_->getChip()->getBlock();
  std::unordered_map<odb::dbInst*, odb::Point> pos_before;
  for (odb::dbInst* inst : block->getInsts()) {
    if (!inst->isFixed()) {
      int x, y;
      inst->getLocation(x, y);
      pos_before[inst] = {x, y};
    }
  }

  // Run the script.
  Detailed dt(dtParams);
  dt.improve(mgr);

  if (debug_observer_) {
    logger_->report("Pause after improve placement.");
    debug_observer_->redrawAndPause();
  }

  // Write solution back.
  updateDbInstLocations();

  // Compute displacement stats.
  int64_t disp_sum = 0;
  int64_t disp_max = 0;
  int moved = 0;
  for (auto& [inst, before] : pos_before) {
    int x, y;
    inst->getLocation(x, y);
    const int64_t disp
        = std::abs(x - before.x()) + std::abs(y - before.y());
    if (disp > 0) {
      disp_sum += disp;
      disp_max = std::max(disp_max, disp);
      moved++;
    }
  }
  const double dbu_micron = db_->getTech()->getDbUnitsPerMicron();
  const double disp_total_um = disp_sum / dbu_micron;
  const double disp_avg_um = moved > 0 ? disp_sum / (dbu_micron * moved) : 0.0;
  const double disp_max_um = disp_max / dbu_micron;

  // Get final hpwl.
  int64_t hpwlAfter_x = 0;
  int64_t hpwlAfter_y = 0;
  const int64_t hpwlAfter = eval.hpwl(hpwlAfter_x, hpwlAfter_y);
  const int hpwl_delta_pct
      = (hpwlBefore == 0)
            ? 0
            : round((hpwlAfter - hpwlBefore) / (double) hpwlBefore * 100);

  const int total_attempts = mgr.getTotalAttempts();
  logger_->report("Placement Analysis");
  logger_->report("---------------------------------");
  logger_->report("total attempts       {:10d}", total_attempts);
  logger_->report("relocated cells      {:10d}", moved);
  logger_->report("total displacement   {:10.1f} u", disp_total_um);
  logger_->report("average displacement {:10.1f} u", disp_avg_um);
  logger_->report("max displacement     {:10.1f} u", disp_max_um);
  logger_->report("original HPWL        {:10.1f} u",
                  hpwlBefore / dbu_micron);
  logger_->report("improved HPWL        {:10.1f} u",
                  hpwlAfter / dbu_micron);
  logger_->report("delta HPWL           {:10} %", hpwl_delta_pct);
  logger_->report("");
  logger_->metric("dpo__total__attempts", total_attempts);
  logger_->metric("dpo__relocated__cells", moved);
  logger_->metric("dpo__design__instance__displacement__total", disp_total_um);
  logger_->metric("dpo__design__instance__displacement__mean", disp_avg_um);
  logger_->metric("dpo__design__instance__displacement__max", disp_max_um);
  logger_->metric("dpo__hpwl__delta__percent", hpwl_delta_pct);
}

////////////////////////////////////////////////////////////////
}  // namespace dpl
