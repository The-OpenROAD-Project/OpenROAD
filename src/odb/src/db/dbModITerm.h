// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <cstdint>

#include "dbCore.h"
#include "odb/dbId.h"

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;
class _dbModInst;
class _dbModBTerm;
class _dbModNet;

class _dbModITerm : public _dbObject
{
 public:
  _dbModITerm(_dbDatabase*);

  ~_dbModITerm();

  bool operator==(const _dbModITerm& rhs) const;
  bool operator!=(const _dbModITerm& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbModITerm& rhs) const;
  void collectMemInfo(MemInfo& info);

  char* name_;
  dbId<_dbModInst> parent_;
  dbId<_dbModBTerm> child_modbterm_;
  dbId<_dbModNet> mod_net_;
  dbId<_dbModITerm> next_net_moditerm_;
  dbId<_dbModITerm> prev_net_moditerm_;
  dbId<_dbModITerm> next_entry_;
  dbId<_dbModITerm> prev_entry_;
};
dbIStream& operator>>(dbIStream& stream, _dbModITerm& obj);
dbOStream& operator<<(dbOStream& stream, const _dbModITerm& obj);
}  // namespace odb
   // Generator Code End Header
