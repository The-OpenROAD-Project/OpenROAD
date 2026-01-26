// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "frBaseTypes.h"
#include "odb/geom.h"

namespace drt {

class Point3D : public odb::Point
{
 public:
  Point3D() = default;
  Point3D(int x, int y, int z) : odb::Point(x, y), z_(z) {}
  Point3D& operator=(const Point3D&) = default;
  Point3D(const Point3D& p) : odb::Point(p.getX(), p.getY()), z_(p.getZ()) {}
  Point3D(const odb::Point& p, int z) : odb::Point(p), z_(z) {}

  int z() const { return getZ(); }
  int getZ() const { return z_; }
  void setZ(int z) { z_ = z; }
  void set(const int x, const int y, const int z)
  {
    setX(x);
    setY(y);
    z_ = z;
  }
  bool operator==(const Point3D& pIn) const
  {
    return (x() == pIn.x()) && (y() == pIn.y()) && z_ == pIn.z_;
  }

  bool operator!=(const Point3D& pIn) const { return !(*this == pIn); }
  bool operator<(const Point3D& rhs) const
  {
    const odb::Point* pt2d = static_cast<const Point*>(this);
    if (*pt2d != rhs) {
      return *pt2d < rhs;
    }
    return z_ < rhs.z_;
  }

 private:
  int z_{0};
  template <class Archive>
  void serialize(Archive& ar, const unsigned int version)
  {
    (ar) & boost::serialization::base_object<odb::Point>(*this);
    (ar) & z_;
  }

  friend class boost::serialization::access;
};
}  // namespace drt
