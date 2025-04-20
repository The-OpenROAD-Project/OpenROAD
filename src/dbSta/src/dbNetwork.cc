// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// dbSta, OpenSTA on OpenDB

/*
  Need to distinguish between (a) hierarchical cells
  and leaf cells (b) hierarchical ports and leaf ports.

  Leaf ports/cells use weird void* field.

  Two approaches:

  Option A

  (1) Stash all liberty cells pointers in map
  (2) Stash all concrete ports in map
   --note extport (on cport)
   --     extcell (on cell)
   These special cases require us to know the type to get the values.
   So have to discriminate them.

  A.1 use  map, maybe not to bad.
  A.2 Possibility of using pointer tricks to avoid map (mark up lower bit,
unusued in processors).


  option B
  (1) Stash all hierarchical cells in map
  (2) Stash all hierarchical ports in map
  -- ok, but then we need to flush table
  -- and do house keeping with each port update.
  -- feels wrong

  option C
  Somehow use block (which has stash of instances/ports).
  -- wont work needs name and name discrimination needs type
  -- wont work as db has no physical <-> virtual address conversion
  (1) Use bterm hash table (bterm ports)
  (2) use inst hash table (binst)


Recommended conclusion: use map for concrete cells. They are invariant.

 */
#include "db_sta/dbNetwork.hh"

#include <algorithm>
#include <limits>
#include <memory>
#include <set>
#include <unordered_set>
#include <vector>

#include "odb/db.h"
#include "sta/Liberty.hh"
#include "sta/PatternMatch.hh"
#include "sta/PortDirection.hh"
#include "utl/Logger.h"

namespace sta {

using utl::ORD;

using odb::dbBlock;
using odb::dbBoolProperty;
using odb::dbBTerm;
using odb::dbBTermObj;
using odb::dbBusPort;
using odb::dbChip;
using odb::dbDatabase;
using odb::dbInst;
using odb::dbInstObj;
using odb::dbIoType;
using odb::dbITerm;
using odb::dbITermObj;
using odb::dbLib;
using odb::dbMaster;
using odb::dbModBTerm;
using odb::dbModBTermObj;
using odb::dbModInstObj;
using odb::dbModITerm;
using odb::dbModITermObj;
using odb::dbModNetObj;
using odb::dbModule;
using odb::dbModuleObj;
using odb::dbMTerm;
using odb::dbNet;
using odb::dbNetObj;
using odb::dbObject;
using odb::dbObjectType;
using odb::dbPlacementStatus;
using odb::dbSet;
using odb::dbSigType;

// TODO: move to StringUtil
char* tmpStringCopy(const char* str)
{
  char* tmp = makeTmpString(strlen(str) + 1);
  strcpy(tmp, str);
  return tmp;
}

//
// Handling of object ids (Hierachy Mode)
//--------------------------------------
//
// The database assigns a number to each object. These numbers
// are scoped based on the type. Eg dbModInst 1..N or dbInst 1..N.
// The timer requires a unique id for each object for its visit
// pattern, so we uniquify the numbers by suffixing a discriminating
// address pattern to the lower bits and shifting.
// Everytime a new type of timing related object is added, we
// must update this code, so it is isolated and marked up here.
//
// The id is used by the STA traversers to accumulate visited.
// lower 4  bits used to encode type
//

ObjectId dbNetwork::getDbNwkObjectId(dbObjectType typ, ObjectId db_id) const
{
  if (db_id > (std::numeric_limits<ObjectId>::max() >> DBIDTAG_WIDTH)) {
    logger_->error(ORD, 2019, "Database id exceeds capacity");
  }

  switch (typ) {
    case dbITermObj: {
      return ((db_id << DBIDTAG_WIDTH) | DBITERM_ID);
    } break;
    case dbBTermObj: {
      return ((db_id << DBIDTAG_WIDTH) | DBBTERM_ID);
    } break;
    case dbInstObj: {
      return ((db_id << DBIDTAG_WIDTH) | DBINST_ID);
    } break;
    case dbNetObj: {
      return ((db_id << DBIDTAG_WIDTH) | DBNET_ID);
    } break;
    case dbModITermObj: {
      return ((db_id << DBIDTAG_WIDTH) | DBMODITERM_ID);
    } break;
    case dbModBTermObj: {
      return ((db_id << DBIDTAG_WIDTH) | DBMODBTERM_ID);
    } break;
    case dbModInstObj: {
      return ((db_id << DBIDTAG_WIDTH) | DBMODINST_ID);
    } break;
    case dbModNetObj: {
      return ((db_id << DBIDTAG_WIDTH) | DBMODNET_ID);
    } break;
    case dbModuleObj: {
      return ((db_id << DBIDTAG_WIDTH) | DBMODULE_ID);
    } break;
    default:
      logger_->error(
          ORD, 2017, "Unknown database type passed into unique id generation");
      // note the default "exception undefined case" in database is 0.
      // so we reasonably expect upstream tools to handle this.
      return 0;
      break;
  }
  return 0;
}

class DbLibraryIterator1 : public Iterator<Library*>
{
 public:
  explicit DbLibraryIterator1(ConcreteLibraryIterator* iter);
  ~DbLibraryIterator1() override;
  bool hasNext() override;
  Library* next() override;

 private:
  ConcreteLibraryIterator* iter_;
};

DbLibraryIterator1::DbLibraryIterator1(ConcreteLibraryIterator* iter)
    : iter_(iter)
{
}

DbLibraryIterator1::~DbLibraryIterator1()
{
  delete iter_;
}

bool DbLibraryIterator1::hasNext()
{
  return iter_->hasNext();
}

Library* DbLibraryIterator1::next()
{
  return reinterpret_cast<Library*>(iter_->next());
}

//
// check the leaves accessible from the network
// match those accessible from block.
//

////////////////////////////////////////////////////////////////

class DbInstanceChildIterator : public InstanceChildIterator
{
 public:
  DbInstanceChildIterator(const Instance* instance, const dbNetwork* network);
  bool hasNext() override;
  Instance* next() override;

 private:
  const dbNetwork* network_;
  bool top_;
  dbSet<dbInst>::iterator dbinst_iter_;
  dbSet<dbInst>::iterator dbinst_end_;
  dbSet<dbModInst>::iterator modinst_iter_;
  dbSet<dbModInst>::iterator modinst_end_;
};

DbInstanceChildIterator::DbInstanceChildIterator(const Instance* instance,
                                                 const dbNetwork* network)
    : network_(network)
{
  dbBlock* block = network->block();

  // original code for non hierarchy
  if (!network->hasHierarchy()) {
    if (instance == network->topInstance() && block) {
      dbSet<dbInst> insts = block->getInsts();
      top_ = true;
      dbinst_iter_ = insts.begin();
      dbinst_end_ = insts.end();
    } else {
      top_ = false;
    }
  } else {
    dbModule* module = nullptr;
    if (instance == network->topInstance() && block) {
      module = block->getTopModule();
      top_ = true;
    } else {
      top_ = false;
      // need to get module for instance
      dbInst* db_inst = nullptr;
      dbModInst* mod_inst = nullptr;
      network->staToDb(instance, db_inst, mod_inst);
      if (mod_inst) {
        module = mod_inst->getMaster();
      }
    }
    if (module) {
      modinst_iter_ = module->getModInsts().begin();
      modinst_end_ = module->getModInsts().end();
      dbinst_iter_ = module->getInsts().begin();
      dbinst_end_ = module->getInsts().end();
    }
  }
}

bool DbInstanceChildIterator::hasNext()
{
  return !((dbinst_iter_ == dbinst_end_) && (modinst_iter_ == modinst_end_));
}

Instance* DbInstanceChildIterator::next()
{
  Instance* ret = nullptr;
  if (dbinst_iter_ != dbinst_end_) {
    dbInst* child = *dbinst_iter_;
    dbinst_iter_++;
    ret = network_->dbToSta(child);
  } else if (modinst_iter_ != modinst_end_) {
    dbModInst* child = *modinst_iter_;
    modinst_iter_++;
    ret = network_->dbToSta(child);
  }
  return ret;
}

class DbInstanceNetIterator : public InstanceNetIterator
{
 public:
  DbInstanceNetIterator(const Instance* instance, const dbNetwork* network);
  bool hasNext() override;
  Net* next() override;

 private:
  const dbNetwork* network_;
  dbSet<dbNet>::iterator iter_;
  dbSet<dbNet>::iterator end_;
  dbSet<dbModNet>::iterator mod_net_iter_;
  dbSet<dbModNet>::iterator mod_net_end_;
};

DbInstanceNetIterator::DbInstanceNetIterator(const Instance* instance,
                                             const dbNetwork* network)
    : network_(network)
{
  if (network_->hasHierarchy()) {
    dbInst* db_inst;
    dbModInst* mod_inst;
    network_->staToDb(instance, db_inst, mod_inst);
    if (mod_inst) {
      dbModule* master = mod_inst->getMaster();
      dbSet<dbModNet> nets = master->getModNets();
      mod_net_iter_ = nets.begin();
      mod_net_end_ = nets.end();
    }
  } else {
    if (instance == network->topInstance()) {
      dbSet<dbNet> nets = network->block()->getNets();
      iter_ = nets.begin();
      end_ = nets.end();
    }
  }
}

bool DbInstanceNetIterator::hasNext()
{
  if (network_->hasHierarchy()) {
    return mod_net_iter_ != mod_net_end_;
  }
  return iter_ != end_;
}

Net* DbInstanceNetIterator::next()
{
  if (network_->hasHierarchy()) {
    dbModNet* net = *mod_net_iter_;
    mod_net_iter_++;
    return network_->dbToSta(net);
  }
  dbNet* net = *iter_;
  iter_++;
  return network_->dbToSta(net);
}

////////////////////////////////////////////////////////////////
class DbInstancePinIterator : public InstancePinIterator
{
 public:
  DbInstancePinIterator(const Instance* inst, const dbNetwork* network);
  bool hasNext() override;
  Pin* next() override;

 private:
  const dbNetwork* network_;
  bool top_;
  dbSet<dbITerm>::iterator iitr_;
  dbSet<dbITerm>::iterator iitr_end_;
  dbSet<dbBTerm>::iterator bitr_;
  dbSet<dbBTerm>::iterator bitr_end_;
  dbSet<dbModITerm>::iterator mi_itr_;
  dbSet<dbModITerm>::iterator mi_itr_end_;
  Pin* next_ = nullptr;
  dbInst* db_inst_;
  dbModInst* mod_inst_;
};

DbInstancePinIterator::DbInstancePinIterator(const Instance* inst,
                                             const dbNetwork* network)
    : network_(network)
{
  top_ = (inst == network->topInstance());
  db_inst_ = nullptr;
  mod_inst_ = nullptr;

  if (top_) {
    dbBlock* block = network->block();
    // it is possible that a block might not have been created if no design
    // has been read in.
    if (block) {
      bitr_ = block->getBTerms().begin();
      bitr_end_ = block->getBTerms().end();
    }
  } else {
    dbInst* db_inst;
    dbModInst* mod_inst;
    network_->staToDb(inst, db_inst, mod_inst);
    if (db_inst) {
      iitr_ = db_inst->getITerms().begin();
      iitr_end_ = db_inst->getITerms().end();
    } else if (mod_inst) {
      if (network_->hasHierarchy()) {
        mi_itr_ = mod_inst->getModITerms().begin();
        mi_itr_end_ = mod_inst->getModITerms().end();
      }
    }
  }
}

bool DbInstancePinIterator::hasNext()
{
  if (top_) {
    if (bitr_ == bitr_end_) {
      return false;
    }
    dbBTerm* bterm = *bitr_;
    next_ = network_->dbToSta(bterm);
    bitr_++;
    return true;
  }

  while (iitr_ != iitr_end_) {
    dbITerm* iterm = *iitr_;
    if (!iterm->getSigType().isSupply()) {
      next_ = network_->dbToSta(*iitr_);
      ++iitr_;
      return true;
    }
    iitr_++;
  }

  if ((mi_itr_ != mi_itr_end_) && (network_->hasHierarchy())) {
    dbModITerm* mod_iterm = *mi_itr_;
    next_ = network_->dbToSta(mod_iterm);
    mi_itr_++;
    return true;
  }
  return false;
}

Pin* DbInstancePinIterator::next()
{
  return next_;
}

////////////////////////////////////////////////////////////////

class DbNetPinIterator : public NetPinIterator
{
 public:
  DbNetPinIterator(const Net* net, const dbNetwork* network);
  bool hasNext() override;
  Pin* next() override;

 private:
  dbSet<dbITerm>::iterator iitr_;
  dbSet<dbITerm>::iterator iitr_end_;
  dbSet<dbModITerm>::iterator mitr_;
  dbSet<dbModITerm>::iterator mitr_end_;
  Pin* next_;
  const dbNetwork* network_;
};

DbNetPinIterator::DbNetPinIterator(const Net* net, const dbNetwork* network)
{
  dbNet* dnet = nullptr;
  dbModNet* modnet = nullptr;
  network_ = network;
  network->staToDb(net, dnet, modnet);
  next_ = nullptr;
  if (dnet) {
    iitr_ = dnet->getITerms().begin();
    iitr_end_ = dnet->getITerms().end();
  }
  if (modnet) {
    iitr_ = modnet->getITerms().begin();
    iitr_end_ = modnet->getITerms().end();
    mitr_ = modnet->getModITerms().begin();
    mitr_end_ = modnet->getModITerms().end();
  }
}

bool DbNetPinIterator::hasNext()
{
  while (iitr_ != iitr_end_) {
    dbITerm* iterm = *iitr_;
    if (!iterm->getSigType().isSupply()) {
      next_ = reinterpret_cast<Pin*>(*iitr_);
      ++iitr_;
      return true;
    }
    iitr_++;
  }
  if ((mitr_ != mitr_end_) && (network_->hasHierarchy())) {
    next_ = reinterpret_cast<Pin*>(*mitr_);
    ++mitr_;
    return true;
  }
  return false;
}

Pin* DbNetPinIterator::next()
{
  return next_;
}

////////////////////////////////////////////////////////////////

class DbNetTermIterator : public NetTermIterator
{
 public:
  DbNetTermIterator(const Net* net, const dbNetwork* network);
  bool hasNext() override;
  Term* next() override;

