// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "CloneCandidate.hh"

#include <memory>
#include <utility>
#include <vector>

#include "MoveCandidate.hh"
#include "OptimizerTypes.hh"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "odb/geom.h"
#include "rsz/Resizer.hh"
#include "sta/Liberty.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/PortDirection.hh"
#include "utl/Logger.h"

namespace rsz {

using utl::RSZ;

CloneCandidate::CloneCandidate(Resizer& resizer,
                               const Target& target,
                               sta::Pin* drvr_pin,
                               sta::Instance* drvr_inst,
                               sta::Instance* parent,
                               sta::LibertyCell* original_cell,
                               sta::LibertyCell* clone_cell,
                               const odb::Point& clone_loc,
                               std::vector<sta::Pin*> moved_loads)
    : MoveCandidate(resizer, target),
      drvr_pin_(drvr_pin),
      drvr_inst_(drvr_inst),
      parent_(parent),
      original_cell_(original_cell),
      clone_cell_(clone_cell),
      clone_loc_(clone_loc),
      moved_loads_(std::move(moved_loads))
{
}

MoveResult CloneCandidate::apply()
{
  return applyClone();
}

bool CloneCandidate::connectCloneInputs()
{
  std::unique_ptr<sta::InstancePinIterator> inst_pin_iter{
      resizer_.network()->pinIterator(drvr_inst_)};
  while (inst_pin_iter->hasNext()) {
    sta::Pin* pin = inst_pin_iter->next();
    if (!resizer_.network()->direction(pin)->isInput()) {
      continue;
    }

    sta::LibertyPort* lib_port = resizer_.network()->libertyPort(pin);
    odb::dbNet* dbnet = resizer_.dbNetwork()->flatNet(pin);
    odb::dbModNet* modnet = resizer_.dbNetwork()->hierNet(pin);
    sta::Pin* clone_pin
        = resizer_.dbNetwork()->findPin(clone_inst_, lib_port->name());
    odb::dbITerm* iterm = resizer_.dbNetwork()->flatPin(clone_pin);

    if (dbnet != nullptr) {
      resizer_.sta()->connectPin(
          clone_inst_, lib_port, resizer_.dbNetwork()->dbToSta(dbnet));
    }
    if (modnet != nullptr) {
      iterm->connect(modnet);
    }
  }
  return true;
}

bool CloneCandidate::connectCloneOutput()
{
  std::unique_ptr<sta::InstancePinIterator> clone_pin_iter{
      resizer_.network()->pinIterator(clone_inst_)};
  while (clone_pin_iter->hasNext()) {
    sta::Pin* pin = clone_pin_iter->next();
    if (resizer_.network()->direction(pin)->isOutput()) {
      clone_output_pin_ = pin;
      break;
    }
  }

  if (clone_output_pin_ == nullptr) {
    resizer_.logger()->error(
        RSZ,
        kMsgCloneOutputPinMissing,
        "Cannot find output pin of the clone instance. Driver pin: {}, "
        "Clone output pin: {}",
        resizer_.network()->pathName(drvr_pin_),
        "Null");
    return false;
  }

  sta::Port* clone_output_port = resizer_.network()->port(clone_output_pin_);
  out_net_ = resizer_.dbNetwork()->makeNet(parent_);
  resizer_.sta()->connectPin(clone_inst_, clone_output_port, out_net_);
  clone_output_iterm_ = resizer_.dbNetwork()->flatPin(clone_output_pin_);
  return true;
}

bool CloneCandidate::moveLoads()
{
  for (sta::Pin* load_pin : moved_loads_) {
    odb::dbITerm* load_iterm = resizer_.dbNetwork()->flatPin(load_pin);
    sta::Port* load_port = resizer_.network()->port(load_pin);
    sta::Instance* load = resizer_.network()->instance(load_pin);
    sta::Instance* load_parent
        = resizer_.dbNetwork()->getOwningInstanceParent(load_pin);
    resizer_.sta()->disconnectPin(load_pin);
    if (load_parent != parent_) {
      resizer_.dbNetwork()->hierarchicalConnect(clone_output_iterm_,
                                                load_iterm);
    } else {
      resizer_.sta()->connectPin(load, load_port, out_net_);
    }
  }
  return true;
}

bool CloneCandidate::rewireCloneTopology()
{
  return connectCloneInputs() && connectCloneOutput() && moveLoads();
}

MoveResult CloneCandidate::applyClone()
{
  clone_inst_
      = resizer_.makeInstance(clone_cell_, "clone", parent_, clone_loc_);
  if (clone_inst_ == nullptr) {
    return rejectedMove();
  }

  debugPrint(resizer_.logger(),
             RSZ,
             "clone_move",
             1,
             "ACCEPT CloneMove {}: ({}) -> {} ({})",
             resizer_.network()->pathName(drvr_pin_),
             original_cell_->name(),
             resizer_.network()->pathName(clone_inst_),
             clone_cell_->name());
  if (!rewireCloneTopology()) {
    return rejectedMove();
  }

  return {
      .accepted = true,
      .type = MoveType::kClone,
      .move_count = 1,
      .touched_instances = {clone_inst_, drvr_inst_},
  };
}

}  // namespace rsz
