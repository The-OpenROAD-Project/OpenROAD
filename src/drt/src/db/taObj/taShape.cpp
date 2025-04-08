// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "db/taObj/taShape.h"

#include "db/obj/frShape.h"

namespace drt {

taPathSeg::taPathSeg(const frPathSeg& in)
{
  std::tie(begin_, end_) = in.getPoints();
  layer_ = in.getLayerNum();
  style_ = in.getStyle();
  owner_ = nullptr;
}

}  // namespace drt
