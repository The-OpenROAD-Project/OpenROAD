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
#include "frProfileTask.h"
#include "gc/FlexGC_impl.h"

using namespace std;
using namespace fr;

bool FlexGCWorker::Impl::isCornerOverlap(gcCorner* corner, const frBox &box) {
  frCoord cornerX = corner->getNextEdge()->low().x();
  frCoord cornerY = corner->getNextEdge()->low().y();
  switch (corner->getDir()) {
    case frCornerDirEnum::NE:
      if (cornerX == box.right() && cornerY == box.top()) {
        return true;
      }
      break;
    case frCornerDirEnum::SE:
      if (cornerX == box.right() && cornerY == box.bottom()) {
        return true;
      }
      break;
    case frCornerDirEnum::SW:
      if (cornerX == box.left() && cornerY == box.bottom()) {
        return true;
      }
      break;
    case frCornerDirEnum::NW:
      if (cornerX == box.left() && cornerY == box.top()) {
        return true;
      }
      break;
    default:
      ;
  }
  return false;
}

bool FlexGCWorker::Impl::isCornerOverlap(gcCorner* corner, const gtl::rectangle_data<frCoord> &rect) {
  frCoord cornerX = corner->getNextEdge()->low().x();
  frCoord cornerY = corner->getNextEdge()->low().y();
  switch (corner->getDir()) {
    case frCornerDirEnum::NE:
      if (cornerX == gtl::xh(rect) && cornerY == gtl::yh(rect)) {
        return true;
      }
      break;
    case frCornerDirEnum::SE:
      if (cornerX == gtl::xh(rect) && cornerY == gtl::yl(rect)) {
        return true;
      }
      break;
    case frCornerDirEnum::SW:
      if (cornerX == gtl::xl(rect) && cornerY == gtl::yl(rect)) {
        return true;
      }
      break;
    case frCornerDirEnum::NW:
      if (cornerX == gtl::xl(rect) && cornerY == gtl::yh(rect)) {
        return true;
      }
      break;
    default:
      ;
  }
  return false;
}

void FlexGCWorker::Impl::myBloat(const gtl::rectangle_data<frCoord> &rect, frCoord val, box_t &box) {
  bg::set<bg::min_corner, 0>(box, gtl::xl(rect) - val);
  bg::set<bg::min_corner, 1>(box, gtl::yl(rect) - val);
  bg::set<bg::max_corner, 0>(box, gtl::xh(rect) + val);
  bg::set<bg::max_corner, 1>(box, gtl::yh(rect) + val);
}

frCoord FlexGCWorker::Impl::checkMetalSpacing_getMaxSpcVal(frLayerNum layerNum, bool isNDR) {
  frCoord maxSpcVal = 0;
  auto currLayer = getDesign()->getTech()->getLayer(layerNum);
  if (currLayer->hasMinSpacing()) {
    auto con = currLayer->getMinSpacing();
    switch (con->typeId()) {
      case frConstraintTypeEnum::frcSpacingConstraint:
        maxSpcVal = static_cast<frSpacingConstraint*>(con)->getMinSpacing();
        break;
      case frConstraintTypeEnum::frcSpacingTablePrlConstraint:
        maxSpcVal = static_cast<frSpacingTablePrlConstraint*>(con)->findMax();
        break;
      default:
        logger_->warn(DRT, 41, "Warning: Unsupported metSpc rule.");
    }
    if (isNDR) return max(maxSpcVal, getDesign()->getTech()->getMaxNondefaultSpacing(layerNum/2-1));
  }
  return maxSpcVal;
}

void FlexGCWorker::Impl::checkMetalCornerSpacing_getMaxSpcVal(frLayerNum layerNum, frCoord &maxSpcValX, frCoord &maxSpcValY) {
  maxSpcValX = 0;
  maxSpcValY = 0;
  auto currLayer = getDesign()->getTech()->getLayer(layerNum);
  auto &lef58CornerSpacingCons = currLayer->getLef58CornerSpacingConstraints();
  if (!lef58CornerSpacingCons.empty()) {
    for (auto &con: lef58CornerSpacingCons) {
      maxSpcValX = std::max(maxSpcValX, con->findMax(true));
      maxSpcValY = std::max(maxSpcValY, con->findMax(false));
    }
  }
  return;
}

bool FlexGCWorker::Impl::isOppositeDir(gcCorner* corner, gcSegment* seg) {
  auto cornerDir = corner->getDir();
  auto segDir = seg->getDir();
  if ((cornerDir == frCornerDirEnum::NE && (segDir == frDirEnum::S || segDir == frDirEnum::E)) ||
      (cornerDir == frCornerDirEnum::SE && (segDir == frDirEnum::S || segDir == frDirEnum::W)) ||
      (cornerDir == frCornerDirEnum::SW && (segDir == frDirEnum::N || segDir == frDirEnum::W)) ||
      (cornerDir == frCornerDirEnum::NW && (segDir == frDirEnum::N || segDir == frDirEnum::E))) {
    return true;
  } else {
    return false;
  }
}

box_t FlexGCWorker::Impl::checkMetalCornerSpacing_getQueryBox(gcCorner* corner, frCoord &maxSpcValX, frCoord &maxSpcValY) {
  box_t queryBox;
  frCoord baseX = corner->getNextEdge()->low().x();
  frCoord baseY = corner->getNextEdge()->low().y();
  frCoord llx = baseX;
  frCoord lly = baseY;
  frCoord urx = baseX;
  frCoord ury = baseY;
  switch (corner->getDir()) {
    case frCornerDirEnum::NE:
      urx += maxSpcValX;
      ury += maxSpcValY;
      break;
    case frCornerDirEnum::SE:
      urx += maxSpcValX;
      lly -= maxSpcValY;
      break;
    case frCornerDirEnum::SW:
      llx -= maxSpcValX;
      lly -= maxSpcValY;
      break;
    case frCornerDirEnum::NW:
      llx -= maxSpcValX;
      ury += maxSpcValY;
      break;
    default:
      logger_->error(DRT, 42, "UNKNOWN corner dir");
  }

  bg::set<bg::min_corner, 0>(queryBox, llx);
  bg::set<bg::min_corner, 1>(queryBox, lly);
  bg::set<bg::max_corner, 0>(queryBox, urx);
  bg::set<bg::max_corner, 1>(queryBox, ury);

  return queryBox;
}

frCoord FlexGCWorker::Impl::checkMetalSpacing_prl_getReqSpcVal(gcRect* rect1, gcRect* rect2, frCoord prl/*, bool &hasRoute*/) {
  auto layerNum = rect1->getLayerNum();
  frCoord reqSpcVal = 0;
  auto currLayer = getDesign()->getTech()->getLayer(layerNum);
  bool isObs  = false;
  auto width1 = rect1->width();
  auto width2 = rect2->width();
  // override width and spacing
  if (rect1->getNet()->getOwner() && 
      (rect1->getNet()->getOwner()->typeId() == frcInstBlockage || rect1->getNet()->getOwner()->typeId() == frcBlockage)) {
    isObs = true;
    if (USEMINSPACING_OBS) {
      width1 = currLayer->getWidth();
    }
  }
  if (rect2->getNet()->getOwner() && 
      (rect2->getNet()->getOwner()->typeId() == frcInstBlockage || rect2->getNet()->getOwner()->typeId() == frcBlockage)) {
    isObs = true;
    if (USEMINSPACING_OBS) {
      width2 = currLayer->getWidth();
    }
  }
  // check if width is a result of route shape
  // if the width a shape is smaller if only using fixed shape, then it's route shape -- wrong...
  if (currLayer->hasMinSpacing()) {
    auto con = currLayer->getMinSpacing();
    switch (con->typeId()) {
      case frConstraintTypeEnum::frcSpacingTablePrlConstraint:
        reqSpcVal = static_cast<frSpacingTablePrlConstraint*>(con)->find(std::max(width1, width2), prl);
        // same-net override
        if (!isObs && rect1->getNet() == rect2->getNet()) {
          if (currLayer->hasSpacingSamenet()) {
            auto conSamenet = currLayer->getSpacingSamenet();
            if (!conSamenet->hasPGonly()) {
              reqSpcVal = std::max(conSamenet->getMinSpacing(), static_cast<frSpacingTablePrlConstraint*>(con)->findMin());
            } else {
              bool isPG = false;
              auto owner = rect1->getNet()->getOwner();
              if (owner->typeId() == frcNet) {
                if (static_cast<frNet*>(owner)->getType() == frNetEnum::frcPowerNet || 
                    static_cast<frNet*>(owner)->getType() == frNetEnum::frcGroundNet) {
                  isPG = true;
                }
              } else if (owner->typeId() == frcInstTerm) {
                if (static_cast<frInstTerm*>(owner)->getTerm()->getType() == frTermEnum::frcPowerTerm || 
                    static_cast<frInstTerm*>(owner)->getTerm()->getType() == frTermEnum::frcGroundTerm) {
                  isPG = true;
                }
              } else if (owner->typeId() == frcTerm) {
                if (static_cast<frTerm*>(owner)->getType() == frTermEnum::frcPowerTerm || 
                    static_cast<frTerm*>(owner)->getType() == frTermEnum::frcGroundTerm) {
                  isPG = true;
                }
              }
              if (isPG) {
                reqSpcVal = std::max(conSamenet->getMinSpacing(), static_cast<frSpacingTablePrlConstraint*>(con)->findMin());
              }
            }
          }
        }
        break;
      default:
        logger_->warn(DRT, 43, "Unsupported metSpc rule.");
    }
  }
  return reqSpcVal;
}

// type: 0 -- check H edge only
// type: 1 -- check V edge only
// type: 2 -- check both
bool FlexGCWorker::Impl::checkMetalSpacing_prl_hasPolyEdge(gcRect* rect1, gcRect* rect2, const gtl::rectangle_data<frCoord> &markerRect, int type, frCoord prl) {
  auto layerNum = rect1->getLayerNum();
  auto net1 = rect1->getNet();
  auto net2 = rect2->getNet();
  auto &workerRegionQuery = getWorkerRegionQuery();
  vector<pair<segment_t, gcSegment*> > result;
  box_t queryBox(point_t(gtl::xl(markerRect), gtl::yl(markerRect)), 
                 point_t(gtl::xh(markerRect), gtl::yh(markerRect)));
  workerRegionQuery.queryPolygonEdge(queryBox, layerNum, result);
  // whether markerRect edge has true poly edge of either net1 or net2
  bool flagL = false;
  bool flagR = false;
  bool flagB = false;
  bool flagT = false;
  // type == 2 allows zero overlapping
  if (prl <= 0) {
    for (auto &[seg, objPtr]: result) {
      if ((objPtr->getNet() != net1 && objPtr->getNet() != net2)) {
        continue;
      }
      if (objPtr->getDir() == frDirEnum::W && objPtr->low().y() == gtl::yl(markerRect)) {
        flagB = true;
      } else if (objPtr->getDir() == frDirEnum::E && objPtr->low().y() == gtl::yh(markerRect)) {
        flagT = true;
      } else if (objPtr->getDir() == frDirEnum::N && objPtr->low().x() == gtl::xl(markerRect)) {
        flagL = true;
      } else if (objPtr->getDir() == frDirEnum::S && objPtr->low().x() == gtl::xh(markerRect)) {
        flagR = true;
      }
    }
  // type == 0 / 1 requires non-zero overlapping poly edge 
  } else {
    for (auto &[seg, objPtr]: result) {
      if ((objPtr->getNet() != net1 && objPtr->getNet() != net2)) {
        continue;
      }
      // allow fake edge if inside markerRect has same dir edge
      if (objPtr->getDir() == frDirEnum::W && 
          (objPtr->low().y() >= gtl::yl(markerRect) && objPtr->low().y() < gtl::yh(markerRect)) &&
          !(objPtr->low().x() <= gtl::xl(markerRect) || objPtr->high().x() >= gtl::xh(markerRect))) {
        flagB = true;
      } else if (objPtr->getDir() == frDirEnum::E && 
                 (objPtr->low().y() > gtl::yl(markerRect) && objPtr->low().y() <= gtl::yh(markerRect)) &&
                 !(objPtr->high().x() <= gtl::xl(markerRect) || objPtr->low().x() >= gtl::xh(markerRect))) {
        flagT = true;
      } else if (objPtr->getDir() == frDirEnum::N && 
                 (objPtr->low().x() >= gtl::xl(markerRect) && objPtr->low().x() < gtl::xh(markerRect)) &&
                 !(objPtr->high().y() <= gtl::yl(markerRect) || objPtr->low().y() >= gtl::yh(markerRect))) {
        flagL = true;
      } else if (objPtr->getDir() == frDirEnum::S && 
                 (objPtr->low().x() > gtl::xl(markerRect) && objPtr->low().x() <= gtl::xh(markerRect)) && 
                 !(objPtr->low().y() <= gtl::yl(markerRect) || objPtr->high().y() >= gtl::yh(markerRect))) {
        flagR = true;
      }
    }
  }
  if ((type == 0 || type == 2) && (flagB && flagT)) {
    return true;
  } else if ((type == 1 || type == 2) && (flagL && flagR)) {
    return true;
  } else {
    return false;
  }
}
//int print(){
//    cout << "ASDASJHDIASHDHSA\n";
//    int b = 1;
//    return b;
//}
void FlexGCWorker::Impl::checkMetalSpacing_prl(gcRect* rect1, gcRect* rect2, const gtl::rectangle_data<frCoord> &markerRect,
                                     frCoord prl, frCoord distX, frCoord distY, bool isNDR) {
  
  // no violation if fixed shapes
  if (rect1->isFixed() && rect2->isFixed()) {
    return;
  }
  bool enableOutput = printMarker_;
  auto layerNum = rect1->getLayerNum();
  auto net1 = rect1->getNet();
  auto net2 = rect2->getNet();
//  if ((gtl::xh(*rect1) == 63.335*2000 && gtl::yh(*rect1) == 67.61*2000 || gtl::xh(*rect2) == 63.335*2000 && gtl::yh(*rect2) == 67.61*2000) &&
//          (gtl::xl(*rect1) == 126930-70 && gtl::xh(*rect1) == 126930+210 //gtl::yl(*rect1) == 135020-70
//            || gtl::xl(*rect2) == 126930+70 && gtl::xh(*rect2) == 126930+210/*gtl::yl(*rect2) == 135020-70*/))
//      print();
  auto reqSpcVal = checkMetalSpacing_prl_getReqSpcVal(rect1, rect2, prl);  
  if (isNDR){
        frCoord ndrSpc1 = 0, ndrSpc2 = 0;
        if (!rect1->isFixed() && net1->isNondefault() && !rect1->isTapered())
            ndrSpc1 = net1->getFrNet()->getNondefaultRule()->getSpacing(layerNum/2-1);
        if (!rect2->isFixed() && net2->isNondefault() && !rect2->isTapered())
            ndrSpc2 = net2->getFrNet()->getNondefaultRule()->getSpacing(layerNum/2-1);

        reqSpcVal = max(reqSpcVal, max(ndrSpc1, ndrSpc2));
  }
  
  // no violation if spacing satisfied
  if (distX * distX + distY * distY >= reqSpcVal * reqSpcVal) {
    return;
  }
  // no violation if no two true polygon edges (prl > 0 requires non-zero true poly edge; prl <= 0 allows zero true poly edge)
  int type = 0;
  if (prl <= 0) {
    type = 2;
  } else {
    if (distX == 0) {
      type = 0;
    } else if (distY == 0) {
      type = 1;
    }
  }
  if (!checkMetalSpacing_prl_hasPolyEdge(rect1, rect2, markerRect, type, prl)) {
    return;
  }
  // no violation if bloat width cannot find non-fixed route shapes
  bool hasRoute = false;
  if (!hasRoute) {
    // marker enlarged by width
    auto width = rect1->width();
    gtl::rectangle_data<frCoord> enlargedMarkerRect(markerRect);
    gtl::bloat(enlargedMarkerRect, width);
    // widthrect
    gtl::polygon_90_set_data<frCoord> tmpPoly;
    using namespace boost::polygon::operators;
    tmpPoly += enlargedMarkerRect;
    tmpPoly &= *rect1; // tmpPoly now is widthrect
    auto targetArea = gtl::area(tmpPoly);
    // get fixed shapes
    tmpPoly &= net1->getPolygons(layerNum, true);
    if (gtl::area(tmpPoly) < targetArea) {
      hasRoute = true;
    }
  }
  if (!hasRoute) {
    // marker enlarged by width
    auto width = rect2->width();
    gtl::rectangle_data<frCoord> enlargedMarkerRect(markerRect);
    gtl::bloat(enlargedMarkerRect, width);
    // widthrect
    gtl::polygon_90_set_data<frCoord> tmpPoly;
    using namespace boost::polygon::operators;
    tmpPoly += enlargedMarkerRect;
    tmpPoly &= *rect2; // tmpPoly now is widthrect
    auto targetArea = gtl::area(tmpPoly);
    // get fixed shapes
    tmpPoly &= net2->getPolygons(layerNum, true);
    if (gtl::area(tmpPoly) < targetArea) {
      hasRoute = true;
    }
  }
  if (!hasRoute) {
    return;
  }

  auto marker = make_unique<frMarker>();
  frBox box(gtl::xl(markerRect), gtl::yl(markerRect), gtl::xh(markerRect), gtl::yh(markerRect));
  marker->setBBox(box);
  marker->setLayerNum(layerNum);
  marker->setConstraint(getDesign()->getTech()->getLayer(layerNum)->getMinSpacing());
  marker->addSrc(net1->getOwner());
  marker->addVictim(net1->getOwner(), make_tuple(rect1->getLayerNum(), 
                                                 frBox(gtl::xl(*rect1), gtl::yl(*rect1), gtl::xh(*rect1), gtl::yh(*rect1)),
                                                 rect1->isFixed()));
  marker->addSrc(net2->getOwner());
  marker->addAggressor(net2->getOwner(), make_tuple(rect2->getLayerNum(), 
                                                    frBox(gtl::xl(*rect2), gtl::yl(*rect2), gtl::xh(*rect2), gtl::yh(*rect2)),
                                                    rect2->isFixed()));
  if (addMarker(std::move(marker))) {
    // true marker
    if (enableOutput) {
      double dbu = getDesign()->getTopBlock()->getDBUPerUU();
      cout <<"MetSpc@(" <<gtl::xl(markerRect) / dbu <<", " <<gtl::yl(markerRect) / dbu <<") ("
                        <<gtl::xh(markerRect) / dbu <<", " <<gtl::yh(markerRect) / dbu <<") "
           <<getDesign()->getTech()->getLayer(layerNum)->getName() <<" ";
      auto owner = net1->getOwner();
      if (owner == nullptr) {
        cout <<"FLOATING";
      } else {
        if (owner->typeId() == frcNet) {
          cout <<static_cast<frNet*>(owner)->getName();
        } else if (owner->typeId() == frcInstTerm) {
          cout <<static_cast<frInstTerm*>(owner)->getInst()->getName() <<"/" 
               <<static_cast<frInstTerm*>(owner)->getTerm()->getName();
        } else if (owner->typeId() == frcTerm) {
          cout <<"PIN/" <<static_cast<frTerm*>(owner)->getName();
        } else if (owner->typeId() == frcInstBlockage) {
          cout <<static_cast<frInstBlockage*>(owner)->getInst()->getName() <<"/OBS";
        } else if (owner->typeId() == frcBlockage) {
          cout <<"PIN/OBS";
        } else {
          cout <<"UNKNOWN";
        }
      }
      cout <<" ";
      owner = net2->getOwner();
      if (owner == nullptr) {
        cout <<"FLOATING";
      } else {
        if (owner->typeId() == frcNet) {
          cout <<static_cast<frNet*>(owner)->getName();
        } else if (owner->typeId() == frcInstTerm) {
          cout <<static_cast<frInstTerm*>(owner)->getInst()->getName() <<"/" 
               <<static_cast<frInstTerm*>(owner)->getTerm()->getName();
        } else if (owner->typeId() == frcTerm) {
          cout <<"PIN/" <<static_cast<frTerm*>(owner)->getName();
        } else if (owner->typeId() == frcInstBlockage) {
          cout <<static_cast<frInstBlockage*>(owner)->getInst()->getName() <<"/OBS";
        } else if (owner->typeId() == frcBlockage) {
          cout <<"PIN/OBS";
        } else {
          cout <<"UNKNOWN";
        }
      }
      cout <<endl;
    }
  }
}

