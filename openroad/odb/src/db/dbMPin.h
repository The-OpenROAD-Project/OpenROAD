// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdint>

#include "dbCore.h"
#include "dbVector.h"
#include "odb/dbId.h"

namespace odb {

class _dbMTerm;
class _dbBox;
class _dbPolygon;
class _dbDatabase;
class dbIStream;
class dbOStream;
class _dbAccessPoint;

class _dbMPin : public _dbObject
{
 public:
  _dbMPin(_dbDatabase*, const _dbMPin& p);
  _dbMPin(_dbDatabase*);

  bool operator==(const _dbMPin& rhs) const;
  bool operator!=(const _dbMPin& rhs) const { return !operator==(rhs); }
  void collectMemInfo(MemInfo& info);
  void addAccessPoint(uint32_t idx, _dbAccessPoint* ap);

  // PERSISTANT-MEMBERS
  dbId<_dbMTerm> mterm_;
  dbId<_dbBox> geoms_;
  dbId<_dbPolygon> poly_geoms_;
  dbId<_dbMPin> next_mpin_;
  // A vector of access points for each unique instance(master, orient, origin
  // relevant to track). The outer index is the pin-access/unique-instance idx.
  dbVector<dbVector<dbId<_dbAccessPoint>>> aps_;
};

dbOStream& operator<<(dbOStream& stream, const _dbMPin& mpin);
dbIStream& operator>>(dbIStream& stream, _dbMPin& mpin);

}  // namespace odb
