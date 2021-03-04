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

#include "ta/FlexTA.h"

using namespace std;
using namespace fr;

void FlexTAWorker::initTracks() {
  //bool enableOutput = true;
  bool enableOutput = false;
  trackLocs_.clear();
  trackLocs_.resize(getDesign()->getTech()->getLayers().size());
  vector<set<frCoord> > trackCoordSets(getDesign()->getTech()->getLayers().size());
  // uPtr for tp
  for (int lNum = 0; lNum < (int)getDesign()->getTech()->getLayers().size(); lNum++) {
    auto layer = getDesign()->getTech()->getLayer(lNum);
    if (layer->getType() != frLayerTypeEnum::ROUTING) {
      continue;
    }
    if (layer->getDir() != getDir()) {
      continue;
    }
    for (auto &tp: getDesign()->getTopBlock()->getTrackPatterns(lNum)) {
      if ((getDir() == frcHorzPrefRoutingDir && tp->isHorizontal() == false) ||
          (getDir() == frcVertPrefRoutingDir && tp->isHorizontal() == true)) {
        if (enableOutput) {
          cout <<"TRACKS " <<(tp->isHorizontal() ? string("X ") : string("Y "))
               <<tp->getStartCoord() <<" DO " <<tp->getNumTracks() <<" STEP "
               <<tp->getTrackSpacing() <<" LAYER " <<tp->getLayerNum() 
               <<" ;" <<endl;
        }
        bool isH = (getDir() == frcHorzPrefRoutingDir);
        frCoord tempCoord1 = (isH ? getRouteBox().bottom() : getRouteBox().left());
        frCoord tempCoord2 = (isH ? getRouteBox().top()    : getRouteBox().right());
        int trackNum = (tempCoord1 - tp->getStartCoord()) / (int)tp->getTrackSpacing();
        if (trackNum < 0) {
          trackNum = 0;
        }
        if (trackNum * (int)tp->getTrackSpacing() + tp->getStartCoord() < tempCoord1) {
          trackNum++;
        }
        for (; trackNum < (int)tp->getNumTracks() && trackNum * (int)tp->getTrackSpacing() + tp->getStartCoord() < tempCoord2; trackNum++) {
          frCoord trackCoord = trackNum * tp->getTrackSpacing() + tp->getStartCoord();
          //cout <<"TRACKLOC " <<trackCoord * 1.0 / getDesign()->getTopBlock()->getDBUPerUU() <<endl;
          trackCoordSets[lNum].insert(trackCoord);
        }
      }
    }
  }
  for (int i = 0; i < (int)trackCoordSets.size(); i++) {
    if (enableOutput) {
      cout <<"lNum " <<i <<":";
    }
    for (auto coord: trackCoordSets[i]) {
      if (enableOutput) {
        cout <<" " <<coord * 1.0 / getDesign()->getTopBlock()->getDBUPerUU();
      }
      trackLocs_[i].push_back(coord);
    }
    if (enableOutput) {
      cout <<endl;
    }
  }
}

// use prefAp, otherwise return false
bool FlexTAWorker::initIroute_helper_pin(frGuide* guide, frCoord &maxBegin, frCoord &minEnd, 
                                         set<frCoord> &downViaCoordSet, set<frCoord> &upViaCoordSet,
                                         int &wlen, frCoord &wlen2) {
  bool enableOutput = false;
  frPoint bp, ep;
  guide->getPoints(bp, ep);
  if (!(bp == ep)) {
    return false;
  }

  auto net      = guide->getNet();
  auto layerNum = guide->getBeginLayerNum();
  bool isH      = (getDir() == frPrefRoutingDirEnum::frcHorzPrefRoutingDir);
  bool hasDown  = false;
  bool hasUp    = false;

  vector<frGuide*> nbrGuides;
  auto rq = getRegionQuery();
  frBox box;
  box.set(bp, bp);
  nbrGuides.clear();
  if (layerNum - 2 >= BOTTOM_ROUTING_LAYER) {
    rq->queryGuide(box, layerNum - 2, nbrGuides);
    for (auto &nbrGuide: nbrGuides) {
      if (nbrGuide->getNet() == net) {
        hasDown = true;
        break;
      }
    }
  } 
  nbrGuides.clear();
  if (layerNum + 2 < (int) design_->getTech()->getLayers().size()) {
    rq->queryGuide(box, layerNum + 2, nbrGuides);
    for (auto &nbrGuide: nbrGuides) {
      if (nbrGuide->getNet() == net) {
        hasUp = true;
        break;
      }
    }
  } 

  vector<frBlockObject*> result;
  box.set(bp, bp);
  rq->queryGRPin(box, result);
  frTransform instXform; // (0,0), frcR0
  frTransform shiftXform;
  frTerm* trueTerm = nullptr;
  for (auto &term: result) {
    frInst* inst = nullptr;
    if (term->typeId() == frcInstTerm) {
      if (static_cast<frInstTerm*>(term)->getNet() != net) {
        continue;
      }
      inst = static_cast<frInstTerm*>(term)->getInst();
      inst->getTransform(shiftXform);
      shiftXform.set(frOrient(frcR0));
      inst->getUpdatedXform(instXform);
      trueTerm = static_cast<frInstTerm*>(term)->getTerm();
    } else if (term->typeId() == frcTerm) {
      if (static_cast<frTerm*>(term)->getNet() != net) {
        continue;
      }
      trueTerm = static_cast<frTerm*>(term);
    }
    if (trueTerm) {
      int pinIdx = 0;
      int pinAccessIdx = (inst) ? inst->getPinAccessIdx() : -1;
      for (auto &pin: trueTerm->getPins()) {
        frAccessPoint* ap = nullptr;
        if (inst) {
          ap = (static_cast<frInstTerm*>(term)->getAccessPoints())[pinIdx];
        }
        if (!pin->hasPinAccess()) {
          continue;
        }
        if (pinAccessIdx == -1) {
          continue;
        }
        if (ap == nullptr) {
          continue;
        }
        frPoint apBp;
        ap->getPoint(apBp);
        if (enableOutput) {
          cout <<" (" <<apBp.x() * 1.0 / getDesign()->getTopBlock()->getDBUPerUU() <<", "
                      <<apBp.y() * 1.0 / getDesign()->getTopBlock()->getDBUPerUU() <<") origin";
        }
        auto bNum = ap->getLayerNum();
        apBp.transform(shiftXform);
        if (layerNum == bNum && getRouteBox().contains(apBp)) {
          wlen2 = isH ? apBp.y() : apBp.x();
          maxBegin = isH ? apBp.x() : apBp.y();
          minEnd   = isH ? apBp.x() : apBp.y();
          wlen = 0;
          if (hasDown) {
            downViaCoordSet.insert(maxBegin);
          }
          if (hasUp) {
            upViaCoordSet.insert(maxBegin);
          }
          return true;
        }
        pinIdx++;
      }
    }
  }
  
  return false;
}

