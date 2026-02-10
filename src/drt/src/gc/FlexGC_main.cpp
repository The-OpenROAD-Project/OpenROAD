// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "boost/geometry/geometry.hpp"
#include "boost/polygon/polygon.hpp"
#include "db/drObj/drNet.h"
#include "db/drObj/drShape.h"
#include "db/drObj/drVia.h"
#include "db/gcObj/gcShape.h"
#include "db/obj/frBlockObject.h"
#include "db/obj/frMarker.h"
#include "db/tech/frConstraint.h"
#include "frBaseTypes.h"
#include "frProfileTask.h"
#include "gc/FlexGC.h"
#include "gc/FlexGC_impl.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "odb/isotropy.h"

using odb::dbTechLayerType;

namespace drt {

using polygon_t = bg::model::polygon<point_t>;
using mpolygon_t = bg::model::multi_polygon<polygon_t>;

bool FlexGCWorker::Impl::isCornerOverlap(gcCorner* corner, const odb::Rect& box)
{
  frCoord cornerX = corner->getNextEdge()->low().x();
  frCoord cornerY = corner->getNextEdge()->low().y();
  switch (corner->getDir()) {
    case frCornerDirEnum::NE:
      if (cornerX == box.xMax() && cornerY == box.yMax()) {
        return true;
      }
      break;
    case frCornerDirEnum::SE:
      if (cornerX == box.xMax() && cornerY == box.yMin()) {
        return true;
      }
      break;
    case frCornerDirEnum::SW:
      if (cornerX == box.xMin() && cornerY == box.yMin()) {
        return true;
      }
      break;
    case frCornerDirEnum::NW:
      if (cornerX == box.xMin() && cornerY == box.yMax()) {
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

bool FlexGCWorker::Impl::hasRoute(gcRect* rect,
                                  gtl::rectangle_data<frCoord> markerRect)
{
  // marker enlarged by width
  auto width = rect->width();
  gtl::bloat(markerRect, width);
  // widthrect
  gtl::polygon_90_set_data<frCoord> tmpPoly;
  using boost::polygon::operators::operator+=;
  using boost::polygon::operators::operator&=;
  tmpPoly += markerRect;
  tmpPoly &= *rect;  // tmpPoly now is widthrect
  auto targetArea = gtl::area(tmpPoly);
  // get fixed shapes
  tmpPoly &= rect->getNet()->getPolygons(rect->getLayerNum(), true);
  if (gtl::area(tmpPoly) < targetArea) {
    return true;
  }
  return false;
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
        logger_->warn(DRT, 41, "Unsupported metSpc rule.");
    }
    if (checkNDRs) {
      return std::max(maxSpcVal,
                      getTech()->getMaxNondefaultSpacing(layerNum / 2 - 1));
    }
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
  }
  return false;
}

box_t FlexGCWorker::Impl::checkMetalCornerSpacing_getQueryBox(
    gcCorner* corner,
    const frCoord maxSpcValX,
    const frCoord maxSpcValY)
{
  box_t queryBox;
  frCoord baseX = corner->x();
  frCoord baseY = corner->y();
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
      logger_->error(DRT, 42, "Unknown corner direction.");
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
      return type.isSupply();
    }
    case frcInstTerm: {
      auto type = static_cast<frInstTerm*>(obj)->getTerm()->getType();
      return type.isSupply();
    }
    case frcBTerm: {
      auto type = static_cast<frTerm*>(obj)->getType();
      return type.isSupply();
    }
    default:
      return false;
  }
}

