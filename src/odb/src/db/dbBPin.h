// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "dbCore.h"
#include "dbVector.h"
#include "odb/dbId.h"
#include "odb/dbTypes.h"
#include "odb/odb.h"

namespace odb {

class _dbBTerm;
class _dbBox;
class _dbDatabase;
class _dbAccessPoint;
class dbIStream;
class dbOStream;

struct _dbBPinFlags
{
  dbPlacementStatus::Value _status : 4;
  uint _has_min_spacing : 1;
  uint _has_effective_width : 1;
  uint _spare_bits : 26;
};

class _dbBPin : public _dbObject
{
 public:
  // PERSISTANT-MEMBERS
  _dbBPinFlags _flags;
  dbId<_dbBTerm> _bterm;
  dbId<_dbBox> _boxes;
  dbId<_dbBPin> _next_bpin;
  uint _min_spacing;      // 5.6 DEF
  uint _effective_width;  // 5.6 DEF
  dbVector<dbId<_dbAccessPoint>> aps_;

  _dbBPin(_dbDatabase*, const _dbBPin& p);
  _dbBPin(_dbDatabase*);

  bool operator==(const _dbBPin& rhs) const;
  bool operator!=(const _dbBPin& rhs) const { return !operator==(rhs); }
  void collectMemInfo(MemInfo& info);
};

dbIStream& operator>>(dbIStream& stream, _dbBPin& bpin);
dbOStream& operator<<(dbOStream& stream, const _dbBPin& bpin);

}  // namespace odb
