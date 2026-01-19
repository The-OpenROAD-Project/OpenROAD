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

bool SizeUpMove::doMove(const Pin* drvr_pin, float setup_slack_margin)
{
  startMove(drvr_pin);

  Instance* drvr = network_->instance(drvr_pin);

  if (resizer_->dontTouch(drvr)) {
    debugPrint(logger_,
               RSZ,
               "size_up_move",
               2,
               "REJECT SizeUpMove {}: {} is \"don't touch\"",
               network_->pathName(drvr_pin),
               network_->pathName(drvr));
    return endMove(false);
  }

  if (!resizer_->isLogicStdCell(drvr)) {
    debugPrint(logger_,
               RSZ,
               "size_up_move",
               2,
               "REJECT SizeUpMove {}: {} isn't logic std cell",
               network_->pathName(drvr_pin),
               network_->pathName(drvr));
    return endMove(false);
  }

  Pin* prev_drvr_pin;
  Pin* drvr_input_pin;
  Pin* load_pin;
  getPrevNextPins(drvr_pin, prev_drvr_pin, drvr_input_pin, load_pin);

  float prev_drive;
  if (prev_drvr_pin) {
    prev_drive = 0.0;
    LibertyPort* prev_drvr_port = network_->libertyPort(prev_drvr_pin);
    if (prev_drvr_port) {
      prev_drive = prev_drvr_port->driveResistance();
    }
  } else {
    prev_drive = 0.0;
  }

  const DcalcAnalysisPt* dcalc_ap = resizer_->tgt_slew_dcalc_ap_;
  const float load_cap = graph_delay_calc_->loadCap(drvr_pin, dcalc_ap);
  LibertyPort* in_port = network_->libertyPort(drvr_input_pin);
  LibertyPort* drvr_port = network_->libertyPort(drvr_pin);
  LibertyCell* upsize
      = upsizeCell(in_port, drvr_port, load_cap, prev_drive, dcalc_ap);

  if (!upsize) {
    debugPrint(logger_,
               RSZ,
               "size_up_move",
               2,
               "REJECT SizeUpMove {}: Couldn't upsize cell {}",
               network_->pathName(drvr_pin),
               drvr_port->libertyCell()->name());
    return endMove(false);
  }

  if (!replaceCell(drvr, upsize)) {
    debugPrint(logger_,
               RSZ,
               "size_up_move",
               2,
               "REJECT SizeUpMove {}: Couldn't replace cell {} with {}",
               network_->pathName(drvr_pin),
               drvr_port->libertyCell()->name(),
               upsize->name());
    return endMove(false);
  }

  debugPrint(logger_,
             RSZ,
             "size_up_move",
             1,
             "ACCEPT SizeUpMove {}: {} -> {}",
             network_->pathName(drvr_pin),
             drvr_port->libertyCell()->name(),
             upsize->name());
  countMove(drvr);
  return endMove(true);
}

