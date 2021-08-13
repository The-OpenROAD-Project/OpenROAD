/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2019, The Regents of the University of California
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

#include "db_sta/dbReadVerilog.hh"

#include <map>

#include "sta/Vector.hh"
#include "sta/PortDirection.hh"
#include "sta/ConcreteNetwork.hh"
#include "sta/NetworkCmp.hh"
#include "sta/VerilogReader.hh"

#include "db_sta/dbNetwork.hh"
#include "odb/db.h"

#include "ord/OpenRoad.hh"
#include "utl/Logger.h"

namespace ord {

using utl::ORD;
using odb::dbDatabase;
using odb::dbChip;
using odb::dbBlock;
using odb::dbTech;
using odb::dbLib;
using odb::dbMaster;
using odb::dbInst;
using odb::dbNet;
using odb::dbBTerm;
using odb::dbMTerm;
using odb::dbITerm;
using odb::dbSet;
using odb::dbIoType;

using sta::dbNetwork;
using sta::ConcreteNetwork;
using sta::Network;
using sta::NetworkReader;
using sta::PortDirection;
using sta::Library;
using sta::Instance;
using sta::Pin;
using sta::PinSeq;
using sta::Net;
using sta::Cell;
using sta::deleteVerilogReader;
using sta::LeafInstanceIterator;
using sta::InstanceChildIterator;
using sta::InstancePinIterator;
using sta::NetIterator;
using sta::NetTermIterator;
using sta::ConnectedPinIterator;
using sta::NetConnectedPinIterator;
using sta::PinPathNameLess;
using sta::LibertyCell;
using sta::CellPortBitIterator;
using sta::CellPortIterator;
using sta::Port;

using utl::Logger;
using utl::STA;

dbVerilogNetwork::dbVerilogNetwork() :
  ConcreteNetwork(),
  db_network_(nullptr)
{
  report_ = nullptr;
  debug_ = nullptr;
}

void
dbVerilogNetwork::init(dbNetwork *db_network)
{
  db_network_ = db_network;
  report_ = db_network_->report();
  debug_ = db_network_->debug();
}

dbVerilogNetwork *
makeDbVerilogNetwork()
{
  return new dbVerilogNetwork;
}

void
initDbVerilogNetwork(ord::OpenRoad *openroad)
{
  openroad->getVerilogNetwork()->init(openroad->getDbNetwork());
}

void
deleteDbVerilogNetwork(dbVerilogNetwork *verilog_network)
{
  delete verilog_network;
}

// Facade that looks in the db network for a liberty cell if
// there isn't one in the verilog network.
Cell *
dbVerilogNetwork::findAnyCell(const char *name)
{
  Cell *cell = ConcreteNetwork::findAnyCell(name);
  if (cell == nullptr)
    cell = db_network_->findAnyCell(name);
  return cell;
}

void
dbReadVerilog(const char *filename,
	      dbVerilogNetwork *verilog_network)
{
  sta::readVerilogFile(filename, verilog_network);
}

////////////////////////////////////////////////////////////////

class Verilog2db
{
public:
  Verilog2db(Network *verilog_network,
	     dbDatabase *db,
             Logger *logger);
  void makeBlock();
  void makeDbNetlist();

protected:
  void makeDbInsts();
  dbIoType staToDb(PortDirection *dir);
  void makeDbPins();
  void makeDbNets(const Instance *inst);
  bool hasTerminals(Net *net) const;
  dbMaster *getMaster(Cell *cell);

