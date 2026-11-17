// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <cstdlib>

#include "dbCommon.h"
#include "dbCore.h"
#include "odb/dbId.h"
#include "odb/dbTypes.h"

namespace odb {

class dbIStream;
class dbOStream;
class dbSite;
class dbLib;
class _dbSite;
class _dbLib;

struct dbRowFlags
{
  dbOrientType::Value orient : 4;
  dbRowDir::Value dir : 2;
  uint32_t spare_bits : 26;
};

class _dbRow : public _dbObject
{
 public:
  _dbRow(_dbDatabase*, const _dbRow& r);
  _dbRow(_dbDatabase*);
  ~_dbRow();

  bool operator==(const _dbRow& rhs) const;
  bool operator!=(const _dbRow& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbRow& rhs) const;
  void collectMemInfo(MemInfo& info);

  // PERSISTANT-MEMBERS
  dbRowFlags flags_;
  char* name_;
  dbId<_dbLib> lib_;
  dbId<_dbSite> site_;
  int x_;
  int y_;
  int site_cnt_;
  int spacing_;
};

inline _dbRow::_dbRow(_dbDatabase*, const _dbRow& r)
    : flags_(r.flags_),
      name_(nullptr),
      lib_(r.lib_),
      site_(r.site_),
      x_(r.x_),
      y_(r.y_),
      site_cnt_(r.site_cnt_),
      spacing_(r.spacing_)
{
  if (r.name_) {
    name_ = safe_strdup(r.name_);
  }
}

inline _dbRow::_dbRow(_dbDatabase*)
{
  flags_.orient = dbOrientType::R0;
  flags_.dir = dbRowDir::HORIZONTAL;
  flags_.spare_bits = 0;
  name_ = nullptr;
  x_ = 0;
  y_ = 0;
  site_cnt_ = 0;
  spacing_ = 0;
}

inline _dbRow::~_dbRow()
{
  if (name_) {
    free((void*) name_);
  }
}

inline dbOStream& operator<<(dbOStream& stream, const _dbRow& row)
{
  uint32_t* bit_field = (uint32_t*) &row.flags_;
  stream << *bit_field;
  stream << row.name_;
  stream << row.lib_;
  stream << row.site_;
  stream << row.x_;
  stream << row.y_;
  stream << row.site_cnt_;
  stream << row.spacing_;
  return stream;
}

inline dbIStream& operator>>(dbIStream& stream, _dbRow& row)
{
  uint32_t* bit_field = (uint32_t*) &row.flags_;
  stream >> *bit_field;
  stream >> row.name_;
  stream >> row.lib_;
  stream >> row.site_;
  stream >> row.x_;
  stream >> row.y_;
  stream >> row.site_cnt_;
  stream >> row.spacing_;
  return stream;
}

}  // namespace odb
