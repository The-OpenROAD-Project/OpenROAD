// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <algorithm>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "boost/polygon/polygon.hpp"
#include "db/gcObj/gcShape.h"
#include "db/obj/frMarker.h"
#include "db/obj/frVia.h"
#include "db/tech/frConstraint.h"
#include "frBaseTypes.h"
#include "frProfileTask.h"
#include "gc/FlexGC.h"
#include "gc/FlexGC_impl.h"
#include "global.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"

using odb::dbTechLayerType;

namespace drt {
using LOOKUP_STRATEGY = odb::dbTechLayerCutSpacingTableDefRule::LOOKUP_STRATEGY;

inline frSquaredDistance getC2CDistSquare(
    const gtl::rectangle_data<frCoord>& rect1,
    const gtl::rectangle_data<frCoord>& rect2)
{
  gtl::point_data<frCoord> centerPt1, centerPt2;
  gtl::center(centerPt1, rect1);
  gtl::center(centerPt2, rect2);
  return gtl::distance_squared(centerPt1, centerPt2);
}

bool FlexGCWorker::Impl::checkLef58CutSpacingTbl_prlValid(
    const gtl::rectangle_data<frCoord>& viaRect1,
    const gtl::rectangle_data<frCoord>& viaRect2,
    const gtl::rectangle_data<frCoord>& markerRect,
    const std::string& cutClass1,
    const std::string& cutClass2,
    frCoord& prl,
    odb::dbTechLayerCutSpacingTableDefRule* dbRule)
{
  auto reqPrl = dbRule->getPrlEntry(cutClass1, cutClass2);

  // check for prl
  auto distX = gtl::euclidean_distance(viaRect1, viaRect2, gtl::HORIZONTAL);
  auto distY = gtl::euclidean_distance(viaRect1, viaRect2, gtl::VERTICAL);

  auto prlX = gtl::delta(markerRect, gtl::HORIZONTAL);
  auto prlY = gtl::delta(markerRect, gtl::VERTICAL);

  if (distX) {
    prlX = -prlX;
  }
  if (distY) {
    prlY = -prlY;
  }
  if (prlX > reqPrl || prlY > reqPrl) {
    prl = std::max(prlX, prlY);
    return true;
  }
  return false;
}

bool FlexGCWorker::Impl::checkLef58CutSpacingTbl_helper(
    gcRect* viaRect1,
    gcRect* viaRect2,
    const frString& class1,
    const frString& class2,
    const frDirEnum dir,
    frSquaredDistance distSquare,
    frSquaredDistance c2cSquare,
    bool prlValid,
    frCoord prl,
    odb::dbTechLayerCutSpacingTableDefRule* dbRule)
{
  bool isSide1, isSide2;
  auto deltaH1 = gtl::delta(*viaRect1, gtl::HORIZONTAL);
  auto deltaV1 = gtl::delta(*viaRect1, gtl::VERTICAL);
  auto deltaH2 = gtl::delta(*viaRect2, gtl::HORIZONTAL);
  auto deltaV2 = gtl::delta(*viaRect2, gtl::VERTICAL);
  frSquaredDistance reqSpcSqr;

  if (dir == frDirEnum::N || dir == frDirEnum::S) {
    isSide1 = deltaH1 > deltaV1;
    isSide2 = deltaH2 > deltaV2;
  } else {
    isSide1 = deltaV1 > deltaH1;
    isSide2 = deltaV2 > deltaH2;
  }

  if (dbRule->isNoPrl() && dbRule->isCenterAndEdge(class1, class2)) {
    reqSpcSqr = dbRule->getSpacing(class1,
                                   isSide1,
                                   class2,
                                   isSide2,
                                   odb::dbTechLayerCutSpacingTableDefRule::MAX);
    reqSpcSqr *= reqSpcSqr;
    if (c2cSquare < reqSpcSqr) {
      return true;
    }
    reqSpcSqr = dbRule->getSpacing(class1,
                                   isSide1,
                                   class2,
                                   isSide2,
                                   odb::dbTechLayerCutSpacingTableDefRule::MIN);
    reqSpcSqr *= reqSpcSqr;
    if (distSquare < reqSpcSqr) {
      return true;
    }
    return false;
  }
  if (class1 == class2 && !dbRule->isLayerValid()) {
    bool exactlyAligned = false;
    if (dir == frDirEnum::N || dir == frDirEnum::S) {
      exactlyAligned = (prl == deltaH1) && !dbRule->isHorizontal();
    } else {
      exactlyAligned = (prl == deltaV1) && !dbRule->isVertical();
    }

    auto exAlSpc = dbRule->getExactAlignedSpacing(class1);
    if (exactlyAligned && exAlSpc != -1) {
      reqSpcSqr = (frSquaredDistance) exAlSpc * exAlSpc;
      return distSquare < reqSpcSqr;
    }
  }
  LOOKUP_STRATEGY spcIdx
      = prlValid ? LOOKUP_STRATEGY::SECOND : LOOKUP_STRATEGY::FIRST;
  // check PRLALIGNED
  if (prlValid && dbRule->isPrlForAlignedCutClasses(class1, class2)) {
    gtl::rectangle_data<frCoord> edgeRect2;
    switch (dir) {
      case frDirEnum::S:
        gtl::set_points(edgeRect2, gtl::ul(*viaRect2), gtl::ur(*viaRect2));
        break;
      case frDirEnum::N:
        gtl::set_points(edgeRect2, gtl::ll(*viaRect2), gtl::lr(*viaRect2));
        break;
      case frDirEnum::E:
        gtl::set_points(edgeRect2, gtl::ll(*viaRect2), gtl::ul(*viaRect2));
        break;
      default:
        gtl::set_points(edgeRect2, gtl::lr(*viaRect2), gtl::ur(*viaRect2));
        break;
    }
    box_t qb(point_t(gtl::xl(edgeRect2), gtl::yl(edgeRect2)),
             point_t(gtl::xh(edgeRect2), gtl::yh(edgeRect2)));
    std::vector<std::pair<segment_t, gcSegment*>> results;
    auto& workerRegionQuery = getWorkerRegionQuery();
    workerRegionQuery.queryPolygonEdge(
        qb, viaRect2->getLayerNum() + 1, results);
    if (results.empty()) {
      spcIdx = LOOKUP_STRATEGY::FIRST;
    }
  }

  frCoord reqSpc = dbRule->getSpacing(class1, isSide1, class2, isSide2, spcIdx);
  reqSpcSqr = (frSquaredDistance) reqSpc * reqSpc;

  bool useCenter
      = dbRule->isCenterToCenter(class1, class2)
        || (dbRule->isCenterAndEdge(class1, class2)
            && reqSpc
                   == dbRule->getSpacing(
                       class1, isSide1, class2, isSide2, LOOKUP_STRATEGY::MAX));
  if (useCenter) {
    if (c2cSquare < reqSpcSqr) {
      return true;
    }
  } else if (distSquare < reqSpcSqr) {
    return true;
  }
  return false;
}

bool FlexGCWorker::Impl::checkLef58CutSpacingTbl_sameMetal(gcRect* viaRect1,
                                                           gcRect* viaRect2)
{
  box_t qb(point_t(gtl::xl(*viaRect1), gtl::yl(*viaRect1)),
           point_t(gtl::xh(*viaRect1), gtl::yh(*viaRect1)));
  std::vector<rq_box_value_t<gcRect*>> results;
  auto& workerRegionQuery = getWorkerRegionQuery();
  workerRegionQuery.queryMaxRectangle(qb, viaRect1->getLayerNum() - 1, results);
  for (const auto& res : results) {
    auto metalRect = res.second;
    if (gtl::intersects(*metalRect, *viaRect1, false)) {
      if (gtl::intersects(*metalRect, *viaRect2, false)) {
        return true;
      }
    }
  }
  return false;
}

bool FlexGCWorker::Impl::checkLef58CutSpacingTbl_stacked(gcRect* viaRect1,
                                                         gcRect* viaRect2)
{
  if (*viaRect1 == *viaRect2) {
    return true;
  }
  return gtl::contains(*viaRect1, *viaRect2)
         || gtl::contains(*viaRect2, *viaRect1);
}

void FlexGCWorker::Impl::checkLef58CutSpacingTbl_main(
    gcRect* viaRect1,
    gcRect* viaRect2,
    frLef58CutSpacingTableConstraint* con)
{
  auto dbRule = con->getODBRule();
  auto layerNum1 = viaRect1->getLayerNum();
  auto layer1 = getTech()->getLayer(layerNum1);
  auto layerNum2 = viaRect2->getLayerNum();
  auto layer2 = getTech()->getLayer(layerNum2);
  bool viol = false;
  if (dbRule->isLayerValid()) {
    if (dbRule->isSameNet()) {
      if (viaRect1->getNet() != viaRect2->getNet()) {
        return;
      }
      if (layer1->hasLef58SameMetalInterCutSpcTblConstraint()
          && checkLef58CutSpacingTbl_sameMetal(viaRect1, viaRect2)) {
        return;
      }
    } else if (dbRule->isSameMetal()) {
      if (viaRect1->getNet() != viaRect2->getNet()) {
        return;
      }
      if (!checkLef58CutSpacingTbl_sameMetal(viaRect1, viaRect2)) {
        return;
      }
    } else {
      if (viaRect1->getNet() == viaRect2->getNet()) {
        return;
      }
    }
  } else {
    if (dbRule->isSameNet()) {
      if (viaRect1->getNet() != viaRect2->getNet()) {
        return;
      }
    } else if (dbRule->isSameMetal()) {
      if (viaRect1->getNet() != viaRect2->getNet()) {
        return;
      }
      if (!checkLef58CutSpacingTbl_sameMetal(viaRect1, viaRect2)) {
        return;
      }
    } else {
      if (viaRect1->getNet() == viaRect2->getNet()) {
        if (layer1->hasLef58SameNetCutSpcTblConstraint()) {
          return;
        }
        if (checkLef58CutSpacingTbl_sameMetal(viaRect1, viaRect2)
            && layer1->hasLef58SameMetalCutSpcTblConstraint()) {
          return;
        }
      }
    }
  }
  auto cutClassIdx1
      = layer1->getCutClassIdx(viaRect1->width(), viaRect1->length());
  auto cutClassIdx2
      = layer2->getCutClassIdx(viaRect2->width(), viaRect2->length());
  frString class1;
  frString class2;
  if (cutClassIdx1 != -1) {
    class1 = layer1->getCutClass(cutClassIdx1)->getName();
  }
  if (cutClassIdx2 != -1) {
    class2 = layer2->getCutClass(cutClassIdx2)->getName();
  }

  gtl::rectangle_data<frCoord> markerRect(*viaRect1);
  gtl::generalized_intersect(markerRect, *viaRect2);
  frSquaredDistance distSquare
      = gtl::square_euclidean_distance(*viaRect1, *viaRect2);
  if (distSquare == 0) {
    if (dbRule->getMaxSpacing(class1, class2) == 0) {
      return;
    }
    if (!dbRule->isLayerValid()) {
      checkCutSpacing_short(viaRect1, viaRect2, markerRect);
    } else if (dbRule->isNoStack()) {
      viol = true;
    } else {
      viol = !checkLef58CutSpacingTbl_stacked(viaRect1, viaRect2);
    }
  }
  frSquaredDistance c2cSquare = getC2CDistSquare(*viaRect1, *viaRect2);

  bool isRight = gtl::xl(*viaRect2) > gtl::xh(*viaRect1);
  bool isLeft = gtl::xh(*viaRect2) < gtl::xl(*viaRect1);
  bool isUp = gtl::yl(*viaRect2) > gtl::yh(*viaRect1);
  bool isDown = gtl::yh(*viaRect2) < gtl::yl(*viaRect1);

  frCoord prl = -1;
  auto prlValid = checkLef58CutSpacingTbl_prlValid(
      *viaRect1, *viaRect2, markerRect, class1, class2, prl, dbRule);

  if (!viol && (isUp || isDown)) {
    frDirEnum secondToFirstDir = isUp ? frDirEnum::N : frDirEnum::S;
    viol = checkLef58CutSpacingTbl_helper(viaRect1,
                                          viaRect2,
                                          class1,
                                          class2,
                                          secondToFirstDir,
                                          distSquare,
                                          c2cSquare,
                                          prlValid,
                                          prl,
                                          dbRule);
  }
  if (!viol && (isRight || isLeft)) {
    frDirEnum secondToFirstDir = isRight ? frDirEnum::E : frDirEnum::W;
    viol = checkLef58CutSpacingTbl_helper(viaRect1,
                                          viaRect2,
                                          class1,
                                          class2,
                                          secondToFirstDir,
                                          distSquare,
                                          c2cSquare,
                                          prlValid,
                                          prl,
                                          dbRule);
  }

  if (viol) {
    // violation
    auto net1 = viaRect1->getNet();
    auto net2 = viaRect2->getNet();
    auto marker = std::make_unique<frMarker>();
    odb::Rect box(gtl::xl(markerRect),
                  gtl::yl(markerRect),
                  gtl::xh(markerRect),
                  gtl::yh(markerRect));
    marker->setBBox(box);
    marker->setLayerNum(layerNum1);
    marker->setConstraint(con);
    marker->addSrc(net1->getOwner());
    frCoord llx = gtl::xl(*viaRect1);
    frCoord lly = gtl::yl(*viaRect1);
    frCoord urx = gtl::xh(*viaRect1);
    frCoord ury = gtl::xh(*viaRect1);

    marker->addVictim(
        net1->getOwner(),
        std::make_tuple(
            layerNum1, odb::Rect(llx, lly, urx, ury), viaRect1->isFixed()));
    marker->addSrc(net2->getOwner());
    llx = gtl::xl(*viaRect2);
    lly = gtl::yl(*viaRect2);
    urx = gtl::xh(*viaRect2);
    ury = gtl::xh(*viaRect2);
    marker->addAggressor(
        net2->getOwner(),
        std::make_tuple(
            layerNum2, odb::Rect(llx, lly, urx, ury), viaRect2->isFixed()));
    addMarker(std::move(marker));
  }
}

inline bool isSupplyVia(gcRect* rect)
{
  return rect->isFixed() && rect->hasNet() && rect->getNet()->getFrNet()
         && rect->getNet()->getFrNet()->getType().isSupply();
}

inline bool isSkipVia(gcRect* rect, RouterConfiguration* router_cfg)
{
  return rect->getLayerNum() == router_cfg->GC_IGNORE_PDN_LAYER_NUM
         && isSupplyVia(rect);
}

inline bool isFixedVia(gcRect* rect, RouterConfiguration* router_cfg)
{
  if (rect->getLayerNum() == router_cfg->REPAIR_PDN_LAYER_NUM
      && isSupplyVia(rect)) {
    return false;
  }
  return rect->isFixed();
}

void FlexGCWorker::Impl::checkLef58CutSpacingTbl(
    gcRect* viaRect,
    frLef58CutSpacingTableConstraint* con)
{
  auto layerNum1 = viaRect->getLayerNum();
  auto layer1 = getTech()->getLayer(layerNum1);
  auto width = viaRect->width();
  auto length = viaRect->length();
  auto cutClassIdx = layer1->getCutClassIdx(width, length);
  frString cutClass;
  if (cutClassIdx != -1) {
    cutClass = layer1->getCutClass(cutClassIdx)->getName();
  }

  auto dbRule = con->getODBRule();
  if (isSkipVia(viaRect, router_cfg_)) {
    return;
  }

  bool isUpperVia = true;
  frLayerNum queryLayerNum;
  if (dbRule->isLayerValid()) {
    if (dbRule->getSecondLayer()->getName() == layer1->getName()) {
      isUpperVia = false;
    }
    if (isUpperVia) {
      queryLayerNum = getTech()
                          ->getLayer(dbRule->getSecondLayer()->getName())
                          ->getLayerNum();
    } else {
      queryLayerNum = getTech()
                          ->getLayer(dbRule->getTechLayer()->getName())
                          ->getLayerNum();
    }
  } else {
    queryLayerNum = layerNum1;
  }
  frCoord maxSpc;

  if (width == length) {
    maxSpc = dbRule->getMaxSpacing(std::move(cutClass), false);
  } else {
    maxSpc = std::max(dbRule->getMaxSpacing(cutClass, true),
                      dbRule->getMaxSpacing(cutClass, false));
  }

  box_t queryBox;
  myBloat(*viaRect, maxSpc, queryBox);
  std::vector<rq_box_value_t<gcRect*>> results;
  auto& workerRegionQuery = getWorkerRegionQuery();
  workerRegionQuery.queryMaxRectangle(queryBox, queryLayerNum, results);
  for (auto& [box, ptr] : results) {
    if (isFixedVia(ptr, router_cfg_) && isFixedVia(viaRect, router_cfg_)) {
      continue;
    }
    if (ptr->getPin() == viaRect->getPin()) {
      continue;
    }
    if (isSkipVia(ptr, router_cfg_)) {
      continue;
    }
    if (isUpperVia) {
      checkLef58CutSpacingTbl_main(viaRect, ptr, con);
    } else {
      checkLef58CutSpacingTbl_main(ptr, viaRect, con);
    }
  }
}
void FlexGCWorker::Impl::checKeepOutZone_main(gcRect* rect,
                                              frLef58KeepOutZoneConstraint* con)
{
  auto layer = getTech()->getLayer(rect->getLayerNum());
  if (isSkipVia(rect, router_cfg_)) {
    return;
  }
  auto dbRule = con->getODBRule();
  odb::Rect viaBox(
      gtl::xl(*rect), gtl::yl(*rect), gtl::xh(*rect), gtl::yh(*rect));
  odb::Rect sideQueryBox(viaBox), endQueryBox(viaBox);
  auto viaCutClass = layer->getCutClass(rect->width(), rect->length());
  if (viaCutClass == nullptr
      || viaCutClass->getName() != dbRule->getFirstCutClass()) {
    return;
  }

  if (viaBox.dx() > viaBox.dy()) {
    sideQueryBox = sideQueryBox.bloat(dbRule->getSideForwardExtension(),
                                      odb::Orientation2D::Horizontal);
    sideQueryBox = sideQueryBox.bloat(dbRule->getSideSideExtension(),
                                      odb::Orientation2D::Vertical);
    endQueryBox = endQueryBox.bloat(dbRule->getEndForwardExtension(),
                                    odb::Orientation2D::Vertical);
    endQueryBox = endQueryBox.bloat(dbRule->getEndSideExtension(),
                                    odb::Orientation2D::Horizontal);
  } else if (viaBox.dx() < viaBox.dy()) {
    sideQueryBox = sideQueryBox.bloat(dbRule->getSideForwardExtension(),
                                      odb::Orientation2D::Vertical);
    sideQueryBox = sideQueryBox.bloat(dbRule->getSideSideExtension(),
                                      odb::Orientation2D::Horizontal);
    endQueryBox = endQueryBox.bloat(dbRule->getEndForwardExtension(),
                                    odb::Orientation2D::Horizontal);
    endQueryBox = endQueryBox.bloat(dbRule->getEndSideExtension(),
                                    odb::Orientation2D::Vertical);
  } else {
    // skip non-rectangular vias
    return;
  }
  std::vector<rq_box_value_t<gcRect*>> allResults;
  auto& workerRegionQuery = getWorkerRegionQuery();
  {
    std::vector<rq_box_value_t<gcRect*>> results;
    workerRegionQuery.queryMaxRectangle(
        sideQueryBox, layer->getLayerNum(), results);
    allResults.insert(allResults.end(), results.begin(), results.end());
  }
  {
    std::vector<rq_box_value_t<gcRect*>> results;
    workerRegionQuery.queryMaxRectangle(
        endQueryBox, layer->getLayerNum(), results);
    allResults.insert(allResults.end(), results.begin(), results.end());
  }
  for (auto& [box, ptr] : allResults) {
    if (isFixedVia(ptr, router_cfg_) && isFixedVia(rect, router_cfg_)) {
      continue;
    }
    if (ptr->getPin() == rect->getPin()) {
      continue;
    }
    if (isSkipVia(ptr, router_cfg_)) {
      continue;
    }
    auto via2CutClass = layer->getCutClass(ptr->width(), ptr->length());
    if (!dbRule->getSecondCutClass().empty()
        && (via2CutClass == nullptr
            || dbRule->getSecondCutClass() != via2CutClass->getName())) {
      continue;
    }
    odb::Rect ptrBox(
        gtl::xl(*ptr), gtl::yl(*ptr), gtl::xh(*ptr), gtl::yh(*ptr));
    if (!sideQueryBox.overlaps(ptrBox) && !endQueryBox.overlaps(ptrBox)) {
      continue;
    }
    gtl::rectangle_data<frCoord> markerRect(*rect);
    gtl::generalized_intersect(markerRect, *ptr);
    odb::Rect markerBox(gtl::xl(markerRect),
                        gtl::yl(markerRect),
                        gtl::xh(markerRect),
                        gtl::yh(markerRect));
    auto marker = std::make_unique<frMarker>();
    marker->setBBox(markerBox);
    marker->setLayerNum(layer->getLayerNum());
    marker->setConstraint(con);
    marker->addSrc(ptr->getNet()->getOwner());
    marker->addAggressor(
        ptr->getNet()->getOwner(),
        std::make_tuple(layer->getLayerNum(), ptrBox, ptr->isFixed()));
    marker->addSrc(rect->getNet()->getOwner());
    marker->addVictim(
        rect->getNet()->getOwner(),
        std::make_tuple(rect->getLayerNum(), viaBox, rect->isFixed()));
    addMarker(std::move(marker));
  }
}

void FlexGCWorker::Impl::checkMetalWidthViaTable_main(gcRect* rect)
{
  if (rect->getLayerNum() > router_cfg_->TOP_ROUTING_LAYER) {
    return;
  }
  for (auto con : getTech()
                      ->getLayer(rect->getLayerNum())
                      ->getMetalWidthViaConstraints()) {
    auto rule = con->getDbRule();
    auto required_viadef = getTech()->getVia(rule->getViaName());
    frVia required_via(required_viadef);
    if (rect->width() == required_via.getCutBBox().minDXDY()
        && rect->length() == required_via.getCutBBox().maxDXDY()) {
      continue;
    }
    auto checkEnclosure =
        [this](gcRect* rect, odb::dbMetalWidthViaMap* rule, bool above) {
          std::vector<rq_box_value_t<gcRect*>> results;

          auto& workerRegionQuery = getWorkerRegionQuery();
          workerRegionQuery.queryMaxRectangle(
              *rect, rect->getLayerNum() + ((above) ? 1 : -1), results);
          gcRect* chosen_rect = nullptr;
          for (auto& [box, obj] : results) {
            if (!gtl::contains(*obj, *rect)) {
              continue;
            }
            auto above_width = box.minDXDY();
            if (above_width >= ((above) ? rule->getAboveLayerWidthLow()
                                        : rule->getBelowLayerWidthLow())
                && above_width <= ((above) ? rule->getAboveLayerWidthHigh()
                                           : rule->getBelowLayerWidthHigh())) {
              chosen_rect = obj;
              if (!obj->isFixed()) {
                break;
              }
            }
          }
          return chosen_rect;
        };

    // check above Metal Layer Width
    gcRect* above_rect = checkEnclosure(rect, rule, true);
    if (above_rect == nullptr) {
      continue;
    }

    // check below Metal Layer Width
    gcRect* below_rect = checkEnclosure(rect, rule, true);
    if (below_rect == nullptr) {
      continue;
    }
    if (below_rect->isFixed() && above_rect->isFixed() && rect->isFixed()) {
      continue;
    }
    odb::Rect markerBox(
        gtl::xl(*rect), gtl::yl(*rect), gtl::xh(*rect), gtl::yh(*rect));
    auto net1 = above_rect->getNet();
    auto net2 = below_rect->getNet();
    auto marker = std::make_unique<frMarker>();
    marker->setBBox(markerBox);
    marker->setLayerNum(rect->getLayerNum());
    marker->setConstraint(con);
    marker->addSrc(net1->getOwner());
    frCoord llx = gtl::xl(*above_rect);
    frCoord lly = gtl::yl(*above_rect);
    frCoord urx = gtl::xh(*above_rect);
    frCoord ury = gtl::xh(*above_rect);
    marker->addAggressor(net1->getOwner(),
                         std::make_tuple(above_rect->getLayerNum(),
                                         odb::Rect(llx, lly, urx, ury),
                                         above_rect->isFixed()));
    marker->addSrc(net2->getOwner());
    llx = gtl::xl(*below_rect);
    lly = gtl::yl(*below_rect);
    urx = gtl::xh(*below_rect);
    ury = gtl::xh(*below_rect);
    marker->addAggressor(net2->getOwner(),
                         std::make_tuple(below_rect->getLayerNum(),
                                         odb::Rect(llx, lly, urx, ury),
                                         below_rect->isFixed()));

    marker->addSrc(rect->getNet()->getOwner());
    marker->addVictim(
        rect->getNet()->getOwner(),
        std::make_tuple(rect->getLayerNum(), markerBox, rect->isFixed()));
    addMarker(std::move(marker));
    return;
  }
}

void FlexGCWorker::Impl::checkMetalWidthViaTable()
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
          checkMetalWidthViaTable_main(maxrect.get());
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
        // There is no need to check vias in nets we don't route
        auto fr_net = net->getFrNet();
        if (fr_net && (fr_net->isSpecial() || fr_net->getType().isSupply())) {
          continue;
        }
        for (auto& pin : net->getPins(i)) {
          for (auto& maxrect : pin->getMaxRectangles()) {
            checkMetalWidthViaTable_main(maxrect.get());
          }
        }
      }
    }
  }
}

