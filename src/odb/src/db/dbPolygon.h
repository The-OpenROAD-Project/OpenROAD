// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <cstdint>

#include "dbCore.h"
#include "odb/dbId.h"
#include "odb/geom.h"
// User Code Begin Includes
#include <vector>
// User Code End Includes

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;
class _dbBox;

struct dbPolygonFlags
{
  uint32_t owner_type : 4;
  uint32_t layer_id : 9;
  uint32_t spare_bits : 19;
};

class _dbPolygon : public _dbObject
{
 public:
  _dbPolygon(_dbDatabase*);

  bool operator==(const _dbPolygon& rhs) const;
  bool operator!=(const _dbPolygon& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbPolygon& rhs) const;
  void collectMemInfo(MemInfo& info);
  // User Code Begin Methods
  static Polygon checkPolygon(const std::vector<Point>& polygon);
  void decompose();
  // User Code End Methods

  dbPolygonFlags flags_;
  Polygon polygon_;
  int design_rule_width_;
  uint32_t owner_;
  dbId<_dbPolygon> next_pbox_;
  dbId<_dbBox> boxes_;
};
dbIStream& operator>>(dbIStream& stream, _dbPolygon& obj);
dbOStream& operator<<(dbOStream& stream, const _dbPolygon& obj);
}  // namespace odb
   // Generator Code End Header
