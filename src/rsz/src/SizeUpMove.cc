// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "SizeUpMove.hh"

#include <cmath>

#include "BaseMove.hh"
#include "CloneMove.hh"

namespace rsz {

using std::string;

using utl::RSZ;

using sta::ArcDelay;
using sta::DcalcAnalysisPt;
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

bool SizeUpMove::doMove(const Path* drvr_path,
                        int drvr_index,
                        Slack drvr_slack,
                        PathExpanded* expanded,
                        float setup_slack_margin)
{
  Pin* drvr_pin = drvr_path->pin(this);
  Instance* drvr = network_->instance(drvr_pin);
  const DcalcAnalysisPt* dcalc_ap = drvr_path->dcalcAnalysisPt(sta_);
  const float load_cap = graph_delay_calc_->loadCap(drvr_pin, dcalc_ap);
  const int in_index = drvr_index - 1;
  const Path* in_path = expanded->path(in_index);
  Pin* in_pin = in_path->pin(sta_);
  LibertyPort* in_port = network_->libertyPort(in_pin);

  // We always size the cloned gates for some reason, but it would be good if we
  // also down-sized here instead since we might want smaller original.
  if (!resizer_->dontTouch(drvr)
      || resizer_->clone_move_->hasPendingMoves(drvr)) {
    float prev_drive;
    if (drvr_index >= 2) {
      const int prev_drvr_index = drvr_index - 2;
      const Path* prev_drvr_path = expanded->path(prev_drvr_index);
      Pin* prev_drvr_pin = prev_drvr_path->pin(sta_);
      prev_drive = 0.0;
      LibertyPort* prev_drvr_port = network_->libertyPort(prev_drvr_pin);
      if (prev_drvr_port) {
        prev_drive = prev_drvr_port->driveResistance();
      }
    } else {
      prev_drive = 0.0;
    }

    LibertyPort* drvr_port = network_->libertyPort(drvr_pin);
    LibertyCell* upsize
        = upsizeCell(in_port, drvr_port, load_cap, prev_drive, dcalc_ap);

    if (upsize && !resizer_->dontTouch(drvr) && replaceCell(drvr, upsize)) {
      debugPrint(logger_,
                 RSZ,
                 "opt_moves",
                 1,
                 "ACCEPT size_up {} {} -> {}",
                 network_->pathName(drvr_pin),
                 drvr_port->libertyCell()->name(),
                 upsize->name());
      debugPrint(logger_,
                 RSZ,
                 "repair_setup",
                 3,
                 "size_up {} {} -> {}",
                 network_->pathName(drvr_pin),
                 drvr_port->libertyCell()->name(),
                 upsize->name());
      addMove(drvr);
      return true;
    }
    debugPrint(logger_,
               RSZ,
               "opt_moves",
               3,
               "REJECT size_up {} {}",
               network_->pathName(drvr_pin),
               drvr_port->libertyCell()->name());
  }

  return false;
}

// namespace rsz
}  // namespace rsz
