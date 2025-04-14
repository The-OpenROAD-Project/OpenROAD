// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include "dbCore.h"
#include "dbVector.h"
#include "odb/odb.h"

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;
class _dbModule;
class _dbModITerm;
class _dbModBTerm;
class _dbITerm;
class _dbBTerm;

class _dbModNet : public _dbObject
{
 public:
  _dbModNet(_dbDatabase*);

  ~_dbModNet();

  bool operator==(const _dbModNet& rhs) const;
  bool operator!=(const _dbModNet& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbModNet& rhs) const;
  void collectMemInfo(MemInfo& info);

  char* _name;
  dbId<_dbModule> _parent;
  dbId<_dbModNet> _next_entry;
  dbId<_dbModNet> _prev_entry;
  dbId<_dbModITerm> _moditerms;
  dbId<_dbModBTerm> _modbterms;
  dbId<_dbITerm> _iterms;
  dbId<_dbBTerm> _bterms;
};
dbIStream& operator>>(dbIStream& stream, _dbModNet& obj);
dbOStream& operator<<(dbOStream& stream, const _dbModNet& obj);
}  // namespace odb
   // Generator Code End Header
