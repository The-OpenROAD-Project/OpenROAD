// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "db/infra/frBox.h"
#include "db/obj/frBlockObject.h"
#include "frBaseTypes.h"
#include "odb/geom.h"

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
  void setFrTerm(frBlockObject* in) { term_ = in; }
  void setAccessPoint(frAccessPoint* in) { accessPoint_ = in; }
  void addToNet(frNet* in) { net_ = in; }
  // getters
  bool hasFrTerm() const { return term_; }
  frBlockObject* getFrTerm() const { return term_; }
  frAccessPoint* getAccessPoint() const { return accessPoint_; }
  frNet* getNet() const { return net_; }

  // utility
  odb::Rect getBBox();
  frLayerNum getLayerNum();

  // others
  frBlockObjectEnum typeId() const override { return frcRPin; }

 protected:
  frBlockObject* term_{nullptr};         // either frBTerm or frInstTerm
  frAccessPoint* accessPoint_{nullptr};  // pref AP for frBTerm and frInstTerm
  frNet* net_{nullptr};
};
}  // namespace drt