void FlexTAWorker::initIroute_helper(frGuide* guide, frCoord &maxBegin, frCoord &minEnd, 
                                     set<frCoord> &downViaCoordSet, set<frCoord> &upViaCoordSet,
                                     int &wlen, frCoord &wlen2) {
  if (!initIroute_helper_pin(guide, maxBegin, minEnd, downViaCoordSet, upViaCoordSet, wlen, wlen2)) {
    initIroute_helper_generic(guide, maxBegin, minEnd, downViaCoordSet, upViaCoordSet, wlen, wlen2);
  }
}


void FlexTAWorker::initIroute_helper_generic_helper(frGuide* guide, frCoord &wlen2) {
  bool enableOutput = false;

  frPoint bp, ep;
  guide->getPoints(bp, ep);
  auto net = guide->getNet();
  bool isH = (getDir() == frPrefRoutingDirEnum::frcHorzPrefRoutingDir);

  auto rq = getRegionQuery();
  vector<frBlockObject*> result;

  frBox box;
  box.set(bp, bp);
  rq->queryGRPin(box, result);
  if (!(ep == bp)) {
    box.set(ep, ep);
    rq->queryGRPin(box, result);
  }
  frTransform instXform; // (0,0), frcR0
  frTransform shiftXform;
  frTerm* trueTerm = nullptr;
  for (auto &term: result) {
    frInst* inst = nullptr;
    if (term->typeId() == frcInstTerm) {
      if (static_cast<frInstTerm*>(term)->getNet() != net) {
        continue;
      }
      inst = static_cast<frInstTerm*>(term)->getInst();
      inst->getTransform(shiftXform);
      shiftXform.set(frOrient(frcR0));
      inst->getUpdatedXform(instXform);
      trueTerm = static_cast<frInstTerm*>(term)->getTerm();
    } else if (term->typeId() == frcTerm) {
      if (static_cast<frTerm*>(term)->getNet() != net) {
        continue;
      }
      trueTerm = static_cast<frTerm*>(term);
    }
    if (trueTerm) {
      int pinIdx = 0;
      int pinAccessIdx = (inst) ? inst->getPinAccessIdx() : -1;
      for (auto &pin: trueTerm->getPins()) {
        frAccessPoint* ap = nullptr;
        if (inst) {
          ap = (static_cast<frInstTerm*>(term)->getAccessPoints())[pinIdx];
        }
        if (!pin->hasPinAccess()) {
          continue;
        }
        if (pinAccessIdx == -1) {
          continue;
        }
        if (ap == nullptr) {
          continue;
        }
        frPoint apBp;
        ap->getPoint(apBp);
        if (enableOutput) {
          cout <<" (" <<apBp.x() * 1.0 / getDesign()->getTopBlock()->getDBUPerUU() <<", "
                      <<apBp.y() * 1.0 / getDesign()->getTopBlock()->getDBUPerUU() <<") origin";
        }
        apBp.transform(shiftXform);
        if (getRouteBox().contains(apBp)) {
          wlen2 = isH ? apBp.y() : apBp.x();
          return;
        }
        pinIdx++;
      }
    }
    ; // to do @@@@@
    wlen2 = 0;
  }
}