 private:
  const dbNetwork* network_;
  dbSet<dbBTerm>::iterator iter_;
  dbSet<dbBTerm>::iterator end_;
  dbSet<dbModBTerm>::iterator mod_iter_;
  dbSet<dbModBTerm>::iterator mod_end_;
};

DbNetTermIterator::DbNetTermIterator(const Net* net, const dbNetwork* network)
    : network_(network)
{
  dbModNet* modnet = nullptr;
  dbNet* dnet = nullptr;
  network_->staToDb(net, dnet, modnet);
  if (dnet && !modnet) {
    dbSet<dbBTerm> terms = dnet->getBTerms();
    iter_ = terms.begin();
    end_ = terms.end();
  } else if (modnet) {
    dbSet<dbBTerm> terms = modnet->getBTerms();
    iter_ = terms.begin();
    end_ = terms.end();
    dbSet<dbModBTerm> modbterms = modnet->getModBTerms();
    mod_iter_ = modbterms.begin();
    mod_end_ = modbterms.end();
  }
}

bool DbNetTermIterator::hasNext()
{
  return (mod_iter_ != mod_end_ || iter_ != end_);
}

Term* DbNetTermIterator::next()
{
  if (iter_ != end_) {
    dbBTerm* bterm = *iter_;
    iter_++;
    return network_->dbToStaTerm(bterm);
  }
  if (mod_iter_ != mod_end_ && (network_->hasHierarchy())) {
    dbModBTerm* modbterm = *mod_iter_;
    mod_iter_++;
    return network_->dbToStaTerm(modbterm);
  }
  return nullptr;
}

////////////////////////////////////////////////////////////////

dbNetwork::dbNetwork() : top_instance_(reinterpret_cast<Instance*>(1))
{
}

dbNetwork::~dbNetwork() = default;

void dbNetwork::init(dbDatabase* db, Logger* logger)
{
  db_ = db;
  logger_ = logger;
}

void dbNetwork::setBlock(dbBlock* block)
{
  block_ = block;
  readDbNetlistAfter();
}

void dbNetwork::clear()
{
  ConcreteNetwork::clear();
  db_ = nullptr;
}

Instance* dbNetwork::topInstance() const
{
  if (top_cell_) {
    return top_instance_;
  }
  return nullptr;
}

double dbNetwork::dbuToMeters(int dist) const
{
  int dbu = db_->getTech()->getDbUnitsPerMicron();
  return dist / (dbu * 1e+6);
}

int dbNetwork::metersToDbu(double dist) const
{
  int dbu = db_->getTech()->getDbUnitsPerMicron();
  return dist * dbu * 1e+6;
}

ObjectId dbNetwork::id(const Port* port) const
{
  if (!port) {
    // should not match anything else
    return std::numeric_limits<ObjectId>::max();
  }

  if (hierarchy_) {
    if (isConcretePort(port)) {
      return ConcreteNetwork::id(port);
    }
    dbObject* obj = reinterpret_cast<dbObject*>(const_cast<Port*>(port));
    dbObjectType type = obj->getObjectType();
    return getDbNwkObjectId(type, obj->getId());
  }
  // default
  return ConcreteNetwork::id(port);
}

// Note:
// This api call is subtly used by the sta/verilog/VerilogWriter in sta.
// The verilog writer in sta a hash of modules written out, cells.hasKey, which
// uses the cell id as index and will land here (before it defaulted to the
// concrete network api, which is ok for flat networks but wont work with
// hierarchy).
//

ObjectId dbNetwork::id(const Cell* cell) const
{
  // in hierarchical flow we use the object id for the index
  if (hierarchy_) {
    if (!isConcreteCell(cell)) {
      dbObject* obj = reinterpret_cast<dbObject*>(const_cast<Cell*>(cell));
      dbObjectType type = obj->getObjectType();
      return getDbNwkObjectId(type, obj->getId());
    }
  }
  // default behaviour use the concrete cell.
  const ConcreteCell* ccell = reinterpret_cast<const ConcreteCell*>(cell);
  return ccell->id();
}

////////////////////////////////////////////////////////////////

ObjectId dbNetwork::id(const Instance* instance) const
{
  if (instance == top_instance_) {
    return 0;
  }
  if (hierarchy_) {
    dbObject* obj
        = reinterpret_cast<dbObject*>(const_cast<Instance*>(instance));
    dbObjectType type = obj->getObjectType();
    return getDbNwkObjectId(type, obj->getId());
  }
  return staToDb(instance)->getId();
}

const char* dbNetwork::name(const Port* port) const
{
  if (isConcretePort(port)) {
    const ConcretePort* cport = reinterpret_cast<const ConcretePort*>(port);
    return cport->name();
  }
  dbMTerm* mterm = nullptr;
  dbModBTerm* modbterm = nullptr;
  dbBTerm* bterm = nullptr;
  staToDb(port, bterm, mterm, modbterm);
  std::string name;
  if (bterm) {
    name = bterm->getName();
  }
  if (mterm) {
    name = mterm->getName();
  }
  if (modbterm) {
    name = modbterm->getName();
  }

  if (name.empty()) {
    return nullptr;
  }

  if (hierarchy_) {
    size_t last_idx = name.find_last_of('/');
    if (last_idx != std::string::npos) {
      name = name.substr(last_idx + 1);
    }
  }
  return tmpStringCopy(name.c_str());
}

const char* dbNetwork::busName(const Port* port) const
{
  if (isConcretePort(port)) {
    const ConcretePort* cport = reinterpret_cast<const ConcretePort*>(port);
    return cport->busName();
  }
  dbMTerm* mterm = nullptr;
  dbModBTerm* modbterm = nullptr;
  dbBTerm* bterm = nullptr;
  staToDb(port, bterm, mterm, modbterm);
  if (modbterm) {
    if (modbterm->isBusPort()) {
      // modbterm has the name of the bus and refers
      // to the BusPort which has the contents.
      return modbterm->getName();
    }
  }
  logger_->error(ORD, 2020, "Database badly formed bus name");
  return nullptr;
}

const char* dbNetwork::name(const Instance* instance) const
{
  if (instance == top_instance_) {
    return tmpStringCopy(block_->getConstName());
  }

  dbInst* db_inst;
  dbModInst* mod_inst;
  staToDb(instance, db_inst, mod_inst);
  std::string name;
  if (db_inst) {
    name = db_inst->getName();
  }
  if (mod_inst) {
    name = mod_inst->getName();
  }

  if (hierarchy_) {
    size_t last_idx = name.find_last_of('/');
    if (last_idx != std::string::npos) {
      name = name.substr(last_idx + 1);
    }
  }
  return tmpStringCopy(name.c_str());
}

const char* dbNetwork::name(const Cell* cell) const
{
  dbMaster* db_master;
  dbModule* db_module;
  staToDb(cell, db_master, db_module);
  if (db_master || cell == top_cell_) {
    return ConcreteNetwork::name(cell);
  }
  if (db_module) {
    return db_module->getName();
  }
  return nullptr;
}

std::string dbNetwork::getAttribute(const Cell* cell,
                                    const std::string& key) const
{
  dbMaster* db_master;
  dbModule* db_module;
  staToDb(cell, db_master, db_module);
  odb::dbObject* obj;
  if (db_master) {
    obj = db_master;
  } else {
    obj = db_module;
  }
  if (obj) {
    auto property = odb::dbStringProperty::find(obj, key.c_str());
    if (property) {
      return property->getValue();
    }
  }
  return "";
}

void dbNetwork::setAttribute(Cell* cell,
                             const std::string& key,
                             const std::string& value)
{
  dbMaster* db_master;
  dbModule* db_module;
  staToDb(cell, db_master, db_module);
  odb::dbObject* obj;
  if (db_master) {
    obj = db_master;
  } else {
    obj = db_module;
  }
  if (obj) {
    auto property = odb::dbStringProperty::find(obj, key.c_str());
    if (property) {
      property->setValue(value.c_str());
    } else {
      odb::dbStringProperty::create(obj, key.c_str(), value.c_str());
    }
  }
}

////////////////////////////////////////////////////////////////
// Module port iterator, allows traversal across dbModulePorts
// Note that a port is not the same as a dbModBTerm.
//
// A Port is a higher level concept.
// A Port can be one of:
// (a) singleton
// (b) bus port (which has many singletons inside it)
// (c) bundle port -- todo (which has many singletons inside it).
//
// This iterator uses the odb generate iterator dbModulePortItr
// which has knowledge of he underlying port types and skips
// over their contents.
//
//

class dbModulePortIterator : public CellPortIterator
{
 public:
  explicit dbModulePortIterator(dbModule* cell);
  ~dbModulePortIterator() override = default;
  bool hasNext() override;
  Port* next() override;

