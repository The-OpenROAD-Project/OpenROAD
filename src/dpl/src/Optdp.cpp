// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#include <cstdint>

#include "dpl/Opendp.h"
#include "odb/util.h"
#include "utl/Logger.h"

// My stuff.
#include "graphics/DplObserver.h"
#include "legalize_shift.h"
#include "optimization/detailed.h"
#include "optimization/detailed_manager.h"

namespace dpl {
namespace {
void reportHPWL(utl::Logger* logger,
                const double dbu_micron,
                const int64_t hpwlBefore,
                const int64_t hpwlAfter,
                const int64_t hpwlBefore_x,
                const int64_t hpwlAfter_x,
                const int64_t hpwlBefore_y,
                const int64_t hpwlAfter_y)
{
  logger->report("Detailed Improvement Results");
  logger->report("------------------------------------------");
  logger->report("Original HPWL         {:10.1f} u ({:10.1f}, {:10.1f})",
                 hpwlBefore / dbu_micron,
                 hpwlBefore_x / dbu_micron,
                 hpwlBefore_y / dbu_micron);
  logger->report("Final HPWL            {:10.1f} u ({:10.1f}, {:10.1f})",
                 hpwlAfter / dbu_micron,
                 hpwlAfter_x / dbu_micron,
                 hpwlAfter_y / dbu_micron);
  const double hpwl_delta = (hpwlAfter - hpwlBefore) / (double) hpwlBefore;
  const double hpwl_delta_x
      = (hpwlAfter_x - hpwlBefore_x) / (double) hpwlBefore_x;
  const double hpwl_delta_y
      = (hpwlAfter_y - hpwlBefore_y) / (double) hpwlBefore_y;
  logger->report("Delta HPWL            {:10.1f} % ({:10.1f}, {:10.1f})",
                 hpwl_delta * 100.0,
                 hpwl_delta_x * 100.0,
                 hpwl_delta_y * 100.0);
  logger->report("");
}
}  // namespace

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

  // Run the script.
  Detailed dt(dtParams);
  dt.improve(mgr);

  if (debug_observer_) {
    logger_->report("Pause after improve placement.");
    debug_observer_->redrawAndPause();
  }

  // Write solution back.
  updateDbInstLocations();

  // Get final hpwl.
  int64_t hpwlAfter_x = 0;
  int64_t hpwlAfter_y = 0;
  const int64_t hpwlAfter = eval.hpwl(hpwlAfter_x, hpwlAfter_y);
  const double dbu_micron = db_->getTech()->getDbUnitsPerMicron();
  reportHPWL(logger_,
             dbu_micron,
             hpwlBefore,
             hpwlAfter,
             hpwlBefore_x,
             hpwlAfter_x,
             hpwlBefore_y,
             hpwlAfter_y);
}

////////////////////////////////////////////////////////////////
}  // namespace dpl
