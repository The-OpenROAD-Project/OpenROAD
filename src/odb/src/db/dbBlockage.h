// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "dbCore.h"
#include "odb/dbId.h"
#include "odb/odb.h"

namespace odb {

class _dbInst;
class _dbBox;
class _dbDatabase;
class dbIStream;
class dbOStream;

struct _dbBlockageFlags
{
  uint _pushed_down : 1;
  uint _soft : 1;
  // For denoting that the blockage is not to be written or
  // rendered. It only exists to support non-rectangular
  // floorplans.
  uint _is_system_reserved : 1;
  uint _spare_bits : 29;
};

class _dbBlockage : public _dbObject
{
 public:
  _dbBlockageFlags _flags;
  dbId<_dbInst> _inst;
  dbId<_dbBox> _bbox;
  float _max_density;

  _dbBlockage(_dbDatabase* db);
  _dbBlockage(_dbDatabase* db, const _dbBlockage& b);
  ~_dbBlockage();

  _dbInst* getInst();
  _dbBox* getBBox() const;

  bool operator==(const _dbBlockage& rhs) const;
  bool operator!=(const _dbBlockage& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbBlockage& rhs) const;
  void collectMemInfo(MemInfo& info);
};

inline _dbBlockage::_dbBlockage(_dbDatabase*)
{
  _flags._pushed_down = 0;
  _flags._spare_bits = 0;
  _flags._soft = 0;
  _flags._is_system_reserved = 0;
  _max_density = 0.0;
}

inline _dbBlockage::_dbBlockage(_dbDatabase*, const _dbBlockage& b)
    : _flags(b._flags),
      _inst(b._inst),
      _bbox(b._bbox),
      _max_density(b._max_density)
{
}

inline _dbBlockage::~_dbBlockage()
{
}

inline dbOStream& operator<<(dbOStream& stream, const _dbBlockage& blockage)
{
  uint* bit_field = (uint*) &blockage._flags;
  stream << *bit_field;
  stream << blockage._inst;
  stream << blockage._bbox;
  stream << blockage._max_density;
  return stream;
}

inline dbIStream& operator>>(dbIStream& stream, _dbBlockage& blockage)
{
  uint* bit_field = (uint*) &blockage._flags;
  stream >> *bit_field;
  stream >> blockage._inst;
  stream >> blockage._bbox;
  stream >> blockage._max_density;
  return stream;
}

}  // namespace odb