 private:
  dbSet<dbModBTerm>::iterator iter_;
  dbSet<dbModBTerm>::iterator end_;
  const dbModule* module_;
};

dbModulePortIterator::dbModulePortIterator(dbModule* cell)
{
  iter_ = cell->getPorts().begin();
  end_ = cell->getPorts().end();
  module_ = cell;
}

bool dbModulePortIterator::hasNext()
{
  if (iter_ != end_) {
    return true;
  }
  return false;
}

Port* dbModulePortIterator::next()
{
  dbModBTerm* ret = *iter_;
  iter_++;
  return (reinterpret_cast<Port*>(ret));
}

CellPortIterator* dbNetwork::portIterator(const Cell* cell) const
{
  if (isConcreteCell(cell) || cell == top_cell_) {
    return ConcreteNetwork::portIterator(cell);
  }
  dbMaster* db_master;
  dbModule* db_module;
  staToDb(cell, db_master, db_module);
  if (db_module) {
    return new dbModulePortIterator(db_module);
  }
  return nullptr;
}

Cell* dbNetwork::cell(const Port* port) const
{
  if (isConcretePort(port)) {
    const ConcretePort* cport = reinterpret_cast<const ConcretePort*>(port);
    return cport->cell();
  }
  const dbModBTerm* modbterm = reinterpret_cast<const dbModBTerm*>(port);
  return (reinterpret_cast<Cell*>(modbterm->getParent()));
}

Cell* dbNetwork::cell(const Instance* instance) const
{
  if (instance == top_instance_) {
    return reinterpret_cast<Cell*>(top_cell_);
  }
  dbInst* db_inst;
  dbModInst* mod_inst;
  staToDb(instance, db_inst, mod_inst);
  if (db_inst) {
    dbMaster* master = db_inst->getMaster();
    return dbToSta(master);
  }
  if (mod_inst) {
    dbModule* master = mod_inst->getMaster();
    return dbToSta(master);
  }
  return nullptr;
}

Instance* dbNetwork::parent(const Instance* instance) const
{
  if (instance == top_instance_) {
    return nullptr;
  }
  dbInst* db_inst;
  dbModInst* mod_inst;
  staToDb(instance, db_inst, mod_inst);
  if (mod_inst) {
    auto parent_module = mod_inst->getParent();
    if (parent_module) {
      auto parent_inst = parent_module->getModInst();
      if (parent_inst) {
        return dbToSta(parent_inst);
      }
    }
  }
  if (db_inst) {
    if (!hasHierarchy()) {
      return top_instance_;
    }
    auto parent_module = db_inst->getModule();
    if (parent_module) {
      auto parent_inst = parent_module->getModInst();
      if (parent_inst) {
        return dbToSta(parent_inst);
      }
    }
  }
  return top_instance_;
}

Port* dbNetwork::findPort(const Cell* cell, const char* name) const
{
  if (hierarchy_) {
    dbMaster* db_master;
    dbModule* db_module;
    staToDb(cell, db_master, db_module);
    if (db_module) {
      dbModBTerm* mod_bterm = db_module->findModBTerm(name);
      Port* ret = dbToSta(mod_bterm);
      return ret;
    }
  }
  const ConcreteCell* ccell = reinterpret_cast<const ConcreteCell*>(cell);
  return reinterpret_cast<Port*>(ccell->findPort(name));
}

bool dbNetwork::isLeaf(const Pin* pin) const
{
  dbITerm* iterm;
  dbBTerm* bterm;
  dbModITerm* moditerm;
  dbModBTerm* modbterm;

  staToDb(pin, iterm, bterm, moditerm, modbterm);
  if (moditerm || modbterm) {
    return false;
  }
  return true;
}

bool dbNetwork::isLeaf(const Instance* instance) const
{
  if (instance == top_instance_) {
    return false;
  }
  if (hierarchy_) {
    dbMaster* db_master;
    dbModule* db_module;
    Cell* cur_cell = cell(instance);
    staToDb(cur_cell, db_master, db_module);
    if (db_module)
      return false;
    return true;
  }
  return instance != top_instance_;
}

Instance* dbNetwork::findInstance(const char* path_name) const
{
  dbInst* inst = block_->findInst(path_name);
  return dbToSta(inst);
}

Instance* dbNetwork::findChild(const Instance* parent, const char* name) const
{
  if (parent == top_instance_) {
    dbInst* inst = block_->findInst(name);
    if (!inst) {
      auto top_module = block_->getTopModule();
      dbModInst* mod_inst = top_module->findModInst(name);
      return dbToSta(mod_inst);
    }
    return dbToSta(inst);
  }
  dbInst* db_inst;
  dbModInst* mod_inst;
  staToDb(parent, db_inst, mod_inst);
  if (!mod_inst) {
    return nullptr;
  }
  dbModule* master_module = mod_inst->getMaster();
  dbModInst* child_inst = master_module->findModInst(name);
  if (child_inst) {
    return dbToSta(child_inst);
  }
  // Look for a leaf instance
  std::string full_name = mod_inst->getHierarchicalName();
  full_name += pathDivider() + std::string(name);
  dbInst* inst = block_->findInst(full_name.c_str());
  return dbToSta(inst);
}

// port -> pin by name.
Pin* dbNetwork::findPin(const Instance* instance, const char* port_name) const
{
  if (instance == top_instance_) {
    dbBTerm* bterm = block_->findBTerm(port_name);
    return dbToSta(bterm);
  }
  dbInst* db_inst;
  dbModInst* mod_inst;
  staToDb(instance, db_inst, mod_inst);
  if (db_inst) {
    dbITerm* iterm = db_inst->findITerm(port_name);
    return dbToSta(iterm);
  }
  if (mod_inst) {
    dbModule* module = mod_inst->getMaster();
    dbModBTerm* mbterm = module->findModBTerm(port_name);
    if (mbterm) {
      dbModITerm* moditerm = mbterm->getParentModITerm();
      return dbToSta(moditerm);
    }
  }
  return nullptr;
}

Pin* dbNetwork::findPin(const Instance* instance, const Port* port) const
{
  const char* port_name = this->name(port);
  return findPin(instance, port_name);
}

Net* dbNetwork::findNet(const Instance* instance, const char* net_name) const
{
  dbModule* scope = nullptr;

  if (instance == top_instance_) {
    scope = block_->getTopModule();
    dbNet* dnet = block_->findNet(net_name);
    if (dnet) {
      return dbToSta(dnet);
    }
    dbModNet* modnet = scope->getModNet(net_name);
    if (modnet) {
      return dbToSta(modnet);
    }
    return nullptr;
  }

  dbInst* db_inst = nullptr;
  dbModInst* mod_inst = nullptr;
  staToDb(instance, db_inst, mod_inst);

  // check to see if net is in flat space
  std::string flat_net_name = pathName(instance);
  flat_net_name += pathDivider() + std::string(net_name);
  dbNet* dnet = block_->findNet(flat_net_name.c_str());
  if (dnet) {
    return dbToSta(dnet);
  }

  if (mod_inst) {
    scope = mod_inst->getMaster();
    dbModNet* modnet = scope->getModNet(net_name);
    if (modnet) {
      return dbToSta(modnet);
    }
  }

  return nullptr;
}

void dbNetwork::findInstNetsMatching(const Instance* instance,
                                     const PatternMatch* pattern,
                                     NetSeq& nets) const
{
  if (instance == top_instance_) {
    if (pattern->hasWildcards()) {
      for (dbNet* dnet : block_->getNets()) {
        const char* net_name = dnet->getConstName();
        if (pattern->match(net_name)) {
          nets.push_back(dbToSta(dnet));
        }
      }
    } else {
      dbNet* dnet = block_->findNet(pattern->pattern());
      if (dnet) {
        nets.push_back(dbToSta(dnet));
      }
    }
  }
}

InstanceChildIterator* dbNetwork::childIterator(const Instance* instance) const
{
  return new DbInstanceChildIterator(instance, this);
}

InstancePinIterator* dbNetwork::pinIterator(const Instance* instance) const
{
  return new DbInstancePinIterator(instance, this);
}

InstanceNetIterator* dbNetwork::netIterator(const Instance* instance) const
{
  return new DbInstanceNetIterator(instance, this);
}

std::string dbNetwork::getAttribute(const Instance* inst,
                                    const std::string& key) const
{
  dbInst* db_inst;
  dbModInst* mod_inst;
  staToDb(inst, db_inst, mod_inst);
  odb::dbObject* obj;
  if (db_inst) {
    obj = db_inst;
  } else {
    obj = mod_inst;
  }
  if (obj) {
    auto property = odb::dbStringProperty::find(obj, key.c_str());
    if (property) {
      return property->getValue();
    }
  }
  return "";
}

void dbNetwork::setAttribute(Instance* instance,
                             const std::string& key,
                             const std::string& value)
{
  dbInst* db_inst;
  dbModInst* mod_inst;
  staToDb(instance, db_inst, mod_inst);
  odb::dbObject* obj;
  if (db_inst) {
    obj = db_inst;
  } else {
    obj = mod_inst;
  }
  if (obj) {
    auto property = odb::dbStringProperty::find(obj, key.c_str());
    if (property) {
      property->setValue(value.c_str());
    } else {
      odb::dbStringProperty::create(obj, key.c_str(), value.c_str());
    }
  }
}

////////////////////////////////////////////////////////////////

ObjectId dbNetwork::id(const Pin* pin) const
{
  dbITerm* iterm = nullptr;
  dbBTerm* bterm = nullptr;
  dbModITerm* moditerm = nullptr;
  dbModBTerm* modbterm = nullptr;

  staToDb(pin, iterm, bterm, moditerm, modbterm);

  if (hierarchy_) {
    // get the id for hierarchical objects using dbid.
    dbObject* obj = reinterpret_cast<dbObject*>(const_cast<Pin*>(pin));
    dbObjectType type = obj->getObjectType();
    return getDbNwkObjectId(type, obj->getId());
  }
  if (iterm != nullptr) {
    return iterm->getId() << 1;
  }
  if (bterm != nullptr) {
    return (bterm->getId() << 1) + 1;
  }
  return 0;
}

Instance* dbNetwork::instance(const Pin* pin) const
{
  dbITerm* iterm = nullptr;
  dbBTerm* bterm = nullptr;
  dbModITerm* moditerm = nullptr;
  dbModBTerm* modbterm = nullptr;

  staToDb(pin, iterm, bterm, moditerm, modbterm);
  if (iterm) {
    dbInst* dinst = iterm->getInst();
    return dbToSta(dinst);
  }
  if (bterm) {
    return top_instance_;
  }
  if (moditerm) {
    dbModInst* mod_inst = moditerm->getParent();
    return dbToSta(mod_inst);
  }
  if (modbterm) {
    dbModule* module = modbterm->getParent();
    dbModInst* mod_inst = module->getModInst();
    return dbToSta(mod_inst);
  }
  return nullptr;
}

Net* dbNetwork::net(const Pin* pin) const
{
  dbITerm* iterm = nullptr;
  dbBTerm* bterm = nullptr;
  dbModITerm* moditerm = nullptr;
  dbModBTerm* modbterm = nullptr;

  staToDb(pin, iterm, bterm, moditerm, modbterm);
  if (iterm) {
    dbNet* dnet = iterm->getNet();
    dbModNet* mnet = iterm->getModNet();

    //
    // TODO: reverse this logic so we always get the
    // flat net, and fix the verilog writer.
    //

    // It is possible when writing out a hierarchical network
    // that we have both a mod net and a dbinst net.
    // In the case of writing out a hierachical network we always
    // choose the mnet.

    if (mnet) {
      return dbToSta(mnet);
    }

    if (dnet) {
      return dbToSta(dnet);
    }
  }
  // only pins which act as bterms are top levels and have no net
  if (bterm) {
    //    dbNet* dnet = bterm -> getNet();
    //    return dbToSta(dnet);
    ;
  }
  if (moditerm) {
    dbModNet* mnet = moditerm->getModNet();
    return dbToSta(mnet);
  }
  if (modbterm) {
    dbModNet* mnet = modbterm->getModNet();
    return dbToSta(mnet);
  }
  return nullptr;
}

/*
Get the db net (flat net) for the pin
*/

dbNet* dbNetwork::flatNet(const Pin* pin) const
{
  dbNet* db_net;
  dbModNet* db_modnet;
  net(pin, db_net, db_modnet);
  return db_net;
}

dbModNet* dbNetwork::hierNet(const Pin* pin) const
{
  dbNet* db_net;
  dbModNet* db_modnet;
  net(pin, db_net, db_modnet);
  return db_modnet;
}

/*
Get the dbnet or the moddbnet for a pin
Sometimes a pin can be hooked to both and we want to expose them
both, so we add this api
 */
void dbNetwork::net(const Pin* pin, dbNet*& db_net, dbModNet*& db_modnet) const
{
  dbITerm* iterm = nullptr;
  dbBTerm* bterm = nullptr;
  dbModITerm* moditerm = nullptr;
  dbModBTerm* modbterm = nullptr;
  db_net = nullptr;
  db_modnet = nullptr;

  staToDb(pin, iterm, bterm, moditerm, modbterm);
  if (iterm) {
    // in this case we may have both a hierarchical net
    // and a physical net
    db_net = iterm->getNet();
    db_modnet = iterm->getModNet();
  }
  if (bterm) {
    // in this case we may have both a hierarchical net
    // and a physical net
    db_net = bterm->getNet();
    db_modnet = bterm->getModNet();
  }
  // pins which act as bterms are top levels and have no net
  // so we skip that case (defaults to null)

  if (moditerm) {
    db_modnet = moditerm->getModNet();
  }
  if (modbterm) {
    db_modnet = modbterm->getModNet();
  }
}

Term* dbNetwork::term(const Pin* pin) const
{
  dbITerm* iterm = nullptr;
  dbBTerm* bterm = nullptr;
  dbModITerm* moditerm = nullptr;
  dbModBTerm* modbterm = nullptr;
  staToDb(pin, iterm, bterm, moditerm, modbterm);
  if (iterm) {
    return nullptr;
  }
  if (bterm) {
    return dbToStaTerm(bterm);
  }
  if (moditerm) {
    dbModBTerm* mod_port = moditerm->getChildModBTerm();
    if (mod_port) {
      Term* ret = dbToStaTerm(mod_port);
      return ret;
    }
  }
  if (modbterm) {
    return dbToStaTerm(modbterm);
  }
  return nullptr;
}

Port* dbNetwork::port(const Pin* pin) const
{
  dbITerm* iterm;
  dbBTerm* bterm;
  dbModITerm* moditerm;
  dbModBTerm* modbterm;
  Port* ret = nullptr;
  // Will return the bterm for a top level pin
  staToDb(pin, iterm, bterm, moditerm, modbterm);

  if (iterm) {
    dbMTerm* mterm = iterm->getMTerm();
    ret = dbToSta(mterm);
  } else if (bterm) {
    const char* port_name = bterm->getConstName();
    ret = findPort(top_cell_, port_name);
  } else if (moditerm) {
    std::string port_name_str = moditerm->getName();
    const char* port_name = port_name_str.c_str();
    dbModInst* mod_inst = moditerm->getParent();
    dbModule* module = mod_inst->getMaster();
    dbModBTerm* mod_port = module->findModBTerm(port_name);
    if (mod_port) {
      ret = dbToSta(mod_port);
      return ret;
    }
  } else if (modbterm) {
    ret = dbToSta(modbterm);
  }
  assert(ret != nullptr);
  return ret;
}

PortDirection* dbNetwork::direction(const Port* port) const
{
  dbMTerm* mterm = nullptr;
  dbModBTerm* modbterm = nullptr;
  dbBTerm* bterm = nullptr;
  staToDb(port, bterm, mterm, modbterm);
  if (bterm) {
    PortDirection* dir = dbToSta(bterm->getSigType(), bterm->getIoType());
    return dir;
  }
  if (modbterm) {
    PortDirection* dir = dbToSta(modbterm->getSigType(), modbterm->getIoType());
    return dir;
  }
  const ConcretePort* cport = reinterpret_cast<const ConcretePort*>(port);
  return cport->direction();
}

PortDirection* dbNetwork::direction(const Pin* pin) const
{
  // ODB does not undestand tristates so look to liberty before ODB for port
  // direction.
  LibertyPort* lib_port = libertyPort(pin);
  if (lib_port) {
    return lib_port->direction();
  }

  dbITerm* iterm;
  dbBTerm* bterm;
  dbModBTerm* modbterm;
  dbModITerm* moditerm;

  staToDb(pin, iterm, bterm, moditerm, modbterm);
  if (iterm) {
    PortDirection* dir = dbToSta(iterm->getSigType(), iterm->getIoType());
    return dir;
  }
  if (bterm) {
    PortDirection* dir = dbToSta(bterm->getSigType(), bterm->getIoType());
    return dir;
  }
  if (modbterm) {
    PortDirection* dir = dbToSta(modbterm->getSigType(), modbterm->getIoType());
    return dir;
  }
  if (moditerm) {
    // get the direction off the modbterm
    std::string pin_name = moditerm->getName();
    dbModInst* mod_inst = moditerm->getParent();
    dbModule* module = mod_inst->getMaster();
    dbModBTerm* modbterm_local = module->findModBTerm(pin_name.c_str());
    PortDirection* dir
        = dbToSta(modbterm_local->getSigType(), modbterm_local->getIoType());
    return dir;
  }

  //
  // note the nasty default behaviour here.
  // if not a liberty port then return unknown
  // presumably unlinked lefs fall through here
  // This is probably a bug in the original code.
  //
  return PortDirection::unknown();
}

VertexId dbNetwork::vertexId(const Pin* pin) const
{
  dbITerm* iterm = nullptr;
  dbBTerm* bterm = nullptr;
  dbModITerm* miterm = nullptr;
  dbModBTerm* mbterm = nullptr;
  staToDb(pin, iterm, bterm, miterm, mbterm);
  if (iterm) {
    return iterm->staVertexId();
  }
  if (bterm) {
    return bterm->staVertexId();
  }
  return object_id_null;
}

void dbNetwork::setVertexId(Pin* pin, VertexId id)
{
  dbITerm* iterm = nullptr;
  dbBTerm* bterm = nullptr;
  dbModITerm* moditerm = nullptr;
  dbModBTerm* modbterm = nullptr;
  staToDb(pin, iterm, bterm, moditerm, modbterm);
  // timing arcs only set on leaf level iterm/bterm.
  if (iterm) {
    iterm->staSetVertexId(id);
  } else if (bterm) {
    bterm->staSetVertexId(id);
  }
}

void dbNetwork::location(const Pin* pin,
                         // Return values.
                         double& x,
                         double& y,
                         bool& exists) const
{
  if (isPlaced(pin)) {
    Point pt = location(pin);
    x = dbuToMeters(pt.getX());
    y = dbuToMeters(pt.getY());
    exists = true;
  } else {
    x = 0;
    y = 0;
    exists = false;
  }
}

Point dbNetwork::location(const Pin* pin) const
{
  dbITerm* iterm = nullptr;
  dbBTerm* bterm = nullptr;
  dbModITerm* moditerm = nullptr;
  dbModBTerm* modbterm = nullptr;

  staToDb(pin, iterm, bterm, moditerm, modbterm);
  if (iterm) {
    int x, y;
    if (iterm->getAvgXY(&x, &y)) {
      return Point(x, y);
    }
    return iterm->getInst()->getOrigin();
  }
  if (bterm) {
    int x, y;
    if (bterm->getFirstPinLocation(x, y)) {
      return Point(x, y);
    }
  }
  return Point(0, 0);
}

bool dbNetwork::isPlaced(const Pin* pin) const
{
  dbITerm* iterm = nullptr;
  dbBTerm* bterm = nullptr;
  dbModITerm* moditerm = nullptr;
  dbModBTerm* modbterm = nullptr;

  staToDb(pin, iterm, bterm, moditerm, modbterm);
  dbPlacementStatus status = dbPlacementStatus::UNPLACED;
  if (iterm) {
    dbInst* inst = iterm->getInst();
    status = inst->getPlacementStatus();
  }
  if (bterm) {
    status = bterm->getFirstPinPlacementStatus();
  }
  return status.isPlaced();
}

////////////////////////////////////////////////////////////////

ObjectId dbNetwork::id(const Net* net) const
{
  dbModNet* modnet = nullptr;
  dbNet* dnet = nullptr;
  staToDb(net, dnet, modnet);
  if (hierarchy_) {
    dbObject* obj = reinterpret_cast<dbObject*>(const_cast<Net*>(net));
    dbObjectType type = obj->getObjectType();
    return getDbNwkObjectId(type, obj->getId());
  }
  return dnet->getId();
}

/*
Custom dbNetwork code.

All dbNets are created in the top Instance
by default in flat flow. So to figure out their path
name we need to search to see the pins they are connected
to.

If we have a modnet, we construct its hierarchical name
*/

const char* dbNetwork::pathName(const Net* net) const
{
  // note that in flat mode, because a net is always
  // created in the top instance the path name is just
  // its full name, ditto hierarchical mode.
  // For a modnet in hierarchy mode things are a bit more interesting.

  dbModNet* modnet = nullptr;
  dbNet* dnet = nullptr;

  staToDb(net, dnet, modnet);

  if (dnet && modnet == nullptr) {
    return dnet->getConstName();
  }

  if (modnet) {
    std::string modnet_name = modnet->getName();
    // if a top net, don't prefix with top module name
    dbModule* parent_module = modnet->getParent();
    if (parent_module == block_->getTopModule()) {
      return tmpStringCopy(modnet_name.c_str());
    }
    // accumulate a hierachical name, includes top level name
    std::string accumulated_path_name;
    std::vector<dbModule*> parent_hierarchy;
    getParentHierarchy(parent_module, parent_hierarchy);
    std::reverse(parent_hierarchy.begin(), parent_hierarchy.end());
    for (auto db_mod : parent_hierarchy) {
      std::string module_name = db_mod->getName();
      accumulated_path_name.append(module_name);
      accumulated_path_name.append("/");
    }
    accumulated_path_name.append(modnet_name);
    return tmpStringCopy(accumulated_path_name.c_str());
  }
  return nullptr;
}

/*
  dbNets which are connected to pins which have mod nets are
  "boundaries".
 */

class PinModNetConnection : public PinVisitor
{
 public:
  PinModNetConnection(const dbNetwork* nwk);
  void operator()(const Pin* pin) override;
  dbModNet* modnet_;

