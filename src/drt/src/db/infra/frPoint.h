/* Authors: Lutong Wang and Bangqi Xu */
/*
 * Copyright (c) 2019, The Regents of the University of California
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "frBaseTypes.h"
#include "odb/geom.h"

namespace drt {
using odb::Point;

class Point3D : public Point
{
 public:
  Point3D() = default;
  Point3D(int x, int y, int z) : Point(x, y), z_(z) {}
  Point3D(const Point3D& p) : Point(p.getX(), p.getY()), z_(p.getZ()) {}
  Point3D(const Point& p, int z) : Point(p), z_(z) {}

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
    if (Point::operator!=(rhs)) {
      return Point::operator<(rhs);
    }
    return z_ < rhs.z_;
  }

 private:
  int z_{0};
  template <class Archive>
  void serialize(Archive& ar, const unsigned int version)
  {
    (ar) & boost::serialization::base_object<Point>(*this);
    (ar) & z_;
  }

  friend class boost::serialization::access;
};
}  // namespace drt
