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

inline std::string getCutClass(frLayer* layer, gcSegment* viaEdge)
{
  auto width
      = std::min(gtl::length(*viaEdge), gtl::length(*(viaEdge->getPrevEdge())));
  auto length
      = std::max(gtl::length(*viaEdge), gtl::length(*(viaEdge->getPrevEdge())));
  return layer->getCutClass(width, length)->getName();
}

inline bool isSideEdge(gcSegment* viaEdge)
{
  return gtl::length(*viaEdge) > gtl::length(*(viaEdge->getPrevEdge()));
}

void FlexGCWorker::Impl::getLef58CutSpacingTblQueryBox(
    gcSegment* edge,
    frCoord spc,
    box_t& queryBox,
    gtl::rectangle_data<frCoord>& queryRect)
{
  if (edge->getDir() == frDirEnum::E) {
    bg::set<bg::min_corner, 0>(queryBox, edge->low().x() - spc);
    bg::set<bg::min_corner, 1>(queryBox, edge->low().y() - spc);
    bg::set<bg::max_corner, 0>(queryBox, edge->high().x() + spc);
    bg::set<bg::max_corner, 1>(queryBox, edge->high().y());
  } else if (edge->getDir() == frDirEnum::W) {
    bg::set<bg::min_corner, 0>(queryBox, edge->high().x() - spc);
    bg::set<bg::min_corner, 1>(queryBox, edge->high().y());
    bg::set<bg::max_corner, 0>(queryBox, edge->low().x() + spc);
    bg::set<bg::max_corner, 1>(queryBox, edge->low().y() + spc);
  } else if (edge->getDir() == frDirEnum::N) {
    bg::set<bg::min_corner, 0>(queryBox, edge->low().x());
    bg::set<bg::min_corner, 1>(queryBox, edge->low().y() - spc);
    bg::set<bg::max_corner, 0>(queryBox, edge->high().x() + spc);
    bg::set<bg::max_corner, 1>(queryBox, edge->high().y() + spc);
  } else {  // S
    bg::set<bg::min_corner, 0>(queryBox, edge->high().x() - spc);
    bg::set<bg::min_corner, 1>(queryBox, edge->high().y() - spc);
    bg::set<bg::max_corner, 0>(queryBox, edge->low().x());
    bg::set<bg::max_corner, 1>(queryBox, edge->low().y() + spc);
  }
  gtl::xl(queryRect, queryBox.min_corner().x());
  gtl::yl(queryRect, queryBox.min_corner().y());
  gtl::xh(queryRect, queryBox.max_corner().x());
  gtl::yh(queryRect, queryBox.max_corner().y());
}

inline frSquaredDistance getCenterToCenterDist(gcSegment* viaEdge1,
                                               gcSegment* viaEdge2)
{
  auto poly1 = viaEdge1->getPin()->getPolygon();
  auto poly2 = viaEdge2->getPin()->getPolygon();
  gtl::point_data<frCoord> centerPt1, centerPt2;
  gtl::center(centerPt1, *poly1);
  gtl::center(centerPt2, *poly2);
  return gtl::distance_squared(centerPt1, centerPt2);
}

