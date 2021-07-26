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
#include "opendb/db.h"
using namespace fr;

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
  if (prlX > reqPrl || prlY > reqPrl)
    return true;
  else
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
    odb::dbTechLayerCutSpacingTableDefRule* dbRule)
{
  bool isSide1, isSide2;
  auto deltaH1 = gtl::delta(*viaRect1, gtl::HORIZONTAL);
  auto deltaV1 = gtl::delta(*viaRect1, gtl::VERTICAL);
  auto deltaH2 = gtl::delta(*viaRect2, gtl::HORIZONTAL);
  auto deltaV2 = gtl::delta(*viaRect2, gtl::VERTICAL);
  frSquaredDistance reqSpcSqr;

  if (dir == frDirEnum::S || dir == frDirEnum::S) {
    isSide1 = deltaH1 > deltaV1;
    isSide2 = deltaH2 > deltaV2;
  } else {
    isSide1 = deltaV1 > deltaH1;
    isSide2 = deltaV2 > deltaH2;
  }

  if (dbRule->isNoPrl() && dbRule->isCenterAndEdge(class1, class2)) {
    reqSpcSqr = dbRule->getSpacing(class1, isSide1, class2, isSide2, 2);
    reqSpcSqr *= reqSpcSqr;
    if (c2cSquare < reqSpcSqr) {
      return true;
    } else {
      reqSpcSqr = dbRule->getSpacing(class1, isSide1, class2, isSide2, 3);
      reqSpcSqr *= reqSpcSqr;
      if (distSquare < reqSpcSqr)
        return true;
    }
    return false;
  }

  short spcIdx = prlValid ? 1 : 0;
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
      spcIdx = 0;
  }

  frCoord reqSpc = dbRule->getSpacing(class1, isSide1, class2, isSide2, spcIdx);
  reqSpcSqr = (frSquaredDistance) reqSpc * reqSpc;

  bool useCenter
      = dbRule->isCenterToCenter(class1, class2)
        || (dbRule->isCenterAndEdge(class1, class2)
            && reqSpc
                   == dbRule->getSpacing(class1, isSide1, class2, isSide2, 2));
  if (useCenter) {
    if (c2cSquare < reqSpcSqr)
      return true;
  } else if (distSquare < reqSpcSqr) {
    return true;
  }
  return false;
}
void FlexGCWorker::Impl::checkLef58CutSpacingTbl_main(
    gcRect* viaRect1,
    gcRect* viaRect2,
    frLef58CutSpacingTableConstraint* con)
{
  gtl::rectangle_data<frCoord> markerRect(*viaRect1);
  gtl::generalized_intersect(markerRect, *viaRect2);
  frSquaredDistance distSquare
      = gtl::square_euclidean_distance(*viaRect1, *viaRect2);
  if (distSquare == 0)
    checkCutSpacing_short(viaRect1, viaRect2, markerRect);

  frSquaredDistance c2cSquare = getC2CDistSquare(*viaRect1, *viaRect2);

  auto layerNum1 = viaRect1->getLayerNum();
  auto layer1 = getTech()->getLayer(layerNum1);
  auto layerNum2 = viaRect2->getLayerNum();
  auto layer2 = getTech()->getLayer(layerNum2);

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

  bool isRight = gtl::xl(*viaRect2) > gtl::xh(*viaRect1);
  bool isLeft = gtl::xh(*viaRect2) < gtl::xl(*viaRect1);
  bool isUp = gtl::yl(*viaRect2) > gtl::yh(*viaRect1);
  bool isDown = gtl::yh(*viaRect2) < gtl::yl(*viaRect1);

  auto dbRule = con->getODBRule();
  bool viol = false;
  auto prlValid = checkLef58CutSpacingTbl_prlValid(
      *viaRect1, *viaRect2, markerRect, class1, class2, dbRule);

  if (isUp || isDown) {
    frDirEnum secondToFirstDir = isUp ? frDirEnum::N : frDirEnum::S;
    viol = checkLef58CutSpacingTbl_helper(viaRect1,
                                          viaRect2,
                                          class1,
                                          class2,
                                          secondToFirstDir,
                                          distSquare,
                                          c2cSquare,
                                          prlValid,
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
                                          dbRule);
  }

  if (viol) {
    // violation
    auto net1 = viaRect1->getNet();
    auto net2 = viaRect2->getNet();
    auto marker = make_unique<frMarker>();
    frBox box(gtl::xl(markerRect),
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
        make_tuple(layerNum1, frBox(llx, lly, urx, ury), viaRect1->isFixed()));
    marker->addSrc(net2->getOwner());
    llx = gtl::xl(*viaRect2);
    lly = gtl::yl(*viaRect2);
    urx = gtl::xh(*viaRect2);
    ury = gtl::xh(*viaRect2);
    marker->addAggressor(
        net2->getOwner(),
        make_tuple(layerNum2, frBox(llx, lly, urx, ury), viaRect2->isFixed()));
    addMarker(std::move(marker));
  }
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
  frLayerNum queryLayerNum;
  if (dbRule->isLayerValid())
    queryLayerNum = getTech()
                        ->getLayer(dbRule->getSecondLayer()->getName())
                        ->getLayerNum();
  else
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
    if (dbRule->isLayerValid() && ptr->getNet() == viaRect->getNet()
        && !dbRule->isSameNet())
      continue;
    if (dbRule->isLayerValid() && ptr->getNet() != viaRect->getNet()
        && dbRule->isSameNet())
      continue;
    checkLef58CutSpacingTbl_main(viaRect, ptr, con);
  }
}