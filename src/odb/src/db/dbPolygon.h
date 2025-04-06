// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include "dbCore.h"
#include "odb/geom.h"
#include "odb/odb.h"

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;
class _dbBox;

struct dbPolygonFlags
{
  uint owner_type_ : 4;
  uint layer_id_ : 9;
  uint spare_bits_ : 19;
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
  static Polygon checkPolygon(std::vector<Point> polygon);
  void decompose();
  // User Code End Methods

  dbPolygonFlags flags_;
  Polygon polygon_;
  int design_rule_width_;
  uint owner_;
  dbId<_dbPolygon> next_pbox_;
  dbId<_dbBox> boxes_;
};
dbIStream& operator>>(dbIStream& stream, _dbPolygon& obj);
dbOStream& operator<<(dbOStream& stream, const _dbPolygon& obj);
}  // namespace odb
   // Generator Code End Header
