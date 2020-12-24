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
#include "dbHashTable.h"
#include "odb.h"

namespace odb {

template <class T>
class dbTable;
class _dbProperty;
class dbPropertyItr;
class _dbNameCache;
class _dbTech;
class _dbMaster;
class _dbSite;
class _dbDatabase;
class dbIStream;
class dbOStream;
class dbDiff;

class _dbLib : public _dbObject
{
 public:
  // PERSISTANT-MEMBERS
  int                    _lef_units;
  int                    _dbu_per_micron;  // cached value from dbTech
  char                   _hier_delimeter;
  char                   _left_bus_delimeter;
  char                   _right_bus_delimeter;
  char                   _spare;
  char*                  _name;
  dbHashTable<_dbMaster> _master_hash;
  dbHashTable<_dbSite>   _site_hash;

  // NON-PERSISTANT-MEMBERS
  dbTable<_dbMaster>*   _master_tbl;
  dbTable<_dbSite>*     _site_tbl;
  dbTable<_dbProperty>* _prop_tbl;
  _dbNameCache*         _name_cache;

  dbPropertyItr* _prop_itr;

  _dbLib(_dbDatabase* db);
  _dbLib(_dbDatabase* db, const _dbLib& l);
  ~_dbLib();
  bool operator==(const _dbLib& rhs) const;
  bool operator!=(const _dbLib& rhs) const { return !operator==(rhs); }
  void differences(dbDiff& diff, const char* field, const _dbLib& rhs) const;
  void out(dbDiff& diff, char side, const char* field) const;
  dbObjectTable* getObjectTable(dbObjectType type);
};

dbOStream& operator<<(dbOStream& stream, const _dbLib& lib);
dbIStream& operator>>(dbIStream& stream, _dbLib& lib);

}  // namespace odb