bool FlexGCWorker::Impl::checkMetalSpacing_short_skipOBSPin(gcRect* rect1, gcRect* rect2, const gtl::rectangle_data<frCoord> &markerRect) {
  bool isRect1Obs = false;
  bool isRect2Obs = false;
  if (rect1->getNet()->getOwner() && 
      (rect1->getNet()->getOwner()->typeId() == frcInstBlockage ||
       rect1->getNet()->getOwner()->typeId() == frcBlockage)) {
    isRect1Obs = true;
  }
  if (rect2->getNet()->getOwner() && 
      (rect2->getNet()->getOwner()->typeId() == frcInstBlockage ||
       rect2->getNet()->getOwner()->typeId() == frcBlockage)) {
    isRect2Obs = true;
  }
  if (!isRect1Obs && !isRect2Obs) {
    return false;
  }
  if (isRect1Obs && isRect2Obs) {
    return false;
  }
  // always make obs to be rect2
  if (isRect1Obs) {
    std::swap(rect1, rect2);
  }
  // now rect is not obs, rect2 is obs
  auto layerNum = rect1->getLayerNum();
  auto net1 = rect1->getNet();

  // check if markerRect is covered by fixed shapes of net1
  auto &polys1 = net1->getPolygons(layerNum, true);
  vector<gtl::rectangle_data<frCoord> > rects;
  gtl::get_max_rectangles(rects, polys1);
  for (auto &rect: rects) {
    if (gtl::contains(rect, markerRect)) {
      return true;
    }
  }
  return false;
}

void FlexGCWorker::Impl::checkMetalSpacing_short(gcRect* rect1, gcRect* rect2, const gtl::rectangle_data<frCoord> &markerRect) {
  bool enableOutput = printMarker_;

  auto layerNum = rect1->getLayerNum();
  auto net1 = rect1->getNet();
  auto net2 = rect2->getNet();
  if (rect1->isFixed() && rect2->isFixed()) {
    return;
  }
  // skip obs overlaps with pin
  if (checkMetalSpacing_short_skipOBSPin(rect1, rect2, markerRect)) {
    return;
  }

  // skip if marker area does not have route shape, must exclude touching
  {
    // bloat marker by minimum coord if zero
    gtl::rectangle_data<frCoord> bloatMarkerRect(markerRect);
    if (gtl::delta(markerRect, gtl::HORIZONTAL) == 0) {
      gtl::bloat(bloatMarkerRect, gtl::HORIZONTAL, 1);
    }
    if (gtl::delta(markerRect, gtl::VERTICAL) == 0) {
      gtl::bloat(bloatMarkerRect, gtl::VERTICAL, 1);
    }
    using namespace boost::polygon::operators;
    auto &polys1 = net1->getPolygons(layerNum, false);
    auto intersection_polys1 = polys1 & bloatMarkerRect;
    auto &polys2 = net2->getPolygons(layerNum, false);
    auto intersection_polys2 = polys2 & bloatMarkerRect;
    if (gtl::empty(intersection_polys1) && gtl::empty(intersection_polys2)) {
      return;
    }
  }
  // skip same-net sufficient metal
  if (net1 == net2) {
    // skip if good
    auto minWidth = getDesign()->getTech()->getLayer(layerNum)->getMinWidth();
    auto xLen = gtl::delta(markerRect, gtl::HORIZONTAL);
    auto yLen = gtl::delta(markerRect, gtl::VERTICAL);
    if (xLen * xLen + yLen * yLen >= minWidth * minWidth) {
      return;
    }
    // skip if rect < minwidth
    if ((gtl::delta(*rect1, gtl::HORIZONTAL) < minWidth || gtl::delta(*rect1, gtl::VERTICAL) < minWidth) ||
        (gtl::delta(*rect2, gtl::HORIZONTAL) < minWidth || gtl::delta(*rect2, gtl::VERTICAL) < minWidth)) {
      return;
    }
    // query third object that can bridge rect1 and rect2 in bloated marker area
    gtl::point_data<frCoord> centerPt;
    gtl::center(centerPt, markerRect);
    gtl::rectangle_data<frCoord> bloatMarkerRect(centerPt.x(), centerPt.y(), centerPt.x(), centerPt.y());
    box_t queryBox;
    myBloat(bloatMarkerRect, minWidth, queryBox);

    auto &workerRegionQuery = getWorkerRegionQuery();
    vector<rq_box_value_t<gcRect*> > result;
    workerRegionQuery.queryMaxRectangle(queryBox, layerNum, result);
    //cout <<"3rd obj" <<endl;
    for (auto &[objBox, objPtr]: result) {
      if (objPtr == rect1 || objPtr == rect2) {
        continue;
      }
      if (objPtr->getNet() != net1) {
        continue;
      }
      if (!gtl::contains(*objPtr, markerRect)) {
        continue;
      }
      if (gtl::delta(*objPtr, gtl::HORIZONTAL) < minWidth || gtl::delta(*objPtr, gtl::VERTICAL) < minWidth) {
        continue;
      }
      // only check same net third object
      gtl::rectangle_data<frCoord> tmpRect1(*rect1);
      gtl::rectangle_data<frCoord> tmpRect2(*rect2);
      if (gtl::intersect(tmpRect1, *objPtr) && gtl::intersect(tmpRect2, *objPtr)) {
        auto xLen1 = gtl::delta(tmpRect1, gtl::HORIZONTAL);
        auto yLen1 = gtl::delta(tmpRect1, gtl::VERTICAL);
        auto xLen2 = gtl::delta(tmpRect2, gtl::HORIZONTAL);
        auto yLen2 = gtl::delta(tmpRect2, gtl::VERTICAL);
        if (xLen1 * xLen1 + yLen1 * yLen1 >= minWidth * minWidth &&
            xLen2 * xLen2 + yLen2 * yLen2 >= minWidth * minWidth) {
          return;
        }
      }
    }
  }

  auto marker = make_unique<frMarker>();
  frBox box(gtl::xl(markerRect), gtl::yl(markerRect), gtl::xh(markerRect), gtl::yh(markerRect));
  marker->setBBox(box);
  marker->setLayerNum(layerNum);
  if (net1 == net2) {
    marker->setConstraint(getDesign()->getTech()->getLayer(layerNum)->getNonSufficientMetalConstraint());
  } else {
    marker->setConstraint(getDesign()->getTech()->getLayer(layerNum)->getShortConstraint());
  }
  marker->addSrc(net1->getOwner());
  marker->addVictim(net1->getOwner(), make_tuple(rect1->getLayerNum(), 
                                                 frBox(gtl::xl(*rect1), gtl::yl(*rect1), gtl::xh(*rect1), gtl::yh(*rect1)),
                                                 rect1->isFixed()));
  marker->addSrc(net2->getOwner());
  marker->addAggressor(net2->getOwner(), make_tuple(rect2->getLayerNum(), 
                                                    frBox(gtl::xl(*rect2), gtl::yl(*rect2), gtl::xh(*rect2), gtl::yh(*rect2)),
                                                    rect2->isFixed()));
  if (addMarker(std::move(marker))) {
    // true marker
    if (enableOutput) {
      double dbu = getDesign()->getTopBlock()->getDBUPerUU();
      if (net1 == net2) {
        cout <<"NSMetal@(";
      } else {
        cout <<"Short@(";
      }
      cout <<gtl::xl(markerRect) / dbu <<", " <<gtl::yl(markerRect) / dbu <<") ("
           <<gtl::xh(markerRect) / dbu <<", " <<gtl::yh(markerRect) / dbu <<") "
           <<getDesign()->getTech()->getLayer(layerNum)->getName() <<" ";
      auto owner = net1->getOwner();
      if (owner == nullptr) {
        cout <<"FLOATING";
      } else {
        if (owner->typeId() == frcNet) {
          cout <<static_cast<frNet*>(owner)->getName();
        } else if (owner->typeId() == frcInstTerm) {
          cout <<static_cast<frInstTerm*>(owner)->getInst()->getName() <<"/" 
               <<static_cast<frInstTerm*>(owner)->getTerm()->getName();
        } else if (owner->typeId() == frcTerm) {
          cout <<"PIN/" <<static_cast<frTerm*>(owner)->getName();
        } else if (owner->typeId() == frcInstBlockage) {
          cout <<static_cast<frInstBlockage*>(owner)->getInst()->getName() <<"/OBS";
        } else if (owner->typeId() == frcBlockage) {
          cout <<"PIN/OBS";
        } else {
          cout <<"UNKNOWN";
        }
      }
      cout <<" ";
      owner = net2->getOwner();
      if (owner == nullptr) {
        cout <<"FLOATING";
      } else {
        if (owner->typeId() == frcNet) {
          cout <<static_cast<frNet*>(owner)->getName();
        } else if (owner->typeId() == frcInstTerm) {
          cout <<static_cast<frInstTerm*>(owner)->getInst()->getName() <<"/" 
               <<static_cast<frInstTerm*>(owner)->getTerm()->getName();
        } else if (owner->typeId() == frcTerm) {
          cout <<"PIN/" <<static_cast<frTerm*>(owner)->getName();
        } else if (owner->typeId() == frcInstBlockage) {
          cout <<static_cast<frInstBlockage*>(owner)->getInst()->getName() <<"/OBS";
        } else if (owner->typeId() == frcBlockage) {
          cout <<"PIN/OBS";
        } else {
          cout <<"UNKNOWN";
        }
      }
      cout <<endl;
    }
  }
}

void FlexGCWorker::Impl::checkMetalSpacing_main(gcRect* ptr1, gcRect* ptr2, bool isNDR) {
  //bool enableOutput = true;

  // NSMetal does not need self-intersection
  // Minimum width rule handles outsite this function
  if (ptr1 == ptr2) {
    return;
  }
  gtl::rectangle_data<frCoord> markerRect(*ptr1);
  auto distX = gtl::euclidean_distance(markerRect, *ptr2, gtl::HORIZONTAL);
  auto distY = gtl::euclidean_distance(markerRect, *ptr2, gtl::VERTICAL);

  gtl::generalized_intersect(markerRect, *ptr2);
  auto prlX = gtl::delta(markerRect, gtl::HORIZONTAL);
  auto prlY = gtl::delta(markerRect, gtl::VERTICAL);

  if (distX) {
    prlX = -prlX;
  }
  if (distY) {
    prlY = -prlY;
  }
  
  // short, nsmetal
  if (distX == 0 && distY == 0) {
    checkMetalSpacing_short(ptr1, ptr2, markerRect);
  // prl
  } else {
    checkMetalSpacing_prl(ptr1, ptr2, markerRect, std::max(prlX, prlY), distX, distY, isNDR);
  }
}

void FlexGCWorker::Impl::checkMetalSpacing_main(gcRect* rect, bool isNDR, bool querySpcRects) {
  //bool enableOutput = true;
  bool enableOutput = false;
  auto layerNum = rect->getLayerNum();
  auto maxSpcVal = checkMetalSpacing_getMaxSpcVal(layerNum, isNDR);
  box_t queryBox;
  myBloat(*rect, maxSpcVal, queryBox);
  if (enableOutput) {
    double dbu = getDesign()->getTopBlock()->getDBUPerUU();
    cout <<"checkMetalPrl maxRect ";
    if (rect->isFixed()) {
      cout <<"FIXED";
    } else {
      cout <<"ROUTE";
    }
    cout <<" (" <<gtl::xl(*rect) / dbu <<", " <<gtl::yl(*rect) / dbu <<") (" 
                <<gtl::xh(*rect) / dbu <<", " <<gtl::yh(*rect) / dbu <<") "
         <<getDesign()->getTech()->getLayer(layerNum)->getName() <<" ";
    cout <<"bloat maxSpcVal@" <<maxSpcVal / dbu <<" (" <<queryBox.min_corner().x() / dbu <<", " <<queryBox.min_corner().x() / dbu <<") (" 
                      <<queryBox.max_corner().x() / dbu <<", " <<queryBox.max_corner().x() / dbu <<") ";
    auto owner = rect->getNet()->getOwner();
    if (owner == nullptr) {
      cout <<" FLOATING";
    } else {
      if (owner->typeId() == frcNet) {
        cout <<static_cast<frNet*>(owner)->getName();
      } else if (owner->typeId() == frcInstTerm) {
        cout <<static_cast<frInstTerm*>(owner)->getInst()->getName() <<"/" 
             <<static_cast<frInstTerm*>(owner)->getTerm()->getName();
      } else if (owner->typeId() == frcTerm) {
        cout <<"PIN/" <<static_cast<frTerm*>(owner)->getName();
      } else if (owner->typeId() == frcInstBlockage) {
        cout <<static_cast<frInstBlockage*>(owner)->getInst()->getName() <<"/OBS";
      } else if (owner->typeId() == frcBlockage) {
        cout <<"PIN/OBS";
      } else {
        cout <<"UNKNOWN";
      }
    }
    cout <<endl;
  }

  auto &workerRegionQuery = getWorkerRegionQuery();
  vector<rq_box_value_t<gcRect*> > result;
  workerRegionQuery.queryMaxRectangle(queryBox, layerNum, result);
  if (querySpcRects){
    vector<rq_box_value_t<gcRect> > resultS;
    workerRegionQuery.querySpcRectangle(queryBox, layerNum, resultS);
    for (auto &[objBox, ptr]: resultS) {
        checkMetalSpacing_main(rect, &ptr, isNDR);
    }
  }
  // Short, metSpc, NSMetal here
  for (auto &[objBox, ptr]: result) {
    checkMetalSpacing_main(rect, ptr, isNDR);
  }
}

void FlexGCWorker::Impl::checkMetalSpacing() {
  if (targetNet_) {
    // layer --> net --> polygon --> maxrect
    for (int i = std::max((frLayerNum)(getDesign()->getTech()->getBottomLayerNum()), minLayerNum_); 
         i <= std::min((frLayerNum)(getDesign()->getTech()->getTopLayerNum()), maxLayerNum_); i++) {
      auto currLayer = getDesign()->getTech()->getLayer(i);
      if (currLayer->getType() != frLayerTypeEnum::ROUTING) {
        continue;
      }
      for (auto &pin: targetNet_->getPins(i)) {
        for (auto &maxrect: pin->getMaxRectangles()) {
          checkMetalSpacing_main(maxrect.get(), getDRWorker() || !AUTO_TAPER_NDR_NETS);
        }
      }
      for (auto& sr : targetNet_->getSpecialSpcRects())
          checkMetalSpacing_main(sr.get(), getDRWorker() || !AUTO_TAPER_NDR_NETS, true);
    }
  } else {
    // layer --> net --> polygon --> maxrect
    for (int i = std::max((frLayerNum)(getDesign()->getTech()->getBottomLayerNum()), minLayerNum_); 
         i <= std::min((frLayerNum)(getDesign()->getTech()->getTopLayerNum()), maxLayerNum_); i++) {
      auto currLayer = getDesign()->getTech()->getLayer(i);
      if (currLayer->getType() != frLayerTypeEnum::ROUTING) {
        continue;
      }
      for (auto &net: getNets()) {
        for (auto &pin: net->getPins(i)) {
          for (auto &maxrect: pin->getMaxRectangles()) {
            // Short, NSMetal, metSpc
            checkMetalSpacing_main(maxrect.get(), getDRWorker() || !AUTO_TAPER_NDR_NETS);
          }
        }
        for (auto& sr : net->getSpecialSpcRects())
          checkMetalSpacing_main(sr.get(), getDRWorker() || !AUTO_TAPER_NDR_NETS, true);
      }
    }
  }
}

// (new) currently only support ISPD2019-related part
// check between the corner and the target rect
void FlexGCWorker::Impl::checkMetalCornerSpacing_main(gcCorner* corner, gcRect* rect, frLef58CornerSpacingConstraint* con) {
  bool enableOutput = false;
  // skip if corner type mismatch
  if (corner->getType() != con->getCornerType()) {
    return;
  }
  // only trigger if the corner is not contained by the rect
  // TODO: check corner-touching case
  auto cornerPt = corner->getNextEdge()->low();
  if (gtl::contains(*rect, cornerPt)) {
    return;
  }
  frCoord cornerX = gtl::x(cornerPt);
  frCoord cornerY = gtl::y(cornerPt);
  // skip if convex corner and prl is greater than 0
  if (con->getCornerType() == frCornerTypeEnum::CONVEX) {
    if (cornerX > gtl::xl(*rect) && cornerX < gtl::xh(*rect)) {
      return;
    }
    if (cornerY > gtl::yl(*rect) && cornerY < gtl::yh(*rect)) {
      return;
    }
  }
  // skip for EXCEPTEOL eolWidth
  if (con->hasExceptEol()) {
    if (corner->getType() == frCornerTypeEnum::CONVEX) {
      if (corner->getNextCorner()->getType() == frCornerTypeEnum::CONVEX && gtl::length(*(corner->getNextEdge())) < con->getEolWidth()) {
        return;
      }
      if (corner->getPrevCorner()->getType() == frCornerTypeEnum::CONVEX && gtl::length(*(corner->getPrevEdge())) < con->getEolWidth()) {
        return;
      }
    }
  }

  // query and iterate only the rects that have the corner as its corner
  auto layerNum = corner->getNextEdge()->getLayerNum();
  auto net = corner->getNextEdge()->getNet();
  auto &workerRegionQuery = getWorkerRegionQuery();
  vector<rq_box_value_t<gcRect*> > result;
  box_t queryBox(point_t(cornerX, cornerY), point_t(cornerX, cornerY));
  workerRegionQuery.queryMaxRectangle(queryBox, layerNum, result);
  for (auto &[objBox, objPtr]: result) {
    // skip if not same net
    if (objPtr->getNet() != net) {
      continue;
    }
    // skip if corner does not overlap with objBox
    if (!isCornerOverlap(corner, objBox)) {
      continue;
    }
    // get generalized rect between corner and rect
    gtl::rectangle_data<frCoord> markerRect(cornerX, cornerY, cornerX, cornerY);
    gtl::generalized_intersect(markerRect, *rect);
    frCoord maxXY = gtl::delta(markerRect, gtl::guess_orientation(markerRect));

    if (con->hasSameXY()) {
      frCoord reqSpcVal = con->find(objPtr->width());
      if (maxXY >= reqSpcVal) {
        continue;
      }
      // no vaiolation if fixed
      // if (corner->isFixed() && rect->isFixed() && objPtr->isFixed()) {
      if (rect->isFixed() && objPtr->isFixed()) {
      // if (corner->isFixed() && rect->isFixed()) {
        continue;
      }
      // no violation if width is not contributed by route obj
      bool hasRoute = false;
      if (!hasRoute) {
        // marker enlarged by width
        auto width = objPtr->width();
        gtl::rectangle_data<frCoord> enlargedMarkerRect(markerRect);
        gtl::bloat(enlargedMarkerRect, width);
        // widthrect
        gtl::polygon_90_set_data<frCoord> tmpPoly;
        using namespace boost::polygon::operators;
        tmpPoly += enlargedMarkerRect;
        tmpPoly &= *objPtr; // tmpPoly now is widthrect
        auto targetArea = gtl::area(tmpPoly);
        // get fixed shapes
        tmpPoly &= net->getPolygons(layerNum, true);
        if (gtl::area(tmpPoly) < targetArea) {
          hasRoute = true;
        } 
      }

      if (!hasRoute) {
        // marker enlarged by width
        auto width = rect->width();
        gtl::rectangle_data<frCoord> enlargedMarkerRect(markerRect);
        gtl::bloat(enlargedMarkerRect, width);
        // widthrect
        gtl::polygon_90_set_data<frCoord> tmpPoly;
        using namespace boost::polygon::operators;
        tmpPoly += enlargedMarkerRect;
        tmpPoly &= *rect; // tmpPoly now is widthrect
        auto targetArea = gtl::area(tmpPoly);
        // get fixed shapes
        tmpPoly &= rect->getNet()->getPolygons(layerNum, true);
        if (gtl::area(tmpPoly) < targetArea) {
          hasRoute = true;
        }
      }

      if (!hasRoute) {
        continue;
      }

      // real violation
      auto marker = make_unique<frMarker>();
      frBox box(gtl::xl(markerRect), gtl::yl(markerRect), gtl::xh(markerRect), gtl::yh(markerRect));
      marker->setBBox(box);
      marker->setLayerNum(layerNum);
      marker->setConstraint(con);
      marker->addSrc(net->getOwner());
      marker->addVictim(net->getOwner(), make_tuple(layerNum, 
                                                    frBox(corner->x(), corner->y(), corner->x(), corner->y()),
                                                    corner->isFixed()));
      marker->addSrc(rect->getNet()->getOwner());
      marker->addAggressor(rect->getNet()->getOwner(), make_tuple(rect->getLayerNum(), 
                                                       frBox(gtl::xl(*rect), gtl::yl(*rect), gtl::xh(*rect), gtl::yh(*rect)),
                                                       rect->isFixed()));
      if (addMarker(std::move(marker))) {
        if (enableOutput) {
          double dbu = getDesign()->getTopBlock()->getDBUPerUU();
          cout <<"CornerSpc@(" <<gtl::xl(markerRect) / dbu <<", " <<gtl::yl(markerRect) / dbu <<") ("
                            <<gtl::xh(markerRect) / dbu <<", " <<gtl::yh(markerRect) / dbu <<") "
               <<getDesign()->getTech()->getLayer(layerNum)->getName() <<" ";
          auto owner = net->getOwner();
          if (owner == nullptr) {
            cout <<"FLOATING";
          } else {
            if (owner->typeId() == frcNet) {
              cout <<static_cast<frNet*>(owner)->getName();
            } else if (owner->typeId() == frcInstTerm) {
              cout <<static_cast<frInstTerm*>(owner)->getInst()->getName() <<"/" 
                   <<static_cast<frInstTerm*>(owner)->getTerm()->getName();
            } else if (owner->typeId() == frcTerm) {
              cout <<"PIN/" <<static_cast<frTerm*>(owner)->getName();
            } else if (owner->typeId() == frcInstBlockage) {
              cout <<static_cast<frInstBlockage*>(owner)->getInst()->getName() <<"/OBS";
            } else if (owner->typeId() == frcBlockage) {
              cout <<"PIN/OBS";
            } else {
              cout <<"UNKNOWN";
            }
          }
          cout <<" ";
          owner = rect->getNet()->getOwner();
          if (owner == nullptr) {
            cout <<"FLOATING";
          } else {
            if (owner->typeId() == frcNet) {
              cout <<static_cast<frNet*>(owner)->getName();
            } else if (owner->typeId() == frcInstTerm) {
              cout <<static_cast<frInstTerm*>(owner)->getInst()->getName() <<"/" 
                   <<static_cast<frInstTerm*>(owner)->getTerm()->getName();
            } else if (owner->typeId() == frcTerm) {
              cout <<"PIN/" <<static_cast<frTerm*>(owner)->getName();
            } else if (owner->typeId() == frcInstBlockage) {
              cout <<static_cast<frInstBlockage*>(owner)->getInst()->getName() <<"/OBS";
            } else if (owner->typeId() == frcBlockage) {
              cout <<"PIN/OBS";
            } else {
              cout <<"UNKNOWN";
            }
          }
          cout <<endl;
        }
      }
      return;
    } else {
      // TODO: implement others if necessary
    }
    
  }

}

