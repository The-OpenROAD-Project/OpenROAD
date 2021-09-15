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

#include "odb/dbTypes.h"
#include "db/infra/frPoint.h"
using dbOrientType = odb::dbOrientType;

namespace fr {
class frTransform
{
 public:
  // constructor
  frTransform() : offset_(), ori_() {}
  frTransform(const frPoint& pointIn,
              const dbOrientType& orientIn = dbOrientType(dbOrientType::R0))
      : offset_(pointIn), ori_(orientIn)
  {
  }
  frTransform(frCoord xOffsetIn,
              frCoord yOffsetIn,
              const dbOrientType& orientIn = dbOrientType(dbOrientType::R0))
      : offset_(xOffsetIn, yOffsetIn), ori_(orientIn)
  {
  }
  // setters
  void set(const frPoint& pointIn) { offset_ = pointIn; }
  void set(const dbOrientType& orientIn) { ori_ = orientIn; }
  void set(const frPoint& pointIn, const dbOrientType& orientIn)
  {
    set(pointIn);
    set(orientIn);
  }
  void set(frCoord xOffsetIn, frCoord yOffsetIn)
  {
    set(frPoint(xOffsetIn, yOffsetIn));
  }
  void set(frCoord xOffsetIn, frCoord yOffsetIn, const dbOrientType& orientIn)
  {
    set(xOffsetIn, yOffsetIn);
    set(orientIn);
  }
  // getters
  frCoord xOffset() const { return offset_.x(); }
  frCoord yOffset() const { return offset_.y(); }
  dbOrientType orient() const { return ori_; }
  // util
  void updateXform(frPoint& size)
  {
    switch (orient()) {
      // case R0: == default
      case dbOrientType::R90:
        set(xOffset() + size.y(), yOffset());
        break;
      case dbOrientType::R180:  // verified
        set(xOffset() + size.x(), yOffset() + size.y());
        break;
      case dbOrientType::R270:
        set(xOffset(), yOffset() + size.x());
        break;
      case dbOrientType::MY:  // verified
        set(xOffset() + size.x(), yOffset());
        break;
      case dbOrientType::MXR90:
        set(xOffset(), yOffset());
        break;
      case dbOrientType::MX:  // verified
        set(xOffset(), yOffset() + size.y());
        break;
      case dbOrientType::MYR90:
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
    dbOrientType resOrient;
    switch (ori_) {
      case dbOrientType::R0:
        resXOffset = -offset_.x();
        resYOffset = -offset_.y();
        resOrient = dbOrientType::R0;
        break;
      case dbOrientType::R90:
        resXOffset = -offset_.y();
        resYOffset = offset_.x();
        resOrient = dbOrientType::R270;
        break;
      case dbOrientType::R180:
        resXOffset = offset_.x();
        resYOffset = offset_.y();
        resOrient = dbOrientType::R180;
        break;
      case dbOrientType::R270:
        resXOffset = offset_.y();
        resYOffset = -offset_.x();
        resOrient = dbOrientType::R90;
        break;
      case dbOrientType::MY:
        resXOffset = offset_.x();
        resYOffset = -offset_.y();
        resOrient = dbOrientType::MY;
        break;
      case dbOrientType::MYR90:
        resXOffset = offset_.y();
        resYOffset = offset_.x();
        resOrient = dbOrientType::MYR90;
        break;
      case dbOrientType::MX:
        resXOffset = -offset_.x();
        resYOffset = offset_.y();
        resOrient = dbOrientType::MX;
        break;
      case dbOrientType::MXR90:
        resXOffset = -offset_.y();
        resYOffset = -offset_.x();
        resOrient = dbOrientType::MXR90;
        break;
      default:
        resXOffset = -offset_.x();
        resYOffset = -offset_.y();
        resOrient = dbOrientType::R0;
        std::cout << "Error: unrecognized orient in revert\n";
    }
    transformIn.set(resXOffset, resYOffset, resOrient);
  }

 protected:
  frPoint offset_;
  dbOrientType ori_;
};
}  // namespace fr

#endif
