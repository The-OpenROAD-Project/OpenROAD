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

#ifndef _FR_TRANSFORM_H_
#define _FR_TRANSFORM_H_

#include <iostream>

#include "db/infra/frOrient.h"
#include "db/infra/frPoint.h"

namespace fr {
class frTransform
{
 public:
  // constructor
  frTransform() : offset_(), ori_() {}
  frTransform(const frPoint& pointIn,
              const frOrient& orientIn = frOrient(frcR0))
      : offset_(pointIn), ori_(orientIn)
  {
  }
  frTransform(frCoord xOffsetIn,
              frCoord yOffsetIn,
              const frOrient& orientIn = frOrient(frcR0))
      : offset_(xOffsetIn, yOffsetIn), ori_(orientIn)
  {
  }
  // setters
  void set(const frPoint& pointIn) { offset_ = pointIn; }
  void set(const frOrient& orientIn) { ori_ = orientIn; }
  void set(const frPoint& pointIn, const frOrient& orientIn)
  {
    set(pointIn);
    set(orientIn);
  }
  void set(frCoord xOffsetIn, frCoord yOffsetIn)
  {
    set(frPoint(xOffsetIn, yOffsetIn));
  }
  void set(frCoord xOffsetIn, frCoord yOffsetIn, const frOrient& orientIn)
  {
    set(xOffsetIn, yOffsetIn);
    set(orientIn);
  }
  // getters
  frCoord xOffset() const { return offset_.x(); }
  frCoord yOffset() const { return offset_.y(); }
  frOrient orient() const { return ori_; }
  // util
  void updateXform(frPoint& size)
  {
    switch (orient()) {
      // case frcR0: == default
      case frcR90:
        set(xOffset() + size.y(), yOffset());
        break;
      case frcR180:  // verified
        set(xOffset() + size.x(), yOffset() + size.y());
        break;
      case frcR270:
        set(xOffset(), yOffset() + size.x());
        break;
      case frcMY:  // verified
        set(xOffset() + size.x(), yOffset());
        break;
      case frcMXR90:
        set(xOffset(), yOffset());
        break;
      case frcMX:  // verified
        set(xOffset(), yOffset() + size.y());
        break;
      case frcMYR90:
        set(xOffset() + size.y(), yOffset() + size.x());
        break;
      default:  // verified
        set(xOffset(), yOffset());
        break;
    }
  }
  void revert(frTransform& transformIn)
  {
    frCoord resXOffset, resYOffset;
    frOrient resOrient;
    switch (ori_) {
      case frcR0:
        resXOffset = -offset_.x();
        resYOffset = -offset_.y();
        resOrient = frcR0;
        break;
      case frcR90:
        resXOffset = -offset_.y();
        resYOffset = offset_.x();
        resOrient = frcR270;
        break;
      case frcR180:
        resXOffset = offset_.x();
        resYOffset = offset_.y();
        resOrient = frcR180;
        break;
      case frcR270:
        resXOffset = offset_.y();
        resYOffset = -offset_.x();
        resOrient = frcR90;
        break;
      case frcMY:
        resXOffset = offset_.x();
        resYOffset = -offset_.y();
        resOrient = frcMY;
        break;
      case frcMYR90:
        resXOffset = offset_.y();
        resYOffset = offset_.x();
        resOrient = frcMYR90;
        break;
      case frcMX:
        resXOffset = -offset_.x();
        resYOffset = offset_.y();
        resOrient = frcMX;
        break;
      case frcMXR90:
        resXOffset = -offset_.y();
        resYOffset = -offset_.x();
        resOrient = frcMXR90;
        break;
      default:
        resXOffset = -offset_.x();
        resYOffset = -offset_.y();
        resOrient = frcR0;
        std::cout << "Error: unrecognized orient in revert\n";
    }
    transformIn.set(resXOffset, resYOffset, resOrient);
  }

 private:
  frPoint offset_;
  frOrient ori_;

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version)
  {
    (ar) & offset_;
    (ar) & ori_;
  }

  friend class boost::serialization::access;
};
}  // namespace fr

#endif
