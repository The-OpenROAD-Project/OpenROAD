// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <cstdint>

#include "dbCore.h"
#include "dbVector.h"
#include "odb/dbId.h"

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;
class _dbChipNet;
class _dbUnfoldedChipBumpInst;

class _dbUnfoldedChipNet : public _dbObject
{
 public:
  _dbUnfoldedChipNet(_dbDatabase*);

  bool operator==(const _dbUnfoldedChipNet& rhs) const;
  bool operator!=(const _dbUnfoldedChipNet& rhs) const
  {
    return !operator==(rhs);
  }
  bool operator<(const _dbUnfoldedChipNet& rhs) const;
  void collectMemInfo(MemInfo& info);

  dbId<_dbChipNet> chip_net_;
  dbVector<dbId<_dbUnfoldedChipBumpInst>> connected_bumps_;
};
dbIStream& operator>>(dbIStream& stream, _dbUnfoldedChipNet& obj);
dbOStream& operator<<(dbOStream& stream, const _dbUnfoldedChipNet& obj);
}  // namespace odb
// Generator Code End Header