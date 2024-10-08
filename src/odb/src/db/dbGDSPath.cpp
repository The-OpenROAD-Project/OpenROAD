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
#include "dbGDSPath.h"

#include "dbDatabase.h"
#include "dbDiff.hpp"
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
  if (_pathType != rhs._pathType) {
    return false;
  }

  return true;
}

bool _dbGDSPath::operator<(const _dbGDSPath& rhs) const
{
  return true;
}

void _dbGDSPath::differences(dbDiff& diff,
                             const char* field,
                             const _dbGDSPath& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(_layer);
  DIFF_FIELD(_datatype);
  DIFF_FIELD(_width);
  DIFF_FIELD(_pathType);
  DIFF_END
}

void _dbGDSPath::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_layer);
  DIFF_OUT_FIELD(_datatype);
  DIFF_OUT_FIELD(_width);
  DIFF_OUT_FIELD(_pathType);

  DIFF_END
}

_dbGDSPath::_dbGDSPath(_dbDatabase* db)
{
  _layer = 0;
  _datatype = 0;
  _width = 0;
  _pathType = 0;
}

_dbGDSPath::_dbGDSPath(_dbDatabase* db, const _dbGDSPath& r)
{
  _layer = r._layer;
  _datatype = r._datatype;
  _width = r._width;
  _pathType = r._pathType;
}

dbIStream& operator>>(dbIStream& stream, _dbGDSPath& obj)
{
  stream >> obj._layer;
  stream >> obj._datatype;
  stream >> obj._xy;
  stream >> obj._propattr;
  stream >> obj._width;
  stream >> obj._pathType;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbGDSPath& obj)
{
  stream << obj._layer;
  stream << obj._datatype;
  stream << obj._xy;
  stream << obj._propattr;
  stream << obj._width;
  stream << obj._pathType;
  return stream;
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

void dbGDSPath::set_pathType(int16_t pathType)
{
  _dbGDSPath* obj = (_dbGDSPath*) this;

  obj->_pathType = pathType;
}

int16_t dbGDSPath::get_pathType() const
{
  _dbGDSPath* obj = (_dbGDSPath*) this;
  return obj->_pathType;
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
