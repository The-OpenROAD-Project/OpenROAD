// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbGDSBoundary.h"

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
#include "dbGDSStructure.h"
#include "odb/geom.h"
// User Code End Includes
namespace odb {
template class dbTable<_dbGDSBoundary>;

bool _dbGDSBoundary::operator==(const _dbGDSBoundary& rhs) const
{
  if (layer_ != rhs.layer_) {
    return false;
  }
  if (datatype_ != rhs.datatype_) {
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
  layer_ = 0;
  datatype_ = 0;
}

dbIStream& operator>>(dbIStream& stream, _dbGDSBoundary& obj)
{
  stream >> obj.layer_;
  stream >> obj.datatype_;
  stream >> obj.xy_;
  stream >> obj.propattr_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbGDSBoundary& obj)
{
  stream << obj.layer_;
  stream << obj.datatype_;
  stream << obj.xy_;
  stream << obj.propattr_;
  return stream;
}

void _dbGDSBoundary::collectMemInfo(MemInfo& info)
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
// dbGDSBoundary - Methods
//
////////////////////////////////////////////////////////////////////

void dbGDSBoundary::setLayer(int16_t layer)
{
  _dbGDSBoundary* obj = (_dbGDSBoundary*) this;

  obj->layer_ = layer;
}

int16_t dbGDSBoundary::getLayer() const
{
  _dbGDSBoundary* obj = (_dbGDSBoundary*) this;
  return obj->layer_;
}

void dbGDSBoundary::setDatatype(int16_t datatype)
{
  _dbGDSBoundary* obj = (_dbGDSBoundary*) this;

  obj->datatype_ = datatype;
}

int16_t dbGDSBoundary::getDatatype() const
{
  _dbGDSBoundary* obj = (_dbGDSBoundary*) this;
  return obj->datatype_;
}

void dbGDSBoundary::setXy(const std::vector<Point>& xy)
{
  _dbGDSBoundary* obj = (_dbGDSBoundary*) this;

  obj->xy_ = xy;
}

void dbGDSBoundary::getXy(std::vector<Point>& tbl) const
{
  _dbGDSBoundary* obj = (_dbGDSBoundary*) this;
  tbl = obj->xy_;
}

// User Code Begin dbGDSBoundaryPublicMethods
const std::vector<Point>& dbGDSBoundary::getXY()
{
  auto obj = (_dbGDSBoundary*) this;
  return obj->xy_;
}

std::vector<std::pair<std::int16_t, std::string>>& dbGDSBoundary::getPropattr()
{
  auto* obj = (_dbGDSBoundary*) this;
  return obj->propattr_;
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
