// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbModule.h"

#include "dbBlock.h"
#include "dbDatabase.h"
#include "dbHashTable.hpp"
#include "dbInst.h"
#include "dbJournal.h"
#include "dbModBTerm.h"
#include "dbModInst.h"
#include "dbModulePortItr.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/db.h"
// User Code Begin Includes
#include <cstddef>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "dbModNet.h"
#include "dbModuleInstItr.h"
#include "dbModuleModBTermItr.h"
#include "dbModuleModInstItr.h"
#include "dbModuleModNetItr.h"
#include "utl/Logger.h"
// User Code End Includes
namespace odb {
template class dbTable<_dbModule>;

bool _dbModule::operator==(const _dbModule& rhs) const
{
  if (_name != rhs._name) {
    return false;
  }
  if (_next_entry != rhs._next_entry) {
    return false;
  }
  if (_insts != rhs._insts) {
    return false;
  }
  if (_mod_inst != rhs._mod_inst) {
    return false;
  }
  if (_modinsts != rhs._modinsts) {
    return false;
  }
  if (_modnets != rhs._modnets) {
    return false;
  }
  if (_modbterms != rhs._modbterms) {
    return false;
  }

  return true;
}

bool _dbModule::operator<(const _dbModule& rhs) const
{
  // User Code Begin <
  if (strcmp(_name, rhs._name) >= 0) {
    return false;
  }
  // User Code End <
  return true;
}

_dbModule::_dbModule(_dbDatabase* db)
{
  // User Code Begin Constructor
  _name = nullptr;
  _insts = 0;
  _modinsts = 0;
  _mod_inst = 0;
  // User Code End Constructor
}

dbIStream& operator>>(dbIStream& stream, _dbModule& obj)
{
  stream >> obj._name;
  stream >> obj._next_entry;
  stream >> obj._insts;
  stream >> obj._mod_inst;
  stream >> obj._modinsts;
  if (obj.getDatabase()->isSchema(db_schema_update_hierarchy)) {
    stream >> obj._modnets;
  }
  if (obj.getDatabase()->isSchema(db_schema_update_hierarchy)) {
    stream >> obj._modbterms;
  }
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbModule& obj)
{
  stream << obj._name;
  stream << obj._next_entry;
  stream << obj._insts;
  stream << obj._mod_inst;
  stream << obj._modinsts;
  if (obj.getDatabase()->isSchema(db_schema_update_hierarchy)) {
    stream << obj._modnets;
  }
  if (obj.getDatabase()->isSchema(db_schema_update_hierarchy)) {
    stream << obj._modbterms;
  }
  return stream;
}

void _dbModule::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  // User Code Begin collectMemInfo
  info.children_["name"].add(_name);
  info.children_["_dbinst_hash"].add(_dbinst_hash);
  info.children_["_modinst_hash"].add(_modinst_hash);
  info.children_["_modbterm_hash"].add(_modbterm_hash);
  info.children_["_modnet_hash"].add(_modnet_hash);
  // User Code End collectMemInfo
}

_dbModule::~_dbModule()
{
  if (_name) {
    free((void*) _name);
  }
}

////////////////////////////////////////////////////////////////////
//
// dbModule - Methods
//
////////////////////////////////////////////////////////////////////

const char* dbModule::getName() const
{
  _dbModule* obj = (_dbModule*) this;
  return obj->_name;
}

void dbModule::setModInst(dbModInst* mod_inst)
{
  _dbModule* obj = (_dbModule*) this;

  obj->_mod_inst = mod_inst->getImpl()->getOID();
}

dbModInst* dbModule::getModInst() const
{
  _dbModule* obj = (_dbModule*) this;
  if (obj->_mod_inst == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbModInst*) par->_modinst_tbl->getPtr(obj->_mod_inst);
}

// User Code Begin dbModulePublicMethods

