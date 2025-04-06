// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

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
