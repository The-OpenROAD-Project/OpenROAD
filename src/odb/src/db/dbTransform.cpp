// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// OpenDB --- Transformation utilities

#include "odb/dbTransform.h"

#include <stdexcept>
#include <vector>

#include "odb/dbStream.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"

namespace odb {

static const dbOrientType::Value orientMul[8][8] = {{dbOrientType::R0,
                                                     dbOrientType::R90,
                                                     dbOrientType::R180,
                                                     dbOrientType::R270,
                                                     dbOrientType::MY,
                                                     dbOrientType::MYR90,
                                                     dbOrientType::MX,
                                                     dbOrientType::MXR90},
                                                    {dbOrientType::R90,
                                                     dbOrientType::R180,
                                                     dbOrientType::R270,
                                                     dbOrientType::R0,
                                                     dbOrientType::MXR90,
                                                     dbOrientType::MY,
                                                     dbOrientType::MYR90,
                                                     dbOrientType::MX},
                                                    {dbOrientType::R180,
                                                     dbOrientType::R270,
                                                     dbOrientType::R0,
                                                     dbOrientType::R90,
                                                     dbOrientType::MX,
                                                     dbOrientType::MXR90,
                                                     dbOrientType::MY,
                                                     dbOrientType::MYR90},
                                                    {dbOrientType::R270,
                                                     dbOrientType::R0,
                                                     dbOrientType::R90,
                                                     dbOrientType::R180,
                                                     dbOrientType::MYR90,
                                                     dbOrientType::MX,
                                                     dbOrientType::MXR90,
                                                     dbOrientType::MY},
                                                    {dbOrientType::MY,
                                                     dbOrientType::MYR90,
                                                     dbOrientType::MX,
                                                     dbOrientType::MXR90,
                                                     dbOrientType::R0,
                                                     dbOrientType::R90,
                                                     dbOrientType::R180,
                                                     dbOrientType::R270},
                                                    {dbOrientType::MYR90,
                                                     dbOrientType::MX,
                                                     dbOrientType::MXR90,
                                                     dbOrientType::MY,
                                                     dbOrientType::R270,
                                                     dbOrientType::R0,
                                                     dbOrientType::R90,
                                                     dbOrientType::R180},
                                                    {dbOrientType::MX,
                                                     dbOrientType::MXR90,
                                                     dbOrientType::MY,
                                                     dbOrientType::MYR90,
                                                     dbOrientType::R180,
                                                     dbOrientType::R270,
                                                     dbOrientType::R0,
                                                     dbOrientType::R90},
                                                    {dbOrientType::MXR90,
                                                     dbOrientType::MY,
                                                     dbOrientType::MYR90,
                                                     dbOrientType::MX,
                                                     dbOrientType::R90,
                                                     dbOrientType::R180,
                                                     dbOrientType::R270,
                                                     dbOrientType::R0}};

dbOStream& operator<<(dbOStream& stream, const dbTransform& t)
{
  stream << static_cast<int>(t.orient_);
  stream << t.offset_;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, dbTransform& t)
{
  int orient;
  stream >> orient;
  t.orient_ = static_cast<dbOrientType::Value>(orient);
  stream >> t.offset_;
  return stream;
}

//
// The transform matrix is written as:
//
//  T = [ R T ]     or  [r11 r12 x]
//      [ 0 1 ]         [r21 r22 y]
//                      [ 0   0  1]
//
// where R = is the rotation matrix of (R0, R90, R180, R270, MX, MY, MXR90,
// MYR90} where T = is the x,y offset
//
// The inverse is:
//
// Tinv = [ Rinv -(Rinv)(T)]
//        [ 0     1        ]
//
void dbTransform::invert(dbTransform& result) const
{
  Point offset(-offset_.x(), -offset_.y());
  dbOrientType::Value orient;

  switch (orient_) {
    case dbOrientType::R0:
      orient = dbOrientType::R0;
      break;

    case dbOrientType::R90:
      orient = dbOrientType::R270;
      offset.rotate270();
      break;

    case dbOrientType::R180:
      orient = dbOrientType::R180;
      offset.rotate180();
      break;

    case dbOrientType::R270:
      orient = dbOrientType::R90;
      offset.rotate90();
      break;

    case dbOrientType::MY:
      offset.setX(-offset.x());
      orient = dbOrientType::MY;
      break;

    case dbOrientType::MYR90:
      offset.setX(-offset.x());
      offset.rotate90();
      orient = dbOrientType::MYR90;
      break;

    case dbOrientType::MX:
      offset.setY(-offset.y());
      orient = dbOrientType::MX;
      break;

    case dbOrientType::MXR90:
      offset.setY(-offset.y());
      offset.rotate90();
      orient = dbOrientType::MXR90;
      break;
    default:
      throw std::runtime_error("Unknown orientation");
  }

  result.offset_ = Point3D(offset, mirror_z_ ? offset_.z() : -offset_.z());
  result.orient_ = orient;
  result.mirror_z_ = mirror_z_;
}

void dbTransform::apply(Point& p) const
{
  switch (orient_) {
    case dbOrientType::R0:
      break;

    case dbOrientType::R90:
      p.rotate90();
      break;

    case dbOrientType::R180:
      p.rotate180();
      break;

    case dbOrientType::R270:
      p.rotate270();
      break;

    case dbOrientType::MY:
      p.setX(-p.x());
      break;

    case dbOrientType::MYR90:
      p.setX(-p.x());
      p.rotate90();
      break;

    case dbOrientType::MX:
      p.setY(-p.y());
      break;

    case dbOrientType::MXR90:
      p.setY(-p.y());
      p.rotate90();
      break;
  }

  p.addX(offset_.x());
  p.addY(offset_.y());
}

void dbTransform::apply(Point3D& p) const
{
  Point p2d(p.x(), p.y());
  apply(p2d);

  int z = p.z();
  if (mirror_z_) {
    z = -z;
  }

  p.setX(p2d.x());
  p.setY(p2d.y());
  p.setZ(z + offset_.z());
}

void dbTransform::apply(Rect& r) const
{
  Point ll = r.ll();
  Point ur = r.ur();
  apply(ll);
  apply(ur);
  r.init(ll.x(), ll.y(), ur.x(), ur.y());
}

void dbTransform::apply(Cuboid& c) const
{
  Point3D lll = c.lll();
  Point3D uur = c.uur();
  apply(lll);
  apply(uur);
  c.init(lll.x(), lll.y(), lll.z(), uur.x(), uur.y(), uur.z());
}

void dbTransform::apply(Polygon& p) const
{
  std::vector<Point> points = p.getPoints();
  for (Point& pt : points) {
    apply(pt);
  }
  p.setPoints(points);
}

void dbTransform::concat(const dbTransform& t, dbTransform& result)
{
  result.offset_ = offset_;
  t.apply(result.offset_);
  result.orient_ = orientMul[orient_][t.orient_];
  result.mirror_z_ = mirror_z_ ^ t.mirror_z_;
}

}  // namespace odb
