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
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <unordered_set>
#include <vector>

#include "dbEditHierarchy.hh"
#include "odb/db.h"
#include "odb/dbObject.h"
#include "odb/dbSet.h"
#include "odb/dbTypes.h"
#include "odb/dbUtil.h"
#include "odb/geom.h"
#include "sta/ConcreteLibrary.hh"
#include "sta/ConcreteNetwork.hh"
#include "sta/Iterator.hh"
#include "sta/Liberty.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/PatternMatch.hh"
#include "sta/PortDirection.hh"
#include "sta/Search.hh"
#include "sta/StringUtil.hh"
#include "sta/VertexId.hh"
#include "utl/Logger.h"
#include "utl/algorithms.h"

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

namespace {

// TODO: move to StringUtil
char* tmpStringCopy(const char* str)
{
  char* tmp = makeTmpString(strlen(str) + 1);
  strcpy(tmp, str);
  return tmp;
}

// This struct contains common information about Pins
// (dbITerm, dbBTerm or dbModITerm) for debugging purposes.
struct PinInfo
{
  const char* name = "NOT_ALLOC";  // Pin hierarchical name
  int id = 0;                      // dbObject ID
  const char* type_name = "NULL";  // dbObject type name
  bool valid = false;              // false if it is a freed dbObject
  void* addr = nullptr;
};

PinInfo getPinInfo(const dbNetwork* network, const Pin* pin)
{
  PinInfo info{"NOT_ALLOC", 0, "NULL", false, nullptr};
  dbITerm* iterm;
  dbBTerm* bterm;
  dbModITerm* moditerm;
  network->staToDb(pin, iterm, bterm, moditerm);

  if (iterm) {
    info.id = iterm->getId();
    info.type_name = iterm->getTypeName();
    info.valid = iterm->isValid();
    info.addr = static_cast<void*>(iterm);
  } else if (bterm) {
    info.id = bterm->getId();
    info.type_name = bterm->getTypeName();
    info.valid = bterm->isValid();
    info.addr = static_cast<void*>(bterm);
  } else if (moditerm) {
    info.id = moditerm->getId();
    info.type_name = moditerm->getTypeName();
    info.valid = moditerm->isValid();
    info.addr = static_cast<void*>(moditerm);
  }

  if (info.valid) {
    info.name = network->pathName(pin);
  } else {
    network->getLogger()->error(
        ORD,
        2014,
        "Attempted to access invalid pin {}({}). Check if it is "
        "deleted.",
        info.type_name,
        info.id);
  }

  return info;
}
}  // namespace

//
// Handling of object ids (Hierachy Mode)
//--------------------------------------
//
// The database assigns a number to each object. These numbers
// are scoped based on the type. Eg odb::dbModInst 1..N or dbInst 1..N.
// The timer requires a unique id for each object for its visit
// pattern, so we uniquify the numbers by suffixing a discriminating
// address pattern to the lower bits and shifting.
// Everytime a new type of timing related object is added, we
// must update this code, so it is isolated and marked up here.
//
// The id is used by the STA traversers to accumulate visited.
// lower 4  bits used to encode type
//

ObjectId dbNetwork::getDbNwkObjectId(const dbObject* object) const
{
  const dbObjectType typ = object->getObjectType();
  const ObjectId db_id = object->getId();
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

// We have exactly 8 values available in the lower 3 bits of the
// pointer due to these classes all having 8 byte alignment. Used
// to avoid having to call into the database to figure out the
// type information, which requires a lot of pointer indirection.

// Used to get the value of the pointer tag.
static constexpr std::uintptr_t kPointerTagMask = std::uintptr_t(0b111U);

enum class PinPointerTags : std::uintptr_t
{
  kNone = 0U,
  kDbIterm = 1U,
  kDbBterm = 2U,
  kDbModIterm = 3U,
};

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
  dbSet<odb::dbModInst>::iterator modinst_iter_;
  dbSet<odb::dbModInst>::iterator modinst_end_;
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
      odb::dbModInst* mod_inst = nullptr;
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
    odb::dbModInst* child = *modinst_iter_;
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
  dbSet<odb::dbModNet>::iterator mod_net_iter_;
  dbSet<odb::dbModNet>::iterator mod_net_end_;
  std::vector<dbNet*> flat_nets_vec_;
  size_t flat_net_idx_ = 0;
};

DbInstanceNetIterator::DbInstanceNetIterator(const Instance* instance,
                                             const dbNetwork* network)
    : network_(network)
{
  if (network_->hasHierarchy()) {
    //
    // In hierarchical flow, the net iterator collects both hierarchical
    // nets (dbModNets) and unique flat nets (dbNets) within the
    // instance's module scope.
    // Flat nets can be retrieved by traversing instance ITerms and BTerms.
    // Avoids the duplication b/w flat and hierarchical nets.
    //

    // Get the module of the instance
    dbModule* module = nullptr;
    if (instance == network->topInstance()) {
      module = network->block()->getTopModule();
    } else {
      dbInst* db_inst;
      odb::dbModInst* mod_inst;
      network_->staToDb(instance, db_inst, mod_inst);
      if (mod_inst) {
        module = mod_inst->getMaster();
      }
    }

    if (module) {
      // Get dbModNets
      dbSet<odb::dbModNet> mod_nets = module->getModNets();
      mod_net_iter_ = mod_nets.begin();
      mod_net_end_ = mod_nets.end();

      // Keep track of flat nets that are already represented by a mod_net
      // to avoid returning both.
      std::unordered_set<dbNet*> handled_flat_nets;
      for (odb::dbModNet* mod_net : mod_nets) {
        dbNet* flat_net = network_->findRelatedDbNet(mod_net);
        if (flat_net) {
          handled_flat_nets.insert(flat_net);
        }
      }

      // Collect dbNets from children dbInsts' pins that are not already
      // handled.
      std::unordered_set<dbNet*> flat_nets_set;
      for (dbInst* child_inst : module->getInsts()) {
        for (dbITerm* iterm : child_inst->getITerms()) {
          dbNet* flat_net = iterm->getNet();
          if (flat_net
              && handled_flat_nets.find(flat_net) == handled_flat_nets.end()) {
            flat_nets_set.insert(flat_net);
          }
        }
      }

      // For top instance, also check top-level ports (BTerms)
      if (instance == network->topInstance()) {
        for (dbBTerm* bterm : network->block()->getBTerms()) {
          dbNet* flat_net = bterm->getNet();
          if (flat_net
              && handled_flat_nets.find(flat_net) == handled_flat_nets.end()) {
            flat_nets_set.insert(flat_net);
          }
        }
      }
      flat_nets_vec_.assign(flat_nets_set.begin(), flat_nets_set.end());
      // Sort these nets so their order is determinisitc
      std::ranges::sort(
          flat_nets_vec_, {}, [](auto const* net1) { return net1->getId(); });
    }
  } else {
    //
    // In flat flow, the net iterator collects all nets from top block
    //
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
    if (mod_net_iter_ != mod_net_end_) {
      return true;
    }
    return flat_net_idx_ < flat_nets_vec_.size();
  }
  return iter_ != end_;
}

