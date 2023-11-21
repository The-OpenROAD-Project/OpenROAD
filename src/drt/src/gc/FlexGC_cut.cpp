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

#include "frProfileTask.h"
#include "gc/FlexGC_impl.h"
#include "odb/db.h"

using namespace fr;
typedef odb::dbTechLayerCutSpacingTableDefRule::LOOKUP_STRATEGY LOOKUP_STRATEGY;

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
    std::string cutClass1,
    std::string cutClass2,
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
  } else
    return false;
}

bool FlexGCWorker::Impl::checkLef58CutSpacingTbl_helper(
    gcRect* viaRect1,
    gcRect* viaRect2,
    frString class1,
    frString class2,
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
    } else {
      reqSpcSqr
          = dbRule->getSpacing(class1,
                               isSide1,
                               class2,
                               isSide2,
                               odb::dbTechLayerCutSpacingTableDefRule::MIN);
      reqSpcSqr *= reqSpcSqr;
      if (distSquare < reqSpcSqr)
        return true;
    }
    return false;
  }
  if (class1 == class2 && !dbRule->isLayerValid()) {
    bool exactlyAligned = false;
    if (dir == frDirEnum::N || dir == frDirEnum::S)
      exactlyAligned = (prl == deltaH1) && !dbRule->isHorizontal();
    else
      exactlyAligned = (prl == deltaV1) && !dbRule->isVertical();

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
    vector<pair<segment_t, gcSegment*>> results;
    auto& workerRegionQuery = getWorkerRegionQuery();
    workerRegionQuery.queryPolygonEdge(
        qb, viaRect2->getLayerNum() + 1, results);
    if (results.size() == 0)
      spcIdx = LOOKUP_STRATEGY::FIRST;
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
    if (c2cSquare < reqSpcSqr)
      return true;
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
  vector<rq_box_value_t<gcRect*>> results;
  auto& workerRegionQuery = getWorkerRegionQuery();
  workerRegionQuery.queryMaxRectangle(qb, viaRect1->getLayerNum() - 1, results);
  for (const auto& res : results) {
    auto metalRect = res.second;
    if (gtl::intersects(*metalRect, *viaRect1, false))
      if (gtl::intersects(*metalRect, *viaRect2, false))
        return true;
  }
  return false;
}

bool FlexGCWorker::Impl::checkLef58CutSpacingTbl_stacked(gcRect* viaRect1,
                                                         gcRect* viaRect2)
{
  if (*viaRect1 == *viaRect2)
    return true;
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
      if (viaRect1->getNet() != viaRect2->getNet())
        return;
      if (layer1->hasLef58SameMetalInterCutSpcTblConstraint()
          && checkLef58CutSpacingTbl_sameMetal(viaRect1, viaRect2))
        return;
    } else if (dbRule->isSameMetal()) {
      if (viaRect1->getNet() != viaRect2->getNet())
        return;
      if (!checkLef58CutSpacingTbl_sameMetal(viaRect1, viaRect2))
        return;
    } else {
      if (viaRect1->getNet() == viaRect2->getNet()) {
        return;
      }
    }
  } else {
    if (dbRule->isSameNet()) {
      if (viaRect1->getNet() != viaRect2->getNet())
        return;
    } else if (dbRule->isSameMetal()) {
      if (viaRect1->getNet() != viaRect2->getNet())
        return;
      if (!checkLef58CutSpacingTbl_sameMetal(viaRect1, viaRect2))
        return;
    } else {
      if (viaRect1->getNet() == viaRect2->getNet()) {
        if (layer1->hasLef58SameNetCutSpcTblConstraint())
          return;
        else if (checkLef58CutSpacingTbl_sameMetal(viaRect1, viaRect2)
                 && layer1->hasLef58SameMetalCutSpcTblConstraint())
          return;
      }
    }
  }
  auto cutClassIdx1
      = layer1->getCutClassIdx(viaRect1->width(), viaRect1->length());
  auto cutClassIdx2
      = layer2->getCutClassIdx(viaRect2->width(), viaRect2->length());
  frString class1 = "";
  frString class2 = "";
  if (cutClassIdx1 != -1)
    class1 = layer1->getCutClass(cutClassIdx1)->getName();
  if (cutClassIdx2 != -1)
    class2 = layer2->getCutClass(cutClassIdx2)->getName();

  gtl::rectangle_data<frCoord> markerRect(*viaRect1);
  gtl::generalized_intersect(markerRect, *viaRect2);
  frSquaredDistance distSquare
      = gtl::square_euclidean_distance(*viaRect1, *viaRect2);
  if (distSquare == 0) {
    if (dbRule->getMaxSpacing(class1, class2) == 0)
      return;
    if (!dbRule->isLayerValid())
      checkCutSpacing_short(viaRect1, viaRect2, markerRect);
    else if (dbRule->isNoStack())
      viol = true;
    else
      viol = !checkLef58CutSpacingTbl_stacked(viaRect1, viaRect2);
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
    auto marker = make_unique<frMarker>();
    Rect box(gtl::xl(markerRect),
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
        make_tuple(layerNum1, Rect(llx, lly, urx, ury), viaRect1->isFixed()));
    marker->addSrc(net2->getOwner());
    llx = gtl::xl(*viaRect2);
    lly = gtl::yl(*viaRect2);
    urx = gtl::xh(*viaRect2);
    ury = gtl::xh(*viaRect2);
    marker->addAggressor(
        net2->getOwner(),
        make_tuple(layerNum2, Rect(llx, lly, urx, ury), viaRect2->isFixed()));
    addMarker(std::move(marker));
  }
}

