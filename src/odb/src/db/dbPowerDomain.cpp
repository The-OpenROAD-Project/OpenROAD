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
#include "dbPowerDomain.h"

#include "db.h"
#include "dbBlock.h"
#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbHashTable.hpp"
#include "dbIsolation.h"
#include "dbModInst.h"
#include "dbPowerSwitch.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbVector.h"
// User Code Begin Includes
// User Code End Includes
namespace odb {

template class dbTable<_dbPowerDomain>;

bool _dbPowerDomain::operator==(const _dbPowerDomain& rhs) const
{
  if (_name != rhs._name)
    return false;

  if (_next_entry != rhs._next_entry)
    return false;

  // User Code Begin ==
  // User Code End ==
  return true;
}
bool _dbPowerDomain::operator<(const _dbPowerDomain& rhs) const
{
  // User Code Begin <
  // User Code End <
  return true;
}
void _dbPowerDomain::differences(dbDiff& diff,
                                 const char* field,
                                 const _dbPowerDomain& rhs) const
{
  DIFF_BEGIN

  DIFF_FIELD(_name);
  DIFF_FIELD(_next_entry);
  // User Code Begin Differences
  // User Code End Differences
  DIFF_END
}
void _dbPowerDomain::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_name);
  DIFF_OUT_FIELD(_next_entry);

  // User Code Begin Out
  // User Code End Out
  DIFF_END
}
_dbPowerDomain::_dbPowerDomain(_dbDatabase* db)
{
  // User Code Begin Constructor
  // User Code End Constructor
}
_dbPowerDomain::_dbPowerDomain(_dbDatabase* db, const _dbPowerDomain& r)
{
  _name = r._name;
  _next_entry = r._next_entry;
  // User Code Begin CopyConstructor
  // User Code End CopyConstructor
}

dbIStream& operator>>(dbIStream& stream, _dbPowerDomain& obj)
{
  stream >> obj._name;
  stream >> obj._next_entry;
  stream >> obj._elements;
  stream >> obj._power_switch;
  stream >> obj._isolation;
  // User Code Begin >>
  // User Code End >>
  return stream;
}
dbOStream& operator<<(dbOStream& stream, const _dbPowerDomain& obj)
{
  stream << obj._name;
  stream << obj._next_entry;
  stream << obj._elements;
  stream << obj._power_switch;
  stream << obj._isolation;
  // User Code Begin <<
  // User Code End <<
  return stream;
}

_dbPowerDomain::~_dbPowerDomain()
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
// dbPowerDomain - Methods
//
////////////////////////////////////////////////////////////////////

const char* dbPowerDomain::getName() const
{
  _dbPowerDomain* obj = (_dbPowerDomain*) this;
  return obj->_name;
}

// User Code Begin dbPowerDomainPublicMethods
dbPowerDomain* dbPowerDomain::create(dbBlock* block, const char* name)
{
  _dbBlock* _block = (_dbBlock*) block;
  if (_block->_powerdomain_hash.hasMember(name))
    return nullptr;
  _dbPowerDomain* pd = _block->_powerdomain_tbl->create();
  pd->_name = strdup(name);
  ZALLOCATED(pd->_name);

  _block->_powerdomain_hash.insert(pd);
  return (dbPowerDomain*) pd;
}

void dbPowerDomain::destroy(dbPowerDomain* pd)
{
  // TODO
}

void dbPowerDomain::addElement(const std::string& element)
{
  _dbPowerDomain* obj = (_dbPowerDomain*) this;
  obj->_elements.push_back(element);
}

std::vector<std::string> dbPowerDomain::getElements()
{
  _dbPowerDomain* obj = (_dbPowerDomain*) this;
  return obj->_elements;
}

void dbPowerDomain::addPowerSwitch(dbPowerSwitch* ps)
{
  _dbPowerDomain* obj = (_dbPowerDomain*) this;
  obj->_power_switch.push_back(ps->getImpl()->getOID());
}
void dbPowerDomain::addIsolation(dbIsolation* iso)
{
  _dbPowerDomain* obj = (_dbPowerDomain*) this;
  obj->_isolation.push_back(iso->getImpl()->getOID());
}

std::vector<dbPowerSwitch*> dbPowerDomain::getPowerSwitches()
{
  _dbPowerDomain* obj = (_dbPowerDomain*) this;
  _dbBlock* par = (_dbBlock*) obj->getOwner();

  std::vector<dbPowerSwitch*> switches;

  for (auto ps : obj->_power_switch) {
    switches.push_back((dbPowerSwitch*) par->_powerswitch_tbl->getPtr(ps));
  }

  return switches;
}

std::vector<dbIsolation*> dbPowerDomain::getIsolations()
{
  _dbPowerDomain* obj = (_dbPowerDomain*) this;
  _dbBlock* par = (_dbBlock*) obj->getOwner();

  std::vector<dbIsolation*> isolations;

  for (auto iso : obj->_isolation) {
    isolations.push_back((dbIsolation*) par->_isolation_tbl->getPtr(iso));
  }

  return isolations;
}

// User Code End dbPowerDomainPublicMethods
}  // namespace odb
   // Generator Code End Cpp