const dbModBTerm* dbModule::getHeadDbModBTerm() const
{
  _dbModule* obj = (_dbModule*) this;
  _dbBlock* block_ = (_dbBlock*) obj->getOwner();
  if (obj->_modbterms == 0) {
    return nullptr;
  }
  // note that the odb objects are "pre-pended"
  // so first object is at tail. This means we are returning
  // last object added. The application calling this routine
  // needs to be aware of this (and possibly skip to the end
  // of the list and then use prev to reconstruct creation order).
  return (dbModBTerm*) (block_->_modbterm_tbl->getPtr(obj->_modbterms));
}

int dbModule::getModInstCount()
{
  _dbModule* module = (_dbModule*) this;
  _dbBlock* block = (_dbBlock*) module->getOwner();
  return (int) ((dbSet<dbModInst>(module, block->_module_modinst_itr)).size());
}

int dbModule::getDbInstCount()
{
  _dbModule* module = (_dbModule*) this;
  _dbBlock* block = (_dbBlock*) module->getOwner();
  return (int) (dbSet<dbInst>(module, block->_module_inst_itr).size());
}

void dbModule::addInst(dbInst* inst)
{
  _dbModule* module = (_dbModule*) this;
  _dbInst* _inst = (_dbInst*) inst;
  _dbBlock* block = (_dbBlock*) module->getOwner();

  if (_inst->_flags._physical_only) {
    _inst->getLogger()->error(
        utl::ODB,
        297,
        "Physical only instance {} can't be added to module {}",
        inst->getName(),
        getName());
  }

  if (_inst->_module == module->getOID()) {
    return;  // already in this module
  }

  if (_inst->_flags._dont_touch) {
    _inst->getLogger()->error(
        utl::ODB,
        367,
        "Attempt to change the module of dont_touch instance {}",
        _inst->_name);
  }

  if (_inst->_module != 0) {
    dbModule* mod = dbModule::getModule((dbBlock*) block, _inst->_module);
    ((_dbModule*) mod)->removeInst(inst);
  }

  _inst->_module = module->getOID();
  module->_dbinst_hash[inst->getName()] = dbId<_dbInst>(_inst->getOID());

  if (module->_insts == 0) {
    _inst->_module_next = 0;
    _inst->_module_prev = 0;
    module->_insts = _inst->getOID();
  } else {
    _dbInst* cur_head = block->_inst_tbl->getPtr(module->_insts);
    _inst->_module_next = module->_insts;
    module->_insts = _inst->getOID();
    cur_head->_module_prev = _inst->getOID();
  }
}

void _dbModule::removeInst(dbInst* inst)
{
  _dbModule* module = (_dbModule*) this;
  _dbInst* _inst = (_dbInst*) inst;
  uint id = _inst->getOID();

  if (_inst->_module != getOID()) {
    return;
  }

  if (_inst->_flags._dont_touch) {
    _inst->getLogger()->error(
        utl::ODB,
        371,
        "Attempt to remove dont_touch instance {} from parent module",
        _inst->_name);
  }

  _dbBlock* block = (_dbBlock*) getOwner();

  if (module->_insts == id) {
    module->_insts = _inst->_module_next;

    if (module->_insts != 0) {
      _dbInst* t = block->_inst_tbl->getPtr(module->_insts);
      t->_module_prev = 0;
    }
  } else {
    if (_inst->_module_next != 0) {
      _dbInst* next = block->_inst_tbl->getPtr(_inst->_module_next);
      next->_module_prev = _inst->_module_prev;
    }

    if (_inst->_module_prev != 0) {
      _dbInst* prev = block->_inst_tbl->getPtr(_inst->_module_prev);
      prev->_module_next = _inst->_module_next;
    }
  }
  _inst->_module = 0;
  _inst->_module_next = 0;
  _inst->_module_prev = 0;
}

dbSet<dbModInst> dbModule::getChildren()
{
  _dbModule* module = (_dbModule*) this;
  _dbBlock* block = (_dbBlock*) module->getOwner();
  return dbSet<dbModInst>(module, block->_module_modinst_itr);
}

