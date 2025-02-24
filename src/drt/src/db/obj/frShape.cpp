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

#include "db/obj/frShape.h"

#include "db/drObj/drShape.h"
#include "db/taObj/taShape.h"
#include "distributed/frArchive.h"
#include "serialization.h"

namespace drt {

frPathSeg::frPathSeg(const drPathSeg& in)
{
  std::tie(begin_, end_) = in.getPoints();
  layer_ = in.getLayerNum();
  style_ = in.getStyle();
  owner_ = nullptr;
  setTapered(in.isTapered());
  if (in.isApPathSeg()) {
    setApPathSeg(in.getApLoc());
  }
}

frPathSeg::frPathSeg(const taPathSeg& in)
{
  std::tie(begin_, end_) = in.getPoints();
  layer_ = in.getLayerNum();
  style_ = in.getStyle();
  owner_ = nullptr;
  setTapered(false);
}

frPatchWire::frPatchWire(const drPatchWire& in)
{
  origin_ = in.getOrigin();
  offsetBox_ = in.getOffsetBox();
  layer_ = in.getLayerNum();
  owner_ = nullptr;
}

template <class Archive>
void frShape::serialize(Archive& ar, const unsigned int version)
{
  (ar) & boost::serialization::base_object<frPinFig>(*this);
  (ar) & index_in_owner_;
  serializeBlockObject(ar, owner_);
}

template void frShape::serialize<frIArchive>(frIArchive& ar,
                                             const unsigned int file_version);

template void frShape::serialize<frOArchive>(frOArchive& ar,
                                             const unsigned int file_version);

}  // namespace drt
