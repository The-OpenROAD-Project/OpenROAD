// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbGDSPath.h"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "dbDatabase.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
namespace odb {
template class dbTable<_dbGDSPath>;

bool _dbGDSPath::operator==(const _dbGDSPath& rhs) const
{
  if (_layer != rhs._layer) {
    return false;
  }
  if (_datatype != rhs._datatype) {
    return false;
  }
  if (_width != rhs._width) {
    return false;
  }
  if (_path_type != rhs._path_type) {
    return false;
  }

  return true;
}

bool _dbGDSPath::operator<(const _dbGDSPath& rhs) const
{
  return true;
}

_dbGDSPath::_dbGDSPath(_dbDatabase* db)
{
  _layer = 0;
  _datatype = 0;
  _width = 0;
  _path_type = 0;
}

dbIStream& operator>>(dbIStream& stream, _dbGDSPath& obj)
{
  stream >> obj._layer;
  stream >> obj._datatype;
  stream >> obj._xy;
  stream >> obj._propattr;
  stream >> obj._width;
  stream >> obj._path_type;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbGDSPath& obj)
{
  stream << obj._layer;
  stream << obj._datatype;
  stream << obj._xy;
  stream << obj._propattr;
  stream << obj._width;
  stream << obj._path_type;
  return stream;
}

void _dbGDSPath::collectMemInfo(MemInfo& info)
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
// dbGDSPath - Methods
//
////////////////////////////////////////////////////////////////////

void dbGDSPath::setLayer(int16_t layer)
{
  _dbGDSPath* obj = (_dbGDSPath*) this;

  obj->_layer = layer;
}

int16_t dbGDSPath::getLayer() const
{
  _dbGDSPath* obj = (_dbGDSPath*) this;
  return obj->_layer;
}

void dbGDSPath::setDatatype(int16_t datatype)
{
  _dbGDSPath* obj = (_dbGDSPath*) this;

  obj->_datatype = datatype;
}

int16_t dbGDSPath::getDatatype() const
{
  _dbGDSPath* obj = (_dbGDSPath*) this;
  return obj->_datatype;
}

void dbGDSPath::setXy(const std::vector<Point>& xy)
{
  _dbGDSPath* obj = (_dbGDSPath*) this;

  obj->_xy = xy;
}

void dbGDSPath::getXy(std::vector<Point>& tbl) const
{
  _dbGDSPath* obj = (_dbGDSPath*) this;
  tbl = obj->_xy;
}

void dbGDSPath::setWidth(int width)
{
  _dbGDSPath* obj = (_dbGDSPath*) this;

  obj->_width = width;
}

int dbGDSPath::getWidth() const
{
  _dbGDSPath* obj = (_dbGDSPath*) this;
  return obj->_width;
}

void dbGDSPath::setPathType(int16_t path_type)
{
  _dbGDSPath* obj = (_dbGDSPath*) this;

  obj->_path_type = path_type;
}

int16_t dbGDSPath::getPathType() const
{
  _dbGDSPath* obj = (_dbGDSPath*) this;
  return obj->_path_type;
}

// User Code Begin dbGDSPathPublicMethods
std::vector<std::pair<std::int16_t, std::string>>& dbGDSPath::getPropattr()
{
  auto* obj = (_dbGDSPath*) this;
  return obj->_propattr;
}

const std::vector<Point>& dbGDSPath::getXY()
{
  auto obj = (_dbGDSPath*) this;
  return obj->_xy;
}

dbGDSPath* dbGDSPath::create(dbGDSStructure* structure)
{
  auto* obj = (_dbGDSStructure*) structure;
  return (dbGDSPath*) obj->paths_->create();
}

void dbGDSPath::destroy(dbGDSPath* path)
{
  auto* obj = (_dbGDSPath*) path;
  auto* structure = (_dbGDSStructure*) obj->getOwner();
  structure->paths_->destroy(obj);
}
// User Code End dbGDSPathPublicMethods
}  // namespace odb
   // Generator Code End Cpp
