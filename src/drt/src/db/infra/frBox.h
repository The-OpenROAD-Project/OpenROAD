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

#include "db/infra/frPoint.h"
#include "odb/dbTransform.h"

namespace drt {

using odb::dbOrientType;
using odb::dbTransform;
using odb::Rect;

class frBox3D : public Rect
{
 public:
  frBox3D() = default;
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
  int zl_{0};
  int zh_{0};
  template <class Archive>
  void serialize(Archive& ar, const unsigned int version)
  {
    (ar) & boost::serialization::base_object<Rect>(*this);
    (ar) & zl_;
    (ar) & zh_;
  }

  friend class boost::serialization::access;
};
}  // namespace drt
