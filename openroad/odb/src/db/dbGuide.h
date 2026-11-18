// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

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

class _dbGuide : public _dbObject
{
 public:
  _dbGuide(_dbDatabase*);

  bool operator==(const _dbGuide& rhs) const;
  bool operator!=(const _dbGuide& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbGuide& rhs) const;
  void collectMemInfo(MemInfo& info);

  dbId<_dbNet> net_;
  Rect box_;
  dbId<_dbTechLayer> layer_;
  dbId<_dbTechLayer> via_layer_;
  dbId<_dbGuide> guide_next_;
  bool is_congested_;
  bool is_jumper_;
  bool is_connect_to_term_;
};
dbIStream& operator>>(dbIStream& stream, _dbGuide& obj);
dbOStream& operator<<(dbOStream& stream, const _dbGuide& obj);
}  // namespace odb
// Generator Code End Header
