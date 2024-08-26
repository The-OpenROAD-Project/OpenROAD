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
#include "dbGDSElement.h"

#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/db.h"
#include "odb/dbTypes.h"
namespace odb {
template class dbTable<_dbGDSElement>;

bool _dbGDSElement::operator==(const _dbGDSElement& rhs) const
{
  if (_layer != rhs._layer) {
    return false;
  }
  if (_datatype != rhs._datatype) {
    return false;
  }

  return true;
}

bool _dbGDSElement::operator<(const _dbGDSElement& rhs) const
{
  return true;
}

void _dbGDSElement::differences(dbDiff& diff,
                                const char* field,
                                const _dbGDSElement& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(_layer);
  DIFF_FIELD(_datatype);
  DIFF_END
}

void _dbGDSElement::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_layer);
  DIFF_OUT_FIELD(_datatype);

  DIFF_END
}

_dbGDSElement::_dbGDSElement(_dbDatabase* db)
{
}

_dbGDSElement::_dbGDSElement(_dbDatabase* db, const _dbGDSElement& r)
{
  _layer = r._layer;
  _datatype = r._datatype;
}

dbIStream& operator>>(dbIStream& stream, _dbGDSElement& obj)
{
  stream >> obj._layer;
  stream >> obj._datatype;
  stream >> obj._xy;
  stream >> obj._propattr;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbGDSElement& obj)
{
  stream << obj._layer;
  stream << obj._datatype;
  stream << obj._xy;
  stream << obj._propattr;
  return stream;
}

////////////////////////////////////////////////////////////////////
//
// dbGDSElement - Methods
//
////////////////////////////////////////////////////////////////////

void dbGDSElement::setLayer(int16_t layer)
{
  _dbGDSElement* obj = (_dbGDSElement*) this;

  obj->_layer = layer;
}

int16_t dbGDSElement::getLayer() const
{
  _dbGDSElement* obj = (_dbGDSElement*) this;
  return obj->_layer;
}

void dbGDSElement::setDatatype(int16_t datatype)
{
  _dbGDSElement* obj = (_dbGDSElement*) this;

  obj->_datatype = datatype;
}

int16_t dbGDSElement::getDatatype() const
{
  _dbGDSElement* obj = (_dbGDSElement*) this;
  return obj->_datatype;
}

// User Code Begin dbGDSElementPublicMethods

std::vector<Point>& dbGDSElement::getXY()
{
  _dbGDSElement* obj = (_dbGDSElement*) this;
  return obj->_xy;
}

std::vector<std::pair<std::int16_t, std::string>>& dbGDSElement::getPropattr()
{
  _dbGDSElement* obj = (_dbGDSElement*) this;
  return obj->_propattr;
}

// User Code End dbGDSElementPublicMethods
}  // namespace odb
   // Generator Code End Cpp