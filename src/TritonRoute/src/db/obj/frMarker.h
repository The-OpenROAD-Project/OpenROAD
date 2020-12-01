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

#ifndef _FR_MARKER_H_
#define _FR_MARKER_H_

#include "db/obj/frFig.h"
#include <tuple>
#include <set>

namespace fr {
  class frConstraint;
  class frMarker: public frFig {
  public:
    // constructors
    frMarker(): frFig(), constraint(nullptr), bbox(), layerNum(0), srcs(), iter(), vioHasDir(false), vioIsH(false) {}
    frMarker(const frMarker &in): constraint(in.constraint), bbox(in.bbox), layerNum(in.layerNum),
                                  srcs(in.srcs), iter(), vioHasDir(in.vioHasDir), vioIsH(in.vioIsH) {}
    // setters
    void setConstraint(frConstraint* constraintIn) {
      constraint = constraintIn;
    }

    void setBBox(const frBox &bboxIn) {
      bbox = bboxIn;
    }

    void setLayerNum(const frLayerNum &layerNumIn) {
      layerNum = layerNumIn;
    }

    void setHasDir(const bool &in) {
      vioHasDir = in;
    }

    void setIsH(const bool &in) {
      vioIsH = in;
    }

    void addSrc(frBlockObject *srcIn) {
      srcs.insert(srcIn);
    }
    void addAggressor(frBlockObject *obj, const std::tuple<frLayerNum, frBox, bool> &tupleIn) {
      aggressors.push_back(std::make_pair(obj, tupleIn));
    }
    void addVictim(frBlockObject *obj, const std::tuple<frLayerNum, frBox, bool> &tupleIn) {
      victims.push_back(std::make_pair(obj, tupleIn));
    }
    // getters

    /* from frFig
     * getBBox
     * move, in .cpp
     * overlaps in .cpp
     */

    void getBBox(frBox &bboxIn) const override {
      bboxIn.set(bbox);
    }

    frLayerNum getLayerNum() const {
      return layerNum;
    }

    const std::set<frBlockObject*>& getSrcs() const {
      return srcs;
    }
    
    const std::vector<std::pair<frBlockObject*, std::tuple<frLayerNum, frBox, bool> > >& getAggressors() const {
      return aggressors;
    }

    const std::vector<std::pair<frBlockObject*, std::tuple<frLayerNum, frBox, bool> > >& getVictims() const {
      return victims;
    }

    frConstraint* getConstraint() const {
      return constraint;
    }

    bool hasDir() const {
      return vioHasDir;
    }

    bool isH() const {
      return vioIsH;
    }


    void move(const frTransform &xform) override {
      
    }

    bool overlaps(const frBox &box) const override {
      return false;
    }

    // others
    frBlockObjectEnum typeId() const override {
      return frcMarker;
    }

    void setIter(frListIter<std::unique_ptr<frMarker> >& in) {
      iter = in;
    }
    frListIter<std::unique_ptr<frMarker> > getIter() const {
      return iter;
    }

  protected:
    frConstraint* constraint;
    frBox bbox;
    frLayerNum layerNum;
    std::set<frBlockObject*> srcs;
    std::vector<std::pair<frBlockObject*,std::tuple<frLayerNum, frBox, bool> > > victims; // obj, isFixed
    std::vector<std::pair<frBlockObject*, std::tuple<frLayerNum, frBox, bool> > > aggressors; // obj, isFixed
    frListIter<std::unique_ptr<frMarker> > iter;
    bool vioHasDir, vioIsH;
  };
}


#endif
