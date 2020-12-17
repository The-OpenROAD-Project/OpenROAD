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

template <class T>
class dbTable;
class _dbProperty;
class dbPropertyItr;
class _dbNameCache;
class _dbTech;
class _dbBlock;
class _dbDatabase;
class dbBlockItr;
class dbIStream;
class dbOStream;
class dbDiff;

class _dbChip : public _dbObject
{
 public:
  // PERSISTANT-MEMBERS
  dbId<_dbBlock> _top;

  // NON-PERSISTANT-MEMBERS
  dbTable<_dbBlock>*    _block_tbl;
  dbTable<_dbProperty>* _prop_tbl;
  _dbNameCache*         _name_cache;
  dbBlockItr*           _block_itr;
  dbPropertyItr*        _prop_itr;

  _dbChip(_dbDatabase* db);
  _dbChip(_dbDatabase* db, const _dbChip& c);
  ~_dbChip();

  bool operator==(const _dbChip& rhs) const;
  bool operator!=(const _dbChip& rhs) const { return !operator==(rhs); }
  void differences(dbDiff& diff, const char* field, const _dbChip& rhs) const;
  void out(dbDiff& diff, char side, const char* field) const;
  dbObjectTable* getObjectTable(dbObjectType type);
};

dbOStream& operator<<(dbOStream& stream, const _dbChip& chip);
dbIStream& operator>>(dbIStream& stream, _dbChip& chip);

}  // namespace odb
