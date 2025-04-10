// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbGDSBoundary.h"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "dbDatabase.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/db.h"
#include "odb/dbTypes.h"
namespace odb {
template class dbTable<_dbGDSBoundary>;

bool _dbGDSBoundary::operator==(const _dbGDSBoundary& rhs) const
{
  if (_layer != rhs._layer) {
    return false;
  }
  if (_datatype != rhs._datatype) {
    return false;
  }

  return true;
}

bool _dbGDSBoundary::operator<(const _dbGDSBoundary& rhs) const
{
  return true;
}

_dbGDSBoundary::_dbGDSBoundary(_dbDatabase* db)
{
  _layer = 0;
  _datatype = 0;
}

dbIStream& operator>>(dbIStream& stream, _dbGDSBoundary& obj)
{
  stream >> obj._layer;
  stream >> obj._datatype;
  stream >> obj._xy;
  stream >> obj._propattr;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbGDSBoundary& obj)
{
  stream << obj._layer;
  stream << obj._datatype;
  stream << obj._xy;
  stream << obj._propattr;
  return stream;
}

void _dbGDSBoundary::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  // User Code Begin collectMemInfo
  info.children_["xy"].add(_xy);
  info.children_["propattr"].add(_propattr);
  for (auto& [i, s] : _propattr) {
    info.children_["propattr"].add(s);
  }
  // User Code End collectMemInfo
}

////////////////////////////////////////////////////////////////////
//
// dbGDSBoundary - Methods
//
////////////////////////////////////////////////////////////////////

void dbGDSBoundary::setLayer(int16_t layer)
{
  _dbGDSBoundary* obj = (_dbGDSBoundary*) this;

  obj->_layer = layer;
}

int16_t dbGDSBoundary::getLayer() const
{
  _dbGDSBoundary* obj = (_dbGDSBoundary*) this;
  return obj->_layer;
}

void dbGDSBoundary::setDatatype(int16_t datatype)
{
  _dbGDSBoundary* obj = (_dbGDSBoundary*) this;

  obj->_datatype = datatype;
}

int16_t dbGDSBoundary::getDatatype() const
{
  _dbGDSBoundary* obj = (_dbGDSBoundary*) this;
  return obj->_datatype;
}

void dbGDSBoundary::setXy(const std::vector<Point>& xy)
{
  _dbGDSBoundary* obj = (_dbGDSBoundary*) this;

  obj->_xy = xy;
}

void dbGDSBoundary::getXy(std::vector<Point>& tbl) const
{
  _dbGDSBoundary* obj = (_dbGDSBoundary*) this;
  tbl = obj->_xy;
}

// User Code Begin dbGDSBoundaryPublicMethods
const std::vector<Point>& dbGDSBoundary::getXY()
{
  auto obj = (_dbGDSBoundary*) this;
  return obj->_xy;
}

std::vector<std::pair<std::int16_t, std::string>>& dbGDSBoundary::getPropattr()
{
  auto* obj = (_dbGDSBoundary*) this;
  return obj->_propattr;
}

dbGDSBoundary* dbGDSBoundary::create(dbGDSStructure* structure)
{
  auto* obj = (_dbGDSStructure*) structure;
  return (dbGDSBoundary*) obj->boundaries_->create();
}

void dbGDSBoundary::destroy(dbGDSBoundary* boundary)
{
  auto* obj = (_dbGDSBoundary*) boundary;
  auto* structure = (_dbGDSStructure*) obj->getOwner();
  structure->boundaries_->destroy(obj);
}

// User Code End dbGDSBoundaryPublicMethods
}  // namespace odb
   // Generator Code End Cpp
