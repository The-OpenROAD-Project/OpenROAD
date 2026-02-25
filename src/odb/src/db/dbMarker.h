// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <cstdint>
#include <string>

#include "dbCore.h"
#include "dbVector.h"
#include "odb/dbId.h"
// User Code Begin Includes
#include <fstream>
#include <set>
#include <utility>
#include <variant>
#include <vector>

#include "dbMarkerCategory.h"
#include "odb/db.h"
#include "odb/dbObject.h"
// User Code End Includes

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;
class _dbMarkerCategory;
class _dbTechLayer;
// User Code Begin Classes
class Line;
class Rect;
class Polygon;
class _dbChip;
// User Code End Classes

struct dbMarkerFlags
{
  bool visited : 1;
  bool visible : 1;
  bool waived : 1;
  uint32_t spare_bits : 29;
};

class _dbMarker : public _dbObject
{
 public:
  // User Code Begin Enums
  // Order of these enum must be preserved
  enum class ShapeType
  {
    kPoint = 0,
    kLine = 1,
    kRect = 2,
    kPolygon = 3
  };
  // User Code End Enums

  _dbMarker(_dbDatabase*);

  bool operator==(const _dbMarker& rhs) const;
  bool operator!=(const _dbMarker& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbMarker& rhs) const;
  void collectMemInfo(MemInfo& info);
  // User Code Begin Methods
  _dbBlock* getBlock() const;
  _dbChip* getChip() const;

  void populatePTree(_dbMarkerCategory::PropertyTree& tree) const;
  void fromPTree(const _dbMarkerCategory::PropertyTree& tree);
  void writeTR(std::ofstream& report) const;
  // User Code End Methods

  dbMarkerFlags flags_;
  dbId<_dbMarkerCategory> parent_;
  dbId<_dbTechLayer> layer_;
  std::string comment_;
  int line_number_;

  // User Code Begin Fields
  std::set<std::pair<dbObjectType, uint32_t>> sources_;
  std::vector<dbMarker::MarkerShape> shapes_;
  // User Code End Fields
};
dbIStream& operator>>(dbIStream& stream, _dbMarker& obj);
dbOStream& operator<<(dbOStream& stream, const _dbMarker& obj);
// User Code Begin General
dbIStream& operator>>(dbIStream& stream, _dbMarker::ShapeType& obj);
dbOStream& operator<<(dbOStream& stream, const _dbMarker::ShapeType& obj);
// User Code End General
}  // namespace odb
   // Generator Code End Header
