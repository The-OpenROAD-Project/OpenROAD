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

#ifndef _DR_NET_H_
#define _DR_NET_H_

#include <memory>
#include "db/drObj/drBlockObject.h"
#include "db/drObj/drPin.h"
#include "db/drObj/drShape.h"
#include "db/drObj/drVia.h"
#include <set>

namespace fr {
  class frNet;
  class drNet: public drBlockObject {
  public:
    // constructors
    drNet(): drBlockObject(), pins(), extConnFigs(), routeConnFigs(), bestRouteConnFigs(),
             fNetTerms(), fNet(nullptr), modified(false), numMarkers(0), numPinsIn(0), 
             markerDist(std::numeric_limits<frCoord>::max()), allowRipup(true), pinBox(), ripup(false),
             numReroutes(0), inQueue(false), routed(false), origGuides() {}
    // getters
    const std::vector<std::unique_ptr<drPin> >& getPins() const {
      return pins;
    }
    const std::vector<std::unique_ptr<drConnFig> >& getExtConnFigs() const {
      return extConnFigs;
    }
    const std::vector<std::unique_ptr<drConnFig> >& getRouteConnFigs() const {
      return routeConnFigs;
    }
    const std::vector<std::unique_ptr<drConnFig> >& getBestRouteConnFigs() const {
      return bestRouteConnFigs;
    }
    frNet* getFrNet() const {
      return fNet;
    }
    const std::set<frBlockObject*>& getFrNetTerms() const {
      return fNetTerms;
    }
    bool isModified() const {
      return modified;
    }
    int getNumMarkers() const {
      return numMarkers;
    }
    int getNumPinsIn() const {
      return numPinsIn;
    }
    bool hasMarkerDist() const {
      return (markerDist == -1);
    }
    frCoord getMarkerDist() const {
      return markerDist;
    }
    void getPinBox(frBox &in) {
      in.set(pinBox);
    }
    bool isRipup() const {
      return allowRipup ? ripup : false;
    }
    int getNumReroutes() const {
      return numReroutes;
    }
    bool isInQueue() const {
      return inQueue;
    }
    bool isRouted() const {
      return routed;
    }
    const std::vector<frRect>& getOrigGuides() const {
      return origGuides;
    }

    // setters
    void addPin(std::unique_ptr<drPin> pinIn) {
      pinIn->setNet(this);
      //pinIn->setId(pins.size());
      pins.push_back(std::move(pinIn));
    }
    void addRoute(std::unique_ptr<drConnFig> in, bool isExt = false) {
      in->addToNet(this);
      if (isExt) {
        extConnFigs.push_back(std::move(in));
      } else {
        routeConnFigs.push_back(std::move(in));
      }
    }
    void setBestRouteConnFigs() {
      bestRouteConnFigs.clear();
      for (auto &uConnFig: routeConnFigs) {
        if (uConnFig->typeId() == drcPathSeg) {
          std::unique_ptr<drConnFig> uPtr = std::make_unique<drPathSeg>(*static_cast<drPathSeg*>(uConnFig.get()));
          bestRouteConnFigs.push_back(std::move(uPtr));
        } else if (uConnFig->typeId() == drcVia) {
          std::unique_ptr<drConnFig> uPtr = std::make_unique<drVia>(*static_cast<drVia*>(uConnFig.get()));
          bestRouteConnFigs.push_back(std::move(uPtr));
        } else if (uConnFig->typeId() == drcPatchWire) {
          std::unique_ptr<drConnFig> uPtr = std::make_unique<drPatchWire>(*static_cast<drPatchWire*>(uConnFig.get()));
          bestRouteConnFigs.push_back(std::move(uPtr));
        }
      }
    }
    void clear() {
      routeConnFigs.clear();
      modified = true;
      numMarkers = 0;
      routed = false;
    }
    void setFrNet(frNet* in) {
      fNet = in;
    }
    void setFrNetTerms(const std::set<frBlockObject*> &in) {
      fNetTerms = in;
    }
    void setModified(bool in) {
      modified = in;
    }

    void setNumMarkers(int in) {
      numMarkers = in;
    }
    void addMarker() {
      numMarkers++;
    }
    void setNumPinsIn(int in) {
      numPinsIn = in;
    }
    void updateMarkerDist(frCoord in) {
      markerDist = std::min(markerDist, in);
    }
    void resetMarkerDist() {
      markerDist = std::numeric_limits<frCoord>::max();
    }
    void setPinBox(const frBox &in) {
      pinBox.set(in);
    }
    void setRipup() {
      ripup = true;
    }
    void resetRipup() {
      ripup = false;
    }
    void setAllowRipup(bool in) {
      allowRipup = in;
    }
    void addNumReroutes() {
      numReroutes++;
    }
    void resetNumReroutes() {
      numReroutes = 0;
    }
    void setInQueue() {
      inQueue = true;
    }
    void resetInQueue() {
      inQueue = false;
    }
    void setRouted() {
      routed = true;
    }
    void resetRouted() {
      routed = false;
    }
    void setOrigGuides(std::vector<frRect> &in) {
      origGuides.assign(in.begin(), in.end());
    }
    void cleanup() {
      pins.clear();
      pins.shrink_to_fit();
      extConnFigs.clear();
      extConnFigs.shrink_to_fit();
      routeConnFigs.clear();
      routeConnFigs.shrink_to_fit();
      fNetTerms.clear();
      origGuides.clear();
      origGuides.shrink_to_fit();
    }

    // others
    frBlockObjectEnum typeId() const override {
      return drcNet;
    }

    bool operator< (const drNet &b) const {
      return (numMarkers == b.numMarkers) ? (getId() < b.getId()) : (numMarkers > b.numMarkers);
    }

  protected:
    std::vector<std::unique_ptr<drPin> >         pins;
    std::vector<std::unique_ptr<drConnFig> >     extConnFigs;
    std::vector<std::unique_ptr<drConnFig> >     routeConnFigs;
    std::vector<std::unique_ptr<drConnFig> >     bestRouteConnFigs;
    std::set<frBlockObject*>                     fNetTerms;
    frNet*                                       fNet;
    // old
    bool                                         modified;
    int                                          numMarkers;
    int                                          numPinsIn;
    frCoord                                      markerDist;
    bool                                         allowRipup;
    frBox                                        pinBox;
    bool                                         ripup;
    // new
    int                                          numReroutes;
    bool                                         inQueue;
    bool                                         routed;

    std::vector<frRect>                          origGuides;
  };
}

#endif
