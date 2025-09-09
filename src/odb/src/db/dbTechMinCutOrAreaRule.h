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

class _dbTechMinCutRule : public _dbObject
{
 public:
  // PERSISTENT-MEMBERS
  enum _RuleType
  {
    NONE,
    MINIMUM_CUT,
    MINIMUM_CUT_ABOVE,
    MINIMUM_CUT_BELOW
  };

  struct _Flword
  {
    _RuleType _rule : 3;
    uint _cuts_length : 1;
    uint _spare_bits : 28;
  };
  _Flword _flags;
  uint _num_cuts;
  uint _width;
  int _cut_distance;
  uint _length;
  uint _distance;

  _dbTechMinCutRule(_dbDatabase* db, const _dbTechMinCutRule& r);
  _dbTechMinCutRule(_dbDatabase* db);
  ~_dbTechMinCutRule();

  bool operator==(const _dbTechMinCutRule& rhs) const;
  bool operator!=(const _dbTechMinCutRule& rhs) const
  {
    return !operator==(rhs);
  }
  void collectMemInfo(MemInfo& info);
};

inline _dbTechMinCutRule::_dbTechMinCutRule(_dbDatabase* /* unused: db */,
                                            const _dbTechMinCutRule& r)
    : _flags(r._flags),
      _num_cuts(r._num_cuts),
      _width(r._width),
      _cut_distance(r._cut_distance),
      _length(r._length),
      _distance(r._distance)
{
}

inline _dbTechMinCutRule::_dbTechMinCutRule(_dbDatabase* /* unused: db */)
{
  _flags._rule = _dbTechMinCutRule::NONE;
  _flags._cuts_length = 0;
  _flags._spare_bits = 0;
  _num_cuts = 0;
  _width = 0;
  _cut_distance = -1;
  _length = 0;
  _distance = 0;
}

inline _dbTechMinCutRule::~_dbTechMinCutRule()
{
}

dbOStream& operator<<(dbOStream& stream, const _dbTechMinCutRule& rule);
dbIStream& operator>>(dbIStream& stream, _dbTechMinCutRule& rule);

class _dbTechMinEncRule : public _dbObject
{
 public:
  // PERSISTENT-MEMBERS

  struct _Flword
  {
    uint _has_width : 1;
    uint _spare_bits : 31;
  } _flags;

  uint _area;
  uint _width;

  _dbTechMinEncRule(_dbDatabase* db);
  _dbTechMinEncRule(_dbDatabase* db, const _dbTechMinEncRule& r);
  ~_dbTechMinEncRule();
  bool operator==(const _dbTechMinEncRule& rhs) const;
  bool operator!=(const _dbTechMinEncRule& rhs) const
  {
    return !operator==(rhs);
  }
  void collectMemInfo(MemInfo& info);
};

inline _dbTechMinEncRule::_dbTechMinEncRule(_dbDatabase* /* unused: db */,
                                            const _dbTechMinEncRule& r)
    : _flags(r._flags), _area(r._area), _width(r._width)
{
}

inline _dbTechMinEncRule::_dbTechMinEncRule(_dbDatabase* /* unused: db */)
{
  _flags._has_width = 0;
  _flags._spare_bits = 0;
  _area = 0;
  _width = 0;
}

inline _dbTechMinEncRule::~_dbTechMinEncRule()
{
}

dbOStream& operator<<(dbOStream& stream, const _dbTechMinEncRule& rule);
dbIStream& operator>>(dbIStream& stream, _dbTechMinEncRule& rule);

}  // namespace odb
