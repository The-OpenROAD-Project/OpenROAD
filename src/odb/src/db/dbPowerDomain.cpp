// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbPowerDomain.h"

#include "dbBlock.h"
#include "dbDatabase.h"
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
#include <cstdlib>
#include <string>
#include <vector>

#include "dbCommon.h"
#include "dbGroup.h"
#include "dbLevelShifter.h"
// User Code End Includes
namespace odb {
template class dbTable<_dbPowerDomain>;

bool _dbPowerDomain::operator==(const _dbPowerDomain& rhs) const
{
  if (name_ != rhs.name_) {
    return false;
  }
  if (next_entry_ != rhs.next_entry_) {
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

_dbPowerDomain::_dbPowerDomain(_dbDatabase* db)
{
  name_ = nullptr;
  _top = false;
  _voltage = 0;
  // User Code Begin Constructor
  _area.mergeInit();
  // User Code End Constructor
}

dbIStream& operator>>(dbIStream& stream, _dbPowerDomain& obj)
{
  stream >> obj.name_;
  stream >> obj.next_entry_;
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
  stream << obj.name_;
  stream << obj.next_entry_;
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

void _dbPowerDomain::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  // User Code Begin collectMemInfo
  info.children_["name"].add(name_);
  info.children_["elements"].add(_elements);
  info.children_["power_switch"].add(_power_switch);
  info.children_["isolation"].add(_isolation);
  info.children_["levelshifters"].add(_levelshifters);
  // User Code End collectMemInfo
}

////////////////////////////////////////////////////////////////////
//
// dbPowerDomain - Methods
//
////////////////////////////////////////////////////////////////////

const char* dbPowerDomain::getName() const
{
  _dbPowerDomain* obj = (_dbPowerDomain*) this;
  return obj->name_;
}

dbGroup* dbPowerDomain::getGroup() const
{
  _dbPowerDomain* obj = (_dbPowerDomain*) this;
  if (obj->_group == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbGroup*) par->group_tbl_->getPtr(obj->_group);
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
  return (dbPowerDomain*) par->powerdomain_tbl_->getPtr(obj->_parent);
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
  if (_block->powerdomain_hash_.hasMember(name)) {
    return nullptr;
  }
  _dbPowerDomain* pd = _block->powerdomain_tbl_->create();
  pd->name_ = safe_strdup(name);

  _block->powerdomain_hash_.insert(pd);
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
    switches.push_back((dbPowerSwitch*) par->powerswitch_tbl_->getPtr(ps));
  }

  return switches;
}

std::vector<dbIsolation*> dbPowerDomain::getIsolations()
{
  _dbPowerDomain* obj = (_dbPowerDomain*) this;
  _dbBlock* par = (_dbBlock*) obj->getOwner();

  std::vector<dbIsolation*> isolations;

  for (const auto& iso : obj->_isolation) {
    isolations.push_back((dbIsolation*) par->isolation_tbl_->getPtr(iso));
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
        (dbLevelShifter*) par->levelshifter_tbl_->getPtr(shifter));
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
