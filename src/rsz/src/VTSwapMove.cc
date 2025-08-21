// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "VTSwapMove.hh"

#include <cmath>

#include "BaseMove.hh"

namespace rsz {

using sta::Instance;
using sta::InstancePinIterator;
using sta::LibertyCell;
using sta::LibertyPort;
using sta::LoadPinIndexMap;
using sta::NetConnectedPinIterator;
using sta::Path;
using sta::PathExpanded;
using sta::Pin;
using sta::Slack;
using sta::Slew;

bool VTSwapSpeedMove::doMove(const Path* drvr_path,
                             int drvr_index,
                             Slack drvr_slack,
                             PathExpanded* expanded,
                             float setup_slack_margin)
{
  int num_vt = resizer_->lib_data_->sorted_vt_categories.size();
  if (num_vt < 2) {
    return false;
  }
  Pin* drvr_pin = drvr_path->pin(this);
  if (drvr_pin == nullptr) {
    return false;
  }
  Instance* drvr = network_->instance(drvr_pin);
  if (drvr == nullptr) {
    return false;
  }
  if (resizer_->dontTouch(drvr)) {
    return false;
  }

  LibertyPort* drvr_port = network_->libertyPort(drvr_pin);
  if (drvr_port == nullptr) {
    return false;
  }
  LibertyCell* drvr_cell = drvr_port->libertyCell();
  if (drvr_cell == nullptr) {
    return false;
  }
  odb::dbMaster* drvr_master = db_network_->staToDb(drvr_cell);
  if (drvr_master == nullptr) {
    return false;
  }

  VTCategory vt_cat = resizer_->cellVTType(drvr_master);
  if (vt_cat.vt_index
      == resizer_->lib_data_->sorted_vt_categories[num_vt - 1].first.vt_index) {
    return false;
  }

  return false;
}

}  // namespace rsz
