// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#pragma once

#include "dbCore.h"
#include "odb/dbId.h"
#include "odb/dbTypes.h"
#include "odb/odb.h"

namespace odb {

template <class T>
class dbTable;
class dbIStream;
class dbOStream;
class dbSite;
class dbLib;
class _dbTechLayer;

struct dbFillFlags
{
  uint _opc : 1;
  uint _mask_id : 2;
  uint _layer_id : 8;
  uint _spare_bits : 21;
};

class _dbFill : public _dbObject
{
 public:
  // PERSISTANT-MEMBERS
  dbFillFlags _flags;
  Rect _rect;

  _dbFill(_dbDatabase*, const _dbFill& r);
  _dbFill(_dbDatabase*);

  _dbTechLayer* getTechLayer() const;

  bool operator==(const _dbFill& rhs) const;
  bool operator!=(const _dbFill& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbFill& rhs) const;
  void collectMemInfo(MemInfo& info);
};

inline _dbFill::_dbFill(_dbDatabase*, const _dbFill& r)
    : _flags(r._flags), _rect(r._rect)
{
}

inline _dbFill::_dbFill(_dbDatabase*)
{
  _flags._opc = false;
  _flags._mask_id = 0;
  _flags._layer_id = 0;
  _flags._spare_bits = 0;
}

inline dbOStream& operator<<(dbOStream& stream, const _dbFill& fill)
{
  uint* bit_field = (uint*) &fill._flags;
  stream << *bit_field;
  stream << fill._rect;
  return stream;
}

inline dbIStream& operator>>(dbIStream& stream, _dbFill& fill)
{
  uint* bit_field = (uint*) &fill._flags;
  stream >> *bit_field;
  stream >> fill._rect;
  return stream;
}

}  // namespace odb
