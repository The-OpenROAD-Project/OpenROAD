// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

#include "dbCore.h"
#include "odb/dbId.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;
class _dbChip;
class _dbChipRegionInst;
// User Code Begin Classes
class _dbChipRegion;
// User Code End Classes

class _dbChipInst : public _dbObject
{
 public:
  _dbChipInst(_dbDatabase*);

  bool operator==(const _dbChipInst& rhs) const;
  bool operator!=(const _dbChipInst& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbChipInst& rhs) const;
  void collectMemInfo(MemInfo& info);

  std::string name_;
  Point3D origin_;
  dbOrientType3D orient_;
  dbId<_dbChip> master_chip_;
  dbId<_dbChip> parent_chip_;
  dbId<_dbChipInst> chipinst_next_;
  dbId<_dbChipRegionInst> chip_region_insts_;
  std::unordered_map<dbId<_dbChipRegion>, dbId<_dbChipRegionInst>>
      region_insts_map_;
};
dbIStream& operator>>(dbIStream& stream, _dbChipInst& obj);
dbOStream& operator<<(dbOStream& stream, const _dbChipInst& obj);
}  // namespace odb
// Generator Code End Header