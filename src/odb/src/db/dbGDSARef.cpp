// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbGDSARef.h"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "dbCore.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
// User Code Begin Includes
#include "dbGDSLib.h"
// User Code End Includes
namespace odb {
template class dbTable<_dbGDSARef>;

bool _dbGDSARef::operator==(const _dbGDSARef& rhs) const
{
  if (origin_ != rhs.origin_) {
    return false;
  }
  if (lr_ != rhs.lr_) {
    return false;
  }
  if (ul_ != rhs.ul_) {
    return false;
  }
  if (num_rows_ != rhs.num_rows_) {
    return false;
  }
  if (num_columns_ != rhs.num_columns_) {
    return false;
  }
  if (structure_ != rhs.structure_) {
    return false;
  }

  return true;
}

bool _dbGDSARef::operator<(const _dbGDSARef& rhs) const
{
  return true;
}

_dbGDSARef::_dbGDSARef(_dbDatabase* db)
{
  num_rows_ = 1;
  num_columns_ = 1;
}

dbIStream& operator>>(dbIStream& stream, _dbGDSARef& obj)
{
  stream >> obj.origin_;
  stream >> obj.lr_;
  stream >> obj.ul_;
  stream >> obj.propattr_;
  stream >> obj.transform_;
  stream >> obj.num_rows_;
  stream >> obj.num_columns_;
  stream >> obj.structure_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbGDSARef& obj)
{
  stream << obj.origin_;
  stream << obj.lr_;
  stream << obj.ul_;
  stream << obj.propattr_;
  stream << obj.transform_;
  stream << obj.num_rows_;
  stream << obj.num_columns_;
  stream << obj.structure_;
  return stream;
}

void _dbGDSARef::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  // User Code Begin collectMemInfo
  info.children["propattr"].add(propattr_);
  for (auto& [i, s] : propattr_) {
    info.children["propattr"].add(s);
  }
  // User Code End collectMemInfo
}

////////////////////////////////////////////////////////////////////
//
// dbGDSARef - Methods
//
////////////////////////////////////////////////////////////////////

void dbGDSARef::setOrigin(Point origin)
{
  _dbGDSARef* obj = (_dbGDSARef*) this;

  obj->origin_ = origin;
}

Point dbGDSARef::getOrigin() const
{
  _dbGDSARef* obj = (_dbGDSARef*) this;
  return obj->origin_;
}

void dbGDSARef::setLr(Point lr)
{
  _dbGDSARef* obj = (_dbGDSARef*) this;

  obj->lr_ = lr;
}

Point dbGDSARef::getLr() const
{
  _dbGDSARef* obj = (_dbGDSARef*) this;
  return obj->lr_;
}

void dbGDSARef::setUl(Point ul)
{
  _dbGDSARef* obj = (_dbGDSARef*) this;

  obj->ul_ = ul;
}

Point dbGDSARef::getUl() const
{
  _dbGDSARef* obj = (_dbGDSARef*) this;
  return obj->ul_;
}

void dbGDSARef::setTransform(dbGDSSTrans transform)
{
  _dbGDSARef* obj = (_dbGDSARef*) this;

  obj->transform_ = transform;
}

dbGDSSTrans dbGDSARef::getTransform() const
{
  _dbGDSARef* obj = (_dbGDSARef*) this;
  return obj->transform_;
}

void dbGDSARef::setNumRows(int16_t num_rows)
{
  _dbGDSARef* obj = (_dbGDSARef*) this;

  obj->num_rows_ = num_rows;
}

int16_t dbGDSARef::getNumRows() const
{
  _dbGDSARef* obj = (_dbGDSARef*) this;
  return obj->num_rows_;
}

void dbGDSARef::setNumColumns(int16_t num_columns)
{
  _dbGDSARef* obj = (_dbGDSARef*) this;

  obj->num_columns_ = num_columns;
}

int16_t dbGDSARef::getNumColumns() const
{
  _dbGDSARef* obj = (_dbGDSARef*) this;
  return obj->num_columns_;
}

// User Code Begin dbGDSARefPublicMethods

dbGDSStructure* dbGDSARef::getStructure() const
{
  _dbGDSARef* obj = (_dbGDSARef*) this;
  if (obj->structure_ == 0) {
    return nullptr;
  }
  _dbGDSStructure* parent = (_dbGDSStructure*) obj->getOwner();
  _dbGDSLib* lib = (_dbGDSLib*) parent->getOwner();
  return (dbGDSStructure*) lib->gdsstructure_tbl_->getPtr(obj->structure_);
}

std::vector<std::pair<std::int16_t, std::string>>& dbGDSARef::getPropattr()
{
  auto* obj = (_dbGDSARef*) this;
  return obj->propattr_;
}

dbGDSARef* dbGDSARef::create(dbGDSStructure* parent, dbGDSStructure* child)
{
  auto* obj = (_dbGDSStructure*) parent;
  _dbGDSARef* aref = obj->arefs_->create();
  aref->structure_ = child->getImpl()->getOID();
  return (dbGDSARef*) aref;
}

void dbGDSARef::destroy(dbGDSARef* aref)
{
  auto* obj = (_dbGDSARef*) aref;
  auto* structure = (_dbGDSStructure*) obj->getOwner();
  structure->arefs_->destroy(obj);
}

// User Code End dbGDSARefPublicMethods
}  // namespace odb
   // Generator Code End Cpp
