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

#include "db_sta/dbNetwork.hh"

#include "utility/Logger.h"

#include "sta/PatternMatch.hh"
#include "sta/PortDirection.hh"
#include "sta/Liberty.hh"

#include "opendb/db.h"

namespace sta {

using utl::ORD;

using odb::dbDatabase;
using odb::dbChip;
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
using odb::dbPlacementStatus;

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
			  const dbNetwork *network);
  bool hasNext();
  Instance *next();
  
private:
  const dbNetwork *network_;
  bool top_;
  dbSet<dbInst>::iterator iter_;
  dbSet<dbInst>::iterator end_;
};

DbInstanceChildIterator::DbInstanceChildIterator(const Instance *instance,
						 const dbNetwork *network) :
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
			const dbNetwork *network);
  bool hasNext();
  Net *next();

private:
  const dbNetwork *network_;
  bool top_;
  dbSet<dbNet>::iterator iter_;
  dbSet<dbNet>::iterator end_;
};

DbInstanceNetIterator::DbInstanceNetIterator(const Instance *instance,
					     const dbNetwork *network) :
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
			    const dbNetwork *network);
  bool hasNext();
  Pin *next();

private:
  const dbNetwork *network_;
  bool top_;
  dbSet<dbITerm>::iterator iitr_;
  dbSet<dbITerm>::iterator iitr_end_;
  dbSet<dbBTerm>::iterator bitr_;
  dbSet<dbBTerm>::iterator bitr_end_;
  Pin *next_;
};

DbInstancePinIterator::DbInstancePinIterator(const Instance *inst,
					     const dbNetwork *network) :
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
      next_ = network_->dbToSta(bterm);
      bitr_++;
      return true;
    }
  }
  else {
    while (iitr_ != iitr_end_) {
      dbITerm *iterm = *iitr_;
      if (!(iterm->getSigType() == dbSigType::POWER
	    || iterm->getSigType() == dbSigType::GROUND)) {
	next_ = network_->dbToSta(*iitr_);
	++iitr_;
	return true;
      }
      iitr_++;
    }
    return false;
  }
}

Pin *
DbInstancePinIterator::next()
{
  return next_;
}

////////////////////////////////////////////////////////////////

class DbNetPinIterator : public NetPinIterator
{
public:
  DbNetPinIterator(const Net *net,
		   const dbNetwork *network);
  bool hasNext();
  Pin *next();

private:
  const dbNetwork *network_;
  dbSet<dbITerm>::iterator iitr_;
  dbSet<dbITerm>::iterator iitr_end_;
  Pin *next_;
};

DbNetPinIterator::DbNetPinIterator(const Net *net,
				   const dbNetwork *network) :
  network_(network)
{
  dbNet *dnet = reinterpret_cast<dbNet*>(const_cast<Net*>(net));
  iitr_ = dnet->getITerms().begin();
  iitr_end_ = dnet->getITerms().end();
  next_ = nullptr;
}

bool 
DbNetPinIterator::hasNext()
{
  while (iitr_ != iitr_end_) {
    dbITerm *iterm = *iitr_;
    if (!(iterm->getSigType() == dbSigType::POWER
	  || iterm->getSigType() == dbSigType::GROUND)) {
      next_ = reinterpret_cast<Pin*>(*iitr_);
      ++iitr_;
      return true;
    }
    iitr_++;
  }
  return false;
}

Pin *
DbNetPinIterator::next()
{
  return next_;
}

////////////////////////////////////////////////////////////////

class DbNetTermIterator : public NetTermIterator
{
public:
  DbNetTermIterator(const Net *net,
		    const dbNetwork *network);
  bool hasNext();
  Term *next();

private:
  const dbNetwork *network_;
  dbSet<dbBTerm>::iterator iter_;
  dbSet<dbBTerm>::iterator end_;
};

DbNetTermIterator::DbNetTermIterator(const Net *net,
				     const dbNetwork *network) :
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
makedbNetwork()
{
  return new dbNetwork;
}

dbNetwork::dbNetwork() :
  db_(nullptr),
  top_instance_(reinterpret_cast<Instance*>(1)),
  top_cell_(nullptr)
{
}

dbNetwork::~dbNetwork()
{
}

void
dbNetwork::setDb(dbDatabase *db)
{
  db_ = db;
}