dbSet<dbModNet> dbModule::getModNets()
{
  _dbModule* module = (_dbModule*) this;
  _dbBlock* block = (_dbBlock*) module->getOwner();
  return dbSet<dbModNet>(module, block->_module_modnet_itr);
}

dbModNet* dbModule::getModNet(const char* net_name)
{
  _dbModule* module = (_dbModule*) this;
  _dbBlock* block = (_dbBlock*) module->getOwner();
  auto it = module->_modnet_hash.find(net_name);
  if (it != module->_modnet_hash.end()) {
    uint db_id = (*it).second;
    return (dbModNet*) block->_modnet_tbl->getPtr(db_id);
  }
  return nullptr;
}

dbSet<dbModInst> dbModule::getModInsts()
{
  _dbModule* module = (_dbModule*) this;
  _dbBlock* block = (_dbBlock*) module->getOwner();
  return dbSet<dbModInst>(module, block->_module_modinst_itr);
}

//
// The ports include higher level views. These have a special
// iterator which knows about how to skip the contents
// of the hierarchical objects (busports)
//

dbSet<dbModBTerm> dbModule::getPorts()
{
  _dbModule* obj = (_dbModule*) this;
  if (obj->_port_iter == nullptr) {
    _dbBlock* block = (_dbBlock*) obj->getOwner();
    obj->_port_iter = new dbModulePortItr(block->_modbterm_tbl);
  }
  return dbSet<dbModBTerm>(this, obj->_port_iter);
}

//
// The modbterms are the leaf level connections
//"flat view"
//
dbSet<dbModBTerm> dbModule::getModBTerms()
{
  _dbModule* module = (_dbModule*) this;
  _dbBlock* block = (_dbBlock*) module->getOwner();
  return dbSet<dbModBTerm>(module, block->_module_modbterm_itr);
}

dbModBTerm* dbModule::getModBTerm(uint id)
{
  _dbModule* module = (_dbModule*) this;
  _dbBlock* block = (_dbBlock*) module->getOwner();
  return (dbModBTerm*) (block->_modbterm_tbl->getObject(id));
}

dbSet<dbInst> dbModule::getInsts()
{
  _dbModule* module = (_dbModule*) this;
  _dbBlock* block = (_dbBlock*) module->getOwner();
  return dbSet<dbInst>(module, block->_module_inst_itr);
}

dbModule* dbModule::create(dbBlock* block, const char* name)
{
  _dbBlock* _block = (_dbBlock*) block;
  if (_block->_module_hash.hasMember(name)) {
    return nullptr;
  }
  _dbModule* module = _block->_module_tbl->create();
  module->_name = strdup(name);
  ZALLOCATED(module->_name);
  _block->_module_hash.insert(module);

  if (_block->_journal) {
    _block->_journal->beginAction(dbJournal::CREATE_OBJECT);
    _block->_journal->pushParam(dbModuleObj);
    _block->_journal->pushParam(module->_name);
    _block->_journal->pushParam(module->getId());
    _block->_journal->endAction();
  }

  return (dbModule*) module;
}

