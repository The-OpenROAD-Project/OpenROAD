// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "dbTypes.h"
#include "geom.h"
#include "odb.h"

namespace odb {

class dbOStream;
class dbIStream;

//
// Database Transform - Transform points by rotation and translation.
//
class dbTransform
{
  friend class _dbBlock;
  dbOrientType::Value _orient = dbOrientType::R0;
  Point _offset;

 public:
  // T = <R0, (0,0)>
  dbTransform() = default;

  //  T = <R0, offset>
  dbTransform(const Point offset) : _offset(offset) {}

  //  T = <orient, (0,0)>
  dbTransform(const dbOrientType orient) : _orient(orient) {}

  //  T = <orient, offset>
  dbTransform(const dbOrientType orient, const Point& offset)
      : _orient(orient), _offset(offset)
  {
  }

  bool operator==(const dbTransform& t) const
  {
    return (_orient == t._orient) && (_offset == t._offset);
  }

  bool operator!=(const dbTransform& t) const { return !operator==(t); }

  void setOrient(const dbOrientType orient) { _orient = orient; }

  void setOffset(const Point offset) { _offset = offset; }

  void setTransform(const dbOrientType orient, const Point& offset)
  {
    _orient = orient;
    _offset = offset;
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

  dbOrientType getOrient() const { return _orient; }
  Point getOffset() const { return _offset; }

  friend dbOStream& operator<<(dbOStream& stream, const dbTransform& t);
  friend dbIStream& operator>>(dbIStream& stream, dbTransform& t);
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