 protected:
  const dbNetwork* db_network_;
};

PinModNetConnection::PinModNetConnection(const dbNetwork* nwk)
    : db_network_(nwk)
{
  modnet_ = nullptr;
}

void PinModNetConnection::operator()(const Pin* pin)
{
  dbITerm* iterm;
  dbBTerm* bterm;
  dbModBTerm* modbterm;
  dbModITerm* moditerm;

  db_network_->staToDb(pin, iterm, bterm, moditerm, modbterm);

  if (iterm && iterm->getModNet()) {
    modnet_ = iterm->getModNet();
  } else if (bterm && bterm->getModNet()) {
    modnet_ = bterm->getModNet();
  } else if (moditerm && moditerm->getModNet()) {
    modnet_ = moditerm->getModNet();
  } else if (modbterm && modbterm->getModNet()) {
    modnet_ = modbterm->getModNet();
  }
}

/*
Find if there is a modnet equivalent to a dbNet.
*/

dbModNet* dbNetwork::findRelatedModNet(const dbNet* net) const
{
  PinModNetConnection visitor(this);
  NetSet visited_nets;
  visitConnectedPins(dbToSta(net), visitor, visited_nets);
  return visitor.modnet_;
}

const char* dbNetwork::name(const Net* net) const
{
  dbModNet* modnet = nullptr;
  dbNet* dnet = nullptr;
  staToDb(net, dnet, modnet);
  std::string name;

  Network* sta_nwk = (Network*) this;

  if (dnet && !modnet) {
    name = dnet->getName();
    // strip out the parent name in hierarchy mode
    // turn this off to get full flat names

    if (hierarchy_) {
      //
      // If this is not a hierarchical name, return it
      //
      if (name.find_last_of('/') == std::string::npos) {
        return tmpStringCopy(name.c_str());
      }
      //
      // Get the net name within this module of the hierarchy
      // Note we know we are dealing with an instance pin
      // of the form parent/instance/Z
      // Strip out the parent/instance part from the net name.
      // Because this object is not hooked to a modnet
      // then we know it is inside the core of the module..
      //
      dbITerm* connected_iterm = dnet->getFirstOutput();
      if (connected_iterm) {
        Pin* related_pin = dbToSta(connected_iterm);
        std::string related_pin_name_string = sta_nwk->pathName(related_pin);
        const size_t last_idx = related_pin_name_string.find_last_of('/');
        if (last_idx != std::string::npos) {
          related_pin_name_string = related_pin_name_string.substr(0, last_idx);
          const size_t second_last_idx
              = related_pin_name_string.find_last_of('/');
          if (second_last_idx != std::string::npos) {
            std::string header_to_remove
                = related_pin_name_string.substr(0, second_last_idx);
            size_t pos = name.find(header_to_remove);
            if (pos != std::string::npos) {
              name.erase(pos, header_to_remove.length() + 1);
            }
          }
        }
      }
    }
  }
  // Note the fall through: if we have a dnet which has a
  // little modnet friend, we use the modnet name.
  if (modnet) {
    name = modnet->getName();
  }
  if (dnet || modnet) {
    return tmpStringCopy(name.c_str());
  }
  return nullptr;
}

Instance* dbNetwork::instance(const Net*) const
{
  // modnets are in dbModInstance.
  // for dbNet apply an algorithm
  return top_instance_;
}

bool dbNetwork::isPower(const Net* net) const
{
  dbNet* dnet = staToDb(net);
  return (dnet->getSigType() == dbSigType::POWER);
}

bool dbNetwork::isGround(const Net* net) const
{
  dbNet* dnet = staToDb(net);
  return (dnet->getSigType() == dbSigType::GROUND);
}

NetPinIterator* dbNetwork::pinIterator(const Net* net) const
{
  return new DbNetPinIterator(net, this);
}

NetTermIterator* dbNetwork::termIterator(const Net* net) const
{
  return new DbNetTermIterator(net, this);
}

// override ConcreteNetwork::visitConnectedPins

void dbNetwork::visitConnectedPins(const Net* net,
                                   PinVisitor& visitor,
                                   NetSet& visited_nets) const
{
  dbModNet* mod_net = nullptr;
  dbNet* db_net = nullptr;

  if (visited_nets.hasKey(net)) {
    return;
  }

  visited_nets.insert(net);
  staToDb(net, db_net, mod_net);

  if (mod_net) {
    for (dbITerm* iterm : mod_net->getITerms()) {
      Pin* pin = dbToSta(iterm);
      visitor(pin);
    }
    for (dbBTerm* bterm : mod_net->getBTerms()) {
      Pin* pin = dbToSta(bterm);
      visitor(pin);
    }
    for (dbModBTerm* modbterm : mod_net->getModBTerms()) {
      Pin* pin = dbToStaPin(modbterm);
      // search up
      visitor(pin);
    }
    for (dbModITerm* moditerm : mod_net->getModITerms()) {
      Pin* pin = dbToSta(moditerm);
      // search down
      visitor(pin);
    }

    // visit below nets
    for (dbModITerm* moditerm : mod_net->getModITerms()) {
      dbModInst* mod_inst = moditerm->getParent();
      // note we are dealing with a uniquified hierarchy
      // so one master per instance..
      dbModule* module = mod_inst->getMaster();
      std::string pin_name = moditerm->getName();
      dbModBTerm* mod_bterm = module->findModBTerm(pin_name.c_str());
      Pin* below_pin = dbToStaPin(mod_bterm);
      pin_name = name(below_pin);
      visitor(below_pin);
      // traverse along rest of net
      Net* below_net = this->net(below_pin);
      if (below_net) {
        visitConnectedPins(below_net, visitor, visited_nets);
      }
    }

    // visit above nets
    for (dbModBTerm* modbterm : mod_net->getModBTerms()) {
      dbModule* db_module = modbterm->getParent();
      if (db_module == nullptr) {
        continue;
      }
      dbModInst* mod_inst = db_module->getModInst();
      if (mod_inst == nullptr) {
        continue;
      }
      std::string pin_name = modbterm->getName();
      dbModITerm* mod_iterm = mod_inst->findModITerm(pin_name.c_str());
      if (mod_iterm) {
        Pin* above_pin = dbToSta(mod_iterm);
        visitor(above_pin);
        // traverse along rest of net
        Net* above_net = this->net(above_pin);
        visitConnectedPins(above_net, visitor, visited_nets);
      }
    }
  } else if (db_net) {
    for (dbITerm* iterm : db_net->getITerms()) {
      Pin* pin = dbToSta(iterm);
      visitor(pin);
    }
    for (dbBTerm* bterm : db_net->getBTerms()) {
      Pin* pin = dbToSta(bterm);
      visitor(pin);
    }
  }
}

const Net* dbNetwork::highestConnectedNet(Net* net) const
{
  return net;
}

////////////////////////////////////////////////////////////////

ObjectId dbNetwork::id(const Term* term) const
{
  if (hierarchy_) {
    dbObject* obj = reinterpret_cast<dbObject*>(const_cast<Term*>(term));
    dbObjectType type = obj->getObjectType();
    return getDbNwkObjectId(type, obj->getId());
  }
  return staToDb(term)->getId();
}

Pin* dbNetwork::pin(const Term* term) const
{
  // in original code
  // Only terms are for top level instance pins, which are also BTerms.
  // in new code:
  // Only terms are for top level instance pins, which are also BTerms.
  // return reinterpret_cast<Pin*>(const_cast<Term*>(term));
  // terms now include modbterms.
  //
  dbBTerm* bterm = nullptr;
  dbModBTerm* modbterm = nullptr;
  dbITerm* iterm = nullptr;
  dbModITerm* moditerm = nullptr;
  staToDb(term, iterm, bterm, moditerm, modbterm);
  if (bterm) {
    return dbToSta(bterm);
  }
  if (modbterm) {
    // get the moditerm
    dbModule* cur_module = modbterm->getParent();
    dbModInst* cur_mod_inst = cur_module->getModInst();
    std::string pin_name = modbterm->getName();
    dbModITerm* parent_moditerm = cur_mod_inst->findModITerm(pin_name.c_str());
    if (parent_moditerm) {
      return dbToSta(parent_moditerm);
    }
  }
  return nullptr;
}

/*
Get the flat net for a term
*/

dbNet* dbNetwork::flatNet(const Term* term) const
{
  dbITerm* iterm = nullptr;
  dbBTerm* bterm = nullptr;
  dbModITerm* moditerm = nullptr;
  dbModBTerm* modbterm = nullptr;

  staToDb(term, iterm, bterm, moditerm, modbterm);

  if (bterm) {
    dbNet* dnet = bterm->getNet();
    return dnet;
  }
  logger_->error(
      ORD, 2029, "Illegal term for getting a flat net, must be bterm");

  return nullptr;
}

Net* dbNetwork::net(const Term* term) const
{
  dbITerm* iterm = nullptr;
  dbBTerm* bterm = nullptr;
  dbModITerm* moditerm = nullptr;
  dbModBTerm* modbterm = nullptr;

  staToDb(term, iterm, bterm, moditerm, modbterm);

  if (modbterm) {
    return dbToSta(modbterm->getModNet());
  }
  if (bterm) {
    dbModNet* mod_net = bterm->getModNet();
    dbNet* dnet = bterm->getNet();

    // TODO: revert this logic so that we always
    // return the flat net. Fix verilog writer
    // to work with mod nets.

    if (mod_net) {
      return dbToSta(mod_net);
    }
    if (dnet) {
      return dbToSta(dnet);
    }
  }
  return nullptr;
}

////////////////////////////////////////////////////////////////

bool dbNetwork::isLinked() const
{
  return top_cell_ != nullptr;
}

bool dbNetwork::linkNetwork(const char*, bool make_black_boxes, Report* report)
{
  // Not called.
  return true;
}

void dbNetwork::readLefAfter(dbLib* lib)
{
  makeLibrary(lib);
}

void dbNetwork::readDefAfter(dbBlock* block)
{
  db_ = block->getDataBase();
  block_ = block;
  readDbNetlistAfter();
}

// Make ConcreteLibrary/Cell/Port objects for the
// db library/master/MTerm objects.
void dbNetwork::readDbAfter(odb::dbDatabase* db)
{
  db_ = db;
  dbChip* chip = db_->getChip();
  if (chip) {
    block_ = chip->getBlock();
    for (dbLib* lib : db_->getLibs()) {
      makeLibrary(lib);
    }
    readDbNetlistAfter();
  }

  for (auto* observer : observers_) {
    observer->postReadDb();
  }
}

void dbNetwork::makeLibrary(dbLib* lib)
{
  const char* lib_name = lib->getConstName();
  Library* library = makeLibrary(lib_name, nullptr);
  for (dbMaster* master : lib->getMasters()) {
    makeCell(library, master);
  }
}

void dbNetwork::makeCell(Library* library, dbMaster* master)
{
  const char* cell_name = master->getConstName();
  Cell* cell = makeCell(library, cell_name, true, nullptr);
  master->staSetCell(reinterpret_cast<void*>(cell));
  // keep track of db leaf cells. These are cells for which we
  // use the concrete network.
  registerConcreteCell(cell);
  ConcreteCell* ccell = reinterpret_cast<ConcreteCell*>(cell);
  ccell->setExtCell(reinterpret_cast<void*>(master));

  // Use the default liberty for "linking" the db/LEF masters.
  LibertyCell* lib_cell = findLibertyCell(cell_name);
  TestCell* test_cell = nullptr;
  if (lib_cell) {
    ccell->setLibertyCell(lib_cell);
    lib_cell->setExtCell(reinterpret_cast<void*>(master));
    test_cell = lib_cell->testCell();
  }

  for (dbMTerm* mterm : master->getMTerms()) {
    const char* port_name = mterm->getConstName();
    Port* port = makePort(cell, port_name);
    PortDirection* dir = dbToSta(mterm->getSigType(), mterm->getIoType());
    setDirection(port, dir);
    mterm->staSetPort(reinterpret_cast<void*>(port));
    ConcretePort* cport = reinterpret_cast<ConcretePort*>(port);
    cport->setExtPort(reinterpret_cast<void*>(mterm));
    registerConcretePort(port);
    if (lib_cell) {
      LibertyPort* lib_port = lib_cell->findLibertyPort(port_name);
      if (lib_port) {
        cport->setLibertyPort(lib_port);
        lib_port->setExtPort(mterm);
      } else if (!dir->isPowerGround() && !lib_cell->findPgPort(port_name)) {
        logger_->warn(ORD,
                      2001,
                      "LEF macro {} pin {} missing from liberty cell.",
                      cell_name,
                      port_name);
      }
      if (test_cell) {
        LibertyPort* test_port = test_cell->findLibertyPort(port_name);
        if (test_port) {
          test_port->setExtPort(mterm);
        }
      }
    }
  }
  // Assume msb first busses because LEF has no clue about busses.
  // This generates the top level ports
  groupBusPorts(cell, [](const char*) { return true; });

  // Fill in liberty to db/LEF master correspondence for libraries not used
  // for corners that are not used for "linking".
  std::unique_ptr<LibertyLibraryIterator> lib_iter{libertyLibraryIterator()};
  while (lib_iter->hasNext()) {
    LibertyLibrary* lib = lib_iter->next();
    LibertyCell* lib_cell = lib->findLibertyCell(cell_name);
    if (lib_cell) {
      lib_cell->setExtCell(reinterpret_cast<void*>(master));
      TestCell* test_cell = lib_cell->testCell();

      for (dbMTerm* mterm : master->getMTerms()) {
        const char* port_name = mterm->getConstName();
        LibertyPort* lib_port = lib_cell->findLibertyPort(port_name);
        if (lib_port) {
          lib_port->setExtPort(mterm);
        }
        if (test_cell) {
          LibertyPort* test_port = test_cell->findLibertyPort(port_name);
          if (test_port) {
            test_port->setExtPort(mterm);
          }
        }
      }
    }
  }

  std::unique_ptr<CellPortIterator> port_iter{portIterator(cell)};
  while (port_iter->hasNext()) {
    Port* cur_port = port_iter->next();
    registerConcretePort(cur_port);
  }
}

void dbNetwork::readDbNetlistAfter()
{
  makeTopCell();
  findConstantNets();
  checkLibertyCorners();
}

void dbNetwork::makeTopCell()
{
  if (top_cell_) {
    // Reading DEF or linking when a network already exists; remove previous top
    // cell.
    Library* top_lib = library(top_cell_);
    deleteLibrary(top_lib);
  }
  const char* design_name = block_->getConstName();
  Library* top_lib = makeLibrary(design_name, nullptr);
  top_cell_ = makeCell(top_lib, design_name, false, nullptr);
  // bterms in top cell include bus components
  for (dbBTerm* bterm : block_->getBTerms()) {
    makeTopPort(bterm);
  }
  groupBusPorts(top_cell_, [=](const char* port_name) {
    return portMsbFirst(port_name, design_name);
  });

  // record the top level ports
  std::unique_ptr<CellPortIterator> port_iter{portIterator(top_cell_)};
  while (port_iter->hasNext()) {
    Port* cur_port = port_iter->next();
    registerConcretePort(cur_port);
  }
}

Port* dbNetwork::makeTopPort(dbBTerm* bterm)
{
  const char* port_name = bterm->getConstName();
  Port* port = makePort(top_cell_, port_name);
  PortDirection* dir = dbToSta(bterm->getSigType(), bterm->getIoType());
  setDirection(port, dir);
  registerConcretePort(port);
  return port;
}

void dbNetwork::setTopPortDirection(dbBTerm* bterm, const dbIoType& io_type)
{
  Port* port = findPort(top_cell_, bterm->getConstName());
  PortDirection* dir = dbToSta(bterm->getSigType(), bterm->getIoType());
  setDirection(port, dir);
}

// read_verilog / Verilog2db::makeDbPins leaves a cookie to know if a bus port
// is msb first or lsb first.
bool dbNetwork::portMsbFirst(const char* port_name, const char* cell_name)
{
  std::string key = "bus_msb_first ";
  //  key += port_name;
  key = key + port_name + " " + cell_name;
  dbBoolProperty* property = odb::dbBoolProperty::find(block_, key.c_str());
  if (property) {
    return property->getValue();
  }
  // Default when DEF did not come from read_verilog.
  return true;
}

void dbNetwork::findConstantNets()
{
  clearConstantNets();
  for (dbNet* dnet : block_->getNets()) {
    if (dnet->getSigType() == dbSigType::GROUND) {
      addConstantNet(dbToSta(dnet), LogicValue::zero);
    } else if (dnet->getSigType() == dbSigType::POWER) {
      addConstantNet(dbToSta(dnet), LogicValue::one);
    }
  }
}

// Setup mapping from Cell/Port to LibertyCell/LibertyPort.
void dbNetwork::readLibertyAfter(LibertyLibrary* lib)
{
  for (ConcreteLibrary* clib : library_seq_) {
    if (!clib->isLiberty()) {
      std::unique_ptr<ConcreteLibraryCellIterator> cell_iter{
          clib->cellIterator()};
      while (cell_iter->hasNext()) {
        ConcreteCell* ccell = cell_iter->next();
        // Don't clobber an existing liberty cell so link points to the first.
        if (ccell->libertyCell() == nullptr) {
          LibertyCell* lcell = lib->findLibertyCell(ccell->name());
          if (lcell) {
            TestCell* test_cell = lcell->testCell();
            lcell->setExtCell(ccell->extCell());
            ccell->setLibertyCell(lcell);
            std::unique_ptr<ConcreteCellPortBitIterator> port_iter{
                ccell->portBitIterator()};
            while (port_iter->hasNext()) {
              ConcretePort* cport = port_iter->next();
              const char* port_name = cport->name();
              Port* cur_port = reinterpret_cast<Port*>(cport);
              registerConcretePort(cur_port);
              LibertyPort* lport = lcell->findLibertyPort(port_name);
              if (lport) {
                cport->setLibertyPort(lport);
                lport->setExtPort(cport->extPort());
              } else if (!cport->direction()->isPowerGround()
                         && !lcell->findPgPort(port_name)) {
                logger_->warn(ORD,
                              2002,
                              "Liberty cell {} pin {} missing from LEF macro.",
                              lcell->name(),
                              port_name);
              }

              if (test_cell) {
                LibertyPort* test_port = test_cell->findLibertyPort(port_name);
                if (test_port) {
                  test_port->setExtPort(cport->extPort());
                }
              }
            }
          }
        }
      }
    }
  }

  for (auto* observer : observers_) {
    observer->postReadLiberty();
  }
}

////////////////////////////////////////////////////////////////

// Edit functions

Instance* dbNetwork::makeInstance(LibertyCell* cell,
                                  const char* name,
                                  Instance* parent)
{
  const char* cell_name = cell->name();
  if (parent == top_instance_) {
    dbMaster* master = db_->findMaster(cell_name);
    if (master) {
      dbInst* inst = dbInst::create(block_, master, name);
      //
      // Register all liberty cells as being concrete
      // Sometimes this method is called by the sta
      // to build "test circuits" eg to find the max wire length
      // And those cells need to use the external api
      // to get timing characteristics, so they have to be
      // concrete
      Cell* inst_cell = dbToSta(master);
      registerConcreteCell(inst_cell);
      std::unique_ptr<sta::CellPortIterator> port_iter{portIterator(inst_cell)};
      while (port_iter->hasNext()) {
        Port* cur_port = port_iter->next();
        registerConcretePort(cur_port);
      }
      return dbToSta(inst);
    }
  } else {
    dbInst* db_inst = nullptr;
    dbModInst* mod_inst = nullptr;
    staToDb(parent, db_inst, mod_inst);
    if (mod_inst) {
      dbMaster* master = db_->findMaster(cell_name);
      dbModule* parent = mod_inst->getMaster();
      dbInst* inst = dbInst::create(block_, master, name, false, parent);
      Cell* inst_cell = dbToSta(master);
      //
      // Register all liberty cells as being concrete
      // Sometimes this method is called by the sta
      // to build "test circuits" eg to find the max wire length
      // And those cells need to use the external api
      // to get timing characteristics, so they have to be
      // concrete
      registerConcreteCell(inst_cell);
      std::unique_ptr<sta::CellPortIterator> port_iter{portIterator(inst_cell)};
      while (port_iter->hasNext()) {
        Port* cur_port = port_iter->next();
        registerConcretePort(cur_port);
      }
      return dbToSta(inst);
    }
  }
  return nullptr;
}

void dbNetwork::makePins(Instance*)
{
  // This space intentionally left blank.
}

void dbNetwork::replaceCell(Instance* inst, Cell* cell)
{
  dbMaster* master = staToDb(cell);
  dbInst* db_inst;
  dbModInst* mod_inst;
  staToDb(inst, db_inst, mod_inst);
  if (db_inst) {
    db_inst->swapMaster(master);
  }
}

void dbNetwork::deleteInstance(Instance* inst)
{
  dbInst* db_inst;
  dbModInst* mod_inst;
  staToDb(inst, db_inst, mod_inst);
  if (db_inst) {
    dbInst::destroy(db_inst);
  } else {
    dbModInst::destroy(mod_inst);
  }
}

/*
Generic pin -> net connection with support for both hierarchical
nets and regular flat nets.

As a side effect this routine will disconnect any prior
flat/hierarchical connections and then make the new connections

It also checks the legallity of the pin/net combination.


*/

void dbNetwork::connectPin(Pin* pin, Net* flat_net, Net* hier_net)
{
  // get the type of the pin
  odb::dbITerm* iterm = nullptr;
  odb::dbBTerm* bterm = nullptr;
  odb::dbModITerm* moditerm = nullptr;
  odb::dbModBTerm* modbterm = nullptr;
  staToDb(pin, iterm, bterm, moditerm, modbterm);

  // get the type of the net
  dbNet* flat_net_db = nullptr;
  dbModNet* hier_net_db = nullptr;

  // connect the flat net first
  if (flat_net) {
    staToDb(flat_net, flat_net_db, hier_net_db);
    if (hier_net_db) {
      logger_->error(ORD,
                     2025,
                     "Illegal net combination. hierarchical flat net supplied "
                     "as flat net argument to api:connectPin");
    }
    if (flat_net_db) {
      if (iterm) {
        iterm->connect(flat_net_db);
      } else if (bterm) {
        bterm->connect(flat_net_db);
      } else {
        logger_->error(ORD,
                       2026,
                       "Illegal net/pin combination. flat nets can only hook "
                       "to dbIterm, dbBTerm");
      }
    }
  }

  if (hier_net) {
    // sanity check to make hier argument is ok
    dbNet* flat_net_local_check = nullptr;
    staToDb(hier_net, flat_net_local_check, hier_net_db);
    if (flat_net_local_check) {
      logger_->error(ORD,
                     2027,
                     "Illegal net combination. flat net supplied as hier net "
                     "argument to api:connectPin");
    }

    if (hier_net_db) {
      if (iterm) {
        iterm->connect(hier_net_db);
      } else if (bterm) {
        bterm->connect(hier_net_db);
      } else if (moditerm) {
        moditerm->connect(hier_net_db);
      } else if (modbterm) {
        modbterm->connect(hier_net_db);
      } else {
        logger_->error(ORD,
                       2028,
                       "Illegal net combination. hier net expected to be "
                       "hooked to one of iterm, bterm, moditerm, modbterm");
      }
      // do the house keeping. Mod net must always have the flat net associated
      // with it.
      if (flat_net_db) {
        reassociateHierFlatNet(hier_net_db, flat_net_db, nullptr);
      }
    }
  }
}

/*
Generic pin -> net connection with support for hierarchical
nets. If connecting a hierarchical net and associated_flat_net
not null then reassociate the hierarhical net.
*/
void dbNetwork::connectPin(Pin* pin, Net* net)
{
  // get the type of the pin
  odb::dbITerm* iterm = nullptr;
  odb::dbBTerm* bterm = nullptr;
  odb::dbModITerm* moditerm = nullptr;
  odb::dbModBTerm* modbterm = nullptr;
  staToDb(pin, iterm, bterm, moditerm, modbterm);

  // get the type of the net
  dbNet* dnet = nullptr;
  dbModNet* mod_net = nullptr;
  staToDb(net, dnet, mod_net);

  if (iterm && dnet) {
    iterm->connect(dnet);
  } else if (bterm && dnet) {
    bterm->connect(dnet);
  } else if (iterm && mod_net) {
    iterm->connect(mod_net);
  } else if (bterm && mod_net) {
    bterm->connect(mod_net);
  } else if (moditerm && mod_net) {
    moditerm->connect(mod_net);
  } else if (modbterm && mod_net) {
    modbterm->connect(mod_net);
  } else {
    logger_->error(
        ORD,
        2024,
        "Illegal net/pin combination. Modnets can only be hooked to iterm, "
        "bterm, moditerm, modbterm, dbNets can only be hooked to iterm, bterm");
  }
}

Pin* dbNetwork::connect(Instance* inst, Port* port, Net* net)
{
  Pin* pin = nullptr;
  dbNet* dnet = staToDb(net);
  if (inst == top_instance_) {
    const char* port_name = name(port);
    dbBTerm* bterm = block_->findBTerm(port_name);
    if (bterm) {
      bterm->connect(dnet);
    } else {
      bterm = dbBTerm::create(dnet, port_name);
      PortDirection* dir = direction(port);
      dbSigType sig_type;
      dbIoType io_type;
      staToDb(dir, sig_type, io_type);
      bterm->setSigType(sig_type);
      bterm->setIoType(io_type);
    }
    pin = dbToSta(bterm);
  } else {
    dbInst* db_inst;
    dbModInst* mod_inst;
    staToDb(inst, db_inst, mod_inst);
    if (db_inst) {
      dbMTerm* dterm = staToDb(port);
      dbITerm* iterm = db_inst->getITerm(dterm);
      iterm->connect(dnet);
      pin = dbToSta(iterm);
    }
  }
  return pin;
}

// Used by dbStaCbk
// Incrementally update drivers.
void dbNetwork::connectPinAfter(Pin* pin)
{
  if (isDriver(pin)) {
    Net* net = this->net(pin);
    PinSet* drvrs = net_drvr_pin_map_.findKey(net);
    if (drvrs) {
      drvrs->insert(pin);
    }
  }
}

Pin* dbNetwork::connect(Instance* inst, LibertyPort* port, Net* net)
{
  dbNet* dnet = staToDb(net);
  const char* port_name = port->name();
  Pin* pin = nullptr;
  if (inst == top_instance_) {
    dbBTerm* bterm = block_->findBTerm(port_name);
    if (bterm) {
      bterm->connect(dnet);
    } else {
      bterm = dbBTerm::create(dnet, port_name);
    }
    PortDirection* dir = port->direction();
    dbSigType sig_type;
    dbIoType io_type;
    staToDb(dir, sig_type, io_type);
    bterm->setSigType(sig_type);
    bterm->setIoType(io_type);
    pin = dbToSta(bterm);
  } else {
    dbInst* db_inst;
    dbModInst* mod_inst;
    staToDb(inst, db_inst, mod_inst);
    if (db_inst) {
      dbMaster* master = db_inst->getMaster();
      dbMTerm* dterm = master->findMTerm(port_name);
      dbITerm* iterm = db_inst->getITerm(dterm);
      iterm->connect(dnet);
      pin = dbToSta(iterm);
    }
  }
  return pin;
}

//
// remove a net connection to a pin.
// Figure out type of net (modnet or flat net)
// and just delete that type.
//
void dbNetwork::disconnectPin(Pin* pin, Net* net)
{
  dbModNet* mod_net = nullptr;
  dbNet* db_net = nullptr;
  staToDb(net, db_net, mod_net);

  dbITerm* iterm = nullptr;
  dbBTerm* bterm = nullptr;
  dbModITerm* moditerm = nullptr;
  dbModBTerm* modbterm = nullptr;
  staToDb(pin, iterm, bterm, moditerm, modbterm);
  if (iterm) {
    if (db_net) {
      iterm->disconnectDbNet();
    }
    if (mod_net) {
      iterm->disconnectModNet();
    }
  } else if (bterm) {
    if (db_net) {
      bterm->disconnectDbNet();
    }
    if (mod_net) {
      bterm->disconnectDbModNet();
    }
  } else if (moditerm) {
    moditerm->disconnect();
  } else if (modbterm) {
    modbterm->disconnect();
  }
}

//
// remove all connnections to a pin
//(both hierarchical and flat).
//
void dbNetwork::disconnectPin(Pin* pin)
{
  dbITerm* iterm = nullptr;
  dbBTerm* bterm = nullptr;
  dbModITerm* moditerm = nullptr;
  dbModBTerm* modbterm = nullptr;
  staToDb(pin, iterm, bterm, moditerm, modbterm);
  if (iterm) {
    iterm->disconnect();
  } else if (bterm) {
    bterm->disconnect();
  } else if (moditerm) {
    moditerm->disconnect();
  } else if (modbterm) {
    modbterm->disconnect();
  }
}

void dbNetwork::disconnectPinBefore(const Pin* pin)
{
  Net* net = this->net(pin);
  // Incrementally update drivers.
  if (net && isDriver(pin)) {
    PinSet* drvrs = net_drvr_pin_map_.findKey(net);
    if (drvrs) {
      drvrs->erase(pin);
    }
  }
}

void dbNetwork::deletePin(Pin* pin)
{
  dbITerm* iterm;
  dbBTerm* bterm;
  dbModITerm* moditerm;
  dbModBTerm* modbterm;

  staToDb(pin, iterm, bterm, moditerm, modbterm);
  if (iterm) {
    logger_->critical(ORD, 2003, "deletePin not implemented for dbITerm");
  }
  if (bterm) {
    dbBTerm::destroy(bterm);
  }
}

Port* dbNetwork::makePort(Cell* cell, const char* name)
{
  if (cell == top_cell_ && !block_->findBTerm(name)) {
    odb::dbNet* net = block_->findNet(name);
    if (!net) {
      // a bterm must have a net
      net = odb::dbNet::create(block_, name);
    }
    // Making the bterm creates the port in the db callback
    odb::dbBTerm::create(net, name);
    Port* ret = findPort(cell, name);
    registerConcretePort(ret);
    return ret;
  }
  Port* cur_port = ConcreteNetwork::makePort(cell, name);
  registerConcretePort(cur_port);
  return cur_port;
}

Pin* dbNetwork::makePin(Instance* inst, Port* port, Net* net)
{
  if (inst != top_instance_) {
    return ConcreteNetwork::makePin(inst, port, net);
  }
  return nullptr;
}

Net* dbNetwork::makeNet(const char* name, Instance* parent)
{
  dbNet* dnet = dbNet::create(block_, name, false);
  return dbToSta(dnet);
}

void dbNetwork::deleteNet(Net* net)
{
  deleteNetBefore(net);
  dbNet* dnet = staToDb(net);
  dbNet::destroy(dnet);
}

void dbNetwork::deleteNetBefore(const Net* net)
{
  PinSet* drvrs = net_drvr_pin_map_.findKey(net);
  delete drvrs;
  net_drvr_pin_map_.erase(net);
}

void dbNetwork::mergeInto(Net*, Net*)
{
  logger_->critical(ORD, 2004, "unimplemented network function mergeInto");
}

Net* dbNetwork::mergedInto(Net*)
{
  logger_->critical(ORD, 2005, "unimplemented network function mergeInto");
  return nullptr;
}

bool dbNetwork::isSpecial(Net* net)
{
  dbNet* db_net = staToDb(net);
  return db_net->isSpecial();
}

////////////////////////////////////////////////////////////////

dbInst* dbNetwork::staToDb(const Instance* instance) const
{
  dbInst* db_inst;
  dbModInst* mod_inst;
  staToDb(instance, db_inst, mod_inst);
  return db_inst;
}

void dbNetwork::staToDb(const Instance* instance,
                        // Return values.
                        dbInst*& db_inst,
                        dbModInst*& mod_inst) const
{
  if (instance && instance != top_instance_) {
    dbObject* obj
        = reinterpret_cast<dbObject*>(const_cast<Instance*>(instance));
    dbObjectType type = obj->getObjectType();
    if (type == dbInstObj) {
      db_inst = static_cast<dbInst*>(obj);
      mod_inst = nullptr;
    } else if (type == dbModInstObj) {
      db_inst = nullptr;
      mod_inst = static_cast<dbModInst*>(obj);
    } else {
      logger_->critical(ORD, 2016, "instance is not Inst or ModInst");
    }
  } else {
    db_inst = nullptr;
    mod_inst = nullptr;
  }
}

dbNet* dbNetwork::staToDb(const Net* net) const
{
  return reinterpret_cast<dbNet*>(const_cast<Net*>(net));
}

dbNet* dbNetwork::flatNet(const Net* net) const
{
  if (net) {
    dbObject* obj = reinterpret_cast<dbObject*>(const_cast<Net*>(net));
    dbObjectType type = obj->getObjectType();
    if (type == odb::dbNetObj) {
      return static_cast<dbNet*>(obj);
    }
  }
  return nullptr;
}

void dbNetwork::staToDb(const Net* net, dbNet*& dnet, dbModNet*& modnet) const
{
  dnet = nullptr;
  modnet = nullptr;
  if (net) {
    dbObject* obj = reinterpret_cast<dbObject*>(const_cast<Net*>(net));
    dbObjectType type = obj->getObjectType();
    if (type == odb::dbNetObj) {
      dnet = static_cast<dbNet*>(obj);
    } else if (type == odb::dbModNetObj) {
      modnet = static_cast<dbModNet*>(obj);
    }
  }
}

dbITerm* dbNetwork::flatPin(const Pin* pin) const
{
  odb::dbITerm* iterm;
  odb::dbBTerm* bterm;
  odb::dbModITerm* moditerm;
  odb::dbModBTerm* modbterm;
  staToDb(pin, iterm, bterm, moditerm, modbterm);
  (void) bterm;
  (void) moditerm;
  (void) modbterm;
  return iterm;
}

bool dbNetwork::isFlat(const Pin* pin) const
{
  odb::dbITerm* iterm;
  odb::dbBTerm* bterm;
  odb::dbModITerm* moditerm;
  odb::dbModBTerm* modbterm;
  staToDb(pin, iterm, bterm, moditerm, modbterm);
  if (iterm || bterm) {
    return true;
  }
  return false;
}

bool dbNetwork::isFlat(const Net* net) const
{
  odb::dbNet* db_net;
  odb::dbModNet* mod_net;
  staToDb(net, db_net, mod_net);
  if (db_net) {
    return true;
  }
  return false;
}

dbModITerm* dbNetwork::hierPin(const Pin* pin) const
{
  odb::dbITerm* iterm;
  odb::dbBTerm* bterm;
  odb::dbModITerm* moditerm;
  odb::dbModBTerm* modbterm;
  staToDb(pin, iterm, bterm, moditerm, modbterm);
  (void) iterm;
  (void) bterm;
  (void) modbterm;
  return moditerm;
}

void dbNetwork::staToDb(const Pin* pin,
                        // Return values.
                        dbITerm*& iterm,
                        dbBTerm*& bterm,
                        dbModITerm*& moditerm,
                        dbModBTerm*& modbterm) const
{
  iterm = nullptr;
  bterm = nullptr;
  modbterm = nullptr;
  moditerm = nullptr;
  if (pin) {
    dbObject* obj = reinterpret_cast<dbObject*>(const_cast<Pin*>(pin));
    dbObjectType type = obj->getObjectType();
    if (type == dbITermObj) {
      iterm = static_cast<dbITerm*>(obj);
    } else if (type == dbBTermObj) {
      bterm = static_cast<dbBTerm*>(obj);
    } else if (type == dbModBTermObj) {
      modbterm = static_cast<dbModBTerm*>(obj);
    } else if (type == dbModITermObj) {
      moditerm = static_cast<dbModITerm*>(obj);
    } else {
      logger_->warn(
          ORD, 2018, "pin is not ITerm or BTerm or modITerm or ModBTerm");
    }
  }
}

dbBTerm* dbNetwork::staToDb(const Term* term) const
{
  return reinterpret_cast<dbBTerm*>(const_cast<Term*>(term));
}

void dbNetwork::staToDb(const Term* term,
                        dbITerm*& iterm,
                        dbBTerm*& bterm,
                        dbModITerm*& moditerm,
                        dbModBTerm*& modbterm) const
{
  iterm = nullptr;
  bterm = nullptr;
  moditerm = nullptr;
  modbterm = nullptr;
  if (term) {
    dbObject* obj = reinterpret_cast<dbObject*>(const_cast<Term*>(term));
    dbObjectType type = obj->getObjectType();
    if (type == dbITermObj) {
      iterm = static_cast<dbITerm*>(obj);
    } else if (type == dbBTermObj) {
      bterm = static_cast<dbBTerm*>(obj);
    } else if (type == dbModBTermObj) {
      modbterm = static_cast<dbModBTerm*>(obj);
    } else if (type == dbModITermObj) {
      moditerm = static_cast<dbModITerm*>(obj);
    }
  }
}

void dbNetwork::staToDb(const Cell* cell,
                        dbMaster*& master,
                        dbModule*& module) const
{
  module = nullptr;
  master = nullptr;
  if (isConcreteCell(cell) || cell == top_cell_) {
    const ConcreteCell* ccell = reinterpret_cast<const ConcreteCell*>(cell);
    master = reinterpret_cast<dbMaster*>(ccell->extCell());
  } else {
    if (block_) {
      module = reinterpret_cast<dbModule*>(const_cast<Cell*>(cell));
    }
  }
}

//
// Left in, these are only called by db Cells.
//
dbMaster* dbNetwork::staToDb(const Cell* cell) const
{
  const ConcreteCell* ccell = reinterpret_cast<const ConcreteCell*>(cell);
  return reinterpret_cast<dbMaster*>(ccell->extCell());
}

// called only on db cells.
dbMaster* dbNetwork::staToDb(const LibertyCell* cell) const
{
  const ConcreteCell* ccell = cell;
  return reinterpret_cast<dbMaster*>(ccell->extCell());
}

dbMTerm* dbNetwork::staToDb(const Port* port) const
{
  const ConcretePort* cport = reinterpret_cast<const ConcretePort*>(port);
  return reinterpret_cast<dbMTerm*>(cport->extPort());
}

dbBTerm* dbNetwork::isTopPort(const Port* port) const
{
  std::unique_ptr<CellPortIterator> port_iter{portIterator(top_cell_)};
  while (port_iter->hasNext()) {
    if (port == port_iter->next()) {
      const ConcretePort* cport = reinterpret_cast<const ConcretePort*>(port);
      if (cport->isBus()) {
        return block_->findBTerm(busName(port));
      }
      return block_->findBTerm(name(port));
    }
  }
  return nullptr;
}

void dbNetwork::staToDb(const Port* port,
                        dbBTerm*& bterm,
                        dbMTerm*& mterm,
                        dbModBTerm*& modbterm) const
{
  mterm = nullptr;
  modbterm = nullptr;

  if (isConcretePort(port)) {
    const ConcretePort* cport = reinterpret_cast<const ConcretePort*>(port);
    mterm = reinterpret_cast<dbMTerm*>(cport->extPort());
    return;
  }
  // just get the port directly from odb
  dbObject* obj = reinterpret_cast<dbObject*>(const_cast<Port*>(port));
  dbObjectType type = obj->getObjectType();
  if (type == dbModBTermObj) {
    Port* port_unconst = const_cast<Port*>(port);
    modbterm = reinterpret_cast<dbModBTerm*>(port_unconst);
  } else if (type == dbBTermObj) {
    Port* port_unconst = const_cast<Port*>(port);
    bterm = reinterpret_cast<dbBTerm*>(port_unconst);
  }
}

dbMTerm* dbNetwork::staToDb(const LibertyPort* port) const
{
  return reinterpret_cast<dbMTerm*>(port->extPort());
}

void dbNetwork::staToDb(PortDirection* dir,
                        // Return values.
                        dbSigType& sig_type,
                        dbIoType& io_type) const
{
  if (dir == PortDirection::input()) {
    sig_type = dbSigType::SIGNAL;
    io_type = dbIoType::INPUT;
  } else if (dir == PortDirection::output()) {
    sig_type = dbSigType::SIGNAL;
    io_type = dbIoType::OUTPUT;
  } else if (dir == PortDirection::bidirect()) {
    sig_type = dbSigType::SIGNAL;
    io_type = dbIoType::INOUT;
  } else if (dir == PortDirection::power()) {
    sig_type = dbSigType::POWER;
    io_type = dbIoType::INOUT;
  } else if (dir == PortDirection::ground()) {
    sig_type = dbSigType::GROUND;
    io_type = dbIoType::INOUT;
  } else {
    logger_->critical(ORD, 2007, "unhandled port direction");
  }
}

////////////////////////////////////////////////////////////////

Instance* dbNetwork::dbToSta(dbModInst* inst) const
{
  return reinterpret_cast<Instance*>(inst);
}

Pin* dbNetwork::dbToSta(dbModITerm* mod_iterm) const
{
  return reinterpret_cast<Pin*>(mod_iterm);
}

Pin* dbNetwork::dbToStaPin(dbModBTerm* mod_bterm) const
{
  return reinterpret_cast<Pin*>(mod_bterm);
}

Net* dbNetwork::dbToSta(dbModNet* net) const
{
  return reinterpret_cast<Net*>(net);
}

Port* dbNetwork::dbToSta(dbModBTerm* modbterm) const
{
  return reinterpret_cast<Port*>(modbterm);
}

Term* dbNetwork::dbToStaTerm(dbModITerm* moditerm) const
{
  return reinterpret_cast<Term*>(moditerm);
}

Term* dbNetwork::dbToStaTerm(dbModBTerm* modbterm) const
{
  return reinterpret_cast<Term*>(modbterm);
}

Cell* dbNetwork::dbToSta(dbModule* master) const
{
  return reinterpret_cast<Cell*>(master);
}

Instance* dbNetwork::dbToSta(dbInst* inst) const
{
  return reinterpret_cast<Instance*>(inst);
}

Net* dbNetwork::dbToSta(dbNet* net) const
{
  return reinterpret_cast<Net*>(net);
}

const Net* dbNetwork::dbToSta(const dbNet* net) const
{
  return reinterpret_cast<const Net*>(net);
}

const Net* dbNetwork::dbToSta(const dbModNet* net) const
{
  return reinterpret_cast<const Net*>(net);
}

Pin* dbNetwork::dbToSta(dbBTerm* bterm) const
{
  return reinterpret_cast<Pin*>(bterm);
}

Pin* dbNetwork::dbToSta(dbITerm* iterm) const
{
  return reinterpret_cast<Pin*>(iterm);
}

Term* dbNetwork::dbToStaTerm(dbBTerm* bterm) const
{
  return reinterpret_cast<Term*>(bterm);
}

Port* dbNetwork::dbToSta(dbMTerm* mterm) const
{
  return reinterpret_cast<Port*>(mterm->staPort());
}

Cell* dbNetwork::dbToSta(dbMaster* master) const
{
  return reinterpret_cast<Cell*>(master->staCell());
}

PortDirection* dbNetwork::dbToSta(const dbSigType& sig_type,
                                  const dbIoType& io_type) const
{
  if (sig_type == dbSigType::POWER) {
    return PortDirection::power();
  }
  if (sig_type == dbSigType::GROUND) {
    return PortDirection::ground();
  }
  if (io_type == dbIoType::INPUT) {
    return PortDirection::input();
  }
  if (io_type == dbIoType::OUTPUT) {
    return PortDirection::output();
  }
  if (io_type == dbIoType::INOUT) {
    return PortDirection::bidirect();
  }
  if (io_type == dbIoType::FEEDTHRU) {
    return PortDirection::bidirect();
  }
  logger_->critical(ORD, 2008, "unknown master term type");
  return PortDirection::bidirect();
}

////////////////////////////////////////////////////////////////

LibertyCell* dbNetwork::libertyCell(dbInst* inst)
{
  return libertyCell(dbToSta(inst));
}

LibertyPort* dbNetwork::libertyPort(const Port* port) const
{
  if (isConcretePort(port)) {
    LibertyPort* ret = ConcreteNetwork::libertyPort(port);
    return ret;
  }
  return nullptr;
}

LibertyPort* dbNetwork::libertyPort(const Pin* pin) const
{
  // Primary: needs concrete test.
  // Look up instance
  const Instance* cur_instance = instance(pin);
  dbInst* db_inst = nullptr;
  dbModInst* mod_inst = nullptr;
  staToDb(cur_instance, db_inst, mod_inst);
  if (db_inst) {
    LibertyPort* ret = ConcreteNetwork::libertyPort(pin);
    return ret;
  }
  return nullptr;
}

/*
We keep a registry of the concrete cells.
For these we know to use the concrete network interface.
The concrete cells are created outside of the odb world
-- attempting to type cast those can lead to bad pointers.
So we simply note them and then when we inspect a cell
we can decide whether or not to use the ConcreteNetwork api.
*/

void dbNetwork::registerConcreteCell(const Cell* cell)
{
  concrete_cells_.insert(cell);
}

bool dbNetwork::isConcreteCell(const Cell* cell) const
{
  if (!hierarchy_) {
    return true;
  }
  return (concrete_cells_.find(cell) != concrete_cells_.end());
}

void dbNetwork::registerConcretePort(const Port* port)
{
  concrete_ports_.insert(port);
}

bool dbNetwork::isConcretePort(const Port* port) const
{
  if (!hierarchy_) {
    return true;
  }
  if (concrete_ports_.find(port) != concrete_ports_.end()) {
    return true;
  }
  return false;
}

/*
PortBus support
*/

bool dbNetwork::isBus(const Port* port) const
{
  if (isConcretePort(port)) {
    return ConcreteNetwork::isBus(port);
  }

  dbMTerm* mterm = nullptr;
  dbBTerm* bterm = nullptr;
  dbModBTerm* modbterm = nullptr;
  staToDb(port, bterm, mterm, modbterm);
  if (modbterm && modbterm->isBusPort()) {
    return true;
  }
  return false;
}

int dbNetwork::fromIndex(const Port* port) const
{
  if (isConcretePort(port)) {
    return ConcreteNetwork::fromIndex(port);
  }
  dbMTerm* mterm = nullptr;
  dbBTerm* bterm = nullptr;
  dbModBTerm* modbterm = nullptr;
  staToDb(port, bterm, mterm, modbterm);
  if (modbterm && modbterm->isBusPort()) {
    return modbterm->getBusPort()->getFrom();
  }

  logger_->error(ORD, 2021, "Bad bus from_index defintion");
  return 0;
}

int dbNetwork::toIndex(const Port* port) const
{
  if (isConcretePort(port)) {
    return ConcreteNetwork::toIndex(port);
  }
  dbMTerm* mterm = nullptr;
  dbBTerm* bterm = nullptr;
  dbModBTerm* modbterm = nullptr;
  staToDb(port, bterm, mterm, modbterm);
  if (modbterm && modbterm->isBusPort()) {
    int start_ix = modbterm->getBusPort()->getFrom();
    if (modbterm->getBusPort()->getUpdown()) {
      return (start_ix + (modbterm->getBusPort()->getSize() - 1));
    }
    return (start_ix - (modbterm->getBusPort()->getSize() - 1));
  }
  logger_->error(ORD, 2022, "Bad bus to_index defintion");
  return 0;
}

bool dbNetwork::hasMembers(const Port* port) const
{
  if (hierarchy_) {
    dbMTerm* mterm = nullptr;
    dbBTerm* bterm = nullptr;
    dbModBTerm* modbterm = nullptr;
    staToDb(port, bterm, mterm, modbterm);
    if (modbterm && modbterm->isBusPort()) {
      return true;
    }
    return false;
  }
  const ConcretePort* cport = reinterpret_cast<const ConcretePort*>(port);
  return cport->hasMembers();
}

Port* dbNetwork::findMember(const Port* port, int index) const
{
  if (isConcretePort(port)) {
    const ConcretePort* cport = reinterpret_cast<const ConcretePort*>(port);
    return reinterpret_cast<Port*>(cport->findMember(index));
  }
  dbMTerm* mterm = nullptr;
  dbBTerm* bterm = nullptr;
  dbModBTerm* modbterm = nullptr;
  staToDb(port, bterm, mterm, modbterm);
  if (modbterm && modbterm->isBusPort()) {
    dbBusPort* busport = modbterm->getBusPort();
    modbterm = busport->getBusIndexedElement(index);
    return reinterpret_cast<Port*>(modbterm);
  }
  return nullptr;
}

class DbNetworkPortMemberIterator : public PortMemberIterator
{
 public:
  explicit DbNetworkPortMemberIterator(const Port* port, const dbNetwork* nwk);

