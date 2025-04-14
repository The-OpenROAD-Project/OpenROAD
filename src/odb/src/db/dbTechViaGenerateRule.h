// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "dbCore.h"
#include "dbVector.h"
#include "odb/dbId.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "odb/odb.h"

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
  uint _default : 1;
  uint _spare_bits : 31;
};

class _dbTechViaGenerateRule : public _dbObject
{
 public:
  // PERSISTANT-MEMBERS
  _dbTechViaGenerateRuleFlags _flags;
  char* _name;
  dbVector<uint> _layer_rules;

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
    return strcmp(_name, rhs._name) < 0;
  }

  void collectMemInfo(MemInfo& info);
};

dbOStream& operator<<(dbOStream& stream, const _dbTechViaGenerateRule& v);
dbIStream& operator>>(dbIStream& stream, _dbTechViaGenerateRule& v);

}  // namespace odb
