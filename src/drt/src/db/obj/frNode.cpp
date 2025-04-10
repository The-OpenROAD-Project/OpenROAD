// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "db/obj/frNode.h"

#include "db/grObj/grNet.h"
#include "db/grObj/grNode.h"

namespace drt {

frNode::frNode(grNode& in)
{
  net = in.getNet()->getFrNet();
  loc = in.getLoc();
  layerNum = in.getLayerNum();
  connFig = nullptr;
  pin = nullptr;
  type = in.getType();
  parent = nullptr;
}

}  // namespace drt