  bool hasNext() override;
  Port* next() override;

 private:
  dbSet<dbModBTerm>::iterator members_;
  const dbNetwork* nwk_;
  int ix_ = 0;
  int size_ = 0;
};

DbNetworkPortMemberIterator::DbNetworkPortMemberIterator(const Port* port,
                                                         const dbNetwork* nwk)
{
  dbMTerm* mterm = nullptr;
  dbBTerm* bterm = nullptr;
  dbModBTerm* modbterm = nullptr;
  nwk_ = nwk;
  nwk_->staToDb(port, bterm, mterm, modbterm);
  if (modbterm && modbterm->isBusPort()) {
    dbBusPort* busport = modbterm->getBusPort();
    members_ = busport->getBusPortMembers().begin();
    size_ = busport->getSize();
    ix_ = 0;
  }
}

bool DbNetworkPortMemberIterator::hasNext()
{
  return (ix_ != size_);
}

Port* DbNetworkPortMemberIterator::next()
{
  dbModBTerm* ret = *members_;
  ix_++;
  // if we are at the end, don't access the next member
  // as it is null
  if (ix_ != size_) {
    members_++;
  }
  return reinterpret_cast<Port*>(ret);
}

PortMemberIterator* dbNetwork::memberIterator(const Port* port) const
{
  if (!hierarchy_) {
    return ConcreteNetwork::memberIterator(port);
  }
  return new DbNetworkPortMemberIterator(port, this);
}

////////////////////////////////////////////////////////////////
// Observer

void dbNetwork::addObserver(dbNetworkObserver* observer)
{
  observer->owner_ = this;
  observers_.insert(observer);
}

void dbNetwork::removeObserver(dbNetworkObserver* observer)
{
  observer->owner_ = nullptr;
  observers_.erase(observer);
}

////////

dbNetworkObserver::~dbNetworkObserver()
{
  if (owner_ != nullptr) {
    owner_->removeObserver(this);
  }
}

/*
  Hierarchical support api
 */
Instance* dbNetwork::getOwningInstanceParent(Pin* drvr_pin)
{
  if (hasHierarchy()) {
    Instance* inst = instance(drvr_pin);
    dbInst* db_inst = nullptr;
    odb::dbModInst* mod_inst = nullptr;
    staToDb(inst, db_inst, mod_inst);
    odb::dbModule* module = nullptr;
    if (db_inst) {
      module = db_inst->getModule();
    } else if (mod_inst) {
      module = mod_inst->getParent();
    }
    if (module) {
      Instance* parent = (module == (block_->getTopModule()))
                             ? topInstance()
                             : dbToSta(module->getModInst());
      return parent;
    }
  }
  return topInstance();
}

/*
Get the dbModule driving a net.

Sometimes when inserting a buffer we simply want to stop at the current
owning module boundary. Other times we want to go right the way to
the module which owns the leaf driver.


If hier is true, go right the way through the hierarchy and return
the dbModule owning the driver.

In non-hier mode, stop at the module boundaries

*/

dbModule* dbNetwork::getNetDriverParentModule(Net* net,
                                              Pin*& driver_pin,
                                              bool hier)
{
  if (hasHierarchy()) {
    dbNet* dnet;
    dbModNet* modnet;
    staToDb(net, dnet, modnet);
    if (dnet) {
      //
      // get sink driver instance and return its parent
      //
      PinSet* drivers = this->drivers(net);
      if (drivers && !drivers->empty()) {
        PinSet::Iterator drvr_iter(drivers);
        const Pin* drvr_pin = drvr_iter.next();
        driver_pin = const_cast<Pin*>(drvr_pin);
        odb::dbITerm* iterm;
        odb::dbBTerm* bterm;
        odb::dbModITerm* mod_iterm;
        odb::dbModBTerm* mod_bterm;
        staToDb(drvr_pin, iterm, bterm, mod_iterm, mod_bterm);
        (void) (mod_iterm);
        (void) (mod_bterm);
        (void) (bterm);
        if (iterm) {
          dbInst* db_inst = iterm->getInst();
          dbModule* db_inst_module = db_inst->getModule();
          if (db_inst_module) {
            return db_inst_module;
          }
        }
      }
    } else if (modnet) {
      // in hier mode, go right the way through the hierarchy
      // and return the dbModule owning the instance doing the
      // driving
      Net* sta_net = dbToSta(modnet);
      PinSet* drivers = this->drivers(sta_net);
      for (auto pin : *drivers) {
        dbITerm* iterm = nullptr;
        dbBTerm* bterm = nullptr;
        dbModITerm* moditerm = nullptr;
        dbModBTerm* modbterm = nullptr;
        staToDb(pin, iterm, bterm, moditerm, modbterm);
        driver_pin = const_cast<Pin*>(pin);
        if (hier == false) {
          return modnet->getParent();
        }
        (void) modbterm;
        if (bterm) {
          return block_->getTopModule();
        }
        if (iterm) {
          // return the dbmodule containing the driving instance
          dbInst* dinst = iterm->getInst();
          return dinst->getModule();
        }
        if (moditerm) {
          // return the dbmodule containing the driving instance
          return moditerm->getParent()->getParent();
        }
      }
    }
  }
  // default to top module
  return block_->getTopModule();
}

void dbNetwork::getParentHierarchy(
    dbModule* start_module,
    std::vector<dbModule*>& parent_hierarchy) const
{
  dbModule* top_module = block_->getTopModule();
  dbModule* cur_module = start_module;
  while (cur_module) {
    parent_hierarchy.push_back(cur_module);
    if (cur_module == top_module) {
      return;
    }
    cur_module = cur_module->getModInst()
                     ? cur_module->getModInst()->getParent()
                     : nullptr;
  }
}

dbModule* dbNetwork::findHighestCommonModule(std::vector<dbModule*>& itree1,
                                             std::vector<dbModule*>& itree2)
{
  int ix1 = itree1.size();
  int ix2 = itree2.size();
  int limit = std::min(ix1, ix2);
  dbModule* top_module = block_->getTopModule();

  // reverse traversal. (note hierarchy stored so top is end of list)
  // get to first divergence
  std::vector<dbModule*>::reverse_iterator itree1_iter = itree1.rbegin();
  std::vector<dbModule*>::reverse_iterator itree2_iter = itree2.rbegin();
  dbModule* common_module = top_module;
  if (limit > 0) {
    for (int i = 0; i != limit; i++) {
      if (*itree1_iter != *itree2_iter) {
        return common_module;
      }
      common_module = *itree1_iter;
      itree1_iter++;
      itree2_iter++;
    }
  }
  return common_module;  // default to top
}

class PinModuleConnection : public PinVisitor
{
 public:
  PinModuleConnection(const dbNetwork* nwk,
                      const Pin* drvr_pin,
                      const dbModule* target_module_);
  void operator()(const Pin* pin) override;