// currently only support ISPD19-related part
void FlexGCWorker::Impl::checkMetalCornerSpacing_main(gcCorner* corner, gcSegment* seg, frLef58CornerSpacingConstraint* con) {
  bool enableOutput = false;
  // only trigger between opposite corner-edge
  if (!isOppositeDir(corner, seg)) {
    return;
  }
  // skip if corner type mismatch
  if (corner->getType() != con->getCornerType()) {
    return;
  }
  // skip for EXCEPTEOL eolWidth
  if (con->hasExceptEol()) {
    if (corner->getType() == frCornerTypeEnum::CONVEX) {
      if (corner->getNextCorner()->getType() == frCornerTypeEnum::CONVEX && gtl::length(*(corner->getNextEdge())) < con->getEolWidth()) {
        return;
      }
      if (corner->getPrevCorner()->getType() == frCornerTypeEnum::CONVEX && gtl::length(*(corner->getPrevEdge())) < con->getEolWidth()) {
        return;
      }
    }
  }
  // skip if convex and prl is greater than 0
  frCoord cornerX = corner->getNextEdge()->low().x();
  frCoord cornerY = corner->getNextEdge()->low().y();
  if (con->getCornerType() == frCornerTypeEnum::CONVEX) {
    if (seg->getDir() == frDirEnum::E || seg->getDir() == frDirEnum::W) {
      if (cornerX > seg->low().x() && cornerX < seg->high().x()) {
        return;
      }
    }
    if (seg->getDir() == frDirEnum::N || seg->getDir() == frDirEnum::S) {
      if (cornerY > seg->low().x() && cornerY < seg->high().x()) {
        return;
      }
    }
  }
  // get generalized rect between corner and seg
  gtl::rectangle_data<frCoord> markerRect(cornerX, cornerY, cornerX, cornerY);
  gtl::rectangle_data<frCoord> segRect(seg->low().x(), seg->low().y(), seg->high().x(), seg->high().y()); 
  gtl::generalized_intersect(markerRect, segRect);
  frCoord maxXY = gtl::delta(markerRect, gtl::guess_orientation(markerRect));

  // query and iterate overlapping rect of the same net
  auto layerNum = corner->getNextEdge()->getLayerNum();
  auto net = corner->getNextEdge()->getNet();
  auto segNet = seg->getNet();
  auto &workerRegionQuery = getWorkerRegionQuery();
  vector<rq_box_value_t<gcRect*> > result;
  box_t queryBox(point_t(cornerX, cornerY), point_t(cornerX, cornerY));
  workerRegionQuery.queryMaxRectangle(queryBox, layerNum, result);
  for (auto &[objBox, objPtr]: result) {
    if (objPtr->getNet() != net) {
      continue;
    }
    if (con->hasSameXY()) {
      frCoord reqSpcVal = con->find(objPtr->width());
      if (maxXY >= reqSpcVal) {
        continue;
      }
      // no violation if fixed
      if (seg->isFixed() && objPtr->isFixed()) {
        continue;
      }

      // real violation
      auto marker = make_unique<frMarker>();
      frBox box(gtl::xl(markerRect), gtl::yl(markerRect), gtl::xh(markerRect), gtl::yh(markerRect));
      marker->setBBox(box);
      marker->setLayerNum(layerNum);
      marker->setConstraint(con);
      marker->addSrc(net->getOwner());
      marker->addVictim(net->getOwner(), make_tuple(layerNum, 
                                         frBox(corner->x(), corner->y(), corner->x(), corner->y()),
                                         corner->isFixed()));
      marker->addSrc(segNet->getOwner());
      frCoord llx = min(seg->getLowCorner()->x(), seg->getHighCorner()->x());
      frCoord lly = min(seg->getLowCorner()->y(), seg->getHighCorner()->y());
      frCoord urx = max(seg->getLowCorner()->x(), seg->getHighCorner()->x());
      frCoord ury = max(seg->getLowCorner()->y(), seg->getHighCorner()->y());
      marker->addAggressor(segNet->getOwner(), make_tuple(seg->getLayerNum(),
                                                          frBox(llx, lly, urx, ury),
                                                          seg->isFixed()));
      if (addMarker(std::move(marker))) {
        if (enableOutput) {
          double dbu = getDesign()->getTopBlock()->getDBUPerUU();
          cout <<"CornerSpc@(" <<gtl::xl(markerRect) / dbu <<", " <<gtl::yl(markerRect) / dbu <<") ("
                            <<gtl::xh(markerRect) / dbu <<", " <<gtl::yh(markerRect) / dbu <<") "
               <<getDesign()->getTech()->getLayer(layerNum)->getName() <<" ";
          auto owner = net->getOwner();
          if (owner == nullptr) {
            cout <<"FLOATING";
          } else {
            if (owner->typeId() == frcNet) {
              cout <<static_cast<frNet*>(owner)->getName();
            } else if (owner->typeId() == frcInstTerm) {
              cout <<static_cast<frInstTerm*>(owner)->getInst()->getName() <<"/" 
                   <<static_cast<frInstTerm*>(owner)->getTerm()->getName();
            } else if (owner->typeId() == frcTerm) {
              cout <<"PIN/" <<static_cast<frTerm*>(owner)->getName();
            } else if (owner->typeId() == frcInstBlockage) {
              cout <<static_cast<frInstBlockage*>(owner)->getInst()->getName() <<"/OBS";
            } else if (owner->typeId() == frcBlockage) {
              cout <<"PIN/OBS";
            } else {
              cout <<"UNKNOWN";
            }
          }
          cout <<" ";
          owner = segNet->getOwner();
          if (owner == nullptr) {
            cout <<"FLOATING";
          } else {
            if (owner->typeId() == frcNet) {
              cout <<static_cast<frNet*>(owner)->getName();
            } else if (owner->typeId() == frcInstTerm) {
              cout <<static_cast<frInstTerm*>(owner)->getInst()->getName() <<"/" 
                   <<static_cast<frInstTerm*>(owner)->getTerm()->getName();
            } else if (owner->typeId() == frcTerm) {
              cout <<"PIN/" <<static_cast<frTerm*>(owner)->getName();
            } else if (owner->typeId() == frcInstBlockage) {
              cout <<static_cast<frInstBlockage*>(owner)->getInst()->getName() <<"/OBS";
            } else if (owner->typeId() == frcBlockage) {
              cout <<"PIN/OBS";
            } else {
              cout <<"UNKNOWN";
            }
          }
          cout <<endl;
        }
      }
    } else {
      // to be implemented
    }
  }
}

void FlexGCWorker::Impl::checkMetalCornerSpacing_main(gcCorner* corner) {
  // bool enableOutput = true;

  auto layerNum = corner->getPrevEdge()->getLayerNum();
  frCoord maxSpcValX, maxSpcValY;
  checkMetalCornerSpacing_getMaxSpcVal(layerNum, maxSpcValX, maxSpcValY);
  box_t queryBox = checkMetalCornerSpacing_getQueryBox(corner, maxSpcValX, maxSpcValY);

  auto &workerRegionQuery = getWorkerRegionQuery();
  vector<rq_box_value_t<gcRect*> > result;
  workerRegionQuery.queryMaxRectangle(queryBox, layerNum, result);
  // LEF58CornerSpacing
  auto &cons = getDesign()->getTech()->getLayer(layerNum)->getLef58CornerSpacingConstraints();
  for (auto &[objBox, ptr]: result) {
    for (auto &con: cons) {
      checkMetalCornerSpacing_main(corner, ptr, con);
    }
  }
}

void FlexGCWorker::Impl::checkMetalCornerSpacing() {
  if (targetNet_) {
    // layer --> net --> polygon --> corner
    for (int i = std::max((frLayerNum)(getDesign()->getTech()->getBottomLayerNum()), minLayerNum_); 
         i <= std::min((frLayerNum)(getDesign()->getTech()->getTopLayerNum()), maxLayerNum_); i++) {
      auto currLayer = getDesign()->getTech()->getLayer(i);
      if (currLayer->getType() != frLayerTypeEnum::ROUTING || !currLayer->hasLef58CornerSpacingConstraint()) {
        continue;
      }
      for (auto &pin: targetNet_->getPins(i)) {
        for (auto &corners: pin->getPolygonCorners()) {
          for (auto &corner: corners) {
            // LEF58 corner spacing
            checkMetalCornerSpacing_main(corner.get());
          }
        }
      }
    }
  } else {
    // layer --> net --> polygon --> corner
    for (int i = std::max((frLayerNum)(getDesign()->getTech()->getBottomLayerNum()), minLayerNum_); 
         i <= std::min((frLayerNum)(getDesign()->getTech()->getTopLayerNum()), maxLayerNum_); i++) {
      auto currLayer = getDesign()->getTech()->getLayer(i);
      if (currLayer->getType() != frLayerTypeEnum::ROUTING || !currLayer->hasLef58CornerSpacingConstraint()) {
        continue;
      }
      for (auto &net: getNets()) {
        for (auto &pin: net->getPins(i)) {
          for (auto &corners: pin->getPolygonCorners()) {
            for (auto &corner: corners) {
              // LEF58 corner spacing
              checkMetalCornerSpacing_main(corner.get());
            }
          }
        }
      }
    }
  }
}

void FlexGCWorker::Impl::checkMetalShape_minWidth(const gtl::rectangle_data<frCoord> &rect, frLayerNum layerNum, gcNet* net, bool isH) {
  bool enableOutput = printMarker_;
  // skip enough width
  auto minWidth = getDesign()->getTech()->getLayer(layerNum)->getMinWidth();
  auto xLen = gtl::delta(rect, gtl::HORIZONTAL);
  auto yLen = gtl::delta(rect, gtl::VERTICAL);
  if (isH && xLen >= minWidth) {
    return;
  }
  if (!isH && yLen >= minWidth) {
    return;
  }
  // only show marker if fixed area < marker area
  {
    using namespace boost::polygon::operators;
    auto &fixedPolys = net->getPolygons(layerNum, true);
    auto intersection_fixedPolys = fixedPolys & rect;
    if (gtl::area(intersection_fixedPolys) == gtl::area(rect)) {
      return;
    }
  }
  
  auto marker = make_unique<frMarker>();
  frBox box(gtl::xl(rect), gtl::yl(rect), gtl::xh(rect), gtl::yh(rect));
  marker->setBBox(box);
  marker->setLayerNum(layerNum);
  marker->setConstraint(getDesign()->getTech()->getLayer(layerNum)->getMinWidthConstraint());
  marker->addSrc(net->getOwner());
  marker->addVictim(net->getOwner(), make_tuple(layerNum, box, false));
  marker->addAggressor(net->getOwner(), make_tuple(layerNum, box, false));
  if (addMarker(std::move(marker))) {
    // true marker
    if (enableOutput) {
      double dbu = getDesign()->getTopBlock()->getDBUPerUU();
      cout <<"MinWid@(";
      cout <<gtl::xl(rect) / dbu <<", " <<gtl::yl(rect) / dbu <<") ("
           <<gtl::xh(rect) / dbu <<", " <<gtl::yh(rect) / dbu <<") "
           <<getDesign()->getTech()->getLayer(layerNum)->getName() <<" ";
      auto owner = net->getOwner();
      if (owner == nullptr) {
        cout <<"FLOATING";
      } else {
        if (owner->typeId() == frcNet) {
          cout <<static_cast<frNet*>(owner)->getName();
        } else if (owner->typeId() == frcInstTerm) {
          cout <<static_cast<frInstTerm*>(owner)->getInst()->getName() <<"/" 
               <<static_cast<frInstTerm*>(owner)->getTerm()->getName();
        } else if (owner->typeId() == frcTerm) {
          cout <<"PIN/" <<static_cast<frTerm*>(owner)->getName();
        } else if (owner->typeId() == frcInstBlockage) {
          cout <<static_cast<frInstBlockage*>(owner)->getInst()->getName() <<"/OBS";
        } else if (owner->typeId() == frcBlockage) {
          cout <<"PIN/OBS";
        } else {
          cout <<"UNKNOWN";
        }
      }
      cout <<endl;
    }
  }
}

void FlexGCWorker::Impl::checkMetalShape_minStep_helper(const frBox &markerBox, frLayerNum layerNum, gcNet* net, 
                                                  frMinStepConstraint* con,
                                                  bool hasInsideCorner, bool hasOutsideCorner, bool hasStep, 
                                                  int currEdges, frCoord currLength, bool hasRoute) {
  bool enableOutput = printMarker_;
  // skip if no edge
  if (currEdges == 0) {
    return;
  }
  // skip if no route
  if (!hasRoute) {
    return;
  }

  if (con->hasMinstepType()) {
    // skip if no corner or step
    switch(con->getMinstepType()) {
      case frMinstepTypeEnum::INSIDECORNER:
        if (!hasInsideCorner) {
          return;
        }
        break;
      case frMinstepTypeEnum::OUTSIDECORNER:
        if (!hasOutsideCorner) {
          return;
        }
        break;
      case frMinstepTypeEnum::STEP:
        if (!hasStep) {
          return;
        }
        break;
      default:
        ;
    }
    // skip if <= maxlength
    if (currLength <= con->getMaxLength()) {
      return;
    }
  } else if (con->hasMaxEdges()) {
    // skip if <= maxedges
    if (currEdges <= con->getMaxEdges()) {
      return;
    }
  }

  // true marker
  auto marker = make_unique<frMarker>();
  marker->setBBox(markerBox);
  marker->setLayerNum(layerNum);
  marker->setConstraint(con);
  marker->addSrc(net->getOwner());
  marker->addVictim(net->getOwner(), make_tuple(layerNum, markerBox, false));
  marker->addAggressor(net->getOwner(), make_tuple(layerNum, markerBox, false));
  if (addMarker(std::move(marker))) {
    // true marker
    if (enableOutput) {
      double dbu = getDesign()->getTopBlock()->getDBUPerUU();
      cout <<"MinStp@(";
      cout <<markerBox.left()  / dbu <<", " <<markerBox.bottom() / dbu <<") ("
           <<markerBox.right() / dbu <<", " <<markerBox.top()    / dbu <<") "
           <<getDesign()->getTech()->getLayer(layerNum)->getName() <<" ";
      auto owner = net->getOwner();
      if (owner == nullptr) {
        cout <<"FLOATING";
      } else {
        if (owner->typeId() == frcNet) {
          cout <<static_cast<frNet*>(owner)->getName();
        } else if (owner->typeId() == frcInstTerm) {
          cout <<static_cast<frInstTerm*>(owner)->getInst()->getName() <<"/" 
               <<static_cast<frInstTerm*>(owner)->getTerm()->getName();
        } else if (owner->typeId() == frcTerm) {
          cout <<"PIN/" <<static_cast<frTerm*>(owner)->getName();
        } else if (owner->typeId() == frcInstBlockage) {
          cout <<static_cast<frInstBlockage*>(owner)->getInst()->getName() <<"/OBS";
        } else if (owner->typeId() == frcBlockage) {
          cout <<"PIN/OBS";
        } else {
          cout <<"UNKNOWN";
        }
      }
      cout <<endl;
    }
  }
}

void FlexGCWorker::Impl::checkMetalShape_minArea(gcPin* pin) {
  bool enableOutput = false;

  if (ignoreMinArea_) {
    return;
  }

  auto poly = pin->getPolygon();
  auto layerNum = poly->getLayerNum();
  auto net = poly->getNet();

  auto con = design_->getTech()->getLayer(layerNum)->getAreaConstraint();

  if (!con) {
    return;
  }

  auto reqArea = con->getMinArea();
  auto actArea = gtl::area(*poly);

  if (actArea >= reqArea) {
    return;
  }

  bool hasNonFixedEdge = false;
  for (auto &edges: pin->getPolygonEdges()) {
    for (auto &edge: edges) {
      if (edge->isFixed() == false) {
        hasNonFixedEdge = true;
        break;
      }
    }
    if (hasNonFixedEdge) {
      break;
    }
  }

  if (!hasNonFixedEdge) {
    return;
  }

  // add marker
  gtl::polygon_90_set_data<frCoord> tmpPolys;
  using namespace boost::polygon::operators;
  tmpPolys += *poly;
  vector<gtl::rectangle_data<frCoord> > rects;
  gtl::get_max_rectangles(rects, tmpPolys);
  
  int maxArea = 0;
  gtl::rectangle_data<frCoord> maxRect;
  for (auto &rect: rects) {
    if (gtl::area(rect) > maxArea) {
      maxRect = rect;
      maxArea = gtl::area(rect);
    }
  }

  auto marker = make_unique<frMarker>();
  frBox markerBox(gtl::xl(maxRect), gtl::yl(maxRect), gtl::xh(maxRect), gtl::yh(maxRect));
  marker->setBBox(markerBox);
  marker->setLayerNum(layerNum);
  marker->setConstraint(con);
  marker->addSrc(net->getOwner());
  marker->addVictim(net->getOwner(), make_tuple(layerNum, markerBox, false));
  marker->addAggressor(net->getOwner(), make_tuple(layerNum, markerBox, false));

  if (addMarker(std::move(marker))) {
    // true marker
    if (enableOutput) {
      double dbu = getDesign()->getTopBlock()->getDBUPerUU();
      cout <<"MinArea@(";
      cout <<markerBox.left()  / dbu <<", " <<markerBox.bottom() / dbu <<") ("
           <<markerBox.right() / dbu <<", " <<markerBox.top()    / dbu <<") "
           <<getDesign()->getTech()->getLayer(layerNum)->getName() <<" ";
      auto owner = net->getOwner();
      if (owner == nullptr) {
        cout <<"FLOATING";
      } else {
        if (owner->typeId() == frcNet) {
          cout <<static_cast<frNet*>(owner)->getName();
        } else if (owner->typeId() == frcInstTerm) {
          cout <<static_cast<frInstTerm*>(owner)->getInst()->getName() <<"/" 
               <<static_cast<frInstTerm*>(owner)->getTerm()->getName();
        } else if (owner->typeId() == frcTerm) {
          cout <<"PIN/" <<static_cast<frTerm*>(owner)->getName();
        } else if (owner->typeId() == frcInstBlockage) {
          cout <<static_cast<frInstBlockage*>(owner)->getInst()->getName() <<"/OBS";
        } else if (owner->typeId() == frcBlockage) {
          cout <<"PIN/OBS";
        } else {
          cout <<"UNKNOWN";
        }
      }
      cout <<endl;
    }
  }
}

