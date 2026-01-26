// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "VTSwapMove.hh"

#include <cmath>
#include <unordered_set>

#include "BaseMove.hh"
#include "odb/db.h"
#include "sta/Delay.hh"
#include "sta/Liberty.hh"
#include "sta/LibertyClass.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "sta/PathExpanded.hh"
#include "utl/Logger.h"

namespace rsz {

using sta::Instance;
using sta::LibertyCell;
using sta::LibertyPort;
using sta::Path;
using sta::Pin;
using sta::Slack;
using sta::Slew;

#define debugMovePrint1(format_str, ...) \
  debugPrint(logger_, utl::RSZ, "opt_moves", 1, format_str, ##__VA_ARGS__)
#define debugMovePrint3(format_str, ...) \
  debugPrint(logger_, utl::RSZ, "repair_setup", 3, format_str, ##__VA_ARGS__)

bool VTSwapSpeedMove::doMove(const Path* drvr_path,
                             int drvr_index,
                             Slack drvr_slack,
                             sta::PathExpanded* expanded,
                             float setup_slack_margin)
{
  Pin* drvr_pin;
  Instance* drvr;
  LibertyCell* drvr_cell;
  LibertyCell* best_cell;
  if (!isSwappable(drvr_path, drvr_pin, drvr, drvr_cell, best_cell)) {
    return false;
  }

  // TODO: Avoid swapping to the lowest VT by considering slack
  if (replaceCell(drvr, best_cell)) {
    debugMovePrint1("ACCEPT vt_swap {}: {} -> {}",
                    network_->pathName(drvr_pin),
                    drvr_cell->name(),
                    best_cell->name());
    debugMovePrint3("vt_swap {} {} -> {}",
                    network_->pathName(drvr_pin),
                    drvr_cell->name(),
                    best_cell->name());
    addMove(drvr);
    return true;
  }

  debugMovePrint1("REJECT vt_swap {}: {} -> {} swap failed",
                  network_->pathName(drvr_pin),
                  drvr_cell->name(),
                  best_cell->name());
  return false;
}

// This is a special move used during separate critical cell VT swap routine
bool VTSwapSpeedMove::doMove(Instance* drvr,
                             std::unordered_set<Instance*>& notSwappable)
{
  LibertyCell* best_lib_cell;
  if (resizer_->checkAndMarkVTSwappable(drvr, notSwappable, best_lib_cell)) {
    if (replaceCell(drvr, best_lib_cell)) {
      addMove(drvr);
      debugMovePrint1("ACCEPT vt_swap {}: -> {}",
                      network_->pathName(drvr),
                      best_lib_cell->name());
      debugMovePrint3(
          "vt_swap {} -> {}", network_->pathName(drvr), best_lib_cell->name());
      return true;
    }
  }

  debugMovePrint1("REJECT vt_swap {} failed", network_->pathName(drvr));
  return false;
}

bool VTSwapSpeedMove::isSwappable(const Path*& drvr_path,
                                  Pin*& drvr_pin,
                                  Instance*& drvr,
                                  LibertyCell*& drvr_cell,
                                  LibertyCell*& best_cell)
{
  drvr_pin = nullptr;
  drvr = nullptr;
  drvr_cell = nullptr;
  best_cell = nullptr;

  drvr_pin = drvr_path->pin(this);
  if (drvr_pin == nullptr) {
    debugMovePrint1("REJECT vt_swap: no drvr_pin found");
    return false;
  }
  int num_vt = resizer_->lib_data_->sorted_vt_categories.size();
  if (num_vt < 2) {
    debugMovePrint1("REJECT vt_swap {}: insufficient VT types",
                    network_->pathName(drvr_pin));
    return false;
  }
  drvr = network_->instance(drvr_pin);
  if (drvr == nullptr) {
    debugMovePrint1("REJECT vt_swap {}: drvr_pin has no instance",
                    network_->pathName(drvr_pin));
    return false;
  }
  if (resizer_->dontTouch(drvr)) {
    debugMovePrint1("REJECT vt_swap {}: drvr instance is dont-touch",
                    network_->pathName(drvr_pin));
    return false;
  }
  if (!resizer_->isLogicStdCell(drvr)) {
    debugMovePrint1("REJECT vt_swap {}: drvr instance is not a standard cell",
                    network_->pathName(drvr_pin));
    return false;
  }
  LibertyPort* drvr_port = network_->libertyPort(drvr_pin);
  if (drvr_port == nullptr) {
    debugMovePrint1("REJECT vt_swap {}: drvr pin has no library port",
                    network_->pathName(drvr_pin));
    return false;
  }
  drvr_cell = drvr_port->libertyCell();
  if (drvr_cell == nullptr) {
    debugMovePrint1("REJECT vt_swap {}: drvr pin has no library cell",
                    network_->pathName(drvr_pin));
    return false;
  }
  odb::dbMaster* drvr_master = db_network_->staToDb(drvr_cell);
  if (drvr_master == nullptr) {
    debugMovePrint1("REJECT vt_swap {}: drvr pin has no LEF master",
                    network_->pathName(drvr_pin));
    return false;
  }

  sta::LibertyCellSeq equiv_cells = resizer_->getVTEquivCells(drvr_cell);
  best_cell = equiv_cells.empty() ? nullptr : equiv_cells.back();
  if (best_cell == drvr_cell) {
    best_cell = nullptr;
  }
  if (best_cell == nullptr) {
    debugMovePrint1(
        "REJECT vt_swap {}: drvr pin cannot be swapped to lower VT from {}",
        network_->pathName(drvr_pin),
        drvr_cell->name());
    return false;
  }

  return true;
}

}  // namespace rsz
