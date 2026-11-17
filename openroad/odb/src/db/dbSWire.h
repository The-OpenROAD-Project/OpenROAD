// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdint>

#include "dbCore.h"
#include "odb/dbId.h"
#include "odb/dbStream.h"
#include "odb/dbTypes.h"

namespace odb {

class _dbSWire;
class _dbNet;
class _dbSBox;

struct _dbSWireFlags
{
  dbWireType::Value wire_type : 6;
  uint32_t spare_bits : 26;
};

class _dbSWire : public _dbObject
{
 public:
  _dbSWire(_dbDatabase*)
  {
    flags_.wire_type = dbWireType::NONE;
    flags_.spare_bits = 0;
  }

  _dbSWire(_dbDatabase*, const _dbSWire& s)
      : flags_(s.flags_),
        net_(s.net_),
        shield_(s.shield_),
        wires_(s.wires_),
        next_swire_(s.next_swire_)
  {
  }

  void addSBox(_dbSBox* box);
  void removeSBox(_dbSBox* box);

  bool operator==(const _dbSWire& rhs) const;
  bool operator!=(const _dbSWire& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbSWire& rhs) const;

  void collectMemInfo(MemInfo& info);

  _dbSWireFlags flags_;
  dbId<_dbNet> net_;
  dbId<_dbNet> shield_;
  dbId<_dbSBox> wires_;
  dbId<_dbSWire> next_swire_;
};

inline dbOStream& operator<<(dbOStream& stream, const _dbSWire& wire)
{
  uint32_t* bit_field = (uint32_t*) &wire.flags_;
  stream << *bit_field;
  stream << wire.net_;
  stream << wire.shield_;
  stream << wire.wires_;
  stream << wire.next_swire_;

  return stream;
}

inline dbIStream& operator>>(dbIStream& stream, _dbSWire& wire)
{
  uint32_t* bit_field = (uint32_t*) &wire.flags_;
  stream >> *bit_field;
  stream >> wire.net_;
  stream >> wire.shield_;
  stream >> wire.wires_;
  stream >> wire.next_swire_;

  return stream;
}

}  // namespace odb
