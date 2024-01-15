///////////////////////////////////////////////////////////////////////////////
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
#include "dbModBTerm.h"

#include "db.h"
#include "dbBlock.h"
#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbHashTable.hpp"
#include "dbModNet.h"
#include "dbModule.h"
#include "dbTable.h"
#include "dbTable.hpp"
namespace odb {
template class dbTable<_dbModBTerm>;

bool _dbModBTerm::operator==(const _dbModBTerm& rhs) const
{
  if (_name != rhs._name) {
    return false;
  }
  if (_flags != rhs._flags) {
    return false;
  }
  if (_parent != rhs._parent) {
    return false;
  }
  if (_net != rhs._net) {
    return false;
  }

  return true;
}

bool _dbModBTerm::operator<(const _dbModBTerm& rhs) const
{
  return true;
}

void _dbModBTerm::differences(dbDiff& diff,
                              const char* field,
                              const _dbModBTerm& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(_name);
  DIFF_FIELD(_flags);
  DIFF_FIELD(_parent);
  DIFF_FIELD(_net);
  DIFF_END
}

void _dbModBTerm::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_name);
  DIFF_OUT_FIELD(_flags);
  DIFF_OUT_FIELD(_parent);
  DIFF_OUT_FIELD(_net);

  DIFF_END
}

_dbModBTerm::_dbModBTerm(_dbDatabase* db)
{
}

_dbModBTerm::_dbModBTerm(_dbDatabase* db, const _dbModBTerm& r)
{
  _name = r._name;
  _flags = r._flags;
  _parent = r._parent;
  _net = r._net;
}

dbIStream& operator>>(dbIStream& stream, _dbModBTerm& obj)
{
  stream >> obj._name;
  stream >> obj._flags;
  stream >> obj._parent;
  stream >> obj._net;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbModBTerm& obj)
{
  stream << obj._name;
  stream << obj._flags;
  stream << obj._parent;
  stream << obj._net;
  return stream;
}

_dbModBTerm::~_dbModBTerm()
{
  if (_name) {
    free((void*) _name);
  }
}

////////////////////////////////////////////////////////////////////
//
// dbModBTerm - Methods
//
////////////////////////////////////////////////////////////////////

const char* dbModBTerm::getName() const
{
  _dbModBTerm* obj = (_dbModBTerm*) this;
  return obj->_name;
}

void dbModBTerm::setFlags(uint flags)
{
  _dbModBTerm* obj = (_dbModBTerm*) this;

  obj->_flags = flags;
}

uint dbModBTerm::getFlags() const
{
  _dbModBTerm* obj = (_dbModBTerm*) this;
  return obj->_flags;
}

dbModule* dbModBTerm::getParent() const
{
  _dbModBTerm* obj = (_dbModBTerm*) this;
  if (obj->_parent == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbModule*) par->_module_tbl->getPtr(obj->_parent);
}

void dbModBTerm::setNet(dbModNet* net)
{
  _dbModBTerm* obj = (_dbModBTerm*) this;

  obj->_net = net->getImpl()->getOID();
}

dbModNet* dbModBTerm::getNet() const
{
  _dbModBTerm* obj = (_dbModBTerm*) this;
  if (obj->_net == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbModNet*) par->_modnet_tbl->getPtr(obj->_net);
}

// User Code Begin dbModBTermPublicMethods

struct dbModBTermFlags_str
{
  dbIoType::Value _iotype : 4;
  dbSigType::Value _sigtype : 4;
  uint _spare_bits : 24;
};

typedef union dbModBTermFlags
{
  struct dbModBTermFlags_str flags;
  uint uint_val;
} dbModBTermFlagsU;

void dbModBTerm::setSigType(dbSigType type)
{
  dbModBTermFlagsU cur_flags;
  cur_flags.uint_val = getFlags();
  cur_flags.flags._sigtype = type.getValue();
  setFlags(cur_flags.uint_val);
}

dbSigType dbModBTerm::getSigType()
{
  dbModBTermFlagsU cur_flags;
  cur_flags.uint_val = getFlags();
  return dbSigType(cur_flags.flags._sigtype);
}

void dbModBTerm::setIoType(dbIoType type)
{
  dbModBTermFlagsU cur_flags;
  cur_flags.uint_val = getFlags();
  cur_flags.flags._iotype = type.getValue();
  setFlags(cur_flags.uint_val);
}

dbIoType dbModBTerm::getIoType()
{
  dbModBTermFlagsU cur_flags;
  cur_flags.uint_val = getFlags();
  return dbIoType(cur_flags.flags._iotype);
}

dbModBTerm* dbModBTerm::create(dbModule* parentModule, const char* name)
{
  _dbModule* module = (_dbModule*) parentModule;
  _dbBlock* block = (_dbBlock*) module->getOwner();

  std::string str_name(name);
  if (module->_port_map.find(str_name) != module->_port_map.end()) {
    dbId<dbModBTerm> el = module->_port_map[str_name];
    return ((dbModule*) module)->getdbModBTerm((dbBlock*) block, el);
  }
  _dbModBTerm* modbterm = block->_modbterm_tbl->create();
  ((dbModBTerm*) modbterm)->setFlags(0U);
  ((dbModBTerm*) modbterm)->setIoType(dbIoType::INPUT);
  ((dbModBTerm*) modbterm)->setSigType(dbSigType::SIGNAL);
  modbterm->_name = strdup(name);
  ZALLOCATED(modbterm->_name);
  modbterm->_parent = module->getOID();
  module->_port_map[std::string(name)] = module->_port_vec.size();
  module->_port_vec.push_back(modbterm->getOID());
  return (dbModBTerm*) modbterm;
}

char* dbModBTerm::getName()
{
  _dbModBTerm* _bterm = (_dbModBTerm*) this;
  return _bterm->_name;
}

bool dbModBTerm::connect(dbModNet* net)
{
  _dbModNet* local_modnet = (_dbModNet*) net;
  _dbModBTerm* _bterm = (_dbModBTerm*) this;
  dbId<_dbModNet> net_id(local_modnet->getOID());
  if (_bterm->_net == net_id)
    return true;
  _bterm->_net = net_id;
  dbId<_dbModBTerm> dest = _bterm->getOID();
  local_modnet->_modbterms.push_back(dest);
  return true;
}

void dbModBTerm::staSetPort(void* p)
{
  _dbModBTerm* _modbterm = (_dbModBTerm*) this;
  _modbterm->_sta_port = p;
}

void* dbModBTerm::staPort()
{
  _dbModBTerm* _modbterm = (_dbModBTerm*) this;
  return _modbterm->_sta_port;
}

// User Code End dbModBTermPublicMethods
}  // namespace odb
   // Generator Code End Cpp
