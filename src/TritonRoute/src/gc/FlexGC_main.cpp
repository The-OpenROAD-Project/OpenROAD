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

#include <boost/geometry.hpp>
#include <iostream>

#include "frProfileTask.h"
#include "gc/FlexGC_impl.h"

using namespace std;
using namespace fr;
typedef bg::model::polygon<point_t> polygon_t;
typedef bg::model::multi_polygon<polygon_t> mpolygon_t;

inline bool isBlockage(frBlockObject* owner)
{
  return owner
         && (owner->typeId() == frcInstBlockage
             || owner->typeId() == frcBlockage);
}
void updateBlockageWidth(frBlockObject* owner, frCoord& width)
{
  if (isBlockage(owner)) {
    frBlockage* blkg;
    if (owner->typeId() == frcInstBlockage)
      blkg = static_cast<frInstBlockage*>(owner)->getBlockage();
    else
      blkg = static_cast<frBlockage*>(owner);
    if (blkg->getDesignRuleWidth() != -1) {
      width = blkg->getDesignRuleWidth();
    }
  }
}
bool FlexGCWorker::Impl::isCornerOverlap(gcCorner* corner, const frBox& box)
{
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
    default:;
  }
  return false;
}

bool FlexGCWorker::Impl::isCornerOverlap(
    gcCorner* corner,
    const gtl::rectangle_data<frCoord>& rect)
{
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
    default:;
  }
  return false;
}

void FlexGCWorker::Impl::myBloat(const gtl::rectangle_data<frCoord>& rect,
                                 frCoord val,
                                 box_t& box)
{
  bg::set<bg::min_corner, 0>(box, gtl::xl(rect) - val);
  bg::set<bg::min_corner, 1>(box, gtl::yl(rect) - val);
  bg::set<bg::max_corner, 0>(box, gtl::xh(rect) + val);
  bg::set<bg::max_corner, 1>(box, gtl::yh(rect) + val);
}

frCoord FlexGCWorker::Impl::checkMetalSpacing_getMaxSpcVal(frLayerNum layerNum,
                                                           bool checkNDRs)
{
  frCoord maxSpcVal = 0;
  auto currLayer = getTech()->getLayer(layerNum);
  if (currLayer->hasMinSpacing()) {
    auto con = currLayer->getMinSpacing();
    switch (con->typeId()) {
      case frConstraintTypeEnum::frcSpacingConstraint:
        maxSpcVal = static_cast<frSpacingConstraint*>(con)->getMinSpacing();
        break;
      case frConstraintTypeEnum::frcSpacingTablePrlConstraint:
        maxSpcVal = static_cast<frSpacingTablePrlConstraint*>(con)->findMax();
        break;
      case frConstraintTypeEnum::frcSpacingTableTwConstraint:
        maxSpcVal = static_cast<frSpacingTableTwConstraint*>(con)->findMax();
        break;
      default:
        logger_->warn(DRT, 41, "Warning: Unsupported metSpc rule.");
    }
    if (checkNDRs)
      return max(maxSpcVal,
                 getTech()->getMaxNondefaultSpacing(layerNum / 2 - 1));
  }
  return maxSpcVal;
}

void FlexGCWorker::Impl::checkMetalCornerSpacing_getMaxSpcVal(
    frLayerNum layerNum,
    frCoord& maxSpcValX,
    frCoord& maxSpcValY)
{
  maxSpcValX = 0;
  maxSpcValY = 0;
  auto currLayer = getTech()->getLayer(layerNum);
  auto& lef58CornerSpacingCons = currLayer->getLef58CornerSpacingConstraints();
  if (!lef58CornerSpacingCons.empty()) {
    for (auto& con : lef58CornerSpacingCons) {
      maxSpcValX = std::max(maxSpcValX, con->findMax(true));
      maxSpcValY = std::max(maxSpcValY, con->findMax(false));
    }
  }
  return;
}

bool FlexGCWorker::Impl::isOppositeDir(gcCorner* corner, gcSegment* seg)
{
  auto cornerDir = corner->getDir();
  auto segDir = seg->getDir();
  if ((cornerDir == frCornerDirEnum::NE
       && (segDir == frDirEnum::S || segDir == frDirEnum::E))
      || (cornerDir == frCornerDirEnum::SE
          && (segDir == frDirEnum::S || segDir == frDirEnum::W))
      || (cornerDir == frCornerDirEnum::SW
          && (segDir == frDirEnum::N || segDir == frDirEnum::W))
      || (cornerDir == frCornerDirEnum::NW
          && (segDir == frDirEnum::N || segDir == frDirEnum::E))) {
    return true;
  } else {
    return false;
  }
}

