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

struct _dbTechViaRuleFlags
{
  uint _spare_bits : 32;
};

class _dbTechViaRule : public _dbObject
{
 public:
  // PERSISTANT-MEMBERS
  _dbTechViaRuleFlags _flags;
  char* _name;
  dbVector<uint> _layer_rules;
  dbVector<uint> _vias;

  _dbTechViaRule(_dbDatabase*, const _dbTechViaRule& v);
  _dbTechViaRule(_dbDatabase*);
  ~_dbTechViaRule();

  bool operator==(const _dbTechViaRule& rhs) const;
  bool operator!=(const _dbTechViaRule& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbTechViaRule& rhs) const
  {
    return strcmp(_name, rhs._name) < 0;
  }

  void collectMemInfo(MemInfo& info);
};

dbOStream& operator<<(dbOStream& stream, const _dbTechViaRule& v);
dbIStream& operator>>(dbIStream& stream, _dbTechViaRule& v);

}  // namespace odb
