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
class dbIStream;
class dbOStream;
class dbSite;
class dbLib;
class dbDiff;
class _dbSite;
class _dbLib;

struct dbRowFlags
{
  dbOrientType::Value _orient : 4;
  dbRowDir::Value _dir : 2;
  uint _spare_bits : 26;
};

class _dbRow : public _dbObject
{
 public:
  // PERSISTANT-MEMBERS
  dbRowFlags _flags;
  char* _name;
  dbId<_dbLib> _lib;
  dbId<_dbSite> _site;
  int _x;
  int _y;
  int _site_cnt;
  int _spacing;

  _dbRow(_dbDatabase*, const _dbRow& r);
  _dbRow(_dbDatabase*);
  ~_dbRow();

  bool operator==(const _dbRow& rhs) const;
  bool operator!=(const _dbRow& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbRow& rhs) const;
  void differences(dbDiff& diff, const char* field, const _dbRow& rhs) const;
  void out(dbDiff& diff, char side, const char* field) const;
};

inline _dbRow::_dbRow(_dbDatabase*, const _dbRow& r)
    : _flags(r._flags),
      _name(NULL),
      _lib(r._lib),
      _site(r._site),
      _x(r._x),
      _y(r._y),
      _site_cnt(r._site_cnt),
      _spacing(r._spacing)
{
  if (r._name) {
    _name = strdup(r._name);
    ZALLOCATED(_name);
  }
}

inline _dbRow::_dbRow(_dbDatabase*)
{
  _flags._orient = dbOrientType::R0;
  _flags._dir = dbRowDir::HORIZONTAL;
  _flags._spare_bits = 0;
  _name = NULL;
  _x = 0;
  _y = 0;
  _site_cnt = 0;
  _spacing = 0;
}

inline _dbRow::~_dbRow()
{
  if (_name)
    free((void*) _name);
}

inline dbOStream& operator<<(dbOStream& stream, const _dbRow& row)
{
  uint* bit_field = (uint*) &row._flags;
  stream << *bit_field;
  stream << row._name;
  stream << row._lib;
  stream << row._site;
  stream << row._x;
  stream << row._y;
  stream << row._site_cnt;
  stream << row._spacing;
  return stream;
}

inline dbIStream& operator>>(dbIStream& stream, _dbRow& row)
{
  uint* bit_field = (uint*) &row._flags;
  stream >> *bit_field;
  stream >> row._name;
  stream >> row._lib;
  stream >> row._site;
  stream >> row._x;
  stream >> row._y;
  stream >> row._site_cnt;
  stream >> row._spacing;
  return stream;
}

}  // namespace odb
