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

#include <iostream>
#include <sstream>
#include <chrono>
#include "FlexPA.h"
#include "db/infra/frTime.h"
#include "gc/FlexGC.h"

using namespace std;
using namespace fr;

void FlexPA::getPrefTrackPatterns(vector<frTrackPattern*> &prefTrackPatterns) {
  for (auto &trackPattern: design_->getTopBlock()->getTrackPatterns()) {
    auto isVerticalTrack = trackPattern->isHorizontal(); // yes = vertical track
    if (design_->getTech()->getLayer(trackPattern->getLayerNum())->getDir() == frcHorzPrefRoutingDir) {
      if (!isVerticalTrack) {
        prefTrackPatterns.push_back(trackPattern);
      }
    } else {
      if (isVerticalTrack) {
        prefTrackPatterns.push_back(trackPattern);
      }
    }
  }
}


void FlexPA::initUniqueInstance_refBlock2PinLayerRange(map<frBlock*, tuple<frLayerNum, frLayerNum>, frBlockObjectComp> &refBlock2PinLayerRange) {
  //bool enableOutput = true;
  bool enableOutput = false;
  if (enableOutput) {
    cout <<endl <<"initUniqueInstances_refBlock2PinLayerRange ..." <<endl;
  }
  int numLayers = design_->getTech()->getLayers().size();
  for (auto &uRefBlock: design_->getRefBlocks()) {
    auto refBlock = uRefBlock.get();
    frLayerNum minLayerNum = std::numeric_limits<frLayerNum>::max();
    frLayerNum maxLayerNum = std::numeric_limits<frLayerNum>::min();
    for (auto &uTerm: refBlock->getTerms()) {
      if (isSkipTerm(uTerm.get())) {
        continue;
      }
      for (auto &uPin: uTerm->getPins()) {
        for (auto &uPinFig: uPin->getFigs()) {
          auto pinFig = uPinFig.get();
          if (pinFig->typeId() == frcRect) {
            auto lNum = static_cast<frRect*>(pinFig)->getLayerNum();
            if (lNum >= getDesign()->getTech()->getBottomLayerNum()) {
              minLayerNum = std::min(minLayerNum, lNum);
            }
            maxLayerNum = std::max(maxLayerNum, lNum);
          } else if (pinFig->typeId() == frcPolygon) {
            auto lNum = static_cast<frPolygon*>(pinFig)->getLayerNum();
            if (lNum >= getDesign()->getTech()->getBottomLayerNum()) {
              minLayerNum = std::min(minLayerNum, lNum);
            }
            maxLayerNum = std::max(maxLayerNum, lNum);
          } else {
            logger_->error(DRT, 65, "instAnalysis unsupported pinFig.");
          }
        }
      }
    }
    if (minLayerNum < getDesign()->getTech()->getBottomLayerNum() ||
        maxLayerNum > getDesign()->getTech()->getTopLayerNum()) {
      logger_->warn(DRT, 66, "instAnalysis skips {} due to no pin shapes",
                   refBlock->getName());
      continue;
    }
    maxLayerNum = std::min(maxLayerNum + 2, numLayers);
    refBlock2PinLayerRange[refBlock] = make_tuple(minLayerNum, maxLayerNum);
    if (enableOutput) {
      cout << "  " <<refBlock->getName() << " PIN layer ("
           << design_->getTech()->getLayer(minLayerNum)->getName() << ", "
           << design_->getTech()->getLayer(maxLayerNum)->getName() << ")" <<endl;
    }
  }
  //cout <<"  refBlock pin layer range done" <<endl;
}

bool FlexPA::hasTrackPattern(frTrackPattern* tp, const frBox &box) {
  bool enableOutput = false;
  //bool enableOutput = true;
  auto isVerticalTrack = tp->isHorizontal(); // yes = vertical track
  frCoord low  = tp->getStartCoord();
  frCoord high = low + (frCoord)(tp->getTrackSpacing()) * ((frCoord)(tp->getNumTracks()) - 1);
  if (enableOutput) {
    double dbu = getDesign()->getTopBlock()->getDBUPerUU();
    cout <<"tp low@" <<low / dbu <<", high@" <<high / dbu <<endl;
  }
  if (isVerticalTrack) {
    return !(low > box.right() || high < box.left());
  } else {
    return !(low > box.top() || high < box.bottom());
  }
}

