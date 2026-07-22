// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#include <cstdio>
#include <string>
#include <vector>

#include "odb/db.h"
#include "odb/dbBlockCallBackObj.h"
#include "spdlog/fmt/fmt.h"

namespace odb {

class CallBack : public dbBlockCallBackObj
{
 private:
  /* data */
  bool _pause = false;

 public:
  std::vector<std::string> events;
  // dbInst Start
  void inDbInstCreate(dbInst* inst) override
  {
    if (!_pause) {
      dbRegion* region = inst->getRegion();
      if (region) {
        events.push_back("Create inst " + inst->getName() + " in region "
                         + region->getName());
      } else {
        events.push_back("Create inst " + inst->getName());
      }
    }
  }
  void inDbInstDestroy(dbInst* inst) override
  {
    if (!_pause) {
      events.push_back("Destroy inst " + inst->getName());
    }
  }
  void inDbInstSwapMasterBefore(dbInst* inst, dbMaster* master) override
  {
    if (!_pause) {
      events.push_back("PreSwap inst " + inst->getName() + " from master "
                       + inst->getMaster()->getName() + " to master "
                       + master->getName());
    }
  }
  void inDbInstSwapMasterAfter(dbInst* inst) override
  {
    if (!_pause) {
      events.push_back("PostSwap inst " + inst->getName() + " to master "
                       + inst->getMaster()->getName());
    }
  }
  void inDbPreMoveInst(dbInst* inst) override
  {
    if (!_pause) {
      events.push_back("PreMove inst " + inst->getName());
    }
  }
  void inDbPostMoveInst(dbInst* inst) override
  {
    if (!_pause) {
      events.push_back("PostMove inst " + inst->getName());
    }
  }
  void inDbInstPlacementStatusBefore(dbInst* inst,
                                     const dbPlacementStatus& status) override
  {
    if (!_pause) {
      char buffer[100];
      sprintf(buffer,
              "Change inst status: %s -> %s",
              inst->getConstName(),
              status.getString());
      events.emplace_back(buffer);
    }
  }
  // dbInst End

  // dbNet Start
  void inDbNetCreate(dbNet* net) override
  {
    if (!_pause) {
      events.push_back("Create net " + net->getName());
    }
  }
  void inDbNetDestroy(dbNet* net) override
  {
    if (!_pause) {
      events.push_back("Destroy net " + net->getName());
    }
  }
  // dbNet End

  // dbITerm Start
  void inDbITermCreate(dbITerm* iterm) override
  {
    if (!_pause) {
      events.push_back("Create iterm " + iterm->getMTerm()->getName()
                       + " of inst " + iterm->getInst()->getName());
    }
  }
  void inDbITermDestroy(dbITerm* iterm) override
  {
    if (!_pause) {
      events.push_back("Destroy iterm " + iterm->getMTerm()->getName()
                       + " of inst " + iterm->getInst()->getName());
    }
  }
  void inDbITermPreDisconnect(dbITerm* iterm) override
  {
    if (!_pause) {
      events.push_back("PreDisconnect iterm from net "
                       + iterm->getNet()->getName());
    }
  }
  void inDbITermPostDisconnect(dbITerm* iterm, dbNet* net) override
  {
    if (!_pause) {
      events.push_back("PostDisconnect iterm from net " + net->getName());
    }
  }
  void inDbITermPreConnect(dbITerm* iterm, dbNet* net) override
  {
    if (!_pause) {
      events.push_back("PreConnect iterm to net " + net->getName());
    }
  }
  void inDbITermPostConnect(dbITerm* iterm) override
  {
    if (!_pause) {
      events.push_back("PostConnect iterm to net "
                       + iterm->getNet()->getName());
    }
  }

  // dbITerm End

  // dbBTerm Start
  void inDbBTermCreate(dbBTerm* bterm) override
  {
    if (!_pause) {
      events.push_back("Create " + bterm->getName());
    }
  }
  void inDbBTermDestroy(dbBTerm* bterm) override
  {
    if (!_pause) {
      events.push_back("Destroy " + bterm->getName());
    }
  }
  void inDbBTermPreConnect(dbBTerm* bterm, dbNet* net) override
  {
    if (!_pause) {
      events.push_back("Preconnect " + bterm->getName() + " to "
                       + net->getName());
    }
  }
  void inDbBTermPostConnect(dbBTerm* bterm) override
  {
    if (!_pause) {
      events.push_back("Postconnect " + bterm->getName());
    }
  }
  void inDbBTermPreDisconnect(dbBTerm* bterm) override
  {
    if (!_pause) {
      events.push_back("Predisconnect " + bterm->getName());
    }
  }
  void inDbBTermPostDisConnect(dbBTerm* bterm, dbNet* net) override
  {
    if (!_pause) {
      events.push_back("Postdisconnect " + bterm->getName() + " from "
                       + net->getName());
    }
  }
  // dbBTerm End

  // dbBPin Start
  void inDbBPinCreate(dbBPin* pin) override
  {
    if (!_pause) {
      events.push_back("Create BPin for " + pin->getBTerm()->getName());
    }
  }

  void inDbBPinAddBox(dbBox* box) override
  {
    if (!_pause) {
      char buffer[100];
      sprintf(buffer,
              "Create BPin box (%d,%d) (%d,%d)",
              box->xMin(),
              box->yMin(),
              box->xMax(),
              box->yMax());
      events.emplace_back(buffer);
    }
  }

