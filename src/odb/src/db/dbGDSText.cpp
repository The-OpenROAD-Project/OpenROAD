// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbGDSText.h"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "dbCore.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
namespace odb {
template class dbTable<_dbGDSText>;

bool _dbGDSText::operator==(const _dbGDSText& rhs) const
{
  if (layer_ != rhs.layer_) {
    return false;
  }
  if (datatype_ != rhs.datatype_) {
    return false;
  }
  if (origin_ != rhs.origin_) {
    return false;
  }
  if (text_ != rhs.text_) {
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
  layer_ = 0;
  datatype_ = 0;
}

dbIStream& operator>>(dbIStream& stream, _dbGDSText& obj)
{
  stream >> obj.layer_;
  stream >> obj.datatype_;
  stream >> obj.origin_;
  stream >> obj.propattr_;
  stream >> obj.presentation_;
  stream >> obj.transform_;
  stream >> obj.text_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbGDSText& obj)
{
  stream << obj.layer_;
  stream << obj.datatype_;
  stream << obj.origin_;
  stream << obj.propattr_;
  stream << obj.presentation_;
  stream << obj.transform_;
  stream << obj.text_;
  return stream;
}

void _dbGDSText::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  // User Code Begin collectMemInfo
  info.children["propattr"].add(propattr_);
  for (auto& [i, s] : propattr_) {
    info.children["propattr"].add(s);
  }
  info.children["text"].add(text_);
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

  obj->layer_ = layer;
}

int16_t dbGDSText::getLayer() const
{
  _dbGDSText* obj = (_dbGDSText*) this;
  return obj->layer_;
}

void dbGDSText::setDatatype(int16_t datatype)
{
  _dbGDSText* obj = (_dbGDSText*) this;

  obj->datatype_ = datatype;
}

int16_t dbGDSText::getDatatype() const
{
  _dbGDSText* obj = (_dbGDSText*) this;
  return obj->datatype_;
}

void dbGDSText::setOrigin(Point origin)
{
  _dbGDSText* obj = (_dbGDSText*) this;

  obj->origin_ = origin;
}

Point dbGDSText::getOrigin() const
{
  _dbGDSText* obj = (_dbGDSText*) this;
  return obj->origin_;
}

void dbGDSText::setPresentation(dbGDSTextPres presentation)
{
  _dbGDSText* obj = (_dbGDSText*) this;

  obj->presentation_ = presentation;
}

dbGDSTextPres dbGDSText::getPresentation() const
{
  _dbGDSText* obj = (_dbGDSText*) this;
  return obj->presentation_;
}

void dbGDSText::setTransform(dbGDSSTrans transform)
{
  _dbGDSText* obj = (_dbGDSText*) this;

  obj->transform_ = transform;
}

dbGDSSTrans dbGDSText::getTransform() const
{
  _dbGDSText* obj = (_dbGDSText*) this;
  return obj->transform_;
}

void dbGDSText::setText(const std::string& text)
{
  _dbGDSText* obj = (_dbGDSText*) this;

  obj->text_ = text;
}

std::string dbGDSText::getText() const
{
  _dbGDSText* obj = (_dbGDSText*) this;
  return obj->text_;
}

// User Code Begin dbGDSTextPublicMethods
std::vector<std::pair<std::int16_t, std::string>>& dbGDSText::getPropattr()
{
  auto* obj = (_dbGDSText*) this;
  return obj->propattr_;
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
