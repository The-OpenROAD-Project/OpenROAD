///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, Nefelus Inc
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

// OpenDB --- Transformation utilities

#include "dbTransform.h"

#ifndef DB_TRANSFORM_TEST
#include "dbDiff.h"
#include "dbStream.h"
#endif

namespace odb {

static dbOrientType::Value orientMul[8][8] = {{dbOrientType::R0,
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

#ifndef DB_TRANSFORM_TEST
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

dbDiff& operator<<(dbDiff& diff, const dbTransform& t)
{
  diff << (int) t._orient;
  diff << t._offset;
  return diff;
}
#endif

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
  Point               offset(-_offset.x(), -_offset.y());
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
      offset.x() = -offset.x();
      orient     = dbOrientType::MY;
      break;

    case dbOrientType::MYR90:
      offset.x() = -offset.x();
      offset.rotate90();
      orient = dbOrientType::MYR90;
      break;

    case dbOrientType::MX:
      offset.y() = -offset.y();
      orient     = dbOrientType::MX;
      break;

    case dbOrientType::MXR90:
      offset.y() = -offset.y();
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
      p.x() = -p.x();
      break;

    case dbOrientType::MYR90:
      p.x() = -p.x();
      p.rotate90();
      break;

    case dbOrientType::MX:
      p.y() = -p.y();
      break;

    case dbOrientType::MXR90:
      p.y() = -p.y();
      p.rotate90();
      break;
  }

  p.x() += _offset.x();
  p.y() += _offset.y();
}

void dbTransform::apply(Rect& r) const
{
  Point ll = r.ll();
  Point ur = r.ur();
  apply(ll);
  apply(ur);
  r.init(ll.x(), ll.y(), ur.x(), ur.y());
}

void dbTransform::concat(const dbTransform& t, dbTransform& result)
{
  result._offset = _offset;
  t.apply(result._offset);
  result._orient = orientMul[_orient][t._orient];
}

}  // namespace odb
