// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdlib>

#include "dbCommon.h"
#include "dbCore.h"
#include "odb/dbId.h"
#include "odb/dbTypes.h"
#include "odb/odb.h"

namespace odb {

class dbIStream;
class dbOStream;
class dbSite;
class dbLib;
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
  dbRowFlags flags_;
  char* name_;
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
  void collectMemInfo(MemInfo& info);
};

inline _dbRow::_dbRow(_dbDatabase*, const _dbRow& r)
    : flags_(r.flags_),
      name_(nullptr),
      _lib(r._lib),
      _site(r._site),
      _x(r._x),
      _y(r._y),
      _site_cnt(r._site_cnt),
      _spacing(r._spacing)
{
  if (r.name_) {
    name_ = safe_strdup(r.name_);
  }
}

inline _dbRow::_dbRow(_dbDatabase*)
{
  flags_._orient = dbOrientType::R0;
  flags_._dir = dbRowDir::HORIZONTAL;
  flags_._spare_bits = 0;
  name_ = nullptr;
  _x = 0;
  _y = 0;
  _site_cnt = 0;
  _spacing = 0;
}

inline _dbRow::~_dbRow()
{
  if (name_) {
    free((void*) name_);
  }
}

inline dbOStream& operator<<(dbOStream& stream, const _dbRow& row)
{
  uint* bit_field = (uint*) &row.flags_;
  stream << *bit_field;
  stream << row.name_;
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
  uint* bit_field = (uint*) &row.flags_;
  stream >> *bit_field;
  stream >> row.name_;
  stream >> row._lib;
  stream >> row._site;
  stream >> row._x;
  stream >> row._y;
  stream >> row._site_cnt;
  stream >> row._spacing;
  return stream;
}

}  // namespace odb
