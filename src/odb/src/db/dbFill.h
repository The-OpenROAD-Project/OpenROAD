// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#pragma once

#include "dbCore.h"
#include "odb/dbId.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "odb/odb.h"

namespace odb {

class dbIStream;
class dbOStream;
class dbSite;
class dbLib;
class _dbTechLayer;

struct dbFillFlags
{
  uint opc : 1;
  uint mask_id : 2;
  uint layer_id : 8;
  uint spare_bits : 21;
};

class _dbFill : public _dbObject
{
 public:
  _dbFill(_dbDatabase*, const _dbFill& r);
  _dbFill(_dbDatabase*);

  _dbTechLayer* getTechLayer() const;

  bool operator==(const _dbFill& rhs) const;
  bool operator!=(const _dbFill& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbFill& rhs) const;
  void collectMemInfo(MemInfo& info);

  // PERSISTANT-MEMBERS
  dbFillFlags flags_;
  Rect _rect;
};

inline _dbFill::_dbFill(_dbDatabase*, const _dbFill& r)
    : flags_(r.flags_), _rect(r._rect)
{
}

inline _dbFill::_dbFill(_dbDatabase*)
{
  flags_.opc = false;
  flags_.mask_id = 0;
  flags_.layer_id = 0;
  flags_.spare_bits = 0;
}

inline dbOStream& operator<<(dbOStream& stream, const _dbFill& fill)
{
  uint* bit_field = (uint*) &fill.flags_;
  stream << *bit_field;
  stream << fill._rect;
  return stream;
}

inline dbIStream& operator>>(dbIStream& stream, _dbFill& fill)
{
  uint* bit_field = (uint*) &fill.flags_;
  stream >> *bit_field;
  stream >> fill._rect;
  return stream;
}

}  // namespace odb