 protected:
  const dbNetwork* db_network_;
  const Pin* drvr_pin_;
  const dbModule* target_module_;
  dbModBTerm* dest_modbterm_;
  dbModITerm* dest_moditerm_;
  friend class dbNetwork;
};

PinModuleConnection::PinModuleConnection(const dbNetwork* nwk,
                                         const Pin* drvr_pin,
                                         const dbModule* target_module)
{
  db_network_ = nwk;
  drvr_pin_ = drvr_pin;
  target_module_ = target_module;
  dest_modbterm_ = nullptr;
  dest_moditerm_ = nullptr;
}

void PinModuleConnection::operator()(const Pin* pin)
{
  dbITerm* iterm;
  dbBTerm* bterm;
  dbModBTerm* modbterm;
  dbModITerm* moditerm;

  db_network_->staToDb(pin, iterm, bterm, moditerm, modbterm);
  (void) (iterm);
  (void) (bterm);
  (void) (modbterm);
  if (moditerm) {
    dbModBTerm* modbterm = moditerm->getChildModBTerm();
    if (modbterm->getParent() == target_module_) {
      dest_modbterm_ = modbterm;
    }
  } else if (modbterm) {
    if (modbterm->getParent() == target_module_) {
      dest_modbterm_ = modbterm;
    }
    dbModITerm* moditerm = modbterm->getParentModITerm();
    if (moditerm->getParent()->getParent() == target_module_) {
      dest_moditerm_ = moditerm;
    }
  }
}

bool dbNetwork::ConnectionToModuleExists(dbITerm* source_pin,
                                         dbModule* dest_module,
                                         dbModBTerm*& dest_modbterm,
                                         dbModITerm*& dest_moditerm)
{
  PinModuleConnection visitor(this, dbToSta(source_pin), dest_module);
  network_->visitConnectedPins(dbToSta(source_pin), visitor);
  if (visitor.dest_modbterm_ != nullptr) {
    dest_modbterm = visitor.dest_modbterm_;
    return true;
  }
  if (visitor.dest_moditerm_ != nullptr) {
    dest_moditerm = visitor.dest_moditerm_;
    return true;
  }
  return false;
}

//
// Visit all the pins connected to another pin
// will traverse hierarchy and stash all the intermediate
// pins (moditerms, iterms).
//
class PinConnections : public PinVisitor
{
 public:
  PinConnections(const dbNetwork* nwk, const Pin* drvr_pin);
  void operator()(const Pin* pin) override;
  bool connected(const Pin* pin);

