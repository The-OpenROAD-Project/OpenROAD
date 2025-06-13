// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <string>

#include "dbCore.h"
#include "dbVector.h"
#include "odb/odb.h"

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;
class _dbGroup;

class _dbSupplyNet : public _dbObject
{
 public:
  _dbSupplyNet(_dbDatabase*);

  ~_dbSupplyNet();

  bool operator==(const _dbSupplyNet& rhs) const;
  bool operator!=(const _dbSupplyNet& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbSupplyNet& rhs) const;
  void collectMemInfo(MemInfo& info);

  char* _name;
  dbId<_dbSupplyNet> _next_entry;
  std::string _direction;
  dbId<_dbGroup> _group;
  dbId<_dbSupplyNet> _parent;
  dbId<_dbSupplyNet> _in;
  dbId<_dbSupplyNet> _out;
};
dbIStream& operator>>(dbIStream& stream, _dbSupplyNet& obj);
dbOStream& operator<<(dbOStream& stream, const _dbSupplyNet& obj);
}  // namespace odb
   // Generator Code End Header