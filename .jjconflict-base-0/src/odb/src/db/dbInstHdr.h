// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "dbCore.h"
#include "dbVector.h"
#include "odb/dbId.h"
#include "odb/odb.h"

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
  int _mterm_cnt;
  uint _id;
  dbId<_dbInstHdr> _next_entry;
  dbId<_dbLib> _lib;
  dbId<_dbMaster> _master;
  dbVector<dbId<_dbMTerm>> _mterms;
  int _inst_cnt;  // number of instances of this InstHdr

  _dbInstHdr(_dbDatabase* db);
  _dbInstHdr(_dbDatabase* db, const _dbInstHdr& i);
  ~_dbInstHdr();
  bool operator==(const _dbInstHdr& rhs) const;
  bool operator!=(const _dbInstHdr& rhs) const { return !operator==(rhs); }
  void collectMemInfo(MemInfo& info);
};

dbOStream& operator<<(dbOStream& stream, const _dbInstHdr& inst_hdr);
dbIStream& operator>>(dbIStream& stream, _dbInstHdr& inst_hdr);

}  // namespace odb
