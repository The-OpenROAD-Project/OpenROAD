// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdint>

#include "dbCore.h"
#include "dbVector.h"
#include "odb/dbId.h"
#include "odb/dbTypes.h"

namespace odb {

class _dbBTerm;
class _dbBox;
class _dbDatabase;
class _dbAccessPoint;
class dbIStream;
class dbOStream;

struct _dbBPinFlags
{
  dbPlacementStatus::Value status : 4;
  uint32_t has_min_spacing : 1;
  uint32_t has_effective_width : 1;
  uint32_t spare_bits : 26;
};

class _dbBPin : public _dbObject
{
 public:
  _dbBPin(_dbDatabase*, const _dbBPin& p);
  _dbBPin(_dbDatabase*);

  bool operator==(const _dbBPin& rhs) const;
  bool operator!=(const _dbBPin& rhs) const { return !operator==(rhs); }
  void collectMemInfo(MemInfo& info);
  void removeBox(_dbBox* box);

  // PERSISTANT-MEMBERS
  _dbBPinFlags flags_;
  dbId<_dbBTerm> bterm_;
  dbId<_dbBox> boxes_;
  dbId<_dbBPin> next_bpin_;
  uint32_t min_spacing_;      // 5.6 DEF
  uint32_t effective_width_;  // 5.6 DEF
  dbVector<dbId<_dbAccessPoint>> aps_;
};

dbIStream& operator>>(dbIStream& stream, _dbBPin& bpin);
dbOStream& operator<<(dbOStream& stream, const _dbBPin& bpin);

}  // namespace odb
