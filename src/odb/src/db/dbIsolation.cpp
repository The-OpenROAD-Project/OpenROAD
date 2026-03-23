// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbIsolation.h"

#include <cstdlib>
#include <string>

#include "dbBlock.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbHashTable.hpp"
#include "dbMaster.h"
#include "dbNet.h"
#include "dbPowerDomain.h"
#include "dbTable.h"
#include "odb/db.h"
// User Code Begin Includes
#include <vector>

#include "dbCommon.h"
#include "odb/dbTypes.h"
// User Code End Includes
namespace odb {
template class dbTable<_dbIsolation>;

bool _dbIsolation::operator==(const _dbIsolation& rhs) const
{
  if (name_ != rhs.name_) {
    return false;
  }
  if (next_entry_ != rhs.next_entry_) {
    return false;
  }
  if (applies_to_ != rhs.applies_to_) {
    return false;
  }
  if (clamp_value_ != rhs.clamp_value_) {
    return false;
  }
  if (isolation_signal_ != rhs.isolation_signal_) {
    return false;
  }
  if (isolation_sense_ != rhs.isolation_sense_) {
    return false;
  }
  if (location_ != rhs.location_) {
    return false;
  }
  if (power_domain_ != rhs.power_domain_) {
    return false;
  }

  return true;
}

bool _dbIsolation::operator<(const _dbIsolation& rhs) const
{
  return true;
}

_dbIsolation::_dbIsolation(_dbDatabase* db)
{
  name_ = nullptr;
}

dbIStream& operator>>(dbIStream& stream, _dbIsolation& obj)
{
  stream >> obj.name_;
  stream >> obj.next_entry_;
  stream >> obj.applies_to_;
  stream >> obj.clamp_value_;
  stream >> obj.isolation_signal_;
  stream >> obj.isolation_sense_;
  stream >> obj.location_;
  stream >> obj.isolation_cells_;
  stream >> obj.power_domain_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbIsolation& obj)
{
  stream << obj.name_;
  stream << obj.next_entry_;
  stream << obj.applies_to_;
  stream << obj.clamp_value_;
  stream << obj.isolation_signal_;
  stream << obj.isolation_sense_;
  stream << obj.location_;
  stream << obj.isolation_cells_;
  stream << obj.power_domain_;
  return stream;
}

void _dbIsolation::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  // User Code Begin collectMemInfo
  info.children["name"].add(name_);
  info.children["applies_to"].add(applies_to_);
  info.children["clamp_value"].add(clamp_value_);
  info.children["isolation_signal"].add(isolation_signal_);
  info.children["isolation_sense"].add(isolation_sense_);
  info.children["location"].add(location_);
  info.children["isolation_cells"].add(isolation_cells_);
  // User Code End collectMemInfo
}

_dbIsolation::~_dbIsolation()
{
  if (name_) {
    free((void*) name_);
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
  return obj->name_;
}

std::string dbIsolation::getAppliesTo() const
{
  _dbIsolation* obj = (_dbIsolation*) this;
  return obj->applies_to_;
}

std::string dbIsolation::getClampValue() const
{
  _dbIsolation* obj = (_dbIsolation*) this;
  return obj->clamp_value_;
}

std::string dbIsolation::getIsolationSignal() const
{
  _dbIsolation* obj = (_dbIsolation*) this;
  return obj->isolation_signal_;
}

std::string dbIsolation::getIsolationSense() const
{
  _dbIsolation* obj = (_dbIsolation*) this;
  return obj->isolation_sense_;
}

std::string dbIsolation::getLocation() const
{
  _dbIsolation* obj = (_dbIsolation*) this;
  return obj->location_;
}

void dbIsolation::setPowerDomain(dbPowerDomain* power_domain)
{
  _dbIsolation* obj = (_dbIsolation*) this;

  obj->power_domain_ = power_domain->getImpl()->getOID();
}

dbPowerDomain* dbIsolation::getPowerDomain() const
{
  _dbIsolation* obj = (_dbIsolation*) this;
  if (obj->power_domain_ == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbPowerDomain*) par->powerdomain_tbl_->getPtr(obj->power_domain_);
}

// User Code Begin dbIsolationPublicMethods
dbIsolation* dbIsolation::create(dbBlock* block, const char* name)
{
  _dbBlock* _block = (_dbBlock*) block;
  if (_block->isolation_hash_.hasMember(name)) {
    return nullptr;
  }
  _dbIsolation* iso = _block->isolation_tbl_->create();
  iso->name_ = safe_strdup(name);

  _block->isolation_hash_.insert(iso);
  return (dbIsolation*) iso;
}

void dbIsolation::destroy(dbIsolation* iso)
{
  // TODO
}

void dbIsolation::setAppliesTo(const std::string& applies_to)
{
  _dbIsolation* obj = (_dbIsolation*) this;
  obj->applies_to_ = applies_to;
}

void dbIsolation::setClampValue(const std::string& clamp_value)
{
  _dbIsolation* obj = (_dbIsolation*) this;
  obj->clamp_value_ = clamp_value;
}

void dbIsolation::setIsolationSignal(const std::string& isolation_signal)
{
  _dbIsolation* obj = (_dbIsolation*) this;
  obj->isolation_signal_ = isolation_signal;
}

void dbIsolation::setIsolationSense(const std::string& isolation_sense)
{
  _dbIsolation* obj = (_dbIsolation*) this;
  obj->isolation_sense_ = isolation_sense;
}

void dbIsolation::setLocation(const std::string& location)
{
  _dbIsolation* obj = (_dbIsolation*) this;
  obj->location_ = location;
}

void dbIsolation::addIsolationCell(const std::string& master)
{
  _dbIsolation* obj = (_dbIsolation*) this;
  obj->isolation_cells_.push_back(master);
}

std::vector<dbMaster*> dbIsolation::getIsolationCells()
{
  _dbIsolation* obj = (_dbIsolation*) this;
  std::vector<dbMaster*> masters;

  for (const auto& cell : obj->isolation_cells_) {
    masters.push_back(obj->getDb()->findMaster(cell.c_str()));
  }

  return masters;
}

bool dbIsolation::appliesTo(const dbIoType& io)
{
  _dbIsolation* obj = (_dbIsolation*) this;

  if (io == dbIoType::OUTPUT) {
    if (obj->applies_to_ == "inputs") {
      return false;
    }
  } else if (io == dbIoType::INPUT) {
    if (obj->applies_to_ == "outputs") {
      return false;
    }
  }

  // default "both"
  return true;
}

// User Code End dbIsolationPublicMethods
}  // namespace odb
   // Generator Code End Cpp
