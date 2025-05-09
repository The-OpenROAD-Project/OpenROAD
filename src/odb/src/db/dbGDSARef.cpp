// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbGDSARef.h"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "dbDatabase.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/db.h"
#include "odb/dbTypes.h"
// User Code Begin Includes
#include "dbGDSLib.h"
// User Code End Includes
namespace odb {
template class dbTable<_dbGDSARef>;

bool _dbGDSARef::operator==(const _dbGDSARef& rhs) const
{
  if (_origin != rhs._origin) {
    return false;
  }
  if (_lr != rhs._lr) {
    return false;
  }
  if (_ul != rhs._ul) {
    return false;
  }
  if (_num_rows != rhs._num_rows) {
    return false;
  }
  if (_num_columns != rhs._num_columns) {
    return false;
  }
  if (_structure != rhs._structure) {
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
  _num_rows = 1;
  _num_columns = 1;
}

dbIStream& operator>>(dbIStream& stream, _dbGDSARef& obj)
{
  stream >> obj._origin;
  stream >> obj._lr;
  stream >> obj._ul;
  stream >> obj._propattr;
  stream >> obj._transform;
  stream >> obj._num_rows;
  stream >> obj._num_columns;
  stream >> obj._structure;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbGDSARef& obj)
{
  stream << obj._origin;
  stream << obj._lr;
  stream << obj._ul;
  stream << obj._propattr;
  stream << obj._transform;
  stream << obj._num_rows;
  stream << obj._num_columns;
  stream << obj._structure;
  return stream;
}

void _dbGDSARef::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  // User Code Begin collectMemInfo
  info.children_["propattr"].add(_propattr);
  for (auto& [i, s] : _propattr) {
    info.children_["propattr"].add(s);
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

  obj->_origin = origin;
}

Point dbGDSARef::getOrigin() const
{
  _dbGDSARef* obj = (_dbGDSARef*) this;
  return obj->_origin;
}

void dbGDSARef::setLr(Point lr)
{
  _dbGDSARef* obj = (_dbGDSARef*) this;

  obj->_lr = lr;
}

Point dbGDSARef::getLr() const
{
  _dbGDSARef* obj = (_dbGDSARef*) this;
  return obj->_lr;
}

void dbGDSARef::setUl(Point ul)
{
  _dbGDSARef* obj = (_dbGDSARef*) this;

  obj->_ul = ul;
}

Point dbGDSARef::getUl() const
{
  _dbGDSARef* obj = (_dbGDSARef*) this;
  return obj->_ul;
}

void dbGDSARef::setTransform(dbGDSSTrans transform)
{
  _dbGDSARef* obj = (_dbGDSARef*) this;

  obj->_transform = transform;
}

dbGDSSTrans dbGDSARef::getTransform() const
{
  _dbGDSARef* obj = (_dbGDSARef*) this;
  return obj->_transform;
}

void dbGDSARef::setNumRows(int16_t num_rows)
{
  _dbGDSARef* obj = (_dbGDSARef*) this;

  obj->_num_rows = num_rows;
}

int16_t dbGDSARef::getNumRows() const
{
  _dbGDSARef* obj = (_dbGDSARef*) this;
  return obj->_num_rows;
}

void dbGDSARef::setNumColumns(int16_t num_columns)
{
  _dbGDSARef* obj = (_dbGDSARef*) this;

  obj->_num_columns = num_columns;
}

int16_t dbGDSARef::getNumColumns() const
{
  _dbGDSARef* obj = (_dbGDSARef*) this;
  return obj->_num_columns;
}

// User Code Begin dbGDSARefPublicMethods

dbGDSStructure* dbGDSARef::getStructure() const
{
  _dbGDSARef* obj = (_dbGDSARef*) this;
  if (obj->_structure == 0) {
    return nullptr;
  }
  _dbGDSStructure* parent = (_dbGDSStructure*) obj->getOwner();
  _dbGDSLib* lib = (_dbGDSLib*) parent->getOwner();
  return (dbGDSStructure*) lib->_gdsstructure_tbl->getPtr(obj->_structure);
}

std::vector<std::pair<std::int16_t, std::string>>& dbGDSARef::getPropattr()
{
  auto* obj = (_dbGDSARef*) this;
  return obj->_propattr;
}

dbGDSARef* dbGDSARef::create(dbGDSStructure* parent, dbGDSStructure* child)
{
  auto* obj = (_dbGDSStructure*) parent;
  _dbGDSARef* aref = obj->arefs_->create();
  aref->_structure = child->getImpl()->getOID();
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
