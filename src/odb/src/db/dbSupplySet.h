// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include "dbCore.h"
#include "dbVector.h"
#include "odb/odb.h"

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;
class _dbSupplyNet;
class _dbGroup;

class _dbSupplySet : public _dbObject
{
 public:
  _dbSupplySet(_dbDatabase*);

  ~_dbSupplySet();

  bool operator==(const _dbSupplySet& rhs) const;
  bool operator!=(const _dbSupplySet& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbSupplySet& rhs) const;
  void collectMemInfo(MemInfo& info);

  char* _name;
  dbId<_dbSupplySet> _next_entry;
  dbId<_dbSupplyNet> _power_net;
  dbId<_dbSupplyNet> _ground_net;
  dbId<_dbSupplyNet> _nwell_net;
  dbId<_dbSupplyNet> _pwell_net;
  dbId<_dbGroup> _group;
};
dbIStream& operator>>(dbIStream& stream, _dbSupplySet& obj);
dbOStream& operator<<(dbOStream& stream, const _dbSupplySet& obj);
}  // namespace odb
   // Generator Code End Header