// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "dbBox.h"
#include "odb/dbId.h"
#include "odb/dbObject.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "odb/odb.h"

namespace odb {

class _dbDatabase;
class dbIStream;
class dbOStream;

struct _dbSBoxFlags
{
  dbWireShapeType::Value _wire_type : 6;
  uint _direction : 2;  // 0 = undefiend, 1 = horizontal, 2 = vertical, 3 =
                        // octilinear
  uint _via_bottom_mask : 2;
  uint _via_cut_mask : 2;
  uint _via_top_mask : 2;
  uint _spare_bits : 18;
};

class _dbSBox : public _dbBox
{
 public:
  // PERSISTANT-MEMBERS
  _dbSBoxFlags _sflags;

  _dbSBox(_dbDatabase*, const _dbSBox& b);
  _dbSBox(_dbDatabase*);
  ~_dbSBox();

  bool operator==(const _dbSBox& rhs) const;
  bool operator!=(const _dbSBox& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbSBox& rhs) const;
  int equal(const _dbSBox& rhs) const;
};

inline _dbSBox::_dbSBox(_dbDatabase* db, const _dbSBox& b)
    : _dbBox(db, b), _sflags(b._sflags)
{
}

inline _dbSBox::_dbSBox(_dbDatabase* db) : _dbBox(db)
{
  _sflags._wire_type = dbWireShapeType::COREWIRE;
  _sflags._direction = 0;
  _sflags._via_bottom_mask = 0;
  _sflags._via_cut_mask = 0;
  _sflags._via_top_mask = 0;
  _sflags._spare_bits = 0;
}

inline _dbSBox::~_dbSBox()
{
}

inline dbOStream& operator<<(dbOStream& stream, const _dbSBox& box)
{
  stream << (_dbBox&) box;
  uint* bit_field = (uint*) &box._sflags;
  stream << *bit_field;
  return stream;
}

inline dbIStream& operator>>(dbIStream& stream, _dbSBox& box)
{
  stream >> (_dbBox&) box;
  uint* bit_field = (uint*) &box._sflags;
  stream >> *bit_field;
  return stream;
}

}  // namespace odb
