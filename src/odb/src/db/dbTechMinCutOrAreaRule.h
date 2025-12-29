// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdint>

#include "dbCore.h"
#include "odb/dbId.h"
#include "odb/dbTypes.h"

namespace odb {

class _dbDatabase;
class _dbTechLayer;
class dbIStream;
class dbOStream;

class _dbTechMinCutRule : public _dbObject
{
 public:
  // PERSISTENT-MEMBERS
  enum RuleType
  {
    kNone,
    kMinimumCut,
    kMinimumCutAbove,
    kMinimumCutBelow
  };

  struct Flags
  {
    RuleType rule : 3;
    uint32_t cuts_length : 1;
    uint32_t spare_bits : 28;
  };

  _dbTechMinCutRule(_dbDatabase* db, const _dbTechMinCutRule& r);
  _dbTechMinCutRule(_dbDatabase* db);

  bool operator==(const _dbTechMinCutRule& rhs) const;
  bool operator!=(const _dbTechMinCutRule& rhs) const
  {
    return !operator==(rhs);
  }
  void collectMemInfo(MemInfo& info);

  Flags flags_;
  uint32_t num_cuts_;
  uint32_t width_;
  int cut_distance_;
  uint32_t length_;
  uint32_t distance_;
};

inline _dbTechMinCutRule::_dbTechMinCutRule(_dbDatabase* /* unused: db */,
                                            const _dbTechMinCutRule& r)
    : flags_(r.flags_),
      num_cuts_(r.num_cuts_),
      width_(r.width_),
      cut_distance_(r.cut_distance_),
      length_(r.length_),
      distance_(r.distance_)
{
}

inline _dbTechMinCutRule::_dbTechMinCutRule(_dbDatabase* /* unused: db */)
{
  flags_.rule = _dbTechMinCutRule::kNone;
  flags_.cuts_length = 0;
  flags_.spare_bits = 0;
  num_cuts_ = 0;
  width_ = 0;
  cut_distance_ = -1;
  length_ = 0;
  distance_ = 0;
}

dbOStream& operator<<(dbOStream& stream, const _dbTechMinCutRule& rule);
dbIStream& operator>>(dbIStream& stream, _dbTechMinCutRule& rule);

class _dbTechMinEncRule : public _dbObject
{
 public:
  // PERSISTENT-MEMBERS

  struct Flags
  {
    uint32_t has_width : 1;
    uint32_t spare_bits : 31;
  };

  Flags flags_;
  uint32_t area_;
  uint32_t width_;

  _dbTechMinEncRule(_dbDatabase* db);
  _dbTechMinEncRule(_dbDatabase* db, const _dbTechMinEncRule& r);

  bool operator==(const _dbTechMinEncRule& rhs) const;
  bool operator!=(const _dbTechMinEncRule& rhs) const
  {
    return !operator==(rhs);
  }
  void collectMemInfo(MemInfo& info);
};

inline _dbTechMinEncRule::_dbTechMinEncRule(_dbDatabase* /* unused: db */,
                                            const _dbTechMinEncRule& r)
    : flags_(r.flags_), area_(r.area_), width_(r.width_)
{
}

inline _dbTechMinEncRule::_dbTechMinEncRule(_dbDatabase* /* unused: db */)
{
  flags_.has_width = 0;
  flags_.spare_bits = 0;
  area_ = 0;
  width_ = 0;
}

dbOStream& operator<<(dbOStream& stream, const _dbTechMinEncRule& rule);
dbIStream& operator>>(dbIStream& stream, _dbTechMinEncRule& rule);

}  // namespace odb
