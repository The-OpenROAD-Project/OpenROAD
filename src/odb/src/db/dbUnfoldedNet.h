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
class _dbUnfoldedBumpInst;

class _dbUnfoldedNet : public _dbObject
{
 public:
  _dbUnfoldedNet(_dbDatabase*);

  bool operator==(const _dbUnfoldedNet& rhs) const;
  bool operator!=(const _dbUnfoldedNet& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbUnfoldedNet& rhs) const;
  void collectMemInfo(MemInfo& info);

  dbId<_dbChipNet> chip_net_;
  dbVector<dbId<_dbUnfoldedBumpInst>> connected_bumps_;
};
dbIStream& operator>>(dbIStream& stream, _dbUnfoldedNet& obj);
dbOStream& operator<<(dbOStream& stream, const _dbUnfoldedNet& obj);
}  // namespace odb
// Generator Code End Header