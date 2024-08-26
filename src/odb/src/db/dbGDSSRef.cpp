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
#include "dbGDSSRef.h"

#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/db.h"
#include "odb/dbTypes.h"
namespace odb {
template class dbTable<_dbGDSSRef>;

bool _dbGDSSRef::operator==(const _dbGDSSRef& rhs) const
{
  if (_sName != rhs._sName) {
    return false;
  }

  return true;
}

bool _dbGDSSRef::operator<(const _dbGDSSRef& rhs) const
{
  return true;
}

void _dbGDSSRef::differences(dbDiff& diff,
                             const char* field,
                             const _dbGDSSRef& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(_sName);
  DIFF_END
}

void _dbGDSSRef::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_sName);

  DIFF_END
}

_dbGDSSRef::_dbGDSSRef(_dbDatabase* db)
{
}

_dbGDSSRef::_dbGDSSRef(_dbDatabase* db, const _dbGDSSRef& r)
{
  _sName = r._sName;
}

dbIStream& operator>>(dbIStream& stream, _dbGDSSRef& obj)
{
  stream >> obj._sName;
  stream >> obj._sTrans;
  stream >> obj._colRow;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbGDSSRef& obj)
{
  stream << obj._sName;
  stream << obj._sTrans;
  stream << obj._colRow;
  return stream;
}

////////////////////////////////////////////////////////////////////
//
// dbGDSSRef - Methods
//
////////////////////////////////////////////////////////////////////

void dbGDSSRef::set_sName(const std::string& sName)
{
  _dbGDSSRef* obj = (_dbGDSSRef*) this;

  obj->_sName = sName;
}

std::string dbGDSSRef::get_sName() const
{
  _dbGDSSRef* obj = (_dbGDSSRef*) this;
  return obj->_sName;
}

void dbGDSSRef::set_sTrans(dbGDSSTrans sTrans)
{
  _dbGDSSRef* obj = (_dbGDSSRef*) this;

  obj->_sTrans = sTrans;
}

dbGDSSTrans dbGDSSRef::get_sTrans() const
{
  _dbGDSSRef* obj = (_dbGDSSRef*) this;
  return obj->_sTrans;
}

void dbGDSSRef::set_colRow(std::pair<int16_t, int16_t> colRow)
{
  _dbGDSSRef* obj = (_dbGDSSRef*) this;

  obj->_colRow = colRow;
}

std::pair<int16_t, int16_t> dbGDSSRef::get_colRow() const
{
  _dbGDSSRef* obj = (_dbGDSSRef*) this;
  return obj->_colRow;
}

}  // namespace odb
   // Generator Code End Cpp