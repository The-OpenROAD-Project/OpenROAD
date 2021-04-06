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

namespace fr {
class frBox
{
 public:
  // constructor
  frBox() : ll_(), ur_() {}
  frBox(const frBox& tmpBox) : ll_(tmpBox.ll_), ur_(tmpBox.ur_) {}
  frBox(const box_t& in)
  {
    auto minCorner = in.min_corner();
    auto maxCorner = in.max_corner();
    set(minCorner.x(), minCorner.y(), maxCorner.x(), maxCorner.y());
  }
  frBox(frCoord llx, frCoord lly, frCoord urx, frCoord ury)
  {
    ll_.set((llx > urx) ? urx : llx, (lly > ury) ? ury : lly);
    ur_.set((llx > urx) ? llx : urx, (lly > ury) ? lly : ury);
  }
  frBox(const frPoint& tmpLowerLeft, const frPoint& tmpUpperRight)
      : frBox(tmpLowerLeft.x(),
              tmpLowerLeft.y(),
              tmpUpperRight.x(),
              tmpUpperRight.y())
  {
  }
  // setters
  void set(const frBox& tmpBox)
  {
    ll_.set(tmpBox.ll_);
    ur_.set(tmpBox.ur_);
  }
  void set(frCoord llx, frCoord lly, frCoord urx, frCoord ury)
  {
    ll_.set((llx > urx) ? urx : llx, (lly > ury) ? ury : lly);
    ur_.set((llx > urx) ? llx : urx, (lly > ury) ? lly : ury);
  }
  void set(const frPoint& tmpLowerLeft, const frPoint& tmpUpperRight)
  {
    set(tmpLowerLeft.x(),
        tmpLowerLeft.y(),
        tmpUpperRight.x(),
        tmpUpperRight.y());
  }
  void setUnsafe(const frPoint& tmpLowerLeft, const frPoint& tmpUpperRight)
  {
    ll_.set(tmpLowerLeft);
    ur_.set(tmpUpperRight);
  }
  // getters
  frCoord left() const { return ll_.x(); }
  frCoord bottom() const { return ll_.y(); }
  frCoord right() const { return ur_.x(); }
  frCoord top() const { return ur_.y(); }
  frPoint& lowerLeft() { return ll_; }
  const frPoint& lowerLeft() const { return ll_; }
  frPoint& upperRight() { return ur_; }
  const frPoint& upperRight() const { return ur_; }
  frCoord width() const
  {
    frCoord xSpan = right() - left();
    frCoord ySpan = top() - bottom();
    return std::min(xSpan, ySpan);
  }
  frCoord length() const
  {
    frCoord xSpan = right() - left();
    frCoord ySpan = top() - bottom();
    return std::max(xSpan, ySpan);
  }
  frArea area() const
  {
    frCoord w = right() - left();
    frCoord h = top() - bottom();
    return w * (frArea) h;
  }
  bool contains(const frBox& box, bool incEdges = true) const
  {
    ;
    if (incEdges) {
      return (box.right() <= ur_.x() && box.left() >= ll_.x()
              && box.top() <= ur_.y() && box.bottom() >= ll_.y());
    } else {
      return (box.right() < ur_.x() && box.left() > ll_.x()
              && box.top() < ur_.y() && box.bottom() > ll_.y());
    }
  }
  bool contains(const frPoint& in, bool incEdges = true) const
  {
    if (incEdges) {
      return ll_.x() <= in.x() && in.x() <= ur_.x() && ll_.y() <= in.y()
             && in.y() <= ur_.y();
    } else {
      return ll_.x() < in.x() && in.x() < ur_.x() && ll_.y() < in.y()
             && in.y() < ur_.y();
    }
  }
  bool bloatingContains(int x,
                        int y,
                        int bloatXL = 0,
                        int bloatXH = 0,
                        int bloatYL = 0,
                        int bloatYH = 0) const
  {
    return left() - bloatXL <= x && right() + bloatXH >= x
           && bottom() - bloatYL <= y && top() + bloatYH >= y;
  }
  void merge(const frBox& box)
  {
    set(std::min(left(), box.left()),
        std::min(bottom(), box.bottom()),
        std::max(right(), box.right()),
        std::max(top(), box.top()));
  }
  frCoord distMaxXY(const frBox& bx) const
  {
    frCoord xd = 0, yd = 0;
    if (left() > bx.right()) {
      xd = left() - bx.right();
    } else if (bx.left() > right()) {
      xd = bx.left() - right();
    }
    if (bottom() > bx.top()) {
      yd = bottom() - bx.top();
    } else if (bx.bottom() > top()) {
      yd = bx.bottom() - top();
    }
    return std::max(xd, yd);
  }
  void transform(const frTransform& xform);
  bool overlaps(const frBox& boxIn, bool incEdges = true) const;
  void bloat(const frCoord distance, frBox& boxOut) const;
  bool operator==(const frBox& boxIn) const
  {
    return (ll_ == boxIn.ll_) && (ur_ == boxIn.ur_);
  }
  bool operator<(const frBox& boxIn) const
  {
    if (!(ll_ == boxIn.lowerLeft())) {
      return (ll_ < boxIn.lowerLeft());
    } else {
      return (ur_ < boxIn.upperRight());
    }
  }

 protected:
  frPoint ll_, ur_;
};

class frBox3D : public frBox
{
 public:
  frBox3D() : frBox(), zl_(0), zh_(0) {}
  frBox3D(int llx, int lly, int urx, int ury, int zl, int zh)
      : frBox(llx, lly, urx, ury), zl_(zl), zh_(zh)
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
    return zl_ - bloatZ <= z && zh_ + bloatZ >= z && left() - bloatX <= x
           && right() + bloatX >= x && bottom() - bloatY <= y
           && top() + bloatY >= y;
  }
  int zLow() const { return zl_; }
  int zHigh() const { return zh_; }

 private:
  int zl_;
  int zh_;
};
}  // namespace fr

#endif
