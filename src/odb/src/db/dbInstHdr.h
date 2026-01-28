// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdint>

#include "dbCore.h"
#include "dbVector.h"
#include "odb/dbId.h"

namespace odb {

class dbBlock;
class dbLib;
class dbMaster;
class _dbLib;
class _dbMaster;
class _dbMTerm;
class _dbDatabase;
class dbIStream;
class dbOStream;

class dbInstHdr : public _dbObject
{
 public:
  dbBlock* getBlock();
  dbLib* getLib();
  dbMaster* getMaster();

  static dbInstHdr* create(dbBlock* block, dbMaster* master);
  static void destroy(dbInstHdr* hdr);
};

class _dbInstHdr : public _dbObject
{
 public:
  _dbInstHdr(_dbDatabase* db);
  _dbInstHdr(_dbDatabase* db, const _dbInstHdr& i);

  bool operator==(const _dbInstHdr& rhs) const;
  bool operator!=(const _dbInstHdr& rhs) const { return !operator==(rhs); }
  void collectMemInfo(MemInfo& info);

  int mterm_cnt_;
  uint32_t id_;
  dbId<_dbInstHdr> next_entry_;
  dbId<_dbLib> lib_;
  dbId<_dbMaster> master_;
  dbVector<dbId<_dbMTerm>> mterms_;
  int inst_cnt_;  // number of instances of this InstHdr
};

dbOStream& operator<<(dbOStream& stream, const _dbInstHdr& inst_hdr);
dbIStream& operator>>(dbIStream& stream, _dbInstHdr& inst_hdr);

}  // namespace odb