void FlexTAWorker::initIroute_helper_generic(frGuide* guide, frCoord &minBegin, frCoord &maxEnd, 
                                             set<frCoord> &downViaCoordSet, set<frCoord> &upViaCoordSet,
                                             int &wlen, frCoord &wlen2) {
  auto    net         = guide->getNet();
  auto    layerNum    = guide->getBeginLayerNum();
  bool    hasMinBegin = false;
  bool    hasMaxEnd   = false;
          minBegin    = std::numeric_limits<frCoord>::max();
          maxEnd      = std::numeric_limits<frCoord>::min();
          wlen        = 0;
          //wlen2       = std::numeric_limits<frCoord>::max();
  bool    isH         = (getDir() == frPrefRoutingDirEnum::frcHorzPrefRoutingDir);
  downViaCoordSet.clear();
  upViaCoordSet.clear();
  frPoint nbrBp, nbrEp;
  frPoint nbrSegBegin, nbrSegEnd;
  
  frPoint bp, ep;
  guide->getPoints(bp, ep);
  frPoint cp;
  // layerNum in FlexTAWorker
  vector<frGuide*> nbrGuides;
  auto rq = getRegionQuery();
  frBox box;
  for (int i = 0; i < 2; i++) {
    nbrGuides.clear();
    // check left
    if (i == 0) {
      box.set(bp, bp);
      cp = bp;
    // check right
    } else {
      box.set(ep, ep);
      cp = ep;
    }
    if (layerNum - 2 >= BOTTOM_ROUTING_LAYER) {
      rq->queryGuide(box, layerNum - 2, nbrGuides);
    } 
    if (layerNum + 2 < (int) design_->getTech()->getLayers().size()) {
      rq->queryGuide(box, layerNum + 2, nbrGuides);
    } 
    for (auto &nbrGuide: nbrGuides) {
      if (nbrGuide->getNet() == net) {
        nbrGuide->getPoints(nbrBp, nbrEp);
        if (!nbrGuide->hasRoutes()) {
          // via location assumed in center
          auto psLNum = nbrGuide->getBeginLayerNum();
          if (psLNum == layerNum - 2) {
            downViaCoordSet.insert((isH ? nbrBp.x() : nbrBp.y()));
          } else {
            upViaCoordSet.insert((isH ? nbrBp.x() : nbrBp.y()));
          }
        } else {
          for (auto &connFig: nbrGuide->getRoutes()) {
            if (connFig->typeId() == frcPathSeg) {
              auto obj = static_cast<frPathSeg*>(connFig.get());
              obj->getPoints(nbrSegBegin, nbrSegEnd);
              auto psLNum = obj->getLayerNum();
              if (i == 0) {
                minBegin = min(minBegin, (isH ? nbrSegBegin.x() : nbrSegBegin.y()));
                hasMinBegin = true;
              } else {
                maxEnd = max(maxEnd, (isH ? nbrSegBegin.x() : nbrSegBegin.y()));
                hasMaxEnd = true;
              }
              if (psLNum == layerNum - 2) {
                downViaCoordSet.insert((isH ? nbrSegBegin.x() : nbrSegBegin.y()));
              } else {
                upViaCoordSet.insert((isH ? nbrSegBegin.x() : nbrSegBegin.y()));
              }
            }
          }
        }
        if (cp == nbrEp) {
          wlen -= 1;
        }
        if (cp == nbrBp) {
          wlen += 1;
        }
      }
    }
  }

  if (!hasMinBegin) {
    minBegin = (isH ? bp.x() : bp.y());
  }
  if (!hasMaxEnd) {
    maxEnd = (isH ? ep.x() : ep.y());
  }
  if (minBegin > maxEnd) {
    swap(minBegin, maxEnd);
  }
  if (minBegin == maxEnd) {
    maxEnd += 1;
  }

  // wlen2 purely depends on ap regardless of track
  initIroute_helper_generic_helper(guide, wlen2);
}

