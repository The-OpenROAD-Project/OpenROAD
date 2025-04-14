// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// OpenDB --- Transformation utilities

#include "odb/dbTransform.h"

#include <vector>

#include "odb/dbStream.h"

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
  stream << (int) t._orient;
  stream << t._offset;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, dbTransform& t)
{
  int orient;
  stream >> orient;
  t._orient = (dbOrientType::Value) orient;
  stream >> t._offset;
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
  Point offset(-_offset.x(), -_offset.y());
  dbOrientType::Value orient;

  switch (_orient) {
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
      throw ZException("Unknown orientation");
  }

  result._offset = offset;
  result._orient = orient;
}

void dbTransform::apply(Point& p) const
{
  switch (_orient) {
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

  p.addX(_offset.x());
  p.addY(_offset.y());
}

void dbTransform::apply(Rect& r) const
{
  Point ll = r.ll();
  Point ur = r.ur();
  apply(ll);
  apply(ur);
  r.init(ll.x(), ll.y(), ur.x(), ur.y());
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
  result._offset = _offset;
  t.apply(result._offset);
  result._orient = orientMul[_orient][t._orient];
}

}  // namespace odb