void FlexGCWorker::Impl::checkLef58Enclosure_main(gcRect* viaRect,
                                                  gcRect* encRect)
{
  auto layer = getTech()->getLayer(viaRect->getLayerNum());
  auto cutClassIdx = layer->getCutClassIdx(viaRect->width(), viaRect->length());
  bool above = encRect->getLayerNum() > viaRect->getLayerNum();
  frCoord sideOverhang = 0;
  frCoord endOverhang = 0;
  sideOverhang = std::min(gtl::xh(*encRect) - gtl::xh(*viaRect),
                          gtl::xl(*viaRect) - gtl::xl(*encRect));
  endOverhang = std::min(gtl::yh(*encRect) - gtl::yh(*viaRect),
                         gtl::yl(*viaRect) - gtl::yl(*encRect));
  if (gtl::delta(*viaRect, gtl::orientation_2d_enum::HORIZONTAL)
      > gtl::delta(*viaRect, gtl::orientation_2d_enum::VERTICAL)) {
    std::swap(sideOverhang, endOverhang);
  }
  frLef58EnclosureConstraint* lastCon = nullptr;
  for (auto con : layer->getLef58EnclosureConstraints(
           cutClassIdx, encRect->width(), above)) {
    lastCon = con;
    if (con->isValidOverhang(endOverhang, sideOverhang)) {
      return;  // valid overhangs
    }
  }
  odb::Rect markerBox(gtl::xl(*viaRect),
                      gtl::yl(*viaRect),
                      gtl::xh(*viaRect),
                      gtl::yh(*viaRect));
  auto net = viaRect->getNet();
  auto marker = std::make_unique<frMarker>();
  marker->setBBox(markerBox);
  marker->setLayerNum(encRect->getLayerNum());
  marker->setConstraint(lastCon);
  marker->addSrc(net->getOwner());
  frCoord llx = gtl::xl(*encRect);
  frCoord lly = gtl::yl(*encRect);
  frCoord urx = gtl::xh(*encRect);
  frCoord ury = gtl::xh(*encRect);
  marker->addAggressor(net->getOwner(),
                       std::make_tuple(encRect->getLayerNum(),
                                       odb::Rect(llx, lly, urx, ury),
                                       encRect->isFixed()));
  llx = gtl::xl(*viaRect);
  lly = gtl::yl(*viaRect);
  urx = gtl::xh(*viaRect);
  ury = gtl::xh(*viaRect);
  marker->addVictim(net->getOwner(),
                    std::make_tuple(viaRect->getLayerNum(),
                                    odb::Rect(llx, lly, urx, ury),
                                    viaRect->isFixed()));
  marker->addSrc(net->getOwner());
  addMarker(std::move(marker));
}
void FlexGCWorker::Impl::checkLef58Enclosure_main(gcRect* rect)
{
  if (rect->isFixed()) {
    return;
  }
  auto layer = getTech()->getLayer(rect->getLayerNum());
  auto cutClassIdx = layer->getCutClassIdx(rect->width(), rect->length());
  bool hasAboveConstraints
      = layer->hasLef58EnclosureConstraint(cutClassIdx, true);
  bool hasBelowConstraints
      = layer->hasLef58EnclosureConstraint(cutClassIdx, false);
  if (!hasAboveConstraints && !hasBelowConstraints) {
    return;
  }
  auto getEnclosure = [this](gcRect* rect, frLayerNum layerNum) {
    std::vector<rq_box_value_t<gcRect*>> results;
    auto& workerRegionQuery = getWorkerRegionQuery();
    workerRegionQuery.queryMaxRectangle(*rect, layerNum, results);
    gcRect* encRect = nullptr;
    for (auto& [box, ptr] : results) {
      if (ptr->getNet() != rect->getNet()) {
        continue;
      }
      if (!gtl::contains(*ptr, *rect)) {
        continue;
      }
      if (encRect == nullptr) {
        encRect = ptr;
      } else if (ptr->width() > encRect->width()) {
        encRect = ptr;
      }
    }
    return encRect;
  };
  if (hasBelowConstraints) {
    gcRect* belowEnc = getEnclosure(rect, layer->getLayerNum() - 1);
    checkLef58Enclosure_main(rect, belowEnc);
  }
  if (hasAboveConstraints) {
    gcRect* aboveEnc = getEnclosure(rect, layer->getLayerNum() + 1);
    checkLef58Enclosure_main(rect, aboveEnc);
  }
}

