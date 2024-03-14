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
  if (_next_net_modbterm != rhs._next_net_modbterm) {
    return false;
  }
  if (_prev_net_modbterm != rhs._prev_net_modbterm) {
    return false;
  }
  if (_next_entry != rhs._next_entry) {
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
  DIFF_FIELD(_next_net_modbterm);
  DIFF_FIELD(_prev_net_modbterm);
  DIFF_FIELD(_next_entry);
  DIFF_END
}

void _dbModBTerm::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_name);
  DIFF_OUT_FIELD(_flags);
  DIFF_OUT_FIELD(_parent);
  DIFF_OUT_FIELD(_net);
  DIFF_OUT_FIELD(_next_net_modbterm);
  DIFF_OUT_FIELD(_prev_net_modbterm);
  DIFF_OUT_FIELD(_next_entry);

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
  _next_net_modbterm = r._next_net_modbterm;
  _prev_net_modbterm = r._prev_net_modbterm;
  _next_entry = r._next_entry;
}

dbIStream& operator>>(dbIStream& stream, _dbModBTerm& obj)
{
  stream >> obj._name;
  stream >> obj._flags;
  stream >> obj._parent;
  stream >> obj._net;
  stream >> obj._next_net_modbterm;
  stream >> obj._prev_net_modbterm;
  stream >> obj._next_entry;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbModBTerm& obj)
{
  stream << obj._name;
  stream << obj._flags;
  stream << obj._parent;
  stream << obj._net;
  stream << obj._next_net_modbterm;
  stream << obj._prev_net_modbterm;
  stream << obj._next_entry;
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

void dbModBTerm::setNextNetModbterm(dbModBTerm* next_net_modbterm)
{
  _dbModBTerm* obj = (_dbModBTerm*) this;

  obj->_next_net_modbterm = next_net_modbterm->getImpl()->getOID();
}

dbModBTerm* dbModBTerm::getNextNetModbterm() const
{
  _dbModBTerm* obj = (_dbModBTerm*) this;
  if (obj->_next_net_modbterm == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbModBTerm*) par->_modbterm_tbl->getPtr(obj->_next_net_modbterm);
}

void dbModBTerm::setPrevNetModbterm(dbModBTerm* prev_net_modbterm)
{
  _dbModBTerm* obj = (_dbModBTerm*) this;

  obj->_prev_net_modbterm = prev_net_modbterm->getImpl()->getOID();
}

dbModBTerm* dbModBTerm::getPrevNetModbterm() const
{
  _dbModBTerm* obj = (_dbModBTerm*) this;
  if (obj->_prev_net_modbterm == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbModBTerm*) par->_modbterm_tbl->getPtr(obj->_prev_net_modbterm);
}

void dbModBTerm::setNextEntry(dbModBTerm* next_entry)
{
  _dbModBTerm* obj = (_dbModBTerm*) this;

  obj->_next_entry = next_entry->getImpl()->getOID();
}

dbModBTerm* dbModBTerm::getNextEntry() const
{
  _dbModBTerm* obj = (_dbModBTerm*) this;
  if (obj->_next_entry == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbModBTerm*) par->_modbterm_tbl->getPtr(obj->_next_entry);
}

// User Code Begin dbModBTermPublicMethods

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
  dbModBTerm* ret = parentModule->findModBTerm(name);
  if (ret) {
    return ret;
  }

  _dbModule* module = (_dbModule*) parentModule;
  _dbBlock* block = (_dbBlock*) module->getOwner();

  std::string str_name(name);
  _dbModBTerm* modbterm = block->_modbterm_tbl->create();
  // defaults
  ((dbModBTerm*) modbterm)->setFlags(0U);
  ((dbModBTerm*) modbterm)->setIoType(dbIoType::INPUT);
  ((dbModBTerm*) modbterm)->setSigType(dbSigType::SIGNAL);
  modbterm->_net = 0;
  modbterm->_next_net_modbterm = 0;
  modbterm->_prev_net_modbterm = 0;

  modbterm->_name = strdup(name);
  ZALLOCATED(modbterm->_name);
  modbterm->_parent = module->getOID();
  modbterm->_next_entry = module->_modbterms;
  module->_modbterms = modbterm->getOID();

  return (dbModBTerm*) modbterm;
}

char* dbModBTerm::getName()
{
  _dbModBTerm* _bterm = (_dbModBTerm*) this;
  return _bterm->_name;
}

void dbModBTerm::connect(dbModNet* net)
{
  //  printf("Connecting mod net %s to modbterm %s\n",
  //	 net -> getName(),
  //	 getName()
  //	 );
  _dbModule* _module = (_dbModule*) (net->getParent());
  _dbBlock* _block = (_dbBlock*) _module->getOwner();
  _dbModNet* _modnet = (_dbModNet*) net;
  _dbModBTerm* _modbterm = (_dbModBTerm*) this;
  // already connected
  if (_modbterm->_net == net->getId())
    return;
  _modbterm->_net = net->getId();
  // append to net mod bterms. Do this by pushing onto head of list.
  if (_modnet->_modbterms != 0) {
    _dbModBTerm* head = _block->_modbterm_tbl->getPtr(_modnet->_modbterms);
    // next is old head
    _modbterm->_next_net_modbterm = _modnet->_modbterms;
    // previous for old head is this one
    head->_prev_net_modbterm = getId();
  } else {
    _modbterm->_next_net_modbterm = 0;  // only element
  }
  _modbterm->_prev_net_modbterm = 0;  // previous of head always zero
  _modnet->_modbterms = getId();      // set new head
  //  printf("Mod net now connected to %d modbterms\n",
  //	 net -> getModBTerms().size());

  return;
}

void dbModBTerm::disconnect()
{
  _dbModule* module = (_dbModule*) getParent();
  _dbBlock* block = (_dbBlock*) module->getOwner();
  _dbModBTerm* _modbterm = (_dbModBTerm*) this;
  if (_modbterm->_net == 0)
    return;
  _dbModNet* mod_net = block->_modnet_tbl->getPtr(_modbterm->_net);

  if (_modbterm->_prev_net_modbterm == 0) {
    // degenerate case, head element, need to update net starting point
    // and if next is null then make generate empty list
    mod_net->_modbterms = _modbterm->_next_net_modbterm;
  } else {
    _dbModBTerm* prev_modbterm
        = block->_modbterm_tbl->getPtr(_modbterm->_prev_net_modbterm);
    prev_modbterm->_next_net_modbterm
        = _modbterm->_next_net_modbterm;  // short out this element
  }
  if (_modbterm->_next_net_modbterm != 0) {
    _dbModBTerm* next_modbterm
        = block->_modbterm_tbl->getPtr(_modbterm->_next_net_modbterm);
    next_modbterm->_prev_net_modbterm = _modbterm->_prev_net_modbterm;
  }

  //
  // zero out this element for garbage collection
  // Not we can never rely on sequential order of modbterms for offsets.
  //
  _modbterm->_next_net_modbterm = 0;
  _modbterm->_prev_net_modbterm = 0;
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
