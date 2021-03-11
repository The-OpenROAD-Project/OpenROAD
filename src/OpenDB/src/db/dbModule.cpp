///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2020, OpenRoad Project
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

#include "db.h"
#include "dbBlock.h"
#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbHashTable.hpp"
#include "dbTable.h"
#include "dbTable.hpp"
// User Code Begin Includes
#include <string>

#include "dbInst.h"
#include "dbModInst.h"
#include "dbModuleInstItr.h"
#include "dbModuleModInstItr.h"
// User Code End Includes
namespace odb {

template class dbTable<_dbModule>;

bool _dbModule::operator==(const _dbModule& rhs) const
{
  if (_name != rhs._name)
    return false;

  if (_next_entry != rhs._next_entry)
    return false;

  if (_insts != rhs._insts)
    return false;

  if (_modinsts != rhs._modinsts)
    return false;

  if (_mod_inst != rhs._mod_inst)
    return false;

  // User Code Begin ==
  // User Code End ==
  return true;
}
bool _dbModule::operator<(const _dbModule& rhs) const
{
  // User Code Begin <
  if (strcmp(_name, rhs._name) >= 0)
    return false;
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
  DIFF_FIELD(_modinsts);
  DIFF_FIELD(_mod_inst);
  // User Code Begin Differences
  // User Code End Differences
  DIFF_END
}
void _dbModule::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_name);
  DIFF_OUT_FIELD(_next_entry);
  DIFF_OUT_FIELD(_insts);
  DIFF_OUT_FIELD(_modinsts);
  DIFF_OUT_FIELD(_mod_inst);

  // User Code Begin Out
  // User Code End Out
  DIFF_END
}
_dbModule::_dbModule(_dbDatabase* db)
{
  // User Code Begin Constructor
  _name = 0;
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
  _modinsts = r._modinsts;
  _mod_inst = r._mod_inst;
  // User Code Begin CopyConstructor
  // User Code End CopyConstructor
}

dbIStream& operator>>(dbIStream& stream, _dbModule& obj)
{
  stream >> obj._name;
  stream >> obj._next_entry;
  stream >> obj._insts;
  stream >> obj._modinsts;
  stream >> obj._mod_inst;
  // User Code Begin >>
  // User Code End >>
  return stream;
}
dbOStream& operator<<(dbOStream& stream, const _dbModule& obj)
{
  stream << obj._name;
  stream << obj._next_entry;
  stream << obj._insts;
  stream << obj._modinsts;
  stream << obj._mod_inst;
  // User Code Begin <<
  // User Code End <<
  return stream;
}

_dbModule::~_dbModule()
{
  if (_name)
    free((void*) _name);
  // User Code Begin Destructor
  // User Code End Destructor
}

// User Code Begin PrivateMethods
// User Code End PrivateMethods

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

dbModInst* dbModule::getModInst() const
{
  _dbModule* obj = (_dbModule*) this;
  if (obj->_mod_inst == 0)
    return NULL;
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbModInst*) par->_modinst_tbl->getPtr(obj->_mod_inst);
}

// User Code Begin dbModulePublicMethods

void dbModule::addInst(dbInst* inst)
{
  _dbModule* module = (_dbModule*) this;
  _dbInst* _inst = (_dbInst*) inst;
  _dbBlock* block = (_dbBlock*) module->getOwner();

  if (_inst->_module != 0) {
    dbModule* mod = dbModule::getModule((dbBlock*) block, _inst->_module);
    mod->removeInst(inst);
  }

  _inst->_module = module->getOID();
  _inst->_module_next = module->_insts;
  module->_insts = _inst->getOID();
}

void dbModule::removeInst(dbInst* inst)
{
  _dbModule* module = (_dbModule*) this;
  _dbInst* _inst = (_dbInst*) inst;
  if (_inst->_module != module->getOID())
    return;
  _dbBlock* block = (_dbBlock*) module->getOwner();
  uint id = _inst->getOID();

  _dbInst* prev = NULL;
  uint cur = module->_insts;
  while (cur) {
    _dbInst* c = block->_inst_tbl->getPtr(cur);
    if (cur == id) {
      if (prev == NULL)
        module->_insts = _inst->_module_next;
      else
        prev->_module_next = _inst->_module_next;
      break;
    }
    prev = c;
    cur = c->_module_next;
  }
  _inst->_module = 0;
  _inst->_module_next = 0;
}

dbSet<dbModInst> dbModule::getChildren()
{
  _dbModule* module = (_dbModule*) this;
  _dbBlock* block = (_dbBlock*) module->getOwner();
  return dbSet<dbModInst>(module, block->_module_modinst_itr);
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
  if (_block->_module_hash.hasMember(name))
    return nullptr;
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

  dbSet<dbModInst> modinsts = module->getChildren();
  dbSet<dbModInst>::iterator itr;
  for (itr = modinsts.begin(); itr != modinsts.end();) {
    itr = dbModInst::destroy(itr);
  }
  if (_module->_mod_inst != 0)
    dbModInst::destroy(module->getModInst());

  for (auto inst : module->getInsts()) {
    _dbInst* _inst = (_dbInst*) inst;
    _inst->_module = 0;
    _inst->_module_next = 0;
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
  std::string h_name = std::string(obj->_name) + "/" + std::string(name);
  return (dbModInst*) par->_modinst_hash.find(h_name.c_str());
}

// User Code End dbModulePublicMethods
}  // namespace odb
   // Generator Code End Cpp