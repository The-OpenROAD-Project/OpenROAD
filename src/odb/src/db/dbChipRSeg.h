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
class _dbChipCapNode;

class _dbChipRSeg : public _dbObject
{
 public:
  _dbChipRSeg(_dbDatabase*);

  bool operator==(const _dbChipRSeg& rhs) const;
  bool operator!=(const _dbChipRSeg& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbChipRSeg& rhs) const;
  void collectMemInfo(MemInfo& info);

  dbId<_dbChipRSeg> next_chip_r_seg_;
  dbId<_dbChipCapNode> source_cap_node_;
  dbId<_dbChipCapNode> target_cap_node_;
  float resistance_;
};
dbIStream& operator>>(dbIStream& stream, _dbChipRSeg& obj);
dbOStream& operator<<(dbOStream& stream, const _dbChipRSeg& obj);
}  // namespace odb
// Generator Code End Header