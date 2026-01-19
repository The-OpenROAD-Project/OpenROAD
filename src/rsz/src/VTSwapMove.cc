// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "VTSwapMove.hh"

#include <cmath>
#include <unordered_set>

#include "BaseMove.hh"
#include "odb/db.h"
#include "sta/Delay.hh"
#include "sta/Liberty.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "utl/Logger.h"

namespace rsz {

using utl::RSZ;

using sta::Instance;
using sta::LibertyCell;
using sta::LibertyPort;
using sta::Path;
using sta::Pin;
using sta::Slack;
using sta::Slew;

bool VTSwapSpeedMove::doMove(const Pin* drvr_pin, float setup_slack_margin)
{
  startMove(drvr_pin);

  LibertyCell* drvr_cell;
  LibertyCell* best_cell;

  if (!isSwappable(drvr_pin, drvr_cell, best_cell)) {
    debugPrint(logger_,
               RSZ,
               "vt_swap_speed_move",
               2,
               "REJECT VTSwapSpeedMove {}: {} has no swappable cell",
               network_->pathName(drvr_pin),
               drvr_cell->name());
    return endMove(false);
  }

  // TODO: Avoid swapping to the lowest VT by considering slack
  Instance* drvr = network_->instance(drvr_pin);
  if (!replaceCell(drvr, best_cell)) {
    debugPrint(logger_,
               RSZ,
               "vt_swap_speed_move",
               2,
               "REJECT VTSwapSpeedMove {}: Failed to swap {} -> {}",
               network_->pathName(drvr_pin),
               drvr_cell->name(),
               best_cell->name());
    return endMove(false);
  }

  debugPrint(logger_,
             RSZ,
             "vt_swap_speed_move",
             1,
             "ACCEPT VTSwapSpeedMove {}: {} -> {}",
             network_->pathName(drvr_pin),
             drvr_cell->name(),
             best_cell->name());
  countMove(drvr);
  return endMove(true);
}

// This is a special move used during separate critical cell VT swap routine
bool VTSwapSpeedMove::doMove(Instance* drvr,
                             std::unordered_set<Instance*>& notSwappable)
{
  LibertyCell* best_lib_cell;
  if (resizer_->checkAndMarkVTSwappable(drvr, notSwappable, best_lib_cell)) {
    if (replaceCell(drvr, best_lib_cell)) {
      countMove(drvr);
      debugPrint(logger_,
                 RSZ,
                 "vt_swap_speed_move",
                 1,
                 "ACCEPT VTSwapSpeedMove {}: {} -> {}",
                 network_->pathName(drvr),
                 network_->pathName(drvr),
                 best_lib_cell->name());
      return true;
    }
  }

  debugPrint(logger_,
             RSZ,
             "vt_swap_speed_move",
             2,
             "REJECT VTSwapSpeedMove {}: Failed to swap {} -> {}",
             network_->pathName(drvr),
             network_->pathName(drvr),
             best_lib_cell->name());
  return false;
}

bool VTSwapSpeedMove::isSwappable(const Pin*& drvr_pin,
                                  LibertyCell*& drvr_cell,
                                  LibertyCell*& best_cell)
{
  drvr_cell = nullptr;
  best_cell = nullptr;

  if (drvr_pin == nullptr) {
    debugPrint(logger_,
               RSZ,
               "vt_swap_speed_move",
               2,
               "REJECT VTSwapSpeedMove {}: No driver pin found",
               network_->pathName(drvr_pin));
    return false;
  }

  Instance* drvr = network_->instance(drvr_pin);
  int num_vt = resizer_->lib_data_->sorted_vt_categories.size();
  if (num_vt < 2) {
    debugPrint(logger_,
               RSZ,
               "vt_swap_speed_move",
               2,
               "REJECT VTSwapSpeedMove {}: Insufficient VT types",
               network_->pathName(drvr_pin));
    return false;
  }
  if (drvr == nullptr) {
    debugPrint(logger_,
               RSZ,
               "vt_swap_speed_move",
               2,
               "REJECT VTSwapSpeedMove {}: No driver instance found",
               network_->pathName(drvr_pin));
    return false;
  }
  if (resizer_->dontTouch(drvr)) {
    debugPrint(logger_,
               RSZ,
               "vt_swap_speed_move",
               2,
               "REJECT VTSwapSpeedMove {}: {} is \"don't touch\"",
               network_->pathName(drvr_pin),
               network_->pathName(drvr));
    return false;
  }
  if (!resizer_->isLogicStdCell(drvr)) {
    debugPrint(logger_,
               RSZ,
               "vt_swap_speed_move",
               2,
               "REJECT VTSwapSpeedMove {}: {} isn't logic std cell",
               network_->pathName(drvr_pin),
               network_->pathName(drvr));
    return false;
  }
  LibertyPort* drvr_port = network_->libertyPort(drvr_pin);
  drvr_cell = drvr_port ? drvr_port->libertyCell() : nullptr;
  if (drvr_cell == nullptr) {
    debugPrint(logger_,
               RSZ,
               "vt_swap_speed_move",
               2,
               "REJECT VTSwapSpeedMove {}: No liberty cell found for {}",
               network_->pathName(drvr_pin),
               network_->pathName(drvr));
    return false;
  }
  odb::dbMaster* drvr_master = db_network_->staToDb(drvr_cell);
  if (drvr_master == nullptr) {
    debugPrint(logger_,
               RSZ,
               "vt_swap_speed_move",
               2,
               "REJECT VTSwapSpeedMove {}: Driver pin has no LEF master",
               network_->pathName(drvr_pin));
    return false;
  }

  LibertyCellSeq equiv_cells = resizer_->getVTEquivCells(drvr_cell);
  best_cell = equiv_cells.empty() ? nullptr : equiv_cells.back();
  if (best_cell == drvr_cell) {
    best_cell = nullptr;
  }
  if (best_cell == nullptr) {
    debugPrint(logger_,
               RSZ,
               "vt_swap_speed_move",
               2,
               "REJECT VTSwapSpeedMove {}: Driver pin cannot be swapped "
               "to lower VT from {}",
               network_->pathName(drvr_pin),
               drvr_cell->name());
    return false;
  }
  return true;
}

}  // namespace rsz