box_t FlexGCWorker::Impl::checkMetalCornerSpacing_getQueryBox(
    gcCorner* corner,
    frCoord& maxSpcValX,
    frCoord& maxSpcValY)
{
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

bool isPG(frBlockObject* obj)
{
  switch (obj->typeId()) {
    case frcNet: {
      auto type = static_cast<frNet*>(obj)->getType();
      return type == frNetEnum::frcPowerNet || type == frNetEnum::frcGroundNet;
    }
    case frcInstTerm: {
      auto type = static_cast<frInstTerm*>(obj)->getTerm()->getType();
      return type == frTermEnum::frcPowerTerm
             || type == frTermEnum::frcGroundTerm;
    }
    case frcTerm: {
      auto type = static_cast<frTerm*>(obj)->getType();
      return type == frTermEnum::frcPowerTerm
             || type == frTermEnum::frcGroundTerm;
    }
    default:
      return false;
  }
}

frCoord FlexGCWorker::Impl::checkMetalSpacing_prl_getReqSpcVal(
    gcRect* rect1,
    gcRect* rect2,
    frCoord prl /*, bool &hasRoute*/)
{
  auto layerNum = rect1->getLayerNum();
  frCoord reqSpcVal = 0;
  auto currLayer = getTech()->getLayer(layerNum);
  bool isObs = false;
  auto width1 = rect1->width();
  auto width2 = rect2->width();
  // override width and spacing
  if (isBlockage(rect1->getNet()->getOwner())) {
    isObs = true;
    if (USEMINSPACING_OBS) {
      width1 = currLayer->getWidth();
    }
    updateBlockageWidth(rect1->getNet()->getOwner(), width1);
  }
  if (isBlockage(rect2->getNet()->getOwner())) {
    isObs = true;
    if (USEMINSPACING_OBS) {
      width2 = currLayer->getWidth();
    }
    updateBlockageWidth(rect2->getNet()->getOwner(), width2);
  }
  // check if width is a result of route shape
  // if the width a shape is smaller if only using fixed shape, then it's route
  // shape -- wrong...
  frCoord minSpcVal = 0;
  if (currLayer->hasMinSpacing()) {
    auto con = currLayer->getMinSpacing();
    switch (con->typeId()) {
      case frConstraintTypeEnum::frcSpacingTablePrlConstraint:
        reqSpcVal = static_cast<frSpacingTablePrlConstraint*>(con)->find(
            std::max(width1, width2), prl);
        minSpcVal = static_cast<frSpacingTablePrlConstraint*>(con)->findMin();
        break;
      case frConstraintTypeEnum::frcSpacingTableTwConstraint:
        reqSpcVal = static_cast<frSpacingTableTwConstraint*>(con)->find(
            width1, width2, prl);
        minSpcVal = static_cast<frSpacingTableTwConstraint*>(con)->findMin();
        break;
      default:
        logger_->warn(DRT, 43, "Unsupported metSpc rule.");
    }
    if (con->typeId() == frConstraintTypeEnum::frcSpacingTablePrlConstraint
        || con->typeId() == frConstraintTypeEnum::frcSpacingTableTwConstraint) {
      // same-net override
      if (!isObs && rect1->getNet() == rect2->getNet()) {
        if (currLayer->hasSpacingSamenet()) {
          auto conSamenet = currLayer->getSpacingSamenet();
          if (!conSamenet->hasPGonly())
            reqSpcVal = std::max(conSamenet->getMinSpacing(), minSpcVal);
          else if (isPG(rect1->getNet()->getOwner()))
            reqSpcVal = std::max(conSamenet->getMinSpacing(), minSpcVal);
        }
      }
    }
  }
  return reqSpcVal;
}

// type: 0 -- check H edge only
// type: 1 -- check V edge only
// type: 2 -- check both
bool FlexGCWorker::Impl::checkMetalSpacing_prl_hasPolyEdge(
    gcRect* rect1,
    gcRect* rect2,
    const gtl::rectangle_data<frCoord>& markerRect,
    int type,
    frCoord prl)
{
  auto layerNum = rect1->getLayerNum();
  auto net1 = rect1->getNet();
  auto net2 = rect2->getNet();
  auto& workerRegionQuery = getWorkerRegionQuery();
  vector<pair<segment_t, gcSegment*>> result;
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
    for (auto& [seg, objPtr] : result) {
      if ((objPtr->getNet() != net1 && objPtr->getNet() != net2)) {
        continue;
      }
      if (objPtr->getDir() == frDirEnum::W
          && objPtr->low().y() == gtl::yl(markerRect)) {
        flagB = true;
      } else if (objPtr->getDir() == frDirEnum::E
                 && objPtr->low().y() == gtl::yh(markerRect)) {
        flagT = true;
      } else if (objPtr->getDir() == frDirEnum::N
                 && objPtr->low().x() == gtl::xl(markerRect)) {
        flagL = true;
      } else if (objPtr->getDir() == frDirEnum::S
                 && objPtr->low().x() == gtl::xh(markerRect)) {
        flagR = true;
      }
    }
    // type == 0 / 1 requires non-zero overlapping poly edge
  } else {
    for (auto& [seg, objPtr] : result) {
      if ((objPtr->getNet() != net1 && objPtr->getNet() != net2)) {
        continue;
      }
      // allow fake edge if inside markerRect has same dir edge
      if (objPtr->getDir() == frDirEnum::W
          && (objPtr->low().y() >= gtl::yl(markerRect)
              && objPtr->low().y() < gtl::yh(markerRect))
          && !(objPtr->low().x() <= gtl::xl(markerRect)
               || objPtr->high().x() >= gtl::xh(markerRect))) {
        flagB = true;
      } else if (objPtr->getDir() == frDirEnum::E
                 && (objPtr->low().y() > gtl::yl(markerRect)
                     && objPtr->low().y() <= gtl::yh(markerRect))
                 && !(objPtr->high().x() <= gtl::xl(markerRect)
                      || objPtr->low().x() >= gtl::xh(markerRect))) {
        flagT = true;
      } else if (objPtr->getDir() == frDirEnum::N
                 && (objPtr->low().x() >= gtl::xl(markerRect)
                     && objPtr->low().x() < gtl::xh(markerRect))
                 && !(objPtr->high().y() <= gtl::yl(markerRect)
                      || objPtr->low().y() >= gtl::yh(markerRect))) {
        flagL = true;
      } else if (objPtr->getDir() == frDirEnum::S
                 && (objPtr->low().x() > gtl::xl(markerRect)
                     && objPtr->low().x() <= gtl::xh(markerRect))
                 && !(objPtr->low().y() <= gtl::yl(markerRect)
                      || objPtr->high().y() >= gtl::yh(markerRect))) {
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
string rectToString(gcRect& r)
{
  string s = to_string(gtl::xl(r)) + " " + to_string(gtl::yl(r)) + " "
             + to_string(gtl::xh(r)) + " " + to_string(gtl::yh(r))
             + " tapered ? " + to_string(r.isTapered());
  return s;
}
void FlexGCWorker::Impl::checkMetalSpacing_prl(
    gcRect* rect1,
    gcRect* rect2,
    const gtl::rectangle_data<frCoord>& markerRect,
    frCoord prl,
    frCoord distX,
    frCoord distY,
    bool checkNDRs,
    bool checkPolyEdge)
{
  // no violation if fixed shapes
  if (rect1->isFixed() && rect2->isFixed()) {
    return;
  }
  auto layerNum = rect1->getLayerNum();
  auto net1 = rect1->getNet();
  auto net2 = rect2->getNet();

  auto reqSpcVal = checkMetalSpacing_prl_getReqSpcVal(rect1, rect2, prl);
  if (checkNDRs) {
    frCoord ndrSpc1 = 0, ndrSpc2 = 0;
    if (!rect1->isFixed() && net1->isNondefault() && !rect1->isTapered())
      ndrSpc1
          = net1->getFrNet()->getNondefaultRule()->getSpacing(layerNum / 2 - 1);
    if (!rect2->isFixed() && net2->isNondefault() && !rect2->isTapered())
      ndrSpc2
          = net2->getFrNet()->getNondefaultRule()->getSpacing(layerNum / 2 - 1);

    reqSpcVal = max(reqSpcVal, max(ndrSpc1, ndrSpc2));
  }

  // no violation if spacing satisfied
  if (distX * distX + distY * distY >= reqSpcVal * reqSpcVal) {
    return;
  }
  // no violation if no two true polygon edges (prl > 0 requires non-zero true
  // poly edge; prl <= 0 allows zero true poly edge)
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
  if (checkPolyEdge) {
    if (!checkMetalSpacing_prl_hasPolyEdge(
            rect1, rect2, markerRect, type, prl)) {
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
      tmpPoly &= *rect1;  // tmpPoly now is widthrect
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
      tmpPoly &= *rect2;  // tmpPoly now is widthrect
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
  } else {
    using namespace boost::polygon::operators;
    auto& netPoly = net1->getPolygons(
        layerNum, false);  // consider net1 since spc rect is always rect1
    gtl::polygon_90_set_data<frCoord> markerPoly;
    gtl::rectangle_data<frCoord> mRect(markerRect);
    // special treatment for  0 width marker since we cant create a polygon with
    // it
    if (gtl::xh(markerRect) - gtl::xl(markerRect) == 0
        || gtl::yh(markerRect) - gtl::yl(markerRect) == 0) {
      bool isX = gtl::xh(markerRect) - gtl::xl(markerRect) == 0;
      // bloat marker to be able to create a valid polygon
      if (isX) {
        gtl::xh(mRect, gtl::xh(mRect) + 1);
        gtl::xl(mRect, gtl::xl(mRect) - 1);
      } else {
        gtl::yh(mRect, gtl::yh(mRect) + 1);
        gtl::yl(mRect, gtl::yl(mRect) - 1);
      }
      markerPoly += mRect;
      markerPoly -= netPoly;
      if (markerPoly.size() == 0)
        return;
      // check if the edge related to the 0 width is within the net shape (this
      // is an indirect check)
      vector<gtl::rectangle_data<frCoord>> rects;
      markerPoly.get_rectangles(rects);
      if (rects.size() == 1) {
        if (isX) {
          if (gtl::xl(rects[0]) == gtl::xl(markerRect)
              || gtl::xh(rects[0]) == gtl::xl(markerRect))
            return;
        } else {
          if (gtl::yl(rects[0]) == gtl::yl(markerRect)
              || gtl::yh(rects[0]) == gtl::yl(markerRect))
            return;
        }
      }
    } else {
      markerPoly += mRect;
      markerPoly -= netPoly;
      if (markerPoly.size() == 0)
        return;
    }
  }
  auto marker = make_unique<frMarker>();
  frBox box(gtl::xl(markerRect),
            gtl::yl(markerRect),
            gtl::xh(markerRect),
            gtl::yh(markerRect));
  marker->setBBox(box);
  marker->setLayerNum(layerNum);
  marker->setConstraint(getTech()->getLayer(layerNum)->getMinSpacing());
  marker->addSrc(net1->getOwner());
  marker->addVictim(net1->getOwner(),
                    make_tuple(rect1->getLayerNum(),
                               frBox(gtl::xl(*rect1),
                                     gtl::yl(*rect1),
                                     gtl::xh(*rect1),
                                     gtl::yh(*rect1)),
                               rect1->isFixed()));
  marker->addSrc(net2->getOwner());
  marker->addAggressor(net2->getOwner(),
                       make_tuple(rect2->getLayerNum(),
                                  frBox(gtl::xl(*rect2),
                                        gtl::yl(*rect2),
                                        gtl::xh(*rect2),
                                        gtl::yh(*rect2)),
                                  rect2->isFixed()));
  addMarker(std::move(marker));
}

inline polygon_t rect2polygon(const gtl::rectangle_data<frCoord>& rect)
{
  polygon_t poly;
  bg::append(poly.outer(), point_t(gtl::xl(rect), gtl::yl(rect)));
  bg::append(poly.outer(), point_t(gtl::xl(rect), gtl::yh(rect)));
  bg::append(poly.outer(), point_t(gtl::xh(rect), gtl::yh(rect)));
  bg::append(poly.outer(), point_t(gtl::xh(rect), gtl::yl(rect)));
  bg::append(poly.outer(), point_t(gtl::xl(rect), gtl::yl(rect)));
  return poly;
}

inline gtl::polygon_90_set_data<frCoord> bg2gtl(const polygon_t& p)
{
  gtl::polygon_90_set_data<frCoord> set;
  gtl::polygon_90_data<frCoord> poly;
  std::vector<gtl::point_data<frCoord>> points;
  for (auto pt : p.outer()) {
    points.push_back(gtl::point_data<frCoord>(pt.x(), pt.y()));
  }
  poly.set(points.begin(), points.end());
  using namespace boost::polygon::operators;
  set += poly;
  return set;
}
void FlexGCWorker::Impl::checkMetalSpacing_short_obs(
    gcRect* rect1,
    gcRect* rect2,
    const gtl::rectangle_data<frCoord>& markerRect)
{
  if (rect1->isFixed() && rect2->isFixed())
    return;
  bool isRect1Obs = isBlockage(rect1->getNet()->getOwner());
  bool isRect2Obs = isBlockage(rect2->getNet()->getOwner());
  if (isRect1Obs && isRect2Obs) {
    return;
  }
  // always make obs to be rect2
  if (isRect1Obs) {
    std::swap(rect1, rect2);
  }
  // now rect is not obs, rect2 is obs
  auto layerNum = rect1->getLayerNum();
  auto net1 = rect1->getNet();
  // check if markerRect is covered by fixed shapes of net1
  mpolygon_t pins;
  auto& polys1 = net1->getPolygons(layerNum, true);
  vector<gtl::rectangle_data<frCoord>> rects;
  gtl::get_max_rectangles(rects, polys1);
  for (auto& rect : rects) {
    if (gtl::contains(rect, markerRect)) {
      return;
    }
    pins.push_back(rect2polygon(rect));
  }
  polygon_t markerPoly = rect2polygon(markerRect);
  std::vector<polygon_t> result;
  bg::difference(markerPoly, pins, result);
  for (auto poly : result) {
    std::list<gtl::rectangle_data<frCoord>> res;
    gtl::get_max_rectangles(res, bg2gtl(poly));
    for (auto rect : res) {
      gcRect* rect3 = new gcRect(*rect2);
      rect3->setRect(rect);
      gtl::rectangle_data<frCoord> newMarkerRect(markerRect);
      gtl::intersect(newMarkerRect, *rect3);
      checkMetalSpacing_short(rect1, rect3, newMarkerRect);
    }
  }
}

bool FlexGCWorker::Impl::checkMetalSpacing_short_skipFixed(
    gcRect* rect1,
    gcRect* rect2,
    const gtl::rectangle_data<frCoord>& markerRect)
{
  auto layerNum = rect1->getLayerNum();
  auto net1 = rect1->getNet();
  auto net2 = rect2->getNet();
  gtl::rectangle_data<frCoord> bloatMarkerRect(markerRect);
  if (gtl::delta(markerRect, gtl::HORIZONTAL) == 0) {
    gtl::bloat(bloatMarkerRect, gtl::HORIZONTAL, 1);
  }
  if (gtl::delta(markerRect, gtl::VERTICAL) == 0) {
    gtl::bloat(bloatMarkerRect, gtl::VERTICAL, 1);
  }
  using namespace boost::polygon::operators;
  auto& polys1 = net1->getPolygons(layerNum, false);
  auto intersection_polys1 = polys1 & bloatMarkerRect;
  auto& polys2 = net2->getPolygons(layerNum, false);
  auto intersection_polys2 = polys2 & bloatMarkerRect;
  if (gtl::empty(intersection_polys1) && gtl::empty(intersection_polys2)) {
    return true;
  }
  return false;
}

bool FlexGCWorker::Impl::checkMetalSpacing_short_skipSameNet(
    gcRect* rect1,
    gcRect* rect2,
    const gtl::rectangle_data<frCoord>& markerRect)
{
  auto layerNum = rect1->getLayerNum();
  auto net1 = rect1->getNet();
  auto net2 = rect2->getNet();
  if (net1 == net2) {
    // skip if good
    auto minWidth = getTech()->getLayer(layerNum)->getMinWidth();
    auto xLen = gtl::delta(markerRect, gtl::HORIZONTAL);
    auto yLen = gtl::delta(markerRect, gtl::VERTICAL);
    if (xLen * xLen + yLen * yLen >= minWidth * minWidth) {
      return true;
    }
    // skip if rect < minwidth
    if ((gtl::delta(*rect1, gtl::HORIZONTAL) < minWidth
         || gtl::delta(*rect1, gtl::VERTICAL) < minWidth)
        || (gtl::delta(*rect2, gtl::HORIZONTAL) < minWidth
            || gtl::delta(*rect2, gtl::VERTICAL) < minWidth)) {
      return true;
    }
    // query third object that can bridge rect1 and rect2 in bloated marker area
    gtl::point_data<frCoord> centerPt;
    gtl::center(centerPt, markerRect);
    gtl::rectangle_data<frCoord> bloatMarkerRect(
        centerPt.x(), centerPt.y(), centerPt.x(), centerPt.y());
    box_t queryBox;
    myBloat(bloatMarkerRect, minWidth, queryBox);

    auto& workerRegionQuery = getWorkerRegionQuery();
    vector<rq_box_value_t<gcRect*>> result;
    workerRegionQuery.queryMaxRectangle(queryBox, layerNum, result);
    // cout <<"3rd obj" <<endl;
    for (auto& [objBox, objPtr] : result) {
      if (objPtr == rect1 || objPtr == rect2) {
        continue;
      }
      if (objPtr->getNet() != net1) {
        continue;
      }
      if (!gtl::contains(*objPtr, markerRect)) {
        continue;
      }
      if (gtl::delta(*objPtr, gtl::HORIZONTAL) < minWidth
          || gtl::delta(*objPtr, gtl::VERTICAL) < minWidth) {
        continue;
      }
      // only check same net third object
      gtl::rectangle_data<frCoord> tmpRect1(*rect1);
      gtl::rectangle_data<frCoord> tmpRect2(*rect2);
      if (gtl::intersect(tmpRect1, *objPtr)
          && gtl::intersect(tmpRect2, *objPtr)) {
        auto xLen1 = gtl::delta(tmpRect1, gtl::HORIZONTAL);
        auto yLen1 = gtl::delta(tmpRect1, gtl::VERTICAL);
        auto xLen2 = gtl::delta(tmpRect2, gtl::HORIZONTAL);
        auto yLen2 = gtl::delta(tmpRect2, gtl::VERTICAL);
        if (xLen1 * xLen1 + yLen1 * yLen1 >= minWidth * minWidth
            && xLen2 * xLen2 + yLen2 * yLen2 >= minWidth * minWidth) {
          return true;
        }
      }
    }
  }
  return false;
}

void FlexGCWorker::Impl::checkMetalSpacing_short(
    gcRect* rect1,
    gcRect* rect2,
    const gtl::rectangle_data<frCoord>& markerRect)
{
  auto layerNum = rect1->getLayerNum();
  auto net1 = rect1->getNet();
  auto net2 = rect2->getNet();
  if (rect1->isFixed() && rect2->isFixed())
    return;

  // skip if marker area does not have route shape, must exclude touching
  if (checkMetalSpacing_short_skipFixed(rect1, rect2, markerRect))
    return;
  // skip same-net sufficient metal
  if (checkMetalSpacing_short_skipSameNet(rect1, rect2, markerRect))
    return;

  auto marker = make_unique<frMarker>();
  frBox box(gtl::xl(markerRect),
            gtl::yl(markerRect),
            gtl::xh(markerRect),
            gtl::yh(markerRect));
  marker->setBBox(box);
  marker->setLayerNum(layerNum);
  if (net1 == net2) {
    marker->setConstraint(
        getTech()->getLayer(layerNum)->getNonSufficientMetalConstraint());
  } else {
    marker->setConstraint(getTech()->getLayer(layerNum)->getShortConstraint());
  }
  marker->addSrc(net1->getOwner());
  marker->addVictim(net1->getOwner(),
                    make_tuple(rect1->getLayerNum(),
                               frBox(gtl::xl(*rect1),
                                     gtl::yl(*rect1),
                                     gtl::xh(*rect1),
                                     gtl::yh(*rect1)),
                               rect1->isFixed()));
  marker->addSrc(net2->getOwner());
  marker->addAggressor(net2->getOwner(),
                       make_tuple(rect2->getLayerNum(),
                                  frBox(gtl::xl(*rect2),
                                        gtl::yl(*rect2),
                                        gtl::xh(*rect2),
                                        gtl::yh(*rect2)),
                                  rect2->isFixed()));
  addMarker(std::move(marker));
}

void FlexGCWorker::Impl::checkMetalSpacing_main(gcRect* ptr1,
                                                gcRect* ptr2,
                                                bool checkNDRs,
                                                bool isSpcRect)
{
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
    if (isBlockage(ptr1->getNet()->getOwner())
        || isBlockage(ptr2->getNet()->getOwner()))
      checkMetalSpacing_short_obs(ptr1, ptr2, markerRect);
    else
      checkMetalSpacing_short(ptr1, ptr2, markerRect);
    // prl
  } else {
    checkMetalSpacing_prl(ptr1,
                          ptr2,
                          markerRect,
                          std::max(prlX, prlY),
                          distX,
                          distY,
                          checkNDRs,
                          !isSpcRect);
  }
}

void FlexGCWorker::Impl::checkMetalSpacing_main(gcRect* rect,
                                                bool checkNDRs,
                                                bool isSpcRect)
{
  auto layerNum = rect->getLayerNum();
  auto maxSpcVal = checkMetalSpacing_getMaxSpcVal(layerNum, checkNDRs);

  box_t queryBox;
  myBloat(*rect, maxSpcVal, queryBox);

  auto& workerRegionQuery = getWorkerRegionQuery();
  vector<rq_box_value_t<gcRect*>> result;
  workerRegionQuery.queryMaxRectangle(queryBox, layerNum, result);
  if (checkNDRs) {
    vector<rq_box_value_t<gcRect>> resultS;
    workerRegionQuery.querySpcRectangle(queryBox, layerNum, resultS);
    for (auto& [objBox, ptr] : resultS) {
      checkMetalSpacing_main(rect, &ptr, checkNDRs, isSpcRect);
    }
  }
  // Short, metSpc, NSMetal here
  for (auto& [objBox, ptr] : result) {
    checkMetalSpacing_main(rect, ptr, checkNDRs, isSpcRect);
  }
}

void FlexGCWorker::Impl::checkMetalSpacing()
{
  if (targetNet_) {
    // layer --> net --> polygon --> maxrect
    for (int i
         = std::max((frLayerNum)(getTech()->getBottomLayerNum()), minLayerNum_);
         i <= std::min((frLayerNum)(getTech()->getTopLayerNum()), maxLayerNum_);
         i++) {
      auto currLayer = getTech()->getLayer(i);
      if (currLayer->getType() != frLayerTypeEnum::ROUTING) {
        continue;
      }
      for (auto& pin : targetNet_->getPins(i)) {
        for (auto& maxrect : pin->getMaxRectangles()) {
          checkMetalSpacing_main(maxrect.get(),
                                 getDRWorker() || !AUTO_TAPER_NDR_NETS);
        }
      }
      for (auto& sr : targetNet_->getSpecialSpcRects())
        checkMetalSpacing_main(
            sr.get(), getDRWorker() || !AUTO_TAPER_NDR_NETS, true);
    }
  } else {
    // layer --> net --> polygon --> maxrect
    for (int i
         = std::max((frLayerNum)(getTech()->getBottomLayerNum()), minLayerNum_);
         i <= std::min((frLayerNum)(getTech()->getTopLayerNum()), maxLayerNum_);
         i++) {
      auto currLayer = getTech()->getLayer(i);
      if (currLayer->getType() != frLayerTypeEnum::ROUTING) {
        continue;
      }
      for (auto& net : getNets()) {
        for (auto& pin : net->getPins(i)) {
          for (auto& maxrect : pin->getMaxRectangles()) {
            // Short, NSMetal, metSpc
            checkMetalSpacing_main(maxrect.get(),
                                   getDRWorker() || !AUTO_TAPER_NDR_NETS);
          }
        }
        for (auto& sr : net->getSpecialSpcRects())
          checkMetalSpacing_main(
              sr.get(), getDRWorker() || !AUTO_TAPER_NDR_NETS, true);
      }
    }
  }
}

// (new) currently only support ISPD2019-related part
// check between the corner and the target rect
void FlexGCWorker::Impl::checkMetalCornerSpacing_main(
    gcCorner* corner,
    gcRect* rect,
    frLef58CornerSpacingConstraint* con)
{
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
  frCoord candX, candY;
  // ensure this is a real corner to corner case
  if (con->getCornerType() == frCornerTypeEnum::CONVEX) {
    if (cornerX >= (candX = gtl::xh(*rect))) {
      if (cornerY >= (candY = gtl::yh(*rect))) {
        if (corner->getDir() != frCornerDirEnum::SW)
          return;
      } else if (cornerY <= (candY = gtl::yl(*rect))) {
        if (corner->getDir() != frCornerDirEnum::NW)
          return;
      } else
        return;
    } else if (cornerX <= (candX = gtl::xl(*rect))) {
      if (cornerY >= (candY = gtl::yh(*rect))) {
        if (corner->getDir() != frCornerDirEnum::SE)
          return;
      } else if (cornerY <= (candY = gtl::yl(*rect))) {
        if (corner->getDir() != frCornerDirEnum::NE)
          return;
      } else
        return;
    } else
      return;
    if (rect->getNet()
        && !rect->getNet()->hasPolyCornerAt(candX, candY, rect->getLayerNum()))
      return;
  }
  // skip for EXCEPTEOL eolWidth
  if (con->hasExceptEol()) {
    if (corner->getType() == frCornerTypeEnum::CONVEX) {
      if (corner->getNextCorner()->getType() == frCornerTypeEnum::CONVEX
          && gtl::length(*(corner->getNextEdge())) < con->getEolWidth()) {
        return;
      }
      if (corner->getPrevCorner()->getType() == frCornerTypeEnum::CONVEX
          && gtl::length(*(corner->getPrevEdge())) < con->getEolWidth()) {
        return;
      }
    }
  }

  // query and iterate only the rects that have the corner as its corner
  auto layerNum = corner->getNextEdge()->getLayerNum();
  auto net = corner->getNextEdge()->getNet();
  auto& workerRegionQuery = getWorkerRegionQuery();
  vector<rq_box_value_t<gcRect*>> result;
  box_t queryBox(point_t(cornerX, cornerY), point_t(cornerX, cornerY));
  workerRegionQuery.queryMaxRectangle(queryBox, layerNum, result);
  for (auto& [objBox, objPtr] : result) {
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
        tmpPoly &= *objPtr;  // tmpPoly now is widthrect
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
        tmpPoly &= *rect;  // tmpPoly now is widthrect
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
      frBox box(gtl::xl(markerRect),
                gtl::yl(markerRect),
                gtl::xh(markerRect),
                gtl::yh(markerRect));
      marker->setBBox(box);
      marker->setLayerNum(layerNum);
      marker->setConstraint(con);
      marker->addSrc(net->getOwner());
      marker->addVictim(
          net->getOwner(),
          make_tuple(layerNum,
                     frBox(corner->x(), corner->y(), corner->x(), corner->y()),
                     corner->isFixed()));
      marker->addSrc(rect->getNet()->getOwner());
      marker->addAggressor(rect->getNet()->getOwner(),
                           make_tuple(rect->getLayerNum(),
                                      frBox(gtl::xl(*rect),
                                            gtl::yl(*rect),
                                            gtl::xh(*rect),
                                            gtl::yh(*rect)),
                                      rect->isFixed()));
      addMarker(std::move(marker));
      return;
    } else {
      // TODO: implement others if necessary
    }
  }
}

// currently only support ISPD19-related part
void FlexGCWorker::Impl::checkMetalCornerSpacing_main(
    gcCorner* corner,
    gcSegment* seg,
    frLef58CornerSpacingConstraint* con)
{
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
      if (corner->getNextCorner()->getType() == frCornerTypeEnum::CONVEX
          && gtl::length(*(corner->getNextEdge())) < con->getEolWidth()) {
        return;
      }
      if (corner->getPrevCorner()->getType() == frCornerTypeEnum::CONVEX
          && gtl::length(*(corner->getPrevEdge())) < con->getEolWidth()) {
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
  gtl::rectangle_data<frCoord> segRect(
      seg->low().x(), seg->low().y(), seg->high().x(), seg->high().y());
  gtl::generalized_intersect(markerRect, segRect);
  frCoord maxXY = gtl::delta(markerRect, gtl::guess_orientation(markerRect));

  // query and iterate overlapping rect of the same net
  auto layerNum = corner->getNextEdge()->getLayerNum();
  auto net = corner->getNextEdge()->getNet();
  auto segNet = seg->getNet();
  auto& workerRegionQuery = getWorkerRegionQuery();
  vector<rq_box_value_t<gcRect*>> result;
  box_t queryBox(point_t(cornerX, cornerY), point_t(cornerX, cornerY));
  workerRegionQuery.queryMaxRectangle(queryBox, layerNum, result);
  for (auto& [objBox, objPtr] : result) {
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
      frBox box(gtl::xl(markerRect),
                gtl::yl(markerRect),
                gtl::xh(markerRect),
                gtl::yh(markerRect));
      marker->setBBox(box);
      marker->setLayerNum(layerNum);
      marker->setConstraint(con);
      marker->addSrc(net->getOwner());
      marker->addVictim(
          net->getOwner(),
          make_tuple(layerNum,
                     frBox(corner->x(), corner->y(), corner->x(), corner->y()),
                     corner->isFixed()));
      marker->addSrc(segNet->getOwner());
      frCoord llx = min(seg->getLowCorner()->x(), seg->getHighCorner()->x());
      frCoord lly = min(seg->getLowCorner()->y(), seg->getHighCorner()->y());
      frCoord urx = max(seg->getLowCorner()->x(), seg->getHighCorner()->x());
      frCoord ury = max(seg->getLowCorner()->y(), seg->getHighCorner()->y());
      marker->addAggressor(
          segNet->getOwner(),
          make_tuple(
              seg->getLayerNum(), frBox(llx, lly, urx, ury), seg->isFixed()));
      addMarker(std::move(marker));
    } else {
      // to be implemented
    }
  }
}

void FlexGCWorker::Impl::checkMetalCornerSpacing_main(gcCorner* corner)
{
  auto layerNum = corner->getPrevEdge()->getLayerNum();
  frCoord maxSpcValX, maxSpcValY;
  checkMetalCornerSpacing_getMaxSpcVal(layerNum, maxSpcValX, maxSpcValY);
  box_t queryBox
      = checkMetalCornerSpacing_getQueryBox(corner, maxSpcValX, maxSpcValY);

  auto& workerRegionQuery = getWorkerRegionQuery();
  vector<rq_box_value_t<gcRect*>> result;
  workerRegionQuery.queryMaxRectangle(queryBox, layerNum, result);
  // LEF58CornerSpacing
  auto& cons
      = getTech()->getLayer(layerNum)->getLef58CornerSpacingConstraints();
  for (auto& [objBox, ptr] : result) {
    for (auto& con : cons) {
      checkMetalCornerSpacing_main(corner, ptr, con);
    }
  }
}

void FlexGCWorker::Impl::checkMetalCornerSpacing()
{
  if (targetNet_) {
    // layer --> net --> polygon --> corner
    for (int i
         = std::max((frLayerNum)(getTech()->getBottomLayerNum()), minLayerNum_);
         i <= std::min((frLayerNum)(getTech()->getTopLayerNum()), maxLayerNum_);
         i++) {
      auto currLayer = getTech()->getLayer(i);
      if (currLayer->getType() != frLayerTypeEnum::ROUTING
          || !currLayer->hasLef58CornerSpacingConstraint()) {
        continue;
      }
      for (auto& pin : targetNet_->getPins(i)) {
        for (auto& corners : pin->getPolygonCorners()) {
          for (auto& corner : corners) {
            // LEF58 corner spacing
            checkMetalCornerSpacing_main(corner.get());
          }
        }
      }
    }
  } else {
    // layer --> net --> polygon --> corner
    for (int i
         = std::max((frLayerNum)(getTech()->getBottomLayerNum()), minLayerNum_);
         i <= std::min((frLayerNum)(getTech()->getTopLayerNum()), maxLayerNum_);
         i++) {
      auto currLayer = getTech()->getLayer(i);
      if (currLayer->getType() != frLayerTypeEnum::ROUTING
          || !currLayer->hasLef58CornerSpacingConstraint()) {
        continue;
      }
      for (auto& net : getNets()) {
        for (auto& pin : net->getPins(i)) {
          for (auto& corners : pin->getPolygonCorners()) {
            for (auto& corner : corners) {
              // LEF58 corner spacing
              checkMetalCornerSpacing_main(corner.get());
            }
          }
        }
      }
    }
  }
}

void FlexGCWorker::Impl::checkMetalShape_minWidth(
    const gtl::rectangle_data<frCoord>& rect,
    frLayerNum layerNum,
    gcNet* net,
    bool isH)
{
  // skip enough width
  auto minWidth = getTech()->getLayer(layerNum)->getMinWidth();
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
    auto& fixedPolys = net->getPolygons(layerNum, true);
    auto intersection_fixedPolys = fixedPolys & rect;
    if (gtl::area(intersection_fixedPolys) == gtl::area(rect)) {
      return;
    }
  }

  auto marker = make_unique<frMarker>();
  frBox box(gtl::xl(rect), gtl::yl(rect), gtl::xh(rect), gtl::yh(rect));
  marker->setBBox(box);
  marker->setLayerNum(layerNum);
  marker->setConstraint(getTech()->getLayer(layerNum)->getMinWidthConstraint());
  marker->addSrc(net->getOwner());
  marker->addVictim(net->getOwner(), make_tuple(layerNum, box, false));
  marker->addAggressor(net->getOwner(), make_tuple(layerNum, box, false));
  addMarker(std::move(marker));
}

void FlexGCWorker::Impl::checkMetalShape_minStep_helper(
    const frBox& markerBox,
    frLayerNum layerNum,
    gcNet* net,
    frMinStepConstraint* con,
    bool hasInsideCorner,
    bool hasOutsideCorner,
    bool hasStep,
    int currEdges,
    frCoord currLength,
    bool hasRoute)
{
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
    switch (con->getMinstepType()) {
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
      default:;
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
  addMarker(std::move(marker));
}

void FlexGCWorker::Impl::checkMetalShape_minArea(gcPin* pin)
{
  if (ignoreMinArea_) {
    return;
  }

  auto poly = pin->getPolygon();
  auto layerNum = poly->getLayerNum();
  auto net = poly->getNet();

  auto con = getTech()->getLayer(layerNum)->getAreaConstraint();

  if (!con) {
    return;
  }

  auto reqArea = con->getMinArea();
  auto actArea = gtl::area(*poly);

  if (actArea >= reqArea) {
    return;
  }

  bool hasNonFixedEdge = false;
  for (auto& edges : pin->getPolygonEdges()) {
    for (auto& edge : edges) {
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
  vector<gtl::rectangle_data<frCoord>> rects;
  gtl::get_max_rectangles(rects, tmpPolys);

  int maxArea = 0;
  gtl::rectangle_data<frCoord> maxRect;
  for (auto& rect : rects) {
    if (gtl::area(rect) > maxArea) {
      maxRect = rect;
      maxArea = gtl::area(rect);
    }
  }

  auto marker = make_unique<frMarker>();
  frBox markerBox(
      gtl::xl(maxRect), gtl::yl(maxRect), gtl::xh(maxRect), gtl::yh(maxRect));
  marker->setBBox(markerBox);
  marker->setLayerNum(layerNum);
  marker->setConstraint(con);
  marker->addSrc(net->getOwner());
  marker->addVictim(net->getOwner(), make_tuple(layerNum, markerBox, false));
  marker->addAggressor(net->getOwner(), make_tuple(layerNum, markerBox, false));

  addMarker(std::move(marker));
}

void FlexGCWorker::Impl::checkMetalShape_lef58MinStep_noBetweenEol(
    gcPin* pin,
    frLef58MinStepConstraint* con)
{
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
  for (auto& edges : pin->getPolygonEdges()) {
    // get the first edge that is >= minstep length
    for (auto& e : edges) {
      if (gtl::length(*e) < minStepLength) {
        startEdges.push_back(e.get());
      }
    }
  }

  for (auto startEdge : startEdges) {
    // skip if next next edge is not less than minStepLength
    if (gtl::length(*(startEdge->getNextEdge()->getNextEdge()))
        >= minStepLength) {
      continue;
    }
    // skip if next edge is not EOL edge
    auto nextEdge = startEdge->getNextEdge();
    if (nextEdge->getLowCorner()->getType() != frCornerTypeEnum::CONVEX
        || nextEdge->getHighCorner()->getType() != frCornerTypeEnum::CONVEX) {
      continue;
    }
    if (gtl::length(*nextEdge) >= eolWidth) {
      continue;
    }

    // skip if all edges are fixed
    if (startEdge->isFixed() && startEdge->getNextEdge()->isFixed()
        && startEdge->getNextEdge()->getNextEdge()->isFixed()) {
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
    addMarker(std::move(marker));
  }
}

// currently only support nobetweeneol
void FlexGCWorker::Impl::checkMetalShape_lef58MinStep(gcPin* pin)
{
  auto poly = pin->getPolygon();
  auto layerNum = poly->getLayerNum();
  // auto net = poly->getNet();

  for (auto con : getTech()->getLayer(layerNum)->getLef58MinStepConstraints()) {
    if (!con->hasEolWidth()) {
      continue;
    }
    checkMetalShape_lef58MinStep_noBetweenEol(pin, con);
  }
}

void FlexGCWorker::Impl::checkMetalShape_minStep(gcPin* pin)
{
  auto poly = pin->getPolygon();
  auto layerNum = poly->getLayerNum();
  auto net = poly->getNet();

  auto con = getTech()->getLayer(layerNum)->getMinStepConstraint();

  if (!con) {
    return;
  }

  gcSegment* be = nullptr;
  gcSegment* firste = nullptr;  // first edge to check
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
  for (auto& edges : pin->getPolygonEdges()) {
    // get the first edge that is >= minstep length
    firste = nullptr;
    for (auto& e : edges) {
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
        checkMetalShape_minStep_helper(markerBox,
                                       layerNum,
                                       net,
                                       con,
                                       hasInsideCorner,
                                       hasOutsideCorner,
                                       hasStep,
                                       currEdges,
                                       currLength,
                                       hasRoute);
        be = edge;  // new begin edge
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

void FlexGCWorker::Impl::checkMetalShape_rectOnly(gcPin* pin)
{
  auto poly = pin->getPolygon();
  auto layerNum = poly->getLayerNum();
  auto layerMinWidth = getTech()->getLayer(layerNum)->getMinWidth();
  auto net = poly->getNet();

  auto con = getTech()->getLayer(layerNum)->getLef58RectOnlyConstraint();

  if (!con) {
    return;
  }

  // not rectangle, potential violation
  vector<gtl::rectangle_data<frCoord>> rects;
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
  vector<gtl::point_data<frCoord>> concaveCorners;
  // get concave corners of the polygon
  for (auto& edges : pin->getPolygonEdges()) {
    for (auto& edge : edges) {
      auto currEdge = edge.get();
      auto nextEdge = currEdge->getNextEdge();
      if (orientation(*currEdge, *nextEdge) == -1) {
        concaveCorners.push_back(boost::polygon::high(*currEdge));
      }
    }
  }
  // draw rect from concave corners and test intersection
  auto& fixedPolys = net->getPolygons(layerNum, true);
  for (auto& corner : concaveCorners) {
    gtl::rectangle_data<frCoord> rect(gtl::x(corner) - layerMinWidth,
                                      gtl::y(corner) - layerMinWidth,
                                      gtl::x(corner) + layerMinWidth,
                                      gtl::y(corner) + layerMinWidth);
    {
      using namespace boost::polygon::operators;
      auto intersectionPolySet = polySet & rect;
      auto intersectionFixedPolySet = intersectionPolySet & fixedPolys;
      if (gtl::area(intersectionFixedPolySet)
          == gtl::area(intersectionPolySet)) {
        continue;
      }

      vector<gtl::rectangle_data<frCoord>> maxRects;
      gtl::get_max_rectangles(maxRects, intersectionPolySet);
      for (auto& markerRect : maxRects) {
        auto marker = make_unique<frMarker>();
        frBox box(gtl::xl(markerRect),
                  gtl::yl(markerRect),
                  gtl::xh(markerRect),
                  gtl::yh(markerRect));
        marker->setBBox(box);
        marker->setLayerNum(layerNum);
        marker->setConstraint(con);
        marker->addSrc(net->getOwner());
        marker->addVictim(net->getOwner(), make_tuple(layerNum, box, false));
        marker->addAggressor(net->getOwner(), make_tuple(layerNum, box, false));
        addMarker(std::move(marker));
      }
    }
  }
}

void FlexGCWorker::Impl::checkMetalShape_offGrid(gcPin* pin)
{
  auto net = pin->getNet();
  // Needs to be signed to make modulo work correctly with
  // negative coordinates
  int mGrid = getTech()->getManufacturingGrid();
  for (auto& rect : pin->getMaxRectangles()) {
    auto maxRect = rect.get();
    auto layerNum = maxRect->getLayerNum();
    // off grid maxRect
    if (gtl::xl(*maxRect) % mGrid || gtl::xh(*maxRect) % mGrid
        || gtl::yl(*maxRect) % mGrid || gtl::yh(*maxRect) % mGrid) {
      // continue if the marker area does not have route shape
      auto& polys = net->getPolygons(layerNum, false);
      gtl::rectangle_data<frCoord> markerRect(*maxRect);
      using namespace boost::polygon::operators;
      auto intersection_polys = polys & markerRect;
      if (gtl::empty(intersection_polys)) {
        continue;
      }
      auto marker = make_unique<frMarker>();
      frBox box(gtl::xl(markerRect),
                gtl::yl(markerRect),
                gtl::xh(markerRect),
                gtl::yh(markerRect));
      marker->setBBox(box);
      marker->setLayerNum(layerNum);
      marker->setConstraint(
          getTech()->getLayer(layerNum)->getOffGridConstraint());
      marker->addSrc(net->getOwner());
      marker->addVictim(net->getOwner(), make_tuple(layerNum, box, false));
      marker->addAggressor(net->getOwner(), make_tuple(layerNum, box, false));
      addMarker(std::move(marker));
    }
  }
}

void FlexGCWorker::Impl::checkMetalShape_minEnclosedArea(gcPin* pin)
{
  auto net = pin->getNet();
  auto poly = pin->getPolygon();
  auto layerNum = poly->getLayerNum();
  if (getTech()->getLayer(layerNum)->hasMinEnclosedArea()) {
    for (auto holeIt = poly->begin_holes(); holeIt != poly->end_holes();
         holeIt++) {
      auto& hole_poly = *holeIt;
      for (auto con :
           getTech()->getLayer(layerNum)->getMinEnclosedAreaConstraints()) {
        auto reqArea = con->getArea();
        if (gtl::area(hole_poly) < reqArea) {
          auto& polys = net->getPolygons(layerNum, false);
          using namespace boost::polygon::operators;
          auto intersection_polys = polys & (*poly);
          if (gtl::empty(intersection_polys)) {
            continue;
          }
          // create marker
          gtl::rectangle_data<frCoord> markerRect;
          gtl::extents(markerRect, hole_poly);

          auto marker = make_unique<frMarker>();
          frBox box(gtl::xl(markerRect),
                    gtl::yl(markerRect),
                    gtl::xh(markerRect),
                    gtl::yh(markerRect));
          marker->setBBox(box);
          marker->setLayerNum(layerNum);
          marker->setConstraint(con);
          marker->addSrc(net->getOwner());
          marker->addVictim(net->getOwner(), make_tuple(layerNum, box, false));
          marker->addAggressor(net->getOwner(),
                               make_tuple(layerNum, box, false));
          addMarker(std::move(marker));
        }
      }
    }
  }
}

void FlexGCWorker::Impl::checkMetalShape_main(gcPin* pin)
{
  auto poly = pin->getPolygon();
  auto layerNum = poly->getLayerNum();
  auto net = poly->getNet();

  // min width
  vector<gtl::rectangle_data<frCoord>> rects;
  gtl::polygon_90_set_data<frCoord> polySet;
  {
    using namespace boost::polygon::operators;
    polySet += *poly;
  }
  polySet.get_rectangles(rects, gtl::HORIZONTAL);
  for (auto& rect : rects) {
    checkMetalShape_minWidth(rect, layerNum, net, true);
  }
  rects.clear();
  polySet.get_rectangles(rects, gtl::VERTICAL);
  for (auto& rect : rects) {
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

void FlexGCWorker::Impl::checkMetalShape()
{
  if (targetNet_) {
    // layer --> net --> polygon
    for (int i
         = std::max((frLayerNum)(getTech()->getBottomLayerNum()), minLayerNum_);
         i <= std::min((frLayerNum)(getTech()->getTopLayerNum()), maxLayerNum_);
         i++) {
      auto currLayer = getTech()->getLayer(i);
      if (currLayer->getType() != frLayerTypeEnum::ROUTING) {
        continue;
      }
      for (auto& pin : targetNet_->getPins(i)) {
        checkMetalShape_main(pin.get());
      }
    }
  } else {
    // layer --> net --> polygon
    for (int i
         = std::max((frLayerNum)(getTech()->getBottomLayerNum()), minLayerNum_);
         i <= std::min((frLayerNum)(getTech()->getTopLayerNum()), maxLayerNum_);
         i++) {
      auto currLayer = getTech()->getLayer(i);
      if (currLayer->getType() != frLayerTypeEnum::ROUTING) {
        continue;
      }
      for (auto& net : getNets()) {
        for (auto& pin : net->getPins(i)) {
          checkMetalShape_main(pin.get());
        }
      }
    }
  }
}

frCoord FlexGCWorker::Impl::checkCutSpacing_getMaxSpcVal(
    frCutSpacingConstraint* con)
{
  frCoord maxSpcVal = 0;
  if (con) {
    maxSpcVal = con->getCutSpacing();
    if (con->isAdjacentCuts()) {
      maxSpcVal = std::max(maxSpcVal, con->getCutWithin());
    }
  }
  return maxSpcVal;
}

frCoord FlexGCWorker::Impl::checkLef58CutSpacing_getMaxSpcVal(
    frLef58CutSpacingConstraint* con)
{
  frCoord maxSpcVal = 0;
  if (con) {
    maxSpcVal = con->getCutSpacing();
    if (con->hasAdjacentCuts()) {
      maxSpcVal = std::max(maxSpcVal, con->getCutWithin());
    }
  }
  return maxSpcVal;
}

frCoord FlexGCWorker::Impl::checkCutSpacing_spc_getReqSpcVal(
    gcRect* ptr1,
    gcRect* ptr2,
    frCutSpacingConstraint* con)
{
  frCoord maxSpcVal = 0;
  if (con) {
    maxSpcVal = con->getCutSpacing();
    if (con->isAdjacentCuts()) {
      auto owner = ptr1->getNet()->getOwner();
      auto ptr1LayerNum = ptr1->getLayerNum();
      auto ptr1Layer = getTech()->getLayer(ptr1LayerNum);
      if (isBlockage(owner)) {
        frCoord width1 = ptr1->width();
        updateBlockageWidth(owner, width1);
        if (width1 > int(ptr1Layer->getWidth()))
          maxSpcVal = con->getCutWithin();
      }
      owner = ptr2->getNet()->getOwner();
      auto ptr2LayerNum = ptr2->getLayerNum();
      auto ptr2Layer = getTech()->getLayer(ptr2LayerNum);
      if (isBlockage(owner)) {
        frCoord width2 = ptr2->width();
        updateBlockageWidth(owner, width2);
        if (width2 > int(ptr2Layer->getWidth()))
          maxSpcVal = con->getCutWithin();
      }
    }
  }
  return maxSpcVal;
}

void FlexGCWorker::Impl::checkCutSpacing_short(
    gcRect* rect1,
    gcRect* rect2,
    const gtl::rectangle_data<frCoord>& markerRect)
{
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
  frBox box(gtl::xl(markerRect),
            gtl::yl(markerRect),
            gtl::xh(markerRect),
            gtl::yh(markerRect));
  marker->setBBox(box);
  marker->setLayerNum(layerNum);
  marker->setConstraint(getTech()->getLayer(layerNum)->getShortConstraint());
  marker->addSrc(net1->getOwner());
  marker->addVictim(net1->getOwner(),
                    make_tuple(rect1->getLayerNum(),
                               frBox(gtl::xl(*rect1),
                                     gtl::yl(*rect1),
                                     gtl::xh(*rect1),
                                     gtl::yh(*rect1)),
                               rect1->isFixed()));
  marker->addSrc(net2->getOwner());
  marker->addAggressor(net2->getOwner(),
                       make_tuple(rect2->getLayerNum(),
                                  frBox(gtl::xl(*rect2),
                                        gtl::yl(*rect2),
                                        gtl::xh(*rect2),
                                        gtl::yh(*rect2)),
                                  rect2->isFixed()));

  addMarker(std::move(marker));
}

void FlexGCWorker::Impl::checkCutSpacing_spc(
    gcRect* rect1,
    gcRect* rect2,
    const gtl::rectangle_data<frCoord>& markerRect,
    frCutSpacingConstraint* con,
    frCoord prl)
{
  auto layerNum = rect1->getLayerNum();
  auto net1 = rect1->getNet();
  auto net2 = rect2->getNet();
  // skip if adjcut && except samepgnet, note that adj cut handled given only
  // rect1 outside this function
  if (con->isAdjacentCuts() && con->hasExceptSamePGNet() && net1 == net2
      && net1->getOwner()) {
    auto owner = net1->getOwner();
    if (owner->typeId() == frcNet) {
      if (static_cast<frNet*>(owner)->getType() == frNetEnum::frcPowerNet
          || static_cast<frNet*>(owner)->getType() == frNetEnum::frcGroundNet) {
        return;
      }
    } else if (owner->typeId() == frcTerm) {
      if (static_cast<frTerm*>(owner)->getType() == frTermEnum::frcPowerTerm
          || static_cast<frTerm*>(owner)->getType()
                 == frTermEnum::frcGroundTerm) {
        return;
      }
    } else if (owner->typeId() == frcInstTerm) {
      if (static_cast<frInstTerm*>(owner)->getTerm()->getType()
              == frTermEnum::frcPowerTerm
          || static_cast<frInstTerm*>(owner)->getTerm()->getType()
                 == frTermEnum::frcGroundTerm) {
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
      auto& workerRegionQuery = getWorkerRegionQuery();
      vector<rq_box_value_t<gcRect*>> result;
      auto secondLayerNum = rect1->getLayerNum() - 1;
      if (secondLayerNum >= getTech()->getBottomLayerNum()
          && secondLayerNum <= getTech()->getTopLayerNum()) {
        workerRegionQuery.queryMaxRectangle(queryBox, secondLayerNum, result);
      }
      secondLayerNum = rect1->getLayerNum() + 1;
      if (secondLayerNum >= getTech()->getBottomLayerNum()
          && secondLayerNum <= getTech()->getTopLayerNum()) {
        workerRegionQuery.queryMaxRectangle(queryBox, secondLayerNum, result);
      }
      for (auto& [objBox, objPtr] : result) {
        if ((objPtr->getNet() == net1 || objPtr->getNet() == net2)
            && objBox.contains(queryBox)) {
          return;
        }
      }
    }
  }
  // skip if not reaching area
  if (con->isArea() && gtl::area(*rect1) < con->getCutArea()
      && gtl::area(*rect2) < con->getCutArea()) {
    return;
  }

  // no violation if spacing satisfied
  frSquaredDistance reqSpcValSquare
      = checkCutSpacing_spc_getReqSpcVal(rect1, rect2, con);
  reqSpcValSquare *= reqSpcValSquare;

  gtl::point_data<frCoord> center1, center2;
  gtl::center(center1, *rect1);
  gtl::center(center2, *rect2);
  frSquaredDistance distSquare = 0;
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
  frBox box(gtl::xl(markerRect),
            gtl::yl(markerRect),
            gtl::xh(markerRect),
            gtl::yh(markerRect));
  marker->setBBox(box);
  marker->setLayerNum(layerNum);
  marker->setConstraint(con);
  marker->addSrc(net1->getOwner());
  marker->addVictim(net1->getOwner(),
                    make_tuple(rect1->getLayerNum(),
                               frBox(gtl::xl(*rect1),
                                     gtl::yl(*rect1),
                                     gtl::xh(*rect1),
                                     gtl::yh(*rect1)),
                               rect1->isFixed()));
  marker->addSrc(net2->getOwner());
  marker->addAggressor(net2->getOwner(),
                       make_tuple(rect2->getLayerNum(),
                                  frBox(gtl::xl(*rect2),
                                        gtl::yl(*rect2),
                                        gtl::xh(*rect2),
                                        gtl::yh(*rect2)),
                                  rect2->isFixed()));
  addMarker(std::move(marker));
}

void FlexGCWorker::Impl::checkCutSpacing_spc_diff_layer(
    gcRect* rect1,
    gcRect* rect2,
    const gtl::rectangle_data<frCoord>& markerRect,
    frCutSpacingConstraint* con)
{
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
  frSquaredDistance reqSpcValSquare
      = checkCutSpacing_spc_getReqSpcVal(rect1, rect2, con);
  reqSpcValSquare *= reqSpcValSquare;

  gtl::point_data<frCoord> center1, center2;
  gtl::center(center1, *rect1);
  gtl::center(center2, *rect2);
  frSquaredDistance distSquare = 0;

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
  frBox box(gtl::xl(markerRect),
            gtl::yl(markerRect),
            gtl::xh(markerRect),
            gtl::yh(markerRect));
  marker->setBBox(box);
  marker->setLayerNum(layerNum);
  marker->setConstraint(con);
  marker->addSrc(net1->getOwner());
  marker->addVictim(net1->getOwner(),
                    make_tuple(rect1->getLayerNum(),
                               frBox(gtl::xl(*rect1),
                                     gtl::yl(*rect1),
                                     gtl::xh(*rect1),
                                     gtl::yh(*rect1)),
                               rect1->isFixed()));
  marker->addSrc(net2->getOwner());
  marker->addAggressor(net2->getOwner(),
                       make_tuple(rect2->getLayerNum(),
                                  frBox(gtl::xl(*rect2),
                                        gtl::yl(*rect2),
                                        gtl::xh(*rect2),
                                        gtl::yh(*rect2)),
                                  rect2->isFixed()));
  addMarker(std::move(marker));
}

// check LEF58 SPACING constraint for cut layer
// rect1 ==  victim, rect2 == aggressor
void FlexGCWorker::Impl::checkLef58CutSpacing_main(
    gcRect* rect1,
    gcRect* rect2,
    frLef58CutSpacingConstraint* con)
{
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
    logger_->warn(
        DRT, 44, "Unsupported LEF58_SPACING rule for cut layer, skipped.");
  }
}

bool FlexGCWorker::Impl::checkLef58CutSpacing_spc_hasAdjCuts(
    gcRect* rect,
    frLef58CutSpacingConstraint* con)
{
  auto layerNum = rect->getLayerNum();
  auto layer = getTech()->getLayer(layerNum);

  auto conCutClassIdx = con->getCutClassIdx();

  frSquaredDistance cutWithinSquare = con->getCutWithin();
  box_t queryBox;
  myBloat(*rect, cutWithinSquare, queryBox);
  cutWithinSquare *= cutWithinSquare;
  auto& workerRegionQuery = getWorkerRegionQuery();
  vector<rq_box_value_t<gcRect*>> result;
  workerRegionQuery.queryMaxRectangle(queryBox, layerNum, result);
  int reqNumCut = con->getAdjacentCuts();
  int cnt = -1;
  gtl::point_data<frCoord> center1, center2;
  gtl::center(center1, *rect);
  // count adj cuts
  for (auto& [objBox, ptr] : result) {
    frSquaredDistance distSquare = 0;
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

bool FlexGCWorker::Impl::checkLef58CutSpacing_spc_hasTwoCuts(
    gcRect* rect1,
    gcRect* rect2,
    frLef58CutSpacingConstraint* con)
{
  if (checkLef58CutSpacing_spc_hasTwoCuts_helper(rect1, con)
      && checkLef58CutSpacing_spc_hasTwoCuts_helper(rect2, con)) {
    return true;
  } else {
    return false;
  }
}

bool FlexGCWorker::Impl::checkLef58CutSpacing_spc_hasTwoCuts_helper(
    gcRect* rect,
    frLef58CutSpacingConstraint* con)
{
  auto layerNum = rect->getLayerNum();
  auto layer = getTech()->getLayer(layerNum);

  auto conCutClassIdx = con->getCutClassIdx();

  frSquaredDistance cutWithinSquare = con->getCutWithin();
  box_t queryBox;
  myBloat(*rect, cutWithinSquare, queryBox);
  cutWithinSquare *= cutWithinSquare;
  auto& workerRegionQuery = getWorkerRegionQuery();
  vector<rq_box_value_t<gcRect*>> result;
  workerRegionQuery.queryMaxRectangle(queryBox, layerNum, result);
  int reqNumCut = con->getTwoCuts();
  int cnt = -1;
  gtl::point_data<frCoord> center1, center2;
  gtl::center(center1, *rect);
  // count adj cuts
  for (auto& [objBox, ptr] : result) {
    frSquaredDistance distSquare = 0;
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

frCoord FlexGCWorker::Impl::checkLef58CutSpacing_spc_getReqSpcVal(
    gcRect* ptr1,
    gcRect* ptr2,
    frLef58CutSpacingConstraint* con)
{
  frCoord maxSpcVal = 0;
  if (con) {
    maxSpcVal = con->getCutSpacing();
    if (con->hasAdjacentCuts()) {
      auto owner = ptr1->getNet()->getOwner();
      auto ptr1LayerNum = ptr1->getLayerNum();
      auto ptr1Layer = getTech()->getLayer(ptr1LayerNum);
      if (isBlockage(owner)) {
        frCoord width1 = ptr1->width();
        updateBlockageWidth(owner, width1);
        if (width1 > int(ptr1Layer->getWidth()))
          maxSpcVal = con->getCutWithin();
      }
      owner = ptr2->getNet()->getOwner();
      auto ptr2LayerNum = ptr2->getLayerNum();
      auto ptr2Layer = getTech()->getLayer(ptr2LayerNum);
      if (isBlockage(owner)) {
        frCoord width2 = ptr2->width();
        updateBlockageWidth(owner, width2);
        if (width2 > int(ptr2Layer->getWidth()))
          maxSpcVal = con->getCutWithin();
      }
    }
  }
  return maxSpcVal;
}

// only works for GF14 syntax (i.e., TWOCUTS), not full rule support
void FlexGCWorker::Impl::checkLef58CutSpacing_spc_adjCut(
    gcRect* rect1,
    gcRect* rect2,
    const gtl::rectangle_data<frCoord>& markerRect,
    frLef58CutSpacingConstraint* con)
{
  auto layerNum = rect1->getLayerNum();
  auto net1 = rect1->getNet();
  auto net2 = rect2->getNet();

  bool isSkip = true;
  if (checkLef58CutSpacing_spc_hasAdjCuts(rect1, con)) {
    isSkip = false;
  }
  if (con->hasTwoCuts()
      && checkLef58CutSpacing_spc_hasTwoCuts(rect1, rect2, con)) {
    isSkip = false;
  }
  // skip only when neither adjCuts nor twoCuts is satisfied
  if (isSkip) {
    return;
  }

  if (con->hasExactAligned()) {
    logger_->warn(
        DRT,
        45,
        " unsupported branch EXACTALIGNED in checkLef58CutSpacing_spc_adjCut.");
    return;
  }
  if (con->isExceptSamePGNet()) {
    logger_->warn(DRT,
                  46,
                  " unsupported branch EXCEPTSAMEPGNET in "
                  "checkLef58CutSpacing_spc_adjCut.");
    return;
  }
  if (con->hasExceptAllWithin()) {
    logger_->warn(DRT,
                  47,
                  " unsupported branch EXCEPTALLWITHIN in "
                  "checkLef58CutSpacing_spc_adjCut.");
    return;
  }
  if (con->isToAll()) {
    logger_->warn(
        DRT,
        48,
        " unsupported branch TO ALL in checkLef58CutSpacing_spc_adjCut.");
    return;
  }
  if (con->isNoPrl()) {
    logger_->warn(
        DRT,
        49,
        " unsupported branch NOPRL in checkLef58CutSpacing_spc_adjCut.");
    return;
  }
  if (con->hasEnclosure()) {
    logger_->warn(
        DRT,
        50,
        " unsupported branch ENCLOSURE in checkLef58CutSpacing_spc_adjCut.");
    return;
  }
  if (con->isSideParallelOverlap()) {
    logger_->warn(DRT,
                  51,
                  " unsupported branch SIDEPARALLELOVERLAP in "
                  "checkLef58CutSpacing_spc_adjCut.");
    return;
  }
  if (con->isSameMask()) {
    logger_->warn(
        DRT,
        52,
        " unsupported branch SAMEMASK in checkLef58CutSpacing_spc_adjCut.");
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

  frSquaredDistance reqSpcValSquare
      = checkLef58CutSpacing_spc_getReqSpcVal(rect1, rect2, con);
  reqSpcValSquare *= reqSpcValSquare;

  gtl::point_data<frCoord> center1, center2;
  gtl::center(center1, *rect1);
  gtl::center(center2, *rect2);
  frSquaredDistance distSquare = 0;
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
  frBox box(gtl::xl(markerRect),
            gtl::yl(markerRect),
            gtl::xh(markerRect),
            gtl::yh(markerRect));
  marker->setBBox(box);
  marker->setLayerNum(layerNum);
  marker->setConstraint(con);
  marker->addSrc(net1->getOwner());
  marker->addVictim(net1->getOwner(),
                    make_tuple(rect1->getLayerNum(),
                               frBox(gtl::xl(*rect1),
                                     gtl::yl(*rect1),
                                     gtl::xh(*rect1),
                                     gtl::yh(*rect1)),
                               rect1->isFixed()));
  marker->addSrc(net2->getOwner());
  marker->addAggressor(net2->getOwner(),
                       make_tuple(rect2->getLayerNum(),
                                  frBox(gtl::xl(*rect2),
                                        gtl::yl(*rect2),
                                        gtl::xh(*rect2),
                                        gtl::yh(*rect2)),
                                  rect2->isFixed()));
  addMarker(std::move(marker));
}

// only works for GF14 syntax, not full rule support
void FlexGCWorker::Impl::checkLef58CutSpacing_spc_layer(
    gcRect* rect1,
    gcRect* rect2,
    const gtl::rectangle_data<frCoord>& markerRect,
    frLef58CutSpacingConstraint* con)
{
  auto layerNum = rect1->getLayerNum();
  auto secondLayerNum = rect2->getLayerNum();
  auto net1 = rect1->getNet();
  auto net2 = rect2->getNet();
  auto reqSpcVal = con->getCutSpacing();
  frSquaredDistance reqSpcValSquare = (frSquaredDistance) reqSpcVal * reqSpcVal;

  // skip unsupported rule branch
  if (con->isStack()) {
    logger_->warn(
        DRT, 54, "unsupported branch STACK in checkLef58CutSpacing_spc_layer.");
    return;
  } else if (con->hasOrthogonalSpacing()) {
    logger_->warn(DRT,
                  55,
                  "unsupported branch ORTHOGONALSPACING in "
                  "checkLef58CutSpacing_spc_layer.");
    return;
  } else if (con->hasCutClass()) {
    ;
    if (con->isShortEdgeOnly()) {
      logger_->warn(DRT,
                    56,
                    "unsupported branch SHORTEDGEONLY in "
                    "checkLef58CutSpacing_spc_layer.");
      return;
    } else if (con->isConcaveCorner()) {
      if (con->hasWidth()) {
        logger_->warn(
            DRT,
            57,
            "unsupported branch WIDTH in checkLef58CutSpacing_spc_layer.");
      } else if (con->hasParallel()) {
        logger_->warn(
            DRT,
            58,
            "unsupported branch PARALLEL in checkLef58CutSpacing_spc_layer.");
      } else if (con->hasEdgeLength()) {
        logger_->warn(
            DRT,
            59,
            "unsupported branch EDGELENGTH in checkLef58CutSpacing_spc_layer.");
      }
    } else if (con->hasExtension()) {
      logger_->warn(
          DRT,
          60,
          "unsupported branch EXTENSION in checkLef58CutSpacing_spc_layer.");
    } else if (con->hasNonEolConvexCorner()) {
      ;
    } else if (con->hasAboveWidth()) {
      logger_->warn(
          DRT,
          61,
          "unsupported branch ABOVEWIDTH in checkLef58CutSpacing_spc_layer.");
    } else if (con->isMaskOverlap()) {
      logger_->warn(
          DRT,
          62,
          "unsupported branch MASKOVERLAP in checkLef58CutSpacing_spc_layer.");
    } else if (con->isWrongDirection()) {
      logger_->warn(DRT,
                    63,
                    "unsupported branch WRONGDIRECTION in "
                    "checkLef58CutSpacing_spc_layer.");
    }
  }

  // start checking
  if (con->isStack()) {
    ;
  } else if (con->hasOrthogonalSpacing()) {
    ;
  } else if (con->hasCutClass()) {
    auto conCutClassIdx = con->getCutClassIdx();
    auto cutClassIdx = getTech()->getLayer(layerNum)->getCutClassIdx(
        rect1->width(), rect1->length());
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
      vector<pair<segment_t, gcSegment*>> results;
      auto& workerRegionQuery = getWorkerRegionQuery();
      workerRegionQuery.queryPolygonEdge(queryBox, secondLayerNum, results);
      for (auto& [boostSeg, gcSeg] : results) {
        auto corner = gcSeg->getLowCorner();
        if (corner->getType() != frCornerTypeEnum::CONCAVE) {
          continue;
        }
        // exclude non-rect-boundary case
        if ((corner->x() != gtl::xl(*rect2) && corner->x() != gtl::xh(*rect2))
            && (corner->y() != gtl::yl(*rect2)
                && corner->y() != gtl::yh(*rect2))) {
          continue;
        }
        gtl::rectangle_data<frCoord> markerRect(
            corner->x(), corner->y(), corner->x(), corner->y());
        gtl::generalized_intersect(markerRect, *rect1);

        frSquaredDistance distSquare = 0;
        if (con->isCenterToCenter()) {
          gtl::point_data<frCoord> center1;
          gtl::center(center1, *rect1);
          distSquare
              = gtl::distance_squared(center1, corner->getNextEdge()->low());
        } else {
          distSquare = gtl::square_euclidean_distance(
              *rect1, corner->getNextEdge()->low());
        }
        if (distSquare >= reqSpcValSquare) {
          continue;
        }

        if (rect1->isFixed() && corner->isFixed()) {
          continue;
        }
        // this should only happen between samenet
        auto marker = make_unique<frMarker>();
        frBox box(gtl::xl(markerRect),
                  gtl::yl(markerRect),
                  gtl::xh(markerRect),
                  gtl::yh(markerRect));
        marker->setBBox(box);
        marker->setLayerNum(layerNum);
        marker->setConstraint(con);
        marker->addSrc(net1->getOwner());
        marker->addVictim(net1->getOwner(),
                          make_tuple(layerNum,
                                     frBox(gtl::xl(*rect1),
                                           gtl::yl(*rect1),
                                           gtl::xh(*rect1),
                                           gtl::yh(*rect1)),
                                     rect1->isFixed()));
        marker->addSrc(net2->getOwner());
        marker->addAggressor(
            net2->getOwner(),
            make_tuple(
                secondLayerNum,
                frBox(corner->x(), corner->y(), corner->x(), corner->y()),
                corner->isFixed()));
        addMarker(std::move(marker));
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
      vector<pair<segment_t, gcSegment*>> results;
      auto& workerRegionQuery = getWorkerRegionQuery();
      workerRegionQuery.queryPolygonEdge(queryBox, secondLayerNum, results);
      for (auto& [boostSeg, gcSeg] : results) {
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
            if (gtl::length(*(corner->getNextEdge())) < con->getMinLength()
                || gtl::length(*(corner->getPrevCorner()->getPrevEdge()))
                       < con->getMinLength()) {
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
            if (gtl::length(*(corner->getPrevEdge())) < con->getMinLength()
                || gtl::length(*(corner->getNextCorner()->getNextEdge()))
                       < con->getMinLength()) {
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

        if (isPrevEdgeEOL
            && gtl::length(*(corner->getPrevEdge())) < con->getEolWidth()) {
          continue;
        }
        if (isNextEdgeEOL
            && gtl::length(*(corner->getNextEdge())) < con->getEolWidth()) {
          continue;
        }

        // start checking
        gtl::rectangle_data<frCoord> markerRect(
            corner->x(), corner->y(), corner->x(), corner->y());
        gtl::generalized_intersect(markerRect, *rect1);

        auto dx = gtl::delta(markerRect, gtl::HORIZONTAL);
        auto dy = gtl::delta(markerRect, gtl::VERTICAL);

        auto edgeX = reqSpcVal;
        auto edgeY = reqSpcVal;
        if (corner->getNextEdge()->getDir() == frDirEnum::N
            || corner->getNextEdge()->getDir() == frDirEnum::S) {
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
        frBox box(gtl::xl(markerRect),
                  gtl::yl(markerRect),
                  gtl::xh(markerRect),
                  gtl::yh(markerRect));
        marker->setBBox(box);
        marker->setLayerNum(layerNum);
        marker->setConstraint(con);
        marker->addSrc(net1->getOwner());
        marker->addVictim(net1->getOwner(),
                          make_tuple(layerNum,
                                     frBox(gtl::xl(*rect1),
                                           gtl::yl(*rect1),
                                           gtl::xh(*rect1),
                                           gtl::yh(*rect1)),
                                     rect1->isFixed()));
        marker->addSrc(net2->getOwner());
        marker->addAggressor(
            net2->getOwner(),
            make_tuple(
                secondLayerNum,
                frBox(corner->x(), corner->y(), corner->x(), corner->y()),
                corner->isFixed()));
        addMarker(std::move(marker));
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
void FlexGCWorker::Impl::checkCutSpacing_main(gcRect* ptr1,
                                              gcRect* ptr2,
                                              frCutSpacingConstraint* con)
{
  // skip if same obj
  if (ptr1 == ptr2) {
    return;
  }
  // skip if con is not same net rule, but layer has same net rule and are same
  // net
  // TODO: filter the rule upfront
  if (!con->hasSameNet() && ptr1->getNet() == ptr2->getNet()) {
    // same layer same net
    if (!(con->hasSecondLayer())) {
      if (getTech()->getLayer(ptr1->getLayerNum())->hasCutSpacing(true)) {
        return;
      }
      // diff layer same net
    } else {
      if (getTech()
              ->getLayer(ptr1->getLayerNum())
              ->hasInterLayerCutSpacing(con->getSecondLayerNum(), true)) {
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

bool FlexGCWorker::Impl::checkCutSpacing_main_hasAdjCuts(
    gcRect* rect,
    frCutSpacingConstraint* con)
{
  // no adj cut rule, must proceed checking
  if (!con->isAdjacentCuts()) {
    return true;
  }
  auto layerNum = rect->getLayerNum();
  auto layer = getTech()->getLayer(layerNum);

  // rect is obs larger than min. size cut, must check against cutWithin
  if (isBlockage(rect->getNet()->getOwner())
      && rect->width() > int(layer->getWidth())) {
    return true;
  }

  frSquaredDistance cutWithinSquare = con->getCutWithin();
  box_t queryBox;
  myBloat(*rect, cutWithinSquare, queryBox);
  cutWithinSquare *= cutWithinSquare;
  auto& workerRegionQuery = getWorkerRegionQuery();
  vector<rq_box_value_t<gcRect*>> result;
  workerRegionQuery.queryMaxRectangle(queryBox, layerNum, result);
  int reqNumCut = con->getAdjacentCuts();
  int cnt = -1;
  gtl::point_data<frCoord> center1, center2;
  gtl::center(center1, *rect);
  // count adj cuts
  for (auto& [objBox, ptr] : result) {
    frSquaredDistance distSquare = 0;
    if (con->hasCenterToCenter()) {
      gtl::center(center2, *ptr);
      distSquare = gtl::distance_squared(center1, center2);
    } else {
      distSquare = gtl::square_euclidean_distance(*rect, *ptr);
    }
    if (distSquare >= cutWithinSquare) {
      continue;
    }
    // if target is a cut blockage shape larger than min. size, assume it is a
    // blockage from MACRO
    if (isBlockage(ptr->getNet()->getOwner())
        && ptr->width() > int(layer->getWidth())) {
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

void FlexGCWorker::Impl::checkLef58CutSpacing_main(
    gcRect* rect,
    frLef58CutSpacingConstraint* con,
    bool skipDiffNet)
{
  auto layerNum = rect->getLayerNum();
  auto maxSpcVal = checkLef58CutSpacing_getMaxSpcVal(con);
  box_t queryBox;
  myBloat(*rect, maxSpcVal, queryBox);

  auto& workerRegionQuery = getWorkerRegionQuery();
  vector<rq_box_value_t<gcRect*>> result;
  if (con->hasSecondLayer()) {
    workerRegionQuery.queryMaxRectangle(
        queryBox, con->getSecondLayerNum(), result);
  } else {
    workerRegionQuery.queryMaxRectangle(queryBox, layerNum, result);
  }

  for (auto& [objBox, ptr] : result) {
    if (skipDiffNet && rect->getNet() != ptr->getNet()) {
      continue;
    }
    checkLef58CutSpacing_main(rect, ptr, con);
  }
}

void FlexGCWorker::Impl::checkCutSpacing_main(gcRect* rect,
                                              frCutSpacingConstraint* con)
{
  auto layerNum = rect->getLayerNum();
  auto maxSpcVal = checkCutSpacing_getMaxSpcVal(con);
  box_t queryBox;
  myBloat(*rect, maxSpcVal, queryBox);

  // skip if adjcut not satisfied
  if (!checkCutSpacing_main_hasAdjCuts(rect, con)) {
    return;
  }

  auto& workerRegionQuery = getWorkerRegionQuery();
  vector<rq_box_value_t<gcRect*>> result;
  if (con->hasSecondLayer()) {
    workerRegionQuery.queryMaxRectangle(
        queryBox, con->getSecondLayerNum(), result);
  } else {
    workerRegionQuery.queryMaxRectangle(queryBox, layerNum, result);
  }
  // Short, metSpc, NSMetal here
  for (auto& [objBox, ptr] : result) {
    if (con->hasSecondLayer()) {
      if (rect->getNet() != ptr->getNet()
          || con->getSameNetConstraint() == nullptr) {
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

void FlexGCWorker::Impl::checkCutSpacing_main(gcRect* rect)
{
  auto layerNum = rect->getLayerNum();

  // CShort
  // diff net same layer
  for (auto con : getTech()->getLayer(layerNum)->getCutSpacing(false)) {
    checkCutSpacing_main(rect, con);
  }
  // same net same layer
  for (auto con : getTech()->getLayer(layerNum)->getCutSpacing(true)) {
    checkCutSpacing_main(rect, con);
  }
  // diff net diff layer
  for (auto con :
       getTech()->getLayer(layerNum)->getInterLayerCutSpacingConstraint(
           false)) {
    if (con) {
      checkCutSpacing_main(rect, con);
    }
  }

  // LEF58_SPACING for cut layer
  bool skipDiffNet = false;
  // samenet rule
  for (auto con :
       getTech()->getLayer(layerNum)->getLef58CutSpacingConstraints(true)) {
    // skipSameNet if same-net rule exists
    skipDiffNet = true;
    checkLef58CutSpacing_main(rect, con, false);
  }
  // diffnet rule
  for (auto con :
       getTech()->getLayer(layerNum)->getLef58CutSpacingConstraints(false)) {
    checkLef58CutSpacing_main(rect, con, skipDiffNet);
  }
}

void FlexGCWorker::Impl::checkCutSpacing()
{
  if (targetNet_) {
    // layer --> net --> polygon --> maxrect
    for (int i
         = std::max((frLayerNum)(getTech()->getBottomLayerNum()), minLayerNum_);
         i <= std::min((frLayerNum)(getTech()->getTopLayerNum()), maxLayerNum_);
         i++) {
      auto currLayer = getTech()->getLayer(i);
      if (currLayer->getType() != frLayerTypeEnum::CUT) {
        continue;
      }
      for (auto& pin : targetNet_->getPins(i)) {
        for (auto& maxrect : pin->getMaxRectangles()) {
          checkCutSpacing_main(maxrect.get());
        }
      }
    }
  } else {
    // layer --> net --> polygon --> maxrect
    for (int i
         = std::max((frLayerNum)(getTech()->getBottomLayerNum()), minLayerNum_);
         i <= std::min((frLayerNum)(getTech()->getTopLayerNum()), maxLayerNum_);
         i++) {
      auto currLayer = getTech()->getLayer(i);
      if (currLayer->getType() != frLayerTypeEnum::CUT) {
        continue;
      }
      for (auto& net : getNets()) {
        for (auto& pin : net->getPins(i)) {
          for (auto& maxrect : pin->getMaxRectangles()) {
            // cout <<"from " <<maxrect.get() <<endl;
            checkCutSpacing_main(maxrect.get());
          }
        }
      }
    }
  }
}

void FlexGCWorker::Impl::patchMetalShape()
{
  pwires_.clear();
  clearMarkers();

  checkMetalShape();

  patchMetalShape_helper();

  clearMarkers();
}

// loop through violation and patch C5 enclosure minStep for GF14
void FlexGCWorker::Impl::patchMetalShape_helper()
{
  vector<drConnFig*> results;
  for (auto& marker : markers_) {
    results.clear();
    if (marker->getConstraint()->typeId()
        != frConstraintTypeEnum::frcMinStepConstraint) {
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
    auto& workerRegionQuery = getDRWorker()->getWorkerRegionQuery();
    marker->getBBox(markerBBox);
    frPoint markerCenter((markerBBox.left() + markerBBox.right()) / 2,
                         (markerBBox.bottom() + markerBBox.top()) / 2);
    workerRegionQuery.query(markerBBox, lNum, results);
    for (auto& connFig : results) {
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
    gtl::rectangle_data<frCoord> markerRect(markerBBox.left(),
                                            markerBBox.bottom(),
                                            markerBBox.right(),
                                            markerBBox.top());
    gtl::point_data<frCoord> boostOrigin(origin.x(), origin.y());
    gtl::encompass(markerRect, boostOrigin);
    patchLL.set(gtl::xl(markerRect) - origin.x(),
                gtl::yl(markerRect) - origin.y());
    patchUR.set(gtl::xh(markerRect) - origin.x(),
                gtl::yh(markerRect) - origin.y());

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

int FlexGCWorker::Impl::main()
{
  ProfileTask profile("GC:main");
  // printMarker = true;
  //  minStep patching for GF14
  if (surgicalFixEnabled_ && getDRWorker()
      && DBPROCESSNODE == "GF14_13M_3Mx_2Cx_4Kx_2Hx_2Gx_LB") {
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
  // check SpacingTable Influence
  checkMetalSpacingTableInfluence();
  return 0;
}
