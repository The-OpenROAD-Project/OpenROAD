// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "src/drt/src/db/obj/frShape.h"

#include <tuple>

#include "src/drt/src/db/drObj/drShape.h"
#include "src/drt/src/db/obj/frFig.h"
#include "src/drt/src/db/taObj/taShape.h"
#include "src/drt/src/distributed/frArchive.h"
#include "src/drt/src/serialization.h"

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
