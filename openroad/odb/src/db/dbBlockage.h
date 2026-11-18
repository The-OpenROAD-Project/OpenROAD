// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdint>

#include "dbCore.h"
#include "odb/dbId.h"

namespace odb {

class _dbInst;
class _dbBox;
class _dbDatabase;
class dbIStream;
class dbOStream;

struct _dbBlockageFlags
{
  uint32_t pushed_down : 1;
  uint32_t soft : 1;
  // For denoting that the blockage is not to be written or
  // rendered. It only exists to support non-rectangular
  // floorplans.
  uint32_t is_system_reserved : 1;
  uint32_t spare_bits : 29;
};

class _dbBlockage : public _dbObject
{
 public:
  _dbBlockage(_dbDatabase* db);
  _dbBlockage(_dbDatabase* db, const _dbBlockage& b);

  _dbInst* getInst();
  _dbBox* getBBox() const;

  bool operator==(const _dbBlockage& rhs) const;
  bool operator!=(const _dbBlockage& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbBlockage& rhs) const;
  void collectMemInfo(MemInfo& info);

  _dbBlockageFlags flags_;
  dbId<_dbInst> inst_;
  dbId<_dbBox> bbox_;
  float max_density_;
};

inline _dbBlockage::_dbBlockage(_dbDatabase*)
{
  flags_.pushed_down = 0;
  flags_.spare_bits = 0;
  flags_.soft = 0;
  flags_.is_system_reserved = 0;
  max_density_ = 0.0;
}

inline _dbBlockage::_dbBlockage(_dbDatabase*, const _dbBlockage& b)
    : flags_(b.flags_),
      inst_(b.inst_),
      bbox_(b.bbox_),
      max_density_(b.max_density_)
{
}

inline dbOStream& operator<<(dbOStream& stream, const _dbBlockage& blockage)
{
  uint32_t* bit_field = (uint32_t*) &blockage.flags_;
  stream << *bit_field;
  stream << blockage.inst_;
  stream << blockage.bbox_;
  stream << blockage.max_density_;
  return stream;
}

inline dbIStream& operator>>(dbIStream& stream, _dbBlockage& blockage)
{
  uint32_t* bit_field = (uint32_t*) &blockage.flags_;
  stream >> *bit_field;
  stream >> blockage.inst_;
  stream >> blockage.bbox_;
  stream >> blockage.max_density_;
  return stream;
}

}  // namespace odb
