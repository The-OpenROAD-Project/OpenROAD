// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbGDSBox.h"

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
template class dbTable<_dbGDSBox>;

bool _dbGDSBox::operator==(const _dbGDSBox& rhs) const
{
  if (layer_ != rhs.layer_) {
    return false;
  }
  if (datatype_ != rhs.datatype_) {
    return false;
  }
  if (bounds_ != rhs.bounds_) {
    return false;
  }

  return true;
}

bool _dbGDSBox::operator<(const _dbGDSBox& rhs) const
{
  return true;
}

_dbGDSBox::_dbGDSBox(_dbDatabase* db)
{
  layer_ = 0;
  datatype_ = 0;
}

dbIStream& operator>>(dbIStream& stream, _dbGDSBox& obj)
{
  stream >> obj.layer_;
  stream >> obj.datatype_;
  stream >> obj.bounds_;
  stream >> obj.propattr_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbGDSBox& obj)
{
  stream << obj.layer_;
  stream << obj.datatype_;
  stream << obj.bounds_;
  stream << obj.propattr_;
  return stream;
}

void _dbGDSBox::collectMemInfo(MemInfo& info)
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
// dbGDSBox - Methods
//
////////////////////////////////////////////////////////////////////

void dbGDSBox::setLayer(int16_t layer)
{
  _dbGDSBox* obj = (_dbGDSBox*) this;

  obj->layer_ = layer;
}

int16_t dbGDSBox::getLayer() const
{
  _dbGDSBox* obj = (_dbGDSBox*) this;
  return obj->layer_;
}

void dbGDSBox::setDatatype(int16_t datatype)
{
  _dbGDSBox* obj = (_dbGDSBox*) this;

  obj->datatype_ = datatype;
}

int16_t dbGDSBox::getDatatype() const
{
  _dbGDSBox* obj = (_dbGDSBox*) this;
  return obj->datatype_;
}

void dbGDSBox::setBounds(Rect bounds)
{
  _dbGDSBox* obj = (_dbGDSBox*) this;

  obj->bounds_ = bounds;
}

Rect dbGDSBox::getBounds() const
{
  _dbGDSBox* obj = (_dbGDSBox*) this;
  return obj->bounds_;
}

// User Code Begin dbGDSBoxPublicMethods
std::vector<std::pair<std::int16_t, std::string>>& dbGDSBox::getPropattr()
{
  auto* obj = (_dbGDSBox*) this;
  return obj->propattr_;
}

dbGDSBox* dbGDSBox::create(dbGDSStructure* structure)
{
  auto* obj = (_dbGDSStructure*) structure;
  return (dbGDSBox*) obj->boxes_->create();
}

void dbGDSBox::destroy(dbGDSBox* box)
{
  auto* obj = (_dbGDSBox*) box;
  auto* structure = (_dbGDSStructure*) obj->getOwner();
  structure->boxes_->destroy(obj);
}
// User Code End dbGDSBoxPublicMethods
}  // namespace odb
   // Generator Code End Cpp