void FlexTAWorker::initIroute(frGuide *guide) {
  bool enableOutput = false;
  //bool enableOutput = true;
  auto iroute = make_unique<taPin>();
  iroute->setGuide(guide);
  frBox guideBox;
  guide->getBBox(guideBox);
  auto layerNum = guide->getBeginLayerNum();
  bool isExt = !(getRouteBox().contains(guideBox));
  if (isExt) {
    // extIroute empty, skip
    if (guide->getRoutes().empty()) {
      return;
    }
    if (enableOutput) {
      double dbu = getDesign()->getTopBlock()->getDBUPerUU();
      cout <<"ext@(" <<guideBox.left() / dbu  <<", " <<guideBox.bottom() / dbu <<") ("
                     <<guideBox.right() / dbu <<", " <<guideBox.top()    / dbu <<")" <<endl;
    }
  }
  
  frCoord maxBegin, minEnd;
  set<frCoord> downViaCoordSet, upViaCoordSet;
  int wlen = 0;
  frCoord wlen2 = std::numeric_limits<frCoord>::max();
  initIroute_helper(guide, maxBegin, minEnd, downViaCoordSet, upViaCoordSet, wlen, wlen2);

  frCoord trackLoc = 0;
  frPoint segBegin, segEnd;
  bool    isH         = (getDir() == frPrefRoutingDirEnum::frcHorzPrefRoutingDir);
  // set trackIdx
  if (!isInitTA()) {
    for (auto &connFig: guide->getRoutes()) {
      if (connFig->typeId() == frcPathSeg) {
        auto obj = static_cast<frPathSeg*>(connFig.get());
        obj->getPoints(segBegin, segEnd);
        trackLoc = (isH ? segBegin.y() : segBegin.x());
      }
    }
  } else {
    trackLoc = 0;
  }

  unique_ptr<taPinFig> ps = make_unique<taPathSeg>();
  ps->setNet(guide->getNet());
  auto rptr = static_cast<taPathSeg*>(ps.get());
  if (isH) {
    rptr->setPoints(frPoint(maxBegin, trackLoc), frPoint(minEnd, trackLoc));
  } else {
    rptr->setPoints(frPoint(trackLoc, maxBegin), frPoint(trackLoc, minEnd));
  }
  rptr->setLayerNum(layerNum);
  if (guide->getNet() && guide->getNet()->getNondefaultRule()){
        frNonDefaultRule* ndr = guide->getNet()->getNondefaultRule();
        auto style = getDesign()->getTech()->getLayer(layerNum)->getDefaultSegStyle();
        style.setWidth(max((int)style.getWidth(), ndr->getWidth(layerNum/2 -1)));
        rptr->setStyle(style);
  }else 
      rptr->setStyle(getDesign()->getTech()->getLayer(layerNum)->getDefaultSegStyle());
  // owner set when add to taPin
  iroute->addPinFig(std::move(ps));
  frViaDef* viaDef;
  for (auto coord: upViaCoordSet) {
    if (guide->getNet()->getNondefaultRule() && guide->getNet()->getNondefaultRule()->getPrefVia((layerNum+2)/2 -1))
        viaDef = guide->getNet()->getNondefaultRule()->getPrefVia((layerNum+2)/2 -1);
    else 
        viaDef = getDesign()->getTech()->getLayer(layerNum + 1)->getDefaultViaDef();
    unique_ptr<taPinFig> via = make_unique<taVia>(viaDef);
    via->setNet(guide->getNet());
    auto rViaPtr = static_cast<taVia*>(via.get());
    rViaPtr->setOrigin(isH ? frPoint(coord, trackLoc) : frPoint(trackLoc, coord));
    iroute->addPinFig(std::move(via));
  }
  for (auto coord: downViaCoordSet) {
    if (guide->getNet()->getNondefaultRule() && guide->getNet()->getNondefaultRule()->getPrefVia(layerNum/2 -1))
      viaDef = guide->getNet()->getNondefaultRule()->getPrefVia(layerNum/2 -1);
    else 
        viaDef = getDesign()->getTech()->getLayer(layerNum - 1)->getDefaultViaDef();
    unique_ptr<taPinFig> via = make_unique<taVia>(viaDef);
    via->setNet(guide->getNet());
    auto rViaPtr = static_cast<taVia*>(via.get());
    rViaPtr->setOrigin(isH ? frPoint(coord, trackLoc) : frPoint(trackLoc, coord));
    iroute->addPinFig(std::move(via));
  }
  iroute->setWlenHelper(wlen);
  if (wlen2 < std::numeric_limits<frCoord>::max()) {
    iroute->setWlenHelper2(wlen2);
  }
  addIroute(std::move(iroute), isExt);
}



