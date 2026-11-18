// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <array>
#include <cstdint>
#include <tuple>

#include "dbCore.h"
#include "dbVector.h"
#include "odb/dbId.h"
#include "odb/dbObject.h"
// User Code Begin Includes
#include <utility>

#include "odb/dbTypes.h"
#include "odb/geom.h"
// User Code End Includes

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;
class _dbTechLayer;
class _dbLib;
class _dbMaster;
class _dbMPin;
class _dbBPin;
class _dbITerm;
class _dbObject;

class _dbAccessPoint : public _dbObject
{
 public:
  _dbAccessPoint(_dbDatabase*);

  bool operator==(const _dbAccessPoint& rhs) const;
  bool operator!=(const _dbAccessPoint& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbAccessPoint& rhs) const;
  void collectMemInfo(MemInfo& info);
  // User Code Begin Methods
  void setMPin(_dbMPin* mpin);
  // User Code End Methods

  Point point_;
  dbId<_dbTechLayer> layer_;
  dbId<_dbLib> lib_;
  dbId<_dbMaster> master_;
  dbId<_dbMPin> mpin_;
  dbId<_dbBPin> bpin_;
  std::array<bool, 6> accesses_;
  dbAccessType::Value low_type_;
  dbAccessType::Value high_type_;
  // list of iterms that prefer this access point
  dbVector<dbId<_dbITerm>> iterms_;
  // list of vias by num of cuts
  dbVector<dbVector<std::pair<dbObjectType, dbId<_dbObject>>>> vias_;
  dbVector<std::tuple<Rect, bool, bool>> path_segs_;
};
dbIStream& operator>>(dbIStream& stream, _dbAccessPoint& obj);
dbOStream& operator<<(dbOStream& stream, const _dbAccessPoint& obj);
}  // namespace odb
   // Generator Code End Header
