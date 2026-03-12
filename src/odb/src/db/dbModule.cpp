// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbModule.h"

#include <cstdlib>

#include "dbBlock.h"
#include "dbCommon.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbHashTable.hpp"
#include "dbInst.h"
#include "dbJournal.h"
#include "dbModBTerm.h"
#include "dbModInst.h"
#include "dbModulePortItr.h"
#include "dbTable.h"
#include "odb/db.h"
// User Code Begin Includes
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "dbModNet.h"
#include "dbModuleInstItr.h"
#include "dbModuleModBTermItr.h"
#include "dbModuleModInstItr.h"
#include "dbModuleModNetItr.h"
#include "odb/dbBlockCallBackObj.h"
#include "odb/dbObject.h"
#include "odb/dbSet.h"
#include "utl/Logger.h"
// User Code End Includes
namespace odb {
template class dbTable<_dbModule>;

bool _dbModule::operator==(const _dbModule& rhs) const
{
  if (name_ != rhs.name_) {
    return false;
  }
  if (next_entry_ != rhs.next_entry_) {
    return false;
  }
  if (insts_ != rhs.insts_) {
    return false;
  }
  if (mod_inst_ != rhs.mod_inst_) {
    return false;
  }
  if (modinsts_ != rhs.modinsts_) {
    return false;
  }
  if (modnets_ != rhs.modnets_) {
    return false;
  }
  if (modbterms_ != rhs.modbterms_) {
    return false;
  }

  return true;
}

bool _dbModule::operator<(const _dbModule& rhs) const
{
  // User Code Begin <
  if (strcmp(name_, rhs.name_) >= 0) {
    return false;
  }
  // User Code End <
  return true;
}

_dbModule::_dbModule(_dbDatabase* db)
{
  // User Code Begin Constructor
  name_ = nullptr;
  insts_ = 0;
  modinsts_ = 0;
  mod_inst_ = 0;
  // User Code End Constructor
}

dbIStream& operator>>(dbIStream& stream, _dbModule& obj)
{
  stream >> obj.name_;
  stream >> obj.next_entry_;
  stream >> obj.insts_;
  stream >> obj.mod_inst_;
  stream >> obj.modinsts_;
  if (obj.getDatabase()->isSchema(kSchemaUpdateHierarchy)) {
    stream >> obj.modnets_;
  }
  if (obj.getDatabase()->isSchema(kSchemaUpdateHierarchy)) {
    stream >> obj.modbterms_;
  }
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbModule& obj)
{
  stream << obj.name_;
  stream << obj.next_entry_;
  stream << obj.insts_;
  stream << obj.mod_inst_;
  stream << obj.modinsts_;
  stream << obj.modnets_;
  stream << obj.modbterms_;
  return stream;
}

void _dbModule::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  // User Code Begin collectMemInfo
  info.children["name"].add(name_);
  info.children["_dbinst_hash"].add(dbinst_hash_);
  info.children["_modinst_hash"].add(modinst_hash_);
  info.children["_modbterm_hash"].add(modbterm_hash_);
  info.children["_modnet_hash"].add(modnet_hash_);
  // User Code End collectMemInfo
}

_dbModule::~_dbModule()
{
  if (name_) {
    free((void*) name_);
  }
  // User Code Begin Destructor
  delete _port_iter;
  // User Code End Destructor
}

////////////////////////////////////////////////////////////////////
//
// dbModule - Methods
//
////////////////////////////////////////////////////////////////////

const char* dbModule::getName() const
{
  _dbModule* obj = (_dbModule*) this;
  return obj->name_;
}

void dbModule::setModInst(dbModInst* mod_inst)
{
  _dbModule* obj = (_dbModule*) this;

  obj->mod_inst_ = mod_inst->getImpl()->getOID();
}

dbModInst* dbModule::getModInst() const
{
  _dbModule* obj = (_dbModule*) this;
  if (obj->mod_inst_ == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbModInst*) par->modinst_tbl_->getPtr(obj->mod_inst_);
}

// User Code Begin dbModulePublicMethods

