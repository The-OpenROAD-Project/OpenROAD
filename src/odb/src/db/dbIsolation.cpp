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
#include "dbIsolation.h"

#include "dbBlock.h"
#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbHashTable.hpp"
#include "dbMaster.h"
#include "dbNet.h"
#include "dbPowerDomain.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/db.h"
namespace odb {
template class dbTable<_dbIsolation>;

bool _dbIsolation::operator==(const _dbIsolation& rhs) const
{
  if (_name != rhs._name) {
    return false;
  }
  if (_next_entry != rhs._next_entry) {
    return false;
  }
  if (_applies_to != rhs._applies_to) {
    return false;
  }
  if (_clamp_value != rhs._clamp_value) {
    return false;
  }
  if (_isolation_signal != rhs._isolation_signal) {
    return false;
  }
  if (_isolation_sense != rhs._isolation_sense) {
    return false;
  }
  if (_location != rhs._location) {
    return false;
  }
  if (_power_domain != rhs._power_domain) {
    return false;
  }

  return true;
}

bool _dbIsolation::operator<(const _dbIsolation& rhs) const
{
  return true;
}

void _dbIsolation::differences(dbDiff& diff,
                               const char* field,
                               const _dbIsolation& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(_name);
  DIFF_FIELD(_next_entry);
  DIFF_FIELD(_applies_to);
  DIFF_FIELD(_clamp_value);
  DIFF_FIELD(_isolation_signal);
  DIFF_FIELD(_isolation_sense);
  DIFF_FIELD(_location);
  DIFF_FIELD(_power_domain);
  DIFF_END
}

void _dbIsolation::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_name);
  DIFF_OUT_FIELD(_next_entry);
  DIFF_OUT_FIELD(_applies_to);
  DIFF_OUT_FIELD(_clamp_value);
  DIFF_OUT_FIELD(_isolation_signal);
  DIFF_OUT_FIELD(_isolation_sense);
  DIFF_OUT_FIELD(_location);
  DIFF_OUT_FIELD(_power_domain);

  DIFF_END
}

_dbIsolation::_dbIsolation(_dbDatabase* db)
{
  _name = nullptr;
}

_dbIsolation::_dbIsolation(_dbDatabase* db, const _dbIsolation& r)
{
  _name = r._name;
  _next_entry = r._next_entry;
  _applies_to = r._applies_to;
  _clamp_value = r._clamp_value;
  _isolation_signal = r._isolation_signal;
  _isolation_sense = r._isolation_sense;
  _location = r._location;
  _power_domain = r._power_domain;
}

dbIStream& operator>>(dbIStream& stream, _dbIsolation& obj)
{
  stream >> obj._name;
  stream >> obj._next_entry;
  stream >> obj._applies_to;
  stream >> obj._clamp_value;
  stream >> obj._isolation_signal;
  stream >> obj._isolation_sense;
  stream >> obj._location;
  stream >> obj._isolation_cells;
  stream >> obj._power_domain;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbIsolation& obj)
{
  stream << obj._name;
  stream << obj._next_entry;
  stream << obj._applies_to;
  stream << obj._clamp_value;
  stream << obj._isolation_signal;
  stream << obj._isolation_sense;
  stream << obj._location;
  stream << obj._isolation_cells;
  stream << obj._power_domain;
  return stream;
}

_dbIsolation::~_dbIsolation()
{
  if (_name) {
    free((void*) _name);
  }
}

////////////////////////////////////////////////////////////////////
//
// dbIsolation - Methods
//
////////////////////////////////////////////////////////////////////

const char* dbIsolation::getName() const
{
  _dbIsolation* obj = (_dbIsolation*) this;
  return obj->_name;
}

std::string dbIsolation::getAppliesTo() const
{
  _dbIsolation* obj = (_dbIsolation*) this;
  return obj->_applies_to;
}

std::string dbIsolation::getClampValue() const
{
  _dbIsolation* obj = (_dbIsolation*) this;
  return obj->_clamp_value;
}

std::string dbIsolation::getIsolationSignal() const
{
  _dbIsolation* obj = (_dbIsolation*) this;
  return obj->_isolation_signal;
}

std::string dbIsolation::getIsolationSense() const
{
  _dbIsolation* obj = (_dbIsolation*) this;
  return obj->_isolation_sense;
}

std::string dbIsolation::getLocation() const
{
  _dbIsolation* obj = (_dbIsolation*) this;
  return obj->_location;
}

void dbIsolation::setPowerDomain(dbPowerDomain* power_domain)
{
  _dbIsolation* obj = (_dbIsolation*) this;

  obj->_power_domain = power_domain->getImpl()->getOID();
}

dbPowerDomain* dbIsolation::getPowerDomain() const
{
  _dbIsolation* obj = (_dbIsolation*) this;
  if (obj->_power_domain == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbPowerDomain*) par->_powerdomain_tbl->getPtr(obj->_power_domain);
}

// User Code Begin dbIsolationPublicMethods
dbIsolation* dbIsolation::create(dbBlock* block, const char* name)
{
  _dbBlock* _block = (_dbBlock*) block;
  if (_block->_isolation_hash.hasMember(name)) {
    return nullptr;
  }
  _dbIsolation* iso = _block->_isolation_tbl->create();
  iso->_name = strdup(name);
  ZALLOCATED(iso->_name);

  _block->_isolation_hash.insert(iso);
  return (dbIsolation*) iso;
}

void dbIsolation::destroy(dbIsolation* iso)
{
  // TODO
}

void dbIsolation::setAppliesTo(const std::string& applies_to)
{
  _dbIsolation* obj = (_dbIsolation*) this;
  obj->_applies_to = applies_to;
}

void dbIsolation::setClampValue(const std::string& clamp_value)
{
  _dbIsolation* obj = (_dbIsolation*) this;
  obj->_clamp_value = clamp_value;
}

void dbIsolation::setIsolationSignal(const std::string& isolation_signal)
{
  _dbIsolation* obj = (_dbIsolation*) this;
  obj->_isolation_signal = isolation_signal;
}

void dbIsolation::setIsolationSense(const std::string& isolation_sense)
{
  _dbIsolation* obj = (_dbIsolation*) this;
  obj->_isolation_sense = isolation_sense;
}

void dbIsolation::setLocation(const std::string& location)
{
  _dbIsolation* obj = (_dbIsolation*) this;
  obj->_location = location;
}

void dbIsolation::addIsolationCell(std::string& master)
{
  _dbIsolation* obj = (_dbIsolation*) this;
  obj->_isolation_cells.push_back(master);
}

std::vector<dbMaster*> dbIsolation::getIsolationCells()
{
  _dbIsolation* obj = (_dbIsolation*) this;
  std::vector<dbMaster*> masters;

  for (const auto& cell : obj->_isolation_cells) {
    masters.push_back(obj->getDb()->findMaster(cell.c_str()));
  }

  return masters;
}
// User Code End dbIsolationPublicMethods
}  // namespace odb
   // Generator Code End Cpp