// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <cstring>

#include "dbCore.h"
#include "dbVector.h"
#include "odb/dbId.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"

namespace odb {

class _dbTechLayer;
class _dbBox;
class _dbDatabase;
class dbIStream;
class dbOStream;

//
// These flags keep track of the variations between difference LEF versions
//
struct _dbTechViaGenerateRuleFlags
{
  uint32_t default_via : 1;
  uint32_t spare_bits : 31;
};

class _dbTechViaGenerateRule : public _dbObject
{
 public:
  _dbTechViaGenerateRule(_dbDatabase*, const _dbTechViaGenerateRule& v);
  _dbTechViaGenerateRule(_dbDatabase*);
  ~_dbTechViaGenerateRule();

  bool operator==(const _dbTechViaGenerateRule& rhs) const;
  bool operator!=(const _dbTechViaGenerateRule& rhs) const
  {
    return !operator==(rhs);
  }
  bool operator<(const _dbTechViaGenerateRule& rhs) const
  {
    return strcmp(name_, rhs.name_) < 0;
  }

  void collectMemInfo(MemInfo& info);

  // PERSISTANT-MEMBERS
  _dbTechViaGenerateRuleFlags flags_;
  char* name_;
  dbVector<uint32_t> layer_rules_;
};

dbOStream& operator<<(dbOStream& stream, const _dbTechViaGenerateRule& v);
dbIStream& operator>>(dbIStream& stream, _dbTechViaGenerateRule& v);

}  // namespace odb