void
dbNetwork::setBlock(dbBlock *block)
{
  db_ = block->getDataBase();
  block_ = block;
}

void
dbNetwork::setLogger(Logger *logger)
{
  logger_ = logger;
}

void
dbNetwork::clear()
{
  ConcreteNetwork::clear();
  db_ = nullptr;
}

Instance *
dbNetwork::topInstance() const
{
  if (top_cell_)
    return top_instance_;
  else
    return nullptr;
}

double
dbNetwork::dbuToMeters(int dist) const
{
  int dbu = db_->getTech()->getDbUnitsPerMicron();
  return dist / (dbu * 1e+6);
}

int
dbNetwork::metersToDbu(double dist) const
{
  int dbu = db_->getTech()->getDbUnitsPerMicron();
  return dist * dbu * 1e+6;
}

////////////////////////////////////////////////////////////////

const char *
dbNetwork::name(const Instance *instance) const
{
  if (instance == top_instance_)
    return tmpStringCopy(block_->getConstName());
  else {
    dbInst *dinst = staToDb(instance);
    return tmpStringCopy(dinst->getConstName());
  }
}

Cell *
dbNetwork::cell(const Instance *instance) const
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
dbNetwork::parent(const Instance *instance) const
{
  if (instance == top_instance_)
    return nullptr;
  return top_instance_;
}

bool
dbNetwork::isLeaf(const Instance *instance) const
{
  if (instance == top_instance_)
    return false;
  return true;
}

Instance *
dbNetwork::findInstance(const char *path_name) const
{
  dbInst *inst = block_->findInst(path_name);
  return dbToSta(inst);
}

