///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2024, The Regents of the University of California
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

// Generator Code Begin Header
#pragma once

#include "dbCore.h"
#include "dbVector.h"
#include "odb/odb.h"
// User Code Begin Includes
#include <variant>

#include "odb/db.h"
// User Code End Includes

namespace odb {
class dbIStream;
class dbOStream;
class dbDiff;
class _dbDatabase;
class _dbMarkerCategory;
class _dbTechLayer;
// User Code Begin Classes
class Line;
class Rect;
class Polygon;
// User Code End Classes

struct dbMarkerFlags
{
  bool visited_ : 1;
  bool visible_ : 1;
  uint spare_bits_ : 30;
};

class _dbMarker : public _dbObject
{
 public:
  // User Code Begin Enums
  // Order of these enum must be preserved
  enum class ShapeType
  {
    Point,
    Line,
    Rect,
    Polygon
  };
  // User Code End Enums

  _dbMarker(_dbDatabase*, const _dbMarker& r);
  _dbMarker(_dbDatabase*);

  ~_dbMarker() = default;

  bool operator==(const _dbMarker& rhs) const;
  bool operator!=(const _dbMarker& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbMarker& rhs) const;
  void differences(dbDiff& diff, const char* field, const _dbMarker& rhs) const;
  void out(dbDiff& diff, char side, const char* field) const;
  // User Code Begin Methods
  _dbBlock* getBlock() const;
  // User Code End Methods

  dbMarkerFlags flags_;
  dbId<_dbMarkerCategory> parent_;
  dbId<_dbTechLayer> layer_;
  std::string comment_;
  int line_number_;

  // User Code Begin Fields
  std::vector<std::pair<dbObjectType, uint>> sources_;
  std::vector<dbMarker::MarkerShape> shapes_;
  // User Code End Fields
};
dbIStream& operator>>(dbIStream& stream, _dbMarker& obj);
dbOStream& operator<<(dbOStream& stream, const _dbMarker& obj);
}  // namespace odb
// Generator Code End Header