// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <cstdint>

#include "dbCore.h"
#include "dbVector.h"
#include "odb/dbId.h"

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

  char* name_;
  dbId<_dbModule> parent_;
  dbId<_dbModNet> next_entry_;
  dbId<_dbModNet> prev_entry_;
  dbId<_dbModITerm> moditerms_;
  dbId<_dbModBTerm> modbterms_;
  dbId<_dbITerm> iterms_;
  dbId<_dbBTerm> bterms_;

  // User Code Begin Fields
  enum Field  // dbJournalField name
  {
    kName
  };
  // User Code End Fields
};
dbIStream& operator>>(dbIStream& stream, _dbModNet& obj);
dbOStream& operator<<(dbOStream& stream, const _dbModNet& obj);
}  // namespace odb
   // Generator Code End Header
