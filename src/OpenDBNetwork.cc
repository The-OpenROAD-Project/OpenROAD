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
#include "OpenDBNetwork.hh"

#include "ads.h"
#include "db.h"
#include "logger.h"

namespace sta {

using odb::dbDatabase;
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

class OpenDBLibraryIterator1 : public Iterator<Library*>
{
public:
  OpenDBLibraryIterator1(ConcreteLibraryIterator *iter);
  ~OpenDBLibraryIterator1();
  virtual bool hasNext();
  virtual Library *next();

private:
  ConcreteLibraryIterator *iter_;
};

OpenDBLibraryIterator1::OpenDBLibraryIterator1(ConcreteLibraryIterator * iter) :
  iter_(iter)
{
}

OpenDBLibraryIterator1::~OpenDBLibraryIterator1()
{
  delete iter_;
}

bool
OpenDBLibraryIterator1::hasNext()
{
  return iter_->hasNext();
}

Library *
OpenDBLibraryIterator1::next()
{
  return reinterpret_cast<Library*>(iter_->next());
}

////////////////////////////////////////////////////////////////

class OpenDBInstanceChildIterator : public InstanceChildIterator
{
public:
  OpenDBInstanceChildIterator(const Instance *instance,
			      const OpenDBNetwork *network);
  bool hasNext();
  Instance *next();
  
private:
  const OpenDBNetwork *network_;
  bool top_;
  dbSet<dbInst>::iterator iter_;
  dbSet<dbInst>::iterator end_;
};

OpenDBInstanceChildIterator::OpenDBInstanceChildIterator(const Instance *instance,
							 const OpenDBNetwork *network) :
  network_(network)
{
  if (instance == network->topInstance()) {
    top_ = true;
    dbSet<dbInst> insts = network->block()->getInsts();
    iter_ = insts.begin();
    end_ = insts.end();
  }
  else
    top_ = false;
}

bool
OpenDBInstanceChildIterator::hasNext()
{
  return top_ && iter_ != end_;
}

Instance *
OpenDBInstanceChildIterator::next()
{
  dbInst *child = *iter_;
  iter_++;
  return network_->dbToSta(child);
}


class OpenDBInstanceNetIterator : public InstanceNetIterator
{
public:
  OpenDBInstanceNetIterator(const Instance *instance,
			    const OpenDBNetwork *network);
  bool hasNext();
  Net *next();

private:
  const OpenDBNetwork *network_;
  bool top_;
  dbSet<dbNet>::iterator iter_;
  dbSet<dbNet>::iterator end_;
};

OpenDBInstanceNetIterator::OpenDBInstanceNetIterator(const Instance *instance,
						     const OpenDBNetwork *network) :
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
OpenDBInstanceNetIterator::hasNext()
{
  return top_ && iter_ != end_;
}

Net *
OpenDBInstanceNetIterator::next()
{
  dbNet *net = *iter_;
  iter_++;
  return network_->dbToSta(net);
}

////////////////////////////////////////////////////////////////

class OpenDBInstancePinIterator : public InstancePinIterator
{
public:
  OpenDBInstancePinIterator(const Instance *inst,
			    const OpenDBNetwork *network);
  bool hasNext();
  Pin *next();

private:
  const OpenDBNetwork *network_;
  bool top_;
  dbSet<dbITerm>::iterator iitr_;
  dbSet<dbITerm>::iterator iitr_end_;
  dbSet<dbBTerm>::iterator bitr_;
  dbSet<dbBTerm>::iterator bitr_end_;
  Pin *pin_;
};

OpenDBInstancePinIterator::OpenDBInstancePinIterator(const Instance *inst,
						     const OpenDBNetwork *network) :
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
OpenDBInstancePinIterator::hasNext()
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
OpenDBInstancePinIterator::next()
{
  return pin_;
}

////////////////////////////////////////////////////////////////

class OpenDBNetPinIterator : public NetPinIterator
{
public:
  OpenDBNetPinIterator(const Net *net,
		       const OpenDBNetwork *network);
  bool hasNext();
  Pin *next();

private:
  const OpenDBNetwork *network_;
  dbSet<dbITerm>::iterator _iitr;
  dbSet<dbITerm>::iterator _iitr_end;
  dbSet<dbBTerm>::iterator _bitr;
  dbSet<dbBTerm>::iterator _bitr_end;
  void *_term;
};

OpenDBNetPinIterator::OpenDBNetPinIterator(const Net *net,
					   const OpenDBNetwork *network) :
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
OpenDBNetPinIterator::hasNext()
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
OpenDBNetPinIterator::next()
{
  return (Pin*)_term;
}

////////////////////////////////////////////////////////////////

class OpenDBNetTermIterator : public NetTermIterator
{
public:
  OpenDBNetTermIterator(const Net *net,
			const OpenDBNetwork *network);
  bool hasNext();
  Term *next();

private:
  const OpenDBNetwork *network_;
  dbSet<dbBTerm>::iterator iter_;
  dbSet<dbBTerm>::iterator end_;
};

OpenDBNetTermIterator::OpenDBNetTermIterator(const Net *net,
					     const OpenDBNetwork *network) :
  network_(network)
{
  dbNet *dnet = network_->staToDb(net);
  dbSet<dbBTerm> terms = dnet->getBTerms();
  iter_ = terms.begin();
  end_ = terms.end();
}

bool 
OpenDBNetTermIterator::hasNext()
{
  return iter_ != end_;
}

Term *
OpenDBNetTermIterator::next()
{
  dbBTerm *bterm = *iter_;
  iter_++;
  return network_->dbToStaTerm(bterm);
}

////////////////////////////////////////////////////////////////

Network *
makeOpenDBNetwork()
{
  return new OpenDBNetwork;
}

OpenDBNetwork::OpenDBNetwork() :
  db_(nullptr),
  top_instance_(reinterpret_cast<Instance*>(1)),
  top_cell_(nullptr)
{
}

OpenDBNetwork::~OpenDBNetwork()
{
}

void
OpenDBNetwork::clear()
{
  db_ = nullptr;
}

Instance *
OpenDBNetwork::topInstance() const
{
  return top_instance_;
}

////////////////////////////////////////////////////////////////

const char *
OpenDBNetwork::name(const Instance *instance) const
{
  if (instance == top_instance_) {
    const char *name = block_->getName().c_str();
    return tmpStringCopy(name);
  }
  else {
    dbInst *dinst = staToDb(instance);
    const char *name = dinst->getName().c_str();
    return tmpStringCopy(name);
  }
}

Cell *
OpenDBNetwork::cell(const Instance *instance) const
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
OpenDBNetwork::parent(const Instance *instance) const
{
  if (instance == top_instance_)
    return nullptr;
  return top_instance_;
}

bool
OpenDBNetwork::isLeaf(const Instance *instance) const
{
  if (instance == top_instance_)
    return false;
  return true;
}

Instance *
OpenDBNetwork::findChild(const Instance *parent,
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
OpenDBNetwork::findPin(const Instance *instance, 
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
OpenDBNetwork::findPin(const Instance *instance,
		       const Port *port) const
{
  const char *port_name = this->name(port);
  return findPin(instance, port_name);
}

Net *
OpenDBNetwork::findNet(const Instance *instance, 
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
OpenDBNetwork::findInstNetsMatching(const Instance *instance,
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
OpenDBNetwork::childIterator(const Instance *instance) const
{
  return new OpenDBInstanceChildIterator(instance, this);
}

InstancePinIterator *
OpenDBNetwork::pinIterator(const Instance *instance) const
{
  return new OpenDBInstancePinIterator(instance, this);
}

InstanceNetIterator *
OpenDBNetwork::netIterator(const Instance *instance) const
{
  return new OpenDBInstanceNetIterator(instance, this);
}

////////////////////////////////////////////////////////////////

Instance *
OpenDBNetwork::instance(const Pin *pin) const
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
OpenDBNetwork::net(const Pin *pin) const
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
OpenDBNetwork::term(const Pin *pin) const
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
OpenDBNetwork::port(const Pin *pin) const
{
  dbITerm *iterm;
  dbBTerm *bterm;
  staToDb(pin, iterm, bterm);
  if (iterm) {
    dbMTerm *mterm = iterm->getMTerm();
    return dbToSta(mterm);
  }
  else if (bterm) {
    const char *port_name = bterm->getName().c_str();
    return findPort(top_cell_, port_name);
  }
  else
    return nullptr;
}

PortDirection *
OpenDBNetwork::direction(const Pin *pin) const
{
  Port *port = this->port(pin);
  return this->direction(port);
}

VertexIndex
OpenDBNetwork::vertexIndex(const Pin *pin) const
{
  dbITerm *iterm;
  dbBTerm *bterm;
  staToDb(pin, iterm, bterm);
  if (iterm) {
    dbIntProperty *prop = dbIntProperty::find(iterm, "vertex_index");
    return prop->getValue();
  }
  else if (bterm) {
    dbIntProperty *prop = dbIntProperty::find(bterm, "vertex_index");
    return prop->getValue();
  }
  else
    return 0;
}

void
OpenDBNetwork::setVertexIndex(Pin *pin,
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
OpenDBNetwork::name(const Net *net) const
{
  dbNet *dnet = staToDb(net);
  const char *name = dnet->getName().c_str();
  return tmpStringCopy(name);
}

Instance *
OpenDBNetwork::instance(const Net *) const
{
  return top_instance_;
}

bool
OpenDBNetwork::isPower(const Net *net) const
{
  dbNet *dnet = staToDb(net);
  return (dnet->getSigType() == dbSigType::POWER);
}

bool
OpenDBNetwork::isGround(const Net *net) const
{
  dbNet *dnet = staToDb(net);
  return (dnet->getSigType() == dbSigType::GROUND);
}

NetPinIterator *
OpenDBNetwork::pinIterator(const Net *net) const
{
  return new OpenDBNetPinIterator(net, this);
}

NetTermIterator *
OpenDBNetwork::termIterator(const Net *net) const
{
  return new OpenDBNetTermIterator(net, this);
}

// override ConcreteNetwork::visitConnectedPins
void
OpenDBNetwork::visitConnectedPins(const Net *net,
				    PinVisitor &visitor,
				    ConstNetSet &visited_nets) const
{
  Network::visitConnectedPins(net, visitor, visited_nets);
}

////////////////////////////////////////////////////////////////

Pin *
OpenDBNetwork::pin(const Term *) const
{
  // No pin at the next level of hierarchy.
  return nullptr;
}

Net *
OpenDBNetwork::net(const Term *term) const
{
  dbBTerm *bterm = staToDb(term);
  dbNet *dnet = bterm->getNet();
  return dbToSta(dnet);
}

////////////////////////////////////////////////////////////////

class OpenDBConstantPinIterator : public ConstantPinIterator
{
public:
  OpenDBConstantPinIterator(const Network *network);
  bool hasNext();
  void next(Pin *&pin, LogicValue &value);
  
private:
};

OpenDBConstantPinIterator::
OpenDBConstantPinIterator(const Network *)
{
}

bool
OpenDBConstantPinIterator::hasNext()
{
  return false;
}

void
OpenDBConstantPinIterator::next(Pin *&pin, LogicValue &value)
{
  value = LogicValue::zero;
  pin = nullptr;
}

ConstantPinIterator *
OpenDBNetwork::constantPinIterator()
{
  return new OpenDBConstantPinIterator(this);
}

////////////////////////////////////////////////////////////////

bool
OpenDBNetwork::isLinked() const
{
  return true;
}

bool
OpenDBNetwork::linkNetwork(const char *,
			   bool ,
			   Report *)
{
  // Not called.
  return true;
}

void
OpenDBNetwork::init(dbDatabase *db)
{
  db_ = db;
  block_ = db_->getChip()->getBlock();
  for (dbLib *lib : db_->getLibs())
    makeLibrary(lib);
  makeTopCell();
}

void
OpenDBNetwork::makeLibrary(dbLib *lib)
{
  Library *library = makeLibrary(lib->getName().c_str(), nullptr);
  for (dbMaster *master : lib->getMasters())
    makeCell(library, master);
}

void
OpenDBNetwork::makeCell(Library *library,
			dbMaster *master)
{
  const char *cell_name = master->getName().c_str();
  Cell *cell = makeCell(library, cell_name, true, nullptr);
  LibertyCell *lib_cell = findLibertyCell(cell_name);
  ConcreteCell *ccell = reinterpret_cast<ConcreteCell *>(cell);
  ccell->setLibertyCell(lib_cell);
  for (dbMTerm *mterm : master->getMTerms()) {
    const char *port_name = mterm->getName().c_str();
    Port *port = makePort(cell, port_name);
    PortDirection *dir = dbToSta(mterm->getSigType(), mterm->getIoType());
    setDirection(port, dir);
    if (lib_cell) {
      LibertyPort *lib_port = lib_cell->findLibertyPort(port_name);
      ConcretePort *cport = reinterpret_cast<ConcretePort *>(port);
      cport->setLibertyPort(lib_port);
    }
  }
}

void
OpenDBNetwork::makeTopCell()
{
  const char *design_name = block_->getName().c_str();
  Library *top_lib = makeLibrary(design_name, nullptr);
  top_cell_ = makeCell(top_lib, design_name, false, nullptr);
  for (dbBTerm *bterm : block_->getBTerms()) {
    const char *port_name = bterm->getName().c_str();
    Port *port = makePort(top_cell_, port_name);
    PortDirection *dir = dbToSta(bterm->getSigType(), bterm->getIoType());
    setDirection(port, dir);
  }
}

PortDirection *
OpenDBNetwork::dbToSta(dbSigType sig_type,
		       dbIoType io_type)
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

dbInst *
OpenDBNetwork::staToDb(const Instance *instance) const
{
  return reinterpret_cast<dbInst*>(const_cast<Instance*>(instance));
}

dbNet *
OpenDBNetwork::staToDb(const Net *net) const
{
  return reinterpret_cast<dbNet*>(const_cast<Net*>(net));
}

void
OpenDBNetwork::staToDb(const Pin *pin,
		       // Return values.
		       dbITerm *&iterm,
		       dbBTerm *&bterm) const
{
  dbObject *obj = reinterpret_cast<dbObject*>(const_cast<Pin*>(pin));
  dbObjectType type = obj->getType();
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
OpenDBNetwork::staToDb(const Term *term) const
{
  return reinterpret_cast<dbBTerm*>(const_cast<Term*>(term));
}

Instance *
OpenDBNetwork::dbToSta(dbInst *inst) const
{
  return reinterpret_cast<Instance*>(inst);
}

Net *
OpenDBNetwork::dbToSta(dbNet *net) const
{
  return reinterpret_cast<Net*>(net);
}

Pin *
OpenDBNetwork::dbToSta(dbBTerm *bterm) const
{
  return reinterpret_cast<Pin*>(bterm);
}

Pin *
OpenDBNetwork::dbToSta(dbITerm *iterm) const
{
  return reinterpret_cast<Pin*>(iterm);
}

Term *
OpenDBNetwork::dbToStaTerm(dbBTerm *bterm) const
{
  return reinterpret_cast<Term*>(bterm);
}

Port *
OpenDBNetwork::dbToSta(dbMTerm *mterm) const
{
  dbMaster *master = mterm->getMaster();
  Cell *cell = dbToSta(master);
  const char *port_name = mterm->getName().c_str();
  Port *port = findPort(cell, port_name);
  return port;
}

Cell *
OpenDBNetwork::dbToSta(dbMaster *master) const
{
  dbLib *lib = master->getLib();
  const char *lib_name = lib->getName().c_str();  
  Library *library = const_cast<OpenDBNetwork*>(this)->findLibrary(lib_name);
  const char *cell_name = master->getName().c_str();
  return findCell(library, cell_name);
}

} // namespace