void FlexGCWorker::Impl::checkMetalShape_lef58MinStep_noBetweenEol(gcPin* pin, frLef58MinStepConstraint* con) {
  auto enableOutput = false;
  // auto enableOutput = true;

  auto poly = pin->getPolygon();
  auto layerNum = poly->getLayerNum();
  auto net = poly->getNet();

  vector<gcSegment*> startEdges;
  frCoord llx = 0;
  frCoord lly = 0;
  frCoord urx = 0;
  frCoord ury = 0;
  frBox markerBox;
  auto minStepLength = con->getMinStepLength();
  auto eolWidth = con->getEolWidth();
  for (auto &edges: pin->getPolygonEdges()) {
    // get the first edge that is >= minstep length
    for (auto &e: edges) {
      if (gtl::length(*e) < minStepLength) {
        startEdges.push_back(e.get());
      }
    }
  }

  for (auto startEdge: startEdges) {
    // skip if next next edge is not less than minStepLength
    if (gtl::length(*(startEdge->getNextEdge()->getNextEdge())) >= minStepLength) {
      continue;
    }
    // skip if next edge is not EOL edge
    auto nextEdge = startEdge->getNextEdge();
    if (nextEdge->getLowCorner()->getType() != frCornerTypeEnum::CONVEX ||
        nextEdge->getHighCorner()->getType() != frCornerTypeEnum::CONVEX) {
      continue;
    }
    if (gtl::length(*nextEdge) >= eolWidth) {
      continue;
    }

    // skip if all edges are fixed
    if (startEdge->isFixed() &&
        startEdge->getNextEdge()->isFixed() &&
        startEdge->getNextEdge()->getNextEdge()->isFixed()) {
      continue;
    }

    // real violation
    llx = startEdge->low().x();
    lly = startEdge->low().y();
    urx = startEdge->low().x();
    ury = startEdge->low().y();

    llx = min(llx, startEdge->high().x());
    lly = min(lly, startEdge->high().y());
    urx = max(urx, startEdge->high().x());
    ury = max(ury, startEdge->high().y());

    llx = min(llx, startEdge->getNextEdge()->high().x());
    lly = min(lly, startEdge->getNextEdge()->high().y());
    urx = max(urx, startEdge->getNextEdge()->high().x());
    ury = max(ury, startEdge->getNextEdge()->high().y());

    llx = min(llx, startEdge->getNextEdge()->getNextEdge()->high().x());
    lly = min(lly, startEdge->getNextEdge()->getNextEdge()->high().y());
    urx = max(urx, startEdge->getNextEdge()->getNextEdge()->high().x());
    ury = max(ury, startEdge->getNextEdge()->getNextEdge()->high().y());

    auto marker = make_unique<frMarker>();
    frBox box(llx, lly, urx, ury);
    marker->setBBox(box);
    marker->setLayerNum(layerNum);
    marker->setConstraint(con);
    marker->addSrc(net->getOwner());
    marker->addVictim(net->getOwner(), make_tuple(layerNum, box, false));
    marker->addAggressor(net->getOwner(), make_tuple(layerNum, box, false));
    if (addMarker(std::move(marker))) {
      // true marker
      if (enableOutput) {
        double dbu = getDesign()->getTopBlock()->getDBUPerUU();
        cout <<"Lef58MinStp@(";
        cout <<markerBox.left()  / dbu <<", " <<markerBox.bottom() / dbu <<") ("
             <<markerBox.right() / dbu <<", " <<markerBox.top()    / dbu <<") "
             <<getDesign()->getTech()->getLayer(layerNum)->getName() <<" ";
        auto owner = net->getOwner();
        if (owner == nullptr) {
          cout <<"FLOATING";
        } else {
          if (owner->typeId() == frcNet) {
            cout <<static_cast<frNet*>(owner)->getName();
          } else if (owner->typeId() == frcInstTerm) {
            cout <<static_cast<frInstTerm*>(owner)->getInst()->getName() <<"/" 
                 <<static_cast<frInstTerm*>(owner)->getTerm()->getName();
          } else if (owner->typeId() == frcTerm) {
            cout <<"PIN/" <<static_cast<frTerm*>(owner)->getName();
          } else if (owner->typeId() == frcInstBlockage) {
            cout <<static_cast<frInstBlockage*>(owner)->getInst()->getName() <<"/OBS";
          } else if (owner->typeId() == frcBlockage) {
            cout <<"PIN/OBS";
          } else {
            cout <<"UNKNOWN";
          }
        }
        cout <<endl;
      }
    }
  }
}

// currently only support nobetweeneol
void FlexGCWorker::Impl::checkMetalShape_lef58MinStep(gcPin* pin) {
  //bool enableOutput = true;

  auto poly = pin->getPolygon();
  auto layerNum = poly->getLayerNum();
  // auto net = poly->getNet();

  for (auto con: design_->getTech()->getLayer(layerNum)->getLef58MinStepConstraints()) {
    if (!con->hasEolWidth()) {
      continue;
    }
    checkMetalShape_lef58MinStep_noBetweenEol(pin, con);
  }

  
}

void FlexGCWorker::Impl::checkMetalShape_minStep(gcPin* pin) {
  //bool enableOutput = true;

  auto poly = pin->getPolygon();
  auto layerNum = poly->getLayerNum();
  auto net = poly->getNet();

  auto con = design_->getTech()->getLayer(layerNum)->getMinStepConstraint();

  if (!con) {
    return;
  }

  gcSegment* be = nullptr;
  gcSegment* firste = nullptr; // first edge to check
  int currEdges = 0;
  int currLength = 0;
  bool hasRoute = false;
  frCoord llx = 0;
  frCoord lly = 0;
  frCoord urx = 0;
  frCoord ury = 0;
  frBox markerBox;
  bool hasInsideCorner = false;
  bool hasOutsideCorner = false;
  bool hasStep = false;
  auto minStepLength = con->getMinStepLength();
  for (auto &edges: pin->getPolygonEdges()) {
    // get the first edge that is >= minstep length
    firste = nullptr;
    for (auto &e: edges) {
      if (gtl::length(*e) >= minStepLength) {
        firste = e.get();
        break;
      }
    }
    // skip if no first starting edge
    if (!firste) {
      continue;
    }
    // initialize all vars
    auto edge = firste;
    be = edge;
    currEdges = 0;
    currLength = 0;
    hasRoute = edge->isFixed() ? false : true;
    hasInsideCorner = false;
    hasOutsideCorner = false;
    hasStep = false;
    llx = edge->high().x();
    lly = edge->high().y();
    urx = edge->high().x();
    ury = edge->high().y();
    while (true) {
      edge = edge->getNextEdge();
      if (gtl::length(*edge) < minStepLength) {
        currEdges++;
        currLength += gtl::length(*edge);
        hasRoute = hasRoute || (edge->isFixed() ? false : true);
        llx = std::min(llx, edge->high().x());
        lly = std::min(lly, edge->high().y());
        urx = std::max(urx, edge->high().x());
        ury = std::max(ury, edge->high().y());
      } else {
        // skip if begin end edges are the same
        if (edge == be) {
          break;
        }
        // be and ee found, check rule here
        hasRoute = hasRoute || (edge->isFixed() ? false : true);
        markerBox.set(llx, lly, urx, ury);
        checkMetalShape_minStep_helper(markerBox, layerNum, net, con, hasInsideCorner, hasOutsideCorner, hasStep, 
                                       currEdges, currLength, hasRoute);
        be = edge; // new begin edge
        // skip if new begin edge is the first begin edge
        if (be == firste) {
          break;
        }
        currEdges = 0;
        currLength = 0;
        hasRoute = edge->isFixed() ? false : true;
        hasInsideCorner = false;
        hasOutsideCorner = false;
        hasStep = false;
        llx = edge->high().x();
        lly = edge->high().y();
        urx = edge->high().x();
        ury = edge->high().y();
      }
    }
  }
}

void FlexGCWorker::Impl::checkMetalShape_rectOnly(gcPin* pin) {
  // bool enableOutput = true;
  bool enableOutput = false;

  auto poly = pin->getPolygon();
  auto layerNum = poly->getLayerNum();
  auto layerMinWidth = getDesign()->getTech()->getLayer(layerNum)->getMinWidth();
  auto net = poly->getNet();

  auto con = design_->getTech()->getLayer(layerNum)->getLef58RectOnlyConstraint();

  if (!con) {
    return;
  }

  // not rectangle, potential violation
  vector<gtl::rectangle_data<frCoord> > rects;
  gtl::polygon_90_set_data<frCoord> polySet; 
  {
    using namespace boost::polygon::operators;
    polySet += *poly;
  }
  gtl::get_max_rectangles(rects, polySet);
  // rect only is true
  if (rects.size() == 1) {
    return;
  }
  // only show marker if fixed area does not contain marker area
  vector<gtl::point_data<frCoord> > concaveCorners;
  // get concave corners of the polygon
  for (auto &edges: pin->getPolygonEdges()) {
    for (auto &edge: edges) {
      auto currEdge = edge.get();
      auto nextEdge = currEdge->getNextEdge();
      if (orientation(*currEdge, *nextEdge) == -1) {
        concaveCorners.push_back(boost::polygon::high(*currEdge));
      }
    }
  }
  // draw rect from concave corners and test intersection
  auto &fixedPolys = net->getPolygons(layerNum, true);
  for (auto &corner: concaveCorners) {
    gtl::rectangle_data<frCoord> rect(gtl::x(corner) - layerMinWidth, gtl::y(corner) - layerMinWidth,
                                      gtl::x(corner) + layerMinWidth, gtl::y(corner) + layerMinWidth);
    {
      using namespace boost::polygon::operators;
      auto intersectionPolySet = polySet & rect;
      auto intersectionFixedPolySet = intersectionPolySet & fixedPolys;
      if (gtl::area(intersectionFixedPolySet) == gtl::area(intersectionPolySet)) {
        continue;
      }

      vector<gtl::rectangle_data<frCoord> > maxRects;
      gtl::get_max_rectangles(maxRects, intersectionPolySet);
      for (auto &markerRect: maxRects) {
        auto marker = make_unique<frMarker>();
        frBox box(gtl::xl(markerRect), gtl::yl(markerRect), gtl::xh(markerRect), gtl::yh(markerRect));
        marker->setBBox(box);
        marker->setLayerNum(layerNum);
        marker->setConstraint(con);
        marker->addSrc(net->getOwner());
        marker->addVictim(net->getOwner(), make_tuple(layerNum, box, false));
        marker->addAggressor(net->getOwner(), make_tuple(layerNum, box, false));
        if (addMarker(std::move(marker))) {
          // true marker
          if (enableOutput) {
            double dbu = getDesign()->getTopBlock()->getDBUPerUU();
            cout <<"RectOnly@(" <<gtl::xl(markerRect) / dbu <<", " <<gtl::yl(markerRect) / dbu <<") ("
                                <<gtl::xh(markerRect) / dbu <<", " <<gtl::yh(markerRect) / dbu <<") "
                 <<getDesign()->getTech()->getLayer(layerNum)->getName() <<" ";
            auto owner = net->getOwner();
            if (owner == nullptr) {
              cout <<"FLOATING";
            } else {
              if (owner->typeId() == frcNet) {
                cout <<static_cast<frNet*>(owner)->getName();
              } else if (owner->typeId() == frcInstTerm) {
                cout <<static_cast<frInstTerm*>(owner)->getInst()->getName() <<"/" 
                     <<static_cast<frInstTerm*>(owner)->getTerm()->getName();
              } else if (owner->typeId() == frcTerm) {
                cout <<"PIN/" <<static_cast<frTerm*>(owner)->getName();
              } else if (owner->typeId() == frcInstBlockage) {
                cout <<static_cast<frInstBlockage*>(owner)->getInst()->getName() <<"/OBS";
              } else if (owner->typeId() == frcBlockage) {
                cout <<"PIN/OBS";
              } else {
                cout <<"UNKNOWN";
              }
            }
          }
        }
      }

    }
  }
}

void FlexGCWorker::Impl::checkMetalShape_offGrid(gcPin* pin) {
  bool enableOutput = false;
  auto net = pin->getNet();
  // Needs to be signed to make modulo work correctly with
  // negative coordinates
  int mGrid = getDesign()->getTech()->getManufacturingGrid();
  for (auto &rect: pin->getMaxRectangles()) {
    auto maxRect = rect.get();
    auto layerNum = maxRect->getLayerNum();
    // off grid maxRect
    if (gtl::xl(*maxRect) % mGrid || 
        gtl::xh(*maxRect) % mGrid ||
        gtl::yl(*maxRect) % mGrid ||
        gtl::yh(*maxRect) % mGrid) {
      // continue if the marker area does not have route shape
      auto &polys = net->getPolygons(layerNum, false);
      gtl::rectangle_data<frCoord> markerRect(*maxRect);
      using namespace boost::polygon::operators;
      auto intersection_polys = polys & markerRect;
      if (gtl::empty(intersection_polys)) {
        continue;
      }
      auto marker = make_unique<frMarker>();
      frBox box(gtl::xl(markerRect), gtl::yl(markerRect), gtl::xh(markerRect), gtl::yh(markerRect));
      marker->setBBox(box);
      marker->setLayerNum(layerNum);
      marker->setConstraint(getDesign()->getTech()->getLayer(layerNum)->getOffGridConstraint());
      marker->addSrc(net->getOwner());
      marker->addVictim(net->getOwner(), make_tuple(layerNum, box, false));
      marker->addAggressor(net->getOwner(), make_tuple(layerNum, box, false));
      if (addMarker(std::move(marker))) {
        if (enableOutput) {
          double dbu = getDesign()->getTopBlock()->getDBUPerUU();
          
          cout << "OffGrid@(";
          cout <<gtl::xl(markerRect) / dbu <<", " <<gtl::yl(markerRect) / dbu <<") ("
               <<gtl::xh(markerRect) / dbu <<", " <<gtl::yh(markerRect) / dbu <<") "
               <<getDesign()->getTech()->getLayer(layerNum)->getName() <<" ";
          auto owner = net->getOwner();
          if (owner == nullptr) {
            cout <<"FLOATING";
          } else {
            if (owner->typeId() == frcNet) {
              cout <<static_cast<frNet*>(owner)->getName();
            } else if (owner->typeId() == frcInstTerm) {
              cout <<static_cast<frInstTerm*>(owner)->getInst()->getName() <<"/" 
                   <<static_cast<frInstTerm*>(owner)->getTerm()->getName();
            } else if (owner->typeId() == frcTerm) {
              cout <<"PIN/" <<static_cast<frTerm*>(owner)->getName();
            } else if (owner->typeId() == frcInstBlockage) {
              cout <<static_cast<frInstBlockage*>(owner)->getInst()->getName() <<"/OBS";
            } else if (owner->typeId() == frcBlockage) {
              cout <<"PIN/OBS";
            } else {
              cout <<"UNKNOWN";
            }
          }
          cout << endl;
        }
      }
    }
  }
}

void FlexGCWorker::Impl::checkMetalShape_minEnclosedArea(gcPin* pin) {
  bool enableOutput = false;
  auto net = pin->getNet();
  auto poly = pin->getPolygon();
  auto layerNum = poly->getLayerNum();
  if (getDesign()->getTech()->getLayer(layerNum)->hasMinEnclosedArea()) {
    for (auto holeIt = poly->begin_holes(); holeIt != poly->end_holes(); holeIt++) {
      auto &hole_poly = *holeIt;
      for (auto con: getDesign()->getTech()->getLayer(layerNum)->getMinEnclosedAreaConstraints()) {
        auto reqArea = con->getArea();
        if (gtl::area(hole_poly) < reqArea) {
          auto &polys = net->getPolygons(layerNum, false);
          using namespace boost::polygon::operators;
          auto intersection_polys = polys & (*poly);
          if (gtl::empty(intersection_polys)) {
            continue;
          }
          // create marker
          gtl::rectangle_data<frCoord> markerRect;
          gtl::extents(markerRect, hole_poly);

          auto marker = make_unique<frMarker>();
          frBox box(gtl::xl(markerRect), gtl::yl(markerRect), gtl::xh(markerRect), gtl::yh(markerRect));
          marker->setBBox(box);
          marker->setLayerNum(layerNum);
          marker->setConstraint(con);
          marker->addSrc(net->getOwner());
          marker->addVictim(net->getOwner(), make_tuple(layerNum, box, false));
          marker->addAggressor(net->getOwner(), make_tuple(layerNum, box, false));
          if (addMarker(std::move(marker))) {
            if (enableOutput) {
              double dbu = getDesign()->getTopBlock()->getDBUPerUU();
              
              cout << "MinHole@(";
              cout <<gtl::xl(markerRect) / dbu <<", " <<gtl::yl(markerRect) / dbu <<") ("
                   <<gtl::xh(markerRect) / dbu <<", " <<gtl::yh(markerRect) / dbu <<") "
                   <<getDesign()->getTech()->getLayer(layerNum)->getName() <<" ";
              auto owner = net->getOwner();
              if (owner == nullptr) {
                cout <<"FLOATING";
              } else {
                if (owner->typeId() == frcNet) {
                  cout <<static_cast<frNet*>(owner)->getName();
                } else if (owner->typeId() == frcInstTerm) {
                  cout <<static_cast<frInstTerm*>(owner)->getInst()->getName() <<"/" 
                       <<static_cast<frInstTerm*>(owner)->getTerm()->getName();
                } else if (owner->typeId() == frcTerm) {
                  cout <<"PIN/" <<static_cast<frTerm*>(owner)->getName();
                } else if (owner->typeId() == frcInstBlockage) {
                  cout <<static_cast<frInstBlockage*>(owner)->getInst()->getName() <<"/OBS";
                } else if (owner->typeId() == frcBlockage) {
                  cout <<"PIN/OBS";
                } else {
                  cout <<"UNKNOWN";
                }
              }
              cout << endl;
            }
          }
        }
      }
    }
  }
}

void FlexGCWorker::Impl::checkMetalShape_main(gcPin* pin) {
  //bool enableOutput = true;

  auto poly = pin->getPolygon();
  auto layerNum = poly->getLayerNum();
  auto net = poly->getNet();

  // min width
  vector<gtl::rectangle_data<frCoord> > rects;
  gtl::polygon_90_set_data<frCoord> polySet;
  {
    using namespace boost::polygon::operators;
    polySet += *poly;
  }
  polySet.get_rectangles(rects, gtl::HORIZONTAL);
  for (auto &rect: rects) {
    checkMetalShape_minWidth(rect, layerNum, net, true);
  }
  rects.clear();
  polySet.get_rectangles(rects, gtl::VERTICAL);
  for (auto &rect: rects) {
    checkMetalShape_minWidth(rect, layerNum, net, false);
  }

  // min area
  // checkMetalShape_minArea(pin);

  // min step
  checkMetalShape_minStep(pin);

  // lef58 min step
  checkMetalShape_lef58MinStep(pin);

  // rect only
  checkMetalShape_rectOnly(pin);

  // off grid
  checkMetalShape_offGrid(pin);

  // min hole
  checkMetalShape_minEnclosedArea(pin);
}

void FlexGCWorker::Impl::checkMetalShape() {
  if (targetNet_) {
    // layer --> net --> polygon
    for (int i = std::max((frLayerNum)(getDesign()->getTech()->getBottomLayerNum()), minLayerNum_); 
         i <= std::min((frLayerNum)(getDesign()->getTech()->getTopLayerNum()), maxLayerNum_); i++) {
      auto currLayer = getDesign()->getTech()->getLayer(i);
      if (currLayer->getType() != frLayerTypeEnum::ROUTING) {
        continue;
      }
      for (auto &pin: targetNet_->getPins(i)) {
        checkMetalShape_main(pin.get());
      }
    }
  } else {
    // layer --> net --> polygon
    for (int i = std::max((frLayerNum)(getDesign()->getTech()->getBottomLayerNum()), minLayerNum_); 
         i <= std::min((frLayerNum)(getDesign()->getTech()->getTopLayerNum()), maxLayerNum_); i++) {
      auto currLayer = getDesign()->getTech()->getLayer(i);
      if (currLayer->getType() != frLayerTypeEnum::ROUTING) {
        continue;
      }
      for (auto &net: getNets()) {
        for (auto &pin: net->getPins(i)) {
          checkMetalShape_main(pin.get());
        }
      }
    }
  }
}