// must init all unique, including filler, macro, etc. to ensure frInst pinAccessIdx is active
void FlexPA::initUniqueInstance_main(const map<frBlock*, tuple<frLayerNum, frLayerNum>, frBlockObjectComp> &refBlock2PinLayerRange,
                                     const vector<frTrackPattern*> &prefTrackPatterns) {
  bool enableOutput = false;
  //bool enableOutput = true;
  if (enableOutput) {
    cout <<endl <<"initUniqueInstances_main ..." <<endl;
  }
  map<frBlock*, 
      map<frOrient, map<vector<frCoord>, set<frInst*, frBlockObjectComp> > >,
      frBlockObjectComp> refBlockOT2Insts; // refblock orient track-offset to instances
  vector<frInst*> ndrInsts;
  vector<frCoord> offset;
  int cnt = 0;
  for (auto &inst: design_->getTopBlock()->getInsts()) {
      if (isNDRInst(*inst)){
          ndrInsts.push_back(inst.get());
          continue;
      }
    frPoint origin;
    inst->getOrigin(origin);
    frBox boundaryBBox;
    inst->getBoundaryBBox(boundaryBBox);
    if (enableOutput) {
      double dbu = getDesign()->getTopBlock()->getDBUPerUU();
      //cout <<inst->getName() <<": (" 
      //     <<boundaryBBox.left()  <<", " <<boundaryBBox.bottom() <<") ("
      //     <<boundaryBBox.right() <<", " <<boundaryBBox.top()    <<")"
      //     <<endl;
      cout <<inst->getName() <<": (" 
           <<boundaryBBox.left()  / dbu <<", " <<boundaryBBox.bottom() / dbu <<") ("
           <<boundaryBBox.right() / dbu <<", " <<boundaryBBox.top()    / dbu <<")"
           <<endl;
    }
    auto orient = inst->getOrient();
    auto &[minLayerNum, maxLayerNum] = refBlock2PinLayerRange.find(inst->getRefBlock())->second;
    offset.clear();
    for (auto &tp: prefTrackPatterns) {
      if (tp->getLayerNum() >= minLayerNum && tp->getLayerNum() <= maxLayerNum) {
        if (hasTrackPattern(tp, boundaryBBox)) {
          // vertical track
          if (tp->isHorizontal()) {
            offset.push_back(origin.x() % tp->getTrackSpacing());
            //if (enableOutput) {
            //  cout <<"inst/offset/layer " <<inst->getName() <<" " <<origin.y() % tp->getTrackSpacing() 
            //       <<" " <<design->getTech()->getLayer(tp->getLayerNum())->getName() <<endl;
            //}
          } else {
            offset.push_back(origin.y() % tp->getTrackSpacing());
            //if (enableOutput) {
            //  cout <<"inst/offset/layer " <<inst->getName() <<" " <<origin.x() % tp->getTrackSpacing()
            //       <<" " <<design->getTech()->getLayer(tp->getLayerNum())->getName() <<endl;
            //}
          }
        } else {
          offset.push_back(tp->getTrackSpacing());
        }
      } else {
        offset.push_back(tp->getTrackSpacing());
      }
    }
    refBlockOT2Insts[inst->getRefBlock()][orient][offset].insert(inst.get());
    cnt++;
    //if (VERBOSE > 0) {
    //  if (cnt < 100000) {
    //    if (cnt % 10000 == 0) {
    //      cout <<"  complete " <<cnt <<" instances" <<endl;
    //    }
    //  } else {
    //    if (cnt % 100000 == 0) {
    //      cout <<"  complete " <<cnt <<" instances" <<endl;
    //    }
    //  }
    //}
  }

  if (enableOutput) {
    cout <<endl <<"summary: " <<endl;
  }
  cnt = 0;
  frString orientName;
  for (auto &[refBlock, orientMap]: refBlockOT2Insts) {
    if (enableOutput) {
      cout <<"  " <<refBlock->getName() <<" (ORIENT/#diff patterns)" <<endl;
    }
    for (auto &[orient, offsetMap]: orientMap) {
      cnt += offsetMap.size();
      if (enableOutput) {
        orient.getName(orientName);
        cout <<"     (" <<orientName <<", " <<offsetMap.size() <<")";
      }
      for (auto &[vec, inst]: offsetMap) {
        auto uniqueInst = *(inst.begin());
        uniqueInstances_.push_back(uniqueInst);
        for (auto i: inst) {
          inst2unique_[i] = uniqueInst;
        }
        if (enableOutput) {
          cout <<" " <<(*(inst.begin()))->getName();
        }
      }
      if (enableOutput) {
        cout <<endl;
      }
    }
  }
  for (frInst* inst : ndrInsts){
      uniqueInstances_.push_back(inst);
      inst2unique_[inst] = inst;
  }

  // init unique2Idx
  for (int i = 0; i < (int) uniqueInstances_.size(); i++) {
    unique2Idx_[uniqueInstances_[i]] = i;
  }

  //if (VERBOSE > 0) {
  //  cout <<"#unique instances = " <<cnt <<endl;
  //}
}

