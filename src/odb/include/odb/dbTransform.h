// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "odb/odb.h"

namespace odb {

class dbOStream;
class dbIStream;

//
// Database Transform - Transform points by rotation and translation.
//
class dbTransform
{
 public:
  // T = <R0, (0,0)>
  dbTransform() = default;

  //  T = <R0, offset>
  dbTransform(const Point offset) : offset_(offset) {}

  //  T = <orient, (0,0)>
  dbTransform(const dbOrientType orient) : orient_(orient) {}

  //  T = <orient, offset>
  dbTransform(const dbOrientType orient, const Point& offset)
      : orient_(orient), offset_(offset)
  {
  }

  bool operator==(const dbTransform& t) const
  {
    return (orient_ == t.orient_) && (offset_ == t.offset_);
  }

  bool operator!=(const dbTransform& t) const { return !operator==(t); }

  void setOrient(const dbOrientType orient) { orient_ = orient; }

  void setOffset(const Point offset) { offset_ = offset; }

  void setTransform(const dbOrientType orient, const Point& offset)
  {
    orient_ = orient;
    offset_ = offset;
  }

  // Apply transform to this point
  void apply(Point& p) const;

  // Apply transform to this Rect
  void apply(Rect& r) const;

  // Apply transform to this polygon
  void apply(Polygon& p) const;

  // Post multiply transform.
  void concat(const dbTransform& t);

  // Post multiply transform
  void concat(const dbTransform& t, dbTransform& result);

  // Compute inverse transform
  void invert(dbTransform& result) const;

  // Compute inverse transform
  void invert();

  dbOrientType getOrient() const { return orient_; }
  Point getOffset() const { return offset_; }

  friend dbOStream& operator<<(dbOStream& stream, const dbTransform& t);
  friend dbIStream& operator>>(dbIStream& stream, dbTransform& t);

 private:
  friend class _dbBlock;

  dbOrientType::Value orient_ = dbOrientType::R0;
  Point offset_;
};

dbOStream& operator<<(dbOStream& stream, const dbTransform& t);
dbIStream& operator>>(dbIStream& stream, dbTransform& t);

inline void dbTransform::concat(const dbTransform& t)
{
  dbTransform result;
  concat(t, result);
  *this = result;
}

inline void dbTransform::invert()
{
  dbTransform result;
  invert(result);
  *this = result;
}

}  // namespace odb
