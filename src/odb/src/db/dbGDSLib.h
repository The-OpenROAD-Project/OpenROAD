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

#pragma once

#include <ctime>

#include "dbCore.h"
#include "dbGDSStructure.h"
#include "dbHashTable.hpp"
#include "dbTable.hpp"
#include "odb/db.h"
#include "odb/odb.h"

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;
class _dbGDSStructure;

class _dbGDSLib : public _dbObject
{
 public:
  std::string _libname;
  double _uu_per_dbu;
  double _dbu_per_meter;
  dbHashTable<_dbGDSStructure> _gdsstructure_hash;
  dbTable<_dbGDSStructure>* _gdsstructure_tbl;

  _dbGDSLib(_dbDatabase*, const _dbGDSLib& r);
  _dbGDSLib(_dbDatabase*);
  ~_dbGDSLib();

  bool operator==(const _dbGDSLib& rhs) const;
  bool operator!=(const _dbGDSLib& rhs) const { return !operator==(rhs); }
  dbObjectTable* getObjectTable(dbObjectType type);
  void collectMemInfo(MemInfo& info);

  _dbGDSStructure* findStructure(const char* name);
};

dbIStream& operator>>(dbIStream& stream, std::tm& tm);
dbOStream& operator<<(dbOStream& stream, const std::tm& tm);

dbIStream& operator>>(dbIStream& stream, _dbGDSLib& obj);
dbOStream& operator<<(dbOStream& stream, const _dbGDSLib& obj);
}  // namespace odb
