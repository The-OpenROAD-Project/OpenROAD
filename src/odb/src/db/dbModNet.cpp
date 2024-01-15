//////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2022, The Regents of the University of California
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
#include "dbModNet.h"

#include "db.h"
#include "dbBlock.h"
#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbHashTable.hpp"
#include "dbITerm.h"
#include "dbModBTerm.h"
#include "dbModITerm.h"
#include "dbModInst.h"
#include "dbModule.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbVector.h"
namespace odb {
template class dbTable<_dbModNet>;

bool _dbModNet::operator==(const _dbModNet& rhs) const
{
  if (_name != rhs._name) {
    return false;
  }
  if (_parent != rhs._parent) {
    return false;
  }
  if (_next_entry != rhs._next_entry) {
    return false;
  }

  return true;
}

bool _dbModNet::operator<(const _dbModNet& rhs) const
{
  return true;
}

void _dbModNet::differences(dbDiff& diff,
                            const char* field,
                            const _dbModNet& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(_name);
  DIFF_FIELD(_parent);
  DIFF_FIELD(_next_entry);
  DIFF_END
}

void _dbModNet::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_name);
  DIFF_OUT_FIELD(_parent);
  DIFF_OUT_FIELD(_next_entry);

  DIFF_END
}

_dbModNet::_dbModNet(_dbDatabase* db)
{
}

_dbModNet::_dbModNet(_dbDatabase* db, const _dbModNet& r)
{
  _name = r._name;
  _parent = r._parent;
  _next_entry = r._next_entry;
}

dbIStream& operator>>(dbIStream& stream, _dbModNet& obj)
{
  stream >> obj._name;
  stream >> obj._parent;
  stream >> obj._next_entry;
  stream >> obj._moditerms;
  stream >> obj._modbterms;
  stream >> obj._iterms;
  stream >> obj._bterms;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbModNet& obj)
{
  stream << obj._name;
  stream << obj._parent;
  stream << obj._next_entry;
  stream << obj._moditerms;
  stream << obj._modbterms;
  stream << obj._iterms;
  stream << obj._bterms;
  return stream;
}

_dbModNet::~_dbModNet()
{
  if (_name) {
    free((void*) _name);
  }
}

////////////////////////////////////////////////////////////////////
//
// dbModNet - Methods
//
////////////////////////////////////////////////////////////////////

dbModule* dbModNet::getParent() const
{
  _dbModNet* obj = (_dbModNet*) this;
  if (obj->_parent == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbModule*) par->_module_tbl->getPtr(obj->_parent);
}

// User Code Begin dbModNetPublicMethods

const char* dbModNet::getName() const
{
  _dbModNet* obj = (_dbModNet*) this;
  return obj->_name;
}

dbModNet* dbModNet::create(dbModule* parentModule, const char* name)
{
  // give illusion of scoping.
  _dbModule* parent = (_dbModule*) parentModule;
  _dbBlock* block = (_dbBlock*) parent->getOwner();

  std::string name_str(name);
  if (parent->_modnet_map.find(name_str) != parent->_modnet_map.end()) {
    dbId<dbModNet> modnet_id = parent->_modnet_map[name_str];
    _dbModNet* modnet = block->_modnet_tbl->getPtr(modnet_id.id());
    return ((dbModNet*) modnet);
  }
  _dbModNet* modnet = block->_modnet_tbl->create();
  parent->_modnet_map[name_str] = modnet->getOID();
  modnet->_name = strdup(name);
  modnet->_parent = parent->getOID();  // dbmodule
  modnet->_next_entry = parent->_modnets;
  parent->_modnets = modnet->getOID();
  return (dbModNet*) modnet;
}

dbModBTerm* dbModNet::connectedToModBTerm() const
{
  const char* net_name = getName();
  _dbModNet* obj = (_dbModNet*) this;
  dbModule* module = getParent();
  for (auto mib : obj->_modbterms) {
    odb::dbId<odb::dbModBTerm> conv_el(mib.id());
    dbModBTerm* mbterm = module->getdbModBTerm(conv_el);
    const char* mbterm_name = mbterm->getName();
    if (!strcmp(mbterm_name, net_name))
      return mbterm;
  }
  return nullptr;
}

// User Code End dbModNetPublicMethods
}  // namespace odb
   // Generator Code End Cpp