void FlexTAWorker::initIroutes() {
  //bool enableOutput = true;
  bool enableOutput = false;
  frRegionQuery::Objects<frGuide> result;
  auto regionQuery = getRegionQuery();
  for (int lNum = 0; lNum < (int)getDesign()->getTech()->getLayers().size(); lNum++) {
    auto layer = getDesign()->getTech()->getLayer(lNum);
    if (layer->getType() != frLayerTypeEnum::ROUTING) {
      continue;
    }
    if (layer->getDir() != getDir()) {
      continue;
    }
    result.clear();
    regionQuery->queryGuide(getExtBox(), lNum, result);
    //cout <<endl <<"query1:" <<endl;
    for (auto &[boostb, guide]: result) {
      frPoint pt1, pt2;
      guide->getPoints(pt1, pt2);
      //if (enableOutput) {
      //  cout <<"found guide (" 
      //       <<pt1.x() * 1.0 / getDesign()->getTopBlock()->getDBUPerUU() <<", " 
      //       <<pt1.y() * 1.0 / getDesign()->getTopBlock()->getDBUPerUU() <<") (" 
      //       <<pt2.x() * 1.0 / getDesign()->getTopBlock()->getDBUPerUU() <<", " 
      //       <<pt2.y() * 1.0 / getDesign()->getTopBlock()->getDBUPerUU() <<") " 
      //       <<guide->getNet()->getName() << "\n";
      //}
      //cout <<endl;
      initIroute(guide);
    }
  }

  if (enableOutput) {
    bool   isH = (getDir() == frPrefRoutingDirEnum::frcHorzPrefRoutingDir);
    double dbu = getDesign()->getTopBlock()->getDBUPerUU();
    for (auto &iroute: iroutes_) {
      frPoint bp, ep;
      frCoord bc, ec, trackLoc;
      cout <<iroute->getId() <<" " <<iroute->getGuide()->getNet()->getName();
      auto guideLNum = iroute->getGuide()->getBeginLayerNum();
      for (auto &uPinFig: iroute->getFigs()) {
        if (uPinFig->typeId() == tacPathSeg) {
          auto obj = static_cast<taPathSeg*>(uPinFig.get());
          obj->getPoints(bp, ep);
          bc = isH ? bp.x() : bp.y();
          ec = isH ? ep.x() : ep.y();
          trackLoc = isH ? bp.y() : bp.x();
          cout <<" (" <<bc / dbu <<"-->" <<ec / dbu <<"), len@" <<(ec - bc) / dbu <<", track@" <<trackLoc / dbu
               <<", " <<getDesign()->getTech()->getLayer(iroute->getGuide()->getBeginLayerNum())->getName();
        } else if (uPinFig->typeId() == tacVia) {
          auto obj = static_cast<taVia*>(uPinFig.get());
          auto cutLNum = obj->getViaDef()->getCutLayerNum();
          obj->getOrigin(bp);
          bc = isH ? bp.x() : bp.y();
          cout <<string((cutLNum > guideLNum) ? ", U@" : ", D@") <<bc / dbu;
        }
      }
      cout <<", wlen_h@" <<iroute->getWlenHelper() <<endl;
    }
  }
}



void FlexTAWorker::initCosts() {
  //bool enableOutput = true;
  bool enableOutput = false;
  bool isH = (getDir() == frPrefRoutingDirEnum::frcHorzPrefRoutingDir);
  frPoint bp, ep;
  frCoord bc, ec;
  // init cost
  if (isInitTA()) {
    for (auto &iroute: iroutes_) {
      auto pitch = getDesign()->getTech()->getLayer(iroute->getGuide()->getBeginLayerNum())->getPitch();
      for (auto &uPinFig: iroute->getFigs()) {
        if (uPinFig->typeId() == tacPathSeg) {
          auto obj = static_cast<taPathSeg*>(uPinFig.get());
          obj->getPoints(bp, ep);
          bc = isH ? bp.x() : bp.y();
          ec = isH ? ep.x() : ep.y();
          iroute->setCost(ec - bc + iroute->hasWlenHelper2() * pitch * 1000);
        }
      }
    }
  } else {
    auto &workerRegionQuery = getWorkerRegionQuery();
    // update worker rq
    for (auto &iroute: iroutes_) {
      for (auto &uPinFig: iroute->getFigs()) {
        workerRegionQuery.add(uPinFig.get());
        addCost(uPinFig.get());
      }
    }
    for (auto &iroute: extIroutes_) {
      for (auto &uPinFig: iroute->getFigs()) {
        workerRegionQuery.add(uPinFig.get());
        addCost(uPinFig.get());
      }
    }
    // update iroute cost
    for (auto &iroute: iroutes_) {
      frUInt4 drcCost = 0;
      frCoord trackLoc = std::numeric_limits<frCoord>::max();
      for (auto &uPinFig: iroute->getFigs()) {
        if (uPinFig->typeId() == tacPathSeg) {
          static_cast<taPathSeg*>(uPinFig.get())->getPoints(bp, ep);
          if (isH) {
            trackLoc = bp.y();
          } else {
            trackLoc = bp.x();
          }
          break;
        }
      }
      if (trackLoc == std::numeric_limits<frCoord>::max()) {
        cout <<"Error: FlexTAWorker::initCosts does not find trackLoc" <<endl;
        exit(1);
      }
      assignIroute_getCost(iroute.get(), trackLoc, drcCost);
      iroute->setCost(drcCost);
      totCost_ += drcCost;
      if (enableOutput && !isInitTA()) {
        bool   isH = (getDir() == frPrefRoutingDirEnum::frcHorzPrefRoutingDir);
        double dbu = getDesign()->getTopBlock()->getDBUPerUU();
        frPoint bp, ep;
        frCoord bc, ec, trackLoc;
        cout <<iroute->getId() <<" " <<iroute->getGuide()->getNet()->getName();
        auto guideLNum = iroute->getGuide()->getBeginLayerNum();
        for (auto &uPinFig: iroute->getFigs()) {
          if (uPinFig->typeId() == tacPathSeg) {
            auto obj = static_cast<taPathSeg*>(uPinFig.get());
            obj->getPoints(bp, ep);
            bc = isH ? bp.x() : bp.y();
            ec = isH ? ep.x() : ep.y();
            trackLoc = isH ? bp.y() : bp.x();
            cout <<" (" <<bc / dbu <<"-->" <<ec / dbu <<"), len@" <<(ec - bc) / dbu <<", track@" <<trackLoc / dbu
                 <<", " <<getDesign()->getTech()->getLayer(iroute->getGuide()->getBeginLayerNum())->getName();
          } else if (uPinFig->typeId() == tacVia) {
            auto obj = static_cast<taVia*>(uPinFig.get());
            auto cutLNum = obj->getViaDef()->getCutLayerNum();
            obj->getOrigin(bp);
            bc = isH ? bp.x() : bp.y();
            cout <<string((cutLNum > guideLNum) ? ", U@" : ", D@") <<bc / dbu;
          }
        }
        //cout <<", wlen_h@" <<iroute->getWlenHelper() <<", cost@" <<iroute->getCost() <<", drcCost@" <<iroute->getDrcCost() <<endl;
        cout <<", wlen_h@" <<iroute->getWlenHelper() <<", cost@" <<iroute->getCost() <<endl;
      }
    }
  }
}

