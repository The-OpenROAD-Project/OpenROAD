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
#include "dbModITerm.h"

#include "db.h"
#include "dbBlock.h"
#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbHashTable.hpp"
#include "dbModInst.h"
#include "dbModNet.h"
#include "dbTable.h"
#include "dbTable.hpp"
namespace odb {
template class dbTable<_dbModITerm>;

bool _dbModITerm::operator==(const _dbModITerm& rhs) const
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
  if (_next_net_moditerm != rhs._next_net_moditerm) {
    return false;
  }
  if (_prev_net_moditerm != rhs._prev_net_moditerm) {
    return false;
  }
  if (_next_entry != rhs._next_entry) {
    return false;
  }

  return true;
}

bool _dbModITerm::operator<(const _dbModITerm& rhs) const
{
  return true;
}

void _dbModITerm::differences(dbDiff& diff,
                              const char* field,
                              const _dbModITerm& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(_name);
  DIFF_FIELD(_flags);
  DIFF_FIELD(_parent);
  DIFF_FIELD(_net);
  DIFF_FIELD(_next_net_moditerm);
  DIFF_FIELD(_prev_net_moditerm);
  DIFF_FIELD(_next_entry);
  DIFF_END
}

void _dbModITerm::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_name);
  DIFF_OUT_FIELD(_flags);
  DIFF_OUT_FIELD(_parent);
  DIFF_OUT_FIELD(_net);
  DIFF_OUT_FIELD(_next_net_moditerm);
  DIFF_OUT_FIELD(_prev_net_moditerm);
  DIFF_OUT_FIELD(_next_entry);

  DIFF_END
}

_dbModITerm::_dbModITerm(_dbDatabase* db)
{
}

_dbModITerm::_dbModITerm(_dbDatabase* db, const _dbModITerm& r)
{
  _name = r._name;
  _flags = r._flags;
  _parent = r._parent;
  _net = r._net;
  _next_net_moditerm = r._next_net_moditerm;
  _prev_net_moditerm = r._prev_net_moditerm;
  _next_entry = r._next_entry;
}

dbIStream& operator>>(dbIStream& stream, _dbModITerm& obj)
{
  stream >> obj._name;
  stream >> obj._flags;
  stream >> obj._parent;
  stream >> obj._net;
  stream >> obj._next_net_moditerm;
  stream >> obj._prev_net_moditerm;
  stream >> obj._next_entry;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbModITerm& obj)
{
  stream << obj._name;
  stream << obj._flags;
  stream << obj._parent;
  stream << obj._net;
  stream << obj._next_net_moditerm;
  stream << obj._prev_net_moditerm;
  stream << obj._next_entry;
  return stream;
}

_dbModITerm::~_dbModITerm()
{
  if (_name) {
    free((void*) _name);
  }
}

////////////////////////////////////////////////////////////////////
//
// dbModITerm - Methods
//
////////////////////////////////////////////////////////////////////

dbModInst* dbModITerm::getParent() const
{
  _dbModITerm* obj = (_dbModITerm*) this;
  if (obj->_parent == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbModInst*) par->_modinst_tbl->getPtr(obj->_parent);
}

void dbModITerm::setNet(dbModNet* net)
{
  _dbModITerm* obj = (_dbModITerm*) this;

  obj->_net = net->getImpl()->getOID();
}

dbModNet* dbModITerm::getNet() const
{
  _dbModITerm* obj = (_dbModITerm*) this;
  if (obj->_net == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbModNet*) par->_modnet_tbl->getPtr(obj->_net);
}

void dbModITerm::setNextNetModiterm(dbModITerm* next_net_moditerm)
{
  _dbModITerm* obj = (_dbModITerm*) this;

  obj->_next_net_moditerm = next_net_moditerm->getImpl()->getOID();
}

dbModITerm* dbModITerm::getNextNetModiterm() const
{
  _dbModITerm* obj = (_dbModITerm*) this;
  if (obj->_next_net_moditerm == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbModITerm*) par->_moditerm_tbl->getPtr(obj->_next_net_moditerm);
}

void dbModITerm::setPrevNetModiterm(dbModITerm* prev_net_moditerm)
{
  _dbModITerm* obj = (_dbModITerm*) this;

  obj->_prev_net_moditerm = prev_net_moditerm->getImpl()->getOID();
}