void dbModule::destroy(dbModule* module)
{
  _dbModule* _module = (_dbModule*) module;
  _dbBlock* block = (_dbBlock*) _module->getOwner();

  if (block->_top_module == module->getId()) {
    _module->getLogger()->error(
        utl::ODB, 298, "The top module can't be destroyed.");
  }

  //
  //
  // We only destroy the contents of the module
  // We assume that the module instance (if any) of this module
  // has already been deleted.
  // We do this because a module may now have multiple instances
  // So we cannot always delete a module if it module instances
  // have not been cleaned up.
  //

  if (_module->_mod_inst != 0) {
    _module->getLogger()->error(
        utl::ODB,
        389,
        "Must destroy module instance before destroying "
        "module definition to avoid orphanned references");
    return;
  }

  dbSet<dbModInst> modinsts = module->getChildren();
  dbSet<dbModInst>::iterator itr;
  for (itr = modinsts.begin(); itr != modinsts.end();) {
    itr = dbModInst::destroy(itr);
  }

  dbSet<dbInst> insts = module->getInsts();
  dbSet<dbInst>::iterator inst_itr;
  for (inst_itr = insts.begin(); inst_itr != insts.end();) {
    inst_itr = dbInst::destroy(inst_itr);
  }

  dbSet<dbModBTerm> modbterms = module->getModBTerms();
  dbSet<dbModBTerm>::iterator modbterm_itr;
  for (modbterm_itr = modbterms.begin(); modbterm_itr != modbterms.end();) {
    modbterm_itr = dbModBTerm::destroy(modbterm_itr);
  }

  dbSet<dbModNet> modnets = module->getModNets();
  dbSet<dbModNet>::iterator modnet_itr;
  for (modnet_itr = modnets.begin(); modnet_itr != modnets.end();) {
    modnet_itr = dbModNet::destroy(modnet_itr);
  }

  if (block->_journal) {
    block->_journal->beginAction(dbJournal::DELETE_OBJECT);
    block->_journal->pushParam(dbModuleObj);
    block->_journal->pushParam(module->getName());
    block->_journal->pushParam(module->getId());
    block->_journal->endAction();
  }

  dbProperty::destroyProperties(_module);
  block->_module_hash.remove(_module);
  block->_module_tbl->destroy(_module);
}

dbModule* dbModule::getModule(dbBlock* block_, uint dbid_)
{
  _dbBlock* block = (_dbBlock*) block_;
  return (dbModule*) block->_module_tbl->getPtr(dbid_);
}

dbModInst* dbModule::findModInst(const char* name)
{
  _dbModule* obj = (_dbModule*) this;
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  auto it = obj->_modinst_hash.find(name);
  if (it != obj->_modinst_hash.end()) {
    auto db_id = (*it).second;
    return (dbModInst*) par->_modinst_tbl->getPtr(db_id);
  }
  return nullptr;
}

dbInst* dbModule::findDbInst(const char* name)
{
  _dbModule* obj = (_dbModule*) this;
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  auto it = obj->_dbinst_hash.find(name);
  if (it != obj->_dbinst_hash.end()) {
    auto db_id = (*it).second;
    return (dbInst*) par->_inst_tbl->getPtr(db_id);
  }
  return nullptr;
}

static void getLeafInsts(dbModule* module, std::vector<dbInst*>& insts)
{
  for (dbInst* inst : module->getInsts()) {
    insts.push_back(inst);
  }

  for (dbModInst* inst : module->getChildren()) {
    getLeafInsts(inst->getMaster(), insts);
  }
}

std::vector<dbInst*> dbModule::getLeafInsts()
{
  std::vector<dbInst*> insts;
  odb::getLeafInsts(this, insts);
  return insts;
}

dbModBTerm* dbModule::findModBTerm(const char* name)
{
  std::string modbterm_name(name);
  // TODO: use proper hierarchy limiter from _dbBlock->_hier_delimiter
  size_t last_idx = modbterm_name.find_last_of('/');
  if (last_idx != std::string::npos) {
    modbterm_name = modbterm_name.substr(last_idx + 1);
  }
  _dbModule* obj = (_dbModule*) this;
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  auto it = obj->_modbterm_hash.find(name);
  if (it != obj->_modbterm_hash.end()) {
    auto db_id = (*it).second;
    return (dbModBTerm*) par->_modbterm_tbl->getPtr(db_id);
  }
  return nullptr;
}

std::string dbModule::getHierarchicalName() const
{
  dbModInst* inst = getModInst();
  if (inst) {
    return inst->getHierarchicalName();
  }
  return "<top>";
}

dbBlock* dbModule::getOwner()
{
  _dbModule* obj = (_dbModule*) this;
  return (dbBlock*) obj->getOwner();
}

