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
class _dbChipBumpInst;
class _dbUnfoldedChipRegionInst;

class _dbUnfoldedChipBumpInst : public _dbObject
{
 public:
  _dbUnfoldedChipBumpInst(_dbDatabase*);

  bool operator==(const _dbUnfoldedChipBumpInst& rhs) const;
  bool operator!=(const _dbUnfoldedChipBumpInst& rhs) const
  {
    return !operator==(rhs);
  }
  bool operator<(const _dbUnfoldedChipBumpInst& rhs) const;
  void collectMemInfo(MemInfo& info);

  dbId<_dbChipBumpInst> chip_bump_inst_;
  dbId<_dbUnfoldedChipRegionInst> parent_region_;
  dbId<_dbUnfoldedChipBumpInst> region_next_;
  // Pads sizeof to a multiple of 8 so dbTable packs every object 8-byte
  // aligned; dbNetwork tags chip-bump Pin* in the low 3 pointer bits.
  // no-serial.
  uint32_t pad_for_pointer_tag_alignment_;
};
dbIStream& operator>>(dbIStream& stream, _dbUnfoldedChipBumpInst& obj);
dbOStream& operator<<(dbOStream& stream, const _dbUnfoldedChipBumpInst& obj);
}  // namespace odb
// Generator Code End Header