// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "db/infra/frBox.h"
#include "db/obj/frBlockObject.h"
#include "frBaseTypes.h"

namespace drt {
class frNet;
class frAccessPoint;
// serve the same purpose as drPin and grPin, but on fr level
class frRPin : public frBlockObject
{
 public:
  // constructors
  frRPin() = default;
  // setters
  void setFrTerm(frBlockObject* in) { term = in; }
  void setAccessPoint(frAccessPoint* in) { accessPoint = in; }
  void addToNet(frNet* in) { net = in; }
  // getters
  bool hasFrTerm() const { return (term); }
  frBlockObject* getFrTerm() const { return term; }
  frAccessPoint* getAccessPoint() const { return accessPoint; }
  frNet* getNet() const { return net; }

  // utility
  Rect getBBox();
  frLayerNum getLayerNum();

  // others
  frBlockObjectEnum typeId() const override { return frcRPin; }

 protected:
  frBlockObject* term{nullptr};         // either frBTerm or frInstTerm
  frAccessPoint* accessPoint{nullptr};  // pref AP for frBTerm and frInstTerm
  frNet* net{nullptr};
};
}  // namespace drt
