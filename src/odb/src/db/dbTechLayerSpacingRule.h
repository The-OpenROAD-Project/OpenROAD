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

class _dbTechLayerSpacingRule : public _dbObject
{
 public:
  enum RuleType
  {
    kDefault = 0,
    kRangeOnly,
    kRangeUseLength,
    kRangeInfluence,
    kRangeInfluenceRange,
    kRangeRange,
    kLengthThreshold,
    kLengthThresholdRange,
    kCutLayerBelow,
    kAdjacentCutsInfluence,
    kEndOfLine,
    kEndOfLineParallel,
    kEndOfLineParallelTwoEdges
  };

  struct Flags
  {
    RuleType rule : 4;
    bool except_same_pgnet : 1;
    bool cut_stacking : 1;
    bool cut_center_to_center : 1;
    bool cut_same_net : 1;
    bool cut_parallel_overlap : 1;
    bool notch_length : 1;
    bool end_of_notch_width : 1;
    uint32_t spare_bits : 21;
  };

  _dbTechLayerSpacingRule(_dbDatabase*, const _dbTechLayerSpacingRule& r);
  _dbTechLayerSpacingRule(_dbDatabase*);

  bool operator==(const _dbTechLayerSpacingRule& rhs) const;
  bool operator!=(const _dbTechLayerSpacingRule& rhs) const
  {
    return !operator==(rhs);
  }
  void collectMemInfo(MemInfo& info);

  // PERSISTENT-MEMBERS
  Flags flags_;
  uint32_t spacing_;
  uint32_t length_or_influence_;
  uint32_t r1min_;
  uint32_t r1max_;
  uint32_t r2min_;
  uint32_t r2max_;
  uint32_t cut_area_;
  dbId<_dbTechLayer> layer_;
  dbId<_dbTechLayer> cut_layer_below_;
};

inline _dbTechLayerSpacingRule::_dbTechLayerSpacingRule(
    _dbDatabase*,
    const _dbTechLayerSpacingRule& r)
    : flags_(r.flags_),
      spacing_(r.spacing_),
      length_or_influence_(r.length_or_influence_),
      r1min_(r.r1min_),
      r1max_(r.r1max_),
      r2min_(r.r2min_),
      r2max_(r.r2max_),
      cut_area_(r.cut_area_),
      layer_(r.layer_),
      cut_layer_below_(r.cut_layer_below_)
{
}

inline _dbTechLayerSpacingRule::_dbTechLayerSpacingRule(_dbDatabase*)
{
  flags_.rule = kDefault;
  flags_.except_same_pgnet = false;
  flags_.cut_stacking = false;
  flags_.cut_center_to_center = false;
  flags_.cut_same_net = false;
  flags_.cut_parallel_overlap = false;
  flags_.notch_length = false;
  flags_.end_of_notch_width = false;
  flags_.spare_bits = 0;
  spacing_ = 0;
  length_or_influence_ = 0;
  r1min_ = 0;
  r1max_ = 0;
  r2min_ = 0;
  r2max_ = 0;
  cut_area_ = 0;
}

dbOStream& operator<<(dbOStream& stream, const _dbTechLayerSpacingRule& rule);
dbIStream& operator>>(dbIStream& stream, _dbTechLayerSpacingRule& rule);

///  This structure defines entries in the V5.5 influence spacing rule table.

class _dbTechV55InfluenceEntry : public _dbObject
{
 public:
  uint32_t width_;
  uint32_t within_;
  uint32_t spacing_;

  _dbTechV55InfluenceEntry(_dbDatabase* db, const _dbTechV55InfluenceEntry& e);
  _dbTechV55InfluenceEntry(_dbDatabase* db);

  bool operator==(const _dbTechV55InfluenceEntry& rhs) const;
  bool operator!=(const _dbTechV55InfluenceEntry& rhs) const
  {
    return !operator==(rhs);
  }
  void collectMemInfo(MemInfo& info);
};

inline _dbTechV55InfluenceEntry::_dbTechV55InfluenceEntry(
    _dbDatabase* /* unused: db */,
    const _dbTechV55InfluenceEntry& e)
    : width_(e.width_), within_(e.within_), spacing_(e.spacing_)
{
}

inline _dbTechV55InfluenceEntry::_dbTechV55InfluenceEntry(
    _dbDatabase* /* unused: db */)
{
  width_ = 0;
  within_ = 0;
  spacing_ = 0;
}

dbOStream& operator<<(dbOStream& stream,
                      const _dbTechV55InfluenceEntry& infitem);
dbIStream& operator>>(dbIStream& stream, _dbTechV55InfluenceEntry& infitem);

}  // namespace odb
