// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "SizeUpMove.hh"

#include <cmath>
#include <string>

#include "CloneMove.hh"
#include "sta/ArcDelayCalc.hh"
#include "sta/Graph.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/Liberty.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "utl/Logger.h"

namespace rsz {

using std::string;

using utl::RSZ;

bool SizeUpMove::doMove(const sta::Path* drvr_path, float setup_slack_margin)
{
  const sta::Pin* drvr_pin = drvr_path->pin(sta_);
  sta::Instance* drvr = network_->instance(drvr_pin);

  if (resizer_->dontTouch(drvr)) {
    debugPrint(logger_,
               RSZ,
               "size_up_move",
               2,
               "REJECT SizeUpMove {}: {} is \"don't touch\"",
               network_->pathName(drvr_pin),
               network_->pathName(drvr));
    return false;
  }

  if (!resizer_->isLogicStdCell(drvr)) {
    debugPrint(logger_,
               RSZ,
               "size_up_move",
               2,
               "REJECT SizeUpMove {}: {} isn't logic std cell",
               network_->pathName(drvr_pin),
               network_->pathName(drvr));
    return false;
  }

  sta::Pin* drvr_input_pin = drvr_path->prevPath()->pin(sta_);
  sta::Path* prev_drvr_path = drvr_path->prevPath()->prevPath();
  sta::Pin* prev_drvr_pin
      = prev_drvr_path ? prev_drvr_path->pin(sta_) : nullptr;

  float prev_drive;
  if (prev_drvr_pin) {
    prev_drive = 0.0;
    sta::LibertyPort* prev_drvr_port = network_->libertyPort(prev_drvr_pin);
    if (prev_drvr_port) {
      prev_drive = prev_drvr_port->driveResistance();
    }
  } else {
    prev_drive = 0.0;
  }

  sta::Scene* scene = drvr_path->scene(sta_);
  const sta::MinMax* min_max = drvr_path->minMax(sta_);
  const float load_cap = graph_delay_calc_->loadCap(drvr_pin, scene, min_max);
  sta::LibertyPort* in_port = network_->libertyPort(drvr_input_pin);
  sta::LibertyPort* drvr_port = network_->libertyPort(drvr_pin);
  sta::LibertyCell* upsize
      = upsizeCell(in_port, drvr_port, load_cap, prev_drive, scene, min_max);

  if (!upsize) {
    debugPrint(logger_,
               RSZ,
               "size_up_move",
               2,
               "REJECT SizeUpMove {}: Couldn't upsize cell {}",
               network_->pathName(drvr_pin),
               drvr_port->libertyCell()->name());
    return false;
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
    return false;
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
  return true;
}

// Compare drive strength with previous stage
// and match if needed
// For example, if BUFX16 drives BUFX1, replace BUFX1 with BUFX16
bool SizeUpMatchMove::doMove(const sta::Path* drvr_path,
                             float setup_slack_margin)
{
  sta::Pin* drvr_pin = drvr_path->pin(sta_);
  sta::Path* prev_drvr_path = drvr_path->prevPath()->prevPath();
  sta::Pin* prev_drvr_pin
      = prev_drvr_path ? prev_drvr_path->pin(sta_) : nullptr;

  if (prev_drvr_pin == nullptr) {
    debugPrint(logger_,
               RSZ,
               "size_up_match_move",
               2,
               "REJECT SizeUpMatchMove {}: No previous driver pin",
               network_->pathName(drvr_pin));
    return false;
  }

  if (drvr_pin == nullptr) {
    debugPrint(logger_,
               RSZ,
               "size_up_match_move",
               2,
               "REJECT SizeUpMatchMove {}: No driver pin",
               network_->pathName(drvr_pin));
    return false;
  }

  sta::Instance* drvr = network_->instance(drvr_pin);

  if (drvr == nullptr) {
    debugPrint(logger_,
               RSZ,
               "size_up_match_move",
               2,
               "REJECT SizeUpMatchMove {}: No driver instance",
               network_->pathName(drvr_pin));
    return false;
  }

  if (resizer_->dontTouch(drvr)) {
    debugPrint(logger_,
               RSZ,
               "size_up_match_move",
               2,
               "REJECT SizeUpMatchMove {}: {} is \"don't touch\"",
               network_->pathName(drvr_pin),
               network_->pathName(drvr));
    return false;
  }

  if (!resizer_->isLogicStdCell(drvr)) {
    debugPrint(logger_,
               RSZ,
               "size_up_match_move",
               2,
               "REJECT SizeUpMatchMove {}: {} isn't logic std cell",
               network_->pathName(drvr_pin),
               network_->pathName(drvr));
    return false;
  }

  sta::LibertyPort* drvr_port = network_->libertyPort(drvr_pin);
  const sta::LibertyCell* drvr_cell
      = drvr_port ? drvr_port->libertyCell() : nullptr;

  if (drvr_cell == nullptr) {
    debugPrint(logger_,
               RSZ,
               "size_up_match_move",
               2,
               "REJECT SizeUpMatchMove {}: No liberty cell found for {}",
               network_->pathName(drvr_pin),
               network_->pathName(drvr));
    return false;
  }

  sta::Vertex* prev_drvr_vertex = graph_->pinDrvrVertex(prev_drvr_pin);
  if (prev_drvr_vertex == nullptr) {
    debugPrint(logger_,
               RSZ,
               "size_up_match_move",
               2,
               "REJECT SizeUpMatchMove {}: No previous driver vertex",
               network_->pathName(drvr_pin));
    return false;
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
    return false;
  }

  sta::Instance* prev_drvr = network_->instance(prev_drvr_pin);

  if (prev_drvr == nullptr) {
    debugPrint(logger_,
               RSZ,
               "size_up_match_move",
               2,
               "REJECT SizeUpMatchMove {}: No previous driver instance",
               network_->pathName(drvr_pin));
    return false;
  }

  sta::LibertyPort* prev_drvr_port = network_->libertyPort(prev_drvr_pin);
  const sta::LibertyCell* prev_cell
      = prev_drvr_port ? prev_drvr_port->libertyCell() : nullptr;

  if (prev_cell == nullptr) {
    debugPrint(logger_,
               RSZ,
               "size_up_match_move",
               2,
               "REJECT SizeUpMatchMove {}: No liberty cell found for {}",
               network_->pathName(drvr_pin),
               network_->pathName(prev_drvr));
    return false;
  }

  if (prev_cell == drvr_cell) {
    debugPrint(logger_,
               RSZ,
               "size_up_match_move",
               2,
               "REJECT SizeUpMatchMove {}: Two cells are the same ({})",
               network_->pathName(drvr_pin),
               drvr_cell->name());
    return false;
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
    return false;
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
    return false;
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
    return false;
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
  return true;
}

}  // namespace rsz