// Make a unique copy of module based on cell_name and inst_name
dbModule* dbModule::makeUniqueDbModule(const char* cell_name,
                                       const char* inst_name,
                                       dbBlock* block)

{
  dbModule* module = dbModule::create(block, cell_name);
  if (module != nullptr) {
    return module;
  }

  std::map<std::string, int>& name_id_map
      = ((_dbBlock*) block)->_module_name_id_map;
  std::string orig_cell_name(cell_name);
  std::string module_name = orig_cell_name + '_' + std::string(inst_name);
  do {
    std::string full_name = module_name;
    int& id = name_id_map[module_name];
    if (id > 0) {
      full_name += "_" + std::to_string(id);
    }
    ++id;
    module = dbModule::create(block, full_name.c_str());
  } while (module == nullptr);
  return module;
}

// Do a "deep" copy of old_module based on its instance context into new_module.
// All ports, instances, mod nets and parent/child IO will be copied.
// Connections that span multiple modules needs to be done outside this API.
// new_mod_inst is needed to create module instances for instance name
// uniquification.
void dbModule::copy(dbModule* old_module,
                    dbModule* new_module,
                    dbModInst* new_mod_inst)
{
  // Copy module ports including bus members
  modBTMap mod_bt_map;  // map old mbterm to new mbterm
  copyModulePorts(old_module, new_module, mod_bt_map);

  // Copy module instances and create iterm map
  ITMap it_map;  // map old iterm to new iterm
  copyModuleInsts(old_module, new_module, new_mod_inst, it_map);

  // TODO: handle hierarchical child instances

  // Copy mod nets and connect ports and iterms
  copyModuleModNets(old_module, new_module, mod_bt_map, it_map);

  // Establish boundary IO between parent and child
  copyModuleBoundaryIO(old_module, new_module, new_mod_inst);
}

// A bus with N members have N+1 modbterms.  The first one is the "bus port"
// sentinel.   The sentinel has reference to the member size, direction and
// list of member modbterms.
void dbModule::copyModulePorts(dbModule* old_module,
                               dbModule* new_module,
                               modBTMap& mod_bt_map)
{
  utl::Logger* logger = old_module->getImpl()->getLogger();
  for (dbModBTerm* old_port : old_module->getModBTerms()) {
    dbModBTerm* new_port = nullptr;
    if (mod_bt_map.count(old_port) > 0) {
      new_port = mod_bt_map[old_port];
      debugPrint(logger,
                 utl::ODB,
                 "replace_design",
                 1,
                 "Module port {} already exists for old port {}",
                 new_port->getName(),
                 old_port->getName());
    } else {
      new_port = dbModBTerm::create(new_module, old_port->getName());
      if (new_port) {
        debugPrint(logger,
                   utl::ODB,
                   "replace_design",
                   1,
                   "Created module port {} for old port {}",
                   new_port->getName(),
                   old_port->getName());
        mod_bt_map[old_port] = new_port;
        new_port->setIoType(old_port->getIoType());
      } else {
        logger->error(utl::ODB,
                      456,
                      "Module port {} cannot be created",
                      old_port->getName());
      }
      if (old_port->isBusPort()) {
        dbBusPort* old_bus_port = old_port->getBusPort();
        dbBusPort* new_bus_port = dbBusPort::create(new_module,
                                                    new_port,
                                                    old_bus_port->getFrom(),
                                                    old_bus_port->getTo());
        if (new_bus_port) {
          debugPrint(logger,
                     utl::ODB,
                     "replace_design",
                     1,
                     "Created module bus port {}[{}:{}]",
                     new_port->getName(),
                     old_bus_port->getFrom(),
                     old_bus_port->getTo());
        } else {
          logger->error(utl::ODB,
                        457,
                        "Module bus port {} cannot be created",
                        new_port->getName());
        }
        new_port->setBusPort(new_bus_port);

        // create bus members
        int from_index = old_bus_port->getFrom();
        int to_index = old_bus_port->getTo();
        bool updown = from_index <= to_index;
        int size
            = updown ? to_index - from_index + 1 : from_index - to_index + 1;
        for (int i = 0; i < size; i++) {
          int ix = updown ? from_index + i : from_index - i;
          dbModBTerm* old_bus_bit = old_bus_port->getBusIndexedElement(ix);
          if (old_bus_bit == nullptr) {
            logger->error(utl::ODB,
                          468,
                          "Module bus bit {}[{}] does not exist",
                          old_port->getName(),
                          ix);
          }
          // TODO: use proper bus array delimiter instead of '[' and ']'
          std::string bus_bit_name = std::string(old_port->getName())
                                     + std::string("[") + std::to_string(ix)
                                     + std::string("]");
          dbModBTerm* new_bus_bit
              = dbModBTerm::create(new_module, bus_bit_name.c_str());
          if (new_bus_bit == nullptr) {
            logger->error(utl::ODB,
                          458,
                          "Module bus bit {} cannot be created",
                          bus_bit_name);
          }
          if (i == 0) {
            new_bus_port->setMembers(new_bus_bit);
          }
          if (i == size - 1) {
            new_bus_port->setLast(new_bus_bit);
          }
          new_bus_bit->setIoType(old_port->getIoType());
          mod_bt_map[old_bus_bit] = new_bus_bit;
          debugPrint(logger,
                     utl::ODB,
                     "replace_design",
                     1,
                     "Created module bus bit {} for {}",
                     new_bus_bit->getName(),
                     old_bus_bit->getName());
        }  // end of bus port handling
      }
    }
  }

  new_module->getModBTerms().reverse();

  debugPrint(logger,
             utl::ODB,
             "replace_design",
             1,
             "copyModulePorts: modBTMap has {} ports",
             mod_bt_map.size());
}

