///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2020, The Regents of the University of California
// All rights reserved.
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

// Generator Code Begin Cpp
#include "dbModule.h"

#include "dbBlock.h"
#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbHashTable.hpp"
#include "dbInst.h"
#include "dbModBTerm.h"
#include "dbModInst.h"
#include "dbModulePortItr.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/db.h"
// User Code Begin Includes
#include <string>

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

void _dbModule::differences(dbDiff& diff,
                            const char* field,
                            const _dbModule& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(_name);
  DIFF_FIELD(_next_entry);
  DIFF_FIELD(_insts);
  DIFF_FIELD(_mod_inst);
  DIFF_FIELD(_modinsts);
  DIFF_FIELD(_modnets);
  DIFF_FIELD(_modbterms);
  DIFF_END
}

void _dbModule::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_name);
  DIFF_OUT_FIELD(_next_entry);
  DIFF_OUT_FIELD(_insts);
  DIFF_OUT_FIELD(_mod_inst);
  DIFF_OUT_FIELD(_modinsts);
  DIFF_OUT_FIELD(_modnets);
  DIFF_OUT_FIELD(_modbterms);

  DIFF_END
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

_dbModule::_dbModule(_dbDatabase* db, const _dbModule& r)
{
  _name = r._name;
  _next_entry = r._next_entry;
  _insts = r._insts;
  _mod_inst = r._mod_inst;
  _modinsts = r._modinsts;
  _modnets = r._modnets;
  _modbterms = r._modbterms;
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
  else {
    return (dbModBTerm*) (block_->_modbterm_tbl->getPtr(obj->_modbterms));
  }
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
  for (auto mnet : getModNets()) {
    if (!strcmp(net_name, mnet->getName())) {
      return mnet;
    }
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

  if (_module->_mod_inst != 0) {
    // Destroying the modInst will destroy this module too.
    dbModInst::destroy(module->getModInst());
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

  for (auto modbterm : module->getModBTerms()) {
    block->_modbterm_tbl->destroy((_dbModBTerm*) modbterm);
  }

  for (auto modnet : module->getModNets()) {
    block->_modnet_tbl->destroy((_dbModNet*) modnet);
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
  for (dbModInst* mod_inst : getModInsts()) {
    if (!strcmp(mod_inst->getName(), name)) {
      return mod_inst;
    }
  }
  return nullptr;
}

dbInst* dbModule::findDbInst(const char* name)
{
  for (dbInst* inst : getInsts()) {
    if (!strcmp(inst->getName().c_str(), name)) {
      return inst;
    }
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
  std::string bterm_name(name);
  size_t last_idx = bterm_name.find_last_of('/');
  if (last_idx != std::string::npos) {
    bterm_name = bterm_name.substr(last_idx + 1);
  }

  for (dbModBTerm* mod_bterm : getModBTerms()) {
    if (!strcmp(mod_bterm->getName(), bterm_name.c_str())) {
      return mod_bterm;
    }
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

// User Code End dbModulePublicMethods
}  // namespace odb
   // Generator Code End Cpp
