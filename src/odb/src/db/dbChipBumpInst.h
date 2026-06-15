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
  // Pads sizeof to a multiple of 8 so objects pack on 8-byte boundaries in
  // the dbTable page, keeping them safe as low-3-bit pointer-tag targets.
  // NOTE: dbNetwork no longer tags _dbChipBumpInst* directly (chip-bump Pin*
  // now tags the per-unfold-path _dbUnfoldedChipBumpInst*), so this pad is
  // currently unused by STA; kept to preserve the 8-byte invariant in case a
  // raw bump-inst is ever tagged again. no-serial: not persisted.
  uint32_t pad_for_pointer_tag_alignment_;
};
dbIStream& operator>>(dbIStream& stream, _dbChipBumpInst& obj);
dbOStream& operator<<(dbOStream& stream, const _dbChipBumpInst& obj);
}  // namespace odb
// Generator Code End Header