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

#include <chrono>
#include <iostream>
#include <sstream>

#include "FlexPA.h"
#include "db/infra/frTime.h"
#include "gc/FlexGC.h"

using namespace std;
using namespace fr;

void FlexPA::getPrefTrackPatterns(vector<frTrackPattern*>& prefTrackPatterns)
{
  for (auto& trackPattern : design_->getTopBlock()->getTrackPatterns()) {
    auto isVerticalTrack
        = trackPattern->isHorizontal();  // yes = vertical track
    if (design_->getTech()->getLayer(trackPattern->getLayerNum())->getDir()
        == dbTechLayerDir::HORIZONTAL) {
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

void FlexPA::initUniqueInstance_master2PinLayerRange(
    map<frMaster*, tuple<frLayerNum, frLayerNum>, frBlockObjectComp>&
        master2PinLayerRange)
{
  std::set<frString> masters;
  for (auto inst : target_insts_)
    masters.insert(inst->getMaster()->getName());
  int numLayers = design_->getTech()->getLayers().size();
  for (auto& uMaster : design_->getMasters()) {
    auto master = uMaster.get();
    if (!masters.empty() && masters.find(master->getName()) == masters.end())
      continue;
    frLayerNum minLayerNum = std::numeric_limits<frLayerNum>::max();
    frLayerNum maxLayerNum = std::numeric_limits<frLayerNum>::min();
    for (auto& uTerm : master->getTerms()) {
      if (uTerm.get()->getType().isSupply()) {
        continue;
      }
      for (auto& uPin : uTerm->getPins()) {
        for (auto& uPinFig : uPin->getFigs()) {
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
    if (minLayerNum < getDesign()->getTech()->getBottomLayerNum()
        || maxLayerNum > getDesign()->getTech()->getTopLayerNum()) {
      logger_->warn(DRT,
                    66,
                    "instAnalysis skips {} due to no pin shapes.",
                    master->getName());
      continue;
    }
    maxLayerNum = std::min(maxLayerNum + 2, numLayers);
    master2PinLayerRange[master] = make_tuple(minLayerNum, maxLayerNum);
  }
  // cout <<"  master pin layer range done" <<endl;
}

bool FlexPA::hasTrackPattern(frTrackPattern* tp, const Rect& box)
{
  auto isVerticalTrack = tp->isHorizontal();  // yes = vertical track
  frCoord low = tp->getStartCoord();
  frCoord high = low
                 + (frCoord) (tp->getTrackSpacing())
                       * ((frCoord) (tp->getNumTracks()) - 1);
  if (isVerticalTrack) {
    return !(low > box.xMax() || high < box.xMin());
  } else {
    return !(low > box.yMax() || high < box.yMin());
  }
}

// must init all unique, including filler, macro, etc. to ensure frInst
// pinAccessIdx is active
void FlexPA::initUniqueInstance_main(
    const map<frMaster*, tuple<frLayerNum, frLayerNum>, frBlockObjectComp>&
        master2PinLayerRange,
    const vector<frTrackPattern*>& prefTrackPatterns)
{
  vector<frInst*> ndrInsts;
  vector<frCoord> offset;
  int cnt = 0;
  std::set<frInst*> target_frinsts;
  for (auto inst : target_insts_)
    target_frinsts.insert(design_->getTopBlock()->findInst(inst->getName()));
  for (auto& inst : design_->getTopBlock()->getInsts()) {
    if (!target_insts_.empty()
        && target_frinsts.find(inst.get()) == target_frinsts.end())
      continue;
    if (!AUTO_TAPER_NDR_NETS && isNDRInst(*inst)) {
      ndrInsts.push_back(inst.get());
      continue;
    }
    Point origin = inst->getOrigin();
    Rect boundaryBBox = inst->getBoundaryBBox();
    auto orient = inst->getOrient();
    auto& [minLayerNum, maxLayerNum]
        = master2PinLayerRange.find(inst->getMaster())->second;
    offset.clear();
    for (auto& tp : prefTrackPatterns) {
      if (tp->getLayerNum() >= minLayerNum
          && tp->getLayerNum() <= maxLayerNum) {
        if (hasTrackPattern(tp, boundaryBBox)) {
          // vertical track
          if (tp->isHorizontal()) {
            offset.push_back(origin.x() % tp->getTrackSpacing());
          } else {
            offset.push_back(origin.y() % tp->getTrackSpacing());
          }
        } else {
          offset.push_back(tp->getTrackSpacing());
        }
      } else {
        offset.push_back(tp->getTrackSpacing());
      }
    }
    masterOT2Insts[inst->getMaster()][orient][offset].insert(inst.get());
    cnt++;
  }

  cnt = 0;
  frString orientName;
  for (auto& [master, orientMap] : masterOT2Insts) {
    for (auto& [orient, offsetMap] : orientMap) {
      cnt += offsetMap.size();
      for (auto& [vec, insts] : offsetMap) {
        auto uniqueInst = *(insts.begin());
        uniqueInstances_.push_back(uniqueInst);
        for (auto i : insts) {
          inst2unique_[i] = uniqueInst;
          inst2Class_[i] = &insts;
        }
      }
    }
  }
  for (frInst* inst : ndrInsts) {
    uniqueInstances_.push_back(inst);
    inst2unique_[inst] = inst;
    inst2Class_[inst] = nullptr;
  }

  // init unique2Idx
  for (int i = 0; i < (int) uniqueInstances_.size(); i++) {
    unique2Idx_[uniqueInstances_[i]] = i;
  }

  // if (VERBOSE > 0) {
  //   cout <<"#unique instances = " <<cnt <<endl;
  // }
}

bool FlexPA::isNDRInst(frInst& inst)
{
  for (auto& a : inst.getInstTerms()) {
    if (a->getNet()
        && a->getNet()
               ->getNondefaultRule()) {  // this criterion can be improved
      return true;
    }
  }
  return false;
}

void FlexPA::initUniqueInstance()
{
  vector<frTrackPattern*> prefTrackPatterns;
  getPrefTrackPatterns(prefTrackPatterns);

  map<frMaster*, tuple<frLayerNum, frLayerNum>, frBlockObjectComp>
      master2PinLayerRange;
  initUniqueInstance_master2PinLayerRange(master2PinLayerRange);

  initUniqueInstance_main(master2PinLayerRange, prefTrackPatterns);
}

void FlexPA::checkFigsOnGrid(const frMPin* pin)
{
  const frMTerm* term = pin->getTerm();
  const int grid = getTech()->getManufacturingGrid();
  for (auto& fig : pin->getFigs()) {
    if (fig->typeId() == frcRect) {
      auto box = static_cast<frRect*>(fig.get())->getBBox();
      if (box.xMin() % grid || box.yMin() % grid || box.xMax() % grid
          || box.yMax() % grid) {
        logger_->error(DRT,
                       320,
                       "Term {} of {} contains offgrid pin shape",
                       term->getName(),
                       term->getMaster()->getName());
      }
    } else if (fig->typeId() == frcPolygon) {
      auto polygon = static_cast<frPolygon*>(fig.get());
      for (Point pt : polygon->getPoints()) {
        if (pt.getX() % grid || pt.getY() % grid) {
          logger_->error(DRT,
                         321,
                         "Term {} of {} contains offgrid pin shape",
                         term->getName(),
                         term->getMaster()->getName());
        }
      }
    } else {
      logger_->error(DRT, 322, "checkFigsOnGrid unsupported pinFig.");
    }
  }
}

void FlexPA::initPinAccess()
{
  for (auto& inst : uniqueInstances_) {
    for (auto& instTerm : inst->getInstTerms()) {
      for (auto& pin : instTerm->getTerm()->getPins()) {
        if (unique2paidx_.find(inst) == unique2paidx_.end()) {
          unique2paidx_[inst] = pin->getNumPinAccess();
        } else {
          if (unique2paidx_[inst] != pin->getNumPinAccess()) {
            logger_->error(DRT, 69, "initPinAccess error.");
            exit(1);
          }
        }
        checkFigsOnGrid(pin.get());
        auto pa = make_unique<frPinAccess>();
        pin->addPinAccess(std::move(pa));
      }
    }
    inst->setPinAccessIdx(unique2paidx_[inst]);
  }
  for (auto& [inst, uniqueInst] : inst2unique_) {
    inst->setPinAccessIdx(uniqueInst->getPinAccessIdx());
  }

  // IO terms
  if (target_insts_.empty())
    for (auto& term : getDesign()->getTopBlock()->getTerms()) {
      for (auto& pin : term->getPins()) {
        auto pa = make_unique<frPinAccess>();
        pin->addPinAccess(std::move(pa));
      }
    }
}

void FlexPA::initViaRawPriority()
{
  for (auto layerNum = design_->getTech()->getBottomLayerNum();
       layerNum <= design_->getTech()->getTopLayerNum();
       ++layerNum) {
    if (design_->getTech()->getLayer(layerNum)->getType()
        != dbTechLayerType::CUT) {
      continue;
    }
    for (auto& viaDef : design_->getTech()->getLayer(layerNum)->getViaDefs()) {
      int cutNum = int(viaDef->getCutFigs().size());
      viaRawPriorityTuple priority;
      getViaRawPriority(viaDef, priority);
      layerNum2ViaDefs_[layerNum][cutNum][priority] = viaDef;
    }
  }
}

void FlexPA::getViaRawPriority(frViaDef* viaDef, viaRawPriorityTuple& priority)
{
  bool isNotDefaultVia = !(viaDef->getDefault());
  bool isNotUpperAlign = false;
  bool isNotLowerAlign = false;
  gtl::polygon_90_set_data<frCoord> viaLayerPS1;

  for (auto& fig : viaDef->getLayer1Figs()) {
    Rect bbox = fig->getBBox();
    gtl::rectangle_data<frCoord> bboxRect(
        bbox.xMin(), bbox.yMin(), bbox.xMax(), bbox.yMax());
    using namespace boost::polygon::operators;
    viaLayerPS1 += bboxRect;
  }
  gtl::rectangle_data<frCoord> layer1Rect;
  gtl::extents(layer1Rect, viaLayerPS1);
  bool isLayer1Horz = (gtl::xh(layer1Rect) - gtl::xl(layer1Rect))
                      > (gtl::yh(layer1Rect) - gtl::yl(layer1Rect));
  frCoord layer1Width = std::min((gtl::xh(layer1Rect) - gtl::xl(layer1Rect)),
                                 (gtl::yh(layer1Rect) - gtl::yl(layer1Rect)));
  isNotLowerAlign
      = (isLayer1Horz
         && (getDesign()->getTech()->getLayer(viaDef->getLayer1Num())->getDir()
             == dbTechLayerDir::VERTICAL))
        || (!isLayer1Horz
            && (getDesign()
                    ->getTech()
                    ->getLayer(viaDef->getLayer1Num())
                    ->getDir()
                == dbTechLayerDir::HORIZONTAL));

  gtl::polygon_90_set_data<frCoord> viaLayerPS2;
  for (auto& fig : viaDef->getLayer2Figs()) {
    Rect bbox = fig->getBBox();
    gtl::rectangle_data<frCoord> bboxRect(
        bbox.xMin(), bbox.yMin(), bbox.xMax(), bbox.yMax());
    using namespace boost::polygon::operators;
    viaLayerPS2 += bboxRect;
  }
  gtl::rectangle_data<frCoord> layer2Rect;
  gtl::extents(layer2Rect, viaLayerPS2);
  bool isLayer2Horz = (gtl::xh(layer2Rect) - gtl::xl(layer2Rect))
                      > (gtl::yh(layer2Rect) - gtl::yl(layer2Rect));
  frCoord layer2Width = std::min((gtl::xh(layer2Rect) - gtl::xl(layer2Rect)),
                                 (gtl::yh(layer2Rect) - gtl::yl(layer2Rect)));
  isNotUpperAlign
      = (isLayer2Horz
         && (getDesign()->getTech()->getLayer(viaDef->getLayer2Num())->getDir()
             == dbTechLayerDir::VERTICAL))
        || (!isLayer2Horz
            && (getDesign()
                    ->getTech()
                    ->getLayer(viaDef->getLayer2Num())
                    ->getDir()
                == dbTechLayerDir::HORIZONTAL));

  frCoord layer1Area = gtl::area(viaLayerPS1);
  frCoord layer2Area = gtl::area(viaLayerPS2);

  priority = std::make_tuple(isNotDefaultVia,
                             layer1Width,
                             layer2Width,
                             isNotUpperAlign,
                             layer2Area,
                             layer1Area,
                             isNotLowerAlign);
}

void FlexPA::initTrackCoords()
{
  int numLayers = getDesign()->getTech()->getLayers().size();
  frCoord manuGrid = getDesign()->getTech()->getManufacturingGrid();

  // full coords
  trackCoords_.clear();
  trackCoords_.resize(numLayers);
  for (auto& trackPattern : design_->getTopBlock()->getTrackPatterns()) {
    auto layerNum = trackPattern->getLayerNum();
    auto isVLayer = (design_->getTech()->getLayer(layerNum)->getDir()
                     == dbTechLayerDir::VERTICAL);
    auto isVTrack = trackPattern->isHorizontal();  // yes = vertical track
    if ((!isVLayer && !isVTrack) || (isVLayer && isVTrack)) {
      frCoord currCoord = trackPattern->getStartCoord();
      for (int i = 0; i < (int) trackPattern->getNumTracks(); i++) {
        trackCoords_[layerNum][currCoord] = frAccessPointEnum::OnGrid;
        currCoord += trackPattern->getTrackSpacing();
      }
    }
  }

  // half coords
  vector<vector<frCoord>> halfTrackCoords(numLayers);
  for (int i = 0; i < numLayers; i++) {
    frCoord prevFullCoord = std::numeric_limits<frCoord>::max();
    for (auto& [currFullCoord, cost] : trackCoords_[i]) {
      if (currFullCoord > prevFullCoord) {
        frCoord currHalfGrid
            = (currFullCoord + prevFullCoord) / 2 / manuGrid * manuGrid;
        if (currHalfGrid != currFullCoord && currHalfGrid != prevFullCoord) {
          halfTrackCoords[i].push_back(currHalfGrid);
        }
      }
      prevFullCoord = currFullCoord;
    }
    for (auto halfCoord : halfTrackCoords[i]) {
      trackCoords_[i][halfCoord] = frAccessPointEnum::HalfGrid;
    }
  }
}
