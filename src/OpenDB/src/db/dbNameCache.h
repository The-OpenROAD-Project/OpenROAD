///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, Nefelus Inc
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

#pragma once

#include "dbHashTable.h"

namespace odb {

class dbOStream;
class dbIStream;
class dbDiff;
class _dbName;
class _dbDatabase;

class _dbNameCache
{
 public:
  dbTable<_dbName>*    _name_tbl;
  dbHashTable<_dbName> _name_hash;

  _dbNameCache(_dbDatabase* db,
               dbObject*    owner,
               dbObjectTable* (dbObject::*m)(dbObjectType));
  _dbNameCache(_dbDatabase* db, dbObject* owner, const _dbNameCache& cache);
  ~_dbNameCache();

  bool operator==(const _dbNameCache& rhs) const;
  bool operator!=(const _dbNameCache& rhs) const { return !operator==(rhs); }
  void differences(dbDiff&             diff,
                   const char*         field,
                   const _dbNameCache& rhs) const;
  void out(dbDiff& diff, char side, const char* field) const;

  // Find the name, returns 0 if the name does not exists.
  uint findName(const char* name);

  // Add this name to the table if it does not exists.
  // increment the reference count to this name
  uint addName(const char* name);

  // Remove this name to the table if the ref-cnt is 0
  void removeName(uint id);

  // Remove the string this id represents
  const char* getName(uint id);
};

dbOStream& operator<<(dbOStream& stream, const _dbNameCache& net);
dbIStream& operator>>(dbIStream& stream, _dbNameCache& net);

}  // namespace odb
