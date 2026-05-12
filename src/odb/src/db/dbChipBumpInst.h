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
class _dbChipBump;
class _dbChipRegionInst;

class _dbChipBumpInst : public _dbObject
{
 public:
  _dbChipBumpInst(_dbDatabase*);

  bool operator==(const _dbChipBumpInst& rhs) const;
  bool operator!=(const _dbChipBumpInst& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbChipBumpInst& rhs) const;
  void collectMemInfo(MemInfo& info);

  dbId<_dbChipBump> chip_bump_;
  dbId<_dbChipRegionInst> chip_region_inst_;
  dbId<_dbChipBumpInst> region_next_;
  // dbSta encodes Pin* via lower-3-bits pointer tag; the base pointer must
  // be 8-byte aligned. _dbObject(8B) + three dbId(4B) = 20B leaves the
  // dbTable storage stride at 20, alternating 8- and 4-aligned addresses.
  // Padding to 24 restores 8-alignment for every object.
  uint32_t pad_for_pointer_tag_alignment_ = 0;
};
dbIStream& operator>>(dbIStream& stream, _dbChipBumpInst& obj);
dbOStream& operator<<(dbOStream& stream, const _dbChipBumpInst& obj);
}  // namespace odb
// Generator Code End Header