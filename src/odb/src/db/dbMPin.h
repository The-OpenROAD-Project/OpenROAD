// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "dbCore.h"
#include "dbVector.h"
#include "odb/dbId.h"
#include "odb/odb.h"

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
  // PERSISTANT-MEMBERS
  dbId<_dbMTerm> _mterm;
  dbId<_dbBox> _geoms;
  dbId<_dbPolygon> _poly_geoms;
  dbId<_dbMPin> _next_mpin;
  dbVector<dbVector<dbId<_dbAccessPoint>>>
      aps_;  // A vector of access points for each unique instance(master,
             // orient, origin relevant to track). The outer index is the
             // pin-access/unique-instance idx.

  _dbMPin(_dbDatabase*, const _dbMPin& p);
  _dbMPin(_dbDatabase*);
  ~_dbMPin();

  bool operator==(const _dbMPin& rhs) const;
  bool operator!=(const _dbMPin& rhs) const { return !operator==(rhs); }
  void collectMemInfo(MemInfo& info);
  void addAccessPoint(uint idx, _dbAccessPoint* ap);
};

dbOStream& operator<<(dbOStream& stream, const _dbMPin& mpin);
dbIStream& operator>>(dbIStream& stream, _dbMPin& mpin);

}  // namespace odb
