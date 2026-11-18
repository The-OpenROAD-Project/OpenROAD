// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "odb/dbTypes.h"
#include "odb/geom.h"

namespace odb {

class dbOStream;
class dbIStream;

//
// Database Transform - Transform points by rotation and translation.
//
class dbTransform
{
 public:
  // T = <R0, (0,0,0), false>
  dbTransform() = default;

  //  T = <R0, (offset,0), false>
  dbTransform(const Point offset) : offset_(offset, 0) {}

  //  T = <R0, offset, false>
  dbTransform(const Point3D& offset) : offset_(offset) {}

  //  T = <orient, (0,0,0), false>
  dbTransform(const dbOrientType orient) : orient_(orient) {}

  //  T = <orient, (0,0,0), orient.mirror_z_>
  dbTransform(const dbOrientType3D& orient)
      : orient_(orient.getOrientType2D()), mirror_z_(orient.isMirrorZ())
  {
  }

  //  T = <orient, (offset,0), false>
  dbTransform(const dbOrientType orient, const Point& offset)
      : orient_(orient), offset_(offset, 0)
  {
  }

  dbTransform(const dbOrientType3D orient, const Point3D& offset)
      : orient_(orient.getOrientType2D()),
        offset_(offset),
        mirror_z_(orient.isMirrorZ())
  {
  }

  bool operator==(const dbTransform& t) const
  {
    return (orient_ == t.orient_) && (offset_ == t.offset_)
           && (mirror_z_ == t.mirror_z_);
  }

  bool operator!=(const dbTransform& t) const { return !operator==(t); }

  void setOrient(const dbOrientType orient) { orient_ = orient; }

  void setOrient(const dbOrientType3D& orient)
  {
    orient_ = orient.getOrientType2D();
    mirror_z_ = orient.isMirrorZ();
  }

  void setOffset(const Point offset) { offset_ = Point3D(offset, 0); }

  void setOffset(const Point3D& offset) { offset_ = offset; }

  void setTransform(const dbOrientType orient, const Point& offset)
  {
    orient_ = orient;
    offset_ = Point3D(offset, 0);
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

  dbOrientType getOrient() const { return orient_; }
  Point getOffset() const { return Point(offset_.x(), offset_.y()); }
  Point3D getOffset3D() const { return offset_; }
  bool isMirrorZ() const { return mirror_z_; }

  friend dbOStream& operator<<(dbOStream& stream, const dbTransform& t);
  friend dbIStream& operator>>(dbIStream& stream, dbTransform& t);

 private:
  friend class _dbBlock;
  dbOrientType::Value orient_ = dbOrientType::R0;
  Point3D offset_;
  bool mirror_z_ = false;
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
