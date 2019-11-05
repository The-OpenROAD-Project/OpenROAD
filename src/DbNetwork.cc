// OpenStaDB, OpenSTA on OpenDB
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

#include "Machine.hh"
#include "Report.hh"
#include "PatternMatch.hh"
#include "PortDirection.hh"
#include "Liberty.hh"
#include "sta_db/DbNetwork.hh"

#include "opendb/db.h"

namespace sta {

using odb::dbDatabase;
using odb::dbChip;
using odb::dbString;
using odb::dbObject;
using odb::dbLib;
using odb::dbMaster;
using odb::dbBlock;
using odb::dbInst;
using odb::dbNet;
using odb::dbBTerm;
using odb::dbITerm;
using odb::dbMTerm;
using odb::dbSigType;
using odb::dbIoType;
using odb::dbSet;
using odb::dbObjectType;
using odb::dbITermObj;
using odb::dbBTermObj;
using odb::dbIntProperty;

// TODO: move to StringUtil
char *
tmpStringCopy(const char *str)
{
  char *tmp = makeTmpString(strlen(str) + 1);
  strcpy(tmp, str);
  return tmp;
}

class DbLibraryIterator1 : public Iterator<Library*>
{
public:
  DbLibraryIterator1(ConcreteLibraryIterator *iter);
  ~DbLibraryIterator1();
  virtual bool hasNext();
  virtual Library *next();

private:
  ConcreteLibraryIterator *iter_;
};

DbLibraryIterator1::DbLibraryIterator1(ConcreteLibraryIterator * iter) :
  iter_(iter)
{
}

DbLibraryIterator1::~DbLibraryIterator1()
{
  delete iter_;
}

bool
DbLibraryIterator1::hasNext()
{
  return iter_->hasNext();
}

Library *
DbLibraryIterator1::next()
{
  return reinterpret_cast<Library*>(iter_->next());
}

////////////////////////////////////////////////////////////////

class DbInstanceChildIterator : public InstanceChildIterator
{
public:
  DbInstanceChildIterator(const Instance *instance,
			      const DbNetwork *network);
  bool hasNext();
  Instance *next();
  
private:
  const DbNetwork *network_;
  bool top_;
  dbSet<dbInst>::iterator iter_;
  dbSet<dbInst>::iterator end_;
};

DbInstanceChildIterator::DbInstanceChildIterator(const Instance *instance,
						 const DbNetwork *network) :
  network_(network)
{
  dbBlock *block = network->block();
  if (instance == network->topInstance() && block) {
    dbSet<dbInst> insts = block->getInsts();
    top_ = true;
    iter_ = insts.begin();
    end_ = insts.end();
  }
  else
    top_ = false;
}

bool
DbInstanceChildIterator::hasNext()
{
  return top_ && iter_ != end_;
}

Instance *
DbInstanceChildIterator::next()
{
  dbInst *child = *iter_;
  iter_++;
  return network_->dbToSta(child);
}


class DbInstanceNetIterator : public InstanceNetIterator
{
public:
  DbInstanceNetIterator(const Instance *instance,
			    const DbNetwork *network);
  bool hasNext();
  Net *next();

private:
  const DbNetwork *network_;
  bool top_;
  dbSet<dbNet>::iterator iter_;
  dbSet<dbNet>::iterator end_;
};

DbInstanceNetIterator::DbInstanceNetIterator(const Instance *instance,
						     const DbNetwork *network) :
  network_(network)
{
  if (instance == network->topInstance()) {
    top_ = true;
    dbSet<dbNet> nets = network->block()->getNets();
    iter_ = nets.begin();
    end_ = nets.end();
  }
  else
    top_ = false;
}

bool
DbInstanceNetIterator::hasNext()
{
  return top_ && iter_ != end_;
}

Net *
DbInstanceNetIterator::next()
{
  dbNet *net = *iter_;
  iter_++;
  return network_->dbToSta(net);
}

////////////////////////////////////////////////////////////////

class DbInstancePinIterator : public InstancePinIterator
{
public:
  DbInstancePinIterator(const Instance *inst,
			    const DbNetwork *network);
  bool hasNext();
  Pin *next();

private:
  const DbNetwork *network_;
  bool top_;
  dbSet<dbITerm>::iterator iitr_;
  dbSet<dbITerm>::iterator iitr_end_;
  dbSet<dbBTerm>::iterator bitr_;
  dbSet<dbBTerm>::iterator bitr_end_;
  Pin *pin_;
};

DbInstancePinIterator::DbInstancePinIterator(const Instance *inst,
						     const DbNetwork *network) :
  network_(network)
{
  top_ = (inst == network->topInstance());
  if (top_) {
    dbBlock *block = network->block();
    bitr_ = block->getBTerms().begin();
    bitr_end_ = block->getBTerms().end();
  }
  else {
    dbInst *dinst = network_->staToDb(inst);
    iitr_ = dinst->getITerms().begin();
    iitr_end_ = dinst->getITerms().end();
  }
}

bool 
DbInstancePinIterator::hasNext()
{
  if (top_) {
    if (bitr_ == bitr_end_)
      return false;
    else {
      dbBTerm *bterm = *bitr_;
      bitr_++;
      pin_ = network_->dbToSta(bterm);
      return true;
    }
  }
  if (iitr_ == iitr_end_)
    return false;
  else {
    dbITerm *iterm = *iitr_;
    while (iterm->getSigType() == dbSigType::POWER
	   || iterm->getSigType() == dbSigType::GROUND) {
      iitr_++;
      if (iitr_ == iitr_end_)
	return false;
      iterm = *iitr_;
    }
    if (iitr_ == iitr_end_)
      return false;
    else {
      pin_ = network_->dbToSta(iterm);
      iitr_++;
      return true;
    }
  }
}

Pin *
DbInstancePinIterator::next()
{
  return pin_;
}

////////////////////////////////////////////////////////////////

class DbNetPinIterator : public NetPinIterator
{
public:
  DbNetPinIterator(const Net *net,
		       const DbNetwork *network);
  bool hasNext();
  Pin *next();

private:
  const DbNetwork *network_;
  dbSet<dbITerm>::iterator _iitr;
  dbSet<dbITerm>::iterator _iitr_end;
  dbSet<dbBTerm>::iterator _bitr;
  dbSet<dbBTerm>::iterator _bitr_end;
  void *_term;
};

DbNetPinIterator::DbNetPinIterator(const Net *net,
					   const DbNetwork *network) :
  network_(network)
{
  dbNet *dnet = reinterpret_cast<dbNet*>(const_cast<Net*>(net));
  _iitr = dnet->getITerms().begin();
  _iitr_end = dnet->getITerms().end();
  _bitr = dnet->getBTerms().begin();
  _bitr_end = dnet->getBTerms().end();
  _term = NULL;
}

bool 
DbNetPinIterator::hasNext()
{
  if (_iitr != _iitr_end) {
    dbITerm *iterm = *_iitr;
    while (iterm->getSigType() == dbSigType::POWER
          || iterm->getSigType() == dbSigType::GROUND) {
      ++_iitr;
      if (_iitr == _iitr_end) break;
      iterm = *_iitr;
    }
  }
  if (_iitr != _iitr_end) {
    _term = (void*)(*_iitr);
    ++_iitr;
    return true;
  }
  if (_bitr != _bitr_end) {
    dbBTerm *bterm = *_bitr;
    ++_bitr;
    _term = network_->dbToSta(bterm);
    return true;
  }
  return false;
}

Pin *
DbNetPinIterator::next()
{
  return (Pin*)_term;
}

////////////////////////////////////////////////////////////////

class DbNetTermIterator : public NetTermIterator
{
public:
  DbNetTermIterator(const Net *net,
			const DbNetwork *network);
  bool hasNext();
  Term *next();

private:
  const DbNetwork *network_;
  dbSet<dbBTerm>::iterator iter_;
  dbSet<dbBTerm>::iterator end_;
};

DbNetTermIterator::DbNetTermIterator(const Net *net,
					     const DbNetwork *network) :
  network_(network)
{
  dbNet *dnet = network_->staToDb(net);
  dbSet<dbBTerm> terms = dnet->getBTerms();
  iter_ = terms.begin();
  end_ = terms.end();
}

bool 
DbNetTermIterator::hasNext()
{
  return iter_ != end_;
}

Term *
DbNetTermIterator::next()
{
  dbBTerm *bterm = *iter_;
  iter_++;
  return network_->dbToStaTerm(bterm);
}

////////////////////////////////////////////////////////////////

Network *
makeDbNetwork()
{
  return new DbNetwork;
}

DbNetwork::DbNetwork() :
  db_(nullptr),
  top_instance_(reinterpret_cast<Instance*>(1)),
  top_cell_(nullptr)
{
}

DbNetwork::~DbNetwork()
{
}

void
DbNetwork::clear()
{
  db_ = nullptr;
}

Instance *
DbNetwork::topInstance() const
{
  if (top_cell_)
    return top_instance_;
  else
    return nullptr;
}

////////////////////////////////////////////////////////////////

const char *
DbNetwork::name(const Instance *instance) const
{
  if (instance == top_instance_) {
    dbString name = block_->getName();
    return tmpStringCopy(name.c_str());
  }
  else {
    dbInst *dinst = staToDb(instance);
    dbString name = dinst->getName();
    return tmpStringCopy(name.c_str());
  }
}

Cell *
DbNetwork::cell(const Instance *instance) const
{
  if (instance == top_instance_)
    return reinterpret_cast<Cell*>(top_cell_);
  else {
    dbInst *dinst = staToDb(instance);
    dbMaster *master = dinst->getMaster();
    return dbToSta(master);
  }
}

Instance *
DbNetwork::parent(const Instance *instance) const
{
  if (instance == top_instance_)
    return nullptr;
  return top_instance_;
}

bool
DbNetwork::isLeaf(const Instance *instance) const
{
  if (instance == top_instance_)
    return false;
  return true;
}

Instance *
DbNetwork::findChild(const Instance *parent,
			 const char *name) const
{
  if (parent == top_instance_) {
    dbInst *inst = block_->findInst(name);
    return dbToSta(inst);
  }
  else
    return nullptr;
}

Pin *
DbNetwork::findPin(const Instance *instance, 
		       const char *port_name) const
{
  if (instance == top_instance_) {
    dbBTerm *bterm = block_->findBTerm(port_name);
    return dbToSta(bterm);
  }
  else {
    dbInst *dinst = staToDb(instance);
    dbITerm *iterm = dinst->findITerm(port_name);
    return reinterpret_cast<Pin*>(iterm);
  }
}

Pin *
DbNetwork::findPin(const Instance *instance,
		       const Port *port) const
{
  const char *port_name = this->name(port);
  return findPin(instance, port_name);
}

Net *
DbNetwork::findNet(const Instance *instance, 
		       const char *net_name) const
{
  if (instance == top_instance_) {
    dbNet *dnet = block_->findNet(net_name);
    return dbToSta(dnet);
  }
  else
    return nullptr;
}

void
DbNetwork::findInstNetsMatching(const Instance *instance,
				    const PatternMatch *pattern,
				    // Return value.
				    NetSeq *nets) const
{
  if (instance == top_instance_) {
    if (pattern->hasWildcards()) {
      for (dbNet *dnet : block_->getNets()) {
	const char *net_name = dnet->getName();
	if (pattern->match(net_name))
	  nets->push_back(dbToSta(dnet));
      }
    }
    else {
      dbNet *dnet = block_->findNet(pattern->pattern());
      if (dnet)
	nets->push_back(dbToSta(dnet));
    }
  }
}

InstanceChildIterator *
DbNetwork::childIterator(const Instance *instance) const
{
  return new DbInstanceChildIterator(instance, this);
}

InstancePinIterator *
DbNetwork::pinIterator(const Instance *instance) const
{
  return new DbInstancePinIterator(instance, this);
}

InstanceNetIterator *
DbNetwork::netIterator(const Instance *instance) const
{
  return new DbInstanceNetIterator(instance, this);
}

////////////////////////////////////////////////////////////////

Instance *
DbNetwork::instance(const Pin *pin) const
{
  dbITerm *iterm;
  dbBTerm *bterm;
  staToDb(pin, iterm, bterm);
  if (iterm) {
    dbInst *dinst = iterm->getInst();
    return dbToSta(dinst);
  }
  else if (bterm)
    return top_instance_;
  else
    return nullptr;
}

Net *
DbNetwork::net(const Pin *pin) const
{
  dbITerm *iterm;
  dbBTerm *bterm;
  staToDb(pin, iterm, bterm);
  if (iterm) {
    dbNet *dnet = iterm->getNet();
    return dbToSta(dnet);
  }
  else
    return nullptr;
}

Term *
DbNetwork::term(const Pin *pin) const
{
  dbITerm *iterm;
  dbBTerm *bterm;
  staToDb(pin, iterm, bterm);
  if (iterm)
    return nullptr;
  else if (bterm)
    return dbToStaTerm(bterm);
  else
    return nullptr;
}

Port *
DbNetwork::port(const Pin *pin) const
{
  dbITerm *iterm;
  dbBTerm *bterm;
  staToDb(pin, iterm, bterm);
  if (iterm) {
    dbMTerm *mterm = iterm->getMTerm();
    return dbToSta(mterm);
  }
  else if (bterm) {
    dbString port_name = bterm->getName();
    return findPort(top_cell_, port_name.c_str());
  }
  else
    return nullptr;
}

PortDirection *
DbNetwork::direction(const Pin *pin) const
{
  dbITerm *iterm;
  dbBTerm *bterm;
  staToDb(pin, iterm, bterm);
  if (iterm) {
    PortDirection *dir = dbToSta(iterm->getSigType(), iterm->getIoType());
    return dir;
  }
  else if (bterm) {
    PortDirection *dir = dbToSta(bterm->getSigType(), bterm->getIoType());
    return dir;
  }
  else
    return nullptr;
}

VertexIndex
DbNetwork::vertexIndex(const Pin *pin) const
{
  dbITerm *iterm;
  dbBTerm *bterm;
  staToDb(pin, iterm, bterm);
  if (iterm) {
    dbIntProperty *prop = dbIntProperty::find(iterm, "vertex_index");
    if (prop)
      return prop->getValue();
  }
  else if (bterm) {
    dbIntProperty *prop = dbIntProperty::find(bterm, "vertex_index");
    if (prop)
      return prop->getValue();
  }
  return 0;
}

void
DbNetwork::setVertexIndex(Pin *pin,
			      VertexIndex index)
{
  dbITerm *iterm;
  dbBTerm *bterm;
  staToDb(pin, iterm, bterm);
  if (iterm)
    dbIntProperty::create(iterm, "vertex_index", index);
  else if (bterm)
    dbIntProperty::create(bterm, "vertex_index", index);
}

////////////////////////////////////////////////////////////////

const char *
DbNetwork::name(const Net *net) const
{
  dbNet *dnet = staToDb(net);
  dbString name = dnet->getName();
  return tmpStringCopy(name.c_str());
}

Instance *
DbNetwork::instance(const Net *) const
{
  return top_instance_;
}

bool
DbNetwork::isPower(const Net *net) const
{
  dbNet *dnet = staToDb(net);
  return (dnet->getSigType() == dbSigType::POWER);
}

bool
DbNetwork::isGround(const Net *net) const
{
  dbNet *dnet = staToDb(net);
  return (dnet->getSigType() == dbSigType::GROUND);
}

NetPinIterator *
DbNetwork::pinIterator(const Net *net) const
{
  return new DbNetPinIterator(net, this);
}

NetTermIterator *
DbNetwork::termIterator(const Net *net) const
{
  return new DbNetTermIterator(net, this);
}

// override ConcreteNetwork::visitConnectedPins
void
DbNetwork::visitConnectedPins(const Net *net,
				    PinVisitor &visitor,
				    ConstNetSet &visited_nets) const
{
  Network::visitConnectedPins(net, visitor, visited_nets);
}

////////////////////////////////////////////////////////////////

Pin *
DbNetwork::pin(const Term *) const
{
  // No pin at the next level of hierarchy.
  return nullptr;
}

Net *
DbNetwork::net(const Term *term) const
{
  dbBTerm *bterm = staToDb(term);
  dbNet *dnet = bterm->getNet();
  return dbToSta(dnet);
}

////////////////////////////////////////////////////////////////

class DbConstantPinIterator : public ConstantPinIterator
{
public:
  DbConstantPinIterator(const Network *network);
  bool hasNext();
  void next(Pin *&pin, LogicValue &value);
  
private:
};

DbConstantPinIterator::
DbConstantPinIterator(const Network *)
{
}

bool
DbConstantPinIterator::hasNext()
{
  return false;
}

void
DbConstantPinIterator::next(Pin *&pin, LogicValue &value)
{
  value = LogicValue::zero;
  pin = nullptr;
}

ConstantPinIterator *
DbNetwork::constantPinIterator()
{
  return new DbConstantPinIterator(this);
}

////////////////////////////////////////////////////////////////

bool
DbNetwork::isLinked() const
{
  return top_cell_ != nullptr;
}

bool
DbNetwork::linkNetwork(const char *,
			   bool ,
			   Report *)
{
  // Not called.
  return true;
}

// Make ConcreteLibrary/Cell/Port objects for the db library/master/MTerm objects.
void
DbNetwork::init(dbDatabase *db)
{
  db_ = db;
  dbChip *chip = db_->getChip();
  if (chip) {
    block_ = chip->getBlock();
    for (dbLib *lib : db_->getLibs())
      makeLibrary(lib);
    makeTopCell();
  }
}

void
DbNetwork::makeLibrary(dbLib *lib)
{
  dbString lib_name = lib->getName();
  Library *library = makeLibrary(lib_name.c_str(), nullptr);
  for (dbMaster *master : lib->getMasters())
    makeCell(library, master);
}

void
DbNetwork::makeCell(Library *library,
		    dbMaster *master)
{
  dbString cell_name = master->getName();
  Cell *cell = makeCell(library, cell_name.c_str(), true, nullptr);
  LibertyCell *lib_cell = findLibertyCell(cell_name);
  ConcreteCell *ccell = reinterpret_cast<ConcreteCell *>(cell);
  ccell->setLibertyCell(lib_cell);
  for (dbMTerm *mterm : master->getMTerms()) {
    dbString port_name = mterm->getName();
    Port *port = makePort(cell, port_name.c_str());
    PortDirection *dir = dbToSta(mterm->getSigType(), mterm->getIoType());
    setDirection(port, dir);
    if (lib_cell) {
      LibertyPort *lib_port = lib_cell->findLibertyPort(port_name);
      ConcretePort *cport = reinterpret_cast<ConcretePort *>(port);
      cport->setLibertyPort(lib_port);
    }
  }
  groupBusPorts(cell);
}

void
DbNetwork::makeTopCell()
{
  dbString design_name = block_->getName();
  Library *top_lib = makeLibrary(design_name.c_str(), nullptr);
  top_cell_ = makeCell(top_lib, design_name.c_str(), false, nullptr);
  for (dbBTerm *bterm : block_->getBTerms()) {
    dbString port_name = bterm->getName();
    Port *port = makePort(top_cell_, port_name.c_str());
    PortDirection *dir = dbToSta(bterm->getSigType(), bterm->getIoType());
    setDirection(port, dir);
  }
  groupBusPorts(top_cell_);
}

////////////////////////////////////////////////////////////////

// Edit functions

Instance *
DbNetwork::makeInstance(LibertyCell *cell,
			const char *name,
			Instance *parent)
{
  if (parent == top_instance_) {
    Cell *ccell = this->cell(cell);
    dbMaster *master = staToDb(ccell);
    dbInst *inst = dbInst::create(block_, master, name);
    return dbToSta(inst);
  }
  else
    return nullptr;
}

void
DbNetwork::makePins(Instance *)
{
  // This space intentionally left blank.
}

void
DbNetwork::replaceCell(Instance *inst,
		       Cell *cell)
{
  dbMaster *master = staToDb(cell);
  dbInst *dinst = staToDb(inst);
  dinst->swapMaster(master);
}

void
DbNetwork::deleteInstance(Instance *inst)
{
  dbInst *dinst = staToDb(inst);
  dbInst::destroy(dinst);
}

Pin *
DbNetwork::connect(Instance *inst,
		   Port *port,
		   Net *net)
{
  dbNet *dnet = staToDb(net);
  if (inst == top_instance_) {
    const char *port_name = name(port);
    dbBTerm *bterm = dbBTerm::create(dnet, port_name);
    PortDirection *dir = direction(port);
    dbSigType sig_type;
    dbIoType io_type;
    staToDb(dir, sig_type, io_type);
    bterm->setSigType(sig_type);
    bterm->setIoType(io_type);
    return dbToSta(bterm);
  }
  else {
    dbInst *dinst = staToDb(inst);
    dbMTerm *dterm = staToDb(port);
    dbITerm *iterm = dbITerm::connect(dinst, dnet, dterm);
    return dbToSta(iterm);
  }
}

Pin *
DbNetwork::connect(Instance *inst,
		   LibertyPort *port,
		   Net *net)
{
  dbNet *dnet = staToDb(net);
  const char *port_name = port->name();
  if (inst == top_instance_) {
    dbBTerm *bterm = dbBTerm::create(dnet, port_name);
    PortDirection *dir = port->direction();
    dbSigType sig_type;
    dbIoType io_type;
    staToDb(dir, sig_type, io_type);
    bterm->setSigType(sig_type);
    bterm->setIoType(io_type);
    return dbToSta(bterm);
  }
  else {
    dbInst *dinst = staToDb(inst);
    dbMaster *master = dinst->getMaster();
    dbMTerm *dterm = master->findMTerm(port_name);
    dbITerm *iterm = dbITerm::connect(dinst, dnet, dterm);
    return dbToSta(iterm);
  }
}

void
DbNetwork::disconnectPin(Pin *pin)
{
  dbITerm *iterm;
  dbBTerm *bterm;
  staToDb(pin, iterm, bterm);
  if (iterm)
    dbITerm::disconnect(iterm);
  else if (bterm)
    dbBTerm::destroy(bterm);
}

void
DbNetwork::deletePin(Pin *pin)
{
  dbITerm *iterm;
  dbBTerm *bterm;
  staToDb(pin, iterm, bterm);
  if (iterm)
    internalError("not implemented deletePin dbITerm");
  else if (bterm)
    dbBTerm::destroy(bterm);
}

Net *
DbNetwork::makeNet(const char *name,
		   Instance *parent)
{
  if (parent == top_instance_) {
    dbNet *dnet = dbNet::create(block_, name, false);
    return dbToSta(dnet);
  }
  return nullptr;
}

void
DbNetwork::deleteNet(Net *net)
{
  dbNet *dnet = staToDb(net);
  dbNet::destroy(dnet);
}

void
DbNetwork::mergeInto(Net *,
		     Net *)
{
  internalError("unimplemented network function mergeInto\n");
}

Net *
DbNetwork::mergedInto(Net *)
{
  internalError("unimplemented network function mergeInto\n");
}

////////////////////////////////////////////////////////////////

dbInst *
DbNetwork::staToDb(const Instance *instance) const
{
  return reinterpret_cast<dbInst*>(const_cast<Instance*>(instance));
}

dbNet *
DbNetwork::staToDb(const Net *net) const
{
  return reinterpret_cast<dbNet*>(const_cast<Net*>(net));
}

void
DbNetwork::staToDb(const Pin *pin,
		       // Return values.
		       dbITerm *&iterm,
		       dbBTerm *&bterm) const
{
  dbObject *obj = reinterpret_cast<dbObject*>(const_cast<Pin*>(pin));
  dbObjectType type = obj->getObjectType();
  if (type == dbITermObj) {
    iterm = static_cast<dbITerm*>(obj);
    bterm = nullptr;
  }
  else if (type == dbBTermObj) {
    iterm = nullptr;
    bterm = static_cast<dbBTerm*>(obj);
  }
  else
    internalError("pin is not ITerm or BTerm");
}

dbBTerm *
DbNetwork::staToDb(const Term *term) const
{
  return reinterpret_cast<dbBTerm*>(const_cast<Term*>(term));
}

dbMaster *
DbNetwork::staToDb(const Cell *cell) const
{
  Library *lib = library(cell);
  const char *lib_name = name(lib);
  dbLib *dlib = db_->findLib(lib_name);
  const char *cell_name = name(cell);
  return dlib->findMaster(cell_name);
}

dbMTerm *
DbNetwork::staToDb(const Port *port) const
{
  Cell *cell = this->cell(port);
  dbMaster *master = staToDb(cell);
  const char *port_name = name(port);
  return master->findMTerm(port_name);
}

void
DbNetwork::staToDb(PortDirection *dir,
		   // Return values.
		   dbSigType &sig_type,
		   dbIoType &io_type) const
{
  if (dir == PortDirection::input()) {
    sig_type = dbSigType::SIGNAL;
    io_type = dbIoType::INPUT;
  }
  else if (dir == PortDirection::output()) {
    sig_type = dbSigType::SIGNAL;
    io_type = dbIoType::OUTPUT;
  }
  else if (dir == PortDirection::bidirect()) {
    sig_type = dbSigType::SIGNAL;
    io_type = dbIoType::INOUT;
  }
  else if (dir == PortDirection::power()) {
    sig_type = dbSigType::POWER;
    io_type = dbIoType::INOUT;
  }
  else if (dir == PortDirection::ground()) {
    sig_type = dbSigType::GROUND;
    io_type = dbIoType::INOUT;
  }
  else
    internalError("unhandled port direction");
}

////////////////////////////////////////////////////////////////

Instance *
DbNetwork::dbToSta(dbInst *inst) const
{
  return reinterpret_cast<Instance*>(inst);
}

Net *
DbNetwork::dbToSta(dbNet *net) const
{
  return reinterpret_cast<Net*>(net);
}

Pin *
DbNetwork::dbToSta(dbBTerm *bterm) const
{
  return reinterpret_cast<Pin*>(bterm);
}

Pin *
DbNetwork::dbToSta(dbITerm *iterm) const
{
  return reinterpret_cast<Pin*>(iterm);
}

Term *
DbNetwork::dbToStaTerm(dbBTerm *bterm) const
{
  return reinterpret_cast<Term*>(bterm);
}

Port *
DbNetwork::dbToSta(dbMTerm *mterm) const
{
  dbMaster *master = mterm->getMaster();
  Cell *cell = dbToSta(master);
  dbString port_name = mterm->getName();
  Port *port = findPort(cell, port_name.c_str());
  return port;
}

Cell *
DbNetwork::dbToSta(dbMaster *master) const
{
  dbLib *lib = master->getLib();
  dbString lib_name = lib->getName();  
  Library *library = const_cast<DbNetwork*>(this)->findLibrary(lib_name.c_str());
  dbString cell_name = master->getName();
  return findCell(library, cell_name.c_str());
}

PortDirection *
DbNetwork::dbToSta(dbSigType sig_type,
		   dbIoType io_type) const
{
  if (sig_type == dbSigType::POWER)
    return PortDirection::power();
  else if (sig_type == dbSigType::GROUND)
    return PortDirection::ground();
  else if (io_type == dbIoType::INPUT)
    return PortDirection::input();
  else if (io_type == dbIoType::OUTPUT)
    return PortDirection::output();
  else if (io_type == dbIoType::INOUT)
    return PortDirection::bidirect();
  else if (io_type == dbIoType::FEEDTHRU)
    return PortDirection::bidirect();
  else {
    internalError("unknown master term type");
    return PortDirection::bidirect();
  }
}

} // namespace