frCoord FlexGCWorker::Impl::checkMetalSpacing_prl_getReqSpcVal(gcRect* rect1,
                                                               gcRect* rect2,
                                                               frCoord prl,
                                                               bool& isSpcRange)
{
  isSpcRange = false;
  auto layerNum = rect1->getLayerNum();
  frCoord reqSpcVal = 0;
  auto currLayer = getTech()->getLayer(layerNum);
  bool isObs = false;
  auto width1 = rect1->width();
  auto width2 = rect2->width();
  // override width and spacing
  if (rect1->getNet()->isBlockage()) {
    isObs = true;
    if (router_cfg_->USEMINSPACING_OBS) {
      width1 = currLayer->getWidth();
    }
    if (rect1->getNet()->getDesignRuleWidth() != -1) {
      width1 = rect1->getNet()->getDesignRuleWidth();
    }
  }
  if (rect2->getNet()->isBlockage()) {
    isObs = true;
    if (router_cfg_->USEMINSPACING_OBS) {
      width2 = currLayer->getWidth();
    }
    if (rect2->getNet()->getDesignRuleWidth() != -1) {
      width2 = rect2->getNet()->getDesignRuleWidth();
    }
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
          if (!conSamenet->hasPGonly()) {
            reqSpcVal = std::max(conSamenet->getMinSpacing(), minSpcVal);
          } else if (isPG(rect1->getNet()->getOwner())) {
            reqSpcVal = std::max(conSamenet->getMinSpacing(), minSpcVal);
          }
        }
      }
    }
  }
  if (currLayer->hasSpacingRangeConstraints()
      && rect2->getNet() != rect1->getNet()) {
    for (const auto& con : currLayer->getSpacingRangeConstraints()) {
      if (con->inRange(width1) || con->inRange(width2)) {
        if (con->getMinSpacing() > reqSpcVal) {
          isSpcRange = true;
          reqSpcVal = con->getMinSpacing();
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
  std::vector<std::pair<segment_t, gcSegment*>> result;
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
  }
  if ((type == 1 || type == 2) && (flagL && flagR)) {
    return true;
  }
  return false;
}
std::string rectToString(gcRect& r)
{
  std::string s = std::to_string(gtl::xl(r)) + " " + std::to_string(gtl::yl(r))
                  + " " + std::to_string(gtl::xh(r)) + " "
                  + std::to_string(gtl::yh(r)) + " tapered ? "
                  + std::to_string(r.isTapered());
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
  bool isSpcRange = false;
  auto reqSpcVal
      = checkMetalSpacing_prl_getReqSpcVal(rect1, rect2, prl, isSpcRange);
  if (checkNDRs) {
    frCoord ndrSpc1 = 0, ndrSpc2 = 0;
    if (!rect1->isFixed() && net1->isNondefault() && !rect1->isTapered()) {
      ndrSpc1
          = net1->getFrNet()->getNondefaultRule()->getSpacing(layerNum / 2 - 1);
    }
    if (!rect2->isFixed() && net2->isNondefault() && !rect2->isTapered()) {
      ndrSpc2
          = net2->getFrNet()->getNondefaultRule()->getSpacing(layerNum / 2 - 1);
    }
    reqSpcVal = std::max({reqSpcVal, ndrSpc1, ndrSpc2});
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
    if (!hasRoute(rect1, markerRect) && !hasRoute(rect2, markerRect)) {
      return;
    }
  } else {
    using boost::polygon::operators::operator+=;
    using boost::polygon::operators::operator-=;
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
      if (markerPoly.empty()) {
        return;
      }
      // check if the edge related to the 0 width is within the net shape (this
      // is an indirect check)
      std::vector<gtl::rectangle_data<frCoord>> rects;
      markerPoly.get_rectangles(rects);
      if (rects.size() == 1) {
        if (isX) {
          if (gtl::xl(rects[0]) == gtl::xl(markerRect)
              || gtl::xh(rects[0]) == gtl::xl(markerRect)) {
            return;
          }
        } else {
          if (gtl::yl(rects[0]) == gtl::yl(markerRect)
              || gtl::yh(rects[0]) == gtl::yl(markerRect)) {
            return;
          }
        }
      }
    } else {
      markerPoly += mRect;
      markerPoly -= netPoly;
      if (markerPoly.empty()) {
        return;
      }
    }
  }
  auto marker = std::make_unique<frMarker>();
  odb::Rect box(gtl::xl(markerRect),
                gtl::yl(markerRect),
                gtl::xh(markerRect),
                gtl::yh(markerRect));
  marker->setBBox(box);
  marker->setLayerNum(layerNum);
  if (isSpcRange) {
    marker->setConstraint(
        getTech()->getLayer(layerNum)->getSpacingRangeConstraints().at(0));
  } else {
    marker->setConstraint(getTech()->getLayer(layerNum)->getMinSpacing());
  }
  marker->addSrc(net1->getOwner());
  marker->addVictim(net1->getOwner(),
                    std::make_tuple(rect1->getLayerNum(),
                                    odb::Rect(gtl::xl(*rect1),
                                              gtl::yl(*rect1),
                                              gtl::xh(*rect1),
                                              gtl::yh(*rect1)),
                                    rect1->isFixed()));
  marker->addSrc(net2->getOwner());
  marker->addAggressor(net2->getOwner(),
                       std::make_tuple(rect2->getLayerNum(),
                                       odb::Rect(gtl::xl(*rect2),
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
  for (const auto& pt : p.outer()) {
    points.emplace_back(pt.x(), pt.y());
  }
  poly.set(points.begin(), points.end());
  using boost::polygon::operators::operator+=;
  set += poly;
  return set;
}
void FlexGCWorker::Impl::checkMetalSpacing_short_obs(
    gcRect* rect1,
    gcRect* rect2,
    const gtl::rectangle_data<frCoord>& markerRect)
{
  if (rect1->isFixed() && rect2->isFixed()) {
    return;
  }
  bool isRect1Obs = rect1->getNet()->isBlockage();
  bool isRect2Obs = rect2->getNet()->isBlockage();
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
  std::vector<gtl::rectangle_data<frCoord>> rects;
  gtl::get_max_rectangles(rects, polys1);
  for (auto& rect : rects) {
    if (gtl::contains(rect, markerRect)) {
      return;
    }
    if (gtl::intersects(rect, markerRect)) {
      pins.push_back(rect2polygon(rect));
    }
  }
  polygon_t markerPoly = rect2polygon(markerRect);
  std::vector<polygon_t> result;
  if (pins.empty()) {
    result.push_back(markerPoly);
  } else {
    bg::difference(markerPoly, pins, result);
  }
  for (const auto& poly : result) {
    std::list<gtl::rectangle_data<frCoord>> res;
    gtl::get_max_rectangles(res, bg2gtl(poly));
    for (const auto& rect : res) {
      gcRect rect3 = *rect2;
      rect3.setRect(rect);
      gtl::rectangle_data<frCoord> newMarkerRect(markerRect);
      gtl::intersect(newMarkerRect, rect3);
      checkMetalSpacing_short(rect1, &rect3, newMarkerRect);
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
  using boost::polygon::operators::operator&;
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
    int64_t minWidth = getTech()->getLayer(layerNum)->getMinWidth();
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
    std::vector<rq_box_value_t<gcRect*>> result;
    workerRegionQuery.queryMaxRectangle(queryBox, layerNum, result);
    // std::cout <<"3rd obj" <<std::endl;
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
  if (rect1->isFixed() && rect2->isFixed()) {
    return;
  }

  // skip if marker area does not have route shape, must exclude touching
  if (checkMetalSpacing_short_skipFixed(rect1, rect2, markerRect)) {
    return;
  }
  // skip same-net sufficient metal
  if (checkMetalSpacing_short_skipSameNet(rect1, rect2, markerRect)) {
    return;
  }

  auto marker = std::make_unique<frMarker>();
  odb::Rect box(gtl::xl(markerRect),
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
                    std::make_tuple(rect1->getLayerNum(),
                                    odb::Rect(gtl::xl(*rect1),
                                              gtl::yl(*rect1),
                                              gtl::xh(*rect1),
                                              gtl::yh(*rect1)),
                                    rect1->isFixed()));
  marker->addSrc(net2->getOwner());
  marker->addAggressor(net2->getOwner(),
                       std::make_tuple(rect2->getLayerNum(),
                                       odb::Rect(gtl::xl(*rect2),
                                                 gtl::yl(*rect2),
                                                 gtl::xh(*rect2),
                                                 gtl::yh(*rect2)),
                                       rect2->isFixed()));
  addMarker(std::move(marker));
}

void FlexGCWorker::Impl::checkMetalSpacing_main(gcRect* rect1,
                                                gcRect* rect2,
                                                bool checkNDRs,
                                                bool isSpcRect)
{
  // NSMetal does not need self-intersection
  // Minimum width rule handled outside this function
  if (rect1 == rect2) {
    return;
  }
  gtl::rectangle_data<frCoord> markerRect(*rect1);
  auto distX = gtl::euclidean_distance(markerRect, *rect2, gtl::HORIZONTAL);
  auto distY = gtl::euclidean_distance(markerRect, *rect2, gtl::VERTICAL);

  gtl::generalized_intersect(markerRect, *rect2);
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
    // Zero width markers are not well handled by boost polygon as they
    // tend to disappear in boolean operations.  Give them a bit of extent
    // to avoid this.
    if (prlX == 0) {
      gtl::bloat(markerRect, gtl::HORIZONTAL, 1);
    }
    if (prlY == 0) {
      gtl::bloat(markerRect, gtl::VERTICAL, 1);
    }
    if (rect1->getNet()->isBlockage() || rect2->getNet()->isBlockage()) {
      checkMetalSpacing_short_obs(rect1, rect2, markerRect);
    } else {
      checkMetalSpacing_short(rect1, rect2, markerRect);
    }
    // prl
  } else {
    checkMetalSpacing_prl(rect1,
                          rect2,
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
  for (const auto& con :
       getTech()->getLayer(layerNum)->getSpacingRangeConstraints()) {
    if (con->inRange(rect->width())) {
      maxSpcVal = std::max(maxSpcVal, con->getMinSpacing());
    }
  }

  box_t queryBox;
  myBloat(*rect, maxSpcVal, queryBox);

  auto& workerRegionQuery = getWorkerRegionQuery();
  std::vector<rq_box_value_t<gcRect*>> result;
  workerRegionQuery.queryMaxRectangle(queryBox, layerNum, result);
  if (checkNDRs) {
    std::vector<rq_box_value_t<gcRect>> resultS;
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
    for (int i = std::max(getTech()->getBottomLayerNum(), minLayerNum_);
         i <= std::min(getTech()->getTopLayerNum(), maxLayerNum_);
         i++) {
      auto currLayer = getTech()->getLayer(i);
      if (currLayer->getType() != dbTechLayerType::ROUTING) {
        continue;
      }
      for (auto& pin : targetNet_->getPins(i)) {
        if (currLayer->hasLef58SpacingWrongDirConstraints()) {
          checkMetalSpacing_wrongDir(pin.get(), currLayer);
        }
        for (auto& maxrect : pin->getMaxRectangles()) {
          checkMetalSpacing_main(
              maxrect.get(),
              getDRWorker() || !router_cfg_->AUTO_TAPER_NDR_NETS);
          if (currLayer->hasTwoWiresForbiddenSpacingConstraints()) {
            for (auto con :
                 currLayer->getTwoWiresForbiddenSpacingConstraints()) {
              checkTwoWiresForbiddenSpc_main(maxrect.get(), con);
            }
          }
          if (currLayer->hasForbiddenSpacingConstraints()) {
            for (auto con : currLayer->getForbiddenSpacingConstraints()) {
              checkForbiddenSpc_main(maxrect.get(), con);
            }
          }
        }
      }
      for (auto& sr : targetNet_->getSpecialSpcRects()) {
        checkMetalSpacing_main(
            sr.get(), getDRWorker() || !router_cfg_->AUTO_TAPER_NDR_NETS, true);
      }
    }
  } else {
    // layer --> net --> polygon --> maxrect
    for (int i = std::max(getTech()->getBottomLayerNum(), minLayerNum_);
         i <= std::min(getTech()->getTopLayerNum(), maxLayerNum_);
         i++) {
      auto currLayer = getTech()->getLayer(i);
      if (currLayer->getType() != dbTechLayerType::ROUTING) {
        continue;
      }
      for (auto& net : getNets()) {
        for (auto& pin : net->getPins(i)) {
          if (currLayer->hasLef58SpacingWrongDirConstraints()) {
            checkMetalSpacing_wrongDir(pin.get(), currLayer);
          }
          for (auto& maxrect : pin->getMaxRectangles()) {
            // Short, NSMetal, metSpc
            checkMetalSpacing_main(
                maxrect.get(),
                getDRWorker() || !router_cfg_->AUTO_TAPER_NDR_NETS);
            if (currLayer->hasTwoWiresForbiddenSpacingConstraints()) {
              for (auto con :
                   currLayer->getTwoWiresForbiddenSpacingConstraints()) {
                checkTwoWiresForbiddenSpc_main(maxrect.get(), con);
              }
            }
            if (currLayer->hasForbiddenSpacingConstraints()) {
              for (auto con : currLayer->getForbiddenSpacingConstraints()) {
                checkForbiddenSpc_main(maxrect.get(), con);
              }
            }
          }
        }
        for (auto& sr : net->getSpecialSpcRects()) {
          checkMetalSpacing_main(
              sr.get(),
              getDRWorker() || !router_cfg_->AUTO_TAPER_NDR_NETS,
              true);
        }
      }
    }
  }
}

void FlexGCWorker::Impl::checkMetalSpacing_wrongDir_getQueryBox(gcSegment* edge,
                                                                frCoord spcVal,
                                                                box_t& queryBox)
{
  switch (edge->getDir()) {
    case frDirEnum::W:
      bg::set<bg::min_corner, 0>(queryBox, edge->high().x());
      bg::set<bg::min_corner, 1>(queryBox, edge->high().y());
      bg::set<bg::max_corner, 0>(queryBox, edge->low().x());
      bg::set<bg::max_corner, 1>(queryBox, edge->low().y() + spcVal);
      break;
    case frDirEnum::E:
      bg::set<bg::min_corner, 0>(queryBox, edge->low().x());
      bg::set<bg::min_corner, 1>(queryBox, edge->low().y() - spcVal);
      bg::set<bg::max_corner, 0>(queryBox, edge->high().x());
      bg::set<bg::max_corner, 1>(queryBox, edge->high().y());
      break;
    case frDirEnum::S:
      bg::set<bg::min_corner, 0>(queryBox, edge->high().x() - spcVal);
      bg::set<bg::min_corner, 1>(queryBox, edge->high().y());
      bg::set<bg::max_corner, 0>(queryBox, edge->low().x());
      bg::set<bg::max_corner, 1>(queryBox, edge->low().y());
      break;
    case frDirEnum::N:
      bg::set<bg::min_corner, 0>(queryBox, edge->low().x());
      bg::set<bg::min_corner, 1>(queryBox, edge->low().y());
      bg::set<bg::max_corner, 0>(queryBox, edge->high().x() + spcVal);
      bg::set<bg::max_corner, 1>(queryBox, edge->high().y());
      break;
    default:
      break;
  }
}

frCoord FlexGCWorker::Impl::getPrl(gcSegment* edge,
                                   gcSegment* ptr,
                                   const gtl::orientation_2d& orient) const
{
  const frCoord edge1_low = edge->low().get(orient);
  const frCoord edge1_high = edge->high().get(orient);
  const frCoord edge1_min = std::min(edge1_low, edge1_high);
  const frCoord edge1_max = std::max(edge1_low, edge1_high);

  const frCoord edge2_low = ptr->low().get(orient);
  const frCoord edge2_high = ptr->high().get(orient);
  const frCoord edge2_min = std::min(edge2_low, edge2_high);
  const frCoord edge2_max = std::max(edge2_low, edge2_high);
  return std::min(edge1_max, edge2_max) - std::max(edge1_min, edge2_min);
}

std::pair<frCoord, frCoord> FlexGCWorker::Impl::getRectsPrl(gcRect* rect1,
                                                            gcRect* rect2) const
{
  gtl::rectangle_data<frCoord> marker_rect(*rect1);
  gtl::generalized_intersect(marker_rect, *rect2);
  auto prl_x = gtl::delta(marker_rect, gtl::HORIZONTAL);
  auto prl_y = gtl::delta(marker_rect, gtl::VERTICAL);
  auto dist_x = gtl::euclidean_distance(*rect1, *rect2, gtl::HORIZONTAL);
  auto dist_y = gtl::euclidean_distance(*rect1, *rect2, gtl::VERTICAL);
  if (dist_x) {
    prl_x = -prl_x;
  }
  if (dist_y) {
    prl_y = -prl_y;
  }
  return {prl_x, prl_y};
}

void FlexGCWorker::Impl::checkMetalSpacing_wrongDir(gcPin* pin, frLayer* layer)
{
  auto lef58WrongDirCons = layer->getLef58SpacingWrongDirConstraints();
  auto layerNum = layer->getLayerNum();
  for (auto con : lef58WrongDirCons) {
    auto rule = con->getODBRule();
    auto spcVal = rule->getWrongdirSpace();
    // Loop over all edges of pin
    for (auto& edges : pin->getPolygonEdges()) {
      for (auto& edge : edges) {
        // Check wrongDir edge
        if (edge->isVertical() != layer->isVertical()) {
          // Check noneol flag
          if (rule->isNoneolValid()) {
            // Get edge length and compare
            auto edgeLength = gtl::length(*edge);
            auto noneolLength = rule->getNoneolWidth();
            if (edgeLength < noneolLength) {
              continue;
            }
          }
          gtl::rectangle_data<frCoord> rect1(edge->getLowCorner()->x(),
                                             edge->getLowCorner()->y(),
                                             edge->getHighCorner()->x(),
                                             edge->getHighCorner()->y());
          box_t queryBox;
          checkMetalSpacing_wrongDir_getQueryBox(edge.get(), spcVal, queryBox);
          std::vector<std::pair<segment_t, gcSegment*>> results;
          auto& workerRegionQuery = getWorkerRegionQuery();
          workerRegionQuery.queryPolygonEdge(queryBox, layerNum, results);
          for (auto& [boostSeg, ptr] : results) {
            // Check query edge wrongDir
            if (ptr->isVertical() != layer->isVertical()) {
              if (edge.get() == ptr) {
                continue;
              }

              // no violation if fixed shapes
              if (edge->isFixed() && ptr->isFixed()) {
                continue;
              }

              // Get edges prl
              const gtl::orientation_2d orient = edge->getOrientation();
              const frCoord prl = getPrl(edge.get(), ptr, orient);
              // Check PRL branch
              auto prlLength = rule->getPrlLength();
              if (prl <= prlLength) {
                continue;
              }

              // Check other edge noneol
              if (rule->isNoneolValid()) {
                // Get edge length and compare
                auto ptrLength = gtl::length(*ptr);
                auto noneolLength = rule->getNoneolWidth();
                if (ptrLength < noneolLength) {
                  continue;
                }
              }

              // Check wrongDir spacing
              gtl::rectangle_data<frCoord> rect2(ptr->getLowCorner()->x(),
                                                 ptr->getLowCorner()->y(),
                                                 ptr->getHighCorner()->x(),
                                                 ptr->getHighCorner()->y());
              frCoord dist = gtl::euclidean_distance(rect1, rect2);

              if (dist >= spcVal) {
                continue;
              }

              // Make marker
              auto net1 = edge->getNet();
              auto net2 = ptr->getNet();
              gtl::rectangle_data<frCoord> markerRect(rect1);
              gtl::generalized_intersect(markerRect, rect2);
              auto marker = std::make_unique<frMarker>();
              odb::Rect box(gtl::xl(markerRect),
                            gtl::yl(markerRect),
                            gtl::xh(markerRect),
                            gtl::yh(markerRect));
              marker->setBBox(box);
              marker->setLayerNum(layerNum);
              marker->setConstraint(con);
              marker->addSrc(net1->getOwner());
              frCoord llx = std::min(edge->getLowCorner()->x(),
                                     edge->getHighCorner()->x());
              frCoord lly = std::min(edge->getLowCorner()->y(),
                                     edge->getHighCorner()->y());
              frCoord urx = std::max(edge->getLowCorner()->x(),
                                     edge->getHighCorner()->x());
              frCoord ury = std::max(edge->getLowCorner()->y(),
                                     edge->getHighCorner()->y());
              marker->addVictim(net1->getOwner(),
                                std::make_tuple(edge->getLayerNum(),
                                                odb::Rect(llx, lly, urx, ury),
                                                edge->isFixed()));
              marker->addSrc(net2->getOwner());
              llx = std::min(ptr->getLowCorner()->x(),
                             ptr->getHighCorner()->x());
              lly = std::min(ptr->getLowCorner()->y(),
                             ptr->getHighCorner()->y());
              urx = std::max(ptr->getLowCorner()->x(),
                             ptr->getHighCorner()->x());
              ury = std::max(ptr->getLowCorner()->y(),
                             ptr->getHighCorner()->y());
              marker->addAggressor(
                  net2->getOwner(),
                  std::make_tuple(ptr->getLayerNum(),
                                  odb::Rect(llx, lly, urx, ury),
                                  ptr->isFixed()));
              addMarker(std::move(marker));
            }
          }
        }
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
  frCoord candX = -1, candY = -1;
  // ensure this is a real corner to corner case
  if (con->getCornerType() == frCornerTypeEnum::CONVEX) {
    if (candX = gtl::xh(*rect); cornerX >= candX) {
      if (candY = gtl::yh(*rect); cornerY >= candY) {
        if (corner->getDir() != frCornerDirEnum::SW) {
          return;
        }
      } else if (candY = gtl::yl(*rect); cornerY <= candY) {
        if (corner->getDir() != frCornerDirEnum::NW) {
          return;
        }
      } else {
        return;
      }
    } else if (candX = gtl::xl(*rect); cornerX <= candX) {
      if (candY = gtl::yh(*rect); cornerY >= candY) {
        if (corner->getDir() != frCornerDirEnum::SE) {
          return;
        }
      } else if (candY = gtl::yl(*rect); cornerY <= candY) {
        if (corner->getDir() != frCornerDirEnum::NE) {
          return;
        }
      } else {
        return;
      }
    } else {
      return;
    }
    // The corner of the rect has to be convex
    // TODO: detect concave corners
    if (rect->getNet()
        && !rect->getNet()->hasPolyCornerAt(
            candX, candY, rect->getLayerNum())) {
      return;
    }
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
      // check for the rect corners
      if (rect->getNet()) {
        auto corner2 = rect->getNet()->getPolyCornerAt(
            candX, candY, rect->getLayerNum());
        if (corner2->getType() == frCornerTypeEnum::CONVEX) {
          if (corner2->getNextCorner()->getType() == frCornerTypeEnum::CONVEX
              && gtl::length(*(corner2->getNextEdge())) < con->getEolWidth()) {
            return;
          }
          if (corner2->getPrevCorner()->getType() == frCornerTypeEnum::CONVEX
              && gtl::length(*(corner2->getPrevEdge())) < con->getEolWidth()) {
            return;
          }
        }
      }
    }
  }

  // query and iterate only the rects that have the corner as its corner
  auto layerNum = corner->getNextEdge()->getLayerNum();
  auto net = corner->getNextEdge()->getNet();
  auto& workerRegionQuery = getWorkerRegionQuery();
  std::vector<rq_box_value_t<gcRect*>> result;
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
      if (con->isCornerToCorner()) {
        // measure euclidean distance
        gtl::point_data<frCoord> point(cornerX, cornerY);
        frSquaredDistance distSquare
            = gtl::square_euclidean_distance(*rect, point);
        frSquaredDistance reqSpcValSquare
            = reqSpcVal * (frSquaredDistance) reqSpcVal;
        if (distSquare >= reqSpcValSquare) {
          continue;
        }
      } else if (maxXY >= reqSpcVal) {
        continue;
      }
      // no violation if fixed
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
        using boost::polygon::operators::operator+=;
        using boost::polygon::operators::operator&=;
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
        using boost::polygon::operators::operator+=;
        using boost::polygon::operators::operator&=;
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
      auto marker = std::make_unique<frMarker>();
      odb::Rect box(gtl::xl(markerRect),
                    gtl::yl(markerRect),
                    gtl::xh(markerRect),
                    gtl::yh(markerRect));
      marker->setBBox(box);
      marker->setLayerNum(layerNum);
      marker->setConstraint(con);
      marker->addSrc(net->getOwner());
      marker->addVictim(
          net->getOwner(),
          std::make_tuple(
              layerNum,
              odb::Rect(corner->x(), corner->y(), corner->x(), corner->y()),
              corner->isFixed()));
      marker->addSrc(rect->getNet()->getOwner());
      marker->addAggressor(rect->getNet()->getOwner(),
                           std::make_tuple(rect->getLayerNum(),
                                           odb::Rect(gtl::xl(*rect),
                                                     gtl::yl(*rect),
                                                     gtl::xh(*rect),
                                                     gtl::yh(*rect)),
                                           rect->isFixed()));
      addMarker(std::move(marker));
      return;
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
  std::vector<rq_box_value_t<gcRect*>> result;
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
  if (ignoreCornerSpacing_) {
    return;
  }
  if (targetNet_) {
    // layer --> net --> polygon --> corner
    for (int i = std::max(getTech()->getBottomLayerNum(), minLayerNum_);
         i <= std::min(getTech()->getTopLayerNum(), maxLayerNum_);
         i++) {
      auto currLayer = getTech()->getLayer(i);
      if (currLayer->getType() != dbTechLayerType::ROUTING
          || (!currLayer->hasLef58CornerSpacingConstraint()
              && currLayer->getWidthTblOrthCon() == nullptr)) {
        continue;
      }
      for (auto& pin : targetNet_->getPins(i)) {
        for (auto& corners : pin->getPolygonCorners()) {
          for (auto& corner : corners) {
            // LEF58 corner spacing
            if (currLayer->hasLef58CornerSpacingConstraint()) {
              checkMetalCornerSpacing_main(corner.get());
            }
            if (currLayer->getWidthTblOrthCon()) {
              checkWidthTableOrth(corner.get());
            }
          }
        }
      }
    }
  } else {
    // layer --> net --> polygon --> corner
    for (int i = std::max(getTech()->getBottomLayerNum(), minLayerNum_);
         i <= std::min(getTech()->getTopLayerNum(), maxLayerNum_);
         i++) {
      auto currLayer = getTech()->getLayer(i);
      if (currLayer->getType() != dbTechLayerType::ROUTING
          || (!currLayer->hasLef58CornerSpacingConstraint()
              && currLayer->getWidthTblOrthCon() == nullptr)) {
        continue;
      }
      for (auto& net : getNets()) {
        for (auto& pin : net->getPins(i)) {
          for (auto& corners : pin->getPolygonCorners()) {
            for (auto& corner : corners) {
              // LEF58 corner spacing
              if (currLayer->hasLef58CornerSpacingConstraint()) {
                checkMetalCornerSpacing_main(corner.get());
              }
              if (currLayer->getWidthTblOrthCon()) {
                checkWidthTableOrth(corner.get());
              }
            }
          }
        }
      }
    }
  }
}

void FlexGCWorker::Impl::checkWidthTableOrth_main(gcCorner* corner1,
                                                  gcCorner* corner2)
{
  if (corner2 == nullptr) {
    return;
  }
  if (corner1 == corner2) {
    return;
  }
  if (corner2->getType() != frCornerTypeEnum::CONCAVE) {
    return;
  }
  if (corner1->isFixed() && corner2->isFixed()) {
    return;
  }
  const auto layer_num = corner1->getLayerNum();
  const auto layer = getTech()->getLayer(layer_num);
  const auto con = layer->getWidthTblOrthCon();
  const frCoord horz_spc = con->getHorzSpc();
  const frCoord vert_spc = con->getVertSpc();

  odb::Rect marker_rect(corner1->x(), corner1->y(), corner2->x(), corner2->y());
  if (marker_rect.dx() >= horz_spc || marker_rect.dy() >= vert_spc) {
    return;
  }
  const auto owner = corner1->getPin()->getNet()->getOwner();
  auto marker = std::make_unique<frMarker>();
  marker->setBBox(marker_rect);
  marker->setLayerNum(layer_num);
  marker->setConstraint(con);
  marker->addSrc(owner);
  marker->addVictim(
      owner,
      std::make_tuple(
          layer_num,
          odb::Rect(corner1->x(), corner1->y(), corner1->x(), corner1->y()),
          corner1->isFixed()));
  marker->addAggressor(
      owner,
      std::make_tuple(
          layer_num,
          odb::Rect(corner2->x(), corner2->y(), corner2->x(), corner2->y()),
          corner2->isFixed()));
  addMarker(std::move(marker));
}

void FlexGCWorker::Impl::checkWidthTableOrth(gcCorner* corner)
{
  // applied only to inside corners (CONCAVE)
  if (corner->getType() != frCornerTypeEnum::CONCAVE) {
    return;
  }
  auto layer_num = corner->getLayerNum();
  auto layer = getTech()->getLayer(layer_num);
  auto con = layer->getWidthTblOrthCon();
  const frCoord horz_spc = con->getHorzSpc();
  const frCoord vert_spc = con->getVertSpc();
  odb::Rect query_rect(corner->x() - horz_spc,
                       corner->y() - vert_spc,
                       corner->x() + horz_spc,
                       corner->y() + vert_spc);
  std::vector<std::pair<segment_t, gcSegment*>> result;
  getWorkerRegionQuery().queryPolygonEdge(query_rect, layer_num, result);
  for (auto [_, edge] : result) {
    if (edge->getPin() != corner->getPin()) {
      continue;
    }
    checkWidthTableOrth_main(corner, edge->getLowCorner());
    checkWidthTableOrth_main(corner, edge->getHighCorner());
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
    using boost::polygon::operators::operator&;
    auto& fixedPolys = net->getPolygons(layerNum, true);
    auto intersection_fixedPolys = fixedPolys & rect;
    if (gtl::area(intersection_fixedPolys) == gtl::area(rect)) {
      return;
    }
  }

  auto marker = std::make_unique<frMarker>();
  odb::Rect box(gtl::xl(rect), gtl::yl(rect), gtl::xh(rect), gtl::yh(rect));
  marker->setBBox(box);
  marker->setLayerNum(layerNum);
  marker->setConstraint(getTech()->getLayer(layerNum)->getMinWidthConstraint());
  marker->addSrc(net->getOwner());
  marker->addVictim(net->getOwner(), std::make_tuple(layerNum, box, false));
  marker->addAggressor(net->getOwner(), std::make_tuple(layerNum, box, false));
  addMarker(std::move(marker));
}

void FlexGCWorker::Impl::checkMetalShape_minStep_helper(
    const odb::Rect& markerBox,
    frLayerNum layerNum,
    gcNet* net,
    frMinStepConstraint* con,
    bool hasInsideCorner,
    bool hasOutsideCorner,
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
  auto marker = std::make_unique<frMarker>();
  marker->setBBox(markerBox);
  marker->setLayerNum(layerNum);
  marker->setConstraint(con);
  marker->addSrc(net->getOwner());
  marker->addVictim(net->getOwner(),
                    std::make_tuple(layerNum, markerBox, false));
  marker->addAggressor(net->getOwner(),
                       std::make_tuple(layerNum, markerBox, false));
  addMarker(std::move(marker));
}
bool isConvex(gcSegment* s)
{
  return s->getLowCorner()->getType() == frCornerTypeEnum::CONVEX
         && s->getHighCorner()->getType() == frCornerTypeEnum::CONVEX;
}
gcSegment* bestSuitable(gcSegment* a, gcSegment* b)
{
  if (isConvex(a) && !isConvex(b)) {
    return a;
  }
  if (isConvex(b) && !isConvex(a)) {
    return b;
  }
  if (gtl::length(*a) > gtl::length(*b)) {
    return a;
  }
  return b;
}
void FlexGCWorker::Impl::checkMetalShape_minArea(gcPin* pin,
                                                 bool allow_patching)
{
  if (ignoreMinArea_) {
    return;
  }
  if (allow_patching && !targetNet_) {
    return;
  }
  auto poly = pin->getPolygon();
  auto layerNum = poly->getLayerNum();

  auto con = getTech()->getLayer(layerNum)->getAreaConstraint();

  if (!con) {
    return;
  }

  auto reqArea = con->getMinArea();
  auto actArea = gtl::area(*poly);

  if (actArea >= reqArea) {
    return;
  }
  gtl::rectangle_data<frCoord> bbox;
  gtl::extents(bbox, *pin->getPolygon());
  odb::Rect bbox2(gtl::xl(bbox), gtl::yl(bbox), gtl::xh(bbox), gtl::yh(bbox));
  if (!drcBox_.contains(bbox2)) {
    return;
  }
  for (auto& edges : pin->getPolygonEdges()) {
    for (auto& edge : edges) {
      if (edge->isFixed()) {
        return;
      }
    }
  }

  if (allow_patching) {
    checkMetalShape_addPatch(pin, reqArea);
  } else {
    auto net = poly->getNet();
    auto marker = std::make_unique<frMarker>();
    marker->setBBox(bbox2);
    marker->setLayerNum(layerNum);
    marker->setConstraint(con);
    marker->addSrc(net->getOwner());
    marker->addVictim(net->getOwner(), std::make_tuple(layerNum, bbox2, false));
    marker->addAggressor(net->getOwner(),
                         std::make_tuple(layerNum, bbox2, false));
    addMarker(std::move(marker));
  }
}

void FlexGCWorker::Impl::checkMetalShape_lef58MinStep_noBetweenEol(
    gcPin* pin,
    frLef58MinStepConstraint* con)
{
  auto poly = pin->getPolygon();
  auto layerNum = poly->getLayerNum();
  auto net = poly->getNet();

  std::vector<gcSegment*> startEdges;
  frCoord llx = 0;
  frCoord lly = 0;
  frCoord urx = 0;
  frCoord ury = 0;
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

    llx = std::min(llx, startEdge->high().x());
    lly = std::min(lly, startEdge->high().y());
    urx = std::max(urx, startEdge->high().x());
    ury = std::max(ury, startEdge->high().y());

    llx = std::min(llx, startEdge->getNextEdge()->high().x());
    lly = std::min(lly, startEdge->getNextEdge()->high().y());
    urx = std::max(urx, startEdge->getNextEdge()->high().x());
    ury = std::max(ury, startEdge->getNextEdge()->high().y());

    llx = std::min(llx, startEdge->getNextEdge()->getNextEdge()->high().x());
    lly = std::min(lly, startEdge->getNextEdge()->getNextEdge()->high().y());
    urx = std::max(urx, startEdge->getNextEdge()->getNextEdge()->high().x());
    ury = std::max(ury, startEdge->getNextEdge()->getNextEdge()->high().y());

    auto marker = std::make_unique<frMarker>();
    odb::Rect box(llx, lly, urx, ury);
    marker->setBBox(box);
    marker->setLayerNum(layerNum);
    marker->setConstraint(con);
    marker->addSrc(net->getOwner());
    marker->addVictim(net->getOwner(), std::make_tuple(layerNum, box, false));
    marker->addAggressor(net->getOwner(),
                         std::make_tuple(layerNum, box, false));
    addMarker(std::move(marker));
  }
}

inline void joinSegmentCoords(gcSegment* seg,
                              frCoord& llx,
                              frCoord& lly,
                              frCoord& urx,
                              frCoord& ury)
{
  llx = std::min(llx, seg->low().x());
  lly = std::min(lly, seg->low().y());
  urx = std::max(urx, seg->low().x());
  ury = std::max(ury, seg->low().y());

  llx = std::min(llx, seg->high().x());
  lly = std::min(lly, seg->high().y());
  urx = std::max(urx, seg->high().x());
  ury = std::max(ury, seg->high().y());
}

void FlexGCWorker::Impl::checkMetalShape_lef58MinStep_minAdjLength(
    gcPin* pin,
    frLef58MinStepConstraint* con)
{
  auto poly = pin->getPolygon();
  auto layerNum = poly->getLayerNum();
  auto net = poly->getNet();
  if (poly->size() == 4 && con->isExceptRectangle()) {
    return;
  }

  std::vector<gcSegment*> startEdges;
  auto minStepLength = con->getMinStepLength();
  for (auto& edges : pin->getPolygonEdges()) {
    // get the first edge that is >= minstep length
    for (auto& e : edges) {
      if (gtl::length(*e) < minStepLength) {
        startEdges.push_back(e.get());
      }
    }
  }
  for (auto startEdge : startEdges) {
    bool violating = false;
    auto nextEdge = startEdge->getNextEdge();
    auto prevEdge = startEdge->getPrevEdge();
    if (gtl::length(*(nextEdge)) < con->getMinAdjacentLength()
        || gtl::length(*(prevEdge)) < con->getMinAdjacentLength()) {
      violating = true;
    }
    if (con->getNoAdjEol() > -1) {
      if (nextEdge->getLowCorner()->getType() == frCornerTypeEnum::CONVEX
          && nextEdge->getHighCorner()->getType() == frCornerTypeEnum::CONVEX
          && gtl::length(*(nextEdge)) < con->getNoAdjEol()) {
        violating = true;
      }
      if (prevEdge->getLowCorner()->getType() == frCornerTypeEnum::CONVEX
          && prevEdge->getHighCorner()->getType() == frCornerTypeEnum::CONVEX
          && gtl::length(*(prevEdge)) < con->getNoAdjEol()) {
        violating = true;
      }
    }
    // skip if all edges are fixed
    if (startEdge->isFixed() && nextEdge->isFixed() && prevEdge->isFixed()) {
      continue;
    }
    if (!violating) {
      continue;
    }

    // real violation
    frCoord llx = startEdge->low().x();
    frCoord lly = startEdge->low().y();
    frCoord urx = startEdge->low().x();
    frCoord ury = startEdge->low().y();

    joinSegmentCoords(startEdge, llx, lly, urx, ury);
    joinSegmentCoords(nextEdge, llx, lly, urx, ury);
    joinSegmentCoords(prevEdge, llx, lly, urx, ury);

    auto marker = std::make_unique<frMarker>();
    odb::Rect box(llx, lly, urx, ury);
    marker->setBBox(box);
    marker->setLayerNum(layerNum);
    marker->setConstraint(con);
    marker->addSrc(net->getOwner());
    marker->addVictim(net->getOwner(), std::make_tuple(layerNum, box, false));
    marker->addAggressor(net->getOwner(),
                         std::make_tuple(layerNum, box, false));
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
    if (con->hasEolWidth()) {
      checkMetalShape_lef58MinStep_noBetweenEol(pin, con);
    }
    if (con->hasMinAdjacentLength()) {
      checkMetalShape_lef58MinStep_minAdjLength(pin, con);
    }
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
  odb::Rect markerBox;
  bool hasInsideCorner = false;
  bool hasOutsideCorner = false;
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
        markerBox.init(llx, lly, urx, ury);
        checkMetalShape_minStep_helper(markerBox,
                                       layerNum,
                                       net,
                                       con,
                                       hasInsideCorner,
                                       hasOutsideCorner,
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
  std::vector<gtl::rectangle_data<frCoord>> rects;
  gtl::polygon_90_set_data<frCoord> polySet;
  {
    using boost::polygon::operators::operator+=;
    polySet += *poly;
  }
  gtl::get_max_rectangles(rects, polySet);
  // rect only is true
  if (rects.size() == 1) {
    return;
  }
  // only show marker if fixed area does not contain marker area
  std::vector<gtl::point_data<frCoord>> concaveCorners;
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
      using boost::polygon::operators::operator&;
      auto intersectionPolySet = polySet & rect;
      auto intersectionFixedPolySet = intersectionPolySet & fixedPolys;
      if (gtl::area(intersectionFixedPolySet)
          == gtl::area(intersectionPolySet)) {
        continue;
      }

      std::vector<gtl::rectangle_data<frCoord>> maxRects;
      gtl::get_max_rectangles(maxRects, intersectionPolySet);
      for (auto& markerRect : maxRects) {
        auto marker = std::make_unique<frMarker>();
        odb::Rect box(gtl::xl(markerRect),
                      gtl::yl(markerRect),
                      gtl::xh(markerRect),
                      gtl::yh(markerRect));
        marker->setBBox(box);
        marker->setLayerNum(layerNum);
        marker->setConstraint(con);
        marker->addSrc(net->getOwner());
        marker->addVictim(net->getOwner(),
                          std::make_tuple(layerNum, box, false));
        marker->addAggressor(net->getOwner(),
                             std::make_tuple(layerNum, box, false));
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
      using boost::polygon::operators::operator&;
      auto intersection_polys = polys & markerRect;
      if (gtl::empty(intersection_polys)) {
        continue;
      }
      auto marker = std::make_unique<frMarker>();
      odb::Rect box(gtl::xl(markerRect),
                    gtl::yl(markerRect),
                    gtl::xh(markerRect),
                    gtl::yh(markerRect));
      marker->setBBox(box);
      marker->setLayerNum(layerNum);
      marker->setConstraint(
          getTech()->getLayer(layerNum)->getOffGridConstraint());
      marker->addSrc(net->getOwner());
      marker->addVictim(net->getOwner(), std::make_tuple(layerNum, box, false));
      marker->addAggressor(net->getOwner(),
                           std::make_tuple(layerNum, box, false));
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
          using boost::polygon::operators::operator&;
          auto intersection_polys = polys & (*poly);
          if (gtl::empty(intersection_polys)) {
            continue;
          }
          // create marker
          gtl::rectangle_data<frCoord> markerRect;
          gtl::extents(markerRect, hole_poly);

          auto marker = std::make_unique<frMarker>();
          odb::Rect box(gtl::xl(markerRect),
                        gtl::yl(markerRect),
                        gtl::xh(markerRect),
                        gtl::yh(markerRect));
          marker->setBBox(box);
          marker->setLayerNum(layerNum);
          marker->setConstraint(con);
          marker->addSrc(net->getOwner());
          marker->addVictim(net->getOwner(),
                            std::make_tuple(layerNum, box, false));
          marker->addAggressor(net->getOwner(),
                               std::make_tuple(layerNum, box, false));
          addMarker(std::move(marker));
        }
      }
    }
  }
}

void FlexGCWorker::Impl::checkMetalShape_lef58Area(gcPin* pin)
{
  if (ignoreMinArea_ || !targetNet_) {
    return;
  }

  auto poly = pin->getPolygon();
  auto layer_idx = poly->getLayerNum();
  auto layer = getTech()->getLayer(layer_idx);

  if (!layer->hasLef58AreaConstraint()) {
    return;
  }

  auto constraints = layer->getLef58AreaConstraints();

  auto sort_cmp
      = [](const frLef58AreaConstraint* a, const frLef58AreaConstraint* b) {
          auto a_rule = a->getODBRule();
          auto b_rule = b->getODBRule();

          return a_rule->getRectWidth() < b_rule->getRectWidth();
        };

  // sort constraints to ensure the smallest rect width will have
  // preference above other rect width statements
  std::ranges::sort(constraints, sort_cmp);

  bool check_rect_width = true;

  for (auto con : constraints) {
    odb::dbTechLayerAreaRule* db_rule = con->getODBRule();
    auto min_area = db_rule->getArea();
    auto curr_area = gtl::area(*poly);

    if (curr_area >= min_area) {
      continue;
    }

    gtl::rectangle_data<frCoord> bbox;
    gtl::extents(bbox, *pin->getPolygon());
    odb::Rect bbox2(gtl::xl(bbox), gtl::yl(bbox), gtl::xh(bbox), gtl::yh(bbox));
    if (!drWorker_->getDrcBox().contains(bbox2)) {
      continue;
    }
    for (auto& edges : pin->getPolygonEdges()) {
      for (auto& edge : edges) {
        if (edge->isFixed()) {
          continue;
        }
      }
    }

    if (checkMetalShape_lef58Area_exceptRectangle(poly, db_rule)) {
      // add patch only when the poly is not a rect
      checkMetalShape_addPatch(pin, min_area);
    } else if (check_rect_width
               && checkMetalShape_lef58Area_rectWidth(
                   poly, db_rule, check_rect_width)) {
      // add patch only if poly is rect and its width is less than or equal to
      // rectWidth value on constraint
      checkMetalShape_addPatch(pin, min_area);
    } else if (db_rule->getExceptMinWidth() != 0) {
      logger_->warn(
          DRT,
          311,
          "Unsupported branch EXCEPTMINWIDTH in PROPERTY LEF58_AREA.");
    } else if (db_rule->getExceptEdgeLength() != 0
               || db_rule->getExceptEdgeLengths()
                      != std::pair<int, int>(0, 0)) {
      logger_->warn(
          DRT,
          312,
          "Unsupported branch EXCEPTEDGELENGTH in PROPERTY LEF58_AREA.");
    } else if (db_rule->getExceptMinSize() != std::pair<int, int>(0, 0)) {
      logger_->warn(
          DRT, 313, "Unsupported branch EXCEPTMINSIZE in PROPERTY LEF58_AREA.");
    } else if (db_rule->getExceptStep() != std::pair<int, int>(0, 0)) {
      logger_->warn(
          DRT, 314, "Unsupported branch EXCEPTSTEP in PROPERTY LEF58_AREA.");
    } else if (db_rule->getMask() != 0) {
      logger_->warn(
          DRT, 315, "Unsupported branch MASK in PROPERTY LEF58_AREA.");
    } else if (db_rule->getTrimLayer() != nullptr) {
      logger_->warn(
          DRT, 316, "Unsupported branch LAYER in PROPERTY LEF58_AREA.");
    } else {
      checkMetalShape_addPatch(pin, min_area);
    }
  }
}

bool FlexGCWorker::Impl::checkMetalShape_lef58Area_exceptRectangle(
    gcPolygon* poly,
    odb::dbTechLayerAreaRule* db_rule)
{
  if (db_rule->isExceptRectangle()) {
    std::vector<gtl::rectangle_data<frCoord>> rects;
    gtl::polygon_90_set_data<frCoord> polySet;
    {
      using boost::polygon::operators::operator+=;
      polySet += *poly;
    }
    gtl::get_max_rectangles(rects, polySet);
    // rect only is true
    if (rects.size() == 1) {
      return false;
    }
    return true;
  }

  return false;
}

bool FlexGCWorker::Impl::checkMetalShape_lef58Area_rectWidth(
    gcPolygon* poly,
    odb::dbTechLayerAreaRule* db_rule,
    bool& check_rect_width)
{
  if (db_rule->getRectWidth() > 0) {
    std::vector<gtl::rectangle_data<frCoord>> rects;
    gtl::polygon_90_set_data<frCoord> polySet;
    {
      using boost::polygon::operators::operator+=;
      polySet += *poly;
    }
    gtl::get_max_rectangles(rects, polySet);
    if (rects.size() == 1) {
      int min_width = db_rule->getRectWidth();
      const auto& rect = rects.back();
      auto xLen = gtl::delta(rect, gtl::HORIZONTAL);
      auto yLen = gtl::delta(rect, gtl::VERTICAL);
      bool apply_rect_width_area = false;
      apply_rect_width_area = std::min(xLen, yLen) <= min_width;
      check_rect_width = !apply_rect_width_area;
      return apply_rect_width_area;
    }
    return false;
  }

  return false;
}

namespace gc_patch {
bool isPatchValid(drPatchWire* pwire, const odb::Rect& boundary_box)
{
  return pwire->getBBox().intersects(boundary_box)
         || boundary_box.intersects(pwire->getOrigin());
}
}  // namespace gc_patch

void FlexGCWorker::Impl::checkMetalShape_addPatch(gcPin* pin, int min_area)
{
  auto poly = pin->getPolygon();
  auto layer_idx = poly->getLayerNum();
  auto curr_area = gtl::area(*poly);

  // fix min area by adding patches
  frCoord gapArea = min_area - curr_area;
  bool prefDirIsVert = drWorker_->getDesign()->isVerticalLayer(layer_idx);
  gcSegment* chosenEdg = nullptr;
  // traverse polygon edges, searching for the best edge to amend a patch
  for (auto& edges : pin->getPolygonEdges()) {
    for (auto& e : edges) {
      if (e->isVertical() != prefDirIsVert
          && (!chosenEdg || bestSuitable(e.get(), chosenEdg) == e.get())) {
        chosenEdg = e.get();
      }
    }
  }
  frCoord length = ceil((float) gapArea / chosenEdg->length()
                        / getTech()->getManufacturingGrid())
                   * getTech()->getManufacturingGrid();
  odb::Rect patchBx;
  odb::Point offset;  // the lower left corner of the patch box
  if (prefDirIsVert) {
    patchBx.set_xlo(-chosenEdg->length() / 2);
    patchBx.set_xhi(chosenEdg->length() / 2);
    offset.setX((chosenEdg->low().x() + chosenEdg->high().x()) / 2);
    offset.setY(chosenEdg->low().y());
    if (chosenEdg->getOuterDir() == frDirEnum::N) {
      patchBx.set_yhi(length);
    } else if (chosenEdg->getOuterDir() == frDirEnum::S) {
      patchBx.set_ylo(-length);
    } else {
      logger_->error(
          DRT, 4500, "Edge outer dir should be either North or South");
    }
  } else {
    patchBx.set_ylo(-chosenEdg->length() / 2);
    patchBx.set_yhi(chosenEdg->length() / 2);
    offset.setX(chosenEdg->low().x());
    offset.setY((chosenEdg->low().y() + chosenEdg->high().y()) / 2);
    if (chosenEdg->getOuterDir() == frDirEnum::E) {
      patchBx.set_xhi(length);
    } else if (chosenEdg->getOuterDir() == frDirEnum::W) {
      patchBx.set_xlo(-length);
    } else {
      logger_->error(DRT, 4501, "Edge outer dir should be either East or West");
    }
  }
  auto patch = std::make_unique<drPatchWire>();
  patch->setLayerNum(layer_idx);
  patch->setOrigin(offset);
  patch->setOffsetBox(patchBx);

  // get drNet for patch
  gcNet* gc_net = pin->getNet();
  frNet* fr_net = gc_net->getFrNet();
  if (fr_net == nullptr) {
    logger_->error(DRT, 410, "frNet not found.");
  }

  const std::vector<drNet*>* dr_nets = drWorker_->getDRNets(fr_net);
  if (dr_nets == nullptr) {
    logger_->error(
        DRT, 411, "frNet {} does not have drNets.", fr_net->getName());
  }
  if (dr_nets->size() == 1) {
    patch->addToNet((*dr_nets)[0]);
  } else {
    // detect what drNet has objects overlapping with the patch
    checkMetalShape_patchOwner_helper(patch.get(), dr_nets);
  }
  if (!patch->hasNet()) {
    return;
  }

  if (!gc_patch::isPatchValid(patch.get(), getDRWorker()->getRouteBox())) {
    return;
  }
  pwires_.push_back(std::move(patch));
}

void FlexGCWorker::Impl::checkMetalShape_patchOwner_helper(
    drPatchWire* patch,
    const std::vector<drNet*>* dr_nets)
{
  odb::Rect patch_box = patch->getOffsetBox();
  for (drNet* dr_net : *dr_nets) {
    // check if patch overlaps with some rect of dr_net
    if (patch_box.overlaps(dr_net->getPinBox())) {
      patch->addToNet(dr_net);
      return;
    }

    for (auto&& conn_fig : dr_net->getExtConnFigs()) {
      if (patch_box.overlaps(conn_fig->getBBox())) {
        patch->addToNet(dr_net);
        return;
      }
    }

    for (auto&& conn_fig : dr_net->getRouteConnFigs()) {
      if (patch_box.overlaps(conn_fig->getBBox())) {
        patch->addToNet(dr_net);
        return;
      }
    }
  }
}

void FlexGCWorker::Impl::checkMetalShape_main(gcPin* pin, bool allow_patching)
{
  auto poly = pin->getPolygon();
  auto layerNum = poly->getLayerNum();
  auto net = poly->getNet();

  // min width
  std::vector<gtl::rectangle_data<frCoord>> rects;
  gtl::polygon_90_set_data<frCoord> polySet;
  {
    using boost::polygon::operators::operator+=;
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
  checkMetalShape_minArea(pin, allow_patching);

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

  // lef58 area
  if (allow_patching) {
    checkMetalShape_lef58Area(pin);
  }
}

void FlexGCWorker::Impl::checkMetalShape(bool allow_patching)
{
  if (targetNet_) {
    // layer --> net --> polygon
    for (int i = std::max(getTech()->getBottomLayerNum(), minLayerNum_);
         i <= std::min(getTech()->getTopLayerNum(), maxLayerNum_);
         i++) {
      auto currLayer = getTech()->getLayer(i);
      if (currLayer->getType() != dbTechLayerType::ROUTING) {
        continue;
      }
      for (auto& pin : targetNet_->getPins(i)) {
        checkMetalShape_main(pin.get(), allow_patching);
      }
    }
  } else {
    // layer --> net --> polygon
    for (int i = std::max(getTech()->getBottomLayerNum(), minLayerNum_);
         i <= std::min(getTech()->getTopLayerNum(), maxLayerNum_);
         i++) {
      auto currLayer = getTech()->getLayer(i);
      if (currLayer->getType() != dbTechLayerType::ROUTING) {
        continue;
      }
      for (auto& net : getNets()) {
        for (auto& pin : net->getPins(i)) {
          checkMetalShape_main(pin.get(), allow_patching);
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

  auto marker = std::make_unique<frMarker>();
  odb::Rect box(gtl::xl(markerRect),
                gtl::yl(markerRect),
                gtl::xh(markerRect),
                gtl::yh(markerRect));
  marker->setBBox(box);
  marker->setLayerNum(layerNum);
  marker->setConstraint(getTech()->getLayer(layerNum)->getShortConstraint());
  marker->addSrc(net1->getOwner());
  marker->addVictim(net1->getOwner(),
                    std::make_tuple(rect1->getLayerNum(),
                                    odb::Rect(gtl::xl(*rect1),
                                              gtl::yl(*rect1),
                                              gtl::xh(*rect1),
                                              gtl::yh(*rect1)),
                                    rect1->isFixed()));
  marker->addSrc(net2->getOwner());
  marker->addAggressor(net2->getOwner(),
                       std::make_tuple(rect2->getLayerNum(),
                                       odb::Rect(gtl::xl(*rect2),
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
    switch (owner->typeId()) {
      case frcNet: {
        if (static_cast<frNet*>(owner)->getType().isSupply()) {
          return;
        }
        break;
      }
      case frcBTerm: {
        if (static_cast<frBTerm*>(owner)->getType().isSupply()) {
          return;
        }
        break;
      }
      case frcInstTerm: {
        if (static_cast<frInstTerm*>(owner)->getTerm()->getType().isSupply()) {
          return;
        }
        break;
      }
      default:
        break;
    }
  }
  if (con->isParallelOverlap()) {
    // skip if no parallel overlap
    if (prl <= 0) {
      return;
      // skip if parallel overlap but shares the same above/below metal
    }
    box_t queryBox;
    myBloat(markerRect, 0, queryBox);
    auto& workerRegionQuery = getWorkerRegionQuery();
    std::vector<rq_box_value_t<gcRect*>> result;
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
      // TODO why isn't this auto-converted from odb::Rect to box_t?
      odb::Rect queryRect(queryBox.min_corner().get<0>(),
                          queryBox.min_corner().get<1>(),
                          queryBox.max_corner().get<0>(),
                          queryBox.max_corner().get<1>());
      if ((objPtr->getNet() == net1 || objPtr->getNet() == net2)
          && objBox.contains(queryRect)) {
        return;
      }
    }
  }
  // skip if not reaching area
  if (con->isArea() && gtl::area(*rect1) < con->getCutArea()
      && gtl::area(*rect2) < con->getCutArea()) {
    return;
  }

  // no violation if spacing satisfied
  frSquaredDistance reqSpcValSquare = con->getCutSpacing();
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

  auto marker = std::make_unique<frMarker>();
  odb::Rect box(gtl::xl(markerRect),
                gtl::yl(markerRect),
                gtl::xh(markerRect),
                gtl::yh(markerRect));
  marker->setBBox(box);
  marker->setLayerNum(layerNum);
  marker->setConstraint(con);
  marker->addSrc(net1->getOwner());
  marker->addVictim(net1->getOwner(),
                    std::make_tuple(rect1->getLayerNum(),
                                    odb::Rect(gtl::xl(*rect1),
                                              gtl::yl(*rect1),
                                              gtl::xh(*rect1),
                                              gtl::yh(*rect1)),
                                    rect1->isFixed()));
  marker->addSrc(net2->getOwner());
  marker->addAggressor(net2->getOwner(),
                       std::make_tuple(rect2->getLayerNum(),
                                       odb::Rect(gtl::xl(*rect2),
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
  frSquaredDistance reqSpcValSquare = con->getCutSpacing();
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

  auto marker = std::make_unique<frMarker>();
  odb::Rect box(gtl::xl(markerRect),
                gtl::yl(markerRect),
                gtl::xh(markerRect),
                gtl::yh(markerRect));
  marker->setBBox(box);
  marker->setLayerNum(layerNum);
  marker->setConstraint(con);
  marker->addSrc(net1->getOwner());
  marker->addVictim(net1->getOwner(),
                    std::make_tuple(rect1->getLayerNum(),
                                    odb::Rect(gtl::xl(*rect1),
                                              gtl::yl(*rect1),
                                              gtl::xh(*rect1),
                                              gtl::yh(*rect1)),
                                    rect1->isFixed()));
  marker->addSrc(net2->getOwner());
  marker->addAggressor(net2->getOwner(),
                       std::make_tuple(rect2->getLayerNum(),
                                       odb::Rect(gtl::xl(*rect2),
                                                 gtl::yl(*rect2),
                                                 gtl::xh(*rect2),
                                                 gtl::yh(*rect2)),
                                       rect2->isFixed()));
  addMarker(std::move(marker));
}

void FlexGCWorker::Impl::checkLef58CutSpacing_spc_parallelOverlap(
    gcRect* rect1,
    gcRect* rect2,
    frLef58CutSpacingConstraint* con,
    const gtl::rectangle_data<frCoord>& markerRect)
{
  // no violation if fixed shapes
  if (rect1->isFixed() && rect2->isFixed()) {
    return;
  }

  auto net1 = rect1->getNet();
  auto net2 = rect2->getNet();

  auto prlX = gtl::delta(markerRect, gtl::HORIZONTAL);
  auto prlY = gtl::delta(markerRect, gtl::VERTICAL);
  frCoord prl = std::max(prlX, prlY);

  // skip if no parallel overlap
  if (prl <= 0) {
    return;
  }

  box_t queryBox;
  myBloat(markerRect, 0, queryBox);
  auto& workerRegionQuery = getWorkerRegionQuery();
  std::vector<rq_box_value_t<gcRect*>> result;
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
    // TODO why isn't this auto-converted from odb::Rect to box_t?
    odb::Rect queryRect(queryBox.min_corner().get<0>(),
                        queryBox.min_corner().get<1>(),
                        queryBox.max_corner().get<0>(),
                        queryBox.max_corner().get<1>());
    // skip if parallel overlap but shares the same above/below metal
    if ((objPtr->getNet() == net1 || objPtr->getNet() == net2)
        && objBox.contains(queryRect)) {
      return;
    }
  }

  // no violation if spacing satisfied
  frSquaredDistance reqSpcValSquare = con->getCutSpacing();
  reqSpcValSquare *= reqSpcValSquare;

  frSquaredDistance distSquare = 0;
  if (con->isCenterToCenter()) {
    gtl::point_data<frCoord> center1, center2;
    gtl::center(center1, *rect1);
    gtl::center(center2, *rect2);
    distSquare = gtl::distance_squared(center1, center2);
  } else {
    distSquare = gtl::square_euclidean_distance(*rect1, *rect2);
  }
  if (distSquare >= reqSpcValSquare) {
    return;
  }

  auto marker = std::make_unique<frMarker>();
  auto layerNum = rect1->getLayerNum();
  odb::Rect box(gtl::xl(markerRect),
                gtl::yl(markerRect),
                gtl::xh(markerRect),
                gtl::yh(markerRect));
  marker->setBBox(box);
  marker->setLayerNum(layerNum);
  marker->setConstraint(con);
  marker->addSrc(net1->getOwner());
  marker->addVictim(net1->getOwner(),
                    std::make_tuple(rect1->getLayerNum(),
                                    odb::Rect(gtl::xl(*rect1),
                                              gtl::yl(*rect1),
                                              gtl::xh(*rect1),
                                              gtl::yh(*rect1)),
                                    rect1->isFixed()));
  marker->addSrc(net2->getOwner());
  marker->addAggressor(net2->getOwner(),
                       std::make_tuple(rect2->getLayerNum(),
                                       odb::Rect(gtl::xl(*rect2),
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
  } else if (con->isParallelOverlap()) {
    checkLef58CutSpacing_spc_parallelOverlap(rect1, rect2, con, markerRect);
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
  std::vector<rq_box_value_t<gcRect*>> result;
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
      if (con->isNoPrl()) {
        bool no_prl = (objBox.xMin() >= gtl::xh(*rect)
                       || objBox.xMax() <= gtl::xl(*rect))
                      && (objBox.yMin() >= gtl::yh(*rect)
                          || objBox.yMax() <= gtl::yl(*rect));
        // increment cnt when distSquare == 0 to account the current cut being
        // evaluated
        if (no_prl || distSquare == 0) {
          cnt++;
        }
      } else {
        cnt++;
      }
    }
  }
  if (cnt >= reqNumCut) {
    return true;
  }
  return false;
}

bool FlexGCWorker::Impl::checkLef58CutSpacing_spc_hasTwoCuts(
    gcRect* rect1,
    gcRect* rect2,
    frLef58CutSpacingConstraint* con)
{
  if (checkLef58CutSpacing_spc_hasTwoCuts_helper(rect1, con)
      && checkLef58CutSpacing_spc_hasTwoCuts_helper(rect2, con)) {
    return true;
  }
  return false;
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
  std::vector<rq_box_value_t<gcRect*>> result;
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
  }
  return false;
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
        " Unsupported branch EXACTALIGNED in checkLef58CutSpacing_spc_adjCut.");
    return;
  }
  if (con->isExceptSamePGNet()) {
    logger_->warn(DRT,
                  46,
                  " Unsupported branch EXCEPTSAMEPGNET in "
                  "checkLef58CutSpacing_spc_adjCut.");
    return;
  }
  if (con->hasExceptAllWithin()) {
    logger_->warn(DRT,
                  47,
                  " Unsupported branch EXCEPTALLWITHIN in "
                  "checkLef58CutSpacing_spc_adjCut.");
    return;
  }
  if (con->isToAll()) {
    logger_->warn(
        DRT,
        48,
        " Unsupported branch TO ALL in checkLef58CutSpacing_spc_adjCut.");
    return;
  }
  if (con->hasEnclosure()) {
    logger_->warn(
        DRT,
        50,
        " Unsupported branch ENCLOSURE in checkLef58CutSpacing_spc_adjCut.");
    return;
  }
  if (con->isSideParallelOverlap()) {
    logger_->warn(DRT,
                  51,
                  " Unsupported branch SIDEPARALLELOVERLAP in "
                  "checkLef58CutSpacing_spc_adjCut.");
    return;
  }
  if (con->isSameMask()) {
    logger_->warn(
        DRT,
        52,
        " Unsupported branch SAMEMASK in checkLef58CutSpacing_spc_adjCut.");
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

  frSquaredDistance reqSpcValSquare = con->getCutSpacing();
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

  auto marker = std::make_unique<frMarker>();
  odb::Rect box(gtl::xl(markerRect),
                gtl::yl(markerRect),
                gtl::xh(markerRect),
                gtl::yh(markerRect));
  marker->setBBox(box);
  marker->setLayerNum(layerNum);
  marker->setConstraint(con);
  marker->addSrc(net1->getOwner());
  marker->addVictim(net1->getOwner(),
                    std::make_tuple(rect1->getLayerNum(),
                                    odb::Rect(gtl::xl(*rect1),
                                              gtl::yl(*rect1),
                                              gtl::xh(*rect1),
                                              gtl::yh(*rect1)),
                                    rect1->isFixed()));
  marker->addSrc(net2->getOwner());
  marker->addAggressor(net2->getOwner(),
                       std::make_tuple(rect2->getLayerNum(),
                                       odb::Rect(gtl::xl(*rect2),
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
        DRT, 54, "Unsupported branch STACK in checkLef58CutSpacing_spc_layer.");
    return;
  }
  if (con->hasOrthogonalSpacing()) {
    logger_->warn(DRT,
                  55,
                  "Unsupported branch ORTHOGONALSPACING in "
                  "checkLef58CutSpacing_spc_layer.");
    return;
  }
  if (con->hasCutClass()) {
    if (con->isShortEdgeOnly()) {
      logger_->warn(DRT,
                    56,
                    "Unsupported branch SHORTEDGEONLY in "
                    "checkLef58CutSpacing_spc_layer.");
      return;
    }
    if (con->isConcaveCorner()) {
      if (con->hasWidth()) {
        logger_->warn(
            DRT,
            57,
            "Unsupported branch WIDTH in checkLef58CutSpacing_spc_layer.");
      } else if (con->hasParallel()) {
        logger_->warn(
            DRT,
            58,
            "Unsupported branch PARALLEL in checkLef58CutSpacing_spc_layer.");
      } else if (con->hasEdgeLength()) {
        logger_->warn(
            DRT,
            59,
            "Unsupported branch EDGELENGTH in checkLef58CutSpacing_spc_layer.");
      }
    } else if (con->hasExtension()) {
      logger_->warn(
          DRT,
          60,
          "Unsupported branch EXTENSION in checkLef58CutSpacing_spc_layer.");
    } else if (con->hasNonEolConvexCorner()) {
      ;
    } else if (con->hasAboveWidth()) {
      logger_->warn(
          DRT,
          61,
          "Unsupported branch ABOVEWIDTH in checkLef58CutSpacing_spc_layer.");
    } else if (con->isMaskOverlap()) {
      logger_->warn(
          DRT,
          62,
          "Unsupported branch MASKOVERLAP in checkLef58CutSpacing_spc_layer.");
    } else if (con->isWrongDirection()) {
      logger_->warn(DRT,
                    63,
                    "Unsupported branch WRONGDIRECTION in "
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
      std::vector<std::pair<segment_t, gcSegment*>> results;
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
        auto marker = std::make_unique<frMarker>();
        odb::Rect box(gtl::xl(markerRect),
                      gtl::yl(markerRect),
                      gtl::xh(markerRect),
                      gtl::yh(markerRect));
        marker->setBBox(box);
        marker->setLayerNum(layerNum);
        marker->setConstraint(con);
        marker->addSrc(net1->getOwner());
        marker->addVictim(net1->getOwner(),
                          std::make_tuple(layerNum,
                                          odb::Rect(gtl::xl(*rect1),
                                                    gtl::yl(*rect1),
                                                    gtl::xh(*rect1),
                                                    gtl::yh(*rect1)),
                                          rect1->isFixed()));
        marker->addSrc(net2->getOwner());
        marker->addAggressor(
            net2->getOwner(),
            std::make_tuple(
                secondLayerNum,
                odb::Rect(corner->x(), corner->y(), corner->x(), corner->y()),
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
      std::vector<std::pair<segment_t, gcSegment*>> results;
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
            // not EOL if minLength is not satisfied
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
          edgeX = std::min(edgeX, int(gtl::length(*(corner->getPrevEdge()))));
          edgeY = std::min(edgeY, int(gtl::length(*(corner->getNextEdge()))));
        } else {
          edgeX = std::min(edgeX, int(gtl::length(*(corner->getNextEdge()))));
          edgeY = std::min(edgeY, int(gtl::length(*(corner->getPrevEdge()))));
        }
        // outside of keepout zone
        if (edgeX * dy + edgeY * dx >= static_cast<uint64_t>(edgeX) * edgeY) {
          continue;
        }

        if (rect1->isFixed() && corner->isFixed()) {
          continue;
        }

        // this should only happen between samenet
        auto marker = std::make_unique<frMarker>();
        odb::Rect box(gtl::xl(markerRect),
                      gtl::yl(markerRect),
                      gtl::xh(markerRect),
                      gtl::yh(markerRect));
        marker->setBBox(box);
        marker->setLayerNum(layerNum);
        marker->setConstraint(con);
        marker->addSrc(net1->getOwner());
        marker->addVictim(net1->getOwner(),
                          std::make_tuple(layerNum,
                                          odb::Rect(gtl::xl(*rect1),
                                                    gtl::yl(*rect1),
                                                    gtl::xh(*rect1),
                                                    gtl::yh(*rect1)),
                                          rect1->isFixed()));
        marker->addSrc(net2->getOwner());
        marker->addAggressor(
            net2->getOwner(),
            std::make_tuple(
                secondLayerNum,
                odb::Rect(corner->x(), corner->y(), corner->x(), corner->y()),
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
  if (rect->getNet()->isBlockage() && rect->width() > int(layer->getWidth())) {
    return true;
  }

  frSquaredDistance cutWithinSquare = con->getCutWithin();
  box_t queryBox;
  myBloat(*rect, cutWithinSquare, queryBox);
  cutWithinSquare *= cutWithinSquare;
  auto& workerRegionQuery = getWorkerRegionQuery();
  std::vector<rq_box_value_t<gcRect*>> result;
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
    if (ptr->getNet()->isBlockage() && ptr->width() > int(layer->getWidth())) {
      cnt += reqNumCut;
    } else {
      cnt++;
    }
  }
  return cnt >= reqNumCut;
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
  std::vector<rq_box_value_t<gcRect*>> result;
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
  std::vector<rq_box_value_t<gcRect*>> result;
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
  auto layer = getTech()->getLayer(layerNum);
  // CShort
  // diff net same layer
  for (auto con : layer->getCutSpacing(false)) {
    checkCutSpacing_main(rect, con);
  }
  // same net same layer
  for (auto con : layer->getCutSpacing(true)) {
    checkCutSpacing_main(rect, con);
  }
  // diff net diff layer
  for (auto con : layer->getInterLayerCutSpacingConstraint(false)) {
    if (con) {
      checkCutSpacing_main(rect, con);
    }
  }

  // LEF58_SPACING for cut layer
  bool skipDiffNet = false;
  // samenet rule
  for (auto con : layer->getLef58CutSpacingConstraints(true)) {
    // skipSameNet if same-net rule exists
    skipDiffNet = true;
    checkLef58CutSpacing_main(rect, con, false);
  }
  // diffnet rule
  for (auto con : layer->getLef58CutSpacingConstraints(false)) {
    checkLef58CutSpacing_main(rect, con, skipDiffNet);
  }

  for (auto con : layer->getKeepOutZoneConstraints()) {
    checKeepOutZone_main(rect, con);
  }

  // LEF58_SPACINGTABLE
  if (layer->hasLef58SameMetalCutSpcTblConstraint()) {
    checkLef58CutSpacingTbl(rect,
                            layer->getLef58SameMetalCutSpcTblConstraint());
  }
  if (layer->hasLef58SameNetCutSpcTblConstraint()) {
    checkLef58CutSpacingTbl(rect, layer->getLef58SameNetCutSpcTblConstraint());
  }
  if (layer->hasLef58DiffNetCutSpcTblConstraint()) {
    checkLef58CutSpacingTbl(rect, layer->getLef58DiffNetCutSpcTblConstraint());
  }
  if (layer->hasLef58SameNetInterCutSpcTblConstraint()) {
    checkLef58CutSpacingTbl(rect,
                            layer->getLef58SameNetInterCutSpcTblConstraint());
  }
  if (layer->hasLef58SameMetalInterCutSpcTblConstraint()) {
    checkLef58CutSpacingTbl(rect,
                            layer->getLef58SameMetalInterCutSpcTblConstraint());
  }
  if (layer->hasLef58DefaultInterCutSpcTblConstraint()) {
    checkLef58CutSpacingTbl(rect,
                            layer->getLef58DefaultInterCutSpcTblConstraint());
  }
  if (layer->getLayerNum() + 2 < router_cfg_->TOP_ROUTING_LAYER
      && layer->getLayerNum() + 2 < getTech()->getLayers().size()) {
    auto aboveLayer = getTech()->getLayer(layer->getLayerNum() + 2);
    if (aboveLayer->hasLef58SameNetInterCutSpcTblConstraint()) {
      checkLef58CutSpacingTbl(
          rect, aboveLayer->getLef58SameNetInterCutSpcTblConstraint());
    }
    if (aboveLayer->hasLef58SameMetalInterCutSpcTblConstraint()) {
      checkLef58CutSpacingTbl(
          rect, aboveLayer->getLef58SameMetalInterCutSpcTblConstraint());
    }
    if (aboveLayer->hasLef58DefaultInterCutSpcTblConstraint()) {
      checkLef58CutSpacingTbl(
          rect, aboveLayer->getLef58DefaultInterCutSpcTblConstraint());
    }
  }
  if (layer->hasOrthSpacingTableConstraint()) {
    checkCutSpacingTableOrthogonal(rect);
  }
}

void FlexGCWorker::Impl::checkCutSpacing()
{
  if (targetNet_) {
    // layer --> net --> polygon --> maxrect
    for (int i = std::max(getTech()->getBottomLayerNum(), minLayerNum_);
         i <= std::min(getTech()->getTopLayerNum(), maxLayerNum_);
         i++) {
      auto currLayer = getTech()->getLayer(i);
      if (currLayer->getType() != dbTechLayerType::CUT) {
        continue;
      }
      for (auto& pin : targetNet_->getPins(i)) {
        for (auto& maxrect : pin->getMaxRectangles()) {
          checkCutSpacing_main(maxrect.get());
          checkLef58Enclosure_main(maxrect.get());
        }
      }
    }
  } else {
    // layer --> net --> polygon --> maxrect
    for (int i = std::max(getTech()->getBottomLayerNum(), minLayerNum_);
         i <= std::min(getTech()->getTopLayerNum(), maxLayerNum_);
         i++) {
      auto currLayer = getTech()->getLayer(i);
      if (currLayer->getType() != dbTechLayerType::CUT) {
        continue;
      }
      for (auto& net : getNets()) {
        for (auto& pin : net->getPins(i)) {
          for (auto& maxrect : pin->getMaxRectangles()) {
            checkCutSpacing_main(maxrect.get());
            checkLef58Enclosure_main(maxrect.get());
          }
        }
      }
    }
  }
}

void FlexGCWorker::Impl::patchMetalShape()
{
  patchMetalShape_minStep();

  checkMetalCornerSpacing();
  patchMetalShape_cornerSpacing();

  clearMarkers();
}

void FlexGCWorker::Impl::patchMetalShape_cornerSpacing()
{
  std::vector<drConnFig*> results;
  auto& workerRegionQuery = getDRWorker()->getWorkerRegionQuery();
  for (auto& marker : markers_) {
    results.clear();
    if (marker->getConstraint()->typeId()
        != frConstraintTypeEnum::frcLef58CornerSpacingConstraint) {
      continue;
    }
    const auto lNum = marker->getLayerNum();
    const auto layer = tech_->getLayer(lNum);

    odb::Point origin;
    odb::Rect fig_bbox;
    drNet* net = nullptr;
    odb::Rect markerBBox = marker->getBBox();
    workerRegionQuery.query(markerBBox, lNum, results);
    auto& sourceNets = marker->getSrcs();
    const odb::Rect routeBox = getDRWorker()->getRouteBox();
    drConnFig* obj = nullptr;
    for (auto connFig : results) {
      net = connFig->getNet();
      if (sourceNets.find(net->getFrNet()) == sourceNets.end()) {
        continue;
      }
      if (targetNet_ && net->getFrNet() != targetNet_->getFrNet()) {
        continue;
      }
      if (targetDRNet_ && net != targetDRNet_) {
        continue;
      }
      if (connFig->typeId() == drcVia) {
        auto via = static_cast<drVia*>(connFig);
        if (routeBox.intersects(via->getOrigin())) {
          origin = via->getOrigin();
          if (via->getViaDef()->getLayer1Num() == lNum) {
            fig_bbox = via->getLayer1BBox();
          } else {
            fig_bbox = via->getLayer2BBox();
          }
          obj = connFig;
          break;
        }
      } else if (connFig->typeId() == drcPathSeg) {
        auto seg = static_cast<drPathSeg*>(connFig);
        // Pick nearest of begin/end points
        const auto [bp, ep] = seg->getPoints();
        auto dist_bp
            = odb::Point::manhattanDistance(markerBBox.closestPtInside(bp), bp);
        auto dist_ep
            = odb::Point::manhattanDistance(markerBBox.closestPtInside(ep), ep);
        auto tmpOrigin = (dist_bp < dist_ep) ? bp : ep;
        if (routeBox.intersects(tmpOrigin)) {
          origin = tmpOrigin;
          fig_bbox = seg->getBBox();
          obj = connFig;
          break;
        }
      } else if (connFig->typeId() == drcPatchWire) {
        auto patch = static_cast<drPatchWire*>(connFig);
        if (routeBox.intersects(patch->getOrigin())) {
          origin = patch->getOrigin();
          fig_bbox = patch->getBBox();
          obj = connFig;
        }
      }
    }

    if (!obj) {
      continue;
    }

    auto mgrid = tech_->getManufacturingGrid();
    if (layer->isHorizontal()) {
      markerBBox.set_ylo(fig_bbox.yMin());
      markerBBox.set_yhi(fig_bbox.yMax());
      if (fig_bbox.xMin() == markerBBox.xMax()) {
        markerBBox.set_xlo(markerBBox.xMin() - mgrid);
      } else {
        markerBBox.set_xhi(markerBBox.xMax() + mgrid);
      }
    } else {
      markerBBox.set_xlo(fig_bbox.xMin());
      markerBBox.set_xhi(fig_bbox.xMax());
      if (fig_bbox.yMin() == markerBBox.yMax()) {
        markerBBox.set_ylo(markerBBox.yMin() - mgrid);
      } else {
        markerBBox.set_yhi(markerBBox.yMax() + mgrid);
      }
    }
    net = obj->getNet();
    markerBBox.moveDelta(-origin.x(), -origin.y());
    auto patch = std::make_unique<drPatchWire>();
    patch->setLayerNum(lNum);
    patch->setOrigin(origin);
    patch->setOffsetBox(markerBBox);
    patch->addToNet(net);
    if (!gc_patch::isPatchValid(patch.get(), getDRWorker()->getRouteBox())) {
      continue;
    }
    pwires_.push_back(std::move(patch));
  }
}

// Patch minStep markers for transition layers vias
void FlexGCWorker::Impl::patchMetalShape_minStep()
{
  std::vector<drConnFig*> results;
  for (auto& marker : markers_) {
    results.clear();
    if (marker->getConstraint()->typeId()
            != frConstraintTypeEnum::frcMinStepConstraint
        && marker->getConstraint()->typeId()
               != frConstraintTypeEnum::frcLef58MinStepConstraint) {
      continue;
    }
    auto lNum = marker->getLayerNum();
    auto layer = tech_->getLayer(lNum);
    if (!layer->hasVia2ViaMinStepViol()
        && !tech_->getLayer(lNum - 1)->hasLef58MaxSpacingConstraints()
        && (lNum + 1 >= tech_->getLayers().size()
            || !tech_->getLayer(lNum + 1)->hasLef58MaxSpacingConstraints())) {
      continue;
    }

    odb::Point origin;
    drNet* net = nullptr;
    auto& workerRegionQuery = getDRWorker()->getWorkerRegionQuery();
    odb::Rect markerBBox = marker->getBBox();
    if (markerBBox.maxDXDY() < (frCoord) layer->getWidth()) {
      continue;
    }
    workerRegionQuery.query(markerBBox, lNum, results);
    std::map<odb::Point, std::vector<drVia*>> vias;
    for (auto& connFig : results) {
      if (connFig->typeId() != drcVia) {
        continue;
      }
      auto obj = static_cast<drVia*>(connFig);
      if (obj->getNet()->getFrNet() != *(marker->getSrcs().begin())) {
        continue;
      }
      if (targetNet_ && obj->getNet()->getFrNet() != targetNet_->getFrNet()) {
        continue;
      }
      odb::Point tmpOrigin = obj->getOrigin();
      frLayerNum cutLayerNum = obj->getViaDef()->getCutLayerNum();
      if (cutLayerNum == lNum + 1 || cutLayerNum == lNum - 1) {
        vias[tmpOrigin].push_back(obj);
      }
    }
    for (auto& [tmpOrigin, objs] : vias) {
      bool upViaFound = false;
      bool downViaFound = false;
      for (auto obj : objs) {
        frLayerNum cutLayerNum = obj->getViaDef()->getCutLayerNum();
        if (cutLayerNum == lNum + 1) {
          upViaFound = true;
        } else {
          downViaFound = true;
        }
        if (upViaFound && downViaFound && layer->hasVia2ViaMinStepViol()) {
          net = obj->getNet();
          origin = tmpOrigin;
          break;
        }
        if (obj->isLonely() && getDRWorker()
            && getDRWorker()->getRipupMode() == RipUpMode::VIASWAP) {
          odb::Rect enc_box;
          if (obj->getViaDef()->getLayer1Num() == lNum) {
            enc_box = obj->getLayer1BBox();
          } else {
            enc_box = obj->getLayer2BBox();
          }
          int min_step_length = 0;
          if (layer->getMinStepConstraint()) {
            min_step_length = layer->getMinStepConstraint()->getMinStepLength();
          } else {
            continue;
          }
          if (enc_box.getDir() == odb::horizontal
              && markerBBox.yMin() == enc_box.yMin()
              && markerBBox.yMax() == enc_box.yMax()) {
            int bloating_dist = std::max(0, min_step_length - markerBBox.dx());
            if (markerBBox.xMin() >= enc_box.xMin()
                && markerBBox.xMax() == enc_box.xMax()) {
              markerBBox.set_xhi(markerBBox.xMax() + bloating_dist);
            } else if (markerBBox.xMin() == enc_box.xMin()
                       && markerBBox.xMax() <= enc_box.xMax()) {
              markerBBox.set_xlo(markerBBox.xMin() - bloating_dist);
            } else {
              continue;
            }
          } else if (enc_box.getDir() == odb::vertical
                     && markerBBox.xMin() == enc_box.xMin()
                     && markerBBox.xMax() == enc_box.xMax()) {
            int bloating_dist = std::max(0, min_step_length - markerBBox.dy());
            if (markerBBox.yMin() >= enc_box.yMin()
                && markerBBox.yMax() == enc_box.yMax()) {
              markerBBox.set_yhi(markerBBox.yMax() + bloating_dist);
            } else if (markerBBox.yMin() == enc_box.yMin()
                       && markerBBox.yMax() <= enc_box.yMax()) {
              markerBBox.set_ylo(markerBBox.yMin() - bloating_dist);
            } else {
              continue;
            }
          } else {
            continue;
          }
          origin = tmpOrigin;
          net = obj->getNet();
          break;
        }
      }
    }
    if (net == nullptr) {
      continue;
    }
    markerBBox.moveDelta(-origin.x(), -origin.y());
    auto patch = std::make_unique<drPatchWire>();
    patch->setLayerNum(lNum);
    patch->setOrigin(origin);
    patch->setOffsetBox(markerBBox);
    patch->addToNet(net);
    if (!gc_patch::isPatchValid(patch.get(), getDRWorker()->getRouteBox())) {
      continue;
    }
    pwires_.push_back(std::move(patch));
  }
}

void FlexGCWorker::Impl::checkMinimumCut_marker(gcRect* wideRect,
                                                gcRect* viaRect,
                                                frMinimumcutConstraint* con)
{
  auto net = wideRect->getNet();
  gtl::rectangle_data<frCoord> markerRect(*wideRect);
  gtl::generalized_intersect(markerRect, *viaRect);
  auto marker = std::make_unique<frMarker>();
  odb::Rect box(gtl::xl(markerRect),
                gtl::yl(markerRect),
                gtl::xh(markerRect),
                gtl::yh(markerRect));
  marker->setBBox(box);
  marker->setLayerNum(wideRect->getLayerNum());
  marker->setConstraint(con);
  marker->addSrc(net->getOwner());
  marker->addVictim(net->getOwner(),
                    std::make_tuple(wideRect->getLayerNum(),
                                    odb::Rect(gtl::xl(*wideRect),
                                              gtl::yl(*wideRect),
                                              gtl::xh(*wideRect),
                                              gtl::yh(*wideRect)),
                                    wideRect->isFixed()));
  marker->addSrc(net->getOwner());
  marker->addAggressor(net->getOwner(),
                       std::make_tuple(viaRect->getLayerNum(),
                                       odb::Rect(gtl::xl(*viaRect),
                                                 gtl::yl(*viaRect),
                                                 gtl::xh(*viaRect),
                                                 gtl::yh(*viaRect)),
                                       viaRect->isFixed()));
  addMarker(std::move(marker));
}

void FlexGCWorker::Impl::checkMinimumCut_main(gcRect* rect)
{
  auto layerNum = rect->getLayerNum();
  auto layer = getTech()->getLayer(layerNum);
  auto width = rect->width();
  auto length = rect->length();
  for (auto con : layer->getMinimumcutConstraints()) {
    if (width < con->getWidth()) {
      continue;
    }
    if (con->hasLength() && length < con->getLength()) {
      continue;
    }
    auto& workerRegionQuery = getWorkerRegionQuery();
    gtl::rectangle_data<frCoord> queryBox = *rect;
    if (con->hasLength()) {
      gtl::bloat(queryBox, con->getDistance());
    }
    std::vector<rq_box_value_t<gcRect*>> result;
    if (con->getConnection() != frMinimumcutConnectionEnum::FROMABOVE
        && layerNum > getTech()->getBottomLayerNum()) {
      std::vector<rq_box_value_t<gcRect*>> below_result;
      workerRegionQuery.queryMaxRectangle(queryBox, layerNum - 1, below_result);
      result.insert(result.end(), below_result.begin(), below_result.end());
    }
    if (con->getConnection() != frMinimumcutConnectionEnum::FROMBELOW
        && layerNum < getTech()->getTopLayerNum()) {
      std::vector<rq_box_value_t<gcRect*>> above_result;
      workerRegionQuery.queryMaxRectangle(queryBox, layerNum + 1, result);
      result.insert(result.end(), above_result.begin(), above_result.end());
    }

    odb::Rect wideRect(
        gtl::xl(*rect), gtl::yl(*rect), gtl::xh(*rect), gtl::yh(*rect));
    for (auto [viaBox, via] : result) {
      if (via->getNet() != rect->getNet()) {
        continue;
      }
      if (via->isFixed() && rect->isFixed()) {
        continue;
      }
      if (con->hasLength() && wideRect.contains(viaBox)) {
        continue;
      }
      if (!con->hasLength()) {
        checkMinimumCut_marker(rect, via, con);
        continue;
      }
      std::vector<rq_box_value_t<gcRect*>> encResult;
      workerRegionQuery.queryMaxRectangle(viaBox, layerNum, encResult);
      bool viol = false;

      for (auto [encBox, encObj] : encResult) {
        if (encObj->getNet() != via->getNet()) {
          continue;
        }

        if (encBox.intersects(viaBox) && encBox.intersects(wideRect)) {
          viol = true;
          break;
        }
      }
      if (viol) {
        checkMinimumCut_marker(rect, via, con);
      }
    }
  }
}

void FlexGCWorker::Impl::checkMinimumCut()
{
  if (targetNet_) {
    // layer --> net --> polygon --> maxrect
    for (int i = std::max(getTech()->getBottomLayerNum(), minLayerNum_);
         i <= std::min(getTech()->getTopLayerNum(), maxLayerNum_);
         i++) {
      auto currLayer = getTech()->getLayer(i);
      if (currLayer->getType() != dbTechLayerType::ROUTING) {
        continue;
      }
      if (!currLayer->hasMinimumcut()) {
        continue;
      }
      for (auto& pin : targetNet_->getPins(i)) {
        for (auto& maxrect : pin->getMaxRectangles()) {
          checkMinimumCut_main(maxrect.get());
        }
      }
    }
  } else {
    // layer --> net --> polygon --> maxrect
    for (int i = std::max(getTech()->getBottomLayerNum(), minLayerNum_);
         i <= std::min(getTech()->getTopLayerNum(), maxLayerNum_);
         i++) {
      auto currLayer = getTech()->getLayer(i);
      if (currLayer->getType() != dbTechLayerType::ROUTING) {
        continue;
      }
      if (!currLayer->hasMinimumcut()) {
        continue;
      }
      for (auto& net : getNets()) {
        for (auto& pin : net->getPins(i)) {
          for (auto& maxrect : pin->getMaxRectangles()) {
            checkMinimumCut_main(maxrect.get());
          }
        }
      }
    }
  }
}

void FlexGCWorker::Impl::checkTwoWiresForbiddenSpc_main(
    gcRect* rect,
    frLef58TwoWiresForbiddenSpcConstraint* con)
{
  bool isH = getTech()->getLayer(rect->getLayerNum())->getDir()
             == odb::dbTechLayerDir::HORIZONTAL;
  auto width1 = rect->width();
  bool validMinSpanLength1 = con->isValidForMinSpanLength(width1);
  bool validMaxSpanLength1 = con->isValidForMaxSpanLength(width1);
  if (!validMinSpanLength1 && !validMaxSpanLength1) {
    return;
  }
  box_t queryBox;
  myBloat(*rect, con->getODBRule()->getMaxSpacing(), queryBox);
  auto& workerRegionQuery = getWorkerRegionQuery();
  std::vector<rq_box_value_t<gcRect*>> result;
  workerRegionQuery.queryMaxRectangle(queryBox, rect->getLayerNum(), result);
  for (auto& [objBox, ptr] : result) {
    if (rect == ptr) {
      continue;
    }
    if (rect->isFixed() && ptr->isFixed()) {
      continue;
    }
    auto width2 = ptr->width();
    bool validMinSpanLength2 = con->isValidForMinSpanLength(width2);
    bool validMaxSpanLength2 = con->isValidForMaxSpanLength(width2);
    if (!(validMinSpanLength1 && validMaxSpanLength2)
        && !(validMaxSpanLength1 && validMinSpanLength2)) {
      continue;
    }
    gtl::rectangle_data<frCoord> markerRect(*rect);
    gtl::generalized_intersect(markerRect, *ptr);
    auto prlX = gtl::delta(markerRect, gtl::HORIZONTAL);
    auto prlY = gtl::delta(markerRect, gtl::VERTICAL);
    auto distX = gtl::euclidean_distance(*rect, *ptr, gtl::HORIZONTAL);
    auto distY = gtl::euclidean_distance(*rect, *ptr, gtl::VERTICAL);
    // skip short violations
    if (distX == 0 && distY == 0) {
      continue;
    }
    if (distX) {
      prlX = -prlX;
    }
    if (distY) {
      prlY = -prlY;
    }
    if (!con->isValidPrl(isH ? prlX : prlY)) {
      continue;
    }
    if (!con->isForbiddenSpacing(isH ? distY : distX)) {
      continue;
    }

    int type = 0;
    if (std::max(prlX, prlY) <= 0) {
      type = 2;
    } else {
      if (distX == 0) {
        type = 0;
      } else if (distY == 0) {
        type = 1;
      }
    }
    if (!checkMetalSpacing_prl_hasPolyEdge(
            rect, ptr, markerRect, type, std::max(prlX, prlY))) {
      continue;
      ;
    }
    if (!hasRoute(rect, markerRect) && !hasRoute(ptr, markerRect)) {
      continue;
    }
    // add violation
    auto marker = std::make_unique<frMarker>();
    odb::Rect box(gtl::xl(markerRect),
                  gtl::yl(markerRect),
                  gtl::xh(markerRect),
                  gtl::yh(markerRect));
    marker->setBBox(box);
    marker->setLayerNum(rect->getLayerNum());
    marker->setConstraint(con);
    marker->addSrc(rect->getNet()->getOwner());
    marker->addVictim(
        rect->getNet()->getOwner(),
        std::make_tuple(
            rect->getLayerNum(),
            odb::Rect(
                gtl::xl(*rect), gtl::yl(*rect), gtl::xh(*rect), gtl::yh(*rect)),
            rect->isFixed()));
    marker->addSrc(ptr->getNet()->getOwner());
    marker->addAggressor(
        ptr->getNet()->getOwner(),
        std::make_tuple(
            ptr->getLayerNum(),
            odb::Rect(
                gtl::xl(*ptr), gtl::yl(*ptr), gtl::xh(*ptr), gtl::yh(*ptr)),
            ptr->isFixed()));
    addMarker(std::move(marker));
  }
}

void FlexGCWorker::Impl::modifyMarkers()
{
  if (!surgicalFixEnabled_ || pwires_.empty()) {
    return;
  }
  for (auto& pwire : pwires_) {
    if (!pwire->hasNet()) {
      continue;
    }
    odb::Point origin = pwire->getOrigin();
    auto net = pwire->getNet()->getFrNet();
    for (auto& marker : markers_) {
      if (marker->getLayerNum() != pwire->getLayerNum()) {
        continue;
      }
      if (!marker->getBBox().intersects(pwire->getBBox())) {
        continue;
      }
      if (marker->getSrcs().find(net) == marker->getSrcs().end()) {
        continue;
      }
      if (marker->getBBox().intersects(origin)) {
        continue;
      }
      auto bbox = marker->getBBox();
      bbox.merge(odb::Rect(origin, origin));
      marker->setBBox(bbox);
    }
  }
}

int FlexGCWorker::Impl::main()
{
  // incremental updates
  pwires_.clear();
  clearMarkers();
  if (!modifiedDRNets_.empty()) {
    updateGCWorker();
  }
  if (surgicalFixEnabled_ && getDRWorker()) {
    checkMetalShape(true);
    //  minStep patching for GF14
    if (tech_->hasVia2ViaMinStep() || tech_->hasCornerSpacingConstraint()) {
      patchMetalShape();
    }
    if (!pwires_.empty()) {
      updateGCWorker();
    }
  }
  // clear existing markers
  clearMarkers();
  // check LEF58CornerSpacing and LEF58WidthTable ORTH
  checkMetalCornerSpacing();
  // check Short, NSMet, MetSpc based on max rectangles
  checkMetalSpacing();
  // check MinWid, MinStp, RectOnly based on polygon
  checkMetalShape(false);
  // check eolSpc based on polygon
  checkMetalEndOfLine();
  // check CShort, cutSpc, enclosure
  checkCutSpacing();
  // check SpacingTable Influence
  checkMetalSpacingTableInfluence();
  // check MINIMUMCUT
  checkMinimumCut();
  // check LEF58_METALWIDTHVIATABLE
  checkMetalWidthViaTable();
  // modify markers for pwires
  modifyMarkers();
  return 0;
}

}  // namespace drt
