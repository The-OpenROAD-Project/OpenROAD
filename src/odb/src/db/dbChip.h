// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "dbCore.h"
#include "odb/odb.h"

namespace odb {

template <class T>
class dbTable;
class _dbProperty;
class dbPropertyItr;
class _dbNameCache;
class _dbBlock;
class _dbDatabase;
class dbBlockItr;
class dbIStream;
class dbOStream;

class _dbChip : public _dbObject
{
 public:
  // PERSISTANT-MEMBERS
  dbId<_dbBlock> _top;

  // NON-PERSISTANT-MEMBERS
  dbTable<_dbBlock>* _block_tbl;
  dbTable<_dbProperty>* _prop_tbl;
  _dbNameCache* _name_cache;
  dbBlockItr* _block_itr;
  dbPropertyItr* _prop_itr;

  _dbChip(_dbDatabase* db);
  ~_dbChip();

  bool operator==(const _dbChip& rhs) const;
  bool operator!=(const _dbChip& rhs) const { return !operator==(rhs); }
  dbObjectTable* getObjectTable(dbObjectType type);
  void collectMemInfo(MemInfo& info);
};

dbOStream& operator<<(dbOStream& stream, const _dbChip& chip);
dbIStream& operator>>(dbIStream& stream, _dbChip& chip);

}  // namespace odb