Instance *
dbNetwork::findChild(const Instance *parent,
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
dbNetwork::findPin(const Instance *instance, 
		   const char *port_name) const
{
  if (instance == top_instance_) {
    dbBTerm *bterm = block_->findBTerm(port_name);
    return dbToSta(bterm);
  }
  else {
    dbInst *dinst = staToDb(instance);
    dbITerm *iterm = dinst->findITerm(port_name);
    return dbToSta(iterm);
  }
}

Pin *
dbNetwork::findPin(const Instance *instance,
		   const Port *port) const
{
  const char *port_name = this->name(port);
  return findPin(instance, port_name);
}

Net *
dbNetwork::findNet(const Instance *instance, 
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
dbNetwork::findInstNetsMatching(const Instance *instance,
				const PatternMatch *pattern,
				// Return value.
				NetSeq *nets) const
{
  if (instance == top_instance_) {
    if (pattern->hasWildcards()) {
      for (dbNet *dnet : block_->getNets()) {
	const char *net_name = dnet->getConstName();
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
dbNetwork::childIterator(const Instance *instance) const
{
  return new DbInstanceChildIterator(instance, this);
}

InstancePinIterator *
dbNetwork::pinIterator(const Instance *instance) const
{
  return new DbInstancePinIterator(instance, this);
}

InstanceNetIterator *
dbNetwork::netIterator(const Instance *instance) const
{
  return new DbInstanceNetIterator(instance, this);
}

////////////////////////////////////////////////////////////////

Instance *
dbNetwork::instance(const Pin *pin) const
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
dbNetwork::net(const Pin *pin) const
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
dbNetwork::term(const Pin *pin) const
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
dbNetwork::port(const Pin *pin) const
{
  dbITerm *iterm;
  dbBTerm *bterm;
  staToDb(pin, iterm, bterm);
  if (iterm) {
    dbMTerm *mterm = iterm->getMTerm();
    return dbToSta(mterm);
  }
  else if (bterm) {
    const char *port_name = bterm->getConstName();
    return findPort(top_cell_, port_name);
  }
  else
    return nullptr;
}

PortDirection *
dbNetwork::direction(const Pin *pin) const
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

VertexId
dbNetwork::vertexId(const Pin *pin) const
{
  dbITerm *iterm;
  dbBTerm *bterm;
  staToDb(pin, iterm, bterm);
  if (iterm)
    return iterm->staVertexId();
  else if (bterm)
    return bterm->staVertexId();
  return object_id_null;
}

void
dbNetwork::setVertexId(Pin *pin,
		       VertexId id)
{
  dbITerm *iterm;
  dbBTerm *bterm;
  staToDb(pin, iterm, bterm);
  if (iterm)
    return iterm->staSetVertexId(id);
  else if (bterm)
    return bterm->staSetVertexId(id);
}

void
dbNetwork::location(const Pin *pin,
		    // Return values.
		    double &x,
		    double &y,
		    bool &exists) const
{
  if (isPlaced(pin)) {
    Point pt = location(pin);
    x = dbuToMeters(pt.getX());
    y = dbuToMeters(pt.getY());
    exists = true;
  }
  else {
    x = 0;
    y = 0;
    exists = false;
  }
}

Point
dbNetwork::location(const Pin *pin) const
{
  dbITerm *iterm;
  dbBTerm *bterm;
  staToDb(pin, iterm, bterm);
  if (iterm) {
    int x, y;
    if (iterm->getAvgXY(&x, &y))
      return Point(x, y);
    else {
      dbInst *inst = iterm->getInst();
      int x, y;
      inst->getOrigin(x, y);
      return Point(x, y);
    }
  }
  if (bterm) {
    int x, y;
    if (bterm->getFirstPinLocation(x, y))
      return Point(x, y);
  }
  return Point(0, 0);
}

bool
dbNetwork::isPlaced(const Pin *pin) const
{
  dbITerm *iterm;
  dbBTerm *bterm;
  staToDb(pin, iterm, bterm);
  dbPlacementStatus status = dbPlacementStatus::UNPLACED;
  if (iterm) {
    dbInst *inst = iterm->getInst();
    status = inst->getPlacementStatus();
  }
  if (bterm)
    status = bterm->getFirstPinPlacementStatus();
  return status.isPlaced();
}

////////////////////////////////////////////////////////////////

const char *
dbNetwork::name(const Net *net) const
{
  dbNet *dnet = staToDb(net);
  const char *name = dnet->getConstName();
  return tmpStringCopy(name);
}

Instance *
dbNetwork::instance(const Net *) const
{
  return top_instance_;
}

bool
dbNetwork::isPower(const Net *net) const
{
  dbNet *dnet = staToDb(net);
  return (dnet->getSigType() == dbSigType::POWER);
}

bool
dbNetwork::isGround(const Net *net) const
{
  dbNet *dnet = staToDb(net);
  return (dnet->getSigType() == dbSigType::GROUND);
}

NetPinIterator *
dbNetwork::pinIterator(const Net *net) const
{
  return new DbNetPinIterator(net, this);
}

NetTermIterator *
dbNetwork::termIterator(const Net *net) const
{
  return new DbNetTermIterator(net, this);
}

// override ConcreteNetwork::visitConnectedPins
void
dbNetwork::visitConnectedPins(const Net *net,
			      PinVisitor &visitor,
			      ConstNetSet &visited_nets) const
{
  dbNet *db_net = staToDb(net);
  for (dbITerm *iterm : db_net->getITerms()) {
    Pin *pin =  dbToSta(iterm);
    visitor(pin);
  }
  for (dbBTerm *bterm : db_net->getBTerms()) {
    Pin *pin =  dbToSta(bterm);
    visitor(pin);
  }
}

Net *
dbNetwork::highestConnectedNet(Net *net) const
{
  return net;
}

////////////////////////////////////////////////////////////////

Pin *
dbNetwork::pin(const Term *term) const
{
  // Only terms are for top level instance pins, which are also BTerms.
  return reinterpret_cast<Pin*>(const_cast<Term*>(term));
}

Net *
dbNetwork::net(const Term *term) const
{
  dbBTerm *bterm = staToDb(term);
  dbNet *dnet = bterm->getNet();
  return dbToSta(dnet);
}

////////////////////////////////////////////////////////////////

bool
dbNetwork::isLinked() const
{
  return top_cell_ != nullptr;
}

bool
dbNetwork::linkNetwork(const char *,
		       bool ,
		       Report *)
{
  // Not called.
  return true;
}

void
dbNetwork::readLefAfter(dbLib *lib)
{
  makeLibrary(lib);
}

void
dbNetwork::readDefAfter(dbBlock* block)
{
  db_ = block->getDataBase();
  block_ = block;
  readDbNetlistAfter();
}

// Make ConcreteLibrary/Cell/Port objects for the
// db library/master/MTerm objects.
void
dbNetwork::readDbAfter(odb::dbDatabase *db)
{
  db_ = db;
  dbChip *chip = db_->getChip();
  if (chip) {
    block_ = chip->getBlock();
    for (dbLib *lib : db_->getLibs())
      makeLibrary(lib);
    readDbNetlistAfter();
  }
}

void
dbNetwork::makeLibrary(dbLib *lib)
{
  const char *lib_name = lib->getConstName();
  Library *library = makeLibrary(lib_name, nullptr);
  for (dbMaster *master : lib->getMasters())
    makeCell(library, master);
}

void
dbNetwork::makeCell(Library *library,
		    dbMaster *master)
{
  const char *cell_name = master->getConstName();
  Cell *cell = makeCell(library, cell_name, true, nullptr);
  master->staSetCell(reinterpret_cast<void*>(cell));
  ConcreteCell *ccell = reinterpret_cast<ConcreteCell *>(cell);
  ccell->setExtCell(reinterpret_cast<void*>(master));

  LibertyCell *lib_cell = findLibertyCell(cell_name);
  if (lib_cell) {
    ccell->setLibertyCell(lib_cell);
    lib_cell->setExtCell(reinterpret_cast<void*>(master));
  }

  for (dbMTerm *mterm : master->getMTerms()) {
    const char *port_name = mterm->getConstName();
    Port *port = makePort(cell, port_name);
    PortDirection *dir = dbToSta(mterm->getSigType(), mterm->getIoType());
    setDirection(port, dir);
    mterm->staSetPort(reinterpret_cast<void*>(port));
    ConcretePort *cport = reinterpret_cast<ConcretePort *>(port);
    cport->setExtPort(reinterpret_cast<void*>(mterm));

    if (lib_cell) {
      LibertyPort *lib_port = lib_cell->findLibertyPort(port_name);
      if (lib_port) {
	cport->setLibertyPort(lib_port);
	lib_port->setExtPort(mterm);
      }
      else if (!dir->isPowerGround())
	logger_->warn(ORD, 1013, "LEF macro {} pin {} missing from liberty cell.",
		      cell_name,
		      port_name);
    }
  }
  groupBusPorts(cell);
}

void
dbNetwork::readDbNetlistAfter()
{
  makeTopCell();
  findConstantNets();
}

void
dbNetwork::makeTopCell()
{
  const char *design_name = block_->getConstName();
  Library *top_lib = makeLibrary(design_name, nullptr);
  top_cell_ = makeCell(top_lib, design_name, false, nullptr);
  for (dbBTerm *bterm : block_->getBTerms()) {
    const char *port_name = bterm->getConstName();
    Port *port = makePort(top_cell_, port_name);
    PortDirection *dir = dbToSta(bterm->getSigType(), bterm->getIoType());
    setDirection(port, dir);
    
  }
  groupBusPorts(top_cell_);
}

void
dbNetwork::findConstantNets()
{
  for (dbNet *dnet : block_->getNets()) {
    if (dnet->getSigType() == dbSigType::GROUND)
      addConstantNet(dbToSta(dnet), LogicValue::zero);
    else if (dnet->getSigType() == dbSigType::POWER)
      addConstantNet(dbToSta(dnet), LogicValue::one);
  }
}

// Setup mapping from Cell/Port to LibertyCell/LibertyPort.
void
dbNetwork::readLibertyAfter(LibertyLibrary *lib)
{
  for (ConcreteLibrary *clib : library_seq_) {
    if (!clib->isLiberty()) {
      ConcreteLibraryCellIterator *cell_iter = clib->cellIterator();
      while (cell_iter->hasNext()) {
	ConcreteCell *ccell = cell_iter->next();
	// Don't clobber an existing liberty cell so link points to the first.
	if (ccell->libertyCell() == nullptr) {
	  LibertyCell *lcell = lib->findLibertyCell(ccell->name());
	  if (lcell) {
	    lcell->setExtCell(ccell->extCell());
	    ccell->setLibertyCell(lcell);
	    ConcreteCellPortBitIterator *port_iter = ccell->portBitIterator();
	    while (port_iter->hasNext()) {
	      ConcretePort *cport = port_iter->next();
	      const char *port_name = cport->name();
	      LibertyPort *lport = lcell->findLibertyPort(port_name);
	      if (lport) {
		cport->setLibertyPort(lport);
		lport->setExtPort(cport->extPort());
	      }
	      else if (!cport->direction()->isPowerGround())
		logger_->warn(ORD, 1002,
                              "Liberty cell {} pin {} missing from LEF macro.",
			      lcell->name(),
			      port_name);
	    }
	    delete port_iter;
	  }
	}
      }
      delete cell_iter;
    }
  }
}

////////////////////////////////////////////////////////////////

// Edit functions

Instance *
dbNetwork::makeInstance(LibertyCell *cell,
			const char *name,
			Instance *parent)
{
  if (parent == top_instance_) {
    const char *cell_name = cell->name();
    dbMaster *master = db_->findMaster(cell_name);
    if (master) {
      dbInst *inst = dbInst::create(block_, master, name);
      return dbToSta(inst);
    }
  }
  return nullptr;
}

void
dbNetwork::makePins(Instance *)
{
  // This space intentionally left blank.
}

void
dbNetwork::replaceCell(Instance *inst,
		       Cell *cell)
{
  dbMaster *master = staToDb(cell);
  dbInst *dinst = staToDb(inst);
  dinst->swapMaster(master);
}

void
dbNetwork::deleteInstance(Instance *inst)
{
  dbInst *dinst = staToDb(inst);
  dbInst::destroy(dinst);
}

Pin *
dbNetwork::connect(Instance *inst,
		   Port *port,
		   Net *net)
{
  Pin *pin = nullptr;
  dbNet *dnet = staToDb(net);
  if (inst == top_instance_) {
    const char *port_name = name(port);
    dbBTerm *bterm = block_->findBTerm(port_name);
    if (bterm)
      bterm->connect(dnet);
    else {
      bterm = dbBTerm::create(dnet, port_name);
      PortDirection *dir = direction(port);
      dbSigType sig_type;
      dbIoType io_type;
      staToDb(dir, sig_type, io_type);
      bterm->setSigType(sig_type);
      bterm->setIoType(io_type);
    }
    pin = dbToSta(bterm);
  }
  else {
    dbInst *dinst = staToDb(inst);
    dbMTerm *dterm = staToDb(port);
    dbITerm *iterm = dbITerm::connect(dinst, dnet, dterm);
    pin = dbToSta(iterm);
  }
  return pin;
}

// Used by dbStaCbk
// Incrementally update drivers.
void
dbNetwork::connectPinAfter(Pin *pin)
{
  if (isDriver(pin)) {
    Net *net = this->net(pin);
    PinSet *drvrs = net_drvr_pin_map_[net];
    if (drvrs)
      drvrs->insert(pin);
  }
}

Pin *
dbNetwork::connect(Instance *inst,
		   LibertyPort *port,
		   Net *net)
{
  dbNet *dnet = staToDb(net);
  const char *port_name = port->name();
  Pin *pin = nullptr;
  if (inst == top_instance_) {
    dbBTerm *bterm = block_->findBTerm(port_name);
    if (bterm)
      bterm->connect(dnet);
    else
      bterm = dbBTerm::create(dnet, port_name);
    PortDirection *dir = port->direction();
    dbSigType sig_type;
    dbIoType io_type;
    staToDb(dir, sig_type, io_type);
    bterm->setSigType(sig_type);
    bterm->setIoType(io_type);
    pin = dbToSta(bterm);
  }
  else {
    dbInst *dinst = staToDb(inst);
    dbMaster *master = dinst->getMaster();
    dbMTerm *dterm = master->findMTerm(port_name);
    dbITerm *iterm = dbITerm::connect(dinst, dnet, dterm);
    pin = dbToSta(iterm);
  }
  return pin;
}

void
dbNetwork::disconnectPin(Pin *pin)
{
  dbITerm *iterm;
  dbBTerm *bterm;
  staToDb(pin, iterm, bterm);
  if (iterm)
    dbITerm::disconnect(iterm);
  else if (bterm)
    bterm->disconnect();
}

void
dbNetwork::disconnectPinBefore(Pin *pin)
{
  Net *net = this->net(pin);
  // Incrementally update drivers.
  if (net && isDriver(pin)) {
    PinSet *drvrs = net_drvr_pin_map_.findKey(net);
    if (drvrs)
      drvrs->erase(pin);
  }
}

void
dbNetwork::deletePin(Pin *pin)
{
  dbITerm *iterm;
  dbBTerm *bterm;
  staToDb(pin, iterm, bterm);
  if (iterm)
    logger_->critical(ORD, 1003, "deletePin not implemented for dbITerm");
  else if (bterm)
    dbBTerm::destroy(bterm);
}

Net *
dbNetwork::makeNet(const char *name,
		   Instance *parent)
{
  if (parent == top_instance_) {
    dbNet *dnet = dbNet::create(block_, name, false);
    return dbToSta(dnet);
  }
  return nullptr;
}

void
dbNetwork::deleteNet(Net *net)
{
  deleteNetBefore(net);
  dbNet *dnet = staToDb(net);
  dbNet::destroy(dnet);
}

void
dbNetwork::deleteNetBefore(Net *net)
{
  PinSet *drvrs = net_drvr_pin_map_.findKey(net);
  delete drvrs;
  net_drvr_pin_map_.erase(net);
}

void
dbNetwork::mergeInto(Net *,
		     Net *)
{
  logger_->critical(ORD, 1004, "unimplemented network function mergeInto");
}

Net *
dbNetwork::mergedInto(Net *)
{
  logger_->critical(ORD, 1005, "unimplemented network function mergeInto");
  return nullptr;
}

////////////////////////////////////////////////////////////////

dbInst *
dbNetwork::staToDb(const Instance *instance) const
{
  return reinterpret_cast<dbInst*>(const_cast<Instance*>(instance));
}

dbNet *
dbNetwork::staToDb(const Net *net) const
{
  return reinterpret_cast<dbNet*>(const_cast<Net*>(net));
}

void
dbNetwork::staToDb(const Pin *pin,
		   // Return values.
		   dbITerm *&iterm,
		   dbBTerm *&bterm) const
{
  if (pin) {
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
      logger_->critical(ORD, 1006, "pin is not ITerm or BTerm");
  }
  else {
    iterm = nullptr;
    bterm = nullptr;
  }
}

dbBTerm *
dbNetwork::staToDb(const Term *term) const
{
  return reinterpret_cast<dbBTerm*>(const_cast<Term*>(term));
}

dbMaster *
dbNetwork::staToDb(const Cell *cell) const
{
  const ConcreteCell *ccell = reinterpret_cast<const ConcreteCell *>(cell);
  return reinterpret_cast<dbMaster*>(ccell->extCell());
}

dbMaster *
dbNetwork::staToDb(const LibertyCell *cell) const
{
  const ConcreteCell *ccell = cell;
  return reinterpret_cast<dbMaster*>(ccell->extCell());
}

dbMTerm *
dbNetwork::staToDb(const Port *port) const
{
  const ConcretePort *cport = reinterpret_cast<const ConcretePort *>(port);
  return reinterpret_cast<dbMTerm*>(cport->extPort());
}

dbMTerm *
dbNetwork::staToDb(const LibertyPort *port) const
{
  return reinterpret_cast<dbMTerm*>(port->extPort());
}

void
dbNetwork::staToDb(PortDirection *dir,
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
    logger_->critical(ORD, 1007, "unhandled port direction");
}

////////////////////////////////////////////////////////////////

Instance *
dbNetwork::dbToSta(dbInst *inst) const
{
  return reinterpret_cast<Instance*>(inst);
}

Net *
dbNetwork::dbToSta(dbNet *net) const
{
  return reinterpret_cast<Net*>(net);
}

const Net *
dbNetwork::dbToSta(const dbNet *net) const
{
  return reinterpret_cast<const Net*>(net);
}

Pin *
dbNetwork::dbToSta(dbBTerm *bterm) const
{
  return reinterpret_cast<Pin*>(bterm);
}

Pin *
dbNetwork::dbToSta(dbITerm *iterm) const
{
  return reinterpret_cast<Pin*>(iterm);
}

Term *
dbNetwork::dbToStaTerm(dbBTerm *bterm) const
{
  return reinterpret_cast<Term*>(bterm);
}

Port *
dbNetwork::dbToSta(dbMTerm *mterm) const
{
  return reinterpret_cast<Port*>(mterm->staPort());
}

Cell *
dbNetwork::dbToSta(dbMaster *master) const
{
  return reinterpret_cast<Cell*>(master->staCell());
}

PortDirection *
dbNetwork::dbToSta(dbSigType sig_type,
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
    logger_->critical(ORD, 1008, "unknown master term type");
    return PortDirection::bidirect();
  }
}

////////////////////////////////////////////////////////////////

LibertyCell *
dbNetwork::libertyCell(dbInst *inst)
{
  return libertyCell(dbToSta(inst));
}

} // namespace
