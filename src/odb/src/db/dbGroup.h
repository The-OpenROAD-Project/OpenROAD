// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include "dbCore.h"
#include "dbVector.h"
#include "odb/dbId.h"
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
  uint type : 2;
  uint spare_bits : 30;
};

class _dbGroup : public _dbObject
{
 public:
  _dbGroup(_dbDatabase*);

  bool operator==(const _dbGroup& rhs) const;
  bool operator!=(const _dbGroup& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbGroup& rhs) const;
  void collectMemInfo(MemInfo& info);

  dbGroupFlags flags_;
  char* name_;
  dbId<_dbGroup> next_entry_;
  dbId<_dbGroup> group_next_;
  dbId<_dbGroup> parent_group_;
  dbId<_dbInst> insts_;
  dbId<_dbModInst> modinsts_;
  dbId<_dbGroup> groups_;
  dbVector<dbId<_dbNet>> power_nets_;
  dbVector<dbId<_dbNet>> ground_nets_;
  dbId<_dbGroup> region_next_;
  dbId<_dbGroup> region_prev_;
  dbId<_dbRegion> region_;
};
dbIStream& operator>>(dbIStream& stream, _dbGroup& obj);
dbOStream& operator<<(dbOStream& stream, const _dbGroup& obj);
}  // namespace odb
   // Generator Code End Header
