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

#ifndef _FR_BOUNDARY_H_
#define _FR_BOUNDARY_H_

#include <memory>

#include "db/obj/frFig.h"
#include "frBaseTypes.h"

namespace fr {
class frBoundary : public frFig
{
 public:
  // constructors
  frBoundary() : frFig() {}
  // getters
  const std::vector<frPoint>& getPoints() const { return points_; }
  frUInt4 getNumPoints() const { return points_.size(); }
  // setters
  void setPoints(const std::vector<frPoint>& pIn) { points_ = pIn; }
  // others
  frBlockObjectEnum typeId() const override { return frcBoundary; }

  void getBBox(frBox& boxIn) const override
  {
    frCoord llx = 0;
    frCoord lly = 0;
    frCoord urx = 0;
    frCoord ury = 0;
    if (points_.size()) {
      llx = points_.begin()->x();
      urx = points_.begin()->x();
      lly = points_.begin()->y();
      ury = points_.begin()->y();
    }
    for (auto& point : points_) {
      llx = (llx < point.x()) ? llx : point.x();
      lly = (lly < point.y()) ? lly : point.y();
      urx = (urx > point.x()) ? urx : point.x();
      ury = (ury > point.y()) ? ury : point.y();
    }
    boxIn.set(llx, lly, urx, ury);
  }
  void move(const frTransform& xform) override
  {
    for (auto& point : points_) {
      point.transform(xform);
    }
  }
  bool overlaps(const frBox& box) const override { return false; }

 protected:
  std::vector<frPoint> points_;
};
}  // namespace fr

#endif