void FlexGCWorker::Impl::checkLef58CutSpacingTbl_main(
    gcSegment* viaEdge1,
    gcSegment* viaEdge2,
    frLef58CutSpacingTableConstraint* con)
{
  auto layerNum1 = viaEdge1->getLayerNum();
  auto layer1 = getTech()->getLayer(layerNum1);
  auto layerNum2 = viaEdge2->getLayerNum();
  auto layer2 = getTech()->getLayer(layerNum2);
  auto class1 = getCutClass(layer1, viaEdge1);
  auto class2 = getCutClass(layer2, viaEdge2);
  auto isSide1 = isSideEdge(viaEdge1);
  auto isSide2 = isSideEdge(viaEdge2);
  gtl::rectangle_data<frCoord> rect1(viaEdge1->low().x(),
                                     viaEdge1->low().y(),
                                     viaEdge1->high().x(),
                                     viaEdge1->high().y());
  gtl::rectangle_data<frCoord> rect2(viaEdge2->low().x(),
                                     viaEdge2->low().y(),
                                     viaEdge2->high().x(),
                                     viaEdge2->high().y());
  auto dbRule = con->getODBRule();
  bool viol = false;
  if (dbRule->isNoPrl() && dbRule->isCenterAndEdge(class1, class2)) {
    frSquaredDistance spcSqr
        = dbRule->getSpacing(class1, isSide1, class2, isSide2, 2);
    spcSqr *= spcSqr;
    if (getCenterToCenterDist(viaEdge1, viaEdge2) < spcSqr) {
      viol = true;
    } else {
      spcSqr = dbRule->getSpacing(class1, isSide1, class2, isSide2, 3);
      spcSqr *= spcSqr;
      if ((frSquaredDistance) gtl::square_euclidean_distance(rect1, rect2)
          < spcSqr)
        viol = true;
    }
  } else {
    short strategy = 0;  // first spacing value
    auto reqPrl = dbRule->getPrlEntry(class1, class2);
    // check if parallel
    if (viaEdge1->getDir() == viaEdge2->getDir()
        || (int) viaEdge1->getDir() + (int) viaEdge2->getDir() != OPPOSITEDIR) {
      gtl::rectangle_data<frCoord> checkRect(rect1);
      gtl::generalized_intersect(checkRect, rect2);
      // check for prl
      frCoord prl, dist;
      if (viaEdge1->getDir() == frDirEnum::S
          || viaEdge1->getDir() == frDirEnum::N) {
        // vertical prl
        dist = gtl::euclidean_distance(checkRect, rect2, gtl::VERTICAL);
        prl = gtl::delta(checkRect, gtl::VERTICAL);
      } else {
        // horizontal prl
        dist = gtl::euclidean_distance(checkRect, rect2, gtl::HORIZONTAL);
        prl = gtl::delta(checkRect, gtl::HORIZONTAL);
      }
      if (dist == 0 && prl > reqPrl)
        strategy = 1;  // second spacing value
    }
    frSquaredDistance spcSqr
        = dbRule->getSpacing(class1, isSide1, class2, isSide2, strategy);
    spcSqr *= spcSqr;
    if (dbRule->isCenterToCenter(class1, class2) || (dbRule->isCenterAndEdge(class1, class2) && strategy == 1)) {
      if (getCenterToCenterDist(viaEdge1, viaEdge2) < spcSqr)
        viol = true;
    } else if ((frSquaredDistance) gtl::square_euclidean_distance(rect1, rect2)
               < spcSqr) {
      viol = true;
    }
  }
  if (viol) {
    // violation
    gtl::rectangle_data<frCoord> markerRect(
        *viaEdge1->getPin()->getMaxRectangles()[0].get());
    gtl::generalized_intersect(
        markerRect, *viaEdge2->getPin()->getMaxRectangles()[0].get());
    auto net1 = viaEdge1->getNet();
    auto net2 = viaEdge2->getNet();
    auto marker = make_unique<frMarker>();
    frBox box(gtl::xl(markerRect),
              gtl::yl(markerRect),
              gtl::xh(markerRect),
              gtl::yh(markerRect));
    marker->setBBox(box);
    marker->setLayerNum(layerNum1);
    marker->setConstraint(con);
    marker->addSrc(net1->getOwner());
    frCoord llx
        = min(viaEdge1->getLowCorner()->x(), viaEdge1->getHighCorner()->x());
    frCoord lly
        = min(viaEdge1->getLowCorner()->y(), viaEdge1->getHighCorner()->y());
    frCoord urx
        = max(viaEdge1->getLowCorner()->x(), viaEdge1->getHighCorner()->x());
    frCoord ury
        = max(viaEdge1->getLowCorner()->y(), viaEdge1->getHighCorner()->y());
    marker->addVictim(net1->getOwner(),
                      make_tuple(layerNum1,
                                 frBox(llx, lly, urx, ury),
                                 viaEdge1->isFixed()));
    marker->addSrc(net2->getOwner());
    llx = min(viaEdge2->getLowCorner()->x(), viaEdge2->getHighCorner()->x());
    lly = min(viaEdge2->getLowCorner()->y(), viaEdge2->getHighCorner()->y());
    urx = max(viaEdge2->getLowCorner()->x(), viaEdge2->getHighCorner()->x());
    ury = max(viaEdge2->getLowCorner()->y(), viaEdge2->getHighCorner()->y());
    marker->addAggressor(net2->getOwner(),
                         make_tuple(layerNum2,
                                    frBox(llx, lly, urx, ury),
                                    viaEdge2->isFixed()));
    addMarker(std::move(marker));
  }
}

void FlexGCWorker::Impl::checkLef58CutSpacingTbl(
    gcSegment* viaEdge,
    frLef58CutSpacingTableConstraint* con)
{
  auto layerNum1 = viaEdge->getLayerNum();
  auto layer1 = getTech()->getLayer(layerNum1);
  auto isSide = isSideEdge(viaEdge);
  auto cutClass = getCutClass(layer1, viaEdge);
  auto dbRule = con->getODBRule();
  frLayerNum queryLayerNum;
  if(dbRule->isLayerValid())
    queryLayerNum = getTech()->getLayer(dbRule->getSecondLayer()->getName())->getLayerNum();
  else
    queryLayerNum = layerNum1;
  auto maxSpc = dbRule->getMaxSpacing(cutClass, isSide);
  box_t queryBox;
  gtl::rectangle_data<frCoord> queryRect;
  getLef58CutSpacingTblQueryBox(viaEdge, maxSpc, queryBox, queryRect);
  vector<pair<segment_t, gcSegment*>> results;
  auto& workerRegionQuery = getWorkerRegionQuery();
  workerRegionQuery.queryPolygonEdge(queryBox, queryLayerNum, results);
  for (auto res : results) {
    auto ptr = res.second;
    if (ptr->isFixed() && viaEdge->isFixed())
      continue;
    if (ptr->getPin() == viaEdge->getPin())
      continue;
    if (dbRule->isLayerValid() && ptr->getNet() == viaEdge->getNet())
      continue;
    checkLef58CutSpacingTbl_main(viaEdge, ptr, con);
  }
}