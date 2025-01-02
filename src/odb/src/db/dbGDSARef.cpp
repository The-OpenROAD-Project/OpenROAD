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
#include "dbGDSARef.h"

#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/db.h"
#include "odb/dbTypes.h"
namespace odb {
template class dbTable<_dbGDSARef>;

bool _dbGDSARef::operator==(const _dbGDSARef& rhs) const
{
  if (_sName != rhs._sName) {
    return false;
  }

  return true;
}

bool _dbGDSARef::operator<(const _dbGDSARef& rhs) const
{
  return true;
}

void _dbGDSARef::differences(dbDiff& diff,
                             const char* field,
                             const _dbGDSARef& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(_sName);
  DIFF_END
}

void _dbGDSARef::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_sName);

  DIFF_END
}

_dbGDSARef::_dbGDSARef(_dbDatabase* db)
{
}

_dbGDSARef::_dbGDSARef(_dbDatabase* db, const _dbGDSARef& r)
{
  _sName = r._sName;
}

dbIStream& operator>>(dbIStream& stream, _dbGDSARef& obj)
{
  stream >> obj._xy;
  stream >> obj._propattr;
  stream >> obj._sName;
  stream >> obj._transform;
  stream >> obj._colRow;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbGDSARef& obj)
{
  stream << obj._xy;
  stream << obj._propattr;
  stream << obj._sName;
  stream << obj._transform;
  stream << obj._colRow;
  return stream;
}

////////////////////////////////////////////////////////////////////
//
// dbGDSARef - Methods
//
////////////////////////////////////////////////////////////////////

void dbGDSARef::setXy(const std::vector<Point>& xy)
{
  _dbGDSARef* obj = (_dbGDSARef*) this;

  obj->_xy = xy;
}

void dbGDSARef::getXy(std::vector<Point>& tbl) const
{
  _dbGDSARef* obj = (_dbGDSARef*) this;
  tbl = obj->_xy;
}

void dbGDSARef::set_sName(const std::string& sName)
{
  _dbGDSARef* obj = (_dbGDSARef*) this;

  obj->_sName = sName;
}

std::string dbGDSARef::get_sName() const
{
  _dbGDSARef* obj = (_dbGDSARef*) this;
  return obj->_sName;
}

void dbGDSARef::setTransform(dbGDSSTrans transform)
{
  _dbGDSARef* obj = (_dbGDSARef*) this;

  obj->_transform = transform;
}

dbGDSSTrans dbGDSARef::getTransform() const
{
  _dbGDSARef* obj = (_dbGDSARef*) this;
  return obj->_transform;
}

void dbGDSARef::set_colRow(const std::pair<int16_t, int16_t>& colRow)
{
  _dbGDSARef* obj = (_dbGDSARef*) this;

  obj->_colRow = colRow;
}

std::pair<int16_t, int16_t> dbGDSARef::get_colRow() const
{
  _dbGDSARef* obj = (_dbGDSARef*) this;
  return obj->_colRow;
}

// User Code Begin dbGDSARefPublicMethods

std::vector<std::pair<std::int16_t, std::string>>& dbGDSARef::getPropattr()
{
  auto* obj = (_dbGDSARef*) this;
  return obj->_propattr;
}

const std::vector<Point>& dbGDSARef::getXY()
{
  auto obj = (_dbGDSARef*) this;
  return obj->_xy;
}

dbGDSARef* dbGDSARef::create(dbGDSStructure* structure)
{
  auto* obj = (_dbGDSStructure*) structure;
  return (dbGDSARef*) obj->arefs_->create();
}

void dbGDSARef::destroy(dbGDSARef* aref)
{
  auto* obj = (_dbGDSARef*) aref;
  auto* structure = (_dbGDSStructure*) obj->getOwner();
  structure->arefs_->destroy(obj);
}

dbGDSStructure* dbGDSARef::getStructure() const
{
  _dbGDSARef* obj = (_dbGDSARef*) this;
  return obj->_stucture;
}

void dbGDSARef::setStructure(dbGDSStructure* structure) const
{
  _dbGDSARef* obj = (_dbGDSARef*) this;
  obj->_stucture = structure;
}
// User Code End dbGDSARefPublicMethods
}  // namespace odb
   // Generator Code End Cpp
