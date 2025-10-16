// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <algorithm>
#include <memory>

#include "db/obj/frShape.h"
#include "db/taObj/taFig.h"
#include "db/tech/frViaDef.h"
#include "frBaseTypes.h"
#include "odb/dbTransform.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"

namespace drt {
class frNet;
class taRef : public taPinFig
{
 public:
  // getters
  virtual odb::dbOrientType getOrient() const = 0;
  virtual odb::Point getOrigin() const = 0;
  virtual odb::dbTransform getTransform() const = 0;
  // setters
  virtual void setOrient(const odb::dbOrientType& tmpOrient) = 0;
  virtual void setOrigin(const odb::Point& tmpPoint) = 0;
  virtual void setTransform(const odb::dbTransform& xform) = 0;
};

class taVia : public taRef
{
 public:
  // constructors
  taVia() = default;
  taVia(const frViaDef* in) : viaDef_(in) {}
  // getters
  const frViaDef* getViaDef() const { return viaDef_; }
  odb::Rect getLayer1BBox() const
  {
    odb::Rect box;
    box.mergeInit();
    for (auto& fig : viaDef_->getLayer1Figs()) {
      box.merge(fig->getBBox());
    }
    getTransform().apply(box);
    return box;
  }
  odb::Rect getCutBBox() const
  {
    odb::Rect box;
    box.mergeInit();
    for (auto& fig : viaDef_->getCutFigs()) {
      box.merge(fig->getBBox());
    }
    getTransform().apply(box);
    return box;
  }
  odb::Rect getLayer2BBox() const
  {
    odb::Rect box;
    box.mergeInit();
    for (auto& fig : viaDef_->getLayer2Figs()) {
      box.merge(fig->getBBox());
    }
    getTransform().apply(box);
    return box;
  }
  // setters
  void setViaDef(const frViaDef* in) { viaDef_ = in; }
  // others
  frBlockObjectEnum typeId() const override { return tacVia; }

  /* from frRef
   * getOrient
   * setOrient
   * getOrigin
   * setOrigin
   * getTransform
   * setTransform
   */

  odb::dbOrientType getOrient() const override { return odb::dbOrientType(); }
  void setOrient(const odb::dbOrientType& tmpOrient) override { ; }
  odb::Point getOrigin() const override { return origin_; }
  void setOrigin(const odb::Point& tmpPoint) override { origin_ = tmpPoint; }
  odb::dbTransform getTransform() const override
  {
    return odb::dbTransform(origin_);
  }
  void setTransform(const odb::dbTransform& xformIn) override {}

  /* from frPinFig
   * hasPin
   * getPin
   * addToPin
   * removeFromPin
   */
  bool hasPin() const override
  {
    return (owner_) && (owner_->typeId() == tacPin);
  }
  taPin* getPin() const override { return reinterpret_cast<taPin*>(owner_); }
  void addToPin(taPin* in) override
  {
    owner_ = reinterpret_cast<frBlockObject*>(in);
  }
  void removeFromPin() override { owner_ = nullptr; }

  /* from frConnFig
   * hasNet
   * getNet
   * addToNet
   * removeFromNet
   */
  bool hasNet() const override
  {
    return (owner_) && (owner_->typeId() == frcNet);
  }

  void addToNet(frNet* in) override
  {
    owner_ = reinterpret_cast<frBlockObject*>(in);
  }
  void removeFromNet() override { owner_ = nullptr; }

  /* from frFig
   * getBBox
   * move
   * overlaps
   */

  odb::Rect getBBox() const override
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
  void move(const odb::dbTransform& xform) override { ; }
  bool overlaps(const odb::Rect& box) const override { return false; }

 protected:
  odb::Point origin_;
  const frViaDef* viaDef_{nullptr};
  frBlockObject* owner_{nullptr};
};
}  // namespace drt