// Compare drive strength with previous stage
// and match if needed
// For example, if BUFX16 drives BUFX1, replace BUFX1 with BUFX16
bool SizeUpMatchMove::doMove(const Pin* drvr_pin, float setup_slack_margin)
{
  startMove(drvr_pin);

  Pin* prev_drvr_pin;
  Pin* drvr_input_pin;
  Pin* load_pin;
  getPrevNextPins(drvr_pin, prev_drvr_pin, drvr_input_pin, load_pin);

  if (prev_drvr_pin == nullptr) {
    debugPrint(logger_,
               RSZ,
               "size_up_match_move",
               2,
               "REJECT SizeUpMatchMove {}: No previous driver pin",
               network_->pathName(drvr_pin));
    return endMove(false);
  }

  if (drvr_pin == nullptr) {
    debugPrint(logger_,
               RSZ,
               "size_up_match_move",
               2,
               "REJECT SizeUpMatchMove {}: No driver pin",
               network_->pathName(drvr_pin));
    return endMove(false);
  }

  Instance* drvr = network_->instance(drvr_pin);

  if (drvr == nullptr) {
    debugPrint(logger_,
               RSZ,
               "size_up_match_move",
               2,
               "REJECT SizeUpMatchMove {}: No driver instance",
               network_->pathName(drvr_pin));
    return endMove(false);
  }

  if (resizer_->dontTouch(drvr)) {
    debugPrint(logger_,
               RSZ,
               "size_up_match_move",
               2,
               "REJECT SizeUpMatchMove {}: {} is \"don't touch\"",
               network_->pathName(drvr_pin),
               network_->pathName(drvr));
    return endMove(false);
  }

  if (!resizer_->isLogicStdCell(drvr)) {
    debugPrint(logger_,
               RSZ,
               "size_up_match_move",
               2,
               "REJECT SizeUpMatchMove {}: {} isn't logic std cell",
               network_->pathName(drvr_pin),
               network_->pathName(drvr));
    return endMove(false);
  }

  LibertyPort* drvr_port = network_->libertyPort(drvr_pin);
  const LibertyCell* drvr_cell = drvr_port ? drvr_port->libertyCell() : nullptr;

  if (drvr_cell == nullptr) {
    debugPrint(logger_,
               RSZ,
               "size_up_match_move",
               2,
               "REJECT SizeUpMatchMove {}: No liberty cell found for {}",
               network_->pathName(drvr_pin),
               network_->pathName(drvr));
    return endMove(false);
  }

  Vertex* prev_drvr_vertex = graph_->pinDrvrVertex(prev_drvr_pin);
  if (prev_drvr_vertex == nullptr) {
    debugPrint(logger_,
               RSZ,
               "size_up_match_move",
               2,
               "REJECT SizeUpMatchMove {}: No previous driver vertex",
               network_->pathName(drvr_pin));
    return endMove(false);
  }

  // Skip if the previous driver has multi fanout
  int prev_drvr_fanout = fanout(prev_drvr_vertex);
  if (prev_drvr_fanout > 1) {
    debugPrint(logger_,
               RSZ,
               "size_up_match_move",
               2,
               "REJECT SizeUpMatchMove {}: Previous driver fanout {} > 1",
               network_->pathName(drvr_pin),
               prev_drvr_fanout);
    return endMove(false);
  }

  Instance* prev_drvr = network_->instance(prev_drvr_pin);

  if (prev_drvr == nullptr) {
    debugPrint(logger_,
               RSZ,
               "size_up_match_move",
               2,
               "REJECT SizeUpMatchMove {}: No previous driver instance",
               network_->pathName(drvr_pin));
    return endMove(false);
  }

  LibertyPort* prev_drvr_port = network_->libertyPort(prev_drvr_pin);
  const LibertyCell* prev_cell
      = prev_drvr_port ? prev_drvr_port->libertyCell() : nullptr;

  if (prev_cell == nullptr) {
    debugPrint(logger_,
               RSZ,
               "size_up_match_move",
               2,
               "REJECT SizeUpMatchMove {}: No liberty cell found for {}",
               network_->pathName(drvr_pin),
               network_->pathName(prev_drvr));
    return endMove(false);
  }

  if (prev_cell == drvr_cell) {
    debugPrint(logger_,
               RSZ,
               "size_up_match_move",
               2,
               "REJECT SizeUpMatchMove {}: Two cells are the same ({})",
               network_->pathName(drvr_pin),
               drvr_cell->name());
    return endMove(false);
  }

  if (!(prev_cell->isBuffer() && drvr_cell->isBuffer())
      && !(prev_cell->isInverter() && drvr_cell->isInverter())) {
    debugPrint(logger_,
               RSZ,
               "size_up_match_move",
               2,
               "REJECT SizeUpMatchMove {}: Two cells aren't "
               "\"buffer -> buffer\" or \"inverter -> inverter\" ({} -> {})",
               network_->pathName(drvr_pin),
               prev_cell->name(),
               drvr_cell->name());
    return endMove(false);
  }

  const float prev_drive_res = resizer_->bufferDriveResistance(prev_cell);
  const float drive_res = resizer_->bufferDriveResistance(drvr_cell);

  if (prev_drive_res >= drive_res) {
    debugPrint(logger_,
               RSZ,
               "size_up_match_move",
               2,
               "REJECT SizeUpMatchMove {}: Previous cell drive resistance {} "
               ">= {} driver cell drive resistance",
               network_->pathName(drvr_pin),
               prev_drive_res,
               drive_res);
    return endMove(false);
  }

  if (!replaceCell(drvr, prev_cell)) {
    debugPrint(logger_,
               RSZ,
               "size_up_match_move",
               2,
               "REJECT SizeUpMatchMove {}: Couldn't replace {} -> {}",
               network_->pathName(drvr_pin),
               drvr_cell->name(),
               prev_cell->name());
    return endMove(false);
  }

  debugPrint(logger_,
             RSZ,
             "size_up_match_move",
             1,
             "ACCEPT SizeUpMatchMove {}: Replaced {} -> {}",
             network_->pathName(drvr_pin),
             drvr_cell->name(),
             prev_cell->name());
  countMove(drvr);
  return endMove(true);
}

}  // namespace rsz
