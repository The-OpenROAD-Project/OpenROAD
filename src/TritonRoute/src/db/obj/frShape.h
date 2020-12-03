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

#ifndef _FR_SHAPE_H_
#define _FR_SHAPE_H_

#include "db/obj/frFig.h"
#include "db/infra/frSegStyle.h"


namespace fr {
  class frNet;
  class frPin;
  class drPathSeg;
  class taPathSeg;
  class drPatchWire;
  class frShape: public frPinFig {
  public:
    // setters
    virtual void setLayerNum (frLayerNum tmpLayerNum) = 0;
    // getters
    virtual frLayerNum getLayerNum() const = 0;
    // others

    /* from frPinFig
     * hasPin
     * getPin
     * addToPin
     * removeFromPin
     */

    /* from frConnFig
     * hasNet
     * getNet
     * addToNet
     * removeFromNet
     */

    /* from frFig
     * getBBox
     * move
     * overlaps
     */

    virtual void setIter(frListIter<std::unique_ptr<frShape> > &in) = 0;
    virtual frListIter<std::unique_ptr<frShape> > getIter() const = 0;
  protected:
    // constructors
    frShape(): frPinFig() {}
  };

  class frRect: public frShape {
  public:
    // constructors
    frRect(): frShape(), box(), layer(0), owner(nullptr) {}
    frRect(const frRect &in): frShape(in), box(in.box), layer(in.layer), owner(in.owner) {}
    // setters
    void setBBox (const frBox &boxIn) {
      box.set(boxIn);
    }
    // getters
    // others
    bool isHor() const {
      frCoord xSpan = box.right() - box.left();
      frCoord ySpan = box.top()   - box.bottom();
      return (xSpan >= ySpan) ? true : false;
    }
    frCoord width() const {
      frCoord xSpan = box.right() - box.left();
      frCoord ySpan = box.top()   - box.bottom();
      return (xSpan > ySpan) ? ySpan : xSpan;
    }
    frCoord length() const {
      frCoord xSpan = box.right() - box.left();
      frCoord ySpan = box.top()   - box.bottom();
      return (xSpan < ySpan) ? ySpan : xSpan;
    }    
    frBlockObjectEnum typeId() const override {
      return frcRect;
    }

    /* from frShape
     * setLayerNum
     * getLayerNum
     */
    void setLayerNum (frLayerNum numIn) override {
      layer = numIn;
    }
    frLayerNum getLayerNum() const override {
      return layer;
    }
    
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
     * move, in .cpp
     * overlaps in .cpp
     */
    void getBBox (frBox &boxIn) const override {
      boxIn.set(box);
    }
    void move(const frTransform &xform) override {
      box.transform(xform);
    }
    bool overlaps(const frBox &box) const override {
      frBox rectBox;
      getBBox(rectBox);
      return rectBox.overlaps(box);
    }
    
    void setIter(frListIter<std::unique_ptr<frShape> >& in) override {
      iter = in;
    }
    frListIter<std::unique_ptr<frShape> > getIter() const override {
      return iter;
    }

  protected:
    frBox          box;
    frLayerNum     layer;
    frBlockObject* owner; // general back pointer 0
    frListIter<std::unique_ptr<frShape> > iter;
  };

  class frPatchWire: public frShape {
  public:
    // constructors
    frPatchWire(): frShape(), offsetBox(), origin(), layer(0), owner(nullptr) {}
    frPatchWire(const frPatchWire &in): frShape(), offsetBox(in.offsetBox), 
                                        origin(in.origin), layer(in.layer), owner(in.owner) {}
    frPatchWire(const drPatchWire &in);
    // setters
    void setOffsetBox (const frBox &in) {
      offsetBox.set(in);
    }
    void setOrigin(const frPoint &in) {
      origin.set(in);
    }
    // getters
    // others
    frBlockObjectEnum typeId() const override {
      return frcPatchWire;
    }

    /* from frShape
     * setLayerNum
     * getLayerNum
     */
    void setLayerNum (frLayerNum numIn) override {
      layer = numIn;
    }
    frLayerNum getLayerNum() const override {
      return layer;
    }
    
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
     * move, in .cpp
     * overlaps in .cpp
     */
    void getBBox (frBox &boxIn) const override {
      frTransform xform(origin);
      boxIn.set(offsetBox);
      boxIn.transform(xform);
    }
    void getOffsetBox (frBox &boxIn) const {
      boxIn.set(offsetBox);
    }
    void getOrigin(frPoint &in) const {
      in.set(origin);
    }
    void move(const frTransform &xform) override {
    }
    bool overlaps(const frBox &box) const override {
      frBox rectBox;
      getBBox(rectBox);
      return rectBox.overlaps(box);
    }
    
    void setIter(frListIter<std::unique_ptr<frShape> >& in) override {
      iter = in;
    }
    frListIter<std::unique_ptr<frShape> > getIter() const override {
      return iter;
    }

  protected:
    // frBox          box;
    frBox          offsetBox;
    frPoint        origin;
    frLayerNum     layer;
    frBlockObject* owner; // general back pointer 0
    frListIter<std::unique_ptr<frShape> > iter;
  };

