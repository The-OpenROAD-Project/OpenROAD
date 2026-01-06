// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "dbCore.h"
#include "dbVector.h"
#include "odb/dbId.h"

namespace odb {

class _dbInst;
class _dbBlock;
class _dbBTerm;
class _dbDatabase;
class dbBlock;
class dbInst;
class dbIStream;
class dbOStream;

//
// Hidden proxy object hierarchy connectivity.
//
class _dbHier : public _dbObject
{
 public:
  _dbHier(_dbDatabase* db);
  _dbHier(_dbDatabase* db, const _dbHier& h);

  bool operator==(const _dbHier& rhs) const;
  bool operator!=(const _dbHier& rhs) const { return !operator==(rhs); }
  void collectMemInfo(MemInfo& info);

  static _dbHier* create(dbInst* inst, dbBlock* child);
  static void destroy(_dbHier* hdr);

  dbId<_dbInst> inst_;
  dbId<_dbBlock> child_block_;
  dbVector<dbId<_dbBTerm>> child_bterms_;
};

dbOStream& operator<<(dbOStream& stream, const _dbHier& inst_hdr);
dbIStream& operator>>(dbIStream& stream, _dbHier& inst_hdr);

}  // namespace odb