void dbModule::copyModuleInsts(dbModule* old_module,
                               dbModule* new_module,
                               dbModInst* new_mod_inst,
                               ITMap& it_map)
{
  utl::Logger* logger = old_module->getImpl()->getLogger();
  // Add insts to new module
  for (dbInst* old_inst : old_module->getInsts()) {
    // Change unique instance name from old_inst/leaf to new_inst/leaf
    std::string new_inst_name;
    if (new_mod_inst) {
      new_inst_name = new_mod_inst->getName();
      new_inst_name += '/';
    }
    std::string old_inst_name = old_inst->getName();
    // TODO: use proper hierarchy limiter from _dbBlock->_hier_delimiter
    size_t first_idx = old_inst_name.find_first_of('/');
    new_inst_name += (first_idx != std::string::npos)
                         ? std::move(old_inst_name).substr(first_idx + 1)
                         : std::move(old_inst_name);

    dbInst* new_inst = dbInst::create(new_module->getOwner(),
                                      old_inst->getMaster(),
                                      new_inst_name.c_str(),
                                      /* phyical only */ false,
                                      new_module);
    if (new_inst) {
      debugPrint(logger,
                 utl::ODB,
                 "replace_design",
                 1,
                 "Created module instance {}",
                 new_inst->getName());
    } else {
      logger->error(
          utl::ODB, 459, "Module instance {} cannot be created", new_inst_name);
    }

    // Map old iterms to new iterms and connect iterms that are local to this
    // module only.  Nets that connect to iterms of other modules will be
    // done outside this API in dbModInst::swapMaster().
    dbSet<dbITerm> old_iterms = old_inst->getITerms();
    dbSet<dbITerm> new_iterms = new_inst->getITerms();
    dbSet<dbITerm>::iterator iter1, iter2;
    iter1 = old_iterms.begin();
    iter2 = new_iterms.begin();
    for (; iter1 != old_iterms.end() && iter2 != new_iterms.end();
         ++iter1, ++iter2) {
      dbITerm* old_iterm = *iter1;
      dbITerm* new_iterm = *iter2;
      it_map[old_iterm] = new_iterm;
      debugPrint(logger,
                 utl::ODB,
                 "replace_design",
                 1,
                 "  old iterm {} maps to new iterm {}",
                 old_iterm->getName(),
                 new_iterm->getName());
      dbNet* old_net = old_iterm->getNet();
      if (old_net) {
        // Create a local net only if it connects to iterms inside this module
        std::string new_net_name;
        if (new_mod_inst) {
          new_net_name = new_mod_inst->getName();
          new_net_name += '/';
        }
        std::string old_net_name = old_net->getName();
        // TODO: use proper hierarchy limiter from _dbBlock->_hier_delimiter
        size_t first_idx = old_net_name.find_first_of('/');
        new_net_name += (first_idx != std::string::npos)
                            ? std::move(old_net_name).substr(first_idx + 1)
                            : std::move(old_net_name);

        dbNet* new_net = new_module->getOwner()->findNet(new_net_name.c_str());
        if (new_net) {
          new_iterm->connect(new_net);
          debugPrint(logger,
                     utl::ODB,
                     "replace_design",
                     1,
                     "  connected iterm {} to existing local net {}",
                     new_iterm->getName(),
                     new_net->getName());
        } else {
          new_net = dbNet::create(new_module->getOwner(), new_net_name.c_str());
          new_iterm->connect(new_net);
          debugPrint(logger,
                     utl::ODB,
                     "replace_design",
                     1,
                     "  Connected iterm {} to new local net {}",
                     new_iterm->getName(),
                     new_net->getName());
        }
      }
    }
  }

  if (new_module->getInsts().reversible()
      && new_module->getInsts().orderReversed()) {
    new_module->getInsts().reverse();
  }
}

