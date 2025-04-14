// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include "dbCore.h"
#include "odb/odb.h"

// User Code Begin Includes
#include "dbVector.h"
// User Code End Includes

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;
class _dbInst;
class _dbModInst;
class _dbNet;
class _dbRegion;

struct dbGroupFlags
{
  uint _type : 2;
  uint spare_bits_ : 30;
};

class _dbGroup : public _dbObject
{
 public:
  _dbGroup(_dbDatabase*);

  ~_dbGroup();

  bool operator==(const _dbGroup& rhs) const;
  bool operator!=(const _dbGroup& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbGroup& rhs) const;
  void collectMemInfo(MemInfo& info);

  dbGroupFlags flags_;
  char* _name;
  dbId<_dbGroup> _next_entry;
  dbId<_dbGroup> _group_next;
  dbId<_dbGroup> _parent_group;
  dbId<_dbInst> _insts;
  dbId<_dbModInst> _modinsts;
  dbId<_dbGroup> _groups;
  dbVector<dbId<_dbNet>> _power_nets;
  dbVector<dbId<_dbNet>> _ground_nets;
  dbId<_dbGroup> region_next_;
  dbId<_dbGroup> region_prev_;
  dbId<_dbRegion> region_;
};
dbIStream& operator>>(dbIStream& stream, _dbGroup& obj);
dbOStream& operator<<(dbOStream& stream, const _dbGroup& obj);
}  // namespace odb
   // Generator Code End Header