void FlexTAWorker::sortIroutes() {
  //bool enableOutput = true;
  bool enableOutput = false;
  // init cost
  if (isInitTA()) {
    for (auto &iroute: iroutes_) {
      addToReassignIroutes(iroute.get());
    }
  } else {
    for (auto &iroute: iroutes_) {
      if (iroute->getCost()) {
        addToReassignIroutes(iroute.get());
      }
    }
  }
  if (enableOutput && !isInitTA()) {
    bool   isH = (getDir() == frPrefRoutingDirEnum::frcHorzPrefRoutingDir);
    double dbu = getDesign()->getTopBlock()->getDBUPerUU();
    for (auto &iroute: reassignIroutes_) {
      frPoint bp, ep;
      frCoord bc, ec, trackLoc;
      cout <<iroute->getId() <<" " <<iroute->getGuide()->getNet()->getName();
      auto guideLNum = iroute->getGuide()->getBeginLayerNum();
      for (auto &uPinFig: iroute->getFigs()) {
        if (uPinFig->typeId() == tacPathSeg) {
          auto obj = static_cast<taPathSeg*>(uPinFig.get());
          obj->getPoints(bp, ep);
          bc = isH ? bp.x() : bp.y();
          ec = isH ? ep.x() : ep.y();
          trackLoc = isH ? bp.y() : bp.x();
          cout <<" (" <<bc / dbu <<"-->" <<ec / dbu <<"), len@" <<(ec - bc) / dbu <<", track@" <<trackLoc / dbu
               <<", " <<getDesign()->getTech()->getLayer(iroute->getGuide()->getBeginLayerNum())->getName();
        } else if (uPinFig->typeId() == tacVia) {
          auto obj = static_cast<taVia*>(uPinFig.get());
          auto cutLNum = obj->getViaDef()->getCutLayerNum();
          obj->getOrigin(bp);
          bc = isH ? bp.x() : bp.y();
          cout <<string((cutLNum > guideLNum) ? ", U@" : ", D@") <<bc / dbu;
        }
      }
      //cout <<", wlen_h@" <<iroute->getWlenHelper() <<", cost@" <<iroute->getCost() <<", drcCost@" <<iroute->getDrcCost() <<endl;
      cout <<", wlen_h@" <<iroute->getWlenHelper() <<", cost@" <<iroute->getCost() <<endl;
    }
  }
}

void FlexTAWorker::initFixedObjs_helper(const frBox &box, frCoord bloatDist, frLayerNum lNum, frNet* net) {
  //bool enableOutput = true;
  bool enableOutput = false;
  double dbu = getTech()->getDBUPerUU();
  frBox bloatBox;
  box.bloat(bloatDist, bloatBox);
  auto con = getDesign()->getTech()->getLayer(lNum)->getShortConstraint();
  bool isH = (getDir() == frPrefRoutingDirEnum::frcHorzPrefRoutingDir);
  int idx1, idx2;
  //frCoord x1, x2;
  if (isH) {
    getTrackIdx(bloatBox.bottom(), bloatBox.top(),   lNum, idx1, idx2);
  } else {
    getTrackIdx(bloatBox.left(),   bloatBox.right(), lNum, idx1, idx2);
  }
  auto &trackLocs = getTrackLocs(lNum);
  auto &workerRegionQuery = getWorkerRegionQuery();
  for (int i = idx1; i <= idx2; i++) {
    // new
    //auto &track = tracks[i];
    //track.addToCost(net, x1, x2, 0);
    //track.addToCost(net, x1, x2, 1);
    //track.addToCost(net, x1, x2, 2);
    // old
    auto trackLoc = trackLocs[i];
    frBox tmpBox;
    if (isH) {
      tmpBox.set(bloatBox.left(), trackLoc, bloatBox.right(), trackLoc);
    } else {
      tmpBox.set(trackLoc, bloatBox.bottom(), trackLoc, bloatBox.top());
    }
    workerRegionQuery.addCost(tmpBox, lNum, net, con);
    if (enableOutput) {
      cout <<"  add fixed obj cost ("
           <<tmpBox.left()  / dbu <<", " <<tmpBox.bottom() / dbu <<") (" 
           <<tmpBox.right() / dbu <<", " <<tmpBox.top()    / dbu <<") " 
           <<getDesign()->getTech()->getLayer(lNum)->getName();
      if (net != nullptr) {
        cout <<" " <<net->getName();
      }
      cout <<endl <<flush;
    }
  }
}


