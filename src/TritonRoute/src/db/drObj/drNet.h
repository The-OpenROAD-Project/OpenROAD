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

#ifndef _DR_NET_H_
#define _DR_NET_H_

#include <memory>
#include <set>

#include "db/drObj/drBlockObject.h"
#include "db/drObj/drPin.h"
#include "db/drObj/drShape.h"
#include "db/drObj/drVia.h"

namespace fr {
class frNet;
class drNet : public drBlockObject
{
 public:
  // constructors
  drNet()
      : drBlockObject(),
        pins_(),
        extConnFigs_(),
        routeConnFigs_(),
        bestRouteConnFigs_(),
        fNetTerms_(),
        fNet_(nullptr),
        modified_(false),
        numMarkers_(0),
        numPinsIn_(0),
        markerDist_(std::numeric_limits<frCoord>::max()),
        allowRipup_(true),
        pinBox_(),
        ripup_(false),
        numReroutes_(0),
        ndrRipupThresh_(0),
        inQueue_(false),
        routed_(false),
        origGuides_()
  {
  }
  // getters
  const std::vector<std::unique_ptr<drPin>>& getPins() const { return pins_; }
  const std::vector<std::unique_ptr<drConnFig>>& getExtConnFigs() const
  {
    return extConnFigs_;
  }
  const std::vector<std::unique_ptr<drConnFig>>& getRouteConnFigs() const
  {
    return routeConnFigs_;
  }
  const std::vector<std::unique_ptr<drConnFig>>& getBestRouteConnFigs() const
  {
    return bestRouteConnFigs_;
  }
  frNet* getFrNet() const { return fNet_; }
  const std::set<frBlockObject*>& getFrNetTerms() const { return fNetTerms_; }
  bool isModified() const { return modified_; }
  int getNumMarkers() const { return numMarkers_; }
  int getNumPinsIn() const { return numPinsIn_; }
  bool hasMarkerDist() const { return (markerDist_ == -1); }
  frCoord getMarkerDist() const { return markerDist_; }
  void getPinBox(frBox& in) { in.set(pinBox_); }
  bool isRipup() const { return allowRipup_ ? ripup_ : false; }
  int getNumReroutes() const { return numReroutes_; }
  bool isInQueue() const { return inQueue_; }
  bool isRouted() const { return routed_; }
  const std::vector<frRect>& getOrigGuides() const { return origGuides_; }

  // setters
  void addPin(std::unique_ptr<drPin> pinIn)
  {
    pinIn->setNet(this);
    // pinIn->setId(pins.size());
    pins_.push_back(std::move(pinIn));
  }
  void addRoute(std::unique_ptr<drConnFig> in, bool isExt = false)
  {
    in->addToNet(this);
    if (isExt) {
      extConnFigs_.push_back(std::move(in));
    } else {
      routeConnFigs_.push_back(std::move(in));
    }
  }
  void setBestRouteConnFigs()
  {
    bestRouteConnFigs_.clear();
    for (auto& uConnFig : routeConnFigs_) {
      if (uConnFig->typeId() == drcPathSeg) {
        std::unique_ptr<drConnFig> uPtr = std::make_unique<drPathSeg>(
            *static_cast<drPathSeg*>(uConnFig.get()));
        bestRouteConnFigs_.push_back(std::move(uPtr));
      } else if (uConnFig->typeId() == drcVia) {
        std::unique_ptr<drConnFig> uPtr
            = std::make_unique<drVia>(*static_cast<drVia*>(uConnFig.get()));
        bestRouteConnFigs_.push_back(std::move(uPtr));
      } else if (uConnFig->typeId() == drcPatchWire) {
        std::unique_ptr<drConnFig> uPtr = std::make_unique<drPatchWire>(
            *static_cast<drPatchWire*>(uConnFig.get()));
        bestRouteConnFigs_.push_back(std::move(uPtr));
      }
    }
  }
  void clear()
  {
    routeConnFigs_.clear();
    modified_ = true;
    numMarkers_ = 0;
    routed_ = false;
  }
  void setFrNet(frNet* in) { fNet_ = in; }
  void setFrNetTerms(const std::set<frBlockObject*>& in) { fNetTerms_ = in; }
  void setModified(bool in) { modified_ = in; }

  void setNumMarkers(int in) { numMarkers_ = in; }
  void addMarker() { numMarkers_++; }
  void setNumPinsIn(int in) { numPinsIn_ = in; }
  void updateMarkerDist(frCoord in) { markerDist_ = std::min(markerDist_, in); }
  void resetMarkerDist() { markerDist_ = std::numeric_limits<frCoord>::max(); }
  void setPinBox(const frBox& in) { pinBox_.set(in); }
  void setRipup() { ripup_ = true; }
  void resetRipup() { ripup_ = false; }
  void setAllowRipup(bool in) { allowRipup_ = in; }
  void addNumReroutes() { numReroutes_++; }
  void resetNumReroutes() { numReroutes_ = 0; }
  void setInQueue() { inQueue_ = true; }
  void resetInQueue() { inQueue_ = false; }
  void setRouted() { routed_ = true; }
  void resetRouted() { routed_ = false; }
  void setOrigGuides(std::vector<frRect>& in)
  {
    origGuides_.assign(in.begin(), in.end());
  }
  void cleanup()
  {
    pins_.clear();
    pins_.shrink_to_fit();
    extConnFigs_.clear();
    extConnFigs_.shrink_to_fit();
    routeConnFigs_.clear();
    routeConnFigs_.shrink_to_fit();
    fNetTerms_.clear();
    origGuides_.clear();
    origGuides_.shrink_to_fit();
  }
  int getNdrRipupThresh() { return ndrRipupThresh_; }
  void setNdrRipupThresh(int n) { ndrRipupThresh_ = n; }
  void incNdrRipupThresh();
  bool hasNDR();
  // others
  frBlockObjectEnum typeId() const override { return drcNet; }

  bool operator<(const drNet& b) const
  {
    return (numMarkers_ == b.numMarkers_) ? (getId() < b.getId())
                                          : (numMarkers_ > b.numMarkers_);
  }

  bool hasAccessPoint(const frPoint& pt, frLayerNum lNum) {
      return getAccessPoint(pt, lNum) != nullptr;
  }
  
  frPoint const* getAccessPoint(const frPoint& pt, frLayerNum lNum) {
    for (auto& pin : getPins()) {
        for (auto& ap : pin->getAccessPatterns()) {
            if (ap->getPoint() == pt && ap->getBeginLayerNum() == lNum) 
                return &ap->getPoint();
        }
    }
    return nullptr;
  }
  
 protected:
  std::vector<std::unique_ptr<drPin>> pins_;
  std::vector<std::unique_ptr<drConnFig>> extConnFigs_;
  std::vector<std::unique_ptr<drConnFig>> routeConnFigs_;
  std::vector<std::unique_ptr<drConnFig>> bestRouteConnFigs_;
  std::set<frBlockObject*> fNetTerms_;
  frNet* fNet_;
  // old
  bool modified_;
  int numMarkers_;
  int numPinsIn_;
  frCoord markerDist_;
  bool allowRipup_;
  frBox pinBox_;
  bool ripup_;
  // new
  int numReroutes_;
  int ndrRipupThresh_;
  bool inQueue_;
  bool routed_;

  std::vector<frRect> origGuides_;
};
}  // namespace fr

#endif
