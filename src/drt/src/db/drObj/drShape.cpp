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

#include "db/drObj/drShape.h"

#include "db/drObj/drNet.h"
#include "db/obj/frShape.h"

namespace drt {

Rect drPathSeg::getBBox() const
{
  bool isHorizontal = true;
  if (begin_.x() == end_.x()) {
    isHorizontal = false;
  }
  const auto width = style_.getWidth();
  const auto beginExt = style_.getBeginExt();
  const auto endExt = style_.getEndExt();
  if (isHorizontal) {
    return Rect(begin_.x() - beginExt,
                begin_.y() - width / 2,
                end_.x() + endExt,
                end_.y() + width / 2);
  }
  return Rect(begin_.x() - width / 2,
              begin_.y() - beginExt,
              end_.x() + width / 2,
              end_.y() + endExt);
}

drPathSeg::drPathSeg(const frPathSeg& in) : layer_(in.getLayerNum())
{
  std::tie(begin_, end_) = in.getPoints();
  style_ = in.getStyle();
  setTapered(in.isTapered());
}

drPatchWire::drPatchWire(const frPatchWire& in)
    : layer_(in.getLayerNum()), owner_(nullptr)
{
  offsetBox_ = in.getOffsetBox();
  origin_ = in.getOrigin();
}

}  // namespace drt