dbModule* dbModule::getParentModule() const
{
  dbModInst* mod_inst = getModInst();
  return (mod_inst != nullptr) ? mod_inst->getParent() : nullptr;
}

const dbModBTerm* dbModule::getHeadDbModBTerm() const
{
  _dbModule* obj = (_dbModule*) this;
  _dbBlock* block_ = (_dbBlock*) obj->getOwner();
  if (obj->modbterms_ == 0) {
    return nullptr;
  }
  // note that the odb objects are "pre-pended"
  // so first object is at tail. This means we are returning
  // last object added. The application calling this routine
  // needs to be aware of this (and possibly skip to the end
  // of the list and then use prev to reconstruct creation order).
  return (dbModBTerm*) (block_->modbterm_tbl_->getPtr(obj->modbterms_));
}

int dbModule::getModInstCount()
{
  _dbModule* module = (_dbModule*) this;
  _dbBlock* block = (_dbBlock*) module->getOwner();
  return (int) ((dbSet<dbModInst>(module, block->module_modinst_itr_)).size());
}

int dbModule::getDbInstCount()
{
  _dbModule* module = (_dbModule*) this;
  _dbBlock* block = (_dbBlock*) module->getOwner();
  return (int) (dbSet<dbInst>(module, block->module_inst_itr_).size());
}

void dbModule::addInst(dbInst* inst)
{
  _dbModule* module = (_dbModule*) this;
  _dbInst* _inst = (_dbInst*) inst;
  _dbBlock* block = (_dbBlock*) module->getOwner();

  if (isTop() == false && _inst->flags_.physical_only) {
    _inst->getLogger()->error(
        utl::ODB,
        297,
        "Physical only instance {} can't be added to module {}",
        inst->getName(),
        getName());
  }

  if (_inst->module_ == module->getOID()) {
    return;  // already in this module
  }

  if (_inst->flags_.dont_touch) {
    _inst->getLogger()->error(
        utl::ODB,
        367,
        "Attempt to change the module of dont_touch instance {}",
        _inst->name_);
  }

  if (_inst->module_ != 0) {
    dbModule* mod = dbModule::getModule((dbBlock*) block, _inst->module_);
    ((_dbModule*) mod)->removeInst(inst);
  }

  _inst->module_ = module->getOID();
  module->dbinst_hash_[inst->getName()] = dbId<_dbInst>(_inst->getOID());

  if (module->insts_ == 0) {
    _inst->module_next_ = 0;
    _inst->module_prev_ = 0;
    module->insts_ = _inst->getOID();
  } else {
    _dbInst* cur_head = block->inst_tbl_->getPtr(module->insts_);
    _inst->module_next_ = module->insts_;
    module->insts_ = _inst->getOID();
    cur_head->module_prev_ = _inst->getOID();
  }
}

void _dbModule::removeInst(dbInst* inst)
{
  _dbModule* module = (_dbModule*) this;
  _dbInst* _inst = (_dbInst*) inst;
  uint32_t id = _inst->getOID();

  if (_inst->module_ != getOID()) {
    return;
  }

  if (_inst->flags_.dont_touch) {
    _inst->getLogger()->error(
        utl::ODB,
        371,
        "Attempt to remove dont_touch instance {} from parent module",
        _inst->name_);
  }

  _dbBlock* block = (_dbBlock*) getOwner();

  if (module->insts_ == id) {
    module->insts_ = _inst->module_next_;

    if (module->insts_ != 0) {
      _dbInst* t = block->inst_tbl_->getPtr(module->insts_);
      t->module_prev_ = 0;
    }
  } else {
    if (_inst->module_next_ != 0) {
      _dbInst* next = block->inst_tbl_->getPtr(_inst->module_next_);
      next->module_prev_ = _inst->module_prev_;
    }

    if (_inst->module_prev_ != 0) {
      _dbInst* prev = block->inst_tbl_->getPtr(_inst->module_prev_);
      prev->module_next_ = _inst->module_next_;
    }
  }
  _inst->module_ = 0;
  _inst->module_next_ = 0;
  _inst->module_prev_ = 0;
}