namespace orth {
gtl::rectangle_data<frCoord> bloatRectangle(
    const gtl::rectangle_data<frCoord>& rect,
    const gtl::orientation_2d_enum dir,
    const frCoord spacing)
{
  gtl::rectangle_data<frCoord> temp_rect(rect);
  gtl::bloat(temp_rect, dir, spacing);
  return temp_rect;
}
gtl::polygon_90_set_data<frCoord> getQueryPolygonSet(
    const gtl::rectangle_data<frCoord>& marker_rect,
    const gtl::rectangle_data<frCoord>& rect1,
    const gtl::rectangle_data<frCoord>& rect2,
    const gtl::orientation_2d_enum dir,
    const frCoord spacing)
{
  gtl::polygon_90_set_data<frCoord> query_polygon_set;
  query_polygon_set.insert(bloatRectangle(marker_rect, dir, spacing));
  query_polygon_set.insert(bloatRectangle(rect1, dir, spacing));
  query_polygon_set.insert(bloatRectangle(rect2, dir, spacing));
  return query_polygon_set;
}

}  // namespace orth

void FlexGCWorker::Impl::checkCutSpacingTableOrthogonal_helper(
    gcRect* rect1,
    gcRect* rect2,
    const frCoord spacing)
{
  const auto [prl_x, prl_y] = getRectsPrl(rect1, rect2);
  if (std::max(prl_x, prl_y) <= 0) {  // no prl
    return;
  }
  if (std::min(prl_x, prl_y) > 0) {  // short
    return;
  }
  auto con = getTech()
                 ->getLayer(rect1->getLayerNum())
                 ->getOrthSpacingTableConstraint();
  // start checking orthogonal spacing
  // initialize query area
  gtl::rectangle_data<frCoord> prl_rect(*rect1);
  gtl::generalized_intersect(prl_rect, *rect2);
  gtl::polygon_90_set_data<frCoord> query_polygon_set
      = orth::getQueryPolygonSet(prl_rect,
                                 *rect1,
                                 *rect2,
                                 prl_x > 0 ? gtl::HORIZONTAL : gtl::VERTICAL,
                                 spacing);
  std::vector<gtl::rectangle_data<frCoord>> query_rects;
  gtl::get_max_rectangles(query_rects, query_polygon_set);
  std::vector<rq_box_value_t<gcRect*>> results;
  for (const auto& query_rect : query_rects) {
    getWorkerRegionQuery().queryMaxRectangle(
        query_rect, rect1->getLayerNum(), results);
  }
  for (const auto& [box, rect3] : results) {
    if (rect3 == rect1 || rect3 == rect2) {
      continue;
    }
    if (rect1->isFixed() && rect2->isFixed() && rect3->isFixed()) {
      continue;
    }
    bool is_violating = false;
    for (const auto& query_rect : query_rects) {
      if (gtl::intersects(query_rect, *rect3, false)) {
        is_violating = true;
        break;
      }
    }
    if (!is_violating) {
      continue;
    }
    // create violation
    gtl::rectangle_data<frCoord> marker_rect(prl_rect);
    gtl::generalized_intersect(marker_rect, *rect3);
    odb::Rect marker_box(gtl::xl(marker_rect),
                         gtl::yl(marker_rect),
                         gtl::xh(marker_rect),
                         gtl::yh(marker_rect));
    auto net = rect3->getNet();
    auto marker = std::make_unique<frMarker>();
    marker->setBBox(marker_box);
    marker->setLayerNum(rect3->getLayerNum());
    marker->setConstraint(con);
    marker->addAggressor(
        net->getOwner(),
        std::make_tuple(rect3->getLayerNum(), box, rect3->isFixed()));
    marker->addSrc(net->getOwner());

    frCoord llx = gtl::xl(*rect1);
    frCoord lly = gtl::yl(*rect1);
    frCoord urx = gtl::xh(*rect1);
    frCoord ury = gtl::xh(*rect1);
    net = rect1->getNet();
    marker->addVictim(net->getOwner(),
                      std::make_tuple(rect1->getLayerNum(),
                                      odb::Rect(llx, lly, urx, ury),
                                      rect1->isFixed()));
    marker->addSrc(net->getOwner());

    llx = gtl::xl(*rect2);
    lly = gtl::yl(*rect2);
    urx = gtl::xh(*rect2);
    ury = gtl::xh(*rect2);
    net = rect2->getNet();
    marker->addVictim(net->getOwner(),
                      std::make_tuple(rect2->getLayerNum(),
                                      odb::Rect(llx, lly, urx, ury),
                                      rect2->isFixed()));
    marker->addSrc(net->getOwner());

    addMarker(std::move(marker));
  }
}

void FlexGCWorker::Impl::checkCutSpacingTableOrthogonal(gcRect* rect)
{
  auto layer = getTech()->getLayer(rect->getLayerNum());
  auto con = layer->getOrthSpacingTableConstraint();
  for (const auto& [within, spacing] : con->getSpacingTable()) {
    box_t query_box;
    myBloat(*rect, within, query_box);
    std::vector<rq_box_value_t<gcRect*>> results;
    getWorkerRegionQuery().queryMaxRectangle(
        query_box, rect->getLayerNum(), results);
    for (auto& [box, via_rect] : results) {
      if (via_rect == rect) {
        continue;
      }
      checkCutSpacingTableOrthogonal_helper(rect, via_rect, spacing);
    }
  }
}
}  // namespace drt
