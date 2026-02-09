// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

#include <memory>
#include <utility>
#include <vector>

#include "db/obj/frAccess.h"
#include "db/obj/frBPin.h"
#include "db/obj/frNet.h"
#include "db/obj/frTerm.h"
#include "frBaseTypes.h"

namespace drt {
class frBlock;

class frBTerm : public frTerm
{
 public:
  // constructors
  frBTerm(const frString& name) : frTerm(name) {}
  frBTerm(const frBTerm& in) = delete;
  frBTerm& operator=(const frBTerm&) = delete;
  // getters
  bool hasNet() const override { return (net_); }
  frNet* getNet() const override { return net_; }
  const std::vector<std::unique_ptr<frBPin>>& getPins() const { return pins_; }
  bool hasPinAccessUpdate() const { return has_pin_access_update_; }
  void setHasPinAccessUpdate(bool in) { has_pin_access_update_ = in; }
  // setters
  void addToNet(frNet* in) { net_ = in; }
  void addPin(std::unique_ptr<frBPin> in)
  {
    in->setTerm(this);
    for (auto& uFig : in->getFigs()) {
      auto pinFig = uFig.get();
      if (pinFig->typeId() == frcRect) {
        if (bbox_.dx() == 0 && bbox_.dy() == 0) {
          bbox_ = static_cast<frRect*>(pinFig)->getBBox();
        } else {
          bbox_.merge(static_cast<frRect*>(pinFig)->getBBox());
        }
      }
    }
    in->setId(pins_.size());
    pins_.push_back(std::move(in));
  }
  // others
  frBlockObjectEnum typeId() const override { return frcBTerm; }
  frAccessPoint* getAccessPoint(frCoord x,
                                frCoord y,
                                frLayerNum lNum,
                                int pinAccessIdx) override
  {
    if (pinAccessIdx == -1) {
      return nullptr;
    }
    for (auto& pin : pins_) {
      if (!pin->hasPinAccess()) {
        continue;
      }
      for (auto& ap : pin->getPinAccess(pinAccessIdx)->getAccessPoints()) {
        if (x == ap->getPoint().x() && y == ap->getPoint().y()
            && lNum == ap->getLayerNum()) {
          return ap.get();
        }
      }
    }
    return nullptr;
  }
  // fills outShapes with copies of the pinFigs
  void getShapes(std::vector<frRect>& outShapes) const override
  {
    for (const auto& pin : pins_) {
      for (const auto& pinShape : pin->getFigs()) {
        if (pinShape->typeId() == frcRect) {
          outShapes.push_back(*static_cast<const frRect*>(pinShape.get()));
        }
      }
    }
  }

  void setIsAboveTopLayer(bool isAboveTopLayer)
  {
    isAboveTopLayer_ = isAboveTopLayer;
  }
  bool isAboveTopLayer() const { return isAboveTopLayer_; }

 protected:
  std::vector<std::unique_ptr<frBPin>> pins_;  // set later
  frNet* net_{nullptr};
  bool isAboveTopLayer_{false};
  bool has_pin_access_update_{true};
};

}  // namespace drt
