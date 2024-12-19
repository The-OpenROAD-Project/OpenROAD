/* Authors: Lutong Wang and Bangqi Xu */
/*
 * Copyright (c) 2019, The Regents of the University of California
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <memory>

#include "db/obj/frShape.h"
#include "db/taObj/taFig.h"
#include "db/tech/frViaDef.h"
#include "odb/dbTypes.h"

namespace drt {
class frNet;
class taRef : public taPinFig
{
 public:
  // getters
  virtual dbOrientType getOrient() const = 0;
  virtual Point getOrigin() const = 0;
  virtual dbTransform getTransform() const = 0;
  // setters
  virtual void setOrient(const dbOrientType& tmpOrient) = 0;
  virtual void setOrigin(const Point& tmpPoint) = 0;
  virtual void setTransform(const dbTransform& xform) = 0;
};

class taVia : public taRef
{
 public:
  // constructors
  taVia() = default;
  taVia(const frViaDef* in) : viaDef_(in) {}
  // getters
  const frViaDef* getViaDef() const { return viaDef_; }
  Rect getLayer1BBox() const
  {
    Rect box;
    box.mergeInit();
    for (auto& fig : viaDef_->getLayer1Figs()) {
      box.merge(fig->getBBox());
    }
    getTransform().apply(box);
    return box;
  }
  Rect getCutBBox() const
  {
    Rect box;
    box.mergeInit();
    for (auto& fig : viaDef_->getCutFigs()) {
      box.merge(fig->getBBox());
    }
    getTransform().apply(box);
    return box;
  }
  Rect getLayer2BBox() const
  {
    Rect box;
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

  dbOrientType getOrient() const override { return dbOrientType(); }
  void setOrient(const dbOrientType& tmpOrient) override { ; }
  Point getOrigin() const override { return origin_; }
  void setOrigin(const Point& tmpPoint) override { origin_ = tmpPoint; }
  dbTransform getTransform() const override { return dbTransform(origin_); }
  void setTransform(const dbTransform& xformIn) override {}

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

  Rect getBBox() const override
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
      Rect box = fig->getBBox();
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
      Rect box = fig->getBBox();
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
      Rect box = fig->getBBox();
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
    Rect box(xl, yl, xh, yh);
    getTransform().apply(box);
    return box;
  }
  void move(const dbTransform& xform) override { ; }
  bool overlaps(const Rect& box) const override { return false; }

 protected:
  Point origin_;
  const frViaDef* viaDef_{nullptr};
  frBlockObject* owner_{nullptr};
};
}  // namespace drt
