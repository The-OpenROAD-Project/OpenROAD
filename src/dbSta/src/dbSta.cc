/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2019, OpenROAD
// All rights reserved.
//
// BSD 3-Clause License
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////////

// dbSta, OpenSTA on OpenDB

#include "db_sta/dbSta.hh"

#include <tcl.h>

#include "sta/StaMain.hh"
#include "sta/Graph.hh"
#include "sta/Clock.hh"
#include "sta/Sdc.hh"
#include "sta/Search.hh"
#include "sta/Bfs.hh"
#include "sta/EquivCells.hh"
#include "db_sta/dbNetwork.hh"
#include "db_sta/MakeDbSta.hh"
#include "opendb/db.h"
#include "openroad/OpenRoad.hh"
#include "openroad/Error.hh"
#include "dbSdcNetwork.hh"

namespace ord {

sta::dbSta *
makeDbSta()
{
  return new sta::dbSta;
}

void
initDbSta(OpenRoad *openroad)
{
  auto* sta = openroad->getSta();
  sta->init(openroad->tclInterp(), openroad->getDb());
  openroad->addObserver(sta);
}

void
deleteDbSta(sta::dbSta *sta)
{
  delete sta;
}

} // namespace ord

////////////////////////////////////////////////////////////////

namespace sta {

dbSta *
makeBlockSta(dbBlock *block)
{
  Sta *sta = Sta::sta();
  dbSta *sta2 = new dbSta;
  sta2->makeComponents();
  sta2->getDbNetwork()->setBlock(block);
  sta2->setTclInterp(sta->tclInterp());
  sta2->copyUnits(sta->units());
  return sta2;
}

extern "C" {
extern int Dbsta_Init(Tcl_Interp *interp);
}

extern const char *dbsta_tcl_inits[];

dbSta::dbSta() :
  Sta(),
  db_(nullptr),
  db_cbk_(this)
{
}

void
dbSta::init(Tcl_Interp *tcl_interp,
	    dbDatabase *db)
{
  initSta();
  Sta::setSta(this);
  db_ = db;
  makeComponents();
  setTclInterp(tcl_interp);
  // Define swig TCL commands.
  Dbsta_Init(tcl_interp);
  // Eval encoded sta TCL sources.
  evalTclInit(tcl_interp, dbsta_tcl_inits);
}

// Wrapper to init network db.
void
dbSta::makeComponents()
{
  Sta::makeComponents();
  db_network_->setDb(db_);
}

void
dbSta::makeNetwork()
{
  db_network_ = new class dbNetwork();
  network_ = db_network_;
}

void
dbSta::makeSdcNetwork()
{
  sdc_network_ = new dbSdcNetwork(network_);
}

void
dbSta::postReadLef(dbTech* tech,
                   dbLib* library)
{
  if (library) {
    db_network_->readLefAfter(library);
  }
}

void
dbSta::postReadDef(dbBlock* block)
{
  db_network_->readDefAfter(block);
  db_cbk_.addOwner(block);
  db_cbk_.setNetwork(db_network_);
}

void
dbSta::postReadDb(dbDatabase* db)
{
  db_network_->readDbAfter(db);
}

Slack
dbSta::netSlack(const dbNet *db_net,
		const MinMax *min_max)
{
  const Net *net = db_network_->dbToSta(db_net);
  return netSlack(net, min_max);
}

void
dbSta::findClkNets(// Return value.
		   std::set<dbNet*> &clk_nets)
{
  ensureClkNetwork();
  for (Clock *clk : sdc_->clks()) {
    for (const Pin *pin : *pins(clk)) {
      Net *net = network_->net(pin);      
      if (net)
	clk_nets.insert(db_network_->staToDb(net));
    }
  }
}

void
dbSta::findClkNets(const Clock *clk,
		   // Return value.
		   std::set<dbNet*> &clk_nets)
{
  ensureClkNetwork();
  for (const Pin *pin : *pins(clk)) {
    Net *net = network_->net(pin);      
    if (net)
      clk_nets.insert(db_network_->staToDb(net));
  }
}

////////////////////////////////////////////////////////////////

// Network edit functions.
// These override the default sta functions that call sta before/after
// functions because the db calls them via callbacks.

Instance *
dbSta::makeInstance(const char *name,
		  LibertyCell *cell,
		  Instance *parent)
{
  NetworkEdit *network = networkCmdEdit();
  Instance *inst = network->makeInstance(cell, name, parent);
  network->makePins(inst);
  return inst;
}

void
dbSta::deleteInstance(Instance *inst)
{
  NetworkEdit *network = networkCmdEdit();
  network->deleteInstance(inst);
}

void
dbSta::replaceCell(Instance *inst,
		 Cell *to_cell,
		 LibertyCell *to_lib_cell)
{
  NetworkEdit *network = networkCmdEdit();
  LibertyCell *from_lib_cell = network->libertyCell(inst);
  if (sta::equivCells(from_lib_cell, to_lib_cell)) {
    replaceEquivCellBefore(inst, to_lib_cell);
    network->replaceCell(inst, to_cell);
    replaceEquivCellAfter(inst);
  }
  else {
    replaceCellBefore(inst, to_lib_cell);
    network->replaceCell(inst, to_cell);
    replaceCellAfter(inst);
  }
}

void
dbSta::deleteNet(Net *net)
{
  NetworkEdit *network = networkCmdEdit();
  network->deleteNet(net);
}

void
dbSta::connectPin(Instance *inst,
		Port *port,
		Net *net)
{
  NetworkEdit *network = networkCmdEdit();
  Pin *pin = network->connect(inst, port, net);
}

void
dbSta::connectPin(Instance *inst,
		LibertyPort *port,
		Net *net)
{
  NetworkEdit *network = networkCmdEdit();
  network->connect(inst, port, net);
}

void
dbSta::disconnectPin(Pin *pin)
{
  NetworkEdit *network = networkCmdEdit();
  network->disconnectPin(pin);
}

////////////////////////////////////////////////////////////////

dbStaCbk::dbStaCbk(dbSta *sta) :
  sta_(sta),
  network_(nullptr)  // not built yet
{
}

void
dbStaCbk::setNetwork(dbNetwork *network)
{
  network_ = network;
}

void
dbStaCbk::inDbInstCreate(dbInst* inst)
{
  sta_->makeInstanceAfter(network_->dbToSta(inst));
}

void
dbStaCbk::inDbInstDestroy(dbInst *inst)
{
  // This is called after the iterms have been destroyed
  // so it side-steps Sta::deleteInstanceAfter.
  sta_->deleteLeafInstanceBefore(network_->dbToSta(inst));
}

void
dbStaCbk::inDbInstSwapMasterBefore(dbInst *inst,
                                   dbMaster *master)
{
  LibertyCell *to_lib_cell = network_->libertyCell(network_->dbToSta(master));
  LibertyCell *from_lib_cell = network_->libertyCell(inst);
  Instance *sta_inst = network_->dbToSta(inst);
  if (sta::equivCells(from_lib_cell, to_lib_cell))
    sta_->replaceEquivCellBefore(sta_inst, to_lib_cell);
  else
    ord::error("instance %s swap master %s is not equivalent",
               inst->getConstName(),
               master->getConstName());
}

void
dbStaCbk::inDbInstSwapMasterAfter(dbInst *inst)
{
  sta_->replaceEquivCellAfter(network_->dbToSta(inst));
}

void
dbStaCbk::inDbNetDestroy(dbNet *net)
{
  sta_->deleteNetBefore(network_->dbToSta(net));
}

void
dbStaCbk::inDbITermPostConnect(dbITerm *iterm)
{
  sta_->connectPinAfter(network_->dbToSta(iterm));
}

void
dbStaCbk::inDbITermPreDisconnect(dbITerm *iterm)
{
  sta_->disconnectPinBefore(network_->dbToSta(iterm));
}

void
dbStaCbk::inDbITermDestroy(dbITerm *iterm)
{
  sta_->deletePinBefore(network_->dbToSta(iterm));
}

void
dbStaCbk::inDbBTermPostConnect(dbBTerm *bterm)
{
  sta_->connectPinAfter(network_->dbToSta(bterm));
}

void
dbStaCbk::inDbBTermPreDisconnect(dbBTerm *bterm)
{
  sta_->disconnectPinBefore(network_->dbToSta(bterm));
}

void
dbStaCbk::inDbBTermDestroy(dbBTerm *bterm)
{
  sta_->deletePinBefore(network_->dbToSta(bterm));
}

} // namespace sta
