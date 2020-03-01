// Copyright (c) 2019, Parallax Software, Inc.
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <map>
#include "Machine.hh"
#include "Error.hh"
#include "Report.hh"
#include "Debug.hh"
#include "Vector.hh"
#include "PortDirection.hh"
#include "ConcreteNetwork.hh"
#include "NetworkCmp.hh"
#include "VerilogReader.hh"

#include "db_sta/dbNetwork.hh"
#include "opendb/db.h"

#include "openroad/OpenRoad.hh"

namespace ord {

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
using sta::Report;
using sta::Debug;
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
using sta::NetIterator;
using sta::NetTermIterator;
using sta::ConnectedPinIterator;
using sta::NetConnectedPinIterator;
using sta::PinPathNameLess;

// Hierarchical network for read_verilog.
// Verilog cells and module networks are built here.
// It is NOT part of an Sta.
class dbVerilogNetwork : public  ConcreteNetwork
{
public:
  dbVerilogNetwork();
  virtual Cell *findAnyCell(const char *name);
  void init(dbNetwork *db_network);

private:
  NetworkReader *db_network_;
};

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
	     dbDatabase *db);
  void makeBlock();
  void makeDbNetlist();

protected:
  void makeDbInsts();
  dbIoType staToDb(PortDirection *dir);
  void makeDbNets(const Instance *inst);
  bool hasTerminals(Net *net) const;
  dbMaster *getMaster(Cell *cell);

  Network *network_;
  dbDatabase *db_;
  dbBlock *block_;
  std::map<Cell*, dbMaster*> master_map_;
};

void
dbLinkDesign(const char *top_cell_name,
	     dbVerilogNetwork *verilog_network,
	     dbDatabase *db)
{
  bool link_make_black_boxes = true;
  bool success = verilog_network->linkNetwork(top_cell_name,
					      link_make_black_boxes,
					      verilog_network->report());
  if (success) {
    Verilog2db v2db(verilog_network, db);
    v2db.makeBlock();
    v2db.makeDbNetlist();
    deleteVerilogReader();
  }
}

Verilog2db::Verilog2db(Network *network,
		       dbDatabase *db) :
  network_(network),
  db_(db),
  block_(nullptr)
{
}

void
Verilog2db::makeBlock()
{
  dbChip *chip = db_->getChip();
  if (chip == nullptr)
    chip = dbChip::create(db_);
  dbBlock *block = chip->getBlock();
  if (block)
    dbBlock::destroy(block);
  const char *design = network_->name(network_->cell(network_->topInstance()));
  block_ = dbBlock::create(chip, design, network_->pathDivider());
  dbTech *tech = db_->getTech();
  block_->setDefUnits(tech->getLefUnits());
  block_->setBusDelimeters('[', ']');
}

void
Verilog2db::makeDbNetlist()
{
  makeDbInsts();
  makeDbNets(network_->topInstance());
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
    if ((is_top || !hasTerminals(net))
	&& !network_->isGround(net)
	&& !network_->isPower(net)) {
      dbNet *db_net = dbNet::create(block_, net_name);
      
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
	if (network_->isTopLevelPort(pin)) {
	  const char *port_name = network_->portName(pin);
	  if (block_->findBTerm(port_name) == nullptr) {
	    dbBTerm *bterm = dbBTerm::create(db_net, port_name);
	    dbIoType io_type = staToDb(network_->direction(pin));
	    bterm->setIoType(io_type);
	  }
	}
	else if (network_->isLeaf(pin)) {
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
      return master;
    }
    else {
      master_map_[cell] = nullptr;
      return nullptr;
    }
  }
}

}