bool FlexPA::isNDRInst(frInst& inst){
    for (auto& a : inst.getInstTerms()){
        if (a->getNet() && a->getNet()->getNondefaultRule()){   //this criterion can be improved
            return true;
        }
    }
    return false;
}

void FlexPA::initUniqueInstance() {
  vector<frTrackPattern*> prefTrackPatterns;
  getPrefTrackPatterns(prefTrackPatterns);
  
  map<frBlock*, tuple<frLayerNum, frLayerNum>, frBlockObjectComp> refBlock2PinLayerRange;
  initUniqueInstance_refBlock2PinLayerRange(refBlock2PinLayerRange);

  initUniqueInstance_main(refBlock2PinLayerRange, prefTrackPatterns);
}

void FlexPA::initPinAccess() {
  bool enableOutput = false;
  //bool enableOutput = true;
  for (auto &inst: uniqueInstances_) {
    for (auto &instTerm: inst->getInstTerms()) {
      for (auto &pin: instTerm->getTerm()->getPins()) {
        if (unique2paidx_.find(inst) == unique2paidx_.end()) {
          unique2paidx_[inst] = pin->getNumPinAccess();
        } else {
          if (unique2paidx_[inst] != pin->getNumPinAccess()) {
            logger_->error(DRT, 69, "initPinAccess error.");
            exit(1);
          }
        }
        auto pa = make_unique<frPinAccess>();
        pin->addPinAccess(std::move(pa));
      }
    }
    inst->setPinAccessIdx(unique2paidx_[inst]);
  }
  for (auto &[inst, uniqueInst]: inst2unique_) {
    inst->setPinAccessIdx(uniqueInst->getPinAccessIdx());
  }
  if (enableOutput) {
    cout <<"unique2paidx:" <<endl;
    for (auto &[inst, idx]: unique2paidx_) {
      cout <<inst->getName() <<" " <<idx <<endl;
    }
  }
  if (enableOutput) {
    cout <<"inst2paidx:" <<endl;
    for (auto &inst: getDesign()->getTopBlock()->getInsts()) {
      cout <<inst->getName() <<" " <<inst->getPinAccessIdx() <<endl;
    }
  }

  // IO terms
  for (auto &term: getDesign()->getTopBlock()->getTerms()) {
    for (auto &pin: term->getPins()) {
      auto pa = make_unique<frPinAccess>();
      pin->addPinAccess(std::move(pa));
    }
  }
}

void FlexPA::initViaRawPriority() {
  for (auto layerNum = design_->getTech()->getBottomLayerNum(); layerNum <= design_->getTech()->getTopLayerNum(); ++layerNum) {
    if (design_->getTech()->getLayer(layerNum)->getType() != frLayerTypeEnum::CUT) {
      continue;
    }
    for (auto &viaDef: design_->getTech()->getLayer(layerNum)->getViaDefs()) {
      int cutNum = int(viaDef->getCutFigs().size());
      viaRawPriorityTuple priority;
      getViaRawPriority(viaDef, priority);
      layerNum2ViaDefs_[layerNum][cutNum][priority] = viaDef;
    }
  }
}