bool FlexGCWorker::Impl::checkMetalEndOfLine_eol_isEolEdge(gcSegment *edge, frSpacingEndOfLineConstraint *con) {
  // skip if >= eolWidth
  if (gtl::length(*edge) >= con->getEolWidth()) {
    return false;
  }
  // skip if non convex edge
  auto prevEdge = edge->getPrevEdge();
  auto nextEdge = edge->getNextEdge();
  if (!(gtl::orientation(*prevEdge, *edge) == 1 && gtl::orientation(*edge, *nextEdge) == 1)) {
    return false;
  }
  return true;
}

// bbox on the gcSegment->low() side
void FlexGCWorker::Impl::checkMetalEndOfLine_eol_hasParallelEdge_oneDir_getQueryBox(gcSegment *edge, frSpacingEndOfLineConstraint *con, 
                                                                              bool isSegLow, box_t &queryBox, 
                                                                              gtl::rectangle_data<frCoord> &queryRect) {
  frCoord ptX, ptY;
  auto eolWithin = con->getEolWithin();
  auto parWithin = con->getParWithin();
  auto parSpace  = con->getParSpace();
  if (isSegLow) {
    ptX = edge->low().x();
    ptY = edge->low().y();
    if (edge->getDir() == frDirEnum::E) {
      bg::set<bg::min_corner, 0>(queryBox, ptX - parSpace  );
      bg::set<bg::min_corner, 1>(queryBox, ptY - eolWithin );
      bg::set<bg::max_corner, 0>(queryBox, ptX             );
      bg::set<bg::max_corner, 1>(queryBox, ptY + parWithin );
    } else if (edge->getDir() == frDirEnum::W) {
      bg::set<bg::min_corner, 0>(queryBox, ptX             );
      bg::set<bg::min_corner, 1>(queryBox, ptY - parWithin );
      bg::set<bg::max_corner, 0>(queryBox, ptX + parSpace  );
      bg::set<bg::max_corner, 1>(queryBox, ptY + eolWithin );
    } else if (edge->getDir() == frDirEnum::N) {
      bg::set<bg::min_corner, 0>(queryBox, ptX - parWithin );
      bg::set<bg::min_corner, 1>(queryBox, ptY - parSpace  );
      bg::set<bg::max_corner, 0>(queryBox, ptX + eolWithin );
      bg::set<bg::max_corner, 1>(queryBox, ptY             );
    } else { // S
      bg::set<bg::min_corner, 0>(queryBox, ptX - eolWithin );
      bg::set<bg::min_corner, 1>(queryBox, ptY             );
      bg::set<bg::max_corner, 0>(queryBox, ptX + parWithin );
      bg::set<bg::max_corner, 1>(queryBox, ptY + parSpace  );
    }
  } else {
    ptX = edge->high().x();
    ptY = edge->high().y();
    if (edge->getDir() == frDirEnum::E) {
      bg::set<bg::min_corner, 0>(queryBox, ptX             );
      bg::set<bg::min_corner, 1>(queryBox, ptY - eolWithin );
      bg::set<bg::max_corner, 0>(queryBox, ptX + parSpace  );
      bg::set<bg::max_corner, 1>(queryBox, ptY + parWithin );
    } else if (edge->getDir() == frDirEnum::W) {
      bg::set<bg::min_corner, 0>(queryBox, ptX - parSpace  );
      bg::set<bg::min_corner, 1>(queryBox, ptY - parWithin );
      bg::set<bg::max_corner, 0>(queryBox, ptX             );
      bg::set<bg::max_corner, 1>(queryBox, ptY + eolWithin );
    } else if (edge->getDir() == frDirEnum::N) {
      bg::set<bg::min_corner, 0>(queryBox, ptX - parWithin );
      bg::set<bg::min_corner, 1>(queryBox, ptY             );
      bg::set<bg::max_corner, 0>(queryBox, ptX + eolWithin );
      bg::set<bg::max_corner, 1>(queryBox, ptY + parSpace  );
    } else { // S
      bg::set<bg::min_corner, 0>(queryBox, ptX - eolWithin );
      bg::set<bg::min_corner, 1>(queryBox, ptY - parSpace  );
      bg::set<bg::max_corner, 0>(queryBox, ptX + parWithin );
      bg::set<bg::max_corner, 1>(queryBox, ptY             );
    }
  }
  gtl::xl(queryRect, queryBox.min_corner().x());
  gtl::yl(queryRect, queryBox.min_corner().y());
  gtl::xh(queryRect, queryBox.max_corner().x());
  gtl::yh(queryRect, queryBox.max_corner().y());
}

void FlexGCWorker::Impl::checkMetalEndOfLine_eol_hasParallelEdge_oneDir_getParallelEdgeRect(gcSegment *edge, gtl::rectangle_data<frCoord> &rect) {
  if (edge->getDir() == frDirEnum::E) {
    gtl::xl(rect, edge->low().x());
    gtl::yl(rect, edge->low().y());
    gtl::xh(rect, edge->high().x());
    gtl::yh(rect, edge->high().y() + 1);
  } else if (edge->getDir() == frDirEnum::W) {
    gtl::xl(rect, edge->high().x());
    gtl::yl(rect, edge->high().y() - 1);
    gtl::xh(rect, edge->low().x());
    gtl::yh(rect, edge->low().y());
  } else if (edge->getDir() == frDirEnum::N) {
    gtl::xl(rect, edge->low().x() - 1);
    gtl::yl(rect, edge->low().y());
    gtl::xh(rect, edge->high().x());
    gtl::yh(rect, edge->high().y());
  } else { // S
    gtl::xl(rect, edge->high().x());
    gtl::yl(rect, edge->high().y());
    gtl::xh(rect, edge->low().x() + 1);
    gtl::yh(rect, edge->low().y());
  }
}


bool FlexGCWorker::Impl::checkMetalEndOfLine_eol_hasParallelEdge_oneDir(gcSegment *edge, frSpacingEndOfLineConstraint *con, bool isSegLow, bool &hasRoute) {
  bool sol = false;
  auto layerNum = edge->getLayerNum();
  box_t queryBox; // (shrink by one, disabled)
  gtl::rectangle_data<frCoord> queryRect; // original size
  checkMetalEndOfLine_eol_hasParallelEdge_oneDir_getQueryBox(edge, con, isSegLow, queryBox, queryRect);
  gtl::rectangle_data<frCoord> triggerRect;

  vector<pair<segment_t, gcSegment*> > results;
  auto &workerRegionQuery = getWorkerRegionQuery();
  workerRegionQuery.queryPolygonEdge(queryBox, edge->getLayerNum(), results);
  gtl::polygon_90_set_data<frCoord> tmpPoly;
  for (auto &[boostSeg, ptr]: results) {
    // skip if non oppo-dir parallel edge
    if (isSegLow) {
      if (gtl::orientation(*ptr, *edge) != -1) {
        continue;
      }
    } else {
      if (gtl::orientation(*edge, *ptr) != -1) {
        continue;
      }
    }
    // (skip if no area, done by reducing queryBox... skipped here, --> not skipped)
    checkMetalEndOfLine_eol_hasParallelEdge_oneDir_getParallelEdgeRect(ptr, triggerRect);
    if (!gtl::intersects(queryRect, triggerRect, false)) {
      continue;
    }
    // check whether parallel edge is route or not
    sol = true;
    if (!hasRoute && !ptr->isFixed()) {
      tmpPoly.clear();
      //checkMetalEndOfLine_eol_hasParallelEdge_oneDir_getParallelEdgeRect(ptr, triggerRect);
      // tmpPoly is the intersection of queryRect and minimum paralleledge rect
      using namespace boost::polygon::operators;
      tmpPoly += queryRect;
      tmpPoly &= triggerRect;
      auto &polys = ptr->getNet()->getPolygons(layerNum, false);
      // tmpPoly should have route shapes in order to be considered route
      tmpPoly &= polys;
      if (!gtl::empty(tmpPoly)) {
        hasRoute = true;
        break;
      }
    }
  }
  return sol;
}

bool FlexGCWorker::Impl::checkMetalEndOfLine_eol_hasParallelEdge(gcSegment *edge, frSpacingEndOfLineConstraint *con, bool &hasRoute) {
  if (!con->hasParallelEdge()) {
    return true;
  }
  bool left  = checkMetalEndOfLine_eol_hasParallelEdge_oneDir(edge, con, true, hasRoute);
  bool right = checkMetalEndOfLine_eol_hasParallelEdge_oneDir(edge, con, false, hasRoute);
  if ((!con->hasTwoEdges()) && (left || right)) {
    return true;
  }
  if (con->hasTwoEdges() && left && right) {
    return true;
  }
  return false;
}

void FlexGCWorker::Impl::checkMetalEndOfLine_eol_hasEol_getQueryBox(gcSegment *edge, frSpacingEndOfLineConstraint *con, 
                                                              box_t &queryBox, gtl::rectangle_data<frCoord> &queryRect) {
  auto eolWithin = con->getEolWithin();
  auto eolSpace  = con->getMinSpacing();
  if (edge->getDir() == frDirEnum::E) {
    bg::set<bg::min_corner, 0>(queryBox, edge->low().x()  - eolWithin);
    bg::set<bg::min_corner, 1>(queryBox, edge->low().y()  - eolSpace);
    bg::set<bg::max_corner, 0>(queryBox, edge->high().x() + eolWithin);
    bg::set<bg::max_corner, 1>(queryBox, edge->high().y());
  } else if (edge->getDir() == frDirEnum::W) {
    bg::set<bg::min_corner, 0>(queryBox, edge->high().x() - eolWithin);
    bg::set<bg::min_corner, 1>(queryBox, edge->high().y());
    bg::set<bg::max_corner, 0>(queryBox, edge->low().x()  + eolWithin);
    bg::set<bg::max_corner, 1>(queryBox, edge->low().y()  + eolSpace);
  } else if (edge->getDir() == frDirEnum::N) {
    bg::set<bg::min_corner, 0>(queryBox, edge->low().x());
    bg::set<bg::min_corner, 1>(queryBox, edge->low().y()  - eolWithin);
    bg::set<bg::max_corner, 0>(queryBox, edge->high().x() + eolSpace);
    bg::set<bg::max_corner, 1>(queryBox, edge->high().y() + eolWithin);
  } else { // S
    bg::set<bg::min_corner, 0>(queryBox, edge->high().x() - eolSpace);
    bg::set<bg::min_corner, 1>(queryBox, edge->high().y() - eolWithin);
    bg::set<bg::max_corner, 0>(queryBox, edge->low().x());
    bg::set<bg::max_corner, 1>(queryBox, edge->low().y()  + eolWithin);
  }
  gtl::xl(queryRect, queryBox.min_corner().x());
  gtl::yl(queryRect, queryBox.min_corner().y());
  gtl::xh(queryRect, queryBox.max_corner().x());
  gtl::yh(queryRect, queryBox.max_corner().y());
}

void FlexGCWorker::Impl::checkMetalEndOfLine_eol_hasEol_helper(gcSegment *edge1, gcSegment *edge2, frSpacingEndOfLineConstraint *con) {
  bool enableOutput = printMarker_;
  auto layerNum = edge1->getLayerNum();
  auto net1 = edge1->getNet();
  auto net2 = edge2->getNet();
  gtl::rectangle_data<frCoord> markerRect, rect2;
  checkMetalEndOfLine_eol_hasParallelEdge_oneDir_getParallelEdgeRect(edge1, markerRect);
  checkMetalEndOfLine_eol_hasParallelEdge_oneDir_getParallelEdgeRect(edge2, rect2);
  gtl::generalized_intersect(markerRect, rect2);
  // skip if markerRect contains anything
  auto &workerRegionQuery = getWorkerRegionQuery();
  vector<rq_box_value_t<gcRect*> > result;
  gtl::rectangle_data<frCoord> bloatMarkerRect(markerRect);
  if (gtl::area(markerRect) == 0) {
    if (edge1->getDir() == frDirEnum::W || edge1->getDir() == frDirEnum::E) {
      gtl::bloat(bloatMarkerRect, gtl::HORIZONTAL, 1);
    } else if (edge1->getDir() == frDirEnum::S || edge1->getDir() == frDirEnum::N) {
      gtl::bloat(bloatMarkerRect, gtl::VERTICAL, 1);
    }
  }
  box_t queryBox(point_t(gtl::xl(bloatMarkerRect), gtl::yl(bloatMarkerRect)), 
                 point_t(gtl::xh(bloatMarkerRect), gtl::yh(bloatMarkerRect)));
  workerRegionQuery.queryMaxRectangle(queryBox, layerNum, result);
  for (auto &[objBox, objPtr]: result) {
    if (gtl::intersects(bloatMarkerRect, *objPtr, false)) {
      return;
    }
  }

  auto marker = make_unique<frMarker>();
  frBox box(gtl::xl(markerRect), gtl::yl(markerRect), gtl::xh(markerRect), gtl::yh(markerRect));
  marker->setBBox(box);
  marker->setLayerNum(layerNum);
  marker->setConstraint(con);
  marker->addSrc(net1->getOwner());
  frCoord llx = min(edge1->getLowCorner()->x(), edge1->getHighCorner()->x());
  frCoord lly = min(edge1->getLowCorner()->y(), edge1->getHighCorner()->y());
  frCoord urx = max(edge1->getLowCorner()->x(), edge1->getHighCorner()->x());
  frCoord ury = max(edge1->getLowCorner()->y(), edge1->getHighCorner()->y());
  marker->addVictim(net1->getOwner(), make_tuple(edge1->getLayerNum(),
                                                 frBox(llx, lly, urx, ury),
                                                 edge1->isFixed()));
  marker->addSrc(net2->getOwner());
  llx = min(edge2->getLowCorner()->x(), edge2->getHighCorner()->x());
  lly = min(edge2->getLowCorner()->y(), edge2->getHighCorner()->y());
  urx = max(edge2->getLowCorner()->x(), edge2->getHighCorner()->x());
  ury = max(edge2->getLowCorner()->y(), edge2->getHighCorner()->y());
  marker->addAggressor(net2->getOwner(), make_tuple(edge2->getLayerNum(), 
                                                    frBox(llx, lly, urx, ury),
                                                    edge2->isFixed()));
  if (addMarker(std::move(marker))) {
    // true marker
    if (enableOutput) {
      double dbu = getDesign()->getTopBlock()->getDBUPerUU();
      cout <<"EOLSpc@(" <<gtl::xl(markerRect) / dbu <<", " <<gtl::yl(markerRect) / dbu <<") ("
                        <<gtl::xh(markerRect) / dbu <<", " <<gtl::yh(markerRect) / dbu <<") "
           <<getDesign()->getTech()->getLayer(layerNum)->getName() <<" ";
      auto owner = net1->getOwner();
      if (owner == nullptr) {
        cout <<"FLOATING";
      } else {
        if (owner->typeId() == frcNet) {
          cout <<static_cast<frNet*>(owner)->getName();
        } else if (owner->typeId() == frcInstTerm) {
          cout <<static_cast<frInstTerm*>(owner)->getInst()->getName() <<"/" 
               <<static_cast<frInstTerm*>(owner)->getTerm()->getName();
        } else if (owner->typeId() == frcTerm) {
          cout <<"PIN/" <<static_cast<frTerm*>(owner)->getName();
        } else if (owner->typeId() == frcInstBlockage) {
          cout <<static_cast<frInstBlockage*>(owner)->getInst()->getName() <<"/OBS";
        } else if (owner->typeId() == frcBlockage) {
          cout <<"PIN/OBS";
        } else {
          cout <<"UNKNOWN";
        }
      }
      cout <<" ";
      owner = net2->getOwner();
      if (owner == nullptr) {
        cout <<"FLOATING";
      } else {
        if (owner->typeId() == frcNet) {
          cout <<static_cast<frNet*>(owner)->getName();
        } else if (owner->typeId() == frcInstTerm) {
          cout <<static_cast<frInstTerm*>(owner)->getInst()->getName() <<"/" 
               <<static_cast<frInstTerm*>(owner)->getTerm()->getName();
        } else if (owner->typeId() == frcTerm) {
          cout <<"PIN/" <<static_cast<frTerm*>(owner)->getName();
        } else if (owner->typeId() == frcInstBlockage) {
          cout <<static_cast<frInstBlockage*>(owner)->getInst()->getName() <<"/OBS";
        } else if (owner->typeId() == frcBlockage) {
          cout <<"PIN/OBS";
        } else {
          cout <<"UNKNOWN";
        }
      }
      cout <<endl;
    }
  }

}

void FlexGCWorker::Impl::checkMetalEndOfLine_eol_hasEol(gcSegment *edge, frSpacingEndOfLineConstraint *con, bool hasRoute) {
  auto layerNum = edge->getLayerNum();
  box_t queryBox;
  gtl::rectangle_data<frCoord> queryRect; // original size
  checkMetalEndOfLine_eol_hasEol_getQueryBox(edge, con, queryBox, queryRect);

  gtl::rectangle_data<frCoord> triggerRect;
  vector<pair<segment_t, gcSegment*> > results;
  auto &workerRegionQuery = getWorkerRegionQuery();
  workerRegionQuery.queryPolygonEdge(queryBox, edge->getLayerNum(), results);
  gtl::polygon_90_set_data<frCoord> tmpPoly;
  for (auto &[boostSeg, ptr]: results) {
    // skip if non oppo-dir edge
    if ((int)edge->getDir() + (int)ptr->getDir() != OPPOSITEDIR) {
      continue;
    }
    checkMetalEndOfLine_eol_hasParallelEdge_oneDir_getParallelEdgeRect(ptr, triggerRect);
    // skip if no area
    if (!gtl::intersects(queryRect, triggerRect, false)) {
      continue;
    }
    // skip if no route shapes
    if (!hasRoute && !ptr->isFixed()) {
      tmpPoly.clear();
      // tmpPoly is the intersection of queryRect and minimum paralleledge rect
      using namespace boost::polygon::operators;
      tmpPoly += queryRect;
      tmpPoly &= triggerRect;
      auto &polys = ptr->getNet()->getPolygons(layerNum, false);
      // tmpPoly should have route shapes in order to be considered route
      tmpPoly &= polys;
      if (!gtl::empty(tmpPoly)) {
        hasRoute = true;
      }
    }
    // skip if no route
    if (!hasRoute) {
      continue;
    }
    checkMetalEndOfLine_eol_hasEol_helper(edge, ptr, con);
  }
}

void FlexGCWorker::Impl::checkMetalEndOfLine_eol(gcSegment *edge, frSpacingEndOfLineConstraint *con) {
  if (!checkMetalEndOfLine_eol_isEolEdge(edge, con)) {
    return;
  }
  auto layerNum = edge->getLayerNum();
  // check left/right parallel edge
  //auto hasRoute = edge->isFixed() ? false : true;
  // check if current eol edge has route shapes
  bool hasRoute = false;
  if (!edge->isFixed()) {
    gtl::polygon_90_set_data<frCoord> tmpPoly;
    gtl::rectangle_data<frCoord> triggerRect;
    checkMetalEndOfLine_eol_hasParallelEdge_oneDir_getParallelEdgeRect(edge, triggerRect);
    // tmpPoly is the intersection of queryRect and minimum paralleledge rect
    using namespace boost::polygon::operators;
    tmpPoly += triggerRect;
    auto &polys = edge->getNet()->getPolygons(layerNum, false);
    // tmpPoly should have route shapes in order to be considered route
    tmpPoly &= polys;
    if (!gtl::empty(tmpPoly)) {
      hasRoute = true;
    }
  }
  auto triggered = checkMetalEndOfLine_eol_hasParallelEdge(edge, con, hasRoute);
  if (!triggered) {
    return;
  }
  // check eol
  checkMetalEndOfLine_eol_hasEol(edge, con, hasRoute);
}

