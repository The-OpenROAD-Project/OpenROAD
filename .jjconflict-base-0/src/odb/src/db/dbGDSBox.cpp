// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbGDSBox.h"

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
template class dbTable<_dbGDSBox>;

bool _dbGDSBox::operator==(const _dbGDSBox& rhs) const
{
  if (_layer != rhs._layer) {
    return false;
  }
  if (_datatype != rhs._datatype) {
    return false;
  }
  if (_bounds != rhs._bounds) {
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
  _layer = 0;
  _datatype = 0;
}

dbIStream& operator>>(dbIStream& stream, _dbGDSBox& obj)
{
  stream >> obj._layer;
  stream >> obj._datatype;
  stream >> obj._bounds;
  stream >> obj._propattr;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbGDSBox& obj)
{
  stream << obj._layer;
  stream << obj._datatype;
  stream << obj._bounds;
  stream << obj._propattr;
  return stream;
}

void _dbGDSBox::collectMemInfo(MemInfo& info)
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
// dbGDSBox - Methods
//
////////////////////////////////////////////////////////////////////

void dbGDSBox::setLayer(int16_t layer)
{
  _dbGDSBox* obj = (_dbGDSBox*) this;

  obj->_layer = layer;
}

int16_t dbGDSBox::getLayer() const
{
  _dbGDSBox* obj = (_dbGDSBox*) this;
  return obj->_layer;
}

void dbGDSBox::setDatatype(int16_t datatype)
{
  _dbGDSBox* obj = (_dbGDSBox*) this;

  obj->_datatype = datatype;
}

int16_t dbGDSBox::getDatatype() const
{
  _dbGDSBox* obj = (_dbGDSBox*) this;
  return obj->_datatype;
}

void dbGDSBox::setBounds(Rect bounds)
{
  _dbGDSBox* obj = (_dbGDSBox*) this;

  obj->_bounds = bounds;
}

Rect dbGDSBox::getBounds() const
{
  _dbGDSBox* obj = (_dbGDSBox*) this;
  return obj->_bounds;
}

// User Code Begin dbGDSBoxPublicMethods
std::vector<std::pair<std::int16_t, std::string>>& dbGDSBox::getPropattr()
{
  auto* obj = (_dbGDSBox*) this;
  return obj->_propattr;
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
