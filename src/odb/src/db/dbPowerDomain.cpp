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
#include "odb/db.h"
// User Code Begin Includes
#include <cmath>

#include "dbGroup.h"
#include "dbLevelShifter.h"
// User Code End Includes
namespace odb {
template class dbTable<_dbPowerDomain>;

bool _dbPowerDomain::operator==(const _dbPowerDomain& rhs) const
{
  if (_name != rhs._name) {
    return false;
  }
  if (_next_entry != rhs._next_entry) {
    return false;
  }
  if (_group != rhs._group) {
    return false;
  }
  if (_top != rhs._top) {
    return false;
  }
  if (_parent != rhs._parent) {
    return false;
  }
  if (_area != rhs._area) {
    return false;
  }
  if (_voltage != rhs._voltage) {
    return false;
  }

  return true;
}

bool _dbPowerDomain::operator<(const _dbPowerDomain& rhs) const
{
  return true;
}

void _dbPowerDomain::differences(dbDiff& diff,
                                 const char* field,
                                 const _dbPowerDomain& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(_name);
  DIFF_FIELD(_next_entry);
  DIFF_FIELD(_group);
  DIFF_FIELD(_top);
  DIFF_FIELD(_parent);
  DIFF_FIELD(_area);
  DIFF_FIELD(_voltage);
  DIFF_END
}

void _dbPowerDomain::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_name);
  DIFF_OUT_FIELD(_next_entry);
  DIFF_OUT_FIELD(_group);
  DIFF_OUT_FIELD(_top);
  DIFF_OUT_FIELD(_parent);
  DIFF_OUT_FIELD(_area);
  DIFF_OUT_FIELD(_voltage);

  DIFF_END
}

_dbPowerDomain::_dbPowerDomain(_dbDatabase* db)
{
  _name = nullptr;
  _top = false;
  _voltage = 0;
  // User Code Begin Constructor
  _area.mergeInit();
  // User Code End Constructor
}

_dbPowerDomain::_dbPowerDomain(_dbDatabase* db, const _dbPowerDomain& r)
{
  _name = r._name;
  _next_entry = r._next_entry;
  _group = r._group;
  _top = r._top;
  _parent = r._parent;
  _area = r._area;
  _voltage = r._voltage;
}

dbIStream& operator>>(dbIStream& stream, _dbPowerDomain& obj)
{
  stream >> obj._name;
  stream >> obj._next_entry;
  stream >> obj._elements;
  stream >> obj._power_switch;
  stream >> obj._isolation;
  stream >> obj._group;
  stream >> obj._top;
  stream >> obj._parent;
  stream >> obj._area;
  // User Code Begin >>
  if (stream.getDatabase()->isSchema(db_schema_level_shifter)) {
    stream >> obj._levelshifters;
  }

  if (stream.getDatabase()->isSchema(db_schema_power_domain_voltage)) {
    stream >> obj._voltage;
  }
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
  stream << obj._group;
  stream << obj._top;
  stream << obj._parent;
  stream << obj._area;
  // User Code Begin <<
  stream << obj._levelshifters;
  stream << obj._voltage;
  // User Code End <<
  return stream;
}

_dbPowerDomain::~_dbPowerDomain()
{
  if (_name) {
    free((void*) _name);
  }
}

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

dbGroup* dbPowerDomain::getGroup() const
{
  _dbPowerDomain* obj = (_dbPowerDomain*) this;
  if (obj->_group == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbGroup*) par->_group_tbl->getPtr(obj->_group);
}

void dbPowerDomain::setTop(bool top)
{
  _dbPowerDomain* obj = (_dbPowerDomain*) this;

  obj->_top = top;
}

bool dbPowerDomain::isTop() const
{
  _dbPowerDomain* obj = (_dbPowerDomain*) this;
  return obj->_top;
}

void dbPowerDomain::setParent(dbPowerDomain* parent)
{
  _dbPowerDomain* obj = (_dbPowerDomain*) this;

  obj->_parent = parent->getImpl()->getOID();
}

dbPowerDomain* dbPowerDomain::getParent() const
{
  _dbPowerDomain* obj = (_dbPowerDomain*) this;
  if (obj->_parent == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbPowerDomain*) par->_powerdomain_tbl->getPtr(obj->_parent);
}

void dbPowerDomain::setVoltage(float voltage)
{
  _dbPowerDomain* obj = (_dbPowerDomain*) this;

  obj->_voltage = voltage;
}

float dbPowerDomain::getVoltage() const
{
  _dbPowerDomain* obj = (_dbPowerDomain*) this;
  return obj->_voltage;
}

// User Code Begin dbPowerDomainPublicMethods
dbPowerDomain* dbPowerDomain::create(dbBlock* block, const char* name)
{
  _dbBlock* _block = (_dbBlock*) block;
  if (_block->_powerdomain_hash.hasMember(name)) {
    return nullptr;
  }
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

void dbPowerDomain::setGroup(dbGroup* group)
{
  _dbPowerDomain* obj = (_dbPowerDomain*) this;
  _dbGroup* _group = (_dbGroup*) group;
  obj->_group = _group->getOID();
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

void dbPowerDomain::addLevelShifter(dbLevelShifter* shifter)
{
  _dbPowerDomain* obj = (_dbPowerDomain*) this;
  obj->_levelshifters.push_back(shifter->getImpl()->getOID());
}

std::vector<dbPowerSwitch*> dbPowerDomain::getPowerSwitches()
{
  _dbPowerDomain* obj = (_dbPowerDomain*) this;
  _dbBlock* par = (_dbBlock*) obj->getOwner();

  std::vector<dbPowerSwitch*> switches;

  for (const auto& ps : obj->_power_switch) {
    switches.push_back((dbPowerSwitch*) par->_powerswitch_tbl->getPtr(ps));
  }

  return switches;
}

std::vector<dbIsolation*> dbPowerDomain::getIsolations()
{
  _dbPowerDomain* obj = (_dbPowerDomain*) this;
  _dbBlock* par = (_dbBlock*) obj->getOwner();

  std::vector<dbIsolation*> isolations;

  for (const auto& iso : obj->_isolation) {
    isolations.push_back((dbIsolation*) par->_isolation_tbl->getPtr(iso));
  }

  return isolations;
}

std::vector<dbLevelShifter*> dbPowerDomain::getLevelShifters()
{
  _dbPowerDomain* obj = (_dbPowerDomain*) this;
  _dbBlock* par = (_dbBlock*) obj->getOwner();

  std::vector<dbLevelShifter*> levelshifters;

  for (const auto& shifter : obj->_levelshifters) {
    levelshifters.push_back(
        (dbLevelShifter*) par->_levelshifter_tbl->getPtr(shifter));
  }

  return levelshifters;
}

void dbPowerDomain::setArea(const Rect& area)
{
  _dbPowerDomain* obj = (_dbPowerDomain*) this;
  obj->_area = area;
}

bool dbPowerDomain::getArea(Rect& area)
{
  _dbPowerDomain* obj = (_dbPowerDomain*) this;
  if (obj->_area.isInverted()) {  // area unset
    return false;
  }

  area = obj->_area;
  return true;
}

// User Code End dbPowerDomainPublicMethods
}  // namespace odb
// Generator Code End Cpp
