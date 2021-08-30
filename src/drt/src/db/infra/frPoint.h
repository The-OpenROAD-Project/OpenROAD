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

#ifndef _FR_POINT_H_
#define _FR_POINT_H_

#include "frBaseTypes.h"

namespace fr {
class frTransform;

class frPoint
{
 public:
  // constructors
  frPoint() : xCoord_(0), yCoord_(0) {}
  frPoint(const frPoint& tmpPoint)
      : xCoord_(tmpPoint.xCoord_), yCoord_(tmpPoint.yCoord_)
  {
  }
  frPoint(const frCoord tmpX, const frCoord tmpY)
      : xCoord_(tmpX), yCoord_(tmpY){};
  // setters
  void set(const frPoint& tmpPoint)
  {
    xCoord_ = tmpPoint.xCoord_;
    yCoord_ = tmpPoint.yCoord_;
  }
  void set(const frCoord tmpX, const frCoord tmpY)
  {
    xCoord_ = tmpX;
    yCoord_ = tmpY;
  }
  void setX(const frCoord tmpX) { xCoord_ = tmpX; }
  void setY(const frCoord tmpY) { yCoord_ = tmpY; }
  // getters
  frCoord x() const { return xCoord_; }
  frCoord y() const { return yCoord_; }
  // others
  void transform(const frTransform& xform);
  bool operator<(const frPoint& pIn) const
  {
    return (xCoord_ == pIn.xCoord_) ? (yCoord_ < pIn.yCoord_)
                                    : (xCoord_ < pIn.xCoord_);
  }
  bool operator==(const frPoint& pIn) const
  {
    return (xCoord_ == pIn.xCoord_) && (yCoord_ == pIn.yCoord_);
  }
  bool operator!=(const frPoint& pIn) const { return !(*this == pIn); }

 protected:
  frCoord xCoord_, yCoord_;
};

class Point3D : public frPoint {
    public:
        Point3D() : frPoint(0, 0), z_(0) { }
        Point3D(int x, int y, int z) : frPoint(x, y), z_(z) { }
        Point3D(const Point3D& p) : frPoint(p.x(), p.y()), z_(p.z()) { }
        
        int z() const { return z_; }
        void setZ(int z) { z_ = z; }
        
        bool operator==(const Point3D& pIn) const {
          return (xCoord_ == pIn.xCoord_) && (yCoord_ == pIn.yCoord_) && z_ == pIn.z_;
        }
        
        bool operator!=(const Point3D& pIn) const { return !(*this == pIn); }

    private:
        int z_;
};
}  // namespace fr

#endif
