// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "dbCore.h"
#include "dbVector.h"
#include "odb/dbId.h"
#include "odb/dbTypes.h"
#include "odb/odb.h"

namespace odb {

class _dbWire;
class _dbNet;

struct _dbWireFlags
{
  uint _is_global : 1;
  uint _spare_bits : 31;
};

class _dbWire : public _dbObject
{
 public:
  _dbWireFlags _flags;
  dbVector<int> _data;
  dbVector<unsigned char> _opcodes;
  dbId<_dbNet> _net;

  _dbWire(_dbDatabase*)
  {
    _flags._is_global = 0;
    _flags._spare_bits = 0;
  }

  _dbWire(_dbDatabase*, const _dbWire& w)
      : _flags(w._flags), _data(w._data), _opcodes(w._opcodes), _net(w._net)
  {
  }

  ~_dbWire() {}

  uint length() { return _opcodes.size(); }

  bool operator==(const _dbWire& rhs) const;
  bool operator!=(const _dbWire& rhs) const { return !operator==(rhs); }
  void collectMemInfo(MemInfo& info);
};

dbOStream& operator<<(dbOStream& stream, const _dbWire& wire);
dbIStream& operator>>(dbIStream& stream, _dbWire& wire);

}  // namespace odb
