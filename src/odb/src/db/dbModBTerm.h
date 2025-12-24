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

  char* name_;
  uint32_t flags_;
  dbId<_dbModITerm> parent_moditerm_;
  dbId<_dbModule> parent_;
  dbId<_dbModNet> modnet_;
  dbId<_dbModBTerm> next_net_modbterm_;
  dbId<_dbModBTerm> prev_net_modbterm_;
  dbId<_dbBusPort> busPort_;
  dbId<_dbModBTerm> next_entry_;
  dbId<_dbModBTerm> prev_entry_;

  // User Code Begin Fields
  void* _sta_port = nullptr;
  // User Code End Fields
};
dbIStream& operator>>(dbIStream& stream, _dbModBTerm& obj);
dbOStream& operator<<(dbOStream& stream, const _dbModBTerm& obj);
}  // namespace odb
   // Generator Code End Header
