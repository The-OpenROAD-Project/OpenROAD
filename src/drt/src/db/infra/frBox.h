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

#ifndef _FR_BOX_H_
#define _FR_BOX_H_

#include "db/infra/frPoint.h"
#include "frBaseTypes.h"
#include "odb/dbTransform.h"
using odb::dbOrientType;
using odb::dbTransform;
using odb::Rect;

namespace fr {
class frBox : public Rect
{
 public:
  // constructor
  frBox() : Rect() {}
  frBox(const Rect& tmpBox) : Rect(tmpBox.ll(), tmpBox.ur()) {}
  frBox(const box_t& in)
  {
    auto minCorner = in.min_corner();
    auto maxCorner = in.max_corner();
    init(minCorner.x(), minCorner.y(),
         maxCorner.x(), maxCorner.y());
  }
  frBox(frCoord llx, frCoord lly, frCoord urx, frCoord ury)
    : Rect(llx, lly, urx, ury) {}
  frBox(const Point& tmpLowerLeft, const Point& tmpUpperRight)
    : Rect(tmpLowerLeft, tmpUpperRight) {}
  // setters
  void set(const Rect& tmpBox) { *this = tmpBox; }
  void set(const Point& tmpLowerLeft, const Point& tmpUpperRight)
  {
    init(tmpLowerLeft.getX(),
         tmpLowerLeft.getY(),
         tmpUpperRight.getX(),
         tmpUpperRight.getY());
  }
  // getters
  bool contains(const Rect& box) const
  {
    return Rect::contains(Rect(box.ll(), box.ur()));
  }
  bool intersects(const Point& in) const
  {
    return Rect::intersects(in);
  }
  void merge(const Rect& box) { Rect::merge(box); }
  void moveDelta(int x, int y) { Rect::moveDelta(x, y); }
  bool overlaps(const Rect& boxIn) const
  {
    return Rect::overlaps(Rect(boxIn.ll(), boxIn.ur()));
  }
  bool operator==(const Rect& boxIn) const
  {
    return (ll() == boxIn.ll()) && (ur() == boxIn.ur());
  }
  bool operator<(const Rect& boxIn) const
  {
    if (!(ll() == boxIn.ll())) {
      return (ll() < boxIn.ll());
    } else {
      return (ur() < boxIn.ur());
    }
  }
  void intersection(const Rect& b, Rect& result)
  {
    Rect r;
    Rect in(b.ll(), b.ur());
    Rect::intersection(in, r);
    result.init(r.xMin(), r.yMin(), r.xMax(), r.yMax());
  }
};

class frBox3D : public Rect
{
 public:
  frBox3D() : Rect(), zl_(0), zh_(0) {}
  frBox3D(int llx, int lly, int urx, int ury, int zl, int zh)
      : Rect(llx, lly, urx, ury), zl_(zl), zh_(zh)
  {
  }
  frBox3D(const frBox3D& in) = default;
  bool contains(int x,
                int y,
                int z,
                int bloatX = 0,
                int bloatY = 0,
                int bloatZ = 0) const
  {
    return zl_ - bloatZ <= z && zh_ + bloatZ >= z && xMin() - bloatX <= x
           && xMax() + bloatX >= x && yMin() - bloatY <= y
           && yMax() + bloatY >= y;
  }
  int zLow() const { return zl_; }
  int zHigh() const { return zh_; }

 private:
  int zl_;
  int zh_;
};
}  // namespace fr

#endif