void FlexGCWorker::Impl::checkMetalEndOfLine_main(gcPin* pin) {
  //bool enableOutput = true;

  auto poly = pin->getPolygon();
  auto layerNum = poly->getLayerNum();
  //auto net = poly->getNet();

  auto &cons = design_->getTech()->getLayer(layerNum)->getEolSpacing();
  if (cons.empty()) {
    return;
  }

  for (auto &edges: pin->getPolygonEdges()) {
    for (auto &edge: edges) {
      for (auto con: cons) {
        checkMetalEndOfLine_eol(edge.get(), con);
      }
    }
  }
}

void FlexGCWorker::Impl::checkMetalEndOfLine() {
  if (targetNet_) {
    // layer --> net --> polygon
    for (int i = std::max((frLayerNum)(getDesign()->getTech()->getBottomLayerNum()), minLayerNum_); 
         i <= std::min((frLayerNum)(getDesign()->getTech()->getTopLayerNum()), maxLayerNum_); i++) {
      auto currLayer = getDesign()->getTech()->getLayer(i);
      if (currLayer->getType() != frLayerTypeEnum::ROUTING) {
        continue;
      }
      for (auto &pin: targetNet_->getPins(i)) {
        checkMetalEndOfLine_main(pin.get());
      }
    }
  } else {
    // layer --> net --> polygon
    for (int i = std::max((frLayerNum)(getDesign()->getTech()->getBottomLayerNum()), minLayerNum_); 
         i <= std::min((frLayerNum)(getDesign()->getTech()->getTopLayerNum()), maxLayerNum_); i++) {
      auto currLayer = getDesign()->getTech()->getLayer(i);
      if (currLayer->getType() != frLayerTypeEnum::ROUTING) {
        continue;
      }
      for (auto &net: getNets()) {
        for (auto &pin: net->getPins(i)) {
          checkMetalEndOfLine_main(pin.get());
        }
      }
    }
  }
}

frCoord FlexGCWorker::Impl::checkCutSpacing_getMaxSpcVal(frCutSpacingConstraint* con) {
  frCoord maxSpcVal = 0;
  if (con) {
    maxSpcVal = con->getCutSpacing();
    if (con->isAdjacentCuts()) {
      maxSpcVal = std::max(maxSpcVal, con->getCutWithin());
    }
  }
  return maxSpcVal;
}

frCoord FlexGCWorker::Impl::checkLef58CutSpacing_getMaxSpcVal(frLef58CutSpacingConstraint* con) {
  frCoord maxSpcVal = 0;
  if (con) {
    maxSpcVal = con->getCutSpacing();
    if (con->hasAdjacentCuts()) {
      maxSpcVal = std::max(maxSpcVal, con->getCutWithin());
    }
  }
  return maxSpcVal;
}

frCoord FlexGCWorker::Impl::checkCutSpacing_spc_getReqSpcVal(gcRect* ptr1, gcRect* ptr2, frCutSpacingConstraint* con) {
  frCoord maxSpcVal = 0;
  if (con) {
    maxSpcVal = con->getCutSpacing();
    if (con->isAdjacentCuts()) {
      auto owner = ptr1->getNet()->getOwner();
      auto ptr1LayerNum = ptr1->getLayerNum();
      auto ptr1Layer = getDesign()->getTech()->getLayer(ptr1LayerNum);
      if (owner && (owner->typeId() == frcInstBlockage || owner->typeId() == frcBlockage) && ptr1->width() > int(ptr1Layer->getWidth())) {
        maxSpcVal = con->getCutWithin();
      }
      owner = ptr2->getNet()->getOwner();
      auto ptr2LayerNum = ptr2->getLayerNum();
      auto ptr2Layer = getDesign()->getTech()->getLayer(ptr2LayerNum);
      if (owner && (owner->typeId() == frcInstBlockage || owner->typeId() == frcBlockage) && ptr2->width() > int(ptr2Layer->getWidth())) {
        maxSpcVal = con->getCutWithin();
      }
    }
  }
  return maxSpcVal;
}

void FlexGCWorker::Impl::checkCutSpacing_short(gcRect* rect1, gcRect* rect2, const gtl::rectangle_data<frCoord> &markerRect) {
  bool enableOutput = printMarker_;

  auto layerNum = rect1->getLayerNum();
  auto net1 = rect1->getNet();
  auto net2 = rect2->getNet();
  // skip if different layer
  if (rect1->getLayerNum() != rect2->getLayerNum()) {
    return;
  }
  // skip fixed shape
  if (rect1->isFixed() && rect2->isFixed()) {
    return;
  }

  auto marker = make_unique<frMarker>();
  frBox box(gtl::xl(markerRect), gtl::yl(markerRect), gtl::xh(markerRect), gtl::yh(markerRect));
  marker->setBBox(box);
  marker->setLayerNum(layerNum);
  marker->setConstraint(getDesign()->getTech()->getLayer(layerNum)->getShortConstraint());
  marker->addSrc(net1->getOwner());
  marker->addVictim(net1->getOwner(), make_tuple(rect1->getLayerNum(),
                                                  frBox(gtl::xl(*rect1), gtl::yl(*rect1), gtl::xh(*rect1), gtl::yh(*rect1)),
                                                  rect1->isFixed()));
  marker->addSrc(net2->getOwner());
  marker->addAggressor(net2->getOwner(), make_tuple(rect2->getLayerNum(),
                                                    frBox(gtl::xl(*rect2), gtl::yl(*rect2), gtl::xh(*rect2), gtl::yh(*rect2)),
                                                    rect2->isFixed()));

  if (addMarker(std::move(marker))) {
    // true marker
    if (enableOutput) {
      double dbu = getDesign()->getTopBlock()->getDBUPerUU();
      cout <<"CShort@(";
      cout <<gtl::xl(markerRect) / dbu <<", " <<gtl::yl(markerRect) / dbu <<") ("
           <<gtl::xh(markerRect) / dbu <<", " <<gtl::yh(markerRect) / dbu <<") "
           <<getDesign()->getTech()->getLayer(layerNum)->getName() <<" ";
      auto owner = net1->getOwner();
      if (owner == nullptr) {
        cout <<"FLOATING";
      } else {
        if (owner->typeId() == frcNet) {
          cout <<static_cast<frNet*>(owner)->getName();
        } else if (owner->typeId() == frcInstTerm) {
          cout <<static_cast<frInstTerm*>(owner)->getInst()->getName() <<"/" 
               <<static_cast<frInstTerm*>(owner)->getTerm()->getName();
        } else if (owner->typeId() == frcTerm) {
          cout <<"PIN/" <<static_cast<frTerm*>(owner)->getName();
        } else if (owner->typeId() == frcInstBlockage) {
          cout <<static_cast<frInstBlockage*>(owner)->getInst()->getName() <<"/OBS";
        } else if (owner->typeId() == frcBlockage) {
          cout <<"PIN/OBS";
        } else {
          cout <<"UNKNOWN";
        }
      }
      cout <<" ";
      owner = net2->getOwner();
      if (owner == nullptr) {
        cout <<"FLOATING";
      } else {
        if (owner->typeId() == frcNet) {
          cout <<static_cast<frNet*>(owner)->getName();
        } else if (owner->typeId() == frcInstTerm) {
          cout <<static_cast<frInstTerm*>(owner)->getInst()->getName() <<"/" 
               <<static_cast<frInstTerm*>(owner)->getTerm()->getName();
        } else if (owner->typeId() == frcTerm) {
          cout <<"PIN/" <<static_cast<frTerm*>(owner)->getName();
        } else if (owner->typeId() == frcInstBlockage) {
          cout <<static_cast<frInstBlockage*>(owner)->getInst()->getName() <<"/OBS";
        } else if (owner->typeId() == frcBlockage) {
          cout <<"PIN/OBS";
        } else {
          cout <<"UNKNOWN";
        }
      }
      cout <<endl;
    }
  }
}

void FlexGCWorker::Impl::checkCutSpacing_spc(gcRect* rect1, gcRect* rect2, const gtl::rectangle_data<frCoord> &markerRect, 
                                       frCutSpacingConstraint* con, frCoord prl) {
  bool enableOutput = printMarker_;
  // bool enableOutput = true;
  auto layerNum = rect1->getLayerNum();
  auto net1 = rect1->getNet();
  auto net2 = rect2->getNet();
  // skip if adjcut && except samepgnet, note that adj cut handled given only rect1 outside this function
  if (con->isAdjacentCuts() && con->hasExceptSamePGNet() && net1 == net2 && net1->getOwner()) {
    auto owner = net1->getOwner();
    if (owner->typeId() == frcNet) {
      if (static_cast<frNet*>(owner)->getType() == frNetEnum::frcPowerNet ||
          static_cast<frNet*>(owner)->getType() == frNetEnum::frcGroundNet) {
        return;
      }
    } else if (owner->typeId() == frcTerm) {
      if (static_cast<frTerm*>(owner)->getType() == frTermEnum::frcPowerTerm ||
          static_cast<frTerm*>(owner)->getType() == frTermEnum::frcGroundTerm) {
        return;
      }
    } else if (owner->typeId() == frcInstTerm) {
      if (static_cast<frInstTerm*>(owner)->getTerm()->getType() == frTermEnum::frcPowerTerm ||
          static_cast<frInstTerm*>(owner)->getTerm()->getType() == frTermEnum::frcGroundTerm) {
        return;
      }
    }
  }
  if (con->isParallelOverlap()) {
    // skip if no parallel overlap
    if (prl <= 0) {
      return;
    // skip if paralell overlap but shares the same above/below metal
    } else {
      box_t queryBox;
      myBloat(markerRect, 0, queryBox);
      auto &workerRegionQuery = getWorkerRegionQuery();
      vector<rq_box_value_t<gcRect*> > result;
      auto secondLayerNum = rect1->getLayerNum() - 1;
      if (secondLayerNum >= getDesign()->getTech()->getBottomLayerNum() &&
          secondLayerNum <= getDesign()->getTech()->getTopLayerNum()) {
        workerRegionQuery.queryMaxRectangle(queryBox, secondLayerNum, result);
      }
      secondLayerNum = rect1->getLayerNum() + 1;
      if (secondLayerNum >= getDesign()->getTech()->getBottomLayerNum() &&
          secondLayerNum <= getDesign()->getTech()->getTopLayerNum()) {
        workerRegionQuery.queryMaxRectangle(queryBox, secondLayerNum, result);
      }
      for (auto &[objBox, objPtr]: result) {
        if ((objPtr->getNet() == net1 || objPtr->getNet() == net2) && objBox.contains(queryBox)) {
          return;
        }
      }
    }
  }
  // skip if not reaching area
  if (con->isArea() && gtl::area(*rect1) < con->getCutArea() && gtl::area(*rect2) < con->getCutArea()) {
    return;
  }

  // no violation if spacing satisfied
  auto reqSpcValSquare = checkCutSpacing_spc_getReqSpcVal(rect1, rect2, con);
  reqSpcValSquare *= reqSpcValSquare;

  gtl::point_data<frCoord> center1, center2;
  gtl::center(center1, *rect1);
  gtl::center(center2, *rect2);
  frCoord distSquare = 0;
  if (con->hasCenterToCenter()) {
    distSquare = gtl::distance_squared(center1, center2);
  } else {
    distSquare = gtl::square_euclidean_distance(*rect1, *rect2);
  }
  if (distSquare >= reqSpcValSquare) {
    return;
  }
  // no violation if fixed shapes
  if (rect1->isFixed() && rect2->isFixed()) {
    return;
  }

  auto marker = make_unique<frMarker>();
  frBox box(gtl::xl(markerRect), gtl::yl(markerRect), gtl::xh(markerRect), gtl::yh(markerRect));
  marker->setBBox(box);
  marker->setLayerNum(layerNum);
  marker->setConstraint(con);
  marker->addSrc(net1->getOwner());
  marker->addVictim(net1->getOwner(), make_tuple(rect1->getLayerNum(),
                                                  frBox(gtl::xl(*rect1), gtl::yl(*rect1), gtl::xh(*rect1), gtl::yh(*rect1)),
                                                  rect1->isFixed()));
  marker->addSrc(net2->getOwner());
  marker->addAggressor(net2->getOwner(), make_tuple(rect2->getLayerNum(),
                                                    frBox(gtl::xl(*rect2), gtl::yl(*rect2), gtl::xh(*rect2), gtl::yh(*rect2)),
                                                    rect2->isFixed()));
  if (addMarker(std::move(marker))) {
    // true marker
    if (enableOutput) {
      double dbu = getDesign()->getTopBlock()->getDBUPerUU();
      cout <<"CutSpc@(" <<gtl::xl(markerRect) / dbu <<", " <<gtl::yl(markerRect) / dbu <<") ("
                        <<gtl::xh(markerRect) / dbu <<", " <<gtl::yh(markerRect) / dbu <<") "
           <<getDesign()->getTech()->getLayer(layerNum)->getName() <<" ";
      auto owner = net1->getOwner();
      if (owner == nullptr) {
        cout <<"FLOATING";
      } else {
        if (owner->typeId() == frcNet) {
          cout <<static_cast<frNet*>(owner)->getName();
        } else if (owner->typeId() == frcInstTerm) {
          cout <<static_cast<frInstTerm*>(owner)->getInst()->getName() <<"/" 
               <<static_cast<frInstTerm*>(owner)->getTerm()->getName();
        } else if (owner->typeId() == frcTerm) {
          cout <<"PIN/" <<static_cast<frTerm*>(owner)->getName();
        } else if (owner->typeId() == frcInstBlockage) {
          cout <<static_cast<frInstBlockage*>(owner)->getInst()->getName() <<"/OBS";
        } else if (owner->typeId() == frcBlockage) {
          cout <<"PIN/OBS";
        } else {
          cout <<"UNKNOWN";
        }
      }
      cout <<" ";
      owner = net2->getOwner();
      if (owner == nullptr) {
        cout <<"FLOATING";
      } else {
        if (owner->typeId() == frcNet) {
          cout <<static_cast<frNet*>(owner)->getName();
        } else if (owner->typeId() == frcInstTerm) {
          cout <<static_cast<frInstTerm*>(owner)->getInst()->getName() <<"/" 
               <<static_cast<frInstTerm*>(owner)->getTerm()->getName();
        } else if (owner->typeId() == frcTerm) {
          cout <<"PIN/" <<static_cast<frTerm*>(owner)->getName();
        } else if (owner->typeId() == frcInstBlockage) {
          cout <<static_cast<frInstBlockage*>(owner)->getInst()->getName() <<"/OBS";
        } else if (owner->typeId() == frcBlockage) {
          cout <<"PIN/OBS";
        } else {
          cout <<"UNKNOWN";
        }
      }
      cout <<endl;
    }
  }
}

void FlexGCWorker::Impl::checkCutSpacing_spc_diff_layer(gcRect* rect1, gcRect* rect2, 
                                                  const gtl::rectangle_data<frCoord> &markerRect, frCutSpacingConstraint* con) {
  bool enableOutput = printMarker_;
  
  // no violation if fixed shapes
  if (rect1->isFixed() && rect2->isFixed()) {
    return;
  }
  
  auto layerNum = rect1->getLayerNum();
  auto net1 = rect1->getNet();
  auto net2 = rect2->getNet();
  if (con->hasStack() && con->hasSameNet() && net1 == net2) {
    if (gtl::contains(*rect1, *rect2) || gtl::contains(*rect2, *rect1)) {
      return;
    }
  }

  // no violation if spacing satisfied
  auto reqSpcValSquare = checkCutSpacing_spc_getReqSpcVal(rect1, rect2, con);
  reqSpcValSquare *= reqSpcValSquare;

  gtl::point_data<frCoord> center1, center2;
  gtl::center(center1, *rect1);
  gtl::center(center2, *rect2);
  frCoord distSquare = 0;

  if (con->hasCenterToCenter()) {
    distSquare = gtl::distance_squared(center1, center2);
  } else {
    // if overlap, still calculate c2c
    if (gtl::intersects(*rect1, *rect2, false)) {
      distSquare = gtl::distance_squared(center1, center2);
    } else {
      distSquare = gtl::square_euclidean_distance(*rect1, *rect2);
    }
  }
  if (distSquare >= reqSpcValSquare) {
    return;
  }

  auto marker = make_unique<frMarker>();
  frBox box(gtl::xl(markerRect), gtl::yl(markerRect), gtl::xh(markerRect), gtl::yh(markerRect));
  marker->setBBox(box);
  marker->setLayerNum(layerNum);
  marker->setConstraint(con);
  marker->addSrc(net1->getOwner());
  marker->addVictim(net1->getOwner(), make_tuple(rect1->getLayerNum(),
                                                  frBox(gtl::xl(*rect1), gtl::yl(*rect1), gtl::xh(*rect1), gtl::yh(*rect1)),
                                                  rect1->isFixed()));
  marker->addSrc(net2->getOwner());
  marker->addAggressor(net2->getOwner(), make_tuple(rect2->getLayerNum(),
                                                    frBox(gtl::xl(*rect2), gtl::yl(*rect2), gtl::xh(*rect2), gtl::yh(*rect2)),
                                                    rect2->isFixed()));
  if (addMarker(std::move(marker))) {
    // true marker
    if (enableOutput) {
      double dbu = getDesign()->getTopBlock()->getDBUPerUU();
      cout <<"CutSpc@(" <<gtl::xl(markerRect) / dbu <<", " <<gtl::yl(markerRect) / dbu <<") ("
                        <<gtl::xh(markerRect) / dbu <<", " <<gtl::yh(markerRect) / dbu <<") "
           <<getDesign()->getTech()->getLayer(layerNum)->getName() <<" ";
      auto owner = net1->getOwner();
      if (owner == nullptr) {
        cout <<"FLOATING";
      } else {
        if (owner->typeId() == frcNet) {
          cout <<static_cast<frNet*>(owner)->getName();
        } else if (owner->typeId() == frcInstTerm) {
          cout <<static_cast<frInstTerm*>(owner)->getInst()->getName() <<"/" 
               <<static_cast<frInstTerm*>(owner)->getTerm()->getName();
        } else if (owner->typeId() == frcTerm) {
          cout <<"PIN/" <<static_cast<frTerm*>(owner)->getName();
        } else if (owner->typeId() == frcInstBlockage) {
          cout <<static_cast<frInstBlockage*>(owner)->getInst()->getName() <<"/OBS";
        } else if (owner->typeId() == frcBlockage) {
          cout <<"PIN/OBS";
        } else {
          cout <<"UNKNOWN";
        }
      }
      cout <<" ";
      owner = net2->getOwner();
      if (owner == nullptr) {
        cout <<"FLOATING";
      } else {
        if (owner->typeId() == frcNet) {
          cout <<static_cast<frNet*>(owner)->getName();
        } else if (owner->typeId() == frcInstTerm) {
          cout <<static_cast<frInstTerm*>(owner)->getInst()->getName() <<"/" 
               <<static_cast<frInstTerm*>(owner)->getTerm()->getName();
        } else if (owner->typeId() == frcTerm) {
          cout <<"PIN/" <<static_cast<frTerm*>(owner)->getName();
        } else if (owner->typeId() == frcInstBlockage) {
          cout <<static_cast<frInstBlockage*>(owner)->getInst()->getName() <<"/OBS";
        } else if (owner->typeId() == frcBlockage) {
          cout <<"PIN/OBS";
        } else {
          cout <<"UNKNOWN";
        }
      }
      cout <<endl;
    }
  }
}

// check LEF58 SPACING constraint for cut layer
// rect1 ==  victim, rect2 == aggressor
void FlexGCWorker::Impl::checkLef58CutSpacing_main(gcRect* rect1, gcRect* rect2, frLef58CutSpacingConstraint* con) {
  // skip if same obj
  if (rect1 == rect2) {
    return;
  }
  // skip if con is same net rule but the two objs are diff net
  if (con->isSameNet() && rect1->getNet() != rect2->getNet()) {
    return;
  }

  gtl::rectangle_data<frCoord> markerRect(*rect1);
  gtl::generalized_intersect(markerRect, *rect2);

  if (con->hasSecondLayer()) {
    checkLef58CutSpacing_spc_layer(rect1, rect2, markerRect, con);
  } else if (con->hasAdjacentCuts()) {
    checkLef58CutSpacing_spc_adjCut(rect1, rect2, markerRect, con);
  } else {
    logger_->warn(DRT, 44, "Unsupported LEF58_SPACING rule for cut layer, skipped.");
  }
}

