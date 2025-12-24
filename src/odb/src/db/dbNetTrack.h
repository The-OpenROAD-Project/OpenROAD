// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <cstdint>

#include "dbCore.h"
#include "odb/dbId.h"
// User Code Begin Includes
#include "odb/geom.h"
// User Code End Includes

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;
class _dbNet;
class _dbTechLayer;

class _dbNetTrack : public _dbObject
{
 public:
  _dbNetTrack(_dbDatabase*);

  bool operator==(const _dbNetTrack& rhs) const;
  bool operator!=(const _dbNetTrack& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbNetTrack& rhs) const;
  void collectMemInfo(MemInfo& info);

  dbId<_dbNet> net_;
  Rect box_;
  dbId<_dbTechLayer> layer_;
  dbId<_dbNetTrack> track_next_;
};
dbIStream& operator>>(dbIStream& stream, _dbNetTrack& obj);
dbOStream& operator<<(dbOStream& stream, const _dbNetTrack& obj);
}  // namespace odb
   // Generator Code End Header