  void inDbBPinRemoveBox(dbBox* box) override
  {
    if (!_pause) {
      events.emplace_back("Destroy bpin box");
    }
  }

  void inDbBPinDestroy(dbBPin*) override
  {
    if (!_pause) {
      events.emplace_back("Destroy BPin");
    }
  }

  void inDbBPinPlacementStatusBefore(dbBPin* pin,
                                     const dbPlacementStatus& status) override
  {
    if (!_pause) {
      char buffer[100];
      const std::string name = pin->getName();
      sprintf(buffer,
              "Change BPin status: %s -> %s",
              name.c_str(),
              status.getString());
      events.emplace_back(buffer);
    }
  }
  // dbBPin End

  // dbBlockage Start
  void inDbBlockageCreate(dbBlockage* blockage) override
  {
    if (_pause) {
      return;
    }
    char buffer[100];
    auto box = blockage->getBBox();
    sprintf(buffer,
            "Create blockage (%d,%d) (%d,%d)",
            box->xMin(),
            box->yMin(),
            box->xMax(),
            box->yMax());
    events.emplace_back(buffer);
  }
  void inDbBlockageDestroy(dbBlockage* blockage) override
  {
    if (!_pause) {
      events.emplace_back("Destroy blockage");
    }
  }
  // dbBlockage End

  // dbObstruction Start
  void inDbObstructionCreate(dbObstruction* obs) override
  {
    if (_pause) {
      return;
    }
    char buffer[100];
    auto box = obs->getBBox();
    sprintf(buffer,
            "Create obstruction (%d,%d) (%d,%d)",
            box->xMin(),
            box->yMin(),
            box->xMax(),
            box->yMax());
    events.emplace_back(buffer);
  }
  void inDbObstructionDestroy(dbObstruction*) override
  {
    if (!_pause) {
      events.emplace_back("Destroy obstruction");
    }
  }
  // dbObstruction End

  // dbRegion Start
  void inDbRegionCreate(dbRegion* region) override
  {
    if (!_pause) {
      events.push_back("Create region " + region->getName());
    }
  }
  void inDbRegionAddBox(dbRegion* region, dbBox* box) override
  {
    if (!_pause) {
      events.push_back(fmt::format("Add box ({}, {}) ({}, {}) to region {}",
                                   box->xMin(),
                                   box->yMin(),
                                   box->xMax(),
                                   box->yMax(),
                                   region->getName()));
    }
  }
  void inDbRegionDestroy(dbRegion* region) override
  {
    if (!_pause) {
      events.push_back("Destroy region " + region->getName());
    }
  }
  // dbRegion End

  // dbRow Start
  void inDbRowCreate(dbRow* row) override
  {
    if (!_pause) {
      events.push_back("Create row " + row->getName());
    }
  }
  void inDbRowDestroy(dbRow* row) override
  {
    if (!_pause) {
      events.push_back("Destroy row " + row->getName());
    }
  }
  // dbRow End

  // dbWire Start
  void inDbWireCreate(dbWire* wire) override
  {
    if (!_pause) {
      events.emplace_back("Create wire");
    }
  }
  void inDbWirePreAttach(dbWire* wire, dbNet* net) override
  {
    if (!_pause) {
      events.push_back("PreAttach wire to " + net->getName());
    }
  }
  void inDbWirePostAttach(dbWire* wire) override
  {
    if (!_pause) {
      events.emplace_back("PostAttach wire");
    }
  }
  void inDbWirePreDetach(dbWire* wire) override
  {
    if (!_pause) {
      events.emplace_back("PreDetach wire");
    }
  }
  void inDbWirePostDetach(dbWire* wire, dbNet* net) override
  {
    if (!_pause) {
      events.push_back("PostDetach wire from " + net->getName());
    }
  }
  void inDbWirePreAppend(dbWire* src, dbWire* dst) override
  {
    if (!_pause) {
      events.emplace_back("PreAppend wire");
    }
  }
  void inDbWirePostAppend(dbWire* src, dbWire* dst) override
  {
    if (!_pause) {
      events.emplace_back("PostAppend wire");
    }
  }
  void inDbWirePreCopy(dbWire* src, dbWire* dst) override
  {
    if (!_pause) {
      events.emplace_back("PreCopy wire");
    }
  }
  void inDbWirePostCopy(dbWire* src, dbWire* dst) override
  {
    if (!_pause) {
      events.emplace_back("PostCopy wire");
    }
  }
  void inDbWireDestroy(dbWire* wire) override
  {
    if (!_pause) {
      events.emplace_back("Destroy wire");
    }
  }
  // dbWire End

  // dbSWire Start
  void inDbSWireCreate(dbSWire*) override
  {
    if (!_pause) {
      events.emplace_back("Create swire");
    }
  }
  void inDbSWireDestroy(dbSWire*) override
  {
    if (!_pause) {
      events.emplace_back("Destroy swire");
    }
  }
  void inDbSWirePreDestroySBoxes(dbSWire* swire) override
  {
    if (!_pause) {
      events.emplace_back("PreDestroySBoxes");
    }
  }
  void inDbSWirePostDestroySBoxes(dbSWire* swire) override
  {
    if (!_pause) {
      events.emplace_back("PostDestroySBoxes");
    }
  }
  // dbSWire End

  void pause() { _pause = true; }
  void unpause() { _pause = false; }
  void clearEvents() { events.clear(); }
};

}  // namespace odb
