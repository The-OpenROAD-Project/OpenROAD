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

#include "db/obj/frAccess.h"

#include "db/tech/frViaDef.h"
#include "distributed/frArchive.h"
#include "serialization.h"

namespace drt {

void frAccessPoint::addViaDef(const frViaDef* in)
{
  auto numCut = in->getNumCut();
  int numCutIdx = numCut - 1;
  if (numCut > (int) viaDefs_.size()) {
    viaDefs_.resize(numCut, {});
  }
  viaDefs_[numCutIdx].push_back(in);
}

template <class Archive>
void frAccessPoint::serialize(Archive& ar, const unsigned int version)
{
  (ar) & boost::serialization::base_object<frBlockObject>(*this);
  (ar) & point_;
  (ar) & layerNum_;
  (ar) & accesses_;
  (ar) & typeL_;
  (ar) & typeH_;
  (ar) & pathSegs_;
  if (is_loading(ar)) {
    int outSz = 0;
    (ar) & outSz;
    for (int i = 0; i < outSz; i++) {
      viaDefs_.emplace_back();
      int inSz = 0;
      (ar) & inSz;
      while (inSz--) {
        const frViaDef* vd;
        serializeViaDef(ar, vd);
        viaDefs_[i].push_back(vd);
      }
    }
  } else {
    int sz = viaDefs_.size();
    (ar) & sz;
    for (const auto& col : viaDefs_) {
      sz = col.size();
      (ar) & sz;
      for (auto vd : col) {
        serializeViaDef(ar, vd);
      }
    }
  }
}
template <class Archive>
void frPinAccess::serialize(Archive& ar, const unsigned int version)
{
  (ar) & boost::serialization::base_object<frBlockObject>(*this);
  (ar) & aps_;
  for (const auto& ap : aps_) {
    ap->addToPinAccess(this);
  }
}

// Explicit instantiations
template void frAccessPoint::serialize<frIArchive>(
    frIArchive& ar,
    const unsigned int file_version);

template void frAccessPoint::serialize<frOArchive>(
    frOArchive& ar,
    const unsigned int file_version);

template void frPinAccess::serialize<frIArchive>(
    frIArchive& ar,
    const unsigned int file_version);

template void frPinAccess::serialize<frOArchive>(
    frOArchive& ar,
    const unsigned int file_version);

}  // namespace drt
