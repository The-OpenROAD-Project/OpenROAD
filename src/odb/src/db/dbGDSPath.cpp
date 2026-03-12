// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbGDSPath.h"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "dbCore.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
// User Code Begin Includes
#include "dbGDSStructure.h"
// User Code End Includes
namespace odb {
template class dbTable<_dbGDSPath>;

bool _dbGDSPath::operator==(const _dbGDSPath& rhs) const
{
  if (layer_ != rhs.layer_) {
    return false;
  }
  if (datatype_ != rhs.datatype_) {
    return false;
  }
  if (width_ != rhs.width_) {
    return false;
  }
  if (path_type_ != rhs.path_type_) {
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
  layer_ = 0;
  datatype_ = 0;
  width_ = 0;
  path_type_ = 0;
}

dbIStream& operator>>(dbIStream& stream, _dbGDSPath& obj)
{
  stream >> obj.layer_;
  stream >> obj.datatype_;
  stream >> obj.xy_;
  stream >> obj.propattr_;
  stream >> obj.width_;
  stream >> obj.path_type_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbGDSPath& obj)
{
  stream << obj.layer_;
  stream << obj.datatype_;
  stream << obj.xy_;
  stream << obj.propattr_;
  stream << obj.width_;
  stream << obj.path_type_;
  return stream;
}

void _dbGDSPath::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  // User Code Begin collectMemInfo
  info.children["xy"].add(xy_);
  info.children["propattr"].add(propattr_);
  for (auto& [i, s] : propattr_) {
    info.children["propattr"].add(s);
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

  obj->layer_ = layer;
}

int16_t dbGDSPath::getLayer() const
{
  _dbGDSPath* obj = (_dbGDSPath*) this;
  return obj->layer_;
}

void dbGDSPath::setDatatype(int16_t datatype)
{
  _dbGDSPath* obj = (_dbGDSPath*) this;

  obj->datatype_ = datatype;
}

int16_t dbGDSPath::getDatatype() const
{
  _dbGDSPath* obj = (_dbGDSPath*) this;
  return obj->datatype_;
}

void dbGDSPath::setXy(const std::vector<Point>& xy)
{
  _dbGDSPath* obj = (_dbGDSPath*) this;

  obj->xy_ = xy;
}

void dbGDSPath::getXy(std::vector<Point>& tbl) const
{
  _dbGDSPath* obj = (_dbGDSPath*) this;
  tbl = obj->xy_;
}

void dbGDSPath::setWidth(int width)
{
  _dbGDSPath* obj = (_dbGDSPath*) this;

  obj->width_ = width;
}

int dbGDSPath::getWidth() const
{
  _dbGDSPath* obj = (_dbGDSPath*) this;
  return obj->width_;
}

void dbGDSPath::setPathType(int16_t path_type)
{
  _dbGDSPath* obj = (_dbGDSPath*) this;

  obj->path_type_ = path_type;
}

int16_t dbGDSPath::getPathType() const
{
  _dbGDSPath* obj = (_dbGDSPath*) this;
  return obj->path_type_;
}

// User Code Begin dbGDSPathPublicMethods
std::vector<std::pair<std::int16_t, std::string>>& dbGDSPath::getPropattr()
{
  auto* obj = (_dbGDSPath*) this;
  return obj->propattr_;
}

const std::vector<Point>& dbGDSPath::getXY()
{
  auto obj = (_dbGDSPath*) this;
  return obj->xy_;
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
