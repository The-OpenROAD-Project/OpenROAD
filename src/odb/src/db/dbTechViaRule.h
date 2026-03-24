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

struct _dbTechViaRuleFlags
{
  uint32_t spare_bits : 32;
};

class _dbTechViaRule : public _dbObject
{
 public:
  _dbTechViaRule(_dbDatabase*, const _dbTechViaRule& v);
  _dbTechViaRule(_dbDatabase*);
  ~_dbTechViaRule();

  bool operator==(const _dbTechViaRule& rhs) const;
  bool operator!=(const _dbTechViaRule& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbTechViaRule& rhs) const
  {
    return strcmp(name_, rhs.name_) < 0;
  }

  void collectMemInfo(MemInfo& info);

  // PERSISTANT-MEMBERS
  _dbTechViaRuleFlags flags_;
  char* name_;
  dbVector<uint32_t> layer_rules_;
  dbVector<uint32_t> vias_;
};

dbOStream& operator<<(dbOStream& stream, const _dbTechViaRule& v);
dbIStream& operator>>(dbIStream& stream, _dbTechViaRule& v);

}  // namespace odb
