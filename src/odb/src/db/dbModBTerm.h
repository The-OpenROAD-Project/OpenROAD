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
class _dbModITerm;
class _dbModule;
class _dbModNet;
class _dbBusPort;

class _dbModBTerm : public _dbObject
{
 public:
  _dbModBTerm(_dbDatabase*);

  ~_dbModBTerm();

  bool operator==(const _dbModBTerm& rhs) const;
  bool operator!=(const _dbModBTerm& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbModBTerm& rhs) const;
  void collectMemInfo(MemInfo& info);

  char* _name;
  uint _flags;
  dbId<_dbModITerm> _parent_moditerm;
  dbId<_dbModule> _parent;
  dbId<_dbModNet> _modnet;
  dbId<_dbModBTerm> _next_net_modbterm;
  dbId<_dbModBTerm> _prev_net_modbterm;
  dbId<_dbBusPort> _busPort;
  dbId<_dbModBTerm> _next_entry;
  dbId<_dbModBTerm> _prev_entry;

  // User Code Begin Fields
  void* _sta_port = nullptr;
  // User Code End Fields
};
dbIStream& operator>>(dbIStream& stream, _dbModBTerm& obj);
dbOStream& operator<<(dbOStream& stream, const _dbModBTerm& obj);
}  // namespace odb
   // Generator Code End Header