void FlexTAWorker::initFixedObjs() {
  //bool enableOutput = false;
  //bool enableOutput = true;
  frRegionQuery::Objects<frBlockObject> result;
  frBox box;
  frCoord width = 0;
  frCoord bloatDist = 0;
  for (auto layerNum = getTech()->getBottomLayerNum(); layerNum <= getTech()->getTopLayerNum(); ++layerNum) {
    result.clear();
    if (getTech()->getLayer(layerNum)->getType() != frLayerTypeEnum::ROUTING ||
        getTech()->getLayer(layerNum)->getDir()  != getDir()) {
      continue;
    }
    width = getTech()->getLayer(layerNum)->getWidth();
    getRegionQuery()->query(getExtBox(), layerNum, result);
    for (auto &[bounds, obj]: result) {
      bounds.bloat(-1, box);
      // instterm term
      if (obj->typeId() == frcTerm || obj->typeId() == frcInstTerm) {
        bloatDist = TASHAPEBLOATWIDTH * width;
        frNet* netPtr = nullptr;
        if (obj->typeId() == frcTerm) {
          netPtr = static_cast<frTerm*>(obj)->getNet();
        } else {
          netPtr = static_cast<frInstTerm*>(obj)->getNet();
        }
        initFixedObjs_helper(box, bloatDist, layerNum, netPtr);
      // snet
      } else if (obj->typeId() == frcPathSeg || obj->typeId() == frcVia) {
        bloatDist = initFixedObjs_calcBloatDist(obj, layerNum, bounds);
        frNet* netPtr = nullptr;
        if (obj->typeId() == frcPathSeg) {
          netPtr = static_cast<frPathSeg*>(obj)->getNet();
        } else {
          netPtr = static_cast<frVia*>(obj)->getNet();
        }
        initFixedObjs_helper(box, bloatDist, layerNum, netPtr);
        if (DBPROCESSNODE == "GF14_13M_3Mx_2Cx_4Kx_2Hx_2Gx_LB" && getTech()->getLayer(layerNum)->getType() == frLayerTypeEnum::ROUTING) {
          // down-via
          if (layerNum - 2 >= getDesign()->getTech()->getBottomLayerNum() && 
              getTech()->getLayer(layerNum - 2)->getType() == frLayerTypeEnum::ROUTING) {
            auto cutLayer = getTech()->getLayer(layerNum - 1);
            frBox viaBox;
            auto via = make_unique<frVia>(cutLayer->getDefaultViaDef());
            via->getLayer2BBox(viaBox);
            frCoord viaWidth = viaBox.width();
            // only add for fat via
            if (viaWidth > width) {
              bloatDist = initFixedObjs_calcOBSBloatDistVia(cutLayer->getDefaultViaDef(), layerNum, bounds, false);
              initFixedObjs_helper(box, bloatDist, layerNum, netPtr);
            }
          }
          // up-via
          if (layerNum + 2 < (int) design_->getTech()->getLayers().size() && 
              getTech()->getLayer(layerNum + 2)->getType() == frLayerTypeEnum::ROUTING) {
            auto cutLayer = getTech()->getLayer(layerNum + 1);
            frBox viaBox;
            auto via = make_unique<frVia>(cutLayer->getDefaultViaDef());
            via->getLayer1BBox(viaBox);
            frCoord viaWidth = viaBox.width();
            // only add for fat via
            if (viaWidth > width) {
              bloatDist = initFixedObjs_calcOBSBloatDistVia(cutLayer->getDefaultViaDef(), layerNum, bounds, false);
              initFixedObjs_helper(box, bloatDist, layerNum, netPtr);
            }
          }
        }
      } else if (obj->typeId() == frcBlockage || obj->typeId() == frcInstBlockage) {
        bloatDist = initFixedObjs_calcBloatDist(obj, layerNum, bounds);
        initFixedObjs_helper(box, bloatDist, layerNum, nullptr);

        if (DBPROCESSNODE == "GF14_13M_3Mx_2Cx_4Kx_2Hx_2Gx_LB") {
          // block track for up-via and down-via for fat MACRO OBS          
          bool isMacro = false;
          if (obj->typeId() == frcBlockage) {
            isMacro = true;
          } else {
            auto inst = (static_cast<frInstBlockage*>(obj))->getInst();
            if (inst->getRefBlock()->getMacroClass() == MacroClassEnum::BLOCK ||
                isPad(inst->getRefBlock()->getMacroClass()) ||
                inst->getRefBlock()->getMacroClass() == MacroClassEnum::RING) {
              isMacro = true;
            }
          }
          bool isFatOBS = true;
          if (bounds.width() <= 2 * width) {
            isFatOBS = false;
          } 
          if (isMacro && isFatOBS) {
            // down-via
            if (layerNum - 2 >= getDesign()->getTech()->getBottomLayerNum() && 
                getTech()->getLayer(layerNum - 2)->getType() == frLayerTypeEnum::ROUTING) {
              auto cutLayer = getTech()->getLayer(layerNum - 1);
              bloatDist = initFixedObjs_calcOBSBloatDistVia(cutLayer->getDefaultViaDef(), layerNum, bounds);
              initFixedObjs_helper(box, bloatDist, layerNum - 2, nullptr);
            }
            // up-via
            if (layerNum + 2 < (int) design_->getTech()->getLayers().size() && 
                getTech()->getLayer(layerNum + 2)->getType() == frLayerTypeEnum::ROUTING) {
              auto cutLayer = getTech()->getLayer(layerNum + 1);
              bloatDist = initFixedObjs_calcOBSBloatDistVia(cutLayer->getDefaultViaDef(), layerNum, bounds);
              initFixedObjs_helper(box, bloatDist, layerNum + 2, nullptr);
            }
          }
        }
      } else {
        cout <<"Warning: unsupported type in initFixedObjs" <<endl;
      }
    }
  }
}

