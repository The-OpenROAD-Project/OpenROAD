// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "dbCore.h"
#include "odb/dbId.h"
#include "odb/dbTypes.h"
#include "odb/odb.h"

namespace odb {

class _dbSWire;
class _dbNet;
class _dbSBox;

struct _dbSWireFlags
{
  dbWireType::Value _wire_type : 6;
  uint _spare_bits : 26;
};

class _dbSWire : public _dbObject
{
 public:
  _dbSWireFlags _flags;
  dbId<_dbNet> _net;
  dbId<_dbNet> _shield;
  dbId<_dbSBox> _wires;
  dbId<_dbSWire> _next_swire;

  _dbSWire(_dbDatabase*)
  {
    _flags._wire_type = dbWireType::NONE;
    _flags._spare_bits = 0;
  }

  _dbSWire(_dbDatabase*, const _dbSWire& s)
      : _flags(s._flags),
        _net(s._net),
        _shield(s._shield),
        _wires(s._wires),
        _next_swire(s._next_swire)
  {
  }

  ~_dbSWire() {}

  void addSBox(_dbSBox* box);
  void removeSBox(_dbSBox* box);

  bool operator==(const _dbSWire& rhs) const;
  bool operator!=(const _dbSWire& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbSWire& rhs) const;

  void collectMemInfo(MemInfo& info);
};

inline dbOStream& operator<<(dbOStream& stream, const _dbSWire& wire)
{
  uint* bit_field = (uint*) &wire._flags;
  stream << *bit_field;
  stream << wire._net;
  stream << wire._shield;
  stream << wire._wires;
  stream << wire._next_swire;

  return stream;
}

inline dbIStream& operator>>(dbIStream& stream, _dbSWire& wire)
{
  uint* bit_field = (uint*) &wire._flags;
  stream >> *bit_field;
  stream >> wire._net;
  stream >> wire._shield;
  stream >> wire._wires;
  stream >> wire._next_swire;

  return stream;
}

}  // namespace odb
