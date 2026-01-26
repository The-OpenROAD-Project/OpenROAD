// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "SizeUpMove.hh"

#include <cmath>
#include <string>

#include "BaseMove.hh"
#include "CloneMove.hh"
#include "sta/ArcDelayCalc.hh"
#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "sta/PathExpanded.hh"
#include "utl/Logger.h"

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
using sta::VertexOutEdgeIterator;

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

  if (!resizer_->dontTouch(drvr) && resizer_->isLogicStdCell(drvr)) {
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

// Compare drive strength with previous stage
// and match if needed
// For example, if BUFX16 drives BUFX1, replace BUFX1 with BUFX16
bool SizeUpMatchMove::doMove(const Path* drvr_path,
                             int drvr_index,
                             Slack drvr_slack,
                             PathExpanded* expanded,
                             float setup_slack_margin)
{
  if (drvr_index < 2) {
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

  if (!resizer_->dontTouch(drvr) && resizer_->isLogicStdCell(drvr)) {
    LibertyPort* drvr_port = network_->libertyPort(drvr_pin);
    if (drvr_port == nullptr) {
      return false;
    }
    const LibertyCell* drvr_cell = drvr_port->libertyCell();
    if (drvr_cell == nullptr) {
      return false;
    }

    const int prev_drvr_index = drvr_index - 2;
    const Path* prev_drvr_path = expanded->path(prev_drvr_index);
    Pin* prev_drvr_pin = prev_drvr_path->pin(sta_);
    if (prev_drvr_pin == nullptr) {
      return false;
    }
    sta::Vertex* prev_drvr_vertex = graph_->pinDrvrVertex(prev_drvr_pin);
    if (prev_drvr_vertex == nullptr) {
      return false;
    }
    // Skip if the previous driver has multi fanout
    int fanout = 0;
    VertexOutEdgeIterator edge_iter(prev_drvr_vertex, graph_);
    while (edge_iter.hasNext()) {
      (void) edge_iter.next();
      fanout++;
      if (fanout > 1) {
        return false;
      }
    }

    LibertyPort* prev_drvr_port = network_->libertyPort(prev_drvr_pin);
    if (prev_drvr_port == nullptr) {
      return false;
    }
    LibertyCell* prev_cell = prev_drvr_port->libertyCell();
    if (prev_cell == nullptr) {
      return false;
    }

    if ((prev_cell != drvr_cell)
        && ((prev_cell->isBuffer() && drvr_cell->isBuffer())
            || (prev_cell->isInverter() && drvr_cell->isInverter()))
        && (resizer_->bufferDriveResistance(prev_cell)
            < resizer_->bufferDriveResistance(drvr_cell))) {
      if (replaceCell(drvr, prev_cell)) {
        debugPrint(logger_,
                   RSZ,
                   "opt_moves",
                   1,
                   "ACCEPT size_up_match {} {} -> {}",
                   network_->pathName(drvr_pin),
                   drvr_cell->name(),
                   prev_cell->name());
        debugPrint(logger_,
                   RSZ,
                   "repair_setup",
                   3,
                   "size_up_match {} {} -> {}",
                   network_->pathName(drvr_pin),
                   drvr_cell->name(),
                   prev_cell->name());
        addMove(drvr);
        return true;
      }
    }
    debugPrint(logger_,
               RSZ,
               "opt_moves",
               1,
               "REJECT size_up_match {} {}",
               network_->pathName(drvr_pin),
               drvr_cell->name());
  }

  return false;
}

}  // namespace rsz