dbSet<dbModInst> dbModule::getChildren() const
{
  _dbModule* module = (_dbModule*) this;
  _dbBlock* block = (_dbBlock*) module->getOwner();
  return dbSet<dbModInst>(module, block->module_modinst_itr_);
}

dbSet<dbModNet> dbModule::getModNets()
{
  _dbModule* module = (_dbModule*) this;
  _dbBlock* block = (_dbBlock*) module->getOwner();
  return dbSet<dbModNet>(module, block->module_modnet_itr_);
}

dbModNet* dbModule::getModNet(const char* net_name) const
{
  const _dbModule* module = (const _dbModule*) this;
  const _dbBlock* block = (const _dbBlock*) module->getOwner();
  auto it = module->modnet_hash_.find(net_name);
  if (it != module->modnet_hash_.end()) {
    uint32_t db_id = (*it).second;
    return (dbModNet*) block->modnet_tbl_->getPtr(db_id);
  }
  return nullptr;
}

dbSet<dbModInst> dbModule::getModInsts() const
{
  _dbModule* module = (_dbModule*) this;
  _dbBlock* block = (_dbBlock*) module->getOwner();
  return dbSet<dbModInst>(module, block->module_modinst_itr_);
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
    obj->_port_iter = new dbModulePortItr(block->modbterm_tbl_);
  }
  return dbSet<dbModBTerm>(this, obj->_port_iter);
}

//
// The modbterms are the leaf level connections
//"flat view"
//
dbSet<dbModBTerm> dbModule::getModBTerms() const
{
  _dbModule* module = (_dbModule*) this;
  _dbBlock* block = (_dbBlock*) module->getOwner();
  return dbSet<dbModBTerm>(module, block->module_modbterm_itr_);
}

dbModBTerm* dbModule::getModBTerm(uint32_t id)
{
  _dbModule* module = (_dbModule*) this;
  _dbBlock* block = (_dbBlock*) module->getOwner();
  return (dbModBTerm*) (block->modbterm_tbl_->getObject(id));
}

dbSet<dbInst> dbModule::getInsts() const
{
  _dbModule* module = (_dbModule*) this;
  _dbBlock* block = (_dbBlock*) module->getOwner();
  return dbSet<dbInst>(module, block->module_inst_itr_);
}

dbModule* dbModule::create(dbBlock* block, const char* name)
{
  _dbBlock* _block = (_dbBlock*) block;
  if (_block->module_hash_.hasMember(name)) {
    return nullptr;
  }
  _dbModule* module = _block->module_tbl_->create();
  module->name_ = safe_strdup(name);
  _block->module_hash_.insert(module);

  debugPrint(block->getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             1,
             "EDIT: create {}",
             module->getDebugName());

  if (_block->journal_) {
    _block->journal_->beginAction(dbJournal::kCreateObject);
    _block->journal_->pushParam(dbModuleObj);
    _block->journal_->pushParam(module->name_);
    _block->journal_->pushParam(module->getId());
    _block->journal_->endAction();
  }

  for (dbBlockCallBackObj* cb : _block->callbacks_) {
    cb->inDbModuleCreate((dbModule*) module);
  }

  return (dbModule*) module;
}

