///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2022, The Regents of the University of California
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

// Generator Code Begin Cpp
#include "dbGDSText.h"

#include "dbDatabase.h"
#include "dbDiff.hpp"
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
  if (_width != rhs._width) {
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

void _dbGDSText::differences(dbDiff& diff,
                             const char* field,
                             const _dbGDSText& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(_layer);
  DIFF_FIELD(_datatype);
  DIFF_FIELD(_width);
  DIFF_FIELD(_text);
  DIFF_END
}

void _dbGDSText::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_layer);
  DIFF_OUT_FIELD(_datatype);
  DIFF_OUT_FIELD(_width);
  DIFF_OUT_FIELD(_text);

  DIFF_END
}

_dbGDSText::_dbGDSText(_dbDatabase* db)
{
  _layer = 0;
  _datatype = 0;
  _width = 0;
}

_dbGDSText::_dbGDSText(_dbDatabase* db, const _dbGDSText& r)
{
  _layer = r._layer;
  _datatype = r._datatype;
  _width = r._width;
  _text = r._text;
}

dbIStream& operator>>(dbIStream& stream, _dbGDSText& obj)
{
  stream >> obj._layer;
  stream >> obj._datatype;
  stream >> obj._xy;
  stream >> obj._propattr;
  stream >> obj._presentation;
  stream >> obj._width;
  stream >> obj._transform;
  stream >> obj._text;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbGDSText& obj)
{
  stream << obj._layer;
  stream << obj._datatype;
  stream << obj._xy;
  stream << obj._propattr;
  stream << obj._presentation;
  stream << obj._width;
  stream << obj._transform;
  stream << obj._text;
  return stream;
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

void dbGDSText::setXy(const std::vector<Point>& xy)
{
  _dbGDSText* obj = (_dbGDSText*) this;

  obj->_xy = xy;
}

void dbGDSText::getXy(std::vector<Point>& tbl) const
{
  _dbGDSText* obj = (_dbGDSText*) this;
  tbl = obj->_xy;
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

void dbGDSText::setWidth(int width)
{
  _dbGDSText* obj = (_dbGDSText*) this;

  obj->_width = width;
}

int dbGDSText::getWidth() const
{
  _dbGDSText* obj = (_dbGDSText*) this;
  return obj->_width;
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

const std::vector<Point>& dbGDSText::getXY()
{
  auto obj = (_dbGDSText*) this;
  return obj->_xy;
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