dbModITerm* dbModITerm::getPrevNetModiterm() const
{
  _dbModITerm* obj = (_dbModITerm*) this;
  if (obj->_prev_net_moditerm == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbModITerm*) par->_moditerm_tbl->getPtr(obj->_prev_net_moditerm);
}

void dbModITerm::setNextEntry(dbModITerm* next_entry)
{
  _dbModITerm* obj = (_dbModITerm*) this;

  obj->_next_entry = next_entry->getImpl()->getOID();
}

dbModITerm* dbModITerm::getNextEntry() const
{
  _dbModITerm* obj = (_dbModITerm*) this;
  if (obj->_next_entry == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbModITerm*) par->_moditerm_tbl->getPtr(obj->_next_entry);
}

// User Code Begin dbModITermPublicMethods
void dbModITerm::setFlags(uint flags)
{
  _dbModITerm* obj = (_dbModITerm*) this;

  obj->_flags = flags;
}

uint dbModITerm::getFlags() const
{
  _dbModITerm* obj = (_dbModITerm*) this;
  return obj->_flags;
}

struct dbModITermFlags_str
{
  dbIoType::Value _iotype : 4;
  dbSigType::Value _sigtype : 4;
  uint _spare_bits : 24;
};

typedef union dbModITermFlags
{
  struct dbModITermFlags_str flags;
  uint uint_val;
} dbModITermFlagsU;

void dbModITerm::setSigType(dbSigType type)
{
  dbModITermFlagsU cur_flags;
  cur_flags.uint_val = getFlags();
  cur_flags.flags._sigtype = type.getValue();
  setFlags(cur_flags.uint_val);
}

dbSigType dbModITerm::getSigType()
{
  dbModITermFlagsU cur_flags;
  cur_flags.uint_val = getFlags();
  return dbSigType(cur_flags.flags._sigtype);
}

void dbModITerm::setIoType(dbIoType type)
{
  dbModITermFlagsU cur_flags;
  cur_flags.uint_val = getFlags();
  cur_flags.flags._iotype = type.getValue();
  setFlags(cur_flags.uint_val);
}

dbIoType dbModITerm::getIoType()
{
  dbModITermFlagsU cur_flags;
  cur_flags.uint_val = getFlags();
  return dbIoType(cur_flags.flags._iotype);
}

dbModITerm* dbModITerm::create(dbModInst* parentInstance, const char* name)
{
  _dbModInst* parent = (_dbModInst*) parentInstance;
  _dbBlock* block = (_dbBlock*) parent->getOwner();
  _dbModITerm* moditerm = block->_moditerm_tbl->create();
  // defaults
  moditerm->_net = 0;
  moditerm->_next_net_moditerm = 0;
  moditerm->_prev_net_moditerm = 0;

  ((dbModITerm*) moditerm)->setFlags(0U);
  ((dbModITerm*) moditerm)->setIoType(dbIoType::INPUT);
  ((dbModITerm*) moditerm)->setSigType(dbSigType::SIGNAL);
  moditerm->_name = strdup(name);
  ZALLOCATED(moditerm->_name);

  moditerm->_parent = parent->getOID();
  moditerm->_next_entry = parent->_moditerms;
  parent->_moditerms = moditerm->getOID();

  return (dbModITerm*) moditerm;
}

bool dbModITerm::connect(dbModNet* net)
{
  _dbModITerm* _moditerm = (_dbModITerm*) this;
  _dbModNet* _modnet = (_dbModNet*) net;
  _dbBlock* _block = (_dbBlock*) _moditerm->getOwner();
  // already connected.
  if (_moditerm->_net == _modnet->getId())
    return true;
  _moditerm->_net = _modnet->getId();
  // append to net moditerms
  if (_modnet->_moditerms != 0) {
    _dbModITerm* head = _block->_moditerm_tbl->getPtr(_modnet->_moditerms);
    // next is old head
    _moditerm->_next_net_moditerm = _modnet->_moditerms;
    head->_prev_net_moditerm = getId();
  } else {
    _moditerm->_next_net_moditerm = 0;
  }
  // set up new head
  _moditerm->_prev_net_moditerm = 0;
  _modnet->_moditerms = getId();
  return true;
}

char* dbModITerm::getName()
{
  _dbModITerm* _iterm = (_dbModITerm*) this;
  return _iterm->_name;
}

// User Code End dbModITermPublicMethods
}  // namespace odb
   // Generator Code End Cpp
