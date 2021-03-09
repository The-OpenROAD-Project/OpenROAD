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

// Generator Code Begin cpp
#include "dbModInst.h"

#include "db.h"
#include "dbBlock.h"
#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbHashTable.hpp"
#include "dbModule.h"
#include "dbTable.h"
#include "dbTable.hpp"
// User Code Begin includes
#include "dbGroup.h"
// User Code End includes
namespace odb {

// User Code Begin definitions
// User Code End definitions
template class dbTable<_dbModInst>;

bool _dbModInst::operator==(const _dbModInst& rhs) const
{
  if (_name != rhs._name)
    return false;

  if (_next_entry != rhs._next_entry)
    return false;

  if (_parent != rhs._parent)
    return false;

  if (_module_next != rhs._module_next)
    return false;

  if (_master != rhs._master)
    return false;

  if (_group_next != rhs._group_next)
    return false;

  if (_group != rhs._group)
    return false;

  // User Code Begin ==
  // User Code End ==
  return true;
}
bool _dbModInst::operator<(const _dbModInst& rhs) const
{
  // User Code Begin <
  if (strcmp(_name, rhs._name) >= 0)
    return false;
  // User Code End <
  return true;
}
void _dbModInst::differences(dbDiff&           diff,
                             const char*       field,
                             const _dbModInst& rhs) const
{
  DIFF_BEGIN

  DIFF_FIELD(_name);
  DIFF_FIELD(_next_entry);
  DIFF_FIELD(_parent);
  DIFF_FIELD(_module_next);
  DIFF_FIELD(_master);
  DIFF_FIELD(_group_next);
  DIFF_FIELD(_group);
  // User Code Begin differences
  // User Code End differences
  DIFF_END
}
void _dbModInst::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_name);
  DIFF_OUT_FIELD(_next_entry);
  DIFF_OUT_FIELD(_parent);
  DIFF_OUT_FIELD(_module_next);
  DIFF_OUT_FIELD(_master);
  DIFF_OUT_FIELD(_group_next);
  DIFF_OUT_FIELD(_group);

  // User Code Begin out
  // User Code End out
  DIFF_END
}
_dbModInst::_dbModInst(_dbDatabase* db)
{
  // User Code Begin constructor
  _name        = 0;
  _parent      = 0;
  _module_next = 0;
  _master      = 0;
  _group       = 0;
  _group_next  = 0;
  // User Code End constructor
}
_dbModInst::_dbModInst(_dbDatabase* db, const _dbModInst& r)
{
  _name        = r._name;
  _next_entry  = r._next_entry;
  _parent      = r._parent;
  _module_next = r._module_next;
  _master      = r._master;
  _group_next  = r._group_next;
  _group       = r._group;
  // User Code Begin CopyConstructor
  // User Code End CopyConstructor
}

dbIStream& operator>>(dbIStream& stream, _dbModInst& obj)
{
  stream >> obj._name;
  stream >> obj._next_entry;
  stream >> obj._parent;
  stream >> obj._module_next;
  stream >> obj._master;
  stream >> obj._group_next;
  stream >> obj._group;
  // User Code Begin >>
  // User Code End >>
  return stream;
}
dbOStream& operator<<(dbOStream& stream, const _dbModInst& obj)
{
  stream << obj._name;
  stream << obj._next_entry;
  stream << obj._parent;
  stream << obj._module_next;
  stream << obj._master;
  stream << obj._group_next;
  stream << obj._group;
  // User Code Begin <<
  // User Code End <<
  return stream;
}

_dbModInst::~_dbModInst()
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
// dbModInst - Methods
//
////////////////////////////////////////////////////////////////////

dbModule* dbModInst::getParent() const
{
  _dbModInst* obj = (_dbModInst*) this;
  if (obj->_parent == 0)
    return NULL;
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbModule*) par->_module_tbl->getPtr(obj->_parent);
}

dbModule* dbModInst::getMaster() const
{
  _dbModInst* obj = (_dbModInst*) this;
  if (obj->_master == 0)
    return NULL;
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbModule*) par->_module_tbl->getPtr(obj->_master);
}

dbGroup* dbModInst::getGroup() const
{
  _dbModInst* obj = (_dbModInst*) this;
  if (obj->_group == 0)
    return NULL;
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbGroup*) par->_group_tbl->getPtr(obj->_group);
}

// User Code Begin dbModInstPublicMethods
dbModInst* dbModInst::create(dbModule*   parentModule,
                             dbModule*   masterModule,
                             const char* name)
{
  _dbModule*  parent = (_dbModule*) parentModule;
  _dbBlock*   block  = (_dbBlock*) parent->getOwner();
  std::string h_name = std::string(parent->_name) + '/' + std::string(name);
  if (block->_modinst_hash.hasMember(h_name.c_str()))
    return nullptr;
  _dbModule* master = (_dbModule*) masterModule;
  if (master->_mod_inst != 0)
    return nullptr;
  _dbModInst* modinst = block->_modinst_tbl->create();
  modinst->_name      = strdup(h_name.c_str());
  ZALLOCATED(modinst->_name);
  modinst->_master      = master->getOID();
  modinst->_parent      = parent->getOID();
  modinst->_module_next = parent->_modinsts;
  parent->_modinsts     = modinst->getOID();
  master->_mod_inst     = modinst->getOID();
  block->_modinst_hash.insert(modinst);
  return (dbModInst*) modinst;
}

void dbModInst::destroy(dbModInst* modinst)
{
  _dbModInst* _modinst = (_dbModInst*) modinst;
  _dbBlock*   block    = (_dbBlock*) _modinst->getOwner();
  _dbModule*  module   = (_dbModule*) modinst->getParent();

  // unlink from parent start
  uint        id   = _modinst->getOID();
  _dbModInst* prev = NULL;
  uint        cur  = module->_modinsts;
  while (cur) {
    _dbModInst* c = block->_modinst_tbl->getPtr(cur);
    if (cur == id) {
      if (prev == NULL)
        module->_modinsts = _modinst->_module_next;
      else
        prev->_module_next = _modinst->_module_next;
      break;
    }
    prev = c;
    cur  = c->_module_next;
  }
  // unlink from parent end
  if (_modinst->_group)
    modinst->getGroup()->removeModInst(modinst);
  dbProperty::destroyProperties(_modinst);
  block->_modinst_hash.remove(_modinst);
  block->_modinst_tbl->destroy(_modinst);
}
dbSet<dbModInst>::iterator dbModInst::destroy(dbSet<dbModInst>::iterator& itr)
{
  dbModInst*                 modinst = *itr;
  dbSet<dbModInst>::iterator next    = ++itr;
  destroy(modinst);
  return next;
}

dbModInst* dbModInst::getModInst(dbBlock* block_, uint dbid_)
{
  _dbBlock* block = (_dbBlock*) block_;
  return (dbModInst*) block->_modinst_tbl->getPtr(dbid_);
}

std::string dbModInst::getName() const
{
  _dbModInst* obj    = (_dbModInst*) this;
  std::string h_name = std::string(obj->_name);
  size_t      idx    = h_name.find_last_of('/');
  return h_name.substr(idx + 1);
}

std::string dbModInst::getHierarchalName() const
{
  _dbModInst* _obj      = (_dbModInst*) this;
  dbBlock*    block     = (dbBlock*) _obj->getOwner();
  std::string inst_name = getName();
  dbModule*   parent    = getParent();
  if (parent == block->getTopModule())
    return inst_name;
  else
    return parent->getModInst()->getHierarchalName() + "/" + inst_name;
}
// User Code End dbModInstPublicMethods
}  // namespace odb
   // Generator Code End cpp