  Network *network_;
  dbDatabase *db_;
  dbBlock *block_;
  Logger *logger_;
  std::map<Cell*, dbMaster*> master_map_;
};

void
dbLinkDesign(const char *top_cell_name,
	     dbVerilogNetwork *verilog_network,
	     dbDatabase *db,
             Logger *logger)
{
  bool link_make_black_boxes = true;
  bool success = verilog_network->linkNetwork(top_cell_name,
					      link_make_black_boxes,
					      verilog_network->report());
  if (success) {
    Verilog2db v2db(verilog_network, db, logger);
    v2db.makeBlock();
    v2db.makeDbNetlist();
    deleteVerilogReader();
  }
}

Verilog2db::Verilog2db(Network *network,
		       dbDatabase *db,
                       Logger *logger) :
  network_(network),
  db_(db),
  block_(nullptr),
  logger_(logger)
{
}

void
Verilog2db::makeBlock()
{
  dbChip *chip = db_->getChip();
  if (chip == nullptr)
    chip = dbChip::create(db_);
  block_ = chip->getBlock();
  if (block_) {
    auto insts = block_->getInsts();
    for (auto iter = insts.begin(); iter != insts.end(); ) {
      iter = dbInst::destroy(iter);
    }
    auto nets = block_->getNets();
    for (auto iter = nets.begin(); iter != nets.end(); ) {
      iter = dbNet::destroy(iter);
    }
  }
  else {
    const char *design = network_->name(network_->cell(network_->topInstance()));
    block_ = dbBlock::create(chip, design, network_->pathDivider());
  }
  dbTech *tech = db_->getTech();
  block_->setDefUnits(tech->getLefUnits());
  block_->setBusDelimeters('[', ']');
}

void
Verilog2db::makeDbNetlist()
{
  makeDbPins();
  makeDbInsts();
  makeDbNets(network_->topInstance());
}

void
Verilog2db::makeDbPins()
{
  Cell *top_cell = network_->cell(network_->topInstance());
  CellPortBitIterator *port_iter = network_->portBitIterator(top_cell);
  while (port_iter->hasNext()) {
    Port *port = port_iter->next();
    const char *port_name = network_->name(port);
    dbNet *db_net = dbNet::create(block_, port_name);
    dbBTerm *bterm = dbBTerm::create(db_net, port_name);
    dbIoType io_type = staToDb(network_->direction(port));
    bterm->setIoType(io_type);
  }
  delete port_iter;

  // OpenDB does not have any concept of bus ports.
  // Use a property to annotate the bus names as msb or lsb first for writing verilog.
  CellPortIterator *bus_iter = network_->portIterator(top_cell);
  while (bus_iter->hasNext()) {
    Port *port = bus_iter->next();
    if (network_->isBus(port)) {
      const char *port_name = network_->name(port);
      int from = network_->fromIndex(port);
      int to = network_->toIndex(port);
      string key = "bus_msb_first ";
      key += port_name;
      odb::dbBoolProperty::create(block_, key.c_str(), from > to);
    }
  }
  delete bus_iter;
}

void
Verilog2db::makeDbInsts()
{
  LeafInstanceIterator *leaf_iter = network_->leafInstanceIterator();
  while (leaf_iter->hasNext()) {
    Instance *inst = leaf_iter->next();
    const char *inst_name = network_->pathName(inst);
    Cell *cell = network_->cell(inst);
    dbMaster *master = getMaster(cell);
    if (master)
      dbInst::create(block_, master, inst_name);
    else
      logger_->warn(ORD, 1013, "instance {} LEF master {} not found.",
                    inst_name,
                    network_->name(cell));
  }
  delete leaf_iter;
}

dbIoType
Verilog2db::staToDb(PortDirection *dir)
{
  if (dir == PortDirection::input())
    return dbIoType::INPUT;
  else if (dir == PortDirection::output())
    return dbIoType::OUTPUT;
  else if (dir == PortDirection::bidirect())
    return dbIoType::INOUT;
  else if (dir == PortDirection::tristate())
    return dbIoType::OUTPUT;
  else
    return dbIoType::INOUT;
}

void
Verilog2db::makeDbNets(const Instance *inst)
{
  bool is_top = (inst == network_->topInstance());
  NetIterator *net_iter = network_->netIterator(inst);
  while (net_iter->hasNext()) {
    Net *net = net_iter->next();
    const char *net_name = network_->pathName(net);
    if (is_top || !hasTerminals(net)) {
      dbNet *db_net = block_->findNet(net_name);
      // Net may already exist from makeDbPins().
      if (db_net == nullptr)
        db_net = dbNet::create(block_, net_name);

      if (network_->isPower(net))
        db_net->setSigType(odb::dbSigType::POWER);
      if (network_->isGround(net))
        db_net->setSigType(odb::dbSigType::GROUND);

      // Sort connected pins for regression stability.
      PinSeq net_pins;
      NetConnectedPinIterator *pin_iter = network_->connectedPinIterator(net);
      while (pin_iter->hasNext()) {
	Pin *pin = pin_iter->next();
	net_pins.push_back(pin);
      }
      delete pin_iter;
      sort(net_pins, PinPathNameLess(network_));

      for (Pin *pin : net_pins) {
        // Note bterms are made in makeDbPins().
        if (network_->isLeaf(pin)) {
	  const char *port_name = network_->portName(pin);
	  Instance *inst = network_->instance(pin);
	  const char *inst_name = network_->pathName(inst);
	  dbInst *db_inst = block_->findInst(inst_name);
	  if (db_inst) {
	    dbMaster *master = db_inst->getMaster();
	    dbMTerm *mterm = master->findMTerm(block_, port_name);
	    if (mterm)
	      dbITerm::connect(db_inst, db_net, mterm);
	  }
	}
      }
    }
  }
  delete net_iter;

  InstanceChildIterator *child_iter = network_->childIterator(inst);
  while (child_iter->hasNext()) {
    const Instance *child = child_iter->next();
    makeDbNets(child);
  }
  delete child_iter;
}

bool
Verilog2db::hasTerminals(Net *net) const
{
  NetTermIterator *term_iter = network_->termIterator(net);
  bool has_terms = term_iter->hasNext();
  delete term_iter;
  return has_terms;
}

dbMaster *
Verilog2db::getMaster(Cell *cell)
{
  auto miter = master_map_.find(cell);
  if (miter != master_map_.end())
    return miter->second;
  else {
    const char *cell_name = network_->name(cell);
    dbMaster *master = db_->findMaster(cell_name);
    if (master) {
      master_map_[cell] = master;
      // Check for corresponding liberty cell.
      LibertyCell *lib_cell = network_->libertyCell(cell);
      if (lib_cell == nullptr)
	logger_->warn(ORD, 1011, "LEF master {} has no liberty cell.", cell_name);
      return master;
    }
    else {
      LibertyCell *lib_cell = network_->libertyCell(cell);
      if (lib_cell)
        logger_->warn(ORD, 1012, "Liberty cell has no LEF master.", cell_name);
      // OpenSTA read_verilog warns about missing cells.
      master_map_[cell] = nullptr;
      return nullptr;
    }
  }
}

}
