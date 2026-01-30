// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbPowerDomain.h"

#include <cstdlib>

#include "dbBlock.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbHashTable.hpp"
#include "dbIsolation.h"
#include "dbModInst.h"
#include "dbPowerSwitch.h"
#include "dbTable.h"
#include "dbVector.h"
#include "odb/db.h"
// User Code Begin Includes
#include <cmath>
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
  if (group_ != rhs.group_) {
    return false;
  }
  if (top_ != rhs.top_) {
    return false;
  }
  if (parent_ != rhs.parent_) {
    return false;
  }
  if (area_ != rhs.area_) {
    return false;
  }
  if (voltage_ != rhs.voltage_) {
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
  top_ = false;
  voltage_ = 0;
  // User Code Begin Constructor
  area_.mergeInit();
  // User Code End Constructor
}

dbIStream& operator>>(dbIStream& stream, _dbPowerDomain& obj)
{
  stream >> obj.name_;
  stream >> obj.next_entry_;
  stream >> obj.elements_;
  stream >> obj.power_switch_;
  stream >> obj.isolation_;
  stream >> obj.group_;
  stream >> obj.top_;
  stream >> obj.parent_;
  stream >> obj.area_;
  // User Code Begin >>
  if (stream.getDatabase()->isSchema(kSchemaLevelShifter)) {
    stream >> obj.levelshifters_;
  }

  if (stream.getDatabase()->isSchema(kSchemaPowerDomainVoltage)) {
    stream >> obj.voltage_;
  }
  // User Code End >>
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbPowerDomain& obj)
{
  stream << obj.name_;
  stream << obj.next_entry_;
  stream << obj.elements_;
  stream << obj.power_switch_;
  stream << obj.isolation_;
  stream << obj.group_;
  stream << obj.top_;
  stream << obj.parent_;
  stream << obj.area_;
  // User Code Begin <<
  stream << obj.levelshifters_;
  stream << obj.voltage_;
  // User Code End <<
  return stream;
}

void _dbPowerDomain::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  // User Code Begin collectMemInfo
  info.children["name"].add(name_);
  info.children["elements"].add(elements_);
  info.children["power_switch"].add(power_switch_);
  info.children["isolation"].add(isolation_);
  info.children["levelshifters"].add(levelshifters_);
  // User Code End collectMemInfo
}

_dbPowerDomain::~_dbPowerDomain()
{
  if (name_) {
    free((void*) name_);
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
  return obj->name_;
}

dbGroup* dbPowerDomain::getGroup() const
{
  _dbPowerDomain* obj = (_dbPowerDomain*) this;
  if (obj->group_ == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbGroup*) par->group_tbl_->getPtr(obj->group_);
}

void dbPowerDomain::setTop(bool top)
{
  _dbPowerDomain* obj = (_dbPowerDomain*) this;

  obj->top_ = top;
}

bool dbPowerDomain::isTop() const
{
  _dbPowerDomain* obj = (_dbPowerDomain*) this;
  return obj->top_;
}

void dbPowerDomain::setParent(dbPowerDomain* parent)
{
  _dbPowerDomain* obj = (_dbPowerDomain*) this;

  obj->parent_ = parent->getImpl()->getOID();
}

dbPowerDomain* dbPowerDomain::getParent() const
{
  _dbPowerDomain* obj = (_dbPowerDomain*) this;
  if (obj->parent_ == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbPowerDomain*) par->powerdomain_tbl_->getPtr(obj->parent_);
}

void dbPowerDomain::setVoltage(float voltage)
{
  _dbPowerDomain* obj = (_dbPowerDomain*) this;

  obj->voltage_ = voltage;
}

float dbPowerDomain::getVoltage() const
{
  _dbPowerDomain* obj = (_dbPowerDomain*) this;
  return obj->voltage_;
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
  obj->elements_.push_back(element);
}

void dbPowerDomain::setGroup(dbGroup* group)
{
  _dbPowerDomain* obj = (_dbPowerDomain*) this;
  _dbGroup* _group = (_dbGroup*) group;
  obj->group_ = _group->getOID();
}

std::vector<std::string> dbPowerDomain::getElements()
{
  _dbPowerDomain* obj = (_dbPowerDomain*) this;
  return obj->elements_;
}

void dbPowerDomain::addPowerSwitch(dbPowerSwitch* ps)
{
  _dbPowerDomain* obj = (_dbPowerDomain*) this;
  obj->power_switch_.push_back(ps->getImpl()->getOID());
}
void dbPowerDomain::addIsolation(dbIsolation* iso)
{
  _dbPowerDomain* obj = (_dbPowerDomain*) this;
  obj->isolation_.push_back(iso->getImpl()->getOID());
}

void dbPowerDomain::addLevelShifter(dbLevelShifter* shifter)
{
  _dbPowerDomain* obj = (_dbPowerDomain*) this;
  obj->levelshifters_.push_back(shifter->getImpl()->getOID());
}

std::vector<dbPowerSwitch*> dbPowerDomain::getPowerSwitches()
{
  _dbPowerDomain* obj = (_dbPowerDomain*) this;
  _dbBlock* par = (_dbBlock*) obj->getOwner();

  std::vector<dbPowerSwitch*> switches;

  for (const auto& ps : obj->power_switch_) {
    switches.push_back((dbPowerSwitch*) par->powerswitch_tbl_->getPtr(ps));
  }

  return switches;
}

std::vector<dbIsolation*> dbPowerDomain::getIsolations()
{
  _dbPowerDomain* obj = (_dbPowerDomain*) this;
  _dbBlock* par = (_dbBlock*) obj->getOwner();

  std::vector<dbIsolation*> isolations;

  for (const auto& iso : obj->isolation_) {
    isolations.push_back((dbIsolation*) par->isolation_tbl_->getPtr(iso));
  }

  return isolations;
}

std::vector<dbLevelShifter*> dbPowerDomain::getLevelShifters()
{
  _dbPowerDomain* obj = (_dbPowerDomain*) this;
  _dbBlock* par = (_dbBlock*) obj->getOwner();

  std::vector<dbLevelShifter*> levelshifters;

  for (const auto& shifter : obj->levelshifters_) {
    levelshifters.push_back(
        (dbLevelShifter*) par->levelshifter_tbl_->getPtr(shifter));
  }

  return levelshifters;
}

void dbPowerDomain::setArea(const Rect& area)
{
  _dbPowerDomain* obj = (_dbPowerDomain*) this;
  obj->area_ = area;
}

bool dbPowerDomain::getArea(Rect& area)
{
  _dbPowerDomain* obj = (_dbPowerDomain*) this;
  if (obj->area_.isInverted()) {  // area unset
    return false;
  }

  area = obj->area_;
  return true;
}

// User Code End dbPowerDomainPublicMethods
}  // namespace odb
// Generator Code End Cpp
