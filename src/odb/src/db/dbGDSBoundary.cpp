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
#include "dbGDSBoundary.h"

#include "dbDatabase.h"
#include "dbDiff.hpp"
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

void _dbGDSBoundary::differences(dbDiff& diff,
                                 const char* field,
                                 const _dbGDSBoundary& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(_layer);
  DIFF_FIELD(_datatype);
  DIFF_END
}

void _dbGDSBoundary::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_layer);
  DIFF_OUT_FIELD(_datatype);

  DIFF_END
}

_dbGDSBoundary::_dbGDSBoundary(_dbDatabase* db)
{
  _layer = 0;
  _datatype = 0;
}

_dbGDSBoundary::_dbGDSBoundary(_dbDatabase* db, const _dbGDSBoundary& r)
{
  _layer = r._layer;
  _datatype = r._datatype;
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