bool FlexGCWorker::Impl::checkLef58CutSpacing_spc_hasAdjCuts(gcRect* rect, frLef58CutSpacingConstraint* con) {
  auto layerNum = rect->getLayerNum();
  auto layer = getDesign()->getTech()->getLayer(layerNum);

  auto conCutClassIdx = con->getCutClassIdx();

  auto cutWithinSquare = con->getCutWithin();
  box_t queryBox;
  myBloat(*rect, cutWithinSquare, queryBox);
  cutWithinSquare *= cutWithinSquare;
  auto &workerRegionQuery = getWorkerRegionQuery();
  vector<rq_box_value_t<gcRect*> > result;
  workerRegionQuery.queryMaxRectangle(queryBox, layerNum, result);
  int reqNumCut = con->getAdjacentCuts();
  int cnt = -1;
  gtl::point_data<frCoord> center1, center2;
  gtl::center(center1, *rect);
  // count adj cuts
  for (auto &[objBox, ptr]: result) {
    frCoord distSquare = 0;
    if (con->isCenterToCenter()) {
      gtl::center(center2, *ptr);
      distSquare = gtl::distance_squared(center1, center2);
    } else {
      distSquare = gtl::square_euclidean_distance(*rect, *ptr);
    }
    if (distSquare >= cutWithinSquare) {
      continue;
    }

    auto cutClassIdx = layer->getCutClassIdx(ptr->width(), ptr->length());
    if (cutClassIdx == conCutClassIdx) {
      cnt++;
    }
  }
  if (cnt >= reqNumCut) {
    return true;
  } else {
    return false;
  }
}

bool FlexGCWorker::Impl::checkLef58CutSpacing_spc_hasTwoCuts(gcRect* rect1, 
                                                       gcRect* rect2, 
                                                       frLef58CutSpacingConstraint* con) {
  if (checkLef58CutSpacing_spc_hasTwoCuts_helper(rect1, con) && checkLef58CutSpacing_spc_hasTwoCuts_helper(rect2, con)) {
    return true;
  } else {
    return false;
  }
}

bool FlexGCWorker::Impl::checkLef58CutSpacing_spc_hasTwoCuts_helper(gcRect* rect,
                                                              frLef58CutSpacingConstraint* con) {
  auto layerNum = rect->getLayerNum();
  auto layer = getDesign()->getTech()->getLayer(layerNum);

  auto conCutClassIdx = con->getCutClassIdx();

  auto cutWithinSquare = con->getCutWithin();
  box_t queryBox;
  myBloat(*rect, cutWithinSquare, queryBox);
  cutWithinSquare *= cutWithinSquare;
  auto &workerRegionQuery = getWorkerRegionQuery();
  vector<rq_box_value_t<gcRect*> > result;
  workerRegionQuery.queryMaxRectangle(queryBox, layerNum, result);
  int reqNumCut = con->getTwoCuts();
  int cnt = -1;
  gtl::point_data<frCoord> center1, center2;
  gtl::center(center1, *rect);
  // count adj cuts
  for (auto &[objBox, ptr]: result) {
    frCoord distSquare = 0;
    if (con->isCenterToCenter()) {
      gtl::center(center2, *ptr);
      distSquare = gtl::distance_squared(center1, center2);
    } else {
      distSquare = gtl::square_euclidean_distance(*rect, *ptr);
    }
    if (distSquare >= cutWithinSquare) {
      continue;
    }

    auto cutClassIdx = layer->getCutClassIdx(ptr->width(), ptr->length());
    if (cutClassIdx == conCutClassIdx) {
      cnt++;
    }
  }

  if (cnt >= reqNumCut) {
    return true;
  } else {
    return false;
  }
}

frCoord FlexGCWorker::Impl::checkLef58CutSpacing_spc_getReqSpcVal(gcRect* ptr1, gcRect* ptr2, frLef58CutSpacingConstraint* con) {
  frCoord maxSpcVal = 0;
  if (con) {
    maxSpcVal = con->getCutSpacing();
    if (con->hasAdjacentCuts()) {
      auto owner = ptr1->getNet()->getOwner();
      auto ptr1LayerNum = ptr1->getLayerNum();
      auto ptr1Layer = getDesign()->getTech()->getLayer(ptr1LayerNum);
      if (owner && (owner->typeId() == frcInstBlockage || owner->typeId() == frcBlockage) && ptr1->width() > int(ptr1Layer->getWidth())) {
        maxSpcVal = con->getCutWithin();
      }
      owner = ptr2->getNet()->getOwner();
      auto ptr2LayerNum = ptr2->getLayerNum();
      auto ptr2Layer = getDesign()->getTech()->getLayer(ptr2LayerNum);
      if (owner && (owner->typeId() == frcInstBlockage || owner->typeId() == frcBlockage) && ptr2->width() > int(ptr2Layer->getWidth())) {
        maxSpcVal = con->getCutWithin();
      }
    }
  }
  return maxSpcVal;
}

// only works for GF14 syntax (i.e., TWOCUTS), not full rule support
void FlexGCWorker::Impl::checkLef58CutSpacing_spc_adjCut(gcRect* rect1, 
                                                   gcRect* rect2, 
                                                   const gtl::rectangle_data<frCoord> &markerRect,
                                                   frLef58CutSpacingConstraint* con) {
  // bool enableOutput = true;
  bool enableOutput = false;

  auto layerNum = rect1->getLayerNum();
  auto net1 = rect1->getNet();
  auto net2 = rect2->getNet();

  bool isSkip = true;
  if (checkLef58CutSpacing_spc_hasAdjCuts(rect1, con)) {
    isSkip = false;
  }
  if (con->hasTwoCuts() && checkLef58CutSpacing_spc_hasTwoCuts(rect1, rect2, con)) {
    isSkip = false;
  }
  // skip only when neither adjCuts nor twoCuts is satisfied
  if (isSkip) {
    return;
  }

  if (con->hasExactAligned()) {
    logger_->warn(DRT, 45, " unsupported branch EXACTALIGNED in checkLef58CutSpacing_spc_adjCut.");
    return;
  }
  if (con->isExceptSamePGNet()) {
    logger_->warn(DRT, 46, " unsupported branch EXCEPTSAMEPGNET in checkLef58CutSpacing_spc_adjCut.");
    return;
  }
  if (con->hasExceptAllWithin()) {
    logger_->warn(DRT, 47, " unsupported branch EXCEPTALLWITHIN in checkLef58CutSpacing_spc_adjCut.");
    return;
  }
  if (con->isToAll()) {
    logger_->warn(DRT, 48, " unsupported branch TO ALL in checkLef58CutSpacing_spc_adjCut.");
    return;
  }
  if (con->isNoPrl()) {
    logger_->warn(DRT, 49, " unsupported branch NOPRL in checkLef58CutSpacing_spc_adjCut.");
    return;
  }
  if (con->hasEnclosure()) {
    logger_->warn(DRT, 50, " unsupported branch ENCLOSURE in checkLef58CutSpacing_spc_adjCut.");
    return;
  }
  if (con->isSideParallelOverlap()) {
    logger_->warn(DRT, 51, " unsupported branch SIDEPARALLELOVERLAP in checkLef58CutSpacing_spc_adjCut.");
    return;
  }
  if (con->isSameMask()) {
    logger_->warn(DRT, 52, " unsupported branch SAMEMASK in checkLef58CutSpacing_spc_adjCut.");
    return;
  }

  // start checking
  if (con->hasExactAligned()) {
    ;
  }
  if (con->isExceptSamePGNet()) {
    ;
  }
  if (con->hasExceptAllWithin()) {
    ;
  }
  if (con->hasEnclosure()) {
    ;
  }
  if (con->isNoPrl()) {
    ;
  }
  if (con->isSameMask()) {
    ;
  }

  auto reqSpcValSquare = checkLef58CutSpacing_spc_getReqSpcVal(rect1, rect2, con);
  reqSpcValSquare *= reqSpcValSquare;

  gtl::point_data<frCoord> center1, center2;
  gtl::center(center1, *rect1);
  gtl::center(center2, *rect2);
  frCoord distSquare = 0;
  if (con->isCenterToCenter()) {
    distSquare = gtl::distance_squared(center1, center2);
  } else {
    distSquare = gtl::square_euclidean_distance(*rect1, *rect2);
  }
  if (distSquare >= reqSpcValSquare) {
    return;
  }
  // no violation if fixed shapes
  if (rect1->isFixed() && rect2->isFixed()) {
    return;
  }

  auto marker = make_unique<frMarker>();
  frBox box(gtl::xl(markerRect), gtl::yl(markerRect), gtl::xh(markerRect), gtl::yh(markerRect));
  marker->setBBox(box);
  marker->setLayerNum(layerNum);
  marker->setConstraint(con);
  marker->addSrc(net1->getOwner());
  marker->addVictim(net1->getOwner(), make_tuple(rect1->getLayerNum(),
                                                  frBox(gtl::xl(*rect1), gtl::yl(*rect1), gtl::xh(*rect1), gtl::yh(*rect1)),
                                                  rect1->isFixed()));
  marker->addSrc(net2->getOwner());
  marker->addAggressor(net2->getOwner(), make_tuple(rect2->getLayerNum(),
                                                    frBox(gtl::xl(*rect2), gtl::yl(*rect2), gtl::xh(*rect2), gtl::yh(*rect2)),
                                                    rect2->isFixed()));
  if (addMarker(std::move(marker))) {
    // true marker
    if (enableOutput) {
      double dbu = getDesign()->getTopBlock()->getDBUPerUU();
      cout <<"CutSpc@(" <<gtl::xl(markerRect) / dbu <<", " <<gtl::yl(markerRect) / dbu <<") ("
                        <<gtl::xh(markerRect) / dbu <<", " <<gtl::yh(markerRect) / dbu <<") "
           <<getDesign()->getTech()->getLayer(layerNum)->getName() <<" ";
      auto owner = net1->getOwner();
      if (owner == nullptr) {
        cout <<"FLOATING";
      } else {
        if (owner->typeId() == frcNet) {
          cout <<static_cast<frNet*>(owner)->getName();
        } else if (owner->typeId() == frcInstTerm) {
          cout <<static_cast<frInstTerm*>(owner)->getInst()->getName() <<"/" 
               <<static_cast<frInstTerm*>(owner)->getTerm()->getName();
        } else if (owner->typeId() == frcTerm) {
          cout <<"PIN/" <<static_cast<frTerm*>(owner)->getName();
        } else if (owner->typeId() == frcInstBlockage) {
          cout <<static_cast<frInstBlockage*>(owner)->getInst()->getName() <<"/OBS";
        } else if (owner->typeId() == frcBlockage) {
          cout <<"PIN/OBS";
        } else {
          cout <<"UNKNOWN";
        }
      }
      cout <<" ";
      owner = net2->getOwner();
      if (owner == nullptr) {
        cout <<"FLOATING";
      } else {
        if (owner->typeId() == frcNet) {
          cout <<static_cast<frNet*>(owner)->getName();
        } else if (owner->typeId() == frcInstTerm) {
          cout <<static_cast<frInstTerm*>(owner)->getInst()->getName() <<"/" 
               <<static_cast<frInstTerm*>(owner)->getTerm()->getName();
        } else if (owner->typeId() == frcTerm) {
          cout <<"PIN/" <<static_cast<frTerm*>(owner)->getName();
        } else if (owner->typeId() == frcInstBlockage) {
          cout <<static_cast<frInstBlockage*>(owner)->getInst()->getName() <<"/OBS";
        } else if (owner->typeId() == frcBlockage) {
          cout <<"PIN/OBS";
        } else {
          cout <<"UNKNOWN";
        }
      }
      cout <<endl;
    }
  }

}

// only works for GF14 syntax, not full rule support
void FlexGCWorker::Impl::checkLef58CutSpacing_spc_layer(gcRect* rect1, 
                                                  gcRect* rect2, 
                                                  const gtl::rectangle_data<frCoord> &markerRect,
                                                  frLef58CutSpacingConstraint* con) {
  bool enableOutput = false;

  auto layerNum = rect1->getLayerNum();
  auto secondLayerNum = rect2->getLayerNum();
  auto net1 = rect1->getNet();
  auto net2 = rect2->getNet();
  auto reqSpcVal = con->getCutSpacing();
  auto reqSpcValSquare = reqSpcVal * reqSpcVal;

  // skip unsupported rule branch
  if (con->isStack()) {
    logger_->warn(DRT, 54, "unsupported branch STACK in checkLef58CutSpacing_spc_layer.");
    return;
  } else if (con->hasOrthogonalSpacing()) {
    logger_->warn(DRT, 55, "unsupported branch ORTHOGONALSPACING in checkLef58CutSpacing_spc_layer.");
    return;
  } else if (con->hasCutClass()) {
      ;
    if (con->isShortEdgeOnly()) {
      logger_->warn(DRT, 56, "unsupported branch SHORTEDGEONLY in checkLef58CutSpacing_spc_layer.");
      return;
    } else if (con->isConcaveCorner()) {
      if (con->hasWidth()) {
        logger_->warn(DRT, 57, "unsupported branch WIDTH in checkLef58CutSpacing_spc_layer.");
      } else if (con->hasParallel()) {
        logger_->warn(DRT, 58, "unsupported branch PARALLEL in checkLef58CutSpacing_spc_layer.");
      } else if (con->hasEdgeLength()) {
        logger_->warn(DRT, 59, "unsupported branch EDGELENGTH in checkLef58CutSpacing_spc_layer.");
      }
    } else if (con->hasExtension()) {
      logger_->warn(DRT, 60, "unsupported branch EXTENSION in checkLef58CutSpacing_spc_layer.");
    } else if (con->hasNonEolConvexCorner()) {
      ;
    } else if (con->hasAboveWidth()) {
      logger_->warn(DRT, 61, "unsupported branch ABOVEWIDTH in checkLef58CutSpacing_spc_layer.");
    } else if (con->isMaskOverlap()) {
      logger_->warn(DRT, 62, "unsupported branch MASKOVERLAP in checkLef58CutSpacing_spc_layer.");
    } else if (con->isWrongDirection()) {
      logger_->warn(DRT, 63, "unsupported branch WRONGDIRECTION in checkLef58CutSpacing_spc_layer.");
    }
  }

  // start checking
  if (con->isStack()) {
    ;
  } else if (con->hasOrthogonalSpacing()) {
    ;
  } else if (con->hasCutClass()) {
    auto conCutClassIdx = con->getCutClassIdx();
    auto cutClassIdx = getDesign()->getTech()->getLayer(layerNum)->getCutClassIdx(rect1->width(), rect1->length());
    if (cutClassIdx != conCutClassIdx) {
      return;
    }

    if (con->isShortEdgeOnly()) {
      ;
    } else if (con->isConcaveCorner()) {
      // skip if rect2 does not contains rect1
      if (!gtl::contains(*rect2, *rect1)) {
        return;
      }
      // query segment corner using the rect, not efficient, but code is cleaner
      box_t queryBox(point_t(gtl::xl(*rect2), gtl::yl(*rect2)), 
                     point_t(gtl::xh(*rect2), gtl::yh(*rect2)));
      vector<pair<segment_t, gcSegment*> > results;
      auto &workerRegionQuery = getWorkerRegionQuery();
      workerRegionQuery.queryPolygonEdge(queryBox, secondLayerNum, results);
      for (auto &[boostSeg, gcSeg]: results) {
        auto corner = gcSeg->getLowCorner();
        if (corner->getType() != frCornerTypeEnum::CONCAVE) {
          continue;
        }
        // exclude non-rect-boundary case
        if ((corner->x() != gtl::xl(*rect2) && corner->x() != gtl::xh(*rect2)) && 
            (corner->y() != gtl::yl(*rect2) && corner->y() != gtl::yh(*rect2))) {
          continue;
        }
        gtl::rectangle_data<frCoord> markerRect(corner->x(), corner->y(), corner->x(), corner->y());
        gtl::generalized_intersect(markerRect, *rect1);
        
        frCoord distSquare = 0;
        if (con->isCenterToCenter()) {
          gtl::point_data<frCoord> center1;
          gtl::center(center1, *rect1);
          distSquare = gtl::distance_squared(center1, corner->getNextEdge()->low());
        } else {
          distSquare = gtl::square_euclidean_distance(*rect1, corner->getNextEdge()->low());
        }
        if (distSquare >= reqSpcValSquare) {
          continue;
        }

        if (rect1->isFixed() && corner->isFixed()) {
          continue;
        }
        // this should only happen between samenet
        auto marker = make_unique<frMarker>();
        frBox box(gtl::xl(markerRect), gtl::yl(markerRect), gtl::xh(markerRect), gtl::yh(markerRect));
        marker->setBBox(box);
        marker->setLayerNum(layerNum);
        marker->setConstraint(con);
        marker->addSrc(net1->getOwner());
        marker->addVictim(net1->getOwner(), make_tuple(layerNum,
                                                       frBox(gtl::xl(*rect1), gtl::yl(*rect1), gtl::xh(*rect1), gtl::yh(*rect1)),
                                                       rect1->isFixed()));
        marker->addSrc(net2->getOwner());
        marker->addAggressor(net2->getOwner(), make_tuple(secondLayerNum, 
                                                          frBox(corner->x(), corner->y(), corner->x(), corner->y()),
                                                          corner->isFixed()));
        if (addMarker(std::move(marker))) {
          if (enableOutput) {
            double dbu = getDesign()->getTopBlock()->getDBUPerUU();
            cout <<"CutSpc@(" <<gtl::xl(markerRect) / dbu <<", " <<gtl::yl(markerRect) / dbu <<") ("
                              <<gtl::xh(markerRect) / dbu <<", " <<gtl::yh(markerRect) / dbu <<") "
                 <<getDesign()->getTech()->getLayer(layerNum)->getName() <<" ";
            auto owner = net1->getOwner();
            if (owner == nullptr) {
              cout <<"FLOATING";
            } else {
              if (owner->typeId() == frcNet) {
                cout <<static_cast<frNet*>(owner)->getName();
              } else if (owner->typeId() == frcInstTerm) {
                cout <<static_cast<frInstTerm*>(owner)->getInst()->getName() <<"/" 
                     <<static_cast<frInstTerm*>(owner)->getTerm()->getName();
              } else if (owner->typeId() == frcTerm) {
                cout <<"PIN/" <<static_cast<frTerm*>(owner)->getName();
              } else if (owner->typeId() == frcInstBlockage) {
                cout <<static_cast<frInstBlockage*>(owner)->getInst()->getName() <<"/OBS";
              } else if (owner->typeId() == frcBlockage) {
                cout <<"PIN/OBS";
              } else {
                cout <<"UNKNOWN";
              }
            }
            cout <<" ";
            owner = net2->getOwner();
            if (owner == nullptr) {
              cout <<"FLOATING";
            } else {
              if (owner->typeId() == frcNet) {
                cout <<static_cast<frNet*>(owner)->getName();
              } else if (owner->typeId() == frcInstTerm) {
                cout <<static_cast<frInstTerm*>(owner)->getInst()->getName() <<"/" 
                     <<static_cast<frInstTerm*>(owner)->getTerm()->getName();
              } else if (owner->typeId() == frcTerm) {
                cout <<"PIN/" <<static_cast<frTerm*>(owner)->getName();
              } else if (owner->typeId() == frcInstBlockage) {
                cout <<static_cast<frInstBlockage*>(owner)->getInst()->getName() <<"/OBS";
              } else if (owner->typeId() == frcBlockage) {
                cout <<"PIN/OBS";
              } else {
                cout <<"UNKNOWN";
              }
            }
            cout <<endl;
          }
        }
      }
    } else if (con->hasExtension()) {
      ;
    } else if (con->hasNonEolConvexCorner()) {
      // skip if rect2 does not contains rect1
      if (!gtl::contains(*rect2, *rect1)) {
        return;
      }
      // query segment corner using the rect, not efficient, but code is cleaner
      box_t queryBox(point_t(gtl::xl(*rect2), gtl::yl(*rect2)), 
                     point_t(gtl::xh(*rect2), gtl::yh(*rect2)));
      vector<pair<segment_t, gcSegment*> > results;
      auto &workerRegionQuery = getWorkerRegionQuery();
      workerRegionQuery.queryPolygonEdge(queryBox, secondLayerNum, results);
      for (auto &[boostSeg, gcSeg]: results) {
        auto corner = gcSeg->getLowCorner();
        if (corner->getType() != frCornerTypeEnum::CONVEX) {
          continue;
        }

        // skip for EOL corner
        bool isPrevEdgeEOL = false;
        bool isNextEdgeEOL = false;

        // check curr and prev form an EOL edge
        if (corner->getPrevCorner()->getType() == frCornerTypeEnum::CONVEX) {
          if (con->hasMinLength()) {
            // not EOL if minLength is not satisfied
            if (gtl::length(*(corner->getNextEdge())) < con->getMinLength() || 
                gtl::length(*(corner->getPrevCorner()->getPrevEdge())) < con->getMinLength()) {
              isPrevEdgeEOL = false;
            } else {
              isPrevEdgeEOL = true;
            }
          } else {
            isPrevEdgeEOL = true;
          }
        } else {
          isPrevEdgeEOL = false;
        }
        // check curr and next form an EOL edge
        if (corner->getNextCorner()->getType() == frCornerTypeEnum::CONVEX) {
          if (con->hasMinLength()) {
            // not EOL if minLength is not satsified
            if (gtl::length(*(corner->getPrevEdge())) < con->getMinLength() ||
                gtl::length(*(corner->getNextCorner()->getNextEdge())) < con->getMinLength()) {
              isNextEdgeEOL = false;
            } else {
              isNextEdgeEOL = true;
            }
          } else {
            isNextEdgeEOL = true;
          }
        } else {
          isNextEdgeEOL = false;
        }

        if (isPrevEdgeEOL && gtl::length(*(corner->getPrevEdge())) < con->getEolWidth()) {
          continue;
        }
        if (isNextEdgeEOL && gtl::length(*(corner->getNextEdge())) < con->getEolWidth()) {
          continue;
        }

        // start checking
        gtl::rectangle_data<frCoord> markerRect(corner->x(), corner->y(), corner->x(), corner->y());
        gtl::generalized_intersect(markerRect, *rect1);

        auto dx = gtl::delta(markerRect, gtl::HORIZONTAL);
        auto dy = gtl::delta(markerRect, gtl::VERTICAL);

        auto edgeX = reqSpcVal;
        auto edgeY = reqSpcVal;
        if (corner->getNextEdge()->getDir() == frDirEnum::N || corner->getNextEdge()->getDir() == frDirEnum::S) {
          edgeX = min(edgeX, int(gtl::length(*(corner->getPrevEdge()))));
          edgeY = min(edgeY, int(gtl::length(*(corner->getNextEdge()))));
        } else {
          edgeX = min(edgeX, int(gtl::length(*(corner->getNextEdge()))));
          edgeY = min(edgeY, int(gtl::length(*(corner->getPrevEdge()))));
        }
        // outside of keepout zone
        if (edgeX * dy + edgeY * dx >= edgeX * edgeY) {
          continue;
        }

        if (rect1->isFixed() && corner->isFixed()) {
          continue;
        }

        // this should only happen between samenet
        auto marker = make_unique<frMarker>();
        frBox box(gtl::xl(markerRect), gtl::yl(markerRect), gtl::xh(markerRect), gtl::yh(markerRect));
        marker->setBBox(box);
        marker->setLayerNum(layerNum);
        marker->setConstraint(con);
        marker->addSrc(net1->getOwner());
        marker->addVictim(net1->getOwner(), make_tuple(layerNum,
                                                       frBox(gtl::xl(*rect1), gtl::yl(*rect1), gtl::xh(*rect1), gtl::yh(*rect1)),
                                                       rect1->isFixed()));
        marker->addSrc(net2->getOwner());
        marker->addAggressor(net2->getOwner(), make_tuple(secondLayerNum, 
                                                          frBox(corner->x(), corner->y(), corner->x(), corner->y()),
                                                          corner->isFixed()));
        if (addMarker(std::move(marker))) {
          if (enableOutput) {
            double dbu = getDesign()->getTopBlock()->getDBUPerUU();
            cout <<"CutSpc@(" <<gtl::xl(markerRect) / dbu <<", " <<gtl::yl(markerRect) / dbu <<") ("
                              <<gtl::xh(markerRect) / dbu <<", " <<gtl::yh(markerRect) / dbu <<") "
                 <<getDesign()->getTech()->getLayer(layerNum)->getName() <<" ";
            auto owner = net1->getOwner();
            if (owner == nullptr) {
              cout <<"FLOATING";
            } else {
              if (owner->typeId() == frcNet) {
                cout <<static_cast<frNet*>(owner)->getName();
              } else if (owner->typeId() == frcInstTerm) {
                cout <<static_cast<frInstTerm*>(owner)->getInst()->getName() <<"/" 
                     <<static_cast<frInstTerm*>(owner)->getTerm()->getName();
              } else if (owner->typeId() == frcTerm) {
                cout <<"PIN/" <<static_cast<frTerm*>(owner)->getName();
              } else if (owner->typeId() == frcInstBlockage) {
                cout <<static_cast<frInstBlockage*>(owner)->getInst()->getName() <<"/OBS";
              } else if (owner->typeId() == frcBlockage) {
                cout <<"PIN/OBS";
              } else {
                cout <<"UNKNOWN";
              }
            }
            cout <<" ";
            owner = net2->getOwner();
            if (owner == nullptr) {
              cout <<"FLOATING";
            } else {
              if (owner->typeId() == frcNet) {
                cout <<static_cast<frNet*>(owner)->getName();
              } else if (owner->typeId() == frcInstTerm) {
                cout <<static_cast<frInstTerm*>(owner)->getInst()->getName() <<"/" 
                     <<static_cast<frInstTerm*>(owner)->getTerm()->getName();
              } else if (owner->typeId() == frcTerm) {
                cout <<"PIN/" <<static_cast<frTerm*>(owner)->getName();
              } else if (owner->typeId() == frcInstBlockage) {
                cout <<static_cast<frInstBlockage*>(owner)->getInst()->getName() <<"/OBS";
              } else if (owner->typeId() == frcBlockage) {
                cout <<"PIN/OBS";
              } else {
                cout <<"UNKNOWN";
              }
            }
            cout <<endl;
          }
        }
      }
    } else if (con->hasAboveWidth()) {
      ;
    } else if (con->isMaskOverlap()) {
      ;
    } else if (con->isWrongDirection()) {
      ;
    }
  }


}