Net* DbInstanceNetIterator::next()
{
  if (network_->hasHierarchy()) {
    if (mod_net_iter_ != mod_net_end_) {
      odb::dbModNet* net = *mod_net_iter_;
      mod_net_iter_++;
      return network_->dbToSta(net);
    }
    if (flat_net_idx_ < flat_nets_vec_.size()) {
      dbNet* net = flat_nets_vec_[flat_net_idx_++];
      return network_->dbToSta(net);
    }
    return nullptr;
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
  odb::dbModInst* mod_inst_;
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
    odb::dbModInst* mod_inst;
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
    while (bitr_ != bitr_end_) {
      dbBTerm* bterm = *bitr_;
      if (!network_->isPGSupply(bterm)) {
        next_ = network_->dbToSta(bterm);
        bitr_++;
        return true;
      }
      bitr_++;
    }
    return false;
  }

  while (iitr_ != iitr_end_) {
    dbITerm* iterm = *iitr_;
    if (!network_->isPGSupply(iterm)) {
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
  odb::dbModNet* modnet = nullptr;
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
    if (!network_->isPGSupply(iterm)) {
      next_ = network_->dbToSta(*iitr_);
      ++iitr_;
      return true;
    }
    iitr_++;
  }
  if ((mitr_ != mitr_end_) && (network_->hasHierarchy())) {
    next_ = network_->dbToSta(*mitr_);
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
  odb::dbModNet* modnet = nullptr;
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
  while (iter_ != end_) {
    dbBTerm* bterm = *iter_;
    iter_++;
    if (!network_->isPGSupply(bterm)) {
      return network_->dbToStaTerm(bterm);
    }
  }

  if (mod_iter_ != mod_end_ && (network_->hasHierarchy())) {
    dbModBTerm* modbterm = *mod_iter_;
    mod_iter_++;
    return network_->dbToStaTerm(modbterm);
  }
  return nullptr;
}

////////////////////////////////////////////////////////////////

dbNetwork::dbNetwork()
    : top_instance_(reinterpret_cast<Instance*>(1)),
      hierarchy_editor_(std::make_unique<dbEditHierarchy>(this, nullptr))
{
}

dbNetwork::~dbNetwork() = default;

void dbNetwork::init(dbDatabase* db, utl::Logger* logger)
{
  db_ = db;
  logger_ = logger;
  hierarchy_editor_->setLogger(logger);
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

bool dbNetwork::isTopInstanceOrNull(const Instance* instance) const
{
  return (instance == nullptr) || (instance == top_instance_);
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

  if (hasHierarchy()) {
    if (isConcretePort(port)) {
      return ConcreteNetwork::id(port);
    }
    const dbObject* obj = reinterpret_cast<const dbObject*>(port);
    return getDbNwkObjectId(obj);
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
  if (hasHierarchy()) {
    if (!isConcreteCell(cell)) {
      const dbObject* obj = reinterpret_cast<const dbObject*>(cell);
      return getDbNwkObjectId(obj);
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
  if (hasHierarchy()) {
    const dbObject* obj = reinterpret_cast<const dbObject*>(instance);
    return getDbNwkObjectId(obj);
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

  if (hasHierarchy()) {
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
  odb::dbModInst* mod_inst;
  staToDb(instance, db_inst, mod_inst);
  std::string name;
  if (db_inst) {
    name = db_inst->getName();
  }
  if (mod_inst) {
    name = mod_inst->getName();
  }

  if (hasHierarchy()) {
    size_t last_idx = std::string::npos;
    size_t pos = name.length();
    while ((pos = name.rfind('/', pos)) != std::string::npos) {
      if (pos > 0 && name[pos - 1] == '\\') {
        // This is an escaped slash, so we should ignore it and continue
        // searching.
        pos--;
      } else {
        last_idx = pos;
        break;
      }
    }
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
    odb::dbStringProperty* property
        = odb::dbStringProperty::find(obj, key.c_str());
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
    odb::dbStringProperty* property
        = odb::dbStringProperty::find(obj, key.c_str());
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
  odb::dbModInst* mod_inst;
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
  odb::dbModInst* mod_inst;
  staToDb(instance, db_inst, mod_inst);
  if (mod_inst) {
    dbModule* parent_module = mod_inst->getParent();
    if (parent_module) {
      odb::dbModInst* parent_inst = parent_module->getModInst();
      if (parent_inst) {
        return dbToSta(parent_inst);
      }
    }
  }
  if (db_inst) {
    if (!hasHierarchy()) {
      return top_instance_;
    }
    dbModule* parent_module = db_inst->getModule();
    if (parent_module) {
      odb::dbModInst* parent_inst = parent_module->getModInst();
      if (parent_inst) {
        return dbToSta(parent_inst);
      }
    }
  }
  return top_instance_;
}

Port* dbNetwork::findPort(const Cell* cell, const char* name) const
{
  if (hasHierarchy()) {
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

  staToDb(pin, iterm, bterm, moditerm);
  return !moditerm;
}

bool dbNetwork::isLeaf(const Instance* instance) const
{
  if (instance == top_instance_) {
    return false;
  }
  if (hasHierarchy()) {
    dbMaster* db_master;
    dbModule* db_module;
    Cell* cur_cell = cell(instance);
    staToDb(cur_cell, db_master, db_module);
    if (db_module) {
      return false;
    }
    return true;
  }
  return instance != top_instance_;
}

Instance* dbNetwork::findInstance(const char* path_name) const
{
  if (hasHierarchy()) {  // are we in hierarchical mode ?
    // find a hierarchical module instance first
    odb::dbModInst* mod_inst = block()->findModInst(path_name);
    if (mod_inst) {
      return dbToSta(mod_inst);
    }

    std::string path_name_str = path_name;
    // search for the last token in the string, which is the leaf instance name
    size_t last_idx = path_name_str.find_last_of('/');
    if (last_idx != std::string::npos) {
      std::string leaf_inst_name = path_name_str.substr(last_idx + 1);
      // get the parent name, which is the hierarchical prefix in the string
      std::string parent_name_str = path_name_str.substr(0, last_idx);
      // get the module instance from the block
      odb::dbModInst* parent_mod_inst
          = block()->findModInst(parent_name_str.c_str());
      if (parent_mod_inst) {
        // get the module definition
        //(we are in a uniquified environment so all modules are uniquified).
        dbModule* module_defn = parent_mod_inst->getMaster();
        if (module_defn) {
          // get the leaf instance definition from the module
          dbInst* ret = module_defn->findDbInst(leaf_inst_name.c_str());
          if (ret) {
            return (Instance*) ret;
          }
        }
      }
    }
  }
  // fall through (even in hierarchical mode).
  // Note: the fall through is the work around so that if the name
  // is stored in flat form it will be found. TODO: stash the names
  // hierachically by default for all cases and not flat in the dbBlock.
  // (currently we,mostly, stash the names flat in the block and that is wrong
  // in hierachical mode).
  //
  dbInst* inst = block_->findInst(path_name);
  return dbToSta(inst);
}

Instance* dbNetwork::findChild(const Instance* parent, const char* name) const
{
  if (parent == top_instance_) {
    dbInst* inst = block_->findInst(name);
    if (!inst) {
      dbModule* top_module = block_->getTopModule();
      odb::dbModInst* mod_inst = top_module->findModInst(name);
      return dbToSta(mod_inst);
    }
    return dbToSta(inst);
  }
  dbInst* db_inst;
  odb::dbModInst* mod_inst;
  staToDb(parent, db_inst, mod_inst);
  if (!mod_inst) {
    return nullptr;
  }
  dbModule* master_module = mod_inst->getMaster();
  odb::dbModInst* child_inst = master_module->findModInst(name);
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
  odb::dbModInst* mod_inst;
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

//
// Catch all see if a net exists anywhere in the design hierarchy
//
// TODO: remove this by removing flat net table so that all
// net names stored in their scope (so dbNet in top dbModule not
// in block).
//
Net* dbNetwork::findNetAllScopes(const char* net_name) const
{
  for (dbModule* dbm : block_->getModules()) {
    dbNet* dnet = block_->findNet(net_name);
    if (dnet) {
      return dbToSta(dnet);
    }
    odb::dbModNet* modnet = dbm->getModNet(net_name);
    if (modnet) {
      return dbToSta(modnet);
    }
  }
  return nullptr;
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
    odb::dbModNet* modnet = scope->getModNet(net_name);
    if (modnet) {
      return dbToSta(modnet);
    }
    return nullptr;
  }

  dbInst* db_inst = nullptr;
  odb::dbModInst* mod_inst = nullptr;
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
    odb::dbModNet* modnet = scope->getModNet(net_name);
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
  odb::dbModInst* mod_inst;
  staToDb(inst, db_inst, mod_inst);
  odb::dbObject* obj;
  if (db_inst) {
    obj = db_inst;
  } else {
    obj = mod_inst;
  }
  if (obj) {
    odb::dbStringProperty* property
        = odb::dbStringProperty::find(obj, key.c_str());
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
  odb::dbModInst* mod_inst;
  staToDb(instance, db_inst, mod_inst);
  odb::dbObject* obj;
  if (db_inst) {
    obj = db_inst;
  } else {
    obj = mod_inst;
  }
  if (obj) {
    odb::dbStringProperty* property
        = odb::dbStringProperty::find(obj, key.c_str());
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

  staToDb(pin, iterm, bterm, moditerm);

  if (hasHierarchy()) {
    // get the id for hierarchical objects using dbid.
    std::uintptr_t tag_value
        = reinterpret_cast<std::uintptr_t>(pin) & kPointerTagMask;
    // Need to cast to char pin to avoid compiler error for pointer
    // arithmetic on incomplete types
    const char* char_pin = reinterpret_cast<const char*>(pin);
    const dbObject* obj
        = reinterpret_cast<const dbObject*>(char_pin - tag_value);
    return getDbNwkObjectId(obj);
  }
  if (iterm != nullptr) {
    return iterm->getId() << 1;
  }
  if (bterm != nullptr) {
    return (bterm->getId() << 1) | 1U;
  }
  return 0;
}

Instance* dbNetwork::instance(const Pin* pin) const
{
  dbITerm* iterm = nullptr;
  dbBTerm* bterm = nullptr;
  dbModITerm* moditerm = nullptr;

  staToDb(pin, iterm, bterm, moditerm);
  if (iterm) {
    dbInst* dinst = iterm->getInst();
    return dbToSta(dinst);
  }
  if (bterm) {
    return top_instance_;
  }
  if (moditerm) {
    odb::dbModInst* mod_inst = moditerm->getParent();
    return dbToSta(mod_inst);
  }
  return nullptr;
}

Net* dbNetwork::net(const Pin* pin) const
{
  dbITerm* iterm = nullptr;
  dbBTerm* bterm = nullptr;
  dbModITerm* moditerm = nullptr;

  staToDb(pin, iterm, bterm, moditerm);
  if (iterm) {
    //
    // TODO: reverse this logic so we always get the
    // flat net, and fix the verilog writer.
    //

    // It is possible when writing out a hierarchical network
    // that we have both a mod net and a dbinst net.
    // In the case of writing out a hierachical network we always
    // choose the mnet.
    if (odb::dbModNet* mnet = iterm->getModNet()) {
      return dbToSta(mnet);
    }
    if (dbNet* dnet = iterm->getNet()) {
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
    if (odb::dbModNet* mnet = moditerm->getModNet()) {
      return dbToSta(mnet);
    }
  }

  return nullptr;
}

/*
Get the db net (flat net) for the pin
*/

dbNet* dbNetwork::flatNet(const Pin* pin) const
{
  dbNet* db_net;
  odb::dbModNet* db_modnet;
  net(pin, db_net, db_modnet);
  return db_net;
}

odb::dbModNet* dbNetwork::hierNet(const Pin* pin) const
{
  dbNet* db_net;
  odb::dbModNet* db_modnet;
  net(pin, db_net, db_modnet);
  return db_modnet;
}

/*
Get the dbnet or the moddbnet for a pin
Sometimes a pin can be hooked to both and we want to expose them
both, so we add this api
 */
void dbNetwork::net(const Pin* pin,
                    dbNet*& db_net,
                    odb::dbModNet*& db_modnet) const
{
  dbITerm* iterm = nullptr;
  dbBTerm* bterm = nullptr;
  dbModITerm* moditerm = nullptr;
  db_net = nullptr;
  db_modnet = nullptr;

  staToDb(pin, iterm, bterm, moditerm);
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
}

Term* dbNetwork::term(const Pin* pin) const
{
  dbITerm* iterm = nullptr;
  dbBTerm* bterm = nullptr;
  dbModITerm* moditerm = nullptr;
  staToDb(pin, iterm, bterm, moditerm);
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
  return nullptr;
}

Port* dbNetwork::port(const Pin* pin) const
{
  dbITerm* iterm;
  dbBTerm* bterm;
  dbModITerm* moditerm;
  Port* ret = nullptr;
  // Will return the bterm for a top level pin
  staToDb(pin, iterm, bterm, moditerm);

  if (iterm) {
    dbMTerm* mterm = iterm->getMTerm();
    ret = dbToSta(mterm);
  } else if (bterm) {
    const char* port_name = bterm->getConstName();
    ret = findPort(top_cell_, port_name);
  } else if (moditerm) {
    std::string port_name_str = moditerm->getName();
    const char* port_name = port_name_str.c_str();
    odb::dbModInst* mod_inst = moditerm->getParent();
    dbModule* module = mod_inst->getMaster();
    dbModBTerm* mod_port = module->findModBTerm(port_name);
    if (mod_port) {
      ret = dbToSta(mod_port);
      return ret;
    }
  }
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
  dbModITerm* moditerm;

  staToDb(pin, iterm, bterm, moditerm);
  if (iterm) {
    PortDirection* dir = dbToSta(iterm->getSigType(), iterm->getIoType());
    return dir;
  }
  if (bterm) {
    PortDirection* dir = dbToSta(bterm->getSigType(), bterm->getIoType());
    return dir;
  }
  if (moditerm) {
    // get the direction of the modbterm
    std::string pin_name = moditerm->getName();
    odb::dbModInst* mod_inst = moditerm->getParent();
    dbModule* module = mod_inst->getMaster();
    dbModBTerm* modbterm_local = module->findModBTerm(pin_name.c_str());
    assert(modbterm_local != nullptr);
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
  staToDb(pin, iterm, bterm, miterm);
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
  staToDb(pin, iterm, bterm, moditerm);
  // timing arcs only set on leaf level iterm/bterm.
  if (iterm) {
    iterm->staSetVertexId(id);
  } else if (bterm) {
    bterm->staSetVertexId(id);
  }
}

dbModITerm* dbNetwork::findInputModITermInParent(const Pin* input_pin) const
{
  if (input_pin == nullptr) {
    return nullptr;
  }

  dbITerm* iterm = nullptr;
  dbBTerm* bterm = nullptr;
  dbModITerm* mod_iterm = nullptr;
  staToDb(input_pin, iterm, bterm, mod_iterm);

  // Get mod net
  odb::dbModNet* mod_net = nullptr;
  if (iterm) {
    assert(iterm->getIoType().getValue() != dbIoType::OUTPUT);
    mod_net = iterm->getModNet();
  } else if (mod_iterm) {
    assert(mod_iterm->getChildModBTerm()->getIoType().getValue()
           != dbIoType::OUTPUT);
    mod_net = mod_iterm->getModNet();
  }

  if (mod_net == nullptr) {
    return nullptr;
  }

  // Get the input modBTerm.
  // - Typically, there will be one or zero input modBTerm.
  dbModBTerm* input_mod_bterm = nullptr;
  for (dbModBTerm* mod_bterm : mod_net->getModBTerms()) {
    if (dbIoType::OUTPUT != mod_bterm->getIoType().getValue()) {
      input_mod_bterm = mod_bterm;
      break;
    }
  }

  if (input_mod_bterm == nullptr) {
    return nullptr;
  }

  // Found the target modITerm in parent.
  dbModITerm* parent_mod_iterm = input_mod_bterm->getParentModITerm();
  return parent_mod_iterm;
}

void dbNetwork::location(const Pin* pin,
                         // Return values.
                         double& x,
                         double& y,
                         bool& exists) const
{
  if (isPlaced(pin)) {
    odb::Point pt = location(pin);
    x = dbuToMeters(pt.getX());
    y = dbuToMeters(pt.getY());
    exists = true;
  } else {
    x = 0;
    y = 0;
    exists = false;
  }
}

odb::Point dbNetwork::location(const Pin* pin) const
{
  dbITerm* iterm = nullptr;
  dbBTerm* bterm = nullptr;
  dbModITerm* moditerm = nullptr;

  staToDb(pin, iterm, bterm, moditerm);
  if (iterm) {
    int x, y;
    if (iterm->getAvgXY(&x, &y)) {
      return odb::Point(x, y);
    }
    return iterm->getInst()->getOrigin();
  }
  if (bterm) {
    int x, y;
    if (bterm->getFirstPinLocation(x, y)) {
      return odb::Point(x, y);
    }
  }
  return odb::Point(0, 0);
}

bool dbNetwork::isPlaced(const Pin* pin) const
{
  dbITerm* iterm = nullptr;
  dbBTerm* bterm = nullptr;
  dbModITerm* moditerm = nullptr;

  staToDb(pin, iterm, bterm, moditerm);
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
  odb::dbModNet* modnet = nullptr;
  dbNet* dnet = nullptr;
  staToDb(net, dnet, modnet);
  if (hasHierarchy()) {
    const dbObject* obj = reinterpret_cast<const dbObject*>(net);
    return getDbNwkObjectId(obj);
  }
  assert(dnet != nullptr);
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

  odb::dbModNet* modnet = nullptr;
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

    // Make a full hierarchical name
    fmt::memory_buffer full_path_buf;
    std::vector<dbModule*> parent_hierarchy;
    hierarchy_editor_->getParentHierarchy(parent_module, parent_hierarchy);
    std::ranges::reverse(parent_hierarchy);
    auto back_inserter = std::back_inserter(full_path_buf);
    for (dbModule* db_mod : parent_hierarchy) {
      fmt::format_to(back_inserter,
                     "{}{}",
                     db_mod->getName(),
                     block_->getHierarchyDelimiter());
    }
    full_path_buf.append(modnet_name);
    full_path_buf.push_back('\0');
    return tmpStringCopy(full_path_buf.data());
  }
  return nullptr;
}

/*
  dbNets which are connected to pins which have mod nets are
  "boundaries".
 */

const char* dbNetwork::name(const Net* net) const
{
  odb::dbModNet* modnet = nullptr;
  dbNet* dnet = nullptr;
  staToDb(net, dnet, modnet);
  std::string name;

  Network* sta_nwk = (Network*) this;

  if (dnet && !modnet) {
    name = dnet->getName();
    // strip out the parent name in hierarchy mode
    // turn this off to get full flat names

    if (hasHierarchy()) {
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
  dbNet* db_net;
  odb::dbModNet* db_modnet;
  staToDb(net, db_net, db_modnet);
  if (db_net) {
    return (db_net->getSigType() == dbSigType::POWER);
  }

  if (db_modnet) {
    dbNet* related_net = findRelatedDbNet(db_modnet);
    if (related_net) {
      return (related_net->getSigType() == dbSigType::POWER);
    }
  }
  return false;
}

bool dbNetwork::isGround(const Net* net) const
{
  dbNet* db_net;
  odb::dbModNet* db_modnet;
  staToDb(net, db_net, db_modnet);
  if (db_net) {
    return (db_net->getSigType() == dbSigType::GROUND);
  }
  if (db_modnet) {
    dbNet* related_net = findRelatedDbNet(db_modnet);
    if (related_net) {
      return (related_net->getSigType() == dbSigType::GROUND);
    }
  }
  return false;
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
  odb::dbModNet* mod_net = nullptr;
  dbNet* db_net = nullptr;

  if (visited_nets.contains(net)) {
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
    for (dbModITerm* moditerm : mod_net->getModITerms()) {
      Pin* pin = dbToSta(moditerm);
      // search down
      visitor(pin);
    }

    // visit below nets
    for (dbModITerm* moditerm : mod_net->getModITerms()) {
      odb::dbModInst* mod_inst = moditerm->getParent();
      // note we are dealing with a uniquified hierarchy
      // so one master per instance..
      dbModule* module = mod_inst->getMaster();
      std::string pin_name = moditerm->getName();
      dbModBTerm* mod_bterm = module->findModBTerm(pin_name.c_str());
      Term* below_term = dbToStaTerm(mod_bterm);
      // traverse along rest of net
      Net* below_net = this->net(below_term);
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
      odb::dbModInst* mod_inst = db_module->getModInst();
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
        if (above_net) {
          visitConnectedPins(above_net, visitor, visited_nets);
        }
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
  if (hasHierarchy()) {
    const dbObject* obj = reinterpret_cast<const dbObject*>(term);
    return getDbNwkObjectId(obj);
  }
  return staToDb(term)->getId();
}

Pin* dbNetwork::pin(const Term* term) const
{
  // In the non-hierarchical case:
  // Only terms are for top level instance pins, which are also BTerms.
  // In the hierarchical case:
  // Terms are for top level instance pins, which are dbBTerms, or
  // dbModBTerms for modules terminals.
  //
  dbBTerm* bterm = nullptr;
  dbModBTerm* modbterm = nullptr;
  dbITerm* iterm = nullptr;
  staToDb(term, iterm, bterm, modbterm);
  if (bterm) {
    return dbToSta(bterm);
  }
  if (modbterm) {
    // get the moditerm
    dbModule* cur_module = modbterm->getParent();
    odb::dbModInst* cur_mod_inst = cur_module->getModInst();
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
  dbModBTerm* modbterm = nullptr;

  staToDb(term, iterm, bterm, modbterm);

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
  dbModBTerm* modbterm = nullptr;

  staToDb(term, iterm, bterm, modbterm);

  if (modbterm) {
    return dbToSta(modbterm->getModNet());
  }
  if (bterm) {
    odb::dbModNet* mod_net = bterm->getModNet();
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

    for (dbModule* module : block_->getModules()) {
      // top_module is not a hierarchical module in this context.
      if (module != block_->getTopModule()) {
        registerHierModule(dbToSta(module));
      }
    }

    readDbNetlistAfter();
  }

  for (dbNetworkObserver* observer : observers_) {
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

        if (lib_port->isClock() && mterm->getSigType() != dbSigType::CLOCK) {
          debugPrint(logger_,
                     utl::ORD,
                     "dbNetwork",
                     1,
                     "Updating LEF pin {}/{} from {} to CLOCK from Liberty",
                     mterm->getMaster()->getName(),
                     mterm->getName(),
                     mterm->getSigType().getString());
          mterm->setSigType(dbSigType::CLOCK);
        }
      } else if (!dir->isPowerGround() && !lib_cell->findPort(port_name)) {
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
  // TODO: MSB first assumption looks risky because there can be LSB first
  // buses.
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
  checkLibertyScenes();
  checkLibertyCellsWithoutLef();
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
  groupBusPorts(top_cell_, [=, this](const char* port_name) {
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

void dbNetwork::checkLibertyCellsWithoutLef() const
{
  std::vector<std::string> cells_without_lef;
  std::unique_ptr<LibertyLibraryIterator> lib_iter{libertyLibraryIterator()};
  while (lib_iter->hasNext()) {
    LibertyLibrary* lib = lib_iter->next();
    LibertyCellIterator cell_iter(lib);
    while (cell_iter.hasNext()) {
      LibertyCell* cell = cell_iter.next();
      odb::dbMaster* master = staToDb(cell);
      if (!master) {
        cells_without_lef.emplace_back(cell->name());
        cell->setDontUse(true);
      }
    }
  }

  if (!cells_without_lef.empty()) {
    logger_->warn(ORD,
                  2056,
                  "The following {} liberty cell(s) do not have LEF masters "
                  "and will be marked as dont-use:",
                  cells_without_lef.size());
    for (const auto& cell : cells_without_lef) {
      logger_->report("  {}", cell);
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
                         && !lcell->findPort(port_name)) {
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

  for (dbNetworkObserver* observer : observers_) {
    observer->postReadLiberty();
  }
}

////////////////////////////////////////////////////////////////

// Edit functions

Instance* dbNetwork::makeInstance(LibertyCell* cell,
                                  const char* name,  // full_name
                                  Instance* parent)
{
  const char* cell_name = cell->name();
  if (isTopInstanceOrNull(parent)) {
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
      std::unique_ptr<sta::CellPortIterator> port_iter{portIterator(inst_cell)};
      while (port_iter->hasNext()) {
        Port* cur_port = port_iter->next();
        registerConcretePort(cur_port);
      }
      return dbToSta(inst);
    }
  } else {
    dbInst* db_inst = nullptr;
    odb::dbModInst* mod_inst = nullptr;
    staToDb(parent, db_inst, mod_inst);
    if (mod_inst) {
      dbMaster* master = db_->findMaster(cell_name);
      dbModule* parent = mod_inst->getMaster();
      dbInst* inst = dbInst::create(block_, master, name, false, parent);
      Cell* inst_cell = dbToSta(master);
      //
      // Register all ports of liberty cells as being concrete
      // Sometimes this method is called by the sta
      // to build "test circuits" eg to find the max wire length
      // And those cells need to use the external api
      // to get timing characteristics, so they have to be
      // concrete
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
  odb::dbModInst* mod_inst;
  staToDb(inst, db_inst, mod_inst);
  if (db_inst) {
    db_inst->swapMaster(master);
  }
}

void dbNetwork::deleteInstance(Instance* inst)
{
  dbInst* db_inst;
  odb::dbModInst* mod_inst;
  staToDb(inst, db_inst, mod_inst);
  if (db_inst) {
    dbInst::destroy(db_inst);
  } else {
    odb::dbModInst::destroy(mod_inst);
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
  staToDb(pin, iterm, bterm, moditerm);

  // get the type of the net
  dbNet* flat_net_db = nullptr;
  odb::dbModNet* hier_net_db = nullptr;

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
  staToDb(pin, iterm, bterm, moditerm);

  // get the type of the net
  dbNet* dnet = nullptr;
  odb::dbModNet* mod_net = nullptr;
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
  } else {
    logger_->error(
        ORD,
        2024,
        "Illegal net/pin combination. Modnets can only be hooked to iterm, "
        "bterm, moditerm, modbterm, dbNets can only be hooked to iterm, bterm. "
        "Pin: {}, Net: {}",
        pathName(pin),
        pathName(net));
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
    odb::dbModInst* mod_inst;
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
    drivers(net);
  } else if (isHierarchical(pin)) {
    Net* net = this->net(pin);
    drivers(net);
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
    odb::dbModInst* mod_inst;
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
  odb::dbModNet* mod_net = nullptr;
  dbNet* db_net = nullptr;
  staToDb(net, db_net, mod_net);

  dbITerm* iterm = nullptr;
  dbBTerm* bterm = nullptr;
  dbModITerm* moditerm = nullptr;
  staToDb(pin, iterm, bterm, moditerm);
  if (iterm) {
    if (db_net) {
      iterm->disconnectDbNet();
    }
    if (mod_net) {
      iterm->disconnectDbModNet();
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
  staToDb(pin, iterm, bterm, moditerm);
  if (iterm) {
    iterm->disconnect();
  } else if (bterm) {
    bterm->disconnect();
  } else if (moditerm) {
    moditerm->disconnect();
  }
}

void dbNetwork::disconnectPinBefore(const Pin* pin)
{
  // This function is called before a pin is disconnected to update (invalidate)
  // the internal driver cache (net_drvr_pin_map_).
  // 1. If load pin, nop because it is not managed by the driver cache
  // 2. If hierarchical pin:
  //    Find the associated physical net (dbNet) and all hierarchical nets
  //    (odb::dbModNet) and remove the driver information from the cache.
  // 3. If driver pin:
  //    Find the dbNet and odb::dbModNet driven by this pin and remove the
  //    cache. Also, clean up the cache for all dbModNets associated with the
  //    dbNet to maintain cache consistency between multiple hierarchical nets
  //    corresponding to a single physical net.

  // 1. Load pin case
  if (isLoad(pin)) {
    return;  // No need to update net_drvr_pin_map_ cache.
  }

  // 2. Hierarchical pin case
  if (isHierarchical(pin)) {
    dbITerm* iterm;
    dbBTerm* bterm;
    dbModITerm* moditerm;
    staToDb(pin, iterm, bterm, moditerm);
    if (moditerm == nullptr) {
      return;
    }

    odb::dbModNet* modnet = moditerm->getModNet();
    if (modnet == nullptr) {
      return;
    }

    dbNet* db_net = modnet->findRelatedNet();
    if (db_net == nullptr) {
      return;
    }

    // Remove flat net from cache
    removeDriverFromCache(dbToSta(db_net));

    // Remove all related hier nets from cache
    std::set<odb::dbModNet*> modnet_set;
    db_net->findRelatedModNets(modnet_set);
    for (odb::dbModNet* modnet : modnet_set) {
      removeDriverFromCache(dbToSta(modnet));
    }
    return;
  }

  // 3. Driver pin case

  // Get all the related dbNet & odb::dbModNet with the pin.
  // Incrementally update the net-drvr cache.
  dbNet* db_net;
  odb::dbModNet* mod_net;
  net(pin, db_net, mod_net);

  if (db_net) {
    // A dbNet can be associated with multiple dbModNets.
    // We need to update the cache for all of them.
    std::set<odb::dbModNet*> related_mod_nets;
    db_net->findRelatedModNets(related_mod_nets);
    for (odb::dbModNet* related_mod_net : related_mod_nets) {
      removeDriverFromCache(dbToSta(related_mod_net), pin);
    }

    removeDriverFromCache(dbToSta(db_net), pin);
  }

  if (mod_net) {
    removeDriverFromCache(dbToSta(mod_net), pin);
  }
}

void dbNetwork::deletePin(Pin* pin)
{
  dbITerm* iterm;
  dbBTerm* bterm;
  dbModITerm* moditerm;

  staToDb(pin, iterm, bterm, moditerm);
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

Net* dbNetwork::makeNet(Instance* parent)
{
  return makeNet("net", parent, odb::dbNameUniquifyType::ALWAYS);
}

Net* dbNetwork::makeNet(const char* base_name, Instance* parent)
{
  return makeNet(base_name, parent, odb::dbNameUniquifyType::ALWAYS);
}

// If uniquify is IF_NEEDED, unique suffix will be added when necessary.
Net* dbNetwork::makeNet(const char* base_name,
                        Instance* parent,
                        const odb::dbNameUniquifyType& uniquify)
{
  // Create a unique full name for a new net
  std::string full_name
      = block_->makeNewNetName(getModInst(parent), base_name, uniquify);

  // Create a new net
  dbNet* dnet = dbNet::create(block_, full_name.c_str(), false);
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
  auto entry = net_drvr_pin_map_.find(net);
  if (entry != net_drvr_pin_map_.end()) {
    delete entry->second;
    net_drvr_pin_map_.erase(entry);
  }
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
  dbNet* db_net = findFlatDbNet(net);
  return (db_net && db_net->isSpecial());
}

////////////////////////////////////////////////////////////////

dbInst* dbNetwork::staToDb(const Instance* instance) const
{
  dbInst* db_inst;
  odb::dbModInst* mod_inst;
  staToDb(instance, db_inst, mod_inst);
  return db_inst;
}

void dbNetwork::staToDb(const Instance* instance,
                        // Return values.
                        dbInst*& db_inst,
                        odb::dbModInst*& mod_inst) const
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
      mod_inst = static_cast<odb::dbModInst*>(obj);
    } else {
      logger_->error(ORD, 2016, "Instance is not dbInst or odb::dbModInst");
    }
  } else {
    db_inst = nullptr;
    mod_inst = nullptr;
  }
}

dbNet* dbNetwork::staToDb(const Net* net) const
{
  dbNet* db_net = reinterpret_cast<dbNet*>(const_cast<Net*>(net));
  assert(!db_net || db_net->getObjectType() == odb::dbNetObj);
  return db_net;
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

void dbNetwork::staToDb(const Net* net,
                        dbNet*& dnet,
                        odb::dbModNet*& modnet) const
{
  dnet = nullptr;
  modnet = nullptr;
  if (net) {
    dbObject* obj = reinterpret_cast<dbObject*>(const_cast<Net*>(net));
    dbObjectType type = obj->getObjectType();
    if (type == odb::dbNetObj) {
      dnet = static_cast<dbNet*>(obj);
    } else if (type == odb::dbModNetObj) {
      modnet = static_cast<odb::dbModNet*>(obj);
    } else {
      logger_->error(ORD, 2034, "Net is not dbNet or odb::dbModNet");
    }
  }
}

dbITerm* dbNetwork::flatPin(const Pin* pin) const
{
  odb::dbITerm* iterm;
  odb::dbBTerm* bterm;
  odb::dbModITerm* moditerm;
  staToDb(pin, iterm, bterm, moditerm);
  (void) bterm;
  (void) moditerm;
  return iterm;
}

bool dbNetwork::isFlat(const Pin* pin) const
{
  odb::dbITerm* iterm;
  odb::dbBTerm* bterm;
  odb::dbModITerm* moditerm;
  staToDb(pin, iterm, bterm, moditerm);
  return iterm || bterm;
}

bool dbNetwork::isFlat(const Net* net) const
{
  odb::dbNet* db_net;
  odb::dbModNet* mod_net;
  staToDb(net, db_net, mod_net);
  return db_net;
}

dbModITerm* dbNetwork::hierPin(const Pin* pin) const
{
  odb::dbITerm* iterm;
  odb::dbBTerm* bterm;
  odb::dbModITerm* moditerm;
  staToDb(pin, iterm, bterm, moditerm);
  (void) iterm;
  (void) bterm;
  return moditerm;
}

dbBlock* dbNetwork::getBlockOf(const Pin* pin) const
{
  odb::dbITerm* iterm = nullptr;
  odb::dbBTerm* bterm = nullptr;
  odb::dbModITerm* moditerm = nullptr;
  staToDb(pin, iterm, bterm, moditerm);

  dbBlock* block = nullptr;
  if (iterm) {
    block = iterm->getInst()->getBlock();
  } else if (bterm) {
    block = bterm->getBlock();
  } else if (moditerm) {
    // moditerm->parent is odb::dbModInst
    // moditerm->parent->parent is dbModule
    // dbModule->owner is dbBlock
    block = moditerm->getParent()->getParent()->getOwner();
  }
  assert(block != nullptr && "Pin must belong to a block.");
  return block;
}

void dbNetwork::staToDb(const Pin* pin,
                        // Return values.
                        dbITerm*& iterm,
                        dbBTerm*& bterm,
                        dbModITerm*& moditerm) const
{
  iterm = nullptr;
  bterm = nullptr;
  moditerm = nullptr;

  // Get the value of the tag
  std::uintptr_t pointer_with_tag = reinterpret_cast<std::uintptr_t>(pin);
  PinPointerTags tag_value
      = static_cast<PinPointerTags>(pointer_with_tag & kPointerTagMask);

  // Cast to char* and avoid casting an integral type to pointer type
  // by doing pointer arithmetic. Compiler apparently prefer this sytle.
  const char* char_pointer_pin = reinterpret_cast<const char*>(pin);
  char* pointer_without_tag = const_cast<char*>(
      char_pointer_pin - static_cast<std::uintptr_t>(tag_value));

  if (pointer_without_tag) {
    switch (tag_value) {
      case PinPointerTags::kDbIterm:
        iterm = reinterpret_cast<dbITerm*>(pointer_without_tag);
        break;
      case PinPointerTags::kDbBterm:
        bterm = reinterpret_cast<dbBTerm*>(pointer_without_tag);
        break;
      case PinPointerTags::kDbModIterm:
        moditerm = reinterpret_cast<dbModITerm*>(pointer_without_tag);
        break;
      case PinPointerTags::kNone:
        logger_->error(ORD, 2018, "Pin is not ITerm or BTerm or modITerm.");
        break;
    }
  }
}

dbObject* dbNetwork::staToDb(const Pin* pin) const
{
  dbITerm* iterm;
  dbBTerm* bterm;
  dbModITerm* moditerm;
  staToDb(pin, iterm, bterm, moditerm);
  if (iterm) {
    return iterm;
  }
  if (bterm) {
    return bterm;
  }
  if (moditerm) {
    return moditerm;
  }
  return nullptr;
}

dbBTerm* dbNetwork::staToDb(const Term* term) const
{
  dbBTerm* bterm = reinterpret_cast<dbBTerm*>(const_cast<Term*>(term));
  assert(!bterm || bterm->getObjectType() == odb::dbBTermObj);
  return bterm;
}

void dbNetwork::staToDb(const Term* term,
                        dbITerm*& iterm,
                        dbBTerm*& bterm,
                        dbModBTerm*& modbterm) const
{
  iterm = nullptr;
  bterm = nullptr;
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
    } else {
      logger_->error(ORD, 2033, "Term is not ITerm or BTerm or modBTerm.");
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
  if (isConcreteCell(cell)) {
    const ConcreteCell* ccell = reinterpret_cast<const ConcreteCell*>(cell);
    dbMaster* master = reinterpret_cast<dbMaster*>(ccell->extCell());
    assert(!master || master->getObjectType() == odb::dbMasterObj);
    return master;
  }

  dbMaster* master = nullptr;
  dbModule* module = nullptr;
  staToDb(cell, master, module);
  return master;
}

// called only on db cells.
dbMaster* dbNetwork::staToDb(const LibertyCell* cell) const
{
  return staToDb(reinterpret_cast<const Cell*>(cell));
}

dbMTerm* dbNetwork::staToDb(const Port* port) const
{
  const ConcretePort* cport = reinterpret_cast<const ConcretePort*>(port);
  dbMTerm* mterm = reinterpret_cast<dbMTerm*>(cport->extPort());
  assert(!mterm || mterm->getObjectType() == odb::dbMTermObj);
  return mterm;
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
  bterm = nullptr;
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
  } else {
    logger_->error(ORD, 2035, "Port is not BTerm or modBTerm.");
  }
}

dbMTerm* dbNetwork::staToDb(const LibertyPort* port) const
{
  dbMTerm* mterm = reinterpret_cast<dbMTerm*>(port->extPort());
  assert(!mterm || mterm->getObjectType() == odb::dbMTermObj);
  return mterm;
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

Instance* dbNetwork::dbToSta(odb::dbModInst* inst) const
{
  return reinterpret_cast<Instance*>(inst);
}

Pin* dbNetwork::dbToSta(dbModITerm* mod_iterm) const
{
  if (mod_iterm == nullptr) {
    return nullptr;
  }
  char* unaligned_pointer = reinterpret_cast<char*>(mod_iterm);
  return reinterpret_cast<Pin*>(
      unaligned_pointer
      + static_cast<std::uintptr_t>(PinPointerTags::kDbModIterm));
}

Net* dbNetwork::dbToSta(odb::dbModNet* net) const
{
  return reinterpret_cast<Net*>(net);
}

Port* dbNetwork::dbToSta(dbModBTerm* modbterm) const
{
  return reinterpret_cast<Port*>(modbterm);
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

const Net* dbNetwork::dbToSta(const odb::dbModNet* net) const
{
  return reinterpret_cast<const Net*>(net);
}

Pin* dbNetwork::dbToSta(dbBTerm* bterm) const
{
  if (bterm == nullptr) {
    return nullptr;
  }
  char* unaligned_pointer = reinterpret_cast<char*>(bterm);
  return reinterpret_cast<Pin*>(
      unaligned_pointer
      + static_cast<std::uintptr_t>(PinPointerTags::kDbBterm));
}

Pin* dbNetwork::dbToSta(dbITerm* iterm) const
{
  if (iterm == nullptr) {
    return nullptr;
  }
  char* unaligned_pointer = reinterpret_cast<char*>(iterm);
  return reinterpret_cast<Pin*>(
      unaligned_pointer
      + static_cast<std::uintptr_t>(PinPointerTags::kDbIterm));
}

Pin* dbNetwork::dbToSta(dbObject* term_obj) const
{
  if (term_obj == nullptr) {
    return nullptr;
  }

  dbObjectType type = term_obj->getObjectType();
  if (type == odb::dbITermObj) {
    return dbToSta(static_cast<dbITerm*>(term_obj));
  }
  if (type == odb::dbBTermObj) {
    return dbToSta(static_cast<dbBTerm*>(term_obj));
  }
  if (type == odb::dbModITermObj) {
    return dbToSta(static_cast<dbModITerm*>(term_obj));
  }

  return nullptr;
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
  switch (io_type.getValue()) {
    case dbIoType::INPUT:
      return PortDirection::input();

    case dbIoType::OUTPUT:
      return PortDirection::output();

    case dbIoType::INOUT:
      return PortDirection::bidirect();

    case dbIoType::FEEDTHRU:
      return PortDirection::bidirect();
  }
  logger_->critical(ORD, 2008, "unknown master term type");
  return PortDirection::bidirect();
}

////////////////////////////////////////////////////////////////

LibertyCell* dbNetwork::libertyCell(Cell* cell) const
{
  if (isConcreteCell(cell) == false) {
    dbMaster* master = nullptr;
    dbModule* module = nullptr;
    staToDb(cell, master, module);
    if (master) {
      cell = reinterpret_cast<Cell*>(master->staCell());
    } else if (module) {
      return nullptr;  // dbModule does not have the corresponding LibertyCell
    }
  }
  return ConcreteNetwork::libertyCell(cell);
}

const LibertyCell* dbNetwork::libertyCell(const Cell* cell) const
{
  return libertyCell(const_cast<Cell*>(cell));
}

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
  const Instance* cur_instance = instance(pin);
  dbInst* db_inst = nullptr;
  odb::dbModInst* mod_inst = nullptr;
  staToDb(cur_instance, db_inst, mod_inst);
  if (db_inst) {
    LibertyPort* ret = ConcreteNetwork::libertyPort(pin);
    return ret;
  }
  return nullptr;
}

void dbNetwork::registerHierModule(const Cell* cell)
{
  hier_modules_.insert(cell);
}

void dbNetwork::unregisterHierModule(const Cell* cell)
{
  hier_modules_.erase(cell);
}

/*
We keep a registry of the concrete cells.
For these we know to use the concrete network interface.
The concrete cells are created outside of the odb world
-- attempting to type cast those can lead to bad pointers.
So we simply note them and then when we inspect a cell
we can decide whether or not to use the ConcreteNetwork api.
*/
bool dbNetwork::isConcreteCell(const Cell* cell) const
{
  if (!hasHierarchy()) {
    return true;
  }

  if (cell == top_cell_) {
    return false;
  }

  return (hier_modules_.find(cell) == hier_modules_.end());
}

void dbNetwork::registerConcretePort(const Port* port)
{
  concrete_ports_.insert(port);
}

bool dbNetwork::isConcretePort(const Port* port) const
{
  if (!hasHierarchy()) {
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
  if (hasHierarchy()) {
    dbMTerm* mterm = nullptr;
    dbBTerm* bterm = nullptr;
    dbModBTerm* modbterm = nullptr;
    staToDb(port, bterm, mterm, modbterm);
    if (modbterm) {
      return modbterm->isBusPort();
    }

    // If port is a bus port of leaf liberty instance, modbterm is null.
    // In the case, cport->hasMembers() should be checked.
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
  // top-level port is concrete port. DbNetworkPortMemberIterator cannot handle
  // it.
  if (!hasHierarchy() || isConcretePort(port)) {
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

void dbNetwork::hierarchicalConnect(dbITerm* source_pin,
                                    dbITerm* dest_pin,
                                    const char* connection_name)
{
  hierarchy_editor_->hierarchicalConnect(source_pin, dest_pin, connection_name);
}

void dbNetwork::hierarchicalConnect(dbITerm* source_pin,
                                    dbModITerm* dest_pin,
                                    const char* connection_name)
{
  hierarchy_editor_->hierarchicalConnect(source_pin, dest_pin, connection_name);
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
    odb::dbModNet* modnet;
    staToDb(net, dnet, modnet);
    if (dnet) {
      //
      // get sink driver instance and return its parent
      //
      PinSet* drivers = this->drivers(net);
      if (drivers && !drivers->empty()) {
        const Pin* drvr_pin = *drivers->begin();
        driver_pin = const_cast<Pin*>(drvr_pin);
        odb::dbITerm* iterm;
        odb::dbBTerm* bterm;
        odb::dbModITerm* mod_iterm;
        staToDb(drvr_pin, iterm, bterm, mod_iterm);
        (void) (mod_iterm);
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
      for (const Pin* pin : *drivers) {
        dbITerm* iterm = nullptr;
        dbBTerm* bterm = nullptr;
        dbModITerm* moditerm = nullptr;
        staToDb(pin, iterm, bterm, moditerm);
        driver_pin = const_cast<Pin*>(pin);
        if (hier == false) {
          return modnet->getParent();
        }
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

  if (net) {
    PinSet* drivers = this->drivers(net);
    if (drivers && !drivers->empty()) {
      const Pin* drvr_pin = *drivers->begin();
      driver_pin = const_cast<Pin*>(drvr_pin);
    }
  }
  // default to top module
  return block_->getTopModule();
}

class PinConnections : public PinVisitor
{
 public:
  void operator()(const Pin* pin) override;
  bool connected(const Pin* pin) const;

 private:
  std::unordered_set<const Pin*> pins_;
};

void PinConnections::operator()(const Pin* pin)
{
  pins_.insert(pin);
}

bool PinConnections::connected(const Pin* pin) const
{
  return pins_.find(pin) != pins_.end();
}

void dbNetwork::removeUnusedPortsAndPinsOnModuleInstances()
{
  for (odb::dbModInst* mi : block()->getModInsts()) {
    mi->removeUnusedPortsAndPins();
  }
}

/*
Top level bterm names must be preserved. If we are reassociating
a hierarchical net with a flat net connected to a primary port
we have to rename the hierarhical net to match the bterm
*/

class DbNetConnectedToBTerm : public PinVisitor
{
 public:
  DbNetConnectedToBTerm(dbNetwork* nwk) : db_network_(nwk) {}
  void operator()(const Pin* pin) override;
  dbBTerm* bterm() const { return bterm_; }

 private:
  dbNetwork* db_network_;
  dbBTerm* bterm_{nullptr};
};

bool dbNetwork::isConnected(const Pin* source_pin, const Pin* dest_pin) const
{
  PinConnections visitor;
  network_->visitConnectedPins(source_pin, visitor);
  return visitor.connected(dest_pin);
}

bool dbNetwork::isConnected(const Net* net, const Pin* pin) const
{
  dbNet* dbnet;
  odb::dbModNet* modnet;
  staToDb(net, dbnet, modnet);

  dbNet* pin_dbnet = findFlatDbNet(pin);
  if (dbnet != nullptr) {
    return dbnet->isConnected(pin_dbnet);
  }
  if (modnet != nullptr) {
    return modnet->isConnected(pin_dbnet);
  }

  return false;
}

bool dbNetwork::isConnected(const Net* net1, const Net* net2) const
{
  if (net1 == net2) {
    return true;
  }

  dbNet* flat_net1 = findFlatDbNet(net1);
  dbNet* flat_net2 = findFlatDbNet(net2);
  if (flat_net1 != nullptr && flat_net1 == flat_net2) {
    return true;
  }

  return false;
}

void DbNetConnectedToBTerm::operator()(const Pin* pin)
{
  dbITerm* iterm;
  dbBTerm* bterm;
  dbModITerm* moditerm;
  db_network_->staToDb(pin, iterm, bterm, moditerm);
  if (bterm) {
    bterm_ = bterm;
  }
}

/*
We expect each hierarchical net to have only one flat net
associated with it. When we make a new flat net we update its
modnet.
*/

class ModDbNetAssociation : public PinVisitor
{
 public:
  ModDbNetAssociation(dbNetwork* nwk,
                      utl::Logger* logger,
                      dbNet* new_flat_net,
                      dbNet* orig_flat_net);
  void operator()(const Pin* pin) override;

 private:
  utl::Logger* logger_;
  dbNetwork* db_network_;
  dbNet* new_flat_net_;
  dbNet* orig_flat_net_;
};

ModDbNetAssociation::ModDbNetAssociation(dbNetwork* nwk,
                                         utl::Logger* logger,
                                         dbNet* new_flat_net,
                                         dbNet* orig_flat_net)
    : logger_(logger),
      db_network_(nwk),
      new_flat_net_(new_flat_net),
      orig_flat_net_(orig_flat_net)
{
}

void ModDbNetAssociation::operator()(const Pin* pin)
{
  dbITerm* iterm;
  dbBTerm* bterm;
  dbModITerm* moditerm;
  db_network_->staToDb(pin, iterm, bterm, moditerm);

  if (moditerm) {
    return;
  }

  // Note that we allow the reassociation to be done across all pins
  // In prior versions we assumed the reassocitation was done after
  // setting up the driver so we did not reassociate the driver
  // This minor change lets the net be reassociated from any pin
  // along the net.
  //  if (db_network_->isDriver(pin)) {
  //    return;
  //  }

  dbNet* cur_flat_net = db_network_->flatNet(pin);

  if (cur_flat_net == new_flat_net_) {
    return;
  }
  if (cur_flat_net == orig_flat_net_ || orig_flat_net_ == nullptr) {
    Pin* pin1 = const_cast<Pin*>(pin);
    db_network_->disconnectPin(pin1, db_network_->dbToSta(cur_flat_net));
    db_network_->connectPin(pin1, db_network_->dbToSta(new_flat_net_));
  } else if (cur_flat_net != orig_flat_net_) {
    logger_->error(
        ORD,
        2032,
        "Illegal hierarchy expect only one flat net type per hierarchical net");
  }
}

/*
Go through all the dbPins related to a modnet and update
their modnet association.When we move modnets around
(eg in makeRepeater) we need to update the modnets for
connected dbNets).
*/

class DbModNetAssociation : public PinVisitor
{
 public:
  DbModNetAssociation(dbNetwork* nwk, odb::dbModNet* mod_net);
  void operator()(const Pin* pin) override;

 private:
  dbNetwork* db_network_;
  odb::dbModNet* mod_net_;
  dbModule* owning_module_;
};

DbModNetAssociation::DbModNetAssociation(dbNetwork* nwk, odb::dbModNet* mod_net)
    : db_network_(nwk), mod_net_(mod_net), owning_module_(mod_net->getParent())
{
}

void DbModNetAssociation::operator()(const Pin* pin)
{
  dbITerm* iterm;
  dbBTerm* bterm;
  dbModITerm* moditerm;
  db_network_->staToDb(pin, iterm, bterm, moditerm);

  if (moditerm) {
    return;
  }

  // Note that we allow the reassociation to be done across all pins
  // In prior versions we assumed the reassocitation was done after
  // setting up the driver so we did not need to reassociate the driver
  // This minor change lets the net be reassociated from any pin
  // along the net.
  //  if (db_network_->isDriver(pin)) {
  //    return;
  //  }

  if (iterm) {
    dbInst* owning_inst = iterm->getInst();
    dbModule* parent_module = owning_inst->getModule();
    if (parent_module == owning_module_) {
      odb::dbModNet* existing_mod_net = db_network_->hierNet(pin);
      if (existing_mod_net) {
        if (existing_mod_net == mod_net_) {
          return;
        }
        db_network_->disconnectPin(const_cast<Pin*>(pin),
                                   db_network_->dbToSta(existing_mod_net));
      }
      db_network_->connectPin(const_cast<Pin*>(pin), (Net*) mod_net_);
    }
  }
}

/*
We require each modnet has exactly one flat net associated with it.
It is convenient to move modnets around during hierarchical operations
which may change the underlying flat net associated with a modnet.
This routine does the house keeping needed to preserve the requirement
of one modnet having only one flat net associated with it.
If the flat net is connected to a primary i/o then prefer than name.

*/
void dbNetwork::reassociateHierFlatNet(odb::dbModNet* mod_net,
                                       dbNet* new_flat_net,
                                       dbNet* orig_flat_net)
{
  //
  // make all the pins on the mod net point to the new flat net
  //

  // Catch case when flat net connects to a primary i/o. Rename the
  // modnet to that io.

  DbNetConnectedToBTerm bterm_visitor(this);
  NetSet visited_btermnets(this);
  visitConnectedPins(dbToSta(new_flat_net), bterm_visitor, visited_btermnets);
  dbBTerm* io_bterm = bterm_visitor.bterm();

  if (io_bterm) {
    Pin* ignore;
    dbModule* scope
        = getNetDriverParentModule(dbToSta(new_flat_net), ignore, false);
    (void) ignore;
    if (mod_net->getParent() == scope) {
      std::string new_name = io_bterm->getName();
      mod_net->rename(new_name.c_str());
    }
  }

  // reassociate the mod_net to use the flat net.
  ModDbNetAssociation visitor(this, logger_, new_flat_net, orig_flat_net);
  NetSet visited_nets(this);
  visitConnectedPins(dbToSta(mod_net), visitor, visited_nets);

  //
  // reassociate the flat nets at this level. Get the fanout of the
  // new_flat net and mark as being associated with modnet.
  //

  DbModNetAssociation visitordb(this, mod_net);
  NetSet visited_dbnets(this);
  visitConnectedPins(dbToSta(new_flat_net), visitordb, visited_dbnets);
}

void dbNetwork::reassociateFromDbNetView(dbNet* flat_net,
                                         odb::dbModNet* mod_net)
{
  if (flat_net == nullptr || mod_net == nullptr) {
    return;
  }

  DbModNetAssociation visitordb(this, mod_net);
  NetSet visited_dbnets(this);
  visitConnectedPins(dbToSta(flat_net), visitordb, visited_dbnets);
}

void dbNetwork::reassociatePinConnection(Pin* pin)
{
  // Ensure that a pin is consistently connected to both its hierarchical
  // (odb::dbModNet) and flat (dbNet) representations. This is often needed
  // after complex hierarchical edits.
  odb::dbModNet* mod_net = hierNet(pin);
  if (mod_net) {
    dbNet* flat_net = this->flatNet(pin);
    // Disconnect both flat and hierarchical nets before reconnecting
    // to ensure a clean state.
    disconnectPin(pin);
    connectPin(pin, dbToSta(flat_net), dbToSta(mod_net));
  }
}

void dbNetwork::replaceHierModule(odb::dbModInst* mod_inst, dbModule* module)
{
  (void) mod_inst->swapMaster(module);
}

/*
  Get all the dbModNets and dbNets related to a pin.
 */

class PinModDbNetConnection : public PinVisitor
{
 public:
  PinModDbNetConnection(const dbNetwork* nwk,
                        utl::Logger* logger,
                        const Net* net_to_search);
  void operator()(const Pin* pin) override;
  dbNet* getNet() const { return dbnet_; }

 private:
  dbNet* dbnet_;
  utl::Logger* logger_ = nullptr;
  bool db_net_search_ = false;
  const Net* search_net_;
  const dbNetwork* db_network_;
};

PinModDbNetConnection::PinModDbNetConnection(const dbNetwork* nwk,
                                             utl::Logger* logger,
                                             const Net* net_to_search)
    : db_network_(nwk)
{
  dbnet_ = nullptr;
  logger_ = logger;
  // figure out type of search
  dbNet* db_net;
  odb::dbModNet* db_modnet;
  nwk->staToDb(net_to_search, db_net, db_modnet);
  db_net_search_ = (db_net != nullptr);  // searching from a db_net. else
                                         // modnet.
  search_net_ = net_to_search;
}

void PinModDbNetConnection::operator()(const Pin* pin)
{
  dbITerm* iterm;
  dbBTerm* bterm;
  dbModITerm* moditerm;

  db_network_->staToDb(pin, iterm, bterm, moditerm);

  dbNet* candidate_flat_net = db_network_->flatNet(pin);
  if (candidate_flat_net) {
    //
    //----------------------------
    //
    // Axiom check. We expect each modnet to have only
    // one flat net associated with it. So when doing  a modnet -> dbNet
    // search flag issue an error if we see multiple dbnets
    //
    // When doing a search from a dbNet it is ok to have tons of dbModNets
    // corresponding to many hierarchical connections up and down the hierarchy
    // But when doing a search from a modnet (ie db_net_search==false) we
    // must always have at most one equivalent flat connection.
    //
    //

    if (db_net_search_ == false) {
      dbNet* db_net;
      odb::dbModNet* db_modnet;
      db_network_->staToDb(search_net_, db_net, db_modnet);
      Network* sta_nwk = (Network*) db_network_;
      (void) sta_nwk;
      Instance* owning_instance
          = (const_cast<dbNetwork*>(db_network_))
                ->getOwningInstanceParent(const_cast<Pin*>(pin));
      (void) owning_instance;
      if (dbnet_ != nullptr && dbnet_ != candidate_flat_net) {
        // Dump net connectivity
        logger_->warn(ORD,
                      2031,
                      "Flat net logical inconsistency found. Dump "
                      "connections of the relevant flat nets.");

        logger_->report("\n===============================================");
        logger_->report("Connectivity of the first dbNet '{}'",
                        dbnet_->getName());
        dbnet_->dump(true);

        logger_->report("\n===============================================");
        logger_->report("Connectivity of the second dbNet '{}'",
                        candidate_flat_net->getName());
        candidate_flat_net->dump(true);

        logger_->error(
            ORD,
            2030,
            "Flat net logical inconsistency, badly formed hierarchical "
            "netlist. "
            "Only expect one flat net reachable per pin. Flat Nets are '{}' "
            "and '{}' modnet is '{}'",
            db_network_->pathName(db_network_->dbToSta(dbnet_)),
            db_network_->pathName(db_network_->dbToSta(candidate_flat_net)),
            db_network_->pathName(search_net_));
      }
    }
    dbnet_ = candidate_flat_net;
  }
}

/*
A modnet can have only one equivalent dbNet.
*/
dbNet* dbNetwork::findRelatedDbNet(const odb::dbModNet* net) const
{
  // we pass in the net and decode it for axiom checking.

  // NOTE: PinModDbNetConnection performs sanity check b/w flat and hier nets.
  // So it should not be used for in the middle or connectivity editing that can
  // cause an inconsistent flat and hier nets temporarily (the inconsistency
  // will be resolved eventually at the end of connectivity editing operation).
  PinModDbNetConnection visitor(this, logger_, dbToSta(net));
  NetSet visited_nets(this);
  visitConnectedPins(dbToSta(net), visitor, visited_nets);
  return visitor.getNet();
}

/*
Given a pin, find any modnets in anything is connected
to at this level of hierarchy.
*/

class ModNetForPin : public PinVisitor
{
 public:
  ModNetForPin(dbNetwork* nwk) : db_network_(nwk) {}

  void operator()(const Pin* pin) override;
  odb::dbModNet* modnet() const { return modnet_; }

 private:
  dbNetwork* db_network_;
  odb::dbModNet* modnet_{nullptr};
};

void ModNetForPin::operator()(const Pin* pin)
{
  dbITerm* iterm;
  dbBTerm* bterm;
  dbModITerm* moditerm;
  db_network_->staToDb(pin, iterm, bterm, moditerm);
  if (iterm && iterm->getModNet()) {
    modnet_ = iterm->getModNet();
  } else if (bterm && bterm->getModNet()) {
    modnet_ = bterm->getModNet();
  } else if (moditerm && moditerm->getModNet()) {
    modnet_ = moditerm->getModNet();
  }
}

/*
Given a driving pin, go through all the nets at the hierarhical
level of the driving pin and return the modnet.
*/
odb::dbModNet* dbNetwork::findModNetForPin(const Pin* drvr_pin)
{
  // get all modnets associated with pin at this level
  dbNet* flat_net = flatNet(drvr_pin);
  // got through all the pins reachable from this flat_net at this level
  // of hierarchy and return any modnet.
  ModNetForPin mdfp_visitor(this);
  NetSet visited_nets(this);
  visitConnectedPins(dbToSta(flat_net), mdfp_visitor, visited_nets);
  return mdfp_visitor.modnet();
}

bool dbNetwork::hasHierarchicalElements() const
{
  if (!block()->getModNets().empty()) {
    return true;
  }
  return false;
}

class AccumulateNetFlatLoadPins : public PinVisitor
{
 public:
  AccumulateNetFlatLoadPins(dbNetwork* nwk,
                            Pin* drvr_pin,
                            std::unordered_set<const Pin*>& accumulated_pins)
      : nwk_(nwk), drvr_pin_(drvr_pin), flat_load_pinset_(accumulated_pins)
  {
  }
  void operator()(const Pin* pin) override;

 private:
  dbNetwork* nwk_;
  Pin* drvr_pin_;
  std::unordered_set<const sta::Pin*>& flat_load_pinset_;
};

void AccumulateNetFlatLoadPins::operator()(const Pin* pin)
{
  if (pin != drvr_pin_) {
    dbITerm* iterm = nullptr;
    dbBTerm* bterm = nullptr;
    dbModITerm* moditerm = nullptr;
    // only stash the flat loads on instances (iterms)
    nwk_->staToDb(pin, iterm, bterm, moditerm);
    if (iterm /*|| bterm */) {
      flat_load_pinset_.insert(pin);
    }
  }
}

void dbNetwork::accumulateFlatLoadPinsOnNet(
    Net* net,
    Pin* drvr_pin,
    std::unordered_set<const Pin*>& accumulated_pins)
{
  NetSet visited_nets(this);
  // just the flat load pins
  AccumulateNetFlatLoadPins rp(this, drvr_pin, accumulated_pins);
  visitConnectedPins(net, rp, visited_nets);
}

// Use this API to check if flat & hier connectivities are ok
int dbNetwork::checkAxioms(odb::dbObject* obj) const
{
  int pre_warn_cnt = logger_->getWarningCount();

  // Check with an object if provided
  if (obj != nullptr) {
    checkSanityNetConnectivity(obj);
  } else {
    // Otherwise, check the whole design
    checkSanityModBTerms();
    checkSanityModITerms();
    checkSanityModInstTerms();
    checkSanityUnusedModules();
    checkSanityTermConnectivity();
    checkSanityNetConnectivity();
    checkSanityInstNames();
    checkSanityNetNames();
    checkSanityModuleInsts();
  }

  int post_warn_cnt = logger_->getWarningCount();
  return post_warn_cnt - pre_warn_cnt;
}

// Given a net that may be hierarchical, find the corresponding flat net.
// If the net is already a flat net, it is returned as is.
// If the net is a hierarchical net (odb::dbModNet), find the associated dbNet
// and return it as Net*.
Net* dbNetwork::findFlatNet(const Net* net) const
{
  return dbToSta(findFlatDbNet(net));
}

// Given a net that may be hierarchical, find the corresponding flat dbNet.
// If the net is already a flat net (dbNet), it is returned as is.
// If the net is a hierarchical net (odb::dbModNet), find the associated dbNet.
dbNet* dbNetwork::findFlatDbNet(const Net* net) const
{
  if (!net) {
    return nullptr;
  }
  // Convert net to a flat net, if not already
  dbNet* db_net = nullptr;
  odb::dbModNet* db_mod_net = nullptr;
  staToDb(net, db_net, db_mod_net);

  if (db_net) {
    // If it's already a flat net, return it
    return db_net;
  }

  if (db_mod_net) {
    // If it's a hierarchical net, find the associated dbNet
    // by traversing the hierarchy.
    db_net = db_mod_net->findRelatedNet();
  }
  return db_net;
}

// Find the flat net connected to the pin.
// This function handles both internal instance pins and top-level port pins.
Net* dbNetwork::findFlatNet(const Pin* pin) const
{
  return dbToSta(findFlatDbNet(pin));
}

// Find the flat net (dbNet) connected to the pin in the OpenDB database.
// This function handles both internal instance pins and top-level port pins.
dbNet* dbNetwork::findFlatDbNet(const Pin* pin) const
{
  Net* sta_net = nullptr;
  if (isTopLevelPort(pin)) {
    sta_net = net(term(pin));
  } else {
    sta_net = net(pin);
  }
  return findFlatDbNet(sta_net);
}

odb::dbModInst* dbNetwork::getModInst(Instance* inst) const
{
  if (!inst) {
    return nullptr;
  }

  dbInst* db_inst = nullptr;
  odb::dbModInst* db_mod_inst = nullptr;
  staToDb(inst, db_inst, db_mod_inst);
  return db_mod_inst;
}

bool dbNetwork::hasPort(const Net* net) const
{
  if (!net) {
    return false;
  }

  dbNet* db_net = nullptr;
  odb::dbModNet* db_mod_net = nullptr;
  staToDb(net, db_net, db_mod_net);
  if (db_net) {
    return !db_net->getBTerms().empty();
  }
  if (db_mod_net) {
    return !db_mod_net->getBTerms().empty();
  }
  return false;
}

void dbNetwork::checkSanityModBTerms() const
{
  if (block_ == nullptr) {
    return;
  }

  for (odb::dbModule* module : block_->getModules()) {
    std::set<std::string> bterm_names;
    for (odb::dbModBTerm* bterm : module->getModBTerms()) {
      const std::string bterm_name = bterm->getName();
      if (bterm_names.find(bterm_name) != bterm_names.end()) {
        logger_->error(
            ORD,
            2036,
            "SanityCheck: Duplicate dbModBTerm name '{}' in module '{}'.",
            bterm_name,
            module->getName());
      }
      bterm_names.insert(bterm_name);
    }
  }
}

void sta::dbNetwork::checkSanityModITerms() const
{
  if (block_ == nullptr) {
    return;
  }

  for (odb::dbModInst* mod_inst : block_->getModInsts()) {
    std::set<std::string> iterm_names;
    for (odb::dbModITerm* iterm : mod_inst->getModITerms()) {
      const std::string iterm_name = iterm->getName();
      if (iterm_names.find(iterm_name) != iterm_names.end()) {
        logger_->error(ORD,
                       2037,
                       "SanityCheck: Duplicate dbModITerm name '{}' in module "
                       "instance '{}'.",
                       iterm_name,
                       mod_inst->getName());
      }
      iterm_names.insert(iterm_name);
    }
  }
}

void dbNetwork::checkSanityModuleInsts() const
{
  int inst_count = 0;
  for (odb::dbModule* module : block_->getModules()) {
    if (module->getModInsts().empty() && module->getInsts().empty()) {
      logger_->warn(ORD,
                    2038,
                    "SanityCheck: Module '{}' has no instances.",
                    module->getHierarchicalName());
    }
    inst_count += module->getInsts().size();
  }

  // Check for # of instances in the block and in all modules.
  if (inst_count != block_->getInsts().size()) {
    logger_->error(ORD,
                   2048,
                   "SanityCheck: Total instance count in block '{}' is {} but "
                   "sum of instance counts in all module is {}.",
                   block_->getName(),
                   block_->getInsts().size(),
                   inst_count);
  }
}

static std::vector<std::string> getNameSetDifferences(
    const std::set<std::string>& names1,
    const std::set<std::string>& names2)
{
  std::vector<std::string> differences;
  std::ranges::set_difference(names1, names2, std::back_inserter(differences));
  return differences;
}

void dbNetwork::checkSanityModInstTerms() const
{
  for (odb::dbModInst* mod_inst : block_->getModInsts()) {
    // Compare ModITerms in the instance and ModBTerms in the master.
    // - Note that ModBTerms may have bus port sentinels which are not in
    //   ModITerms, so the count comparison should consider the sentinel
    //   differences.
    // - For example of bus port A[1:0], dbModITerms will have A[0] and A[1],
    //   while dbModBTerms will have A[0], A[1], and A (sentinel).
    //   So the sentinel names need to be excluded from the comparison.
    odb::dbModule* master = mod_inst->getMaster();
    if (master) {
      std::set<std::string> iterm_names;
      for (odb::dbModITerm* iterm : mod_inst->getModITerms()) {
        iterm_names.insert(iterm->getName());
      }

      std::set<std::string> bterm_names;  // e.g., A[0], A[1], A (sentinel)
      std::set<std::string> bterm_sentinel_names;  // e.g., A (sentinel)
      for (odb::dbModBTerm* bterm : master->getModBTerms()) {
        const std::string bterm_name = bterm->getName();
        bterm_names.insert(bterm_name);
        if (bterm->isBusPort()) {
          bterm_sentinel_names.insert(bterm_name);
        }
      }

      std::vector<std::string> iterms_only
          = getNameSetDifferences(iterm_names, bterm_names);
      std::vector<std::string> bterms_only
          = getNameSetDifferences(bterm_names, iterm_names);

      // Remove bus port sentinels from bterms_only as they are expected to not
      // be in iterms.
      std::erase_if(bterms_only, [&](const std::string& name) {
        return bterm_sentinel_names.count(name);
      });

      if (!iterms_only.empty() || !bterms_only.empty()) {
        logger_->warn(
            ORD,
            2042,
            "SanityCheck: Mismatched terms for module instance '{}' and its "
            "master '{}'.",
            mod_inst->getHierarchicalName(),
            master->getName());

        if (!iterms_only.empty()) {
          std::string s;
          for (const std::string& name : iterms_only) {
            s += " " + name;
          }
          logger_->warn(
              ORD, 2053, "  ModITerms in instance but not in master: {}", s);
        }

        if (!bterms_only.empty()) {
          std::string s;
          for (const std::string& name : bterms_only) {
            s += " " + name;
          }
          logger_->warn(
              ORD, 2054, "  ModBTerms in master but not in instance: {}", s);
        }
      }
    }
  }
}

void dbNetwork::checkSanityUnusedModules() const
{
  if (block_ == nullptr) {
    return;
  }

  // 1. Collect all modules from the top block and its children.
  // Unused modules may be placed in child blocks.
  std::vector<odb::dbModule*> all_modules;
  for (odb::dbModule* module : block_->getModules()) {
    all_modules.push_back(module);
  }
  for (odb::dbBlock* child_block : block_->getChildren()) {
    for (odb::dbModule* module : child_block->getModules()) {
      all_modules.push_back(module);
    }
  }

  // 2. Create a set of all instantiated module masters.
  std::set<odb::dbModule*> instantiated_masters;
  for (odb::dbModule* module : all_modules) {
    for (odb::dbModInst* mod_inst : module->getModInsts()) {
      instantiated_masters.insert(mod_inst->getMaster());
    }
  }

  // 3. Iterate through all collected modules to check for usage.
  odb::dbModule* top_module = block_->getTopModule();
  for (odb::dbModule* module : all_modules) {
    // A module is unused if it's not the top module and it's not a master
    // for any module instance.
    if (module != top_module
        && instantiated_masters.find(module) == instantiated_masters.end()) {
      logger_->warn(
          ORD,
          2043,
          "SanityCheck: Module '{}' is defined but never instantiated.",
          module->getName());
    }
  }
}

void dbNetwork::checkSanityTermConnectivity() const
{
  for (odb::dbBTerm* bterm : block_->getBTerms()) {
    if (bterm->getIoType() != dbIoType::INPUT && bterm->getNet() == nullptr) {
      logger_->error(
          ORD,
          2040,
          "SanityCheck: non-input BTerm '{}' is not connected to any net.",
          bterm->getName());
    }
  }

  for (odb::dbInst* inst : block_->getInsts()) {
    for (odb::dbITerm* iterm : inst->getITerms()) {
      if (iterm->getSigType().isSupply()) {
        continue;  // Skip power/ground pins
      }

      if (iterm->getIoType() != dbIoType::OUTPUT
          && iterm->getNet() == nullptr) {
        logger_->error(ORD,
                       2041,
                       "SanityCheck: non-output ITerm '{}/{}' is not connected "
                       "to any net.",
                       inst->getName(),
                       iterm->getMTerm()->getName());
      }
    }
  }
}

void dbNetwork::checkSanityNetConnectivity(odb::dbObject* obj) const
{
  //
  // Check for a specific object if provided
  //
  if (obj != nullptr) {
    // Collect relevant nets from the provided object
    std::set<odb::dbNet*> nets_to_check;
    std::set<odb::dbModNet*> mod_nets_to_check;

    auto const obj_type = obj->getObjectType();
    if (obj_type == odb::dbNetObj) {
      nets_to_check.insert(static_cast<odb::dbNet*>(obj));
    } else if (obj_type == odb::dbModNetObj) {
      mod_nets_to_check.insert(static_cast<odb::dbModNet*>(obj));
    } else if (obj_type == odb::dbInstObj) {
      auto inst = static_cast<odb::dbInst*>(obj);
      for (auto iterm : inst->getITerms()) {
        if (iterm->getNet() != nullptr) {
          nets_to_check.insert(iterm->getNet());
        }
        if (iterm->getModNet() != nullptr) {
          mod_nets_to_check.insert(iterm->getModNet());
        }
      }
    } else if (obj_type == odb::dbModInstObj) {
      auto mod_inst = static_cast<odb::dbModInst*>(obj);
      for (auto mod_iterm : mod_inst->getModITerms()) {
        if (mod_iterm->getModNet() != nullptr) {
          mod_nets_to_check.insert(mod_iterm->getModNet());
        }
      }
    } else if (obj_type == odb::dbITermObj) {
      auto iterm = static_cast<odb::dbITerm*>(obj);
      if (iterm->getNet() != nullptr) {
        nets_to_check.insert(iterm->getNet());
      }
      if (iterm->getModNet() != nullptr) {
        mod_nets_to_check.insert(iterm->getModNet());
      }
    } else if (obj_type == odb::dbBTermObj) {
      auto bterm = static_cast<odb::dbBTerm*>(obj);
      if (bterm->getNet() != nullptr) {
        nets_to_check.insert(bterm->getNet());
      }
      if (bterm->getModNet() != nullptr) {
        mod_nets_to_check.insert(bterm->getModNet());
      }
    } else if (obj_type == odb::dbModITermObj) {
      auto mod_iterm = static_cast<odb::dbModITerm*>(obj);
      if (mod_iterm->getModNet() != nullptr) {
        mod_nets_to_check.insert(mod_iterm->getModNet());
      }
    } else if (obj_type == odb::dbModBTermObj) {
      auto mod_bterm = static_cast<odb::dbModBTerm*>(obj);
      if (mod_bterm->getModNet() != nullptr) {
        mod_nets_to_check.insert(mod_bterm->getModNet());
      }
    }

    // Now run checks on the collected nets.
    for (odb::dbModNet* mod_net : mod_nets_to_check) {
      findRelatedDbNet(mod_net);
    }

    for (odb::dbNet* net_db : nets_to_check) {
      net_db->checkSanity();
    }
    return;
  }

  //
  // Check for all nets in the design
  //

  // Check for hier net and flat net connectivity
  dbSet<odb::dbModNet> mod_nets = block()->getModNets();
  for (odb::dbModNet* mod_net : mod_nets) {
    mod_net->checkSanity();
    findRelatedDbNet(mod_net);
  }

  // Check for incomplete flat net connections
  for (odb::dbNet* net_db : block()->getNets()) {
    net_db->checkSanity();
  }
}

void dbNetwork::checkSanityInstNames() const
{
  if (block_ == nullptr) {
    return;
  }

  // Check for duplicate instance names
  std::set<std::string> inst_names;
  for (odb::dbInst* inst : block_->getInsts()) {
    const std::string inst_name = inst->getName();
    if (inst_names.find(inst_name) != inst_names.end()) {
      logger_->error(ORD,
                     2044,
                     "SanityCheck: Duplicate instance name '{}' in block '{}'.",
                     inst_name,
                     block_->getName());
    }
    inst_names.insert(inst_name);
  }

  // Check for duplicate module instance names
  for (odb::dbModInst* mod_inst : block_->getModInsts()) {
    const std::string mod_inst_name = mod_inst->getHierarchicalName();
    if (inst_names.find(mod_inst_name) != inst_names.end()) {
      logger_->error(ORD,
                     2045,
                     "SanityCheck: Duplicate module instance name '{}' in "
                     "block '{}'.",
                     mod_inst_name,
                     block_->getName());
    }
    inst_names.insert(mod_inst_name);
  }
}

void dbNetwork::checkSanityNetNames() const
{
  if (block_ == nullptr) {
    return;
  }

  // Check for duplicate flat net names
  std::set<std::string> net_names;
  for (odb::dbNet* net : block_->getNets()) {
    const std::string net_name = net->getName();
    if (net_names.find(net_name) != net_names.end()) {
      logger_->error(ORD,
                     2046,
                     "SanityCheck: Duplicate net name '{}' in block '{}'.",
                     net_name,
                     block_->getName());
    }
    net_names.insert(net_name);
  }

  // Check for duplicate module net names within each module
  for (odb::dbModule* module : block_->getModules()) {
    checkSanityModNetNamesInModule(module);
  }

  // Check for name mismatch between flat net and hierchical net
  // - Flat net name should be one of the hierarchical net names
  for (odb::dbNet* net : block_->getNets()) {
    std::set<odb::dbModNet*> mod_nets;
    if (net->findRelatedModNets(mod_nets) && mod_nets.empty() == false) {
      bool name_match = false;
      for (odb::dbModNet* mod_net : mod_nets) {
        if (net->getName() == mod_net->getHierarchicalName()) {
          name_match = true;
          break;
        }
      }

      if (name_match) {
        continue;
      }

      // Found name mismatch b/w flat net and hierarchical nets
      logger_->warn(ORD,
                    2050,
                    "SanityCheck: Flat net name '{}' does not match any of "
                    "its hierarchical net names.",
                    net->getName());
      for (odb::dbModNet* mod_net : mod_nets) {
        logger_->warn(ORD,
                      2055,
                      "  hierarchical net: {}",
                      mod_net->getHierarchicalName());
      }
    }
  }
}

void dbNetwork::checkSanityModNetNamesInModule(odb::dbModule* module) const
{
  // If duplicate ModNet name found, log details of both existing and new nets
  std::map<std::string, odb::dbModNet*> mod_net_map;
  for (odb::dbModNet* mod_net : module->getModNets()) {
    const std::string mod_net_name = mod_net->getName();
    auto it = mod_net_map.find(mod_net_name);
    if (it != mod_net_map.end()) {
      odb::dbModNet* existing_net = it->second;
      auto get_full_name = [](odb::dbModNet* net) {
        if (!net) {
          return std::string("null");
        }
        return fmt::format(
            "{}/{}",
            net->getParent() ? net->getParent()->getName() : "null",
            net->getName());
      };

      auto get_terminals_str = [&](odb::dbModNet* net) {
        std::string terminals;
        for (dbITerm* iterm : net->getITerms()) {
          terminals += fmt::format(" ITerm:{}({})",
                                   iterm->getInst()->getName(),
                                   iterm->getMTerm()->getName());
        }
        for (dbModITerm* moditerm : net->getModITerms()) {
          terminals
              += fmt::format(" ModITerm:{}", moditerm->getParent()->getName());
        }
        for (dbBTerm* bterm : net->getBTerms()) {
          terminals += fmt::format(" BTerm:{}", bterm->getName());
        }
        for (dbModBTerm* modbterm : net->getModBTerms()) {
          terminals += fmt::format(" ModBTerm:{}", modbterm->getName());
        }
        return terminals;
      };

      logger_->info(ORD,
                    2051,
                    " -> Existing net: {}, id: {}, terminals:{}",
                    get_full_name(existing_net),
                    existing_net->getId(),
                    get_terminals_str(existing_net));
      logger_->info(ORD,
                    2052,
                    " -> New net: {}, id: {}, terminals:{}",
                    get_full_name(mod_net),
                    mod_net->getId(),
                    get_terminals_str(mod_net));
      logger_->error(
          ORD,
          2047,
          "SanityCheck: Duplicate module net name '{}' in module '{}'.",
          mod_net_name,
          module->getName());
    }
    mod_net_map[mod_net_name] = mod_net;
  }
}

void dbNetwork::checkSanityNetDrvrPinMapConsistency() const
{
  if (block_ == nullptr) {
    logger_->error(ORD, 2000, "No block found.");
    return;
  }

  // For each cache element
  for (const auto& [net, cached_drivers_ptr] : net_drvr_pin_map_) {
    dbNet* dbnet = nullptr;
    odb::dbModNet* modnet = nullptr;
    staToDb(net, dbnet, modnet);

    if (dbnet == nullptr && modnet == nullptr) {
      logger_->warn(ORD,
                    2009,
                    "Found net {} in cache that is not in the netlist.",
                    pathName(net));
      continue;
    }

    // Find drivers from the netlist for the current net
    std::vector<odb::dbObject*> drivers;
    if (dbnet) {
      odb::dbUtil::findITermDrivers(dbnet, drivers);
      odb::dbUtil::findBTermDrivers(dbnet, drivers);
    } else if (modnet) {
      odb::dbUtil::findITermDrivers(modnet, drivers);
      odb::dbUtil::findBTermDrivers(modnet, drivers);
      odb::dbUtil::findModITermDrivers(modnet, drivers);

      // Also, find drivers from the related flat net
      dbNet* related_dbnet = findRelatedDbNet(modnet);
      if (related_dbnet) {
        odb::dbUtil::findITermDrivers(related_dbnet, drivers);
        odb::dbUtil::findBTermDrivers(related_dbnet, drivers);
      }

      // Remove duplicates
      utl::sort_and_unique(drivers);
    }

    // Convert to PinSet
    PinSet netlist_drivers(this);
    for (auto driver : drivers) {
      Pin* pin = nullptr;
      switch (driver->getObjectType()) {
        case odb::dbITermObj:
          pin = dbToSta(static_cast<odb::dbITerm*>(driver));
          break;
        case odb::dbBTermObj:
          pin = dbToSta(static_cast<odb::dbBTerm*>(driver));
          break;
        case odb::dbModITermObj:
          // Skip hierarchical pin.
          break;
        default:
          // Should not be here
          break;
      }
      if (pin != nullptr) {
        netlist_drivers.insert(pin);
      }
    }

    // Compare netlist drivers with cached drivers
    bool consistent = true;
    if (cached_drivers_ptr == nullptr
        || netlist_drivers.size() != cached_drivers_ptr->size()) {
      consistent = false;
    } else {
      for (const Pin* pin : netlist_drivers) {
        if (cached_drivers_ptr->find(pin) == cached_drivers_ptr->end()) {
          consistent = false;
          break;
        }
      }
    }

    // Report inconsistent point details
    if (consistent == false) {
      logger_->warn(ORD, 2006, "Inconsistency found for net {}", pathName(net));
      logger_->report("  Netlist drivers:");
      for (const Pin* pin : netlist_drivers) {
        const PinInfo pin_info = getPinInfo(this, pin);
        logger_->report("    - {} (type: {}, id: {})",
                        pin_info.name,
                        pin_info.type_name,
                        pin_info.id);
      }
      logger_->report("  Cached drivers:");
      if (cached_drivers_ptr) {
        for (const Pin* pin : *cached_drivers_ptr) {
          const PinInfo pin_info = getPinInfo(this, pin);
          logger_->report("    - {} (type: {}, id: {})",
                          pin_info.name,
                          pin_info.type_name,
                          pin_info.id);
        }
      }
    }
  }
}

void dbNetwork::dumpNetDrvrPinMap() const
{
  const auto& net_drvr_pin_map = net_drvr_pin_map_;
  logger_->report("--------------------------------------------------");
  logger_->report("Dumping net_drvr_pin_map_ cache (size: {})",
                  net_drvr_pin_map.size());

  for (auto const& [net, pin_set] : net_drvr_pin_map) {
    // Get the underlying dbObject for the net to report its type and ID.
    const dbObject* net_obj
        = reinterpret_cast<const dbObject*>(const_cast<Net*>(net));
    logger_->report("Net: {} {}({}, {:p})",
                    pathName(net),
                    net_obj->getTypeName(),
                    net_obj->getId(),
                    (void*) net_obj);
    if (pin_set == nullptr) {
      logger_->report("  Drivers: null pin set");
      continue;
    }
    if (pin_set->empty()) {
      logger_->report("  Drivers: empty pin set");
      continue;
    }
    for (const Pin* pin : *pin_set) {
      const PinInfo pin_info = getPinInfo(this, pin);
      logger_->report("  - {} {}({}, {:p})",
                      pin_info.name,
                      pin_info.type_name,
                      pin_info.id,
                      pin_info.addr);
    }
  }
  logger_->report("--------------------------------------------------");
}

PinSet* dbNetwork::drivers(const Pin* pin)
{
  if (pin == nullptr) {
    return nullptr;
  }

  dbITerm* iterm;
  dbBTerm* bterm;
  dbModITerm* moditerm;
  staToDb(pin, iterm, bterm, moditerm);

  // IMPORTANT! Check the dbNet first to find the correct driver

  if (iterm) {
    if (dbNet* db_net = iterm->getNet()) {
      Net* net = dbToSta(db_net);
      return drivers(net);
    }
  }

  if (bterm) {
    if (dbNet* db_net = bterm->getNet()) {
      Net* net = dbToSta(db_net);
      return drivers(net);
    }
  }

  // If pin is connected to a odb::dbModNet only, use it to find the driver

  // pin is dbModITerm or pin is connected to odb::dbModNet
  Net* net = this->net(pin);  // net() returns a odb::dbModNet
  if (net) {
    return drivers(net);
  }

  return nullptr;
}

PinSet* dbNetwork::drivers(const Net* net)
{
  if (net == nullptr) {
    return nullptr;
  }

  // Get or create drvrs pin set
  auto drvrs_entry = net_drvr_pin_map_.find(net);
  if (drvrs_entry == net_drvr_pin_map_.end()) {
    std::tie(drvrs_entry, std::ignore)
        = net_drvr_pin_map_.insert({net, new PinSet(this)});
  }

  PinSet* drvrs = drvrs_entry->second;

  // Insert the driver pin of the net
  dbNet* db_net = findFlatDbNet(net);
  if (db_net == nullptr) {
    return drvrs;
  }

  dbObject* drvr = db_net->getFirstDriverTerm();
  Pin* drvr_pin = dbToSta(drvr);
  if (drvr_pin) {
    drvrs->insert(drvr_pin);
  }
  return drvrs;
}

void dbNetwork::removeDriverFromCache(const Net* net)
{
  auto entry = net_drvr_pin_map_.find(net);
  if (entry != net_drvr_pin_map_.end()) {
    delete entry->second;
    net_drvr_pin_map_.erase(entry);
  }
}

void dbNetwork::removeDriverFromCache(const Net* net, const Pin* drvr)
{
  auto entry = net_drvr_pin_map_.find(net);
  if (entry != net_drvr_pin_map_.end()) {
    entry->second->erase(drvr);
  }
}

bool dbNetwork::isPGSupply(dbITerm* iterm) const
{
  if (iterm->getSigType().isSupply()) {
    return true;
  }

  return isPGSupply(iterm->getNet());
}

bool dbNetwork::isPGSupply(dbBTerm* bterm) const
{
  if (bterm->getSigType().isSupply()) {
    return true;
  }

  return isPGSupply(bterm->getNet());
}

bool dbNetwork::isPGSupply(dbNet* net) const
{
  if (net == nullptr) {
    return false;
  }

  return net->isSpecial() && net->getSigType().isSupply();
}

Net* dbNetwork::highestNetAbove(Net* net) const
{
  if (net == nullptr) {
    return nullptr;
  }

  dbNet* dbnet;
  odb::dbModNet* modnet;
  staToDb(net, dbnet, modnet);

  if (dbnet) {
    // If a flat net, return it.
    // - We should not return the highest modnet related to the flat net.
    // - Otherwise, it breaks estimate_parasitics function.
    //   . est module uses flat nets for parasitic estimation
    //   . It has (highestNetAbove(flat_net) != flat_net) comparison.
    //   . If highestNetAbove(flat_net) returns the highest modnet, it
    //     changes the estimate_parasitics behavior.
    return net;
  }

  if (modnet) {
    if (dbNet* related_dbnet = modnet->findRelatedNet()) {
      if (odb::dbModNet* highest_modnet
          = related_dbnet->findModNetInHighestHier()) {
        return dbToSta(highest_modnet);  // Found the highest modnet
      }
    }
  }

  return net;
}

}  // namespace sta