void dbModule::destroy(dbModule* module)
{
  _dbModule* _module = (_dbModule*) module;
  _dbBlock* block = (_dbBlock*) _module->getOwner();

  if (block->top_module_ == module->getId()) {
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

  if (_module->mod_inst_ != 0) {
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
    dbModBTerm* modbterm = *modbterm_itr;
    // as a side effect this will journal the disconnection
    // so it can be undone.
    modbterm->disconnect();
    modbterm_itr = dbModBTerm::destroy(modbterm_itr);
  }

  // destroy the modnets last. At this point we expect them
  // to be totally disconnected (we have disconnected them
  // from any dbModules, dbModInst and dbITerm/dbBTerm.
  //
  dbSet<dbModNet> modnets = module->getModNets();
  dbSet<dbModNet>::iterator modnet_itr;
  for (modnet_itr = modnets.begin(); modnet_itr != modnets.end();) {
    modnet_itr = dbModNet::destroy(modnet_itr);
  }

  for (auto cb : block->callbacks_) {
    cb->inDbModuleDestroy(module);
  }

  dbProperty::destroyProperties(_module);

  debugPrint(block->getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             1,
             "EDIT: delete {}",
             module->getDebugName());

  // Journal the deletion of the dbModule after its ports
  // and properties deleted, so that on restore we have
  // dbModule to hang objects on.
  if (block->journal_) {
    block->journal_->beginAction(dbJournal::kDeleteObject);
    block->journal_->pushParam(dbModuleObj);
    block->journal_->pushParam(module->getName());
    block->journal_->pushParam(module->getId());
    block->journal_->endAction();
  }

  block->module_hash_.remove(_module);
  block->module_tbl_->destroy(_module);
}

dbModule* dbModule::getModule(dbBlock* block_, uint32_t dbid_)
{
  _dbBlock* block = (_dbBlock*) block_;
  return (dbModule*) block->module_tbl_->getPtr(dbid_);
}

dbModInst* dbModule::findModInst(const char* name) const
{
  const _dbModule* obj = (const _dbModule*) this;
  const _dbBlock* par = (const _dbBlock*) obj->getOwner();
  auto it = obj->modinst_hash_.find(name);
  if (it != obj->modinst_hash_.end()) {
    auto db_id = (*it).second;
    return (dbModInst*) par->modinst_tbl_->getPtr(db_id);
  }
  return nullptr;
}

dbInst* dbModule::findDbInst(const char* name) const
{
  const _dbModule* obj = (const _dbModule*) this;
  const _dbBlock* par = (const _dbBlock*) obj->getOwner();
  auto it = obj->dbinst_hash_.find(name);
  if (it != obj->dbinst_hash_.end()) {
    auto db_id = (*it).second;
    return (dbInst*) par->inst_tbl_->getPtr(db_id);
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

dbModBTerm* dbModule::findModBTerm(const char* name) const
{
  std::string modbterm_name(name);
  const char hier_delimiter = getOwner()->getHierarchyDelimiter();
  size_t last_idx = modbterm_name.find_last_of(hier_delimiter);
  if (last_idx != std::string::npos) {
    modbterm_name = modbterm_name.substr(last_idx + 1);
  }
  const _dbModule* obj = (const _dbModule*) this;
  const _dbBlock* par = (const _dbBlock*) obj->getOwner();
  auto it = obj->modbterm_hash_.find(modbterm_name);
  if (it != obj->modbterm_hash_.end()) {
    auto db_id = (*it).second;
    return (dbModBTerm*) par->modbterm_tbl_->getPtr(db_id);
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

dbBlock* dbModule::getOwner() const
{
  const _dbModule* obj = (const _dbModule*) this;
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

  std::unordered_map<std::string, int>& name_id_map
      = ((_dbBlock*) block)->module_name_id_map_;
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

// Check if two hierarchical modules are swappable.
// Two modules must have identical number of ports and port names need to match.
// Functional equivalence is not required.
bool dbModule::canSwapWith(dbModule* new_module) const
{
  const _dbModule* module_impl = reinterpret_cast<const _dbModule*>(this);
  const std::string old_module_name = getName();
  const std::string new_module_name = new_module->getName();

  // Check if module names differ
  if (old_module_name == new_module_name) {
    module_impl->getLogger()->warn(
        utl::ODB,
        470,
        "The modules cannot be swapped because the new module name {} is "
        "identical to the existing module name.",
        new_module_name);
    return false;
  }

  // Check if number of module ports match
  dbSet<dbModBTerm> old_bterms = getModBTerms();
  dbSet<dbModBTerm> new_bterms = new_module->getModBTerms();
  if (old_bterms.size() != new_bterms.size()) {
    module_impl->getLogger()->warn(
        utl::ODB,
        453,
        "The modules cannot be swapped because module {} has {} ports but "
        "module {} has {} ports.",
        old_module_name,
        old_bterms.size(),
        new_module_name,
        new_bterms.size());
    return false;
  }

  // Check if module port names match
  for (dbModBTerm* old_bterm : old_bterms) {
    if (new_module->findModBTerm(old_bterm->getName()) == nullptr) {
      module_impl->getLogger()->warn(utl::ODB,
                                     454,
                                     "The modules cannot be swapped because "
                                     "module {} has port {} which is "
                                     "not in module {}.",
                                     old_module_name,
                                     old_bterm->getName(),
                                     new_module_name);
      return false;
    }
  }

  return true;
}

bool dbModule::isTop() const
{
  const _dbModule* obj = reinterpret_cast<const _dbModule*>(this);
  const dbBlock* block = static_cast<dbBlock*>(obj->getOwner());
  if (block == nullptr) {
    return false;
  }
  return (block->getTopModule() == this);
}

// Do a "deep" copy of old_module based on its instance context into new_module.
// - NOTE: _dbModule::copyModulePorts needs to be called before this function
//   to populate mod_bt_map.
// - All instances, mod nets and parent/child IO will be copied.
// - Connections that span multiple modules needs to be done outside this API.
// - new_mod_inst is needed to create module instances for instance name
//   uniquification.
void _dbModule::copy(dbModule* old_module,
                     dbModule* new_module,
                     dbModInst* new_mod_inst,
                     modBTMap& mod_bt_map)
{
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
void _dbModule::copyModulePorts(dbModule* old_module,
                                dbModule* new_module,
                                modBTMap& mod_bt_map)
{
  utl::Logger* logger = old_module->getImpl()->getLogger();
  for (dbModBTerm* old_port : old_module->getModBTerms()) {
    dbModBTerm* new_port = nullptr;
    if (mod_bt_map.contains(old_port)) {
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

void _dbModule::copyModuleInsts(dbModule* old_module,
                                dbModule* new_module,
                                dbModInst* new_mod_inst,
                                ITMap& it_map)
{
  dbBlock* block = new_module->getOwner();
  const char hier_delimiter = block->getHierarchyDelimiter();
  utl::Logger* logger = old_module->getImpl()->getLogger();

  // Create a net name map (key: new net name, value: new dbNet*).
  std::map<std::string, dbNet*> new_net_name_map;

  // Add insts to new module
  for (dbInst* old_inst : old_module->getInsts()) {
    // Decide an instance name.
    // - Note that new_mod_inst can be null when the corresponding dbModule is
    //   not instantiated.
    std::string new_inst_name;
    if (new_mod_inst) {
      new_inst_name = new_mod_inst->getHierarchicalName();
      new_inst_name += hier_delimiter;
    }

    new_inst_name += block->getBaseName(old_inst->getConstName());

    // Create an instance of the same master
    dbInst* new_inst = dbInst::makeUniqueDbInst(new_module->getOwner(),
                                                old_inst->getMaster(),
                                                new_inst_name.c_str(),
                                                /* phyical only */ false,
                                                new_module);
    if (new_inst) {
      debugPrint(logger,
                 utl::ODB,
                 "replace_design",
                 1,
                 "Created module instance '{}' with master '{}'",
                 new_inst->getName(),
                 old_inst->getMaster()->getName());
    } else {
      logger->error(utl::ODB,
                    13,
                    "Module instance '{}' with master '{}' cannot be created",
                    new_inst_name,
                    old_inst->getMaster()->getName());
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
      if (old_net == nullptr) {
        continue;
      }

      //
      // Create a local net only if it connects to iterms inside this module
      //
      std::string new_net_name;
      if (new_mod_inst) {
        new_net_name = new_mod_inst->getHierarchicalName();
        new_net_name += hier_delimiter;
      }

      // Check if the flat net is an internal net within old_module
      // - If the old net is an internal net, a new net should be created.
      // - If the old net is an external net, a new net will be created later
      //   by boundary IO handling logic.
      // - Note that if old modnet is connected to a dbModBTerm and its
      //   corresponding dbModITerm is unconnected (has_parent_modnet == false),
      //   a new net should be created.
      // - If old_module is uninstantiated module, every net in the module is
      //   an internal net.
      //   e.g., No module instance.
      //         net_name = "_001_"     <-- Internal net.
      //
      // - Otherwise, an internal net should have the hierarchy prefix
      //   (= module instance hierarchical name).
      //   e.g., modinst_name = "u0/alu0"
      //         net_name = u0/alu0/_001_   <-- Internal net.
      //         net_name = u0/_001_        <-- External net crossing module
      //                                        boundary.
      std::string old_net_name = old_net->getName();
      dbModNet* old_mod_net = old_iterm->getModNet();
      bool has_parent_modnet
          = (old_mod_net && old_mod_net->getFirstParentModNet());
      if (old_net->isInternalTo(old_module) == false && has_parent_modnet) {
        // Skip external net crossing module boundary.
        // It will be connected later.
        debugPrint(logger,
                   utl::ODB,
                   "replace_design",
                   3,
                   "    Skip: non-internal dbNet '{}' of old_module '{}'.\n",
                   old_net_name,
                   old_module->getHierarchicalName());
        continue;
      }

      new_net_name += block->getBaseName(old_net_name.c_str());
      auto it = new_net_name_map.find(new_net_name);
      if (it != new_net_name_map.end()) {
        // Connect to an existing local net
        dbNet* new_net = (*it).second;
        new_iterm->connect(new_net);
        debugPrint(logger,
                   utl::ODB,
                   "replace_design",
                   1,
                   "  connected iterm '{}' to existing local net '{}'",
                   new_iterm->getName(),
                   new_net->getName());
      } else {
        // Create and connect to a new local net
        assert(block->findNet(new_net_name.c_str()) == nullptr);
        dbNet* new_net
            = dbNet::create(new_module->getOwner(), new_net_name.c_str());
        new_iterm->connect(new_net);
        debugPrint(logger,
                   utl::ODB,
                   "replace_design",
                   1,
                   "  Connected iterm '{}' to new local net '{}'",
                   new_iterm->getName(),
                   new_net->getName());

        // Insert it to the map
        new_net_name_map[new_net_name] = new_net;
      }
    }
  }

  if (new_module->getInsts().reversible()
      && new_module->getInsts().orderReversed()) {
    new_module->getInsts().reverse();
  }
}

void _dbModule::copyModuleModNets(dbModule* old_module,
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
    dbModNet* new_net = dbModNet::create(new_module, old_net->getConstName());
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
      if (mod_bt_map.contains(old_mbterm)) {
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
      if (it_map.contains(old_iterm)) {
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

void _dbModule::copyModuleBoundaryIO(dbModule* old_module,
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
bool _dbModule::copyToChildBlock(dbModule* module)
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
  dbTech* tech = top_block->getTech();
  // TODO: strip out instance name from block name
  dbBlock* child_block = dbBlock::create(top_block, block_name.c_str());
  if (child_block) {
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
    _dbModule::copyModulePorts(module, new_module, mod_bt_map);
    ITMap it_map;
    copyModuleInsts(module, new_module, nullptr, it_map);
    copyModuleModNets(module, new_module, mod_bt_map, it_map);
  }
  return true;
}

bool dbModule::containsDbInst(dbInst* inst) const
{
  // Check direct child dbInsts
  for (dbInst* child_inst : getInsts()) {
    if (child_inst == inst) {
      return true;
    }
  }

  // Recursively check child dbModInsts
  for (dbModInst* child_mod_inst : getChildren()) {
    if (child_mod_inst->containsDbInst(inst)) {
      return true;
    }
  }

  return false;
}

bool dbModule::containsDbModInst(dbModInst* inst) const
{
  // Recursively check child dbModInsts
  for (dbModInst* child_mod_inst : getModInsts()) {
    if (child_mod_inst == inst || child_mod_inst->containsDbModInst(inst)) {
      return true;
    }
  }

  return false;
}

// User Code End dbModulePublicMethods
}  // namespace odb
   // Generator Code End Cpp