void dbModule::copyModuleModNets(dbModule* old_module,
                                 dbModule* new_module,
                                 modBTMap& mod_bt_map,
                                 ITMap& it_map)
{
  utl::Logger* logger = old_module->getImpl()->getLogger();
  debugPrint(logger,
             utl::ODB,
             "replace_design",
             1,
             "copyModuleModNets: modBT_map has {} ports, it_map has {} iterms",
             mod_bt_map.size(),
             it_map.size());
  // Make boundary port connections.
  for (dbModNet* old_net : old_module->getModNets()) {
    dbModNet* new_net = dbModNet::create(new_module, old_net->getName());
    if (new_net) {
      debugPrint(logger,
                 utl::ODB,
                 "replace_design",
                 1,
                 "Created module mod net {}",
                 new_net->getName());
    } else {
      logger->error(utl::ODB,
                    460,
                    "Module mod net {} cannot be created",
                    old_net->getName());
    }

    // Connect dbModBTerms to new mod net
    for (dbModBTerm* old_mbterm : old_net->getModBTerms()) {
      dbModBTerm* new_mbterm = nullptr;
      if (mod_bt_map.count(old_mbterm) > 0) {
        new_mbterm = mod_bt_map[old_mbterm];
      }
      if (new_mbterm) {
        new_mbterm->connect(new_net);
        debugPrint(logger,
                   utl::ODB,
                   "replace_design",
                   1,
                   "  connected new port {} to mod net {} because old port "
                   "{} maps to new port",
                   new_mbterm->getName(),
                   new_net->getName(),
                   old_mbterm->getName());
      } else {
        logger->error(utl::ODB,
                      461,
                      "Port {} cannot be connected to mod net {} because it "
                      "does not exist",
                      old_mbterm->getName(),
                      new_net->getName());
      }
    }

    // Connect iterms to new mod net
    debugPrint(logger,
               utl::ODB,
               "replace_design",
               1,
               "  old net {} has {} iterms",
               old_net->getName(),
               old_net->getITerms().size());
    for (dbITerm* old_iterm : old_net->getITerms()) {
      dbITerm* new_iterm = nullptr;
      if (it_map.count(old_iterm) > 0) {
        new_iterm = it_map[old_iterm];
      }
      if (new_iterm) {
        new_iterm->connect(new_net);
        debugPrint(logger,
                   utl::ODB,
                   "replace_design",
                   1,
                   "  connected iterm {} to mod net {}",
                   new_iterm->getName(),
                   new_net->getName());
      } else {
        logger->error(utl::ODB,
                      462,
                      "Instance terminal {} cannot be connected to mod net {} "
                      "because it does not exist",
                      old_iterm->getName(),
                      new_net->getName());
      }
    }
  }
}

