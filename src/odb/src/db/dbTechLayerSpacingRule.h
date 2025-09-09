// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "dbCore.h"
#include "odb/dbId.h"
#include "odb/dbTypes.h"
#include "odb/odb.h"

namespace odb {

class _dbDatabase;
class _dbTechLayer;
class dbIStream;
class dbOStream;

class _dbTechLayerSpacingRule : public _dbObject
{
 public:
  enum _RuleType
  {
    DEFAULT = 0,
    RANGE_ONLY,
    RANGE_USELENGTH,
    RANGE_INFLUENCE,
    RANGE_INFLUENCE_RANGE,
    RANGE_RANGE,
    LENGTHTHRESHOLD,
    LENGTHTHRESHOLD_RANGE,
    CUT_LAYER_BELOW,
    ADJACENT_CUTS_INFLUENCE,
    ENDOFLINE,
    ENDOFLINE_PARALLEL,
    ENDOFLINE_PARALLEL_TWOEDGES
  };

  struct _Flword
  {
    _RuleType _rule : 4;
    bool _except_same_pgnet : 1;
    bool _cut_stacking : 1;
    bool _cut_center_to_center : 1;
    bool _cut_same_net : 1;
    bool _cut_parallel_overlap : 1;
    bool _notch_length : 1;
    bool _end_of_notch_width : 1;
    uint _spare_bits : 21;
  };

  // PERSISTENT-MEMBERS
  _Flword _flags;
  uint _spacing;
  uint _length_or_influence;
  uint _r1min;
  uint _r1max;
  uint _r2min;
  uint _r2max;
  uint _cut_area;
  dbId<_dbTechLayer> _layer;
  dbId<_dbTechLayer> _cut_layer_below;

  _dbTechLayerSpacingRule(_dbDatabase*, const _dbTechLayerSpacingRule& r);
  _dbTechLayerSpacingRule(_dbDatabase*);
  ~_dbTechLayerSpacingRule();

  bool operator==(const _dbTechLayerSpacingRule& rhs) const;
  bool operator!=(const _dbTechLayerSpacingRule& rhs) const
  {
    return !operator==(rhs);
  }
  void collectMemInfo(MemInfo& info);
};

inline _dbTechLayerSpacingRule::_dbTechLayerSpacingRule(
    _dbDatabase*,
    const _dbTechLayerSpacingRule& r)
    : _flags(r._flags),
      _spacing(r._spacing),
      _length_or_influence(r._length_or_influence),
      _r1min(r._r1min),
      _r1max(r._r1max),
      _r2min(r._r2min),
      _r2max(r._r2max),
      _cut_area(r._cut_area),
      _layer(r._layer),
      _cut_layer_below(r._cut_layer_below)
{
}

inline _dbTechLayerSpacingRule::_dbTechLayerSpacingRule(_dbDatabase*)
{
  _flags._rule = DEFAULT;
  _flags._except_same_pgnet = false;
  _flags._cut_stacking = false;
  _flags._cut_center_to_center = false;
  _flags._cut_same_net = false;
  _flags._cut_parallel_overlap = false;
  _flags._notch_length = false;
  _flags._end_of_notch_width = false;
  _flags._spare_bits = 0;
  _spacing = 0;
  _length_or_influence = 0;
  _r1min = 0;
  _r1max = 0;
  _r2min = 0;
  _r2max = 0;
  _cut_area = 0;
}

inline _dbTechLayerSpacingRule::~_dbTechLayerSpacingRule()
{
}

dbOStream& operator<<(dbOStream& stream, const _dbTechLayerSpacingRule& rule);
dbIStream& operator>>(dbIStream& stream, _dbTechLayerSpacingRule& rule);

///  This structure defines entries in the V5.5 influence spacing rule table.

class _dbTechV55InfluenceEntry : public _dbObject
{
 public:
  uint _width;
  uint _within;
  uint _spacing;

  _dbTechV55InfluenceEntry(_dbDatabase* db, const _dbTechV55InfluenceEntry& e);
  _dbTechV55InfluenceEntry(_dbDatabase* db);
  ~_dbTechV55InfluenceEntry();
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
    : _width(e._width), _within(e._within), _spacing(e._spacing)
{
}

inline _dbTechV55InfluenceEntry::_dbTechV55InfluenceEntry(
    _dbDatabase* /* unused: db */)
{
  _width = 0;
  _within = 0;
  _spacing = 0;
}

inline _dbTechV55InfluenceEntry::~_dbTechV55InfluenceEntry()
{
}

dbOStream& operator<<(dbOStream& stream,
                      const _dbTechV55InfluenceEntry& infitem);
dbIStream& operator>>(dbIStream& stream, _dbTechV55InfluenceEntry& infitem);

}  // namespace odb