  class frPolygon: public frShape {
  public:
    // constructors
    frPolygon(): frShape(), points(), layer(0), owner(nullptr) {}
    frPolygon(const frPolygon &in): frShape(), points(in.points), layer(in.layer), owner(in.owner) {}
    // setters
    void setPoints (const std::vector<frPoint> &pointsIn) {
      points = pointsIn;
    }
    // getters
    const std::vector<frPoint>& getPoints() const {
      return points;
    }
    // others
    frBlockObjectEnum typeId() const override {
      return frcPolygon;
    }

    /* from frShape
     * setLayerNum
     * getLayerNum
     */
    void setLayerNum (frLayerNum numIn) override {
      layer = numIn;
    }
    frLayerNum getLayerNum() const override {
      return layer;
    }
    
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
     * move, in .cpp
     * overlaps, in .cpp
     */
    void getBBox (frBox &boxIn) const override {
      frCoord llx = 0;
      frCoord lly = 0;
      frCoord urx = 0;
      frCoord ury = 0;
      if (points.size()) {
        llx = points.begin()->x();
        urx = points.begin()->x();
        lly = points.begin()->y();
        ury = points.begin()->y();
      }
      for (auto &point: points) {
        llx = (llx < point.x()) ? llx : point.x();
        lly = (lly < point.y()) ? lly : point.y();
        urx = (urx > point.x()) ? urx : point.x();
        ury = (ury > point.y()) ? ury : point.y();
      }
      boxIn.set(llx, lly, urx, ury);
    }
    void move(const frTransform &xform) override {
      for (auto &point: points) {
        point.transform(xform);
      }
    }
    bool overlaps(const frBox &box) const override {
      return false;
    }
    
    void setIter(frListIter<std::unique_ptr<frShape> >& in) override {
      iter = in;
    }
    frListIter<std::unique_ptr<frShape> > getIter() const override {
      return iter;
    }

  protected:
    std::vector<frPoint> points;
    frLayerNum           layer;
    frBlockObject*       owner;
    frListIter<std::unique_ptr<frShape> > iter;
  };


  class frPathSeg: public frShape {
  public:
    // constructors
    frPathSeg(): frShape(), begin(), end(), layer(0), style(), owner(nullptr) {}
    frPathSeg(const frPathSeg &in): begin(in.begin), end(in.end), layer(in.layer), style(in.style), owner(in.owner) {}
    frPathSeg(const drPathSeg &in);
    frPathSeg(const taPathSeg &in);
    // getters
    void getPoints(frPoint &beginIn, frPoint &endIn) const {
      beginIn.set(begin);
      endIn.set(end);
    }
    void getStyle(frSegStyle &styleIn) const {
      styleIn.setBeginStyle(style.getBeginStyle(), style.getBeginExt());
      styleIn.setEndStyle(style.getEndStyle(), style.getEndExt());
      styleIn.setWidth(style.getWidth());
    }
    // setters
    void setPoints(const frPoint &beginIn, const frPoint &endIn) {
      begin.set(beginIn);
      end.set(endIn);
    }
    void setStyle(const frSegStyle &styleIn) {
      style.setBeginStyle(styleIn.getBeginStyle(), styleIn.getBeginExt());
      style.setEndStyle(styleIn.getEndStyle(), styleIn.getEndExt());
      style.setWidth(styleIn.getWidth());
    }
    // others
    frBlockObjectEnum typeId() const override {
      return frcPathSeg;
    }

    /* from frShape
     * setLayerNum
     * getLayerNum
     */
    void setLayerNum (frLayerNum numIn) override {
      layer = numIn;
    }
    frLayerNum getLayerNum() const override {
      return layer;
    }
    
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
     * move, in .cpp
     * overlaps, in .cpp
     */
    // needs to be updated
    void getBBox (frBox &boxIn) const override {
      bool isHorizontal = true;
      if (begin.x() == end.x()) {
        isHorizontal = false;
      }
      auto width    = style.getWidth();
      auto beginExt = style.getBeginExt();
      auto endExt   = style.getEndExt();
      if (isHorizontal) {
        boxIn.set(begin.x() - beginExt, begin.y() - width / 2,
                  end.x()   + endExt,   end.y()   + width / 2);
      } else {
        boxIn.set(begin.x() - width / 2, begin.y() - beginExt,
                  end.x()   + width / 2, end.y()   + endExt);
      }
    }
    void move(const frTransform &xform) override {
      begin.transform(xform);
      end.transform(xform);
    }
    bool overlaps(const frBox &box) const override {
      return false;
    }

    void setIter(frListIter<std::unique_ptr<frShape> >& in) override {
      iter = in;
    }
    frListIter<std::unique_ptr<frShape> > getIter() const override {
      return iter;
    }
  protected:
    frPoint        begin; // begin always smaller than end, assumed
    frPoint        end;
    frLayerNum     layer;
    frSegStyle     style;
    frBlockObject* owner;
    frListIter<std::unique_ptr<frShape> > iter;
  };
}

#endif
