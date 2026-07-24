// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <cstdint>

#include "dbCore.h"
#include "odb/dbId.h"

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;
class _dbChipNet;
class _dbChipBumpInst;

class _dbChipCapNode : public _dbObject
{
 public:
  _dbChipCapNode(_dbDatabase*);

  bool operator==(const _dbChipCapNode& rhs) const;
  bool operator!=(const _dbChipCapNode& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbChipCapNode& rhs) const;
  void collectMemInfo(MemInfo& info);

  dbId<_dbChipNet> chip_net_;
  dbId<_dbChipCapNode> next_chip_cap_node_;
  dbId<_dbChipBumpInst> chip_bump_inst_;
  float capacitance_;
};
dbIStream& operator>>(dbIStream& stream, _dbChipCapNode& obj);
dbOStream& operator<<(dbOStream& stream, const _dbChipCapNode& obj);
}  // namespace odb
// Generator Code End Header