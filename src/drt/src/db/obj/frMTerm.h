// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

#include <memory>
#include <utility>
#include <vector>

#include "db/obj/frAccess.h"
#include "db/obj/frMPin.h"
#include "db/obj/frNet.h"
#include "db/obj/frTerm.h"
#include "frBaseTypes.h"

namespace drt {
class frMaster;

class frMTerm : public frTerm
{
 public:
  // constructors
  frMTerm(const frString& name) : frTerm(name) {}
  frMTerm(const frMTerm& in) = delete;
  frMTerm& operator=(const frMTerm&) = delete;
  // getters
  bool hasNet() const override { return false; }
  frNet* getNet() const override { return nullptr; }
  frMaster* getMaster() const { return master_; }
  const std::vector<std::unique_ptr<frMPin>>& getPins() const { return pins_; }
  // setters
  void setMaster(frMaster* in) { master_ = in; }
  void addPin(std::unique_ptr<frMPin> in)
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
  frBlockObjectEnum typeId() const override { return frcMTerm; }
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

 protected:
  frMaster* master_{nullptr};
  std::vector<std::unique_ptr<frMPin>> pins_;  // set later
};

}  // namespace drt
