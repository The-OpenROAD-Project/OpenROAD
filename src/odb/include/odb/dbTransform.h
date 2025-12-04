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
  friend class _dbBlock;
  dbOrientType::Value _orient = dbOrientType::R0;
  Point3D _offset;
  bool mirror_z_ = false;

 public:
  // T = <R0, (0,0,0), false>
  dbTransform() = default;

  //  T = <R0, (offset,0), false>
  dbTransform(const Point offset) : _offset(offset, 0) {}

  //  T = <R0, offset, false>
  dbTransform(const Point3D& offset) : _offset(offset) {}

  //  T = <orient, (0,0,0), false>
  dbTransform(const dbOrientType orient) : _orient(orient) {}

  //  T = <orient, (0,0,0), orient.mirror_z_>
  dbTransform(const dbOrientType3D& orient)
      : _orient(orient.getOrientType2D()), mirror_z_(orient.isMirrorZ())
  {
  }

  //  T = <orient, (offset,0), false>
  dbTransform(const dbOrientType orient, const Point& offset)
      : _orient(orient), _offset(offset, 0)
  {
  }

  dbTransform(const dbOrientType3D orient, const Point3D& offset)
      : _orient(orient.getOrientType2D()),
        _offset(offset),
        mirror_z_(orient.isMirrorZ())
  {
  }

  bool operator==(const dbTransform& t) const
  {
    return (_orient == t._orient) && (_offset == t._offset)
           && (mirror_z_ == t.mirror_z_);
  }

  bool operator!=(const dbTransform& t) const { return !operator==(t); }

  void setOrient(const dbOrientType orient) { _orient = orient; }

  void setOrient(const dbOrientType3D& orient)
  {
    _orient = orient.getOrientType2D();
    mirror_z_ = orient.isMirrorZ();
  }

  void setOffset(const Point offset) { _offset = Point3D(offset, 0); }

  void setOffset(const Point3D& offset) { _offset = offset; }

  void setTransform(const dbOrientType orient, const Point& offset)
  {
    _orient = orient;
    _offset = Point3D(offset, 0);
  }

  // Apply transform to this point
  void apply(Point& p) const;

  // Apply transform to this point3D
  void apply(Point3D& p) const;

  // Apply transform to this Rect
  void apply(Rect& r) const;

  // Apply transform to this Cuboid
  void apply(Cuboid& c) const;

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
  Point getOffset() const { return Point(_offset.x(), _offset.y()); }

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
