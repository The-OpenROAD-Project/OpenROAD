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

#ifndef _FR_GUIDE_H_
#define _FR_GUIDE_H_

#include "frBaseTypes.h"
#include "db/obj/frFig.h"

namespace fr {
  class frNet;
  class frGuide: public frConnFig {
  public:
    frGuide(): frConnFig(), begin(), end(), beginLayer(0), endLayer(0), routeObj(), net(nullptr) {}
    frGuide(const frGuide &in): frConnFig(), begin(in.begin), end(in.end), beginLayer(in.beginLayer),
                                endLayer(in.endLayer), routeObj(), net(nullptr) {}
    // getters
    void getPoints(frPoint &beginIn, frPoint &endIn) const {
      beginIn.set(begin);
      endIn.set(end);
    }
    frLayerNum getBeginLayerNum() const {
      return beginLayer;
    }
    frLayerNum getEndLayerNum() const {
      return endLayer;
    }
    bool hasRoutes() const {
      return routeObj.empty() ? false : true;
    }
    const std::vector<std::unique_ptr<frConnFig> >& getRoutes() const {
      return routeObj;
    }
    // setters
    void setPoints(const frPoint &beginIn, const frPoint &endIn) {
      begin.set(beginIn);
      end.set(endIn);
    }
    void setBeginLayerNum (frLayerNum numIn) {
      beginLayer = numIn;
    }
    void setEndLayerNum (frLayerNum numIn) {
      endLayer = numIn;
    }
    void addRoute(std::unique_ptr<frConnFig> cfgIn) {
      routeObj.push_back(std::move(cfgIn));
    }
    void setRoutes(std::vector<std::unique_ptr<frConnFig> > &routesIn) {
      routeObj = std::move(routesIn);
    }
    // others
    frBlockObjectEnum typeId() const override {
      return frcGuide;
    }

    /* from frConnFig
     * hasNet
     * getNet
     * addToNet
     * removeFromNet
     */
    bool hasNet() const override {
      return (net);
    }
    frNet* getNet() const override {
      return net;
    }
    void addToNet(frNet* in) override {
      net = in;
    }
    void removeFromNet() override {
      net = nullptr;
    }

    /* from frFig
     * getBBox
     * move, in .cpp
     * overlaps, incomplete
     */
    // needs to be updated
    void getBBox(frBox &boxIn) const override {
      boxIn.set(begin, end);
    }
    void move(const frTransform &xform) override {
      ;
    }
    bool overlaps(const frBox &box) const override {
      return false;
    }

  protected:
    frPoint begin;
    frPoint end;
    frLayerNum beginLayer;
    frLayerNum endLayer;
    std::vector<std::unique_ptr<frConnFig> > routeObj;
    frNet* net;
  };
}

#endif