void FlexPA::getViaRawPriority(frViaDef* viaDef, viaRawPriorityTuple &priority) {
  bool isNotDefaultVia = !(viaDef->getDefault());
  bool isNotUpperAlign = false;
  bool isNotLowerAlign = false;
  gtl::polygon_90_set_data<frCoord> viaLayerPS1;

  for (auto &fig: viaDef->getLayer1Figs()) {
    frBox bbox;
    fig->getBBox(bbox);
    gtl::rectangle_data<frCoord> bboxRect(bbox.left(), bbox.bottom(), bbox.right(), bbox.top());
    using namespace boost::polygon::operators;
    viaLayerPS1 += bboxRect;
  }
  gtl::rectangle_data<frCoord> layer1Rect;
  gtl::extents(layer1Rect, viaLayerPS1);
  bool isLayer1Horz = (gtl::xh(layer1Rect) - gtl::xl(layer1Rect)) > (gtl::yh(layer1Rect) - gtl::yl(layer1Rect));
  frCoord layer1Width = std::min((gtl::xh(layer1Rect) - gtl::xl(layer1Rect)), (gtl::yh(layer1Rect) - gtl::yl(layer1Rect)));
  isNotLowerAlign = (isLayer1Horz && (getDesign()->getTech()->getLayer(viaDef->getLayer1Num())->getDir() == frcVertPrefRoutingDir)) ||
                    (!isLayer1Horz && (getDesign()->getTech()->getLayer(viaDef->getLayer1Num())->getDir() == frcHorzPrefRoutingDir));

  gtl::polygon_90_set_data<frCoord> viaLayerPS2;
  for (auto &fig: viaDef->getLayer2Figs()) {
    frBox bbox;
    fig->getBBox(bbox);
    gtl::rectangle_data<frCoord> bboxRect(bbox.left(), bbox.bottom(), bbox.right(), bbox.top());
    using namespace boost::polygon::operators;
    viaLayerPS2 += bboxRect;
  }
  gtl::rectangle_data<frCoord> layer2Rect;
  gtl::extents(layer2Rect, viaLayerPS2);
  bool isLayer2Horz = (gtl::xh(layer2Rect) - gtl::xl(layer2Rect)) > (gtl::yh(layer2Rect) - gtl::yl(layer2Rect));
  frCoord layer2Width = std::min((gtl::xh(layer2Rect) - gtl::xl(layer2Rect)), (gtl::yh(layer2Rect) - gtl::yl(layer2Rect)));
  isNotUpperAlign = (isLayer2Horz && (getDesign()->getTech()->getLayer(viaDef->getLayer2Num())->getDir() == frcVertPrefRoutingDir)) ||
                    (!isLayer2Horz && (getDesign()->getTech()->getLayer(viaDef->getLayer2Num())->getDir() == frcHorzPrefRoutingDir));

  frCoord layer1Area = gtl::area(viaLayerPS1);
  frCoord layer2Area = gtl::area(viaLayerPS2);

  priority = std::make_tuple(isNotDefaultVia, layer1Width, layer2Width, isNotUpperAlign, layer2Area, layer1Area, isNotLowerAlign);
}

void FlexPA::initTrackCoords() {
  bool enableOutput = false;
  //bool enableOutput = true;

  if (enableOutput) {
    cout <<endl <<endl;
  }

  int     numLayers = getDesign()->getTech()->getLayers().size();
  frCoord manuGrid  = getDesign()->getTech()->getManufacturingGrid();

  // full coords
  trackCoords_.clear();
  trackCoords_.resize(numLayers);
  for (auto &trackPattern: design_->getTopBlock()->getTrackPatterns()) {
    auto layerNum = trackPattern->getLayerNum();
    auto isVLayer = (design_->getTech()->getLayer(layerNum)->getDir() == frcVertPrefRoutingDir);
    auto isVTrack = trackPattern->isHorizontal(); // yes = vertical track
    if ((!isVLayer && !isVTrack) || (isVLayer && isVTrack)) {
      frCoord currCoord = trackPattern->getStartCoord();
      for (int i = 0; i < (int)trackPattern->getNumTracks(); i++) {
        trackCoords_[layerNum][currCoord] = frAccessPointEnum::OnGridAP;
        currCoord += trackPattern->getTrackSpacing();
      }
    }
  }
  if (enableOutput) {
    cout <<"full coords: " <<endl;
    int cnt = 0;
    for (auto &m: trackCoords_) {
      if (!m.empty()) {
        cout <<getDesign()->getTech()->getLayer(cnt)->getName() <<": " <<m.size() <<endl;
      }
      cnt++;
    }
  }

  // half coords
  vector<vector<frCoord> > halfTrackCoords(numLayers);
  for (int i = 0; i < numLayers; i++) {
    frCoord prevFullCoord = std::numeric_limits<frCoord>::max();
    for (auto &[currFullCoord, cost]: trackCoords_[i]) {
      if (currFullCoord > prevFullCoord) {
        frCoord currHalfGrid = (currFullCoord + prevFullCoord) / 2 / manuGrid * manuGrid;
        if (currHalfGrid != currFullCoord && currHalfGrid != prevFullCoord) {
          halfTrackCoords[i].push_back(currHalfGrid);
        }
      }
      prevFullCoord = currFullCoord;
    }
    for (auto halfCoord: halfTrackCoords[i]) {
      trackCoords_[i][halfCoord] = frAccessPointEnum::HalfGridAP;
    }
  }
  if (enableOutput) {
    cout <<"full+half coords: " <<endl;
    int cnt = 0;
    for (auto &m: trackCoords_) {
      if (!m.empty()) {
        cout <<getDesign()->getTech()->getLayer(cnt)->getName() <<": " <<m.size() <<endl;
      }
      cnt++;
    }
  }
}
