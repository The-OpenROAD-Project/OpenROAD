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

#include "dbCore.h"
#include "odb.h"

namespace odb {

//
// When changing the database schema please add a #define to refer to the schema
// changes. Use the define statement along with the isSchema(rev) method:
//
// GOOD:
//
//    if ( db->isSchema(db_schema_initial) )
//    {
//     ....
//    }
//
// Don't use a revision number in the code, because it is hard to read:
//
// BAD:
//
//    if ( db->_schema_minor > 33 )
//    {
//     ....
//    }
//

//
// Schema Revisions
//
const uint db_schema_major   = 0;  // Not used...
const uint db_schema_initial = 51;
const uint db_schema_minor   = 51;  // Current revision number

template <class T>
class dbTable;
class _dbProperty;
class dbPropertyItr;
class _dbNameCache;
class _dbTech;
class _dbChip;
class _dbLib;
class dbOStream;
class dbIStream;
class dbDiff;

class _dbDatabase : public _dbObject
{
 public:
  // PERSISTANT_MEMBERS
  uint          _magic1;
  uint          _magic2;
  uint          _schema_major;
  uint          _schema_minor;
  uint          _master_id;  // for a unique id across all libraries
  dbId<_dbChip> _chip;
  dbId<_dbTech> _tech;

  // NON_PERSISTANT_MEMBERS
  dbTable<_dbTech>*     _tech_tbl;
  dbTable<_dbLib>*      _lib_tbl;
  dbTable<_dbChip>*     _chip_tbl;
  dbTable<_dbProperty>* _prop_tbl;
  _dbNameCache*         _name_cache;
  dbPropertyItr*        _prop_itr;
  int                   _unique_id;

  char* _file;

  _dbDatabase(_dbDatabase* db);
  _dbDatabase(_dbDatabase* db, int id);
  _dbDatabase(_dbDatabase* db, const _dbDatabase& d);
  ~_dbDatabase();

  bool operator==(const _dbDatabase& rhs) const;
  bool operator!=(const _dbDatabase& rhs) const { return !operator==(rhs); }
  void differences(dbDiff&            diff,
                   const char*        field,
                   const _dbDatabase& rhs) const;
  void out(dbDiff& diff, char side, const char* field) const;

  bool           isSchema(uint rev) { return _schema_minor >= rev; }
  bool           isLessThanSchema(uint rev) { return _schema_minor < rev; }
  dbObjectTable* getObjectTable(dbObjectType type);
};

dbOStream& operator<<(dbOStream& stream, const _dbDatabase& db);
dbIStream& operator>>(dbIStream& stream, _dbDatabase& db);

}  // namespace odb
