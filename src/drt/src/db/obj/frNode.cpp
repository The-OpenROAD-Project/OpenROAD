// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "db/obj/frNode.h"

#include "db/grObj/grNet.h"
#include "db/grObj/grNode.h"

namespace drt {

frNode::frNode(grNode& in)
{
  net_ = in.getNet()->getFrNet();
  loc_ = in.getLoc();
  layerNum_ = in.getLayerNum();
  connFig_ = nullptr;
  pin_ = nullptr;
  type_ = in.getType();
  parent_ = nullptr;
}

}  // namespace drt