 protected:
  const dbNetwork* db_network_;
  const Pin* drvr_pin_;
  std::unordered_set<const Pin*> pins_;

  friend class dbNetwork;
};

PinConnections::PinConnections(const dbNetwork* nwk, const Pin* drvr_pin)
{
  db_network_ = nwk;
  drvr_pin_ = drvr_pin;
}

void PinConnections::operator()(const Pin* pin)
{
  if (pins_.find(pin) == pins_.end()) {
    pins_.insert(pin);
  }
}

bool PinConnections::connected(const Pin* pin)
{
  if (pins_.find(pin) != pins_.end()) {
    return true;
  }
  return false;
}

bool dbNetwork::connected(Pin* source_pin, Pin* dest_pin)
{
  PinConnections visitor(this, source_pin);
  network_->visitConnectedPins(source_pin, visitor);
  return visitor.connected(dest_pin);
}

/*
Connect any two leaf instance pins anywhere in hierarchy
adding pins/nets/ports on the hierarchical objects
*/
void dbNetwork::hierarchicalConnect(dbITerm* source_pin,
                                    dbITerm* dest_pin,
                                    const char* connection_name)
{
  dbModule* source_db_module = source_pin->getInst()->getModule();
  dbModule* dest_db_module = dest_pin->getInst()->getModule();
  // it is possible that one or other of the pins is not involved
  // in hierarchy, which is ok, and the source/dest modnet will be null
  dbModNet* source_db_mod_net = source_pin->getModNet();
  dbModNet* dest_db_mod_net = dest_pin->getModNet();

  //
  // make sure there is a direct flat net connection
  // Recall the hierarchical connections are overlayed
  // onto the flat db network, so we have both worlds
  // co-existing, something we respect even when making
  // new hierarchical connections.
  //

  dbNet* source_db_net = source_pin->getNet();
  if (!source_db_net) {
    std::string connection_name_str(connection_name);
    std::string flat_name = connection_name_str + "_flat";
    source_db_net = dbNet::create(block(), flat_name.c_str(), false);
    source_pin->connect(source_db_net);
  }
  if (!connected(dbToSta(source_pin), dbToSta(dest_pin))) {
    dest_pin->connect(source_db_net);
  }

  //
  // Make the hierarchical connection.
  // in case when pins in different modules
  //
  if (source_db_module != dest_db_module) {
    //
    // Attempt to factor connection (minimize punch through)
    //
    dbModBTerm* dest_modbterm = nullptr;
    dbModITerm* dest_moditerm = nullptr;
    if (ConnectionToModuleExists(
            source_pin, dest_db_module, dest_modbterm, dest_moditerm)) {
      dbModNet* dest_mod_net = nullptr;
      if (dest_modbterm) {
        dest_mod_net = dest_modbterm->getModNet();
      } else if (dest_moditerm) {
        dest_mod_net = dest_moditerm->getModNet();
      }
      if (dest_mod_net) {
        dest_pin->connect(dest_mod_net);
        return;
      }
    }

    // No existent connection. Find highest
    // common module, traverse up adding pins/nets and make
    // connection in highest common module
    std::vector<dbModule*> source_parent_tree;
    std::vector<dbModule*> dest_parent_tree;
    getParentHierarchy(source_db_module, source_parent_tree);
    getParentHierarchy(dest_db_module, dest_parent_tree);
    dbModule* highest_common_module
        = findHighestCommonModule(source_parent_tree, dest_parent_tree);
    dbModNet* top_net = source_db_mod_net;
    dbModITerm* top_mod_dest = nullptr;

    // make source hierarchy (bottom to top).
    dbModule* cur_module = source_db_module;
    while (cur_module != highest_common_module) {
      std::string connection_name_o
          = std::string(connection_name) + std::string("_o");
      dbModBTerm* mod_bterm
          = dbModBTerm::create(cur_module, connection_name_o.c_str());
      if (!source_db_mod_net) {
        source_db_mod_net
            = dbModNet::create(source_db_module, connection_name_o.c_str());
      }
      source_pin->connect(source_db_mod_net);
      mod_bterm->connect(source_db_mod_net);
      mod_bterm->setIoType(dbIoType::OUTPUT);
      mod_bterm->setSigType(dbSigType::SIGNAL);
      dbModInst* parent_inst = cur_module->getModInst();
      cur_module = parent_inst->getParent();
      dbModITerm* mod_iterm = dbModITerm::create(
          parent_inst, connection_name_o.c_str(), mod_bterm);
      source_db_mod_net = dbModNet::create(cur_module, connection_name);
      mod_iterm->connect(source_db_mod_net);
      top_net = source_db_mod_net;
    }

    // make dest hierarchy
    cur_module = dest_db_module;
    while (cur_module != highest_common_module) {
      std::string connection_name_i
          = std::string(connection_name) + std::string("_i");
      dbModBTerm* mod_bterm
          = dbModBTerm::create(cur_module, connection_name_i.c_str());
      // We may have a destination mod net (see first part), but check to make
      // sure it is in this module. If not, create one and hook it to the
      // destination pin also hook up the modbterm to it.
      if ((dest_db_mod_net == nullptr)
          || (dest_db_mod_net->getParent() != cur_module)) {
        dest_db_mod_net
            = dbModNet::create(cur_module, connection_name_i.c_str());
      }
      dest_pin->connect(dest_db_mod_net);
      mod_bterm->connect(dest_db_mod_net);
      mod_bterm->setIoType(dbIoType::INPUT);
      mod_bterm->setSigType(dbSigType::SIGNAL);
      dbModInst* parent_inst = cur_module->getModInst();
      cur_module = parent_inst->getParent();
      dbModITerm* mod_iterm = dbModITerm::create(
          parent_inst, connection_name_i.c_str(), mod_bterm);
      if (cur_module != highest_common_module) {
        dest_db_mod_net = dbModNet::create(cur_module, connection_name);
        mod_iterm->connect(dest_db_mod_net);
      }
      // save the top level destination pin for final connection
      top_mod_dest = mod_iterm;
    }

    // Finally do the connection in the highest common module
    if (top_mod_dest) {
      // if we don't have a top net (case when we are connecting source at top
      // to hierarchically created pin), create one in the highest module
      if (!top_net) {
        source_db_mod_net
            = dbModNet::create(highest_common_module, connection_name);
        top_mod_dest->connect(source_db_mod_net);
        source_pin->connect(source_db_mod_net);
      } else {
        top_mod_dest->connect(top_net);
      }
    } else {
      dest_pin->connect(top_net);
    }

    // During the addition of new ports and new wiring we may
    // leave orphaned pins, clean them up.
    std::set<dbModInst*> cleaned_up;
    for (auto module_to_clean_up : source_parent_tree) {
      dbModInst* mi = module_to_clean_up->getModInst();
      if (mi) {
        mi->RemoveUnusedPortsAndPins();
        cleaned_up.insert(mi);
      }
    }
    for (auto module_to_clean_up : dest_parent_tree) {
      dbModInst* mi = module_to_clean_up->getModInst();
      if (mi) {
        if (cleaned_up.find(mi) == cleaned_up.end()) {
          mi->RemoveUnusedPortsAndPins();
          cleaned_up.insert(mi);
        }
      }
    }
  }
}

/*
We expect each hierarchical net to have only one flat net
associated with it.
The routines below are called each time we move mod nets around
so that we maintain the axiom.
*/

class ModDbNetAssociation : public PinVisitor
{
 public:
  ModDbNetAssociation(dbNetwork* nwk,
                      Logger* logger,
                      dbModNet* mod_net,
                      dbNet* new_flat_net,
                      dbNet* orig_flat_net);
  void operator()(const Pin* pin) override;
  Logger* logger_ = nullptr;

