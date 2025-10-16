// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbGDSText.h"

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
template class dbTable<_dbGDSText>;

bool _dbGDSText::operator==(const _dbGDSText& rhs) const
{
  if (_layer != rhs._layer) {
    return false;
  }
  if (_datatype != rhs._datatype) {
    return false;
  }
  if (_origin != rhs._origin) {
    return false;
  }
  if (_text != rhs._text) {
    return false;
  }

  return true;
}

bool _dbGDSText::operator<(const _dbGDSText& rhs) const
{
  return true;
}

_dbGDSText::_dbGDSText(_dbDatabase* db)
{
  _layer = 0;
  _datatype = 0;
}

dbIStream& operator>>(dbIStream& stream, _dbGDSText& obj)
{
  stream >> obj._layer;
  stream >> obj._datatype;
  stream >> obj._origin;
  stream >> obj._propattr;
  stream >> obj._presentation;
  stream >> obj._transform;
  stream >> obj._text;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbGDSText& obj)
{
  stream << obj._layer;
  stream << obj._datatype;
  stream << obj._origin;
  stream << obj._propattr;
  stream << obj._presentation;
  stream << obj._transform;
  stream << obj._text;
  return stream;
}

void _dbGDSText::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  // User Code Begin collectMemInfo
  info.children_["propattr"].add(_propattr);
  for (auto& [i, s] : _propattr) {
    info.children_["propattr"].add(s);
  }
  info.children_["text"].add(_text);
  // User Code End collectMemInfo
}

////////////////////////////////////////////////////////////////////
//
// dbGDSText - Methods
//
////////////////////////////////////////////////////////////////////

void dbGDSText::setLayer(int16_t layer)
{
  _dbGDSText* obj = (_dbGDSText*) this;

  obj->_layer = layer;
}

int16_t dbGDSText::getLayer() const
{
  _dbGDSText* obj = (_dbGDSText*) this;
  return obj->_layer;
}

void dbGDSText::setDatatype(int16_t datatype)
{
  _dbGDSText* obj = (_dbGDSText*) this;

  obj->_datatype = datatype;
}

int16_t dbGDSText::getDatatype() const
{
  _dbGDSText* obj = (_dbGDSText*) this;
  return obj->_datatype;
}

void dbGDSText::setOrigin(Point origin)
{
  _dbGDSText* obj = (_dbGDSText*) this;

  obj->_origin = origin;
}

Point dbGDSText::getOrigin() const
{
  _dbGDSText* obj = (_dbGDSText*) this;
  return obj->_origin;
}

void dbGDSText::setPresentation(dbGDSTextPres presentation)
{
  _dbGDSText* obj = (_dbGDSText*) this;

  obj->_presentation = presentation;
}

dbGDSTextPres dbGDSText::getPresentation() const
{
  _dbGDSText* obj = (_dbGDSText*) this;
  return obj->_presentation;
}

void dbGDSText::setTransform(dbGDSSTrans transform)
{
  _dbGDSText* obj = (_dbGDSText*) this;

  obj->_transform = transform;
}

dbGDSSTrans dbGDSText::getTransform() const
{
  _dbGDSText* obj = (_dbGDSText*) this;
  return obj->_transform;
}

void dbGDSText::setText(const std::string& text)
{
  _dbGDSText* obj = (_dbGDSText*) this;

  obj->_text = text;
}

std::string dbGDSText::getText() const
{
  _dbGDSText* obj = (_dbGDSText*) this;
  return obj->_text;
}

// User Code Begin dbGDSTextPublicMethods
std::vector<std::pair<std::int16_t, std::string>>& dbGDSText::getPropattr()
{
  auto* obj = (_dbGDSText*) this;
  return obj->_propattr;
}

dbGDSText* dbGDSText::create(dbGDSStructure* structure)
{
  auto* obj = (_dbGDSStructure*) structure;
  return (dbGDSText*) obj->texts_->create();
}

void dbGDSText::destroy(dbGDSText* text)
{
  auto* obj = (_dbGDSText*) text;
  auto* structure = (_dbGDSStructure*) obj->getOwner();
  structure->texts_->destroy(obj);
}
// User Code End dbGDSTextPublicMethods
}  // namespace odb
   // Generator Code End Cpp