// check short for every spacing rule except layer
void FlexGCWorker::Impl::checkCutSpacing_main(gcRect* ptr1, gcRect* ptr2, frCutSpacingConstraint* con) {
  // skip if same obj
  if (ptr1 == ptr2) {
    return;
  }
  // skip if con is not same net rule, but layer has same net rule and are same net
  // TODO: filter the rule upfront
  if (!con->hasSameNet() && ptr1->getNet() == ptr2->getNet()) {
    // same layer same net
    if (!(con->hasSecondLayer())) {
      if (getDesign()->getTech()->getLayer(ptr1->getLayerNum())->hasCutSpacing(true)) {
        return;
      }
    // diff layer same net
    } else {
      if (getDesign()->getTech()->getLayer(ptr1->getLayerNum())->hasInterLayerCutSpacing(con->getSecondLayerNum(), true)) {
        return;
      }
    }
  }

  gtl::rectangle_data<frCoord> markerRect(*ptr1);
  auto distX = gtl::euclidean_distance(markerRect, *ptr2, gtl::HORIZONTAL);
  auto distY = gtl::euclidean_distance(markerRect, *ptr2, gtl::VERTICAL);

  gtl::generalized_intersect(markerRect, *ptr2);
  auto prlX = gtl::delta(markerRect, gtl::HORIZONTAL);
  auto prlY = gtl::delta(markerRect, gtl::VERTICAL);

  if (distX) {
    prlX = -prlX;
  }
  if (distY) {
    prlY = -prlY;
  }
  
  if (ptr1->getLayerNum() == ptr2->getLayerNum()) {
    // CShort
    if (distX == 0 && distY == 0) {
      checkCutSpacing_short(ptr1, ptr2, markerRect);
    // same-layer CutSpc
    } else {
      checkCutSpacing_spc(ptr1, ptr2, markerRect, con, std::max(prlX, prlY));
    }
  } else {
    // diff-layer CutSpc
    checkCutSpacing_spc_diff_layer(ptr1, ptr2, markerRect, con);
  }
}

bool FlexGCWorker::Impl::checkCutSpacing_main_hasAdjCuts(gcRect* rect, frCutSpacingConstraint* con) {
  // no adj cut rule, must proceed checking
  if (!con->isAdjacentCuts()) {
    return true;
  }
  auto layerNum = rect->getLayerNum();
  auto layer = getDesign()->getTech()->getLayer(layerNum);

  // rect is obs larger than min. size cut, must check against cutWithin
  if (rect->getNet()->getOwner() && 
      (rect->getNet()->getOwner()->typeId() == frcInstBlockage ||
       rect->getNet()->getOwner()->typeId() == frcBlockage) &&
      rect->width() > int(layer->getWidth())) {
    return true;
  }
  
  auto cutWithinSquare = con->getCutWithin();
  box_t queryBox;
  myBloat(*rect, cutWithinSquare, queryBox);
  cutWithinSquare *= cutWithinSquare;
  auto &workerRegionQuery = getWorkerRegionQuery();
  vector<rq_box_value_t<gcRect*> > result;
  workerRegionQuery.queryMaxRectangle(queryBox, layerNum, result);
  int reqNumCut = con->getAdjacentCuts();
  int cnt = -1;
  gtl::point_data<frCoord> center1, center2;
  gtl::center(center1, *rect);
  // count adj cuts
  for (auto &[objBox, ptr]: result) {
    frCoord distSquare = 0;
    if (con->hasCenterToCenter()) {
      gtl::center(center2, *ptr);
      distSquare = gtl::distance_squared(center1, center2);
    } else {
      distSquare = gtl::square_euclidean_distance(*rect, *ptr);
    }
    if (distSquare >= cutWithinSquare) {
      continue;
    }
    // if target is a cut blockage shape larger than min. size, assume it is a blockage from MACRO
    if (ptr->getNet()->getOwner() && 
        (ptr->getNet()->getOwner()->typeId() == frcInstBlockage ||
         ptr->getNet()->getOwner()->typeId() == frcBlockage) &&
        ptr->width() > int(layer->getWidth())) {
      cnt += reqNumCut;
    } else {
      cnt++;
    }
  }
  if (cnt >= reqNumCut) {
    return true;
  } else {
    return false;
  }
}

void FlexGCWorker::Impl::checkLef58CutSpacing_main(gcRect* rect, frLef58CutSpacingConstraint* con, bool skipDiffNet) {
  // bool enableOutput = false;
  auto layerNum = rect->getLayerNum();
  auto maxSpcVal = checkLef58CutSpacing_getMaxSpcVal(con);
  box_t queryBox;
  myBloat(*rect, maxSpcVal, queryBox);

  auto &workerRegionQuery = getWorkerRegionQuery();
  vector<rq_box_value_t<gcRect*> > result;
  if (con->hasSecondLayer()) {
    workerRegionQuery.queryMaxRectangle(queryBox, con->getSecondLayerNum(), result);
  } else {
    workerRegionQuery.queryMaxRectangle(queryBox, layerNum, result);
  }

  for (auto &[objBox, ptr]: result) {
    if (skipDiffNet && rect->getNet() != ptr->getNet()) {
      continue;
    }
    checkLef58CutSpacing_main(rect, ptr, con);
  }
}

void FlexGCWorker::Impl::checkCutSpacing_main(gcRect* rect, frCutSpacingConstraint* con) {
  //bool enableOutput = true;
  bool enableOutput = false;
  auto layerNum = rect->getLayerNum();
  auto maxSpcVal = checkCutSpacing_getMaxSpcVal(con);
  box_t queryBox;
  myBloat(*rect, maxSpcVal, queryBox);
  if (enableOutput) {
    double dbu = getDesign()->getTopBlock()->getDBUPerUU();
    cout <<"checkCutSpacing maxRect ";
    if (rect->isFixed()) {
      cout <<"FIXED";
    } else {
      cout <<"ROUTE";
    }
    cout <<" (" <<gtl::xl(*rect) / dbu <<", " <<gtl::yl(*rect) / dbu <<") (" 
                <<gtl::xh(*rect) / dbu <<", " <<gtl::yh(*rect) / dbu <<") "
         <<getDesign()->getTech()->getLayer(layerNum)->getName() <<" ";
    cout <<"bloat maxSpcVal@" <<maxSpcVal / dbu <<" (" <<queryBox.min_corner().x() / dbu <<", " <<queryBox.min_corner().x() / dbu <<") (" 
                      <<queryBox.max_corner().x() / dbu <<", " <<queryBox.max_corner().x() / dbu <<") ";
    auto owner = rect->getNet()->getOwner();
    if (owner == nullptr) {
      cout <<" FLOATING";
    } else {
      if (owner->typeId() == frcNet) {
        cout <<static_cast<frNet*>(owner)->getName();
      } else if (owner->typeId() == frcInstTerm) {
        cout <<static_cast<frInstTerm*>(owner)->getInst()->getName() <<"/" 
             <<static_cast<frInstTerm*>(owner)->getTerm()->getName();
      } else if (owner->typeId() == frcTerm) {
        cout <<"PIN/" <<static_cast<frTerm*>(owner)->getName();
      } else if (owner->typeId() == frcInstBlockage) {
        cout <<static_cast<frInstBlockage*>(owner)->getInst()->getName() <<"/OBS";
      } else if (owner->typeId() == frcBlockage) {
        cout <<"PIN/OBS";
      } else {
        cout <<"UNKNOWN";
      }
    }
    cout <<endl;
  }

  // skip if adjcut not satisfied
  if (!checkCutSpacing_main_hasAdjCuts(rect, con)) {
    return;
  }

  auto &workerRegionQuery = getWorkerRegionQuery();
  vector<rq_box_value_t<gcRect*> > result;
  if (con->hasSecondLayer()) {
    workerRegionQuery.queryMaxRectangle(queryBox, con->getSecondLayerNum(), result);
  } else {
    workerRegionQuery.queryMaxRectangle(queryBox, layerNum, result);
  }
  // Short, metSpc, NSMetal here
  for (auto &[objBox, ptr]: result) {
    if (con->hasSecondLayer()) {
      if (rect->getNet() != ptr->getNet() || con->getSameNetConstraint() == nullptr) {
        checkCutSpacing_main(rect, ptr, con);
      } else {
        if (con->getSameNetConstraint()) {
          checkCutSpacing_main(rect, ptr, con->getSameNetConstraint());
        }
      }
    } else {
      checkCutSpacing_main(rect, ptr, con);
    }
  }
}

void FlexGCWorker::Impl::checkCutSpacing_main(gcRect* rect) {
  auto layerNum = rect->getLayerNum();

  // CShort
  // diff net same layer
  for (auto con: getDesign()->getTech()->getLayer(layerNum)->getCutSpacing(false)) {
    checkCutSpacing_main(rect, con);
  }
  // same net same layer
  for (auto con: getDesign()->getTech()->getLayer(layerNum)->getCutSpacing(true)) {
    checkCutSpacing_main(rect, con);
  }
  // diff net diff layer
  for (auto con: getDesign()->getTech()->getLayer(layerNum)->getInterLayerCutSpacingConstraint(false)) {
    if (con) {
      checkCutSpacing_main(rect, con);
    }
  }

  // LEF58_SPACING for cut layer
  bool skipDiffNet = false;
  // samenet rule
  for (auto con: getDesign()->getTech()->getLayer(layerNum)->getLef58CutSpacingConstraints(true)) {
    // skipSameNet if same-net rule exists
    skipDiffNet = true;
    checkLef58CutSpacing_main(rect, con, false);
  }
  // diffnet rule
  for (auto con: getDesign()->getTech()->getLayer(layerNum)->getLef58CutSpacingConstraints(false)) {
    checkLef58CutSpacing_main(rect, con, skipDiffNet);
  }
}

void FlexGCWorker::Impl::checkCutSpacing() {
  if (targetNet_) {
    // layer --> net --> polygon --> maxrect
    for (int i = std::max((frLayerNum)(getDesign()->getTech()->getBottomLayerNum()), minLayerNum_); 
         i <= std::min((frLayerNum)(getDesign()->getTech()->getTopLayerNum()), maxLayerNum_); i++) {
      auto currLayer = getDesign()->getTech()->getLayer(i);
      if (currLayer->getType() != frLayerTypeEnum::CUT) {
        continue;
      }
      for (auto &pin: targetNet_->getPins(i)) {
        for (auto &maxrect: pin->getMaxRectangles()) {
          checkCutSpacing_main(maxrect.get());
        }
      }
    }
  } else {
    // layer --> net --> polygon --> maxrect
    for (int i = std::max((frLayerNum)(getDesign()->getTech()->getBottomLayerNum()), minLayerNum_); 
         i <= std::min((frLayerNum)(getDesign()->getTech()->getTopLayerNum()), maxLayerNum_); i++) {
      auto currLayer = getDesign()->getTech()->getLayer(i);
      if (currLayer->getType() != frLayerTypeEnum::CUT) {
        continue;
      }
      for (auto &net: getNets()) {
        for (auto &pin: net->getPins(i)) {
          for (auto &maxrect: pin->getMaxRectangles()) {
            //cout <<"from " <<maxrect.get() <<endl;
            checkCutSpacing_main(maxrect.get());
          }
        }
      }
    }
  }
}

void FlexGCWorker::Impl::patchMetalShape() {
  pwires_.clear();
  clearMarkers();

  checkMetalShape();

  patchMetalShape_helper();

  clearMarkers();
}

// loop through violation and patch C5 enclosure minStep for GF14
void FlexGCWorker::Impl::patchMetalShape_helper() {
  vector<drConnFig*> results;
  for (auto &marker: markers_) {
    results.clear();
    if (marker->getConstraint()->typeId() != frConstraintTypeEnum::frcMinStepConstraint) {
      continue;
    }
    auto lNum = marker->getLayerNum();
    if (lNum != 10) {
      continue;
    }

    // bool isPatchable = false;
    // TODO: check whether there is empty space for patching
    // isPatchable = true;

    frBox markerBBox;
    frPoint origin;
    frPoint patchLL, patchUR;
    drNet* net = nullptr;
    auto &workerRegionQuery = getDRWorker()->getWorkerRegionQuery();
    marker->getBBox(markerBBox);
    frPoint markerCenter((markerBBox.left() + markerBBox.right()) / 2, (markerBBox.bottom() + markerBBox.top()) / 2);
    workerRegionQuery.query(markerBBox, lNum, results);
    for (auto &connFig: results) {
      if (connFig->typeId() != drcVia) {
        continue;
      }
      auto obj = static_cast<drVia*>(connFig);
      if (obj->getNet()->getFrNet() != *(marker->getSrcs().begin())) {
        continue;
      }
      if (obj->getViaDef()->getLayer1Num() == lNum) {
        obj->getOrigin(origin);
        if (origin.x() != markerCenter.x() && origin.y() != markerCenter.y()) {
          continue;
        }
        net = obj->getNet();
        break;
      }
    }

    if (!net) {
      continue;
    }

    // calculate patch location
    gtl::rectangle_data<frCoord> markerRect(markerBBox.left(), markerBBox.bottom(), markerBBox.right(), markerBBox.top());
    gtl::point_data<frCoord> boostOrigin(origin.x(), origin.y());
    gtl::encompass(markerRect, boostOrigin);
    patchLL.set(gtl::xl(markerRect) - origin.x(), gtl::yl(markerRect) - origin.y());
    patchUR.set(gtl::xh(markerRect) - origin.x(), gtl::yh(markerRect) - origin.y());

    auto patch = make_unique<drPatchWire>();
    patch->setLayerNum(lNum);
    patch->setOrigin(origin);
    frBox box(frBox(patchLL, patchUR));
    patch->setOffsetBox(box);
    patch->addToNet(net);

    if (!net) {
      logger_->warn(DRT, 64, "attempting to add patch with no drNet.");
    }

    pwires_.push_back(std::move(patch));
  }
}


int FlexGCWorker::Impl::main() {
  ProfileTask profile("GC:main");
  //printMarker = true;
  // minStep patching for GF14
  if (surgicalFixEnabled_ && getDRWorker() && DBPROCESSNODE == "GF14_13M_3Mx_2Cx_4Kx_2Hx_2Gx_LB") {
    patchMetalShape();
  }
  // incremental updates
  if (!modifiedDRNets_.empty() || !pwires_.empty()) {
    updateGCWorker();
  }
  // clear existing markers
  clearMarkers();
  // check LEF58CornerSpacing
  checkMetalCornerSpacing();
  // check Short, NSMet, MetSpc based on max rectangles
  checkMetalSpacing();
  // check MinWid, MinStp, RectOnly based on polygon
  checkMetalShape();
  // check eolSpc based on polygon
  checkMetalEndOfLine();
  // check CShort, cutSpc
  checkCutSpacing();
  return 0;
}