 protected:
  dbNetwork* db_network_;
  dbModNet* mod_net_;
  dbNet* new_flat_net_;
  dbNet* orig_flat_net_;
};

ModDbNetAssociation::ModDbNetAssociation(dbNetwork* nwk,
                                         Logger* logger,
                                         dbModNet* mod_net,
                                         dbNet* new_flat_net,
                                         dbNet* orig_flat_net)
    : db_network_(nwk),
      mod_net_(mod_net),
      new_flat_net_(new_flat_net),
      orig_flat_net_(orig_flat_net)
{
  logger_ = logger;
}

void ModDbNetAssociation::operator()(const Pin* pin)
{
  dbITerm* iterm;
  dbBTerm* bterm;
  dbModBTerm* modbterm;
  dbModITerm* moditerm;
  db_network_->staToDb(pin, iterm, bterm, moditerm, modbterm);

  if (moditerm || modbterm) {
    return;
  }
  dbNet* cur_flat_net = db_network_->flatNet(pin);

  if (cur_flat_net == orig_flat_net_ || orig_flat_net_ == nullptr) {
    db_network_->disconnectPin(const_cast<Pin*>(pin),
                               db_network_->dbToSta(cur_flat_net));
    db_network_->connectPin(const_cast<Pin*>(pin),
                            db_network_->dbToSta(new_flat_net_));
  } else if (cur_flat_net != orig_flat_net_) {
    logger_->error(
        ORD,
        2032,
        "Illegal hierarchy expect only one flat net type per hierarchical net");
  }
}

/*
We require each modnet has exactly one flat net associated with it.
It is convenient to move modnets around during hierarchical operations
which may change the underlying flat net associated with a modnet.
This routine does the house keeping needed to preserve the requirement
of one modnet having only one flat net associated with it.
*/
void dbNetwork::reassociateHierFlatNet(dbModNet* mod_net,
                                       dbNet* new_flat_net,
                                       dbNet* orig_flat_net)
{
  ModDbNetAssociation visitor(
      this, logger_, mod_net, new_flat_net, orig_flat_net);
  NetSet visited_nets(this);
  visitConnectedPins(dbToSta(mod_net), visitor, visited_nets);
}

void dbNetwork::replaceHierModule(dbModInst* mod_inst, dbModule* module)
{
  (void) mod_inst->swapMaster(module);
}

}  // namespace sta
