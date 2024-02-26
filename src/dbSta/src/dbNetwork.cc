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

// dbSta, OpenSTA on OpenDB

#include "db_sta/dbNetwork.hh"

#include "odb/db.h"
#include "odb/dbId.h"
#include "sta/Liberty.hh"
#include "sta/PatternMatch.hh"
#include "sta/PortDirection.hh"
#include "utl/Logger.h"

//#define DEBUG_DBNW
//#define DEBUG_BUS
//#define DEBUG_HNAMES

namespace sta {

using utl::ORD;

using odb::dbBlock;
using odb::dbBoolProperty;
using odb::dbBTerm;
using odb::dbBTermObj;
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
using odb::dbModNet;
using odb::dbModule;
using odb::dbMTerm;
using odb::dbNet;
using odb::dbObject;
using odb::dbObjectType;
using odb::dbPlacementStatus;
using odb::dbSet;
using odb::dbSigType;

// Turns out OR use full port names (instance name + port name)
// So we have this helper to build the port names.

std::string makePinName(std::string inst_name, std::string port_name)
{
  std::string i_name = inst_name;
  size_t last_idx = i_name.find_last_of("/");
  if (last_idx != string::npos)
    i_name = i_name.substr(last_idx + 1);
  std::string p_name = port_name;
  last_idx = p_name.find_last_of("/");
  if (last_idx != string::npos)
    p_name = p_name.substr(last_idx + 1);
  return (i_name + "/" + p_name);
}

// TODO: move to StringUtil
char* tmpStringCopy(const char* str)
{
  char* tmp = makeTmpString(strlen(str) + 1);
  strcpy(tmp, str);
  return tmp;
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

////////////////////////////////////////////////////////////////

class DbInstanceChildIterator : public InstanceChildIterator
{
 public:
  DbInstanceChildIterator(const Instance* instance, const dbNetwork* network);
  bool hasNext() override;
  Instance* next() override;

 private:
  const dbNetwork* network_;
  dbModule* module_;
  std::vector<dbModule*> child_modules_;
  bool top_;
  dbSet<dbInst>::iterator dbinst_iter_;
  dbSet<dbInst>::iterator dbinst_end_;
  dbSet<dbModInst>::iterator modinst_iter_;
  dbSet<dbModInst>::iterator modinst_end_;
  std::vector<dbModule*> candidate_modules_;
};

DbInstanceChildIterator::DbInstanceChildIterator(const Instance* instance,
                                                 const dbNetwork* network)
    : network_(network)
{
  dbBlock* block = network->block();
  module_ = block->getTopModule();
  modinst_iter_ = ((dbModule*) module_)->getModInsts().begin();
  modinst_end_ = modinst_iter_;
  dbinst_iter_ = ((dbModule*) module_)->getInsts().begin();
  dbinst_end_ = dbinst_iter_;

  if (instance == network->topInstance() && block) {
    module_ = block->getTopModule();
    top_ = true;
    modinst_iter_ = module_->getModInsts().begin();
    modinst_end_ = module_->getModInsts().end();
    dbinst_iter_ = module_->getInsts().begin();
    dbinst_end_ = module_->getInsts().end();
#ifdef DEBUG_DBNW
    printf("(top) Child iterator for instance %s\n", network_->name(instance));
    printf("Number of db instances %u\n", module_->getDbInstCount());
    printf("Number of module instances %u\n", module_->getModInstCount());
#endif
  } else {
    top_ = false;
    // need to get module for instance
    dbInst* db_inst = nullptr;
    dbModInst* mod_inst = nullptr;
    network->staToDb(instance, db_inst, mod_inst);
    if (mod_inst) {
      module_ = mod_inst->getMaster();
      modinst_iter_ = module_->getModInsts().begin();
      modinst_end_ = module_->getModInsts().end();
      dbinst_iter_ = module_->getInsts().begin();
      dbinst_end_ = module_->getInsts().end();
#ifdef DEBUG_DBNW
      printf("(non-top/leaf)Child iterator for instance %s\n",
             network_->name(instance));
      printf("Number of db instances %u\n", module_->getDbInstCount());
      printf("Number of module instances %u\n", module_->getModInstCount());
#endif
    }
  }
}

bool DbInstanceChildIterator::hasNext()
{
  bool nomore
      = ((dbinst_iter_ == dbinst_end_) && (modinst_iter_ == modinst_end_))
            ? true
            : false;
  return (!nomore);
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
  Net* next_ = nullptr;
};

DbInstanceNetIterator::DbInstanceNetIterator(const Instance* instance,
                                             const dbNetwork* network)
    : network_(network)
{
  if (instance == network->topInstance()) {
    dbSet<dbNet> nets = network->block()->getNets();
    iter_ = nets.begin();
    end_ = nets.end();
  }
}

bool DbInstanceNetIterator::hasNext()
{
  while (iter_ != end_) {
    dbNet* net = *iter_;
    if (!net->getSigType().isSupply() || !net->isSpecial()) {
      next_ = network_->dbToSta(*iter_);
      ++iter_;
      return true;
    }
    iter_++;
  }
  return false;
}

Net* DbInstanceNetIterator::next()
{
  return next_;
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
  // pins on a module instance
  dbSet<dbModITerm>::iterator mi_itr_;
  dbSet<dbModITerm>::iterator mi_itr_end_;

  Pin* next_;
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
    bitr_ = block->getBTerms().begin();
    bitr_end_ = block->getBTerms().end();
  } else {
    network_->staToDb(inst, db_inst_, mod_inst_);
    if (db_inst_) {
      iitr_ = db_inst_->getITerms().begin();
      iitr_end_ = db_inst_->getITerms().end();
    } else if (mod_inst_) {
      mi_itr_ = mod_inst_->getModITerms().begin();
      mi_itr_end_ = mod_inst_->getModITerms().end();
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

  if (mi_itr_ != mi_itr_end_) {
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

//
// TODO:
//
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
  Pin* next_;
};

DbNetPinIterator::DbNetPinIterator(const Net* net,
                                   const dbNetwork* /* network */)
{
  dbNet* dnet = reinterpret_cast<dbNet*>(const_cast<Net*>(net));
  iitr_ = dnet->getITerms().begin();
  iitr_end_ = dnet->getITerms().end();
  next_ = nullptr;
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
  return false;
}

Pin* DbNetPinIterator::next()
{
  return next_;
}

////////////////////////////////////////////////////////////////

// A term is either a bterm or a modbterm
//
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
  } else {
    dbSet<dbBTerm> terms;
    iter_ = terms.begin();
    end_ = terms.end();
  }
}

bool DbNetTermIterator::hasNext()
{
  if (mod_iter_ != mod_end_ || iter_ != end_)
    return true;
  return false;
}

Term* DbNetTermIterator::next()
{
  if (iter_ != end_) {
    dbBTerm* bterm = *iter_;
    iter_++;
    return network_->dbToStaTerm(bterm);
  } else if (mod_iter_ != mod_end_) {
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
  return ConcreteNetwork::id(port);
}

////////////////////////////////////////////////////////////////

ObjectId dbNetwork::id(const Instance* instance) const
{
  if (instance == top_instance_) {
    return 0;
  }
  return staToDb(instance)->getId();
}

const char* dbNetwork::name(const Instance* instance) const
{
  if (instance == top_instance_) {
    return tmpStringCopy(block_->getConstName());
  }
  dbInst* db_inst = nullptr;
  dbModInst* mod_inst = nullptr;
  const char* ret = nullptr;
  staToDb(instance, db_inst, mod_inst);
  if (db_inst) {
    ret = tmpStringCopy(db_inst->getConstName());
  } else if (mod_inst) {
    ret = tmpStringCopy(mod_inst->getName());
  }
  return ret;
}

class dbModulePortIterator : public CellPortIterator
{
 public:
  explicit dbModulePortIterator(const dbModule* cell, const dbBlock* block);
  ~dbModulePortIterator();
  virtual bool hasNext();
  virtual Port* next();

 private:
  dbSet<dbModBTerm>::iterator iter_;
  const dbModule* module_;
  const dbBlock* block_;
};

dbModulePortIterator::~dbModulePortIterator()
{
}

dbModulePortIterator::dbModulePortIterator(const dbModule* cell,
                                           const dbBlock* block)
{
  module_ = cell;
  block_ = block;
  iter_ = (const_cast<dbModule*>(cell))->getModBTerms().begin();
}

bool dbModulePortIterator::hasNext()
{
  if (iter_ == (const_cast<dbModule*>(module_))->getModBTerms().end())
    return false;
  return true;
}

Port* dbModulePortIterator::next()
{
  if (iter_ == (const_cast<dbModule*>(module_))->getModBTerms().end())
    return nullptr;
  dbModBTerm* modbterm = *iter_;
  Port* ret = reinterpret_cast<Port*>(modbterm);
  // advance to next
  iter_++;
  return ret;
}

bool dbNetwork::hasMembers(const Port* port) const
{
  const ConcretePort* cport = reinterpret_cast<const ConcretePort*>(port);
  return cport->hasMembers();
}

CellPortIterator* dbNetwork::portIterator(const Cell* cell) const
{
  // filters: is this something we have a db object for...
  const char* cell_name = name(cell);
  if (cell == top_cell_) {
    return ConcreteNetwork::portIterator(cell);
  } else {
    // A liberty cell. Can remove this.
    // This is to show default behaviour
    LibertyCell* lib_cell = nullptr;
    LibertyLibraryIterator* lib_iter = libertyLibraryIterator();
    while (lib_iter->hasNext()) {
      LibertyLibrary* lib = lib_iter->next();
      lib_cell = lib->findLibertyCell(cell_name);
    }
    // a liberty cell.
    if (lib_cell) {
      return ConcreteNetwork::portIterator(cell);
    }
    // a regular cell (note that we have built a fake library
    // for the module instances and each cell has an sta_port reference).
    return ConcreteNetwork::portIterator(cell);
  }
  return nullptr;
}

/*
  The STA relies on a cell/library hierarchy to get ports/cells
  etc. So we build in that association using the concrete network
  infra structure. Same is done for dbinstances in original code
  so this is hopefully right ! (feels weird decorating objects
  with void*, must be a better way of doing this).
 */
void dbNetwork::makeVerilogCell(Library* library, dbModInst* mod_inst)
{
  dbModule* master = mod_inst->getMaster();
  if (master->getOwner() != block_) {
#ifdef DEBUG_DBNW
    printf("Badly formed master\n");
#endif
  }

#ifdef DEBUG_BUS
  printf("Making verilog cell for %s\n", master->getName());
  printf("Terms (modbterms) on master\n");
  for (auto i = master->getModBTerms()) {
    printf("ModBTerm: %s\n", (*i)->getName());
  }
#endif

  Cell* local_cell
      = ConcreteNetwork::makeCell(library, master->getName(), false, nullptr);
  master->staSetCell((void*) (local_cell));

  // make the ports.
  // Handle bus ports
  std::map<std::string, dbModBTerm*> name2modbterm;

  for (dbSet<dbModBTerm>::iterator modbterm_iter
       = master->getModBTerms().begin();
       modbterm_iter != master->getModBTerms().end();
       modbterm_iter++) {
    dbModBTerm* modbterm = *modbterm_iter;
    const char* port_name = modbterm->getName();
    Port* port = ConcreteNetwork::makePort(local_cell, port_name);
    PortDirection* dir = dbToSta(modbterm->getSigType(), modbterm->getIoType());
    setDirection(port, dir);
    name2modbterm[std::string(port_name)] = modbterm;
  }

  // make the bus ports. This will generate the bus bits.
  groupBusPorts(local_cell, [=](const char* port_name) {
    return portMsbFirst(port_name, master->getName());
  });

  CellPortIterator* ccport_iter = portIterator(local_cell);
  while (ccport_iter->hasNext()) {
    Port* cport = ccport_iter->next();
    const ConcretePort* ccport = reinterpret_cast<const ConcretePort*>(cport);
    std::string port_name = ccport->name();

    if (ccport->isBus()) {
#ifdef DEBUG_BUS
      printf("Bus %s from %d to %d\n",
             ccport->name(),
             ccport->fromIndex(),
             ccport->toIndex());
      printf("\tMembers\n");
#endif
      PortMemberIterator* pmi = memberIterator(cport);
      while (pmi->hasNext()) {
        Port* bitport = pmi->next();
        const ConcretePort* cbitport
            = reinterpret_cast<const ConcretePort*>(bitport);
#ifdef DEBUG_BUS
        printf("\t\tBus bit %s \n", cbitport->name());
#endif
        dbModBTerm* modbterm = name2modbterm[std::string(cbitport->name())];
        modbterm->staSetPort(bitport);
      }
    } else if (ccport->isBundle()) {
#ifdef DEBUG_BUS
      printf("Bundle %s\n",
             reinterpret_cast<const ConcretePort*>(cport)->name());
#endif
    } else if (ccport->isBusBit()) {
#ifdef DEBUG_BUS
      printf("Busbit %s\n",
             reinterpret_cast<const ConcretePort*>(cport)->name());
#endif
    } else {
#ifdef DEBUG_BUS
      printf("Regular port %s\n", ccport->name());
#endif
      dbModBTerm* modbterm = name2modbterm[port_name];
      modbterm->staSetPort(cport);
    }
  }
}

Cell* dbNetwork::cell(const Port* port) const
{
  const ConcretePort* cport = reinterpret_cast<const ConcretePort*>(port);
  return cport->cell();
}

Cell* dbNetwork::cell(const Instance* instance) const
{
#ifdef DEBUG_DBNWK
  printf("debug %d Getting cell for instance %s\n", debug, name(instance));
#endif
  if (instance == top_instance_) {
    return reinterpret_cast<Cell*>(top_cell_);
  }

  dbInst* db_inst;
  dbModInst* mod_inst;
  staToDb(instance, db_inst, mod_inst);
  if (db_inst) {
    dbMaster* master = db_inst->getMaster();
    return dbToSta(master);
  } else if (mod_inst) {
    dbModule* master = mod_inst->getMaster();
#ifdef DEBUG_DBNWK
    printf("Mod inst Master is %s\n", master->getName());
#endif
    // look up the cell in the verilog library.
    return dbToSta(master);
  }
  // no traversal of the hierarchy this way; we would have to split
  // Cell into dbMaster and dbModule otherwise.  When we have full
  // odb hierarchy this can be revisited.
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
    if (auto parent_inst = parent_module->getModInst()) {
      return dbToSta(parent_inst);
    }
  }

  return top_instance_;
}

bool dbNetwork::isLeaf(const Instance* instance) const
{
  if (instance == top_instance_) {
#ifdef DEBUG_DBNWK
    printf("Top instance, not a leaf\n");
#endif
    return false;
  }
  dbMaster* db_master;
  dbModule* db_module;
  Cell* cur_cell = cell(instance);
  staToDb(cur_cell, db_master, db_module);
  if (db_module) {
#ifdef DEBUG_DBNWK
    printf("Instance %s is hierarchical\n", name(instance));
#endif
    return false;
  }
#ifdef DEBUG_DBNWK
  printf("Instance %s is leaf\n", name(instance));
#endif
  return true;
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

Pin* dbNetwork::findPin(const Instance* instance, const char* port_name) const
{
#ifdef DEBUG_DBNW
  printf("Seeking pin for %s\n", port_name);
#endif
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
    dbModITerm* miterm = nullptr;
    // removed indexing by port ix (which is most efficient)
    // This is *really really* stupid and done solely to shut up OR morons
    std::string instance_name = mod_inst->getName();
    std::string p_name = port_name;
    std::string full_port_name = makePinName(instance_name, p_name);
    if (mod_inst->findModITerm(full_port_name.c_str(), miterm))
      return dbToSta(miterm);
  }
  return nullptr;  // no pins on dbModInst in odb currently
}

Pin* dbNetwork::findPin(const Instance* instance, const Port* port) const
{
  const char* port_name = name(port);
#ifdef DEBUG_DBNWK
  static int debug;
  debug++;
  printf("D%d finding Pin for Port name %s\n", debug, port_name);
#endif
  return findPin(instance, port_name);
}

Net* dbNetwork::findNet(const Instance* instance, const char* net_name) const
{
  if (instance == top_instance_) {
    dbNet* dnet = block_->findNet(net_name);
    return dbToSta(dnet);
  }
  std::string flat_net_name = pathName(instance);
  flat_net_name += pathDivider() + std::string(net_name);
  dbNet* dnet = block_->findNet(flat_net_name.c_str());
  return dbToSta(dnet);
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

////////////////////////////////////////////////////////////////

ObjectId dbNetwork::id(const Pin* pin) const
{
  dbITerm* iterm;
  dbBTerm* bterm;
  dbModITerm* moditerm;
  dbModBTerm* modbterm;

  staToDb(pin, iterm, bterm, moditerm, modbterm);

  if (iterm) {
    return iterm->getId() + iterm->getBlock()->getBTerms().size();
  }
  return bterm->getId();
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
#ifdef DEBUG_DBNWK
    printf(
        "Unsupported modbter to inst conversion. Ports don't have instances\n");
#endif
  }

  return nullptr;
}

// pin -> net
// handle modinst nets driving binst pins
//

Net* dbNetwork::hnet(const Pin* pin) const
{
  dbITerm* iterm;
  dbBTerm* bterm;
  dbModITerm* moditerm;
  dbModBTerm* modbterm;

  staToDb(pin, iterm, bterm, moditerm, modbterm);
  if (iterm) {
    dbNet* dnet = iterm->getNet();
    dbModNet* mnet = iterm->getModNet();

    // It is possible when writing out a hierarchical network
    // that we have both a mod net and a db net.
    // In the case of writing out a hierachical network we always
    // choose the mnet.
    // in regular case (everything else !) we choose the dnet
    //--Check with Matt if this seems to make sense.

    if (dnet && mnet) {
      return dbToSta(mnet);
    }
    if (mnet)
      return dbToSta(mnet);
    if (dnet)
      return dbToSta(dnet);
  }

  if (moditerm) {
    dbModNet* dnet = moditerm->getNet();
    return dbToSta(dnet);
  }
  if (modbterm) {
    dbModNet* dnet = modbterm->getNet();
    return dbToSta(dnet);
  }

  return nullptr;
}

// pin -> net

Net* dbNetwork::net(const Pin* pin) const
{
  dbITerm* iterm;
  dbBTerm* bterm;
  dbModITerm* moditerm;
  dbModBTerm* modbterm;

#ifdef DEBUG_DBNW
  printf("Getting net for pin %s\n", name(pin));
#endif
  staToDb(pin, iterm, bterm, moditerm, modbterm);
  if (iterm) {
    dbNet* dnet = iterm->getNet();
    dbModNet* mnet = iterm->getModNet();

    // It is possible when writing out a hierarchical network
    // that we have both a mod net and a dbinst net.
    // In the case of writing out a hierachical network we always
    // choose the mnet.

    if (dnet && mnet) {
      return dbToSta(mnet);
    }
    if (mnet)
      return dbToSta(mnet);
    if (dnet)
      return dbToSta(dnet);
  }

  if (bterm) {
    dbNet* dnet = bterm->getNet();
    dbModNet* mnet = bterm->getModNet();

    if (dnet && mnet) {
      return dbToSta(mnet);
    }
    if (mnet)
      return dbToSta(mnet);
    if (dnet)
      return dbToSta(dnet);
  }

  if (moditerm) {
    dbModNet* mnet = moditerm->getNet();
    return dbToSta(mnet);
  }
  if (modbterm) {
    dbModNet* mnet = modbterm->getNet();
    return dbToSta(mnet);
  }

  return nullptr;
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

  // probably wrong: term should be modbterm for this moditerm
  if (moditerm) {
    return dbToStaTerm(moditerm);
  }
  // ok
  if (modbterm)
    return dbToStaTerm(modbterm);

  return nullptr;
}

void dbNetwork::findInstPinsHierMatching(const Instance* instance,
                                         const PatternMatch* pattern,
                                         // Return value.
                                         PinSeq& matches) const
{
  const char* inst_name = name(instance);
  InstancePinIterator* pin_iter = pinIterator(instance);
  while (pin_iter->hasNext()) {
    const Pin* pin = pin_iter->next();
    // figure out type of instance, then get the port
    const char* port_name = name(port(pin));
    string pin_name;
    stringPrint(pin_name, "%s%c%s", inst_name, divider_, port_name);
    if (pattern->match(pin_name.c_str()))
      matches.push_back(pin);
  }
  delete pin_iter;
}

Port* dbNetwork::port(const Pin* pin) const
{
  dbITerm* iterm;
  dbBTerm* bterm;
  dbModITerm* moditerm;
  dbModBTerm* modbterm;
  Port* ret = nullptr;
  static int debug;
  debug++;
  // Will return the bterm for a top level pin
  staToDb(pin, iterm, bterm, moditerm, modbterm);

  if (iterm) {
    dbMTerm* mterm = iterm->getMTerm();
    ret = dbToSta(mterm);
  }

  else if (bterm) {
    const char* port_name = bterm->getConstName();
    ret = findPort(top_cell_, port_name);
  }

  else if (moditerm) {
    std::string port_name_str = moditerm->getName();
    size_t last_idx = port_name_str.find_last_of('/');
    if (last_idx != string::npos) {
      port_name_str = port_name_str.substr(last_idx + 1);
    }
    // how to look up a bus pin ???
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
  dbBTerm* bterm = nullptr;
  dbModBTerm* modbterm = nullptr;
  dbMTerm* mterm = nullptr;
  // port -> bterm or modbterm
  staToDb(port, bterm, mterm, modbterm);
  if (bterm) {
    PortDirection* dir = dbToSta(bterm->getSigType(), bterm->getIoType());
    return dir;
  } else if (modbterm) {
    PortDirection* dir = dbToSta(modbterm->getSigType(), modbterm->getIoType());
    return dir;
  } else {
    // fall through
    const ConcretePort* cport = reinterpret_cast<const ConcretePort*>(port);
    return cport->direction();
  }
  return PortDirection::unknown();
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
  // pin -> iterm or moditerm

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
    PortDirection* dir = dbToSta(moditerm->getSigType(), moditerm->getIoType());
    return dir;
  }
  return PortDirection::unknown();
}

VertexId dbNetwork::vertexId(const Pin* pin) const
{
  dbITerm* iterm;
  dbBTerm* bterm;
  dbModITerm* miterm;
  dbModBTerm* mbterm;
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
  dbITerm* iterm;
  dbBTerm* bterm;
  dbModITerm* moditerm;
  dbModBTerm* modbterm;

  staToDb(pin, iterm, bterm, moditerm, modbterm);
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
  dbITerm* iterm;
  dbBTerm* bterm;
  dbModITerm* moditerm;
  dbModBTerm* modbterm;
  staToDb(pin, iterm, bterm, moditerm, modbterm);
  if (iterm) {
    int x, y;
    if (iterm->getAvgXY(&x, &y)) {
      return Point(x, y);
    }
    dbInst* inst = iterm->getInst();
    inst->getOrigin();
    return Point(x, y);
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
  dbITerm* iterm;
  dbBTerm* bterm;
  dbModITerm* moditerm;
  dbModBTerm* modbterm;

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
  return staToDb(net)->getId();
}

const char* dbNetwork::name(const Net* net) const
{
  dbModNet* modnet = nullptr;
  dbNet* dnet = nullptr;

  staToDb(net, dnet, modnet);
  if (dnet) {
    const char* name = dnet->getConstName();
    return tmpStringCopy(name);
  } else if (modnet) {
    std::string net_name = modnet->getName();
    return net_name.c_str();
  }
  return nullptr;
}

Instance* dbNetwork::instance(const Net*) const
{
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
// This traverses through the module hierarchy
void dbNetwork::visitConnectedPins(const Net* net,
                                   PinVisitor& visitor,
                                   NetSet& visited_nets) const
{
  static int debug;
  debug++;
  if (!visited_nets.hasKey(net)) {
    visited_nets.insert(net);

    dbNet* db_net = nullptr;
    dbModNet* mod_net = nullptr;
    staToDb(net, db_net, mod_net);

    if (db_net && !mod_net) {
      for (dbITerm* iterm : db_net->getITerms()) {
        Pin* pin = dbToSta(iterm);
        visitor(pin);
        debug++;
      }
      for (dbBTerm* bterm : db_net->getBTerms()) {
        Pin* pin = dbToSta(bterm);
        visitor(pin);
        debug++;
      }
    } else if (mod_net) {
      for (dbITerm* iterm : mod_net->getITerms()) {
        Pin* pin = dbToSta(iterm);
        visitor(pin);
        debug++;
      }
      for (dbBTerm* bterm : mod_net->getBTerms()) {
        Pin* pin = dbToSta(bterm);
        visitor(pin);
        debug++;
      }
      for (dbModITerm* moditerm : mod_net->getModITerms()) {
        Pin* pin = dbToSta(moditerm);
        visitor(pin);
        debug++;
      }
      // visit below nets
      for (dbModITerm* moditerm : mod_net->getModITerms()) {
        dbModInst* mod_inst = moditerm->getParent();
        // note we are deailing with a uniquified hierarchy
        // so one master per instance..
        dbModule* module = mod_inst->getMaster();
        std::string pin_name = moditerm->getName();
        dbModBTerm* mod_bterm = module->findModBTerm(pin_name.c_str());
        Pin* below_pin = dbToStaPin(mod_bterm);
        pin_name = name(below_pin);
        visitor(below_pin);
        // traverse along rest of net
        Net* below_net = this->net(below_pin);
        visitConnectedPins(below_net, visitor, visited_nets);
        debug++;
      }
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
  return staToDb(term)->getId();
}

Pin* dbNetwork::pin(const Term* term) const
{
  // Only terms are for top level instance pins, which are also BTerms.
  // Note this will screw up hierarchical level when starting from top
  // net
  dbBTerm* bterm = nullptr;
  dbModBTerm* modbterm = nullptr;
  dbITerm* iterm = nullptr;
  dbModITerm* moditerm = nullptr;
  staToDb(term, iterm, bterm, moditerm, modbterm);
  if (bterm) {
    return (dbToSta(bterm));
  } else if (modbterm) {
    // get the moditerm
    dbModule* cur_module = modbterm->getParent();
    dbModInst* cur_mod_inst = cur_module->getModInst();
    dbModITerm* parent_moditerm = nullptr;
    ;
    std::string pin_name
        = makePinName(cur_mod_inst->getName(), modbterm->getName());
    if (cur_mod_inst->findModITerm(pin_name.c_str(), parent_moditerm))
      return dbToSta(parent_moditerm);
  }
  return nullptr;
}

//
// bug here: could be iterm or bterm.. how to tell
//
Net* dbNetwork::net(const Term* term) const
{
  dbITerm* iterm = nullptr;
  dbBTerm* bterm = nullptr;
  dbModITerm* moditerm = nullptr;
  dbModBTerm* modbterm = nullptr;

  staToDb(term, iterm, bterm, moditerm, modbterm);
  if (moditerm) {
    // need to push down in hierachy
    // get modbterm
    // get net off mod bterm
    return dbToSta(moditerm->getNet());
  }
  if (modbterm) {
    return dbToSta(modbterm->getNet());
  }
  if (bterm) {
    dbModNet* mod_net = bterm->getModNet();
    if (mod_net)
      return dbToSta(mod_net);
    dbNet* dnet = bterm->getNet();
    if (dnet)
      return dbToSta(dnet);
  }
  return nullptr;
}

////////////////////////////////////////////////////////////////

bool dbNetwork::isLinked() const
{
  return top_cell_ != nullptr;
}

bool dbNetwork::linkNetwork(const char*, bool, Report*)
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
//
void dbNetwork::readDbAfter(odb::dbDatabase* db)
{
  db_ = db;
  dbChip* chip = db_->getChip();
  if (chip) {
    block_ = chip->getBlock();
    for (dbLib* lib : db_->getLibs()) {
      makeLibrary(lib);
    }

    // special handling: make cells in library for module
    readDbNetlistAfter();

    // Now do same for module hierarchy.
#ifdef DEBUG_DBNW
    printf("Read %d modnets %d modinsts \n",
           block_->getModNets().size(),
           block_->getModInsts().size());
#endif

    // problem is modinsts and liberty insts have no masters
    //(liberties resolved by reading in liberty library) and
    // then used default concretePort iterator, but we cannot
    // make dbModInst work with concrete port iterator.
    // so we need to "mark" dbmod ists

    Library* verilog_library = makeLibrary("verilog", 0);
    dbSet<dbModInst> modinsts = block_->getModInsts();
    dbSet<dbModInst>::iterator modinst_iter_ = modinsts.begin();
    dbSet<dbModInst>::iterator modinst_end_ = modinsts.end();
    for (; modinst_iter_ != modinst_end_; modinst_iter_++) {
      makeVerilogCell(verilog_library, *modinst_iter_);
    }
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
  ConcreteCell* ccell = reinterpret_cast<ConcreteCell*>(cell);
  ccell->setExtCell(reinterpret_cast<void*>(master));

  // Use the default liberty for "linking" the db/LEF masters.
  LibertyCell* lib_cell = findLibertyCell(cell_name);
  if (lib_cell) {
    ccell->setLibertyCell(lib_cell);
    lib_cell->setExtCell(reinterpret_cast<void*>(master));
  }

  for (dbMTerm* mterm : master->getMTerms()) {
    const char* port_name = mterm->getConstName();
    Port* port = makePort(cell, port_name);
    PortDirection* dir = dbToSta(mterm->getSigType(), mterm->getIoType());
    setDirection(port, dir);
    mterm->staSetPort(reinterpret_cast<void*>(port));
    ConcretePort* cport = reinterpret_cast<ConcretePort*>(port);
    cport->setExtPort(reinterpret_cast<void*>(mterm));

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
    }
  }
  // Assume msb first busses because LEF has no clue about busses.
  groupBusPorts(cell, [](const char*) { return true; });

  // Fill in liberty to db/LEF master correspondence for libraries not used
  // for corners that are not used for "linking".
  LibertyLibraryIterator* lib_iter = libertyLibraryIterator();
  while (lib_iter->hasNext()) {
    LibertyLibrary* lib = lib_iter->next();
    LibertyCell* lib_cell = lib->findLibertyCell(cell_name);
    if (lib_cell) {
      lib_cell->setExtCell(reinterpret_cast<void*>(master));
      for (dbMTerm* mterm : master->getMTerms()) {
        const char* port_name = mterm->getConstName();
        LibertyPort* lib_port = lib_cell->findLibertyPort(port_name);
        if (lib_port) {
          lib_port->setExtPort(mterm);
        }
      }
    }
  }
  delete lib_iter;
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
  top_cell_ = nullptr;
  top_cell_ = makeCell(top_lib, design_name, false, nullptr);
  for (dbBTerm* bterm : block_->getBTerms()) {
    makeTopPort(bterm);
  }
  groupBusPorts(top_cell_, [=](const char* port_name) {
    return portMsbFirst(port_name, design_name);
  });
}

Port* dbNetwork::makeTopPort(dbBTerm* bterm)
{
  const char* port_name = bterm->getConstName();
  Port* port = makePort(top_cell_, port_name);
  PortDirection* dir = dbToSta(bterm->getSigType(), bterm->getIoType());
  setDirection(port, dir);
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
// AF: add cell name
bool dbNetwork::portMsbFirst(const char* port_name, const char* cell_name)
{
  string key = "bus_msb_first ";
  // AF
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
      ConcreteLibraryCellIterator* cell_iter = clib->cellIterator();
      while (cell_iter->hasNext()) {
        ConcreteCell* ccell = cell_iter->next();
        // Don't clobber an existing liberty cell so link points to the first.
        if (ccell->libertyCell() == nullptr) {
          LibertyCell* lcell = lib->findLibertyCell(ccell->name());
          if (lcell) {
            lcell->setExtCell(ccell->extCell());
            ccell->setLibertyCell(lcell);
            ConcreteCellPortBitIterator* port_iter = ccell->portBitIterator();
            while (port_iter->hasNext()) {
              ConcretePort* cport = port_iter->next();
              const char* port_name = cport->name();
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
            }
            delete port_iter;
          }
        }
      }
      delete cell_iter;
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
  if (parent == top_instance_) {
    const char* cell_name = cell->name();
    dbMaster* master = db_->findMaster(cell_name);
    if (master) {
      dbInst* inst = dbInst::create(block_, master, name);
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

void dbNetwork::disconnectPin(Pin* pin)
{
  dbITerm* iterm;
  dbBTerm* bterm;
  dbModITerm* moditerm;
  dbModBTerm* modbterm;

  staToDb(pin, iterm, bterm, moditerm, modbterm);
  if (iterm) {
    iterm->disconnect();
  } else if (bterm) {
    bterm->disconnect();
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
    return findPort(cell, name);
  }
  return ConcreteNetwork::makePort(cell, name);
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
  if (parent == top_instance_) {
    dbNet* dnet = dbNet::create(block_, name, false);
    return dbToSta(dnet);
  }
  return nullptr;
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
  db_inst = nullptr;
  mod_inst = nullptr;
  if (instance) {
    dbObject* obj
        = reinterpret_cast<dbObject*>(const_cast<Instance*>(instance));
    dbObjectType type = obj->getObjectType();
    if (type == dbInstObj) {
      db_inst = static_cast<dbInst*>(obj);
    } else if (type == dbModInstObj) {
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

dbMaster* dbNetwork::staToDb(const Cell* cell) const
{
  const ConcreteCell* ccell = reinterpret_cast<const ConcreteCell*>(cell);
  return reinterpret_cast<dbMaster*>(ccell->extCell());
}

void dbNetwork::staToDb(const Cell* cell,
                        dbMaster*& master,
                        dbModule*& module) const
{
  module = nullptr;
  master = nullptr;
  if (findLibertyCell(name(cell))) {
    master = reinterpret_cast<dbMaster*>(const_cast<Cell*>(cell));
  } else {
    if (block_) {
      if (block_->findModule(name(cell)))
        module = reinterpret_cast<dbModule*>(const_cast<Cell*>(cell));
      else
        master = reinterpret_cast<dbMaster*>(const_cast<Cell*>(cell));
    }
  }
}

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

// port is:
// 1. top level block term
// 2. Module block term (term on module definition, module master)
// 3. instance master term (term on libray cell)

void dbNetwork::staToDb(const Port* port,
                        dbBTerm*& bterm,
                        dbMTerm*& mterm,
                        dbModBTerm*& modbterm) const
{
  bterm = nullptr;
  modbterm = nullptr;

  if (port) {
    const char* port_name = name(port);
    dbBlock* local_block = block();

    dbBTerm* top_bterm = local_block->findBTerm(port_name);

    if (top_bterm) {
      dbModule* containing_module = block_->findModule(name(cell(port)));
      if (containing_module == block()->getTopModule()) {
        bterm = top_bterm;
        return;
      }
      // fall through. Expect a modbterm or a bterm
    }

    Cell* candidate_cell = cell(port);
    if (candidate_cell) {
      LibertyCell* lib_cell = findLibertyCell(name(candidate_cell));
      if (lib_cell) {
        LibertyPort* lib_port = lib_cell->findLibertyPort(port_name);
        if (lib_port) {
          mterm = staToDb(lib_port);
          return;
        } else {
          return;
        }
      } else {
        dbModule* module = nullptr;
        block_->findModule(name(candidate_cell));
        if (!module) {
          mterm = staToDb(port);
          return;
        } else {
          if (module->findModBTerm(name(port))) {
            return;
          }
        }
      }
    }
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

Instance* dbNetwork::dbToSta(dbInst* inst) const
{
  return reinterpret_cast<Instance*>(inst);
}

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

Net* dbNetwork::dbToSta(dbNet* net) const
{
  return reinterpret_cast<Net*>(net);
}

const Net* dbNetwork::dbToSta(const dbNet* net) const
{
  return reinterpret_cast<const Net*>(net);
}

Net* dbNetwork::dbToSta(dbModNet* net) const
{
  return reinterpret_cast<Net*>(net);
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

Term* dbNetwork::dbToStaTerm(dbModITerm* moditerm) const
{
  return reinterpret_cast<Term*>(moditerm);
}

Term* dbNetwork::dbToStaTerm(dbModBTerm* modbterm) const
{
  return reinterpret_cast<Term*>(modbterm);
}

Port* dbNetwork::dbToSta(dbMTerm* mterm) const
{
  return reinterpret_cast<Port*>(mterm->staPort());
}

Port* dbNetwork::dbToSta(dbModBTerm* modbterm) const
{
  return reinterpret_cast<Port*>(modbterm->staPort());
}

Cell* dbNetwork::dbToSta(dbMaster* master) const
{
  return reinterpret_cast<Cell*>(master->staCell());
}

Cell* dbNetwork::dbToSta(dbModule* master) const
{
  return ((Cell*) (master->getStaCell()));
  //  Cell* ret = reinterpret_cast<Cell*>(master);
  //  return ret;
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

}  // namespace sta
