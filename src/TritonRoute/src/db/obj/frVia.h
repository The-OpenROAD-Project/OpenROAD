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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE REGENTS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _FR_VIA_H_
#define _FR_VIA_H_

#include <memory>
#include "db/obj/frRef.h"
#include "db/obj/frShape.h"
#include "db/tech/frViaDef.h"

namespace fr {
  class frNet;
  class drVia;
  class frVia: public frRef {
  public:
    // constructors
    frVia(): viaDef(nullptr), owner(nullptr) {}
    frVia(frViaDef* in): frRef(), origin(), viaDef(in), owner(nullptr) {}
    frVia(const frVia &in): frRef(in), origin(in.origin), viaDef(in.viaDef), owner(in.owner) {}
    frVia(const drVia &in);
    // getters
    frViaDef* getViaDef() const {
      return viaDef;
    }
    void getLayer1BBox(frBox &boxIn) const {
      auto &figs = viaDef->getLayer1Figs();
      bool isFirst = true;
      frBox box;
      frCoord xl = 0;
      frCoord yl = 0;
      frCoord xh = 0;
      frCoord yh = 0;
      for (auto &fig: figs) {
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
      boxIn.set(xl,yl,xh,yh);
      frTransform xform;
      xform.set(origin);
      boxIn.transform(xform);
    }
    void getCutBBox(frBox &boxIn) const {
      auto &figs = viaDef->getCutFigs();
      bool isFirst = true;
      frBox box;
      frCoord xl = 0;
      frCoord yl = 0;
      frCoord xh = 0;
      frCoord yh = 0;
      for (auto &fig: figs) {
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
      boxIn.set(xl,yl,xh,yh);
      frTransform xform;
      xform.set(origin);
      boxIn.transform(xform);
    }
    void getLayer2BBox(frBox &boxIn) const {
      auto &figs = viaDef->getLayer2Figs();
      bool isFirst = true;
      frBox box;
      frCoord xl = 0;
      frCoord yl = 0;
      frCoord xh = 0;
      frCoord yh = 0;
      for (auto &fig: figs) {
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
      boxIn.set(xl,yl,xh,yh);
      frTransform xform;
      xform.set(origin);
      boxIn.transform(xform);
    }
    // setters
    void setViaDef(frViaDef* in) {
      viaDef = in;
    }
    // others
    frBlockObjectEnum typeId() const override {
      return frcVia;
    }

    /* from frRef
     * getOrient
     * setOrient
     * getOrigin
     * setOrigin
     * getTransform
     * setTransform
     */

    frOrient getOrient() const override {
      return frOrient();
    }
    void setOrient(const frOrient &tmpOrient) override {
      ;
    }
    void getOrigin(frPoint &tmpOrigin) const override {
      tmpOrigin.set(origin);
    }
    void setOrigin(const frPoint &tmpPoint) override {
      origin.set(tmpPoint);
    }
    void getTransform(frTransform &xformIn) const override {
      xformIn.set(origin);
    }
    void setTransform(const frTransform &xformIn) override {}

    /* from frPinFig
     * hasPin
     * getPin
     * addToPin
     * removeFromPin
     */
    bool hasPin() const override {
      return (owner) && (owner->typeId() == frcPin);
    }
    frPin* getPin() const override {
      return reinterpret_cast<frPin*>(owner);
    }
    void addToPin(frPin* in) override {
      owner = reinterpret_cast<frBlockObject*>(in);
    }
    void removeFromPin() override {
      owner = nullptr;
    }

    /* from frConnFig
     * hasNet
     * getNet
     * addToNet
     * removeFromNet
     */
    bool hasNet() const override {
      return (owner) && (owner->typeId() == frcNet);
    }
    frNet* getNet() const override {
      return reinterpret_cast<frNet*>(owner);
    }
    void addToNet(frNet* in) override {
      owner = reinterpret_cast<frBlockObject*>(in);
    }
    void removeFromNet() override {
      owner = nullptr;
    }

    /* from frFig
     * getBBox
     * move
     * overlaps
     */

    void getBBox (frBox &boxIn) const override {
      auto &layer1Figs = viaDef->getLayer1Figs();
      auto &layer2Figs = viaDef->getLayer2Figs();
      auto &cutFigs    = viaDef->getCutFigs();
      bool isFirst = true;
      frBox box;
      frCoord xl = 0;
      frCoord yl = 0;
      frCoord xh = 0;
      frCoord yh = 0;
      for (auto &fig: layer1Figs) {
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
      for (auto &fig: layer2Figs) {
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
      for (auto &fig: cutFigs) {
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
      boxIn.set(xl,yl,xh,yh);
      frTransform xform;
      xform.set(origin);
      boxIn.transform(xform);
    }
    void move(const frTransform &xform) override {
      ;
    }
    bool overlaps(const frBox &box) const override {
      return false;
    }
    
    void setIter(frListIter<std::unique_ptr<frVia> >& in) {
      iter = in;
    }
    frListIter<std::unique_ptr<frVia> > getIter() const {
      return iter;
    }

  protected:
    frPoint        origin;
    frViaDef*      viaDef;
    frBlockObject* owner;
    frListIter<std::unique_ptr<frVia> > iter;
  };
}

#endif
