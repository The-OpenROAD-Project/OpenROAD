// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include "dbCore.h"
#include "odb/geom.h"
#include "odb/odb.h"

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;
class dbPropertyItr;
class _dbNameCache;
class dbBlockItr;
class _dbProperty;
class _dbBlock;
class _dbChipInst;

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

  char* _name;
  uint type_;
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
  dbId<_dbBlock> _top;
  dbTable<_dbBlock>* _block_tbl;
  _dbNameCache* _name_cache;
  dbBlockItr* _block_itr;
  dbPropertyItr* _prop_itr;
  dbId<_dbChipInst> chipinsts_;
  dbTable<_dbProperty>* _prop_tbl;
  dbId<_dbChip> _next_entry;
};
dbIStream& operator>>(dbIStream& stream, _dbChip& obj);
dbOStream& operator<<(dbOStream& stream, const _dbChip& obj);
}  // namespace odb
// Generator Code End Header