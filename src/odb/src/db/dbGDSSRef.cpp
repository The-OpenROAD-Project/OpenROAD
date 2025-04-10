// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbGDSSRef.h"

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
template class dbTable<_dbGDSSRef>;

bool _dbGDSSRef::operator==(const _dbGDSSRef& rhs) const
{
  if (_origin != rhs._origin) {
    return false;
  }
  if (_structure != rhs._structure) {
    return false;
  }

  return true;
}

bool _dbGDSSRef::operator<(const _dbGDSSRef& rhs) const
{
  return true;
}

_dbGDSSRef::_dbGDSSRef(_dbDatabase* db)
{
}

dbIStream& operator>>(dbIStream& stream, _dbGDSSRef& obj)
{
  stream >> obj._origin;
  stream >> obj._propattr;
  stream >> obj._transform;
  stream >> obj._structure;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbGDSSRef& obj)
{
  stream << obj._origin;
  stream << obj._propattr;
  stream << obj._transform;
  stream << obj._structure;
  return stream;
}

void _dbGDSSRef::collectMemInfo(MemInfo& info)
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
// dbGDSSRef - Methods
//
////////////////////////////////////////////////////////////////////

void dbGDSSRef::setOrigin(Point origin)
{
  _dbGDSSRef* obj = (_dbGDSSRef*) this;

  obj->_origin = origin;
}

Point dbGDSSRef::getOrigin() const
{
  _dbGDSSRef* obj = (_dbGDSSRef*) this;
  return obj->_origin;
}

void dbGDSSRef::setTransform(dbGDSSTrans transform)
{
  _dbGDSSRef* obj = (_dbGDSSRef*) this;

  obj->_transform = transform;
}

dbGDSSTrans dbGDSSRef::getTransform() const
{
  _dbGDSSRef* obj = (_dbGDSSRef*) this;
  return obj->_transform;
}

// User Code Begin dbGDSSRefPublicMethods

dbGDSStructure* dbGDSSRef::getStructure() const
{
  _dbGDSSRef* obj = (_dbGDSSRef*) this;
  if (obj->_structure == 0) {
    return nullptr;
  }
  _dbGDSStructure* parent = (_dbGDSStructure*) obj->getOwner();
  _dbGDSLib* lib = (_dbGDSLib*) parent->getOwner();
  return (dbGDSStructure*) lib->_gdsstructure_tbl->getPtr(obj->_structure);
}

std::vector<std::pair<std::int16_t, std::string>>& dbGDSSRef::getPropattr()
{
  auto* obj = (_dbGDSSRef*) this;
  return obj->_propattr;
}

dbGDSSRef* dbGDSSRef::create(dbGDSStructure* parent, dbGDSStructure* child)
{
  auto* obj = (_dbGDSStructure*) parent;
  _dbGDSSRef* sref = obj->srefs_->create();
  sref->_structure = child->getImpl()->getOID();
  return (dbGDSSRef*) sref;
}

void dbGDSSRef::destroy(dbGDSSRef* sRef)
{
  auto* obj = (_dbGDSSRef*) sRef;
  auto* structure = (_dbGDSStructure*) obj->getOwner();
  structure->srefs_->destroy(obj);
}

// User Code End dbGDSSRefPublicMethods
}  // namespace odb
   // Generator Code End Cpp
