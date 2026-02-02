// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <cstdint>
#include <string>

#include "dbCore.h"
#include "odb/dbId.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;
class _dbChipBump;
class _dbTechLayer;

class _dbChipRegion : public _dbObject
{
 public:
  _dbChipRegion(_dbDatabase*);

  ~_dbChipRegion();

  bool operator==(const _dbChipRegion& rhs) const;
  bool operator!=(const _dbChipRegion& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbChipRegion& rhs) const;
  dbObjectTable* getObjectTable(dbObjectType type);
  void collectMemInfo(MemInfo& info);

  std::string name_;
  uint8_t side_;
  dbId<_dbTechLayer> layer_;
  Rect box_;
  int z_min_;
  int z_max_;
  dbTable<_dbChipBump>* chip_bump_tbl_;
};
dbIStream& operator>>(dbIStream& stream, _dbChipRegion& obj);
dbOStream& operator<<(dbOStream& stream, const _dbChipRegion& obj);
}  // namespace odb
// Generator Code End Header
