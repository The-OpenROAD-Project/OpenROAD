// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

#include "dbCore.h"
#include "odb/dbId.h"
#include "odb/geom.h"
// User Code Begin Includes
#include <list>

#include "odb/dbObject.h"
// User Code End Includes

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;
class dbPropertyItr;
class _dbNameCache;
class dbBlockItr;
class dbChipCallBackObj;
class _dbProperty;
class _dbChipRegion;
class _dbMarkerCategory;
class _dbBlock;
class _dbChipInst;
class _dbChipConn;
class _dbChipNet;
class _dbTech;

class _dbChip : public _dbObject
{
 public:
  _dbChip(_dbDatabase*);

  ~_dbChip();

  bool operator==(const _dbChip& rhs) const;
  bool operator!=(const _dbChip& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbChip& rhs) const;
  dbObjectTable* getObjectTable(dbObjectType type);
  void collectMemInfo(MemInfo& info);

  char* name_;
  uint32_t type_;
  Point offset_;
  int width_;
  int height_;
  int thickness_;
  // range (0, 1]
  float shrink_;
  int seal_ring_east_;
  int seal_ring_west_;
  int seal_ring_north_;
  int seal_ring_south_;
  int scribe_line_east_;
  int scribe_line_west_;
  int scribe_line_north_;
  int scribe_line_south_;
  bool tsv_;
  dbId<_dbBlock> top_;
  dbTable<_dbBlock>* block_tbl_;
  _dbNameCache* name_cache_;
  dbBlockItr* block_itr_;
  dbPropertyItr* prop_itr_;
  dbId<_dbChipInst> chipinsts_;
  dbId<_dbChipConn> conns_;
  dbId<_dbChipNet> nets_;
  std::unordered_map<std::string, dbId<_dbChipInst>> chipinsts_map_;
  std::unordered_map<std::string, dbId<_dbChipRegion>> chip_region_map_;
  dbId<_dbTech> tech_;
  std::unordered_map<std::string, dbId<_dbMarkerCategory>>
      marker_categories_map_;
  dbTable<_dbProperty>* prop_tbl_;
  dbTable<_dbChipRegion>* chip_region_tbl_;
  dbTable<_dbMarkerCategory>* marker_categories_tbl_;
  dbId<_dbChip> next_entry_;

  // User Code Begin Fields
  std::list<dbChipCallBackObj*> callbacks_;
  // User Code End Fields
};
dbIStream& operator>>(dbIStream& stream, _dbChip& obj);
dbOStream& operator<<(dbOStream& stream, const _dbChip& obj);
}  // namespace odb
// Generator Code End Header
