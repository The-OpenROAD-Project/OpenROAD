// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "db/drObj/drVia.h"

#include <algorithm>

#include "db/drObj/drNet.h"
#include "db/drObj/drRef.h"
#include "db/obj/frVia.h"
#include "distributed/frArchive.h"
#include "frBaseTypes.h"
#include "odb/geom.h"

namespace drt {

drVia::drVia(const frVia& in)
    : origin_(in.getOrigin()),
      viaDef_(in.getViaDef()),
      tapered_(in.isTapered()),
      bottomConnected_(in.isBottomConnected()),
      topConnected_(in.isTopConnected()),
      isLonely_(in.isLonely())
{
}

odb::Rect drVia::getBBox() const
{
  auto& layer1Figs = viaDef_->getLayer1Figs();
  auto& layer2Figs = viaDef_->getLayer2Figs();
  auto& cutFigs = viaDef_->getCutFigs();
  bool isFirst = true;
  frCoord xl = 0;
  frCoord yl = 0;
  frCoord xh = 0;
  frCoord yh = 0;
  for (auto& fig : layer1Figs) {
    odb::Rect box = fig->getBBox();
    if (isFirst) {
      xl = box.xMin();
      yl = box.yMin();
      xh = box.xMax();
      yh = box.yMax();
      isFirst = false;
    } else {
      xl = std::min(xl, box.xMin());
      yl = std::min(yl, box.yMin());
      xh = std::max(xh, box.xMax());
      yh = std::max(yh, box.yMax());
    }
  }
  for (auto& fig : layer2Figs) {
    odb::Rect box = fig->getBBox();
    if (isFirst) {
      xl = box.xMin();
      yl = box.yMin();
      xh = box.xMax();
      yh = box.yMax();
      isFirst = false;
    } else {
      xl = std::min(xl, box.xMin());
      yl = std::min(yl, box.yMin());
      xh = std::max(xh, box.xMax());
      yh = std::max(yh, box.yMax());
    }
  }
  for (auto& fig : cutFigs) {
    odb::Rect box = fig->getBBox();
    if (isFirst) {
      xl = box.xMin();
      yl = box.yMin();
      xh = box.xMax();
      yh = box.yMax();
      isFirst = false;
    } else {
      xl = std::min(xl, box.xMin());
      yl = std::min(yl, box.yMin());
      xh = std::max(xh, box.xMax());
      yh = std::max(yh, box.yMax());
    }
  }
  odb::Rect box(xl, yl, xh, yh);
  getTransform().apply(box);
  return box;
}

template <class Archive>
void drVia::serialize(Archive& ar, const unsigned int version)
{
  (ar) & boost::serialization::base_object<drRef>(*this);
  (ar) & origin_;
  (ar) & owner_;
  (ar) & beginMazeIdx_;
  (ar) & endMazeIdx_;
  serializeViaDef(ar, viaDef_);
  bool tmp = false;
  if (is_loading(ar)) {
    (ar) & tmp;
    tapered_ = tmp;
    (ar) & tmp;
    bottomConnected_ = tmp;
    (ar) & tmp;
    topConnected_ = tmp;

  } else {
    tmp = tapered_;
    (ar) & tmp;
    tmp = bottomConnected_;
    (ar) & tmp;
    tmp = topConnected_;
    (ar) & tmp;
  }
}

// Explicit instantiations
template void drVia::serialize<frIArchive>(frIArchive& ar,
                                           const unsigned int file_version);

template void drVia::serialize<frOArchive>(frOArchive& ar,
                                           const unsigned int file_version);

}  // namespace drt
