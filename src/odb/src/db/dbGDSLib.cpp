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
#include "dbGDSLib.h"

#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/db.h"
#include "odb/dbTypes.h"
namespace odb {
template class dbTable<_dbGDSLib>;

bool _dbGDSLib::operator==(const _dbGDSLib& rhs) const
{
  if (_name != rhs._name) {
    return false;
  }
  if (_libDirSize != rhs._libDirSize) {
    return false;
  }
  if (_srfName != rhs._srfName) {
    return false;
  }

  return true;
}

bool _dbGDSLib::operator<(const _dbGDSLib& rhs) const
{
  return true;
}

void _dbGDSLib::differences(dbDiff& diff,
                            const char* field,
                            const _dbGDSLib& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(_name);
  DIFF_FIELD(_libDirSize);
  DIFF_FIELD(_srfName);
  DIFF_END
}

void _dbGDSLib::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_name);
  DIFF_OUT_FIELD(_libDirSize);
  DIFF_OUT_FIELD(_srfName);

  DIFF_END
}

_dbGDSLib::_dbGDSLib(_dbDatabase* db)
{
}

_dbGDSLib::_dbGDSLib(_dbDatabase* db, const _dbGDSLib& r)
{
  _name = r._name;
  _libDirSize = r._libDirSize;
  _srfName = r._srfName;
}

dbIStream& operator>>(dbIStream& stream, _dbGDSLib& obj)
{
  stream >> obj._name;
  stream >> obj._lastAccessed;
  stream >> obj._lastModified;
  stream >> obj._libDirSize;
  stream >> obj._srfName;
  stream >> obj._units;
  stream >> obj._structures;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbGDSLib& obj)
{
  stream << obj._name;
  stream << obj._lastAccessed;
  stream << obj._lastModified;
  stream << obj._libDirSize;
  stream << obj._srfName;
  stream << obj._units;
  stream << obj._structures;
  return stream;
}

_dbGDSLib::~_dbGDSLib()
{
  if (_name) {
    free((void*) _name);
  }
}

////////////////////////////////////////////////////////////////////
//
// dbGDSLib - Methods
//
////////////////////////////////////////////////////////////////////

}  // namespace odb
   // Generator Code End Cpp