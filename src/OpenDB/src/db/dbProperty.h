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
#include "dbId.h"
#include "dbTypes.h"
#include "odb.h"

namespace odb {

template <class T>
class dbTable;
class _dbDatabase;
class _dbProperty;
class dbPropertyItr;
class dbIStream;
class dbOStream;
class dbDiff;
class _dbNameCache;

enum _PropTypeEnum
{
  // Do not change the order of this enum.
  DB_STRING_PROP = 0,
  DB_BOOL_PROP   = 1,
  DB_INT_PROP    = 2,
  DB_DOUBLE_PROP = 3
};

struct _dbPropertyFlags
{
  _PropTypeEnum _type : 4;
  uint          _owner_type : 8;
  uint          _spare_bits : 20;
};

class _dbProperty : public _dbObject
{
 public:
  _dbPropertyFlags  _flags;
  uint              _name;
  dbId<_dbProperty> _next;
  uint              _owner;

  union
  {
    char*  _str_val;
    uint   _bool_val;
    int    _int_val;
    double _double_val;
  } _value;

  _dbProperty(_dbDatabase*);
  _dbProperty(_dbDatabase*, const _dbProperty& n);
  ~_dbProperty();

  bool operator==(const _dbProperty& rhs) const;
  bool operator!=(const _dbProperty& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbProperty& rhs) const;
  void differences(dbDiff&            diff,
                   const char*        field,
                   const _dbProperty& rhs) const;
  void out(dbDiff& diff, char side, const char* field) const;

  static dbTable<_dbProperty>* getPropTable(dbObject* object);
  static _dbNameCache*         getNameCache(dbObject* object);
  static dbPropertyItr*        getItr(dbObject* object);
  static _dbProperty*          createProperty(dbObject*     object,
                                              const char*   name,
                                              _PropTypeEnum type);
};

dbOStream& operator<<(dbOStream& stream, const _dbProperty& prop);
dbIStream& operator>>(dbIStream& stream, _dbProperty& prop);

}  // namespace odb
