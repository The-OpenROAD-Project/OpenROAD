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
#include "dbGDSBox.h"

#include "dbDatabase.h"
#include "dbDiff.hpp"
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

  return true;
}

bool _dbGDSBox::operator<(const _dbGDSBox& rhs) const
{
  return true;
}

void _dbGDSBox::differences(dbDiff& diff,
                            const char* field,
                            const _dbGDSBox& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(_layer);
  DIFF_FIELD(_datatype);
  DIFF_END
}

void _dbGDSBox::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_layer);
  DIFF_OUT_FIELD(_datatype);

  DIFF_END
}

_dbGDSBox::_dbGDSBox(_dbDatabase* db)
{
  _layer = 0;
  _datatype = 0;
}

_dbGDSBox::_dbGDSBox(_dbDatabase* db, const _dbGDSBox& r)
{
  _layer = r._layer;
  _datatype = r._datatype;
}

dbIStream& operator>>(dbIStream& stream, _dbGDSBox& obj)
{
  stream >> obj._layer;
  stream >> obj._datatype;
  stream >> obj._xy;
  stream >> obj._propattr;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbGDSBox& obj)
{
  stream << obj._layer;
  stream << obj._datatype;
  stream << obj._xy;
  stream << obj._propattr;
  return stream;
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

void dbGDSBox::setXy(const std::vector<Point>& xy)
{
  _dbGDSBox* obj = (_dbGDSBox*) this;

  obj->_xy = xy;
}

void dbGDSBox::getXy(std::vector<Point>& tbl) const
{
  _dbGDSBox* obj = (_dbGDSBox*) this;
  tbl = obj->_xy;
}

// User Code Begin dbGDSBoxPublicMethods
std::vector<std::pair<std::int16_t, std::string>>& dbGDSBox::getPropattr()
{
  auto* obj = (_dbGDSBox*) this;
  return obj->_propattr;
}

const std::vector<Point>& dbGDSBox::getXY()
{
  auto obj = (_dbGDSBox*) this;
  return obj->_xy;
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