void dbModule::copyModuleBoundaryIO(dbModule* old_module,
                                    dbModule* new_module,
                                    dbModInst* new_mod_inst)
{
  utl::Logger* logger = old_module->getImpl()->getLogger();
  // Establish "parent/child" port connections
  // dbModBTerm is the port seen from inside the dbModule ("child")
  // dbModITerm is the port seen from outside from the dbModInst ("parent")
  dbSet<dbModITerm> mod_iterms = new_mod_inst->getModITerms();
  std::string msg = fmt::format(
      "copyModuleBoundaryIO for new mod inst {} with {} mod iterms",
      new_mod_inst->getName(),
      mod_iterms.size());
  debugPrint(logger, utl::ODB, "replace_design", 1, msg);
  for (dbModITerm* new_mod_iterm : mod_iterms) {
    // Connect outside dbModITerm to inside dbModBTerm
    dbModBTerm* new_mod_bterm
        = new_module->findModBTerm(new_mod_iterm->getName());
    if (new_mod_bterm) {
      new_mod_iterm->setChildModBTerm(new_mod_bterm);
      new_mod_bterm->setParentModITerm(new_mod_iterm);
      msg = "Created parent/child port connection";
      debugPrint(logger, utl::ODB, "replace_design", 1, msg);
      msg = fmt::format("  parent mod iterm: {}, child mod bterm: {}",
                        new_mod_iterm->getName(),
                        new_mod_bterm->getName());
      debugPrint(logger, utl::ODB, "replace_design", 1, msg);
    } else {
      msg = fmt::format(
          "Parent/child port connection cannot be created for parent "
          "mod iterm {} because child mod bterm does not exist",
          new_mod_iterm->getName());
      logger->error(utl::ODB, 463, msg);
    }
  }
}

// Copy contents of module from current top block to a child block
// such that the module can be deleted from the caller.
// This is used after a module is swapped for a new module.
// The old module shouldn't be deleted because this may be needed later.
// Saving it to a child block serves two purposes:
// 1. It is saved for future module swap
// 2. Optimization avoids iterating any unused instances of the old module
// Return true if copy is successful.
bool dbModule::copyToChildBlock(dbModule* module)
{
  utl::Logger* logger = module->getImpl()->getLogger();

  debugPrint(logger,
             utl::ODB,
             "replace_design",
             1,
             ">>> Copying old module to a child block <<<");

  // Create a new child block under top block.
  // This block contains only one module
  dbBlock* top_block = module->getOwner()->getTopModule()->getOwner();
  std::string block_name = module->getName();
  // TODO: strip out instance name from block name
  dbTech* tech = top_block->getTech();
  dbBlock* child_block = dbBlock::create(top_block, block_name.c_str(), tech);
  child_block->setDefUnits(tech->getLefUnits());
  child_block->setBusDelimiters('[', ']');
  dbModule* new_module = child_block->getTopModule();
  if (!new_module) {
    logger->error(utl::ODB,
                  476,
                  "Top module {} could not be found under child block {}",
                  block_name,
                  block_name);
    return false;
  }

  modBTMap mod_bt_map;
  copyModulePorts(module, new_module, mod_bt_map);
  ITMap it_map;
  copyModuleInsts(module, new_module, nullptr, it_map);
  copyModuleModNets(module, new_module, mod_bt_map, it_map);
  return true;
}

// User Code End dbModulePublicMethods
}  // namespace odb
   // Generator Code End Cpp