inline bool isSkipVia(gcRect* rect)
{
  return rect->getLayerNum() == GC_IGNORE_PDN_LAYER && rect->isFixed()
         && rect->hasNet() && rect->getNet()->getFrNet()
         && rect->getNet()->getFrNet()->getType().isSupply();
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
  frString cutClass = "";
  if (cutClassIdx != -1)
    cutClass = layer1->getCutClass(cutClassIdx)->getName();

  auto dbRule = con->getODBRule();
  if (isSkipVia(viaRect))
    return;

  bool isUpperVia = true;
  frLayerNum queryLayerNum;
  if (dbRule->isLayerValid()) {
    if (dbRule->getSecondLayer()->getName() == layer1->getName())
      isUpperVia = false;
    if (isUpperVia)
      queryLayerNum = getTech()
                          ->getLayer(dbRule->getSecondLayer()->getName())
                          ->getLayerNum();
    else
      queryLayerNum = getTech()
                          ->getLayer(dbRule->getTechLayer()->getName())
                          ->getLayerNum();
  } else
    queryLayerNum = layerNum1;
  frCoord maxSpc;

  if (width == length) {
    maxSpc = dbRule->getMaxSpacing(cutClass, false);
  } else {
    maxSpc = std::max(dbRule->getMaxSpacing(cutClass, true),
                      dbRule->getMaxSpacing(cutClass, false));
  }

  box_t queryBox;
  myBloat(*viaRect, maxSpc, queryBox);
  vector<rq_box_value_t<gcRect*>> results;
  auto& workerRegionQuery = getWorkerRegionQuery();
  workerRegionQuery.queryMaxRectangle(queryBox, queryLayerNum, results);
  for (auto& [box, ptr] : results) {
    if (ptr->isFixed() && viaRect->isFixed())
      continue;
    if (ptr->getPin() == viaRect->getPin())
      continue;
    if (isSkipVia(ptr))
      continue;
    if (isUpperVia)
      checkLef58CutSpacingTbl_main(viaRect, ptr, con);
    else
      checkLef58CutSpacingTbl_main(ptr, viaRect, con);
  }
}
void FlexGCWorker::Impl::checKeepOutZone_main(gcRect* rect,
                                              frLef58KeepOutZoneConstraint* con)
{
  auto layer = getTech()->getLayer(rect->getLayerNum());
  if (isSkipVia(rect))
    return;
  auto dbRule = con->getODBRule();
  Rect viaBox(gtl::xl(*rect), gtl::yl(*rect), gtl::xh(*rect), gtl::yh(*rect));
  Rect sideQueryBox(viaBox), endQueryBox(viaBox);
  auto viaCutClass = layer->getCutClass(rect->width(), rect->length());
  if (viaCutClass == nullptr
      || viaCutClass->getName() != dbRule->getFirstCutClass())
    return;

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
  vector<rq_box_value_t<gcRect*>> allResults;
  auto& workerRegionQuery = getWorkerRegionQuery();
  {
    vector<rq_box_value_t<gcRect*>> results;
    workerRegionQuery.queryMaxRectangle(
        sideQueryBox, layer->getLayerNum(), results);
    allResults.insert(allResults.end(), results.begin(), results.end());
  }
  {
    vector<rq_box_value_t<gcRect*>> results;
    workerRegionQuery.queryMaxRectangle(
        endQueryBox, layer->getLayerNum(), results);
    allResults.insert(allResults.end(), results.begin(), results.end());
  }
  for (auto& [box, ptr] : allResults) {
    if (ptr->isFixed() && rect->isFixed())
      continue;
    if (ptr->getPin() == rect->getPin())
      continue;
    if (isSkipVia(ptr))
      continue;
    auto via2CutClass = layer->getCutClass(ptr->width(), ptr->length());
    if (!dbRule->getSecondCutClass().empty()
        && (via2CutClass == nullptr
            || dbRule->getSecondCutClass() != via2CutClass->getName()))
      continue;
    odb::Rect ptrBox(
        gtl::xl(*ptr), gtl::yl(*ptr), gtl::xh(*ptr), gtl::yh(*ptr));
    if (!sideQueryBox.overlaps(ptrBox) && !endQueryBox.overlaps(ptrBox))
      continue;
    gtl::rectangle_data<frCoord> markerRect(*rect);
    gtl::generalized_intersect(markerRect, *ptr);
    Rect markerBox(gtl::xl(markerRect),
                   gtl::yl(markerRect),
                   gtl::xh(markerRect),
                   gtl::yh(markerRect));
    auto marker = make_unique<frMarker>();
    marker->setBBox(markerBox);
    marker->setLayerNum(layer->getLayerNum());
    marker->setConstraint(con);
    marker->addSrc(ptr->getNet()->getOwner());
    marker->addAggressor(
        ptr->getNet()->getOwner(),
        make_tuple(layer->getLayerNum(), ptrBox, ptr->isFixed()));
    marker->addSrc(rect->getNet()->getOwner());
    marker->addVictim(rect->getNet()->getOwner(),
                      make_tuple(rect->getLayerNum(), viaBox, rect->isFixed()));
    addMarker(std::move(marker));
  }
}

