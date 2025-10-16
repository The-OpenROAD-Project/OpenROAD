// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "dbCore.h"
#include "dbVector.h"
#include "odb/dbId.h"
#include "odb/odb.h"

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
  dbId<_dbInst> _inst;
  dbId<_dbBlock> _child_block;
  dbVector<dbId<_dbBTerm>> _child_bterms;

  _dbHier(_dbDatabase* db);
  _dbHier(_dbDatabase* db, const _dbHier& h);
  ~_dbHier();
  bool operator==(const _dbHier& rhs) const;
  bool operator!=(const _dbHier& rhs) const { return !operator==(rhs); }
  void collectMemInfo(MemInfo& info);

  static _dbHier* create(dbInst* inst, dbBlock* child);
  static void destroy(_dbHier* hdr);
};

dbOStream& operator<<(dbOStream& stream, const _dbHier& inst_hdr);
dbIStream& operator>>(dbIStream& stream, _dbHier& inst_hdr);

}  // namespace odb