frCoord FlexTAWorker::initFixedObjs_calcOBSBloatDistVia(frViaDef *viaDef, const frLayerNum lNum, const frBox &box, bool isOBS) {
  auto layer = getTech()->getLayer(lNum);
  frBox viaBox;
  auto via = make_unique<frVia>(viaDef);
  if (viaDef->getLayer1Num() == lNum) {
    via->getLayer1BBox(viaBox);
  } else {
    via->getLayer2BBox(viaBox);
  }
  frCoord viaWidth = viaBox.width();
  frCoord viaLength = viaBox.length();

  frCoord obsWidth = box.width();
  if (USEMINSPACING_OBS && isOBS) {
    obsWidth = layer->getWidth();
  }

  frCoord bloatDist = 0;
  auto con = getTech()->getLayer(lNum)->getMinSpacing();
  if (con) {
    if (con->typeId() == frConstraintTypeEnum::frcSpacingConstraint) {
      bloatDist = static_cast<frSpacingConstraint*>(con)->getMinSpacing();
    } else if (con->typeId() == frConstraintTypeEnum::frcSpacingTablePrlConstraint) {
      bloatDist = static_cast<frSpacingTablePrlConstraint*>(con)->find(obsWidth, viaWidth/*prl*/);
    } else if (con->typeId() == frConstraintTypeEnum::frcSpacingTableTwConstraint) {
      bloatDist = static_cast<frSpacingTableTwConstraint*>(con)->find(obsWidth, viaWidth, viaWidth/*prl*/);
    }
  }
  // at least via enclosure should not short with obs (OBS has issue with wrongway and PG has issue with prefDir)
  // TODO: generalize the following
  if (isOBS) {
    bloatDist += viaLength / 2;
  } else {
    bloatDist += viaWidth / 2;
  }
  return bloatDist;
}

frCoord FlexTAWorker::initFixedObjs_calcBloatDist(frBlockObject *obj, const frLayerNum lNum, const frBox &box) {
  auto layer = getTech()->getLayer(lNum);
  frCoord width = layer->getWidth();
  // use width if minSpc does not exist
  frCoord bloatDist = width;
  frCoord objWidth = box.width();
  frCoord prl = (layer->getDir() == frPrefRoutingDirEnum::frcHorzPrefRoutingDir) ? (box.right() - box.left()) :
                                                                                   (box.top() - box.bottom());
  if (obj->typeId() == frcBlockage || obj->typeId() == frcInstBlockage) {
    if (USEMINSPACING_OBS) {
      objWidth = width;
    }
  }
  auto con = getTech()->getLayer(lNum)->getMinSpacing();
  if (con) {
    if (con->typeId() == frConstraintTypeEnum::frcSpacingConstraint) {
      bloatDist = static_cast<frSpacingConstraint*>(con)->getMinSpacing();
    } else if (con->typeId() == frConstraintTypeEnum::frcSpacingTablePrlConstraint) {
      bloatDist = static_cast<frSpacingTablePrlConstraint*>(con)->find(objWidth, prl);
    } else if (con->typeId() == frConstraintTypeEnum::frcSpacingTableTwConstraint) {
      bloatDist = static_cast<frSpacingTableTwConstraint*>(con)->find(objWidth, width, prl);
    }
  }
  // assuming the wire width is width
  bloatDist += width / 2;
  return bloatDist;
}

void FlexTAWorker::init() {
  rq_.init();
  initTracks();
  if (getTAIter() != -1) {
    initFixedObjs();
  }
  initIroutes();
  if (getTAIter() != -1) {
    initCosts();
    sortIroutes();
  }
}
