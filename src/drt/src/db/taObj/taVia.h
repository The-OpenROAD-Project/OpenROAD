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

#ifndef _TA_VIA_H_
#define _TA_VIA_H_

#include <memory>

#include "odb/dbTypes.h"
#include "db/obj/frShape.h"
#include "db/taObj/taFig.h"
#include "db/tech/frViaDef.h"

namespace fr {
class frNet;
class taRef : public taPinFig
{
 public:
  // constructors
  taRef() : taPinFig() {}
  // getters
  virtual dbOrientType getOrient() const = 0;
  virtual void getOrigin(frPoint& tmpOrigin) const = 0;
  virtual void getTransform(frTransform& xform) const = 0;
  // setters
  virtual void setOrient(const dbOrientType& tmpOrient) = 0;
  virtual void setOrigin(const frPoint& tmpPoint) = 0;
  virtual void setTransform(const frTransform& xform) = 0;

 protected:
};
class taVia : public taRef
{
 public:
  // constructors
  taVia() : viaDef_(nullptr), owner_(nullptr) {}
  taVia(frViaDef* in) : taRef(), origin_(), viaDef_(in), owner_(nullptr) {}
  // getters
  frViaDef* getViaDef() const { return viaDef_; }
  void getLayer1BBox(frBox& boxIn) const
  {
    auto& figs = viaDef_->getLayer1Figs();
    bool isFirst = true;
    frBox box;
    frCoord xl = 0;
    frCoord yl = 0;
    frCoord xh = 0;
    frCoord yh = 0;
    for (auto& fig : figs) {
      fig->getBBox(box);
      if (isFirst) {
        xl = box.left();
        yl = box.bottom();
        xh = box.right();
        yh = box.top();
        isFirst = false;
      } else {
        xl = std::min(xl, box.left());
        yl = std::min(yl, box.bottom());
        xh = std::max(xh, box.right());
        yh = std::max(yh, box.top());
      }
    }
    boxIn.set(xl, yl, xh, yh);
    frTransform xform;
    xform.set(origin_);
    boxIn.transform(xform);
  }
  void getCutBBox(frBox& boxIn) const
  {
    auto& figs = viaDef_->getCutFigs();
    bool isFirst = true;
    frBox box;
    frCoord xl = 0;
    frCoord yl = 0;
    frCoord xh = 0;
    frCoord yh = 0;
    for (auto& fig : figs) {
      fig->getBBox(box);
      if (isFirst) {
        xl = box.left();
        yl = box.bottom();
        xh = box.right();
        yh = box.top();
        isFirst = false;
      } else {
        xl = std::min(xl, box.left());
        yl = std::min(yl, box.bottom());
        xh = std::max(xh, box.right());
        yh = std::max(yh, box.top());
      }
    }
    boxIn.set(xl, yl, xh, yh);
    frTransform xform;
    xform.set(origin_);
    boxIn.transform(xform);
  }
  void getLayer2BBox(frBox& boxIn) const
  {
    auto& figs = viaDef_->getLayer2Figs();
    bool isFirst = true;
    frBox box;
    frCoord xl = 0;
    frCoord yl = 0;
    frCoord xh = 0;
    frCoord yh = 0;
    for (auto& fig : figs) {
      fig->getBBox(box);
      if (isFirst) {
        xl = box.left();
        yl = box.bottom();
        xh = box.right();
        yh = box.top();
        isFirst = false;
      } else {
        xl = std::min(xl, box.left());
        yl = std::min(yl, box.bottom());
        xh = std::max(xh, box.right());
        yh = std::max(yh, box.top());
      }
    }
    boxIn.set(xl, yl, xh, yh);
    frTransform xform;
    xform.set(origin_);
    boxIn.transform(xform);
  }
  // setters
  void setViaDef(frViaDef* in) { viaDef_ = in; }
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
  void getOrigin(frPoint& tmpOrigin) const override { tmpOrigin.set(origin_); }
  void setOrigin(const frPoint& tmpPoint) override { origin_.set(tmpPoint); }
  void getTransform(frTransform& xformIn) const override
  {
    xformIn.set(origin_);
  }
  void setTransform(const frTransform& xformIn) override {}

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

  void getBBox(frBox& boxIn) const override
  {
    auto& layer1Figs = viaDef_->getLayer1Figs();
    auto& layer2Figs = viaDef_->getLayer2Figs();
    auto& cutFigs = viaDef_->getCutFigs();
    bool isFirst = true;
    frBox box;
    frCoord xl = 0;
    frCoord yl = 0;
    frCoord xh = 0;
    frCoord yh = 0;
    for (auto& fig : layer1Figs) {
      fig->getBBox(box);
      if (isFirst) {
        xl = box.left();
        yl = box.bottom();
        xh = box.right();
        yh = box.top();
        isFirst = false;
      } else {
        xl = std::min(xl, box.left());
        yl = std::min(yl, box.bottom());
        xh = std::max(xh, box.right());
        yh = std::max(yh, box.top());
      }
    }
    for (auto& fig : layer2Figs) {
      fig->getBBox(box);
      if (isFirst) {
        xl = box.left();
        yl = box.bottom();
        xh = box.right();
        yh = box.top();
        isFirst = false;
      } else {
        xl = std::min(xl, box.left());
        yl = std::min(yl, box.bottom());
        xh = std::max(xh, box.right());
        yh = std::max(yh, box.top());
      }
    }
    for (auto& fig : cutFigs) {
      fig->getBBox(box);
      if (isFirst) {
        xl = box.left();
        yl = box.bottom();
        xh = box.right();
        yh = box.top();
        isFirst = false;
      } else {
        xl = std::min(xl, box.left());
        yl = std::min(yl, box.bottom());
        xh = std::max(xh, box.right());
        yh = std::max(yh, box.top());
      }
    }
    boxIn.set(xl, yl, xh, yh);
    frTransform xform;
    xform.set(origin_);
    boxIn.transform(xform);
  }
  void move(const frTransform& xform) override { ; }
  bool overlaps(const frBox& box) const override { return false; }

 protected:
  frPoint origin_;
  frViaDef* viaDef_;
  frBlockObject* owner_;
};
}  // namespace fr

#endif
