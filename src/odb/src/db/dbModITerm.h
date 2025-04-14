// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include "dbCore.h"
#include "odb/odb.h"

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

  char* _name;
  dbId<_dbModInst> _parent;
  dbId<_dbModBTerm> _child_modbterm;
  dbId<_dbModNet> _mod_net;
  dbId<_dbModITerm> _next_net_moditerm;
  dbId<_dbModITerm> _prev_net_moditerm;
  dbId<_dbModITerm> _next_entry;
  dbId<_dbModITerm> _prev_entry;
};
dbIStream& operator>>(dbIStream& stream, _dbModITerm& obj);
dbOStream& operator<<(dbOStream& stream, const _dbModITerm& obj);
}  // namespace odb
   // Generator Code End Header