void FlexGCWorker::Impl::checkMetalWidthViaTable_main(gcRect* rect)
{
  for (auto con : getTech()
                      ->getLayer(rect->getLayerNum())
                      ->getMetalWidthViaConstraints()) {
    auto rule = con->getDbRule();
    auto required_viadef = getTech()->getVia(rule->getViaName());
    frVia required_via(required_viadef);
    if (rect->width() == required_via.getCutBBox().minDXDY()
        && rect->length() == required_via.getCutBBox().maxDXDY())
      continue;
    auto checkEnclosure =
        [this](gcRect* rect, odb::dbMetalWidthViaMap* rule, bool above) {
          vector<rq_box_value_t<gcRect*>> results;

          auto& workerRegionQuery = getWorkerRegionQuery();
          workerRegionQuery.queryMaxRectangle(
              *rect, rect->getLayerNum() + ((above) ? 1 : -1), results);
          gcRect* chosen_rect = nullptr;
          for (auto& [box, obj] : results) {
            if (!gtl::contains(*obj, *rect))
              continue;
            auto above_width = box.minDXDY();
            if (above_width >= ((above) ? rule->getAboveLayerWidthLow()
                                        : rule->getBelowLayerWidthLow())
                && above_width <= ((above) ? rule->getAboveLayerWidthHigh()
                                           : rule->getBelowLayerWidthHigh())) {
              chosen_rect = obj;
              if (!obj->isFixed())
                break;
            }
          }
          return chosen_rect;
        };

    // check above Metal Layer Width
    gcRect* above_rect = checkEnclosure(rect, rule, true);
    if (above_rect == nullptr)
      continue;

    // check below Metal Layer Width
    gcRect* below_rect = checkEnclosure(rect, rule, true);
    if (below_rect == nullptr)
      continue;
    if (below_rect->isFixed() && above_rect->isFixed() && rect->isFixed())
      continue;
    Rect markerBox(
        gtl::xl(*rect), gtl::yl(*rect), gtl::xh(*rect), gtl::yh(*rect));
    auto net1 = above_rect->getNet();
    auto net2 = below_rect->getNet();
    auto marker = make_unique<frMarker>();
    marker->setBBox(markerBox);
    marker->setLayerNum(rect->getLayerNum());
    marker->setConstraint(con);
    marker->addSrc(net1->getOwner());
    frCoord llx = gtl::xl(*above_rect);
    frCoord lly = gtl::yl(*above_rect);
    frCoord urx = gtl::xh(*above_rect);
    frCoord ury = gtl::xh(*above_rect);
    marker->addAggressor(net1->getOwner(),
                         make_tuple(above_rect->getLayerNum(),
                                    Rect(llx, lly, urx, ury),
                                    above_rect->isFixed()));
    marker->addSrc(net2->getOwner());
    llx = gtl::xl(*below_rect);
    lly = gtl::yl(*below_rect);
    urx = gtl::xh(*below_rect);
    ury = gtl::xh(*below_rect);
    marker->addAggressor(net2->getOwner(),
                         make_tuple(below_rect->getLayerNum(),
                                    Rect(llx, lly, urx, ury),
                                    below_rect->isFixed()));

    marker->addSrc(rect->getNet()->getOwner());
    marker->addVictim(
        rect->getNet()->getOwner(),
        make_tuple(rect->getLayerNum(), markerBox, rect->isFixed()));
    addMarker(std::move(marker));
    return;
  }
}

void FlexGCWorker::Impl::checkMetalWidthViaTable()
{
  if (targetNet_) {
    // layer --> net --> polygon --> maxrect
    for (int i = std::max((frLayerNum) (getTech()->getBottomLayerNum()),
                          minLayerNum_);
         i
         <= std::min((frLayerNum) (getTech()->getTopLayerNum()), maxLayerNum_);
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
    for (int i = std::max((frLayerNum) (getTech()->getBottomLayerNum()),
                          minLayerNum_);
         i
         <= std::min((frLayerNum) (getTech()->getTopLayerNum()), maxLayerNum_);
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
