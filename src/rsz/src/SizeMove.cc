// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "SizeMove.hh"

#include <algorithm>
#include <cmath>

#include "BaseMove.hh"
#include "CloneMove.hh"

namespace rsz {

using std::string;

using odb::dbInst;
using odb::dbMaster;

using utl::RSZ;

using sta::ArcDelay;
using sta::Cell;
using sta::DcalcAnalysisPt;
using sta::Instance;
using sta::InstancePinIterator;
using sta::LibertyCell;
using sta::LibertyPort;
using sta::LoadPinIndexMap;
using sta::Net;
using sta::NetConnectedPinIterator;
using sta::Path;
using sta::PathExpanded;
using sta::Pin;
using sta::Slack;
using sta::Slew;

bool SizeMove::doMove(const Path* drvr_path,
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
      || resizer_->clone_move->hasPendingMoves(drvr)) {
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

    if (upsize) {
      debugPrint(logger_,
                 RSZ,
                 "repair_setup",
                 3,
                 "resize {} {} -> {}",
                 network_->pathName(drvr_pin),
                 drvr_port->libertyCell()->name(),
                 upsize->name());
      if (!resizer_->dontTouch(drvr) && replaceCell(drvr, upsize)) {
        debugPrint(logger_,
                   RSZ,
                   "moves",
                   1,
                   "size_move {} {} -> {}",
                   network_->pathName(drvr_pin),
                   drvr_port->libertyCell()->name(),
                   upsize->name());
        addMove(drvr);
        return true;
      }
    }
  }

  return false;
}

LibertyCell* SizeMove::upsizeCell(LibertyPort* in_port,
                                  LibertyPort* drvr_port,
                                  const float load_cap,
                                  const float prev_drive,
                                  const DcalcAnalysisPt* dcalc_ap)
{
  const int lib_ap = dcalc_ap->libertyIndex();
  LibertyCell* cell = drvr_port->libertyCell();
  LibertyCellSeq swappable_cells = resizer_->getSwappableCells(cell);
  if (!swappable_cells.empty()) {
    const char* in_port_name = in_port->name();
    const char* drvr_port_name = drvr_port->name();
    sort(swappable_cells,
         [=](const LibertyCell* cell1, const LibertyCell* cell2) {
           LibertyPort* port1
               = cell1->findLibertyPort(drvr_port_name)->cornerPort(lib_ap);
           LibertyPort* port2
               = cell2->findLibertyPort(drvr_port_name)->cornerPort(lib_ap);
           const float drive1 = port1->driveResistance();
           const float drive2 = port2->driveResistance();
           const ArcDelay intrinsic1 = port1->intrinsicDelay(this);
           const ArcDelay intrinsic2 = port2->intrinsicDelay(this);
           return drive1 > drive2
                  || ((drive1 == drive2 && intrinsic1 < intrinsic2)
                      || (intrinsic1 == intrinsic2
                          && port1->capacitance() < port2->capacitance()));
         });
    const float drive = drvr_port->cornerPort(lib_ap)->driveResistance();
    const float delay
        = resizer_->gateDelay(drvr_port, load_cap, resizer_->tgt_slew_dcalc_ap_)
          + prev_drive * in_port->cornerPort(lib_ap)->capacitance();

    for (LibertyCell* swappable : swappable_cells) {
      LibertyCell* swappable_corner = swappable->cornerCell(lib_ap);
      LibertyPort* swappable_drvr
          = swappable_corner->findLibertyPort(drvr_port_name);
      LibertyPort* swappable_input
          = swappable_corner->findLibertyPort(in_port_name);
      const float swappable_drive = swappable_drvr->driveResistance();
      // Include delay of previous driver into swappable gate.
      const float swappable_delay
          = resizer_->gateDelay(swappable_drvr, load_cap, dcalc_ap)
            + prev_drive * swappable_input->capacitance();
      if (!resizer_->dontUse(swappable) && swappable_drive < drive
          && swappable_delay < delay) {
        return swappable;
      }
    }
  }
  return nullptr;
};

// Replace LEF with LEF so ports stay aligned in instance.
bool SizeMove::replaceCell(Instance* inst, const LibertyCell* replacement)
{
  const char* replacement_name = replacement->name();
  dbMaster* replacement_master = db_->findMaster(replacement_name);

  if (replacement_master) {
    dbInst* dinst = db_network_->staToDb(inst);
    dbMaster* master = dinst->getMaster();
    resizer_->designAreaIncr(-area(master));
    Cell* replacement_cell1 = db_network_->dbToSta(replacement_master);
    sta_->replaceCell(inst, replacement_cell1);
    resizer_->designAreaIncr(area(replacement_master));

    // Legalize the position of the instance in case it leaves the die
    if (resizer_->getParasiticsSrc() == ParasiticsSrc::global_routing
        || resizer_->getParasiticsSrc() == ParasiticsSrc::detailed_routing) {
      opendp_->legalCellPos(db_network_->staToDb(inst));
    }
    if (resizer_->haveEstimatedParasitics()) {
      InstancePinIterator* pin_iter = network_->pinIterator(inst);
      while (pin_iter->hasNext()) {
        const Pin* pin = pin_iter->next();
        const Net* net = network_->net(pin);
        odb::dbNet* db_net = nullptr;
        odb::dbModNet* db_modnet = nullptr;
        db_network_->staToDb(net, db_net, db_modnet);
        // only work on dbnets
        resizer_->invalidateParasitics(pin, db_network_->dbToSta(db_net));
        //        invalidateParasitics(pin, net);
      }
      delete pin_iter;
    }

    return true;
  }
  return false;
}

// namespace rsz
}  // namespace rsz
