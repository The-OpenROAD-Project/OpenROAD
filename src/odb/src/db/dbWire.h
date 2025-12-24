// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdint>

#include "dbCore.h"
#include "dbVector.h"
#include "odb/dbId.h"
#include "odb/dbStream.h"
#include "odb/dbTypes.h"

namespace odb {

class _dbWire;
class _dbNet;

struct _dbWireFlags
{
  uint32_t is_global : 1;
  uint32_t spare_bits : 31;
};

class _dbWire : public _dbObject
{
 public:
  _dbWire(_dbDatabase*)
  {
    flags_.is_global = 0;
    flags_.spare_bits = 0;
  }

  _dbWire(_dbDatabase*, const _dbWire& w)
      : flags_(w.flags_), data_(w.data_), opcodes_(w.opcodes_), net_(w.net_)
  {
  }

  uint32_t length() { return opcodes_.size(); }

  bool operator==(const _dbWire& rhs) const;
  bool operator!=(const _dbWire& rhs) const { return !operator==(rhs); }
  void collectMemInfo(MemInfo& info);

  _dbWireFlags flags_;
  dbVector<int> data_;
  dbVector<unsigned char> opcodes_;
  dbId<_dbNet> net_;
};

dbOStream& operator<<(dbOStream& stream, const _dbWire& wire);
dbIStream& operator>>(dbIStream& stream, _dbWire& wire);

}  // namespace odb
