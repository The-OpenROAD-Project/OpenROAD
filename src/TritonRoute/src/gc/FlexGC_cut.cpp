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

#include "gc/FlexGC_impl.h"
#include "frProfileTask.h"
#include "opendb/db.h"
using namespace fr;
/*
int getCutClassIdx(layer, viaEdge)
{
  width = std::min(gtl::length(viaEdge), gtl::length(viaEdge->prevEdge));
  length = std::max(gtl::length(viaEdge), gtl::length(viaEdge->prevEdge));
  return layer->getCutClassIdx(width, length);
}
bool isSide(viaEdge)
{
  return gtl::length(viaEdge) > gtl::length(viaEdge->prevEdge);
}
void getQueryRegion(viaEdge, maxSpc)
{
  bloat viaEdge by MaxSpc according to direction
}
void checkLef58CutSp1acingTbl(viaEdge1, viaEdge2)
{
  class1 = getCutClassIdx(viaEdge1)
  class2 = getCutClassIdx(viaEdge2)
  reqSpc = getSpacing(class1, isSide(viaEdge1), class2, isSide(viaEdge2))
  isCenterToCenter = isCenterToCenter(class1, class2)
  dist = 0
  if(isCenterToCenter)
  {
    center1 = getCenteroid(viaEdge1->getPin()->getPolygon())
    center2 = getCenteroid(viaEdge2->getPin()->getPolygon())
    dist = euclidean_dist(center1, center2)
  }else
  {
    dist = euclidean_dist(viaEdge1, viaEdge2)
  }
  if(dist < reqSpc)
  {
    //mark violation
  }
}

void FlexGCWorker::Impl::checkLef58CutSpacingTbl_main(gcSegment* viaEdge, frLef58CutSpacingTableConstraint* con)
{
  auto layerNum = viaEdge->getLayerNum();
  auto layer = getTech()->getLayer(layerNum);
  width = std::min(gtl::length(viaEdge), gtl::length(viaEdge->prevEdge));
  length = std::max(gtl::length(viaEdge), gtl::length(viaEdge->prevEdge));
  isSide = isSide(viaEdge);
  auto cutClassIdx = layer->getCutClassIdx(width, length);
  cutClass = layer->getCutClass(cutClassIdx);
  auto dbRule = con->getODBRule();
  maxSpc = dbRule->getMaxSpacing(cutclass, isSide)
  box = getQueryRegion(viaEdge, maxSpc);
  results = queryEdges(box, layer)
  for(edge : results)
  {
    if(edge's pin is viaEdge's pin)
      continue;
    checkLef58CutSpacingTbl(edge, viaEdge);
  }
 
}
 */


inline std::string getCutClass(frLayer* layer, gcSegment* viaEdge)
{
  auto width = std::min(gtl::length(*viaEdge), gtl::length(*(viaEdge->getPrevEdge())));
  auto length = std::max(gtl::length(*viaEdge), gtl::length(*(viaEdge->getPrevEdge())));
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

void FlexGCWorker::Impl::checkLef58CutSpacingTbl_main(gcSegment* viaEdge1, 
                                                      gcSegment* viaEdge2, 
                                                      frLef58CutSpacingTableConstraint* con)
{
  auto layerNum = viaEdge1->getLayerNum();
  auto layer = getTech()->getLayer(layerNum);
  auto class1 = getCutClass(layer, viaEdge1);
  auto class2 = getCutClass(layer, viaEdge2);
  auto isSide1 = isSideEdge(viaEdge1);
  auto isSide2 = isSideEdge(viaEdge2);
  auto spc = con->getODBRule()->getSpacing(class1.c_str(), isSide1, class2.c_str(), isSide2);
  frSquaredDistance spcSqr = std::max(spc.first, spc.second);
  auto isCenterToCenter = con->getODBRule()->isCenterToCenter(class1, class2);
  spcSqr *= spcSqr;
  frSquaredDistance dist = 0;
  gtl::rectangle_data<frCoord> rect1(viaEdge1->low().x(), viaEdge1->low().y(), viaEdge1->high().x(), viaEdge1->high().y());
  gtl::rectangle_data<frCoord> rect2(viaEdge2->low().x(), viaEdge2->low().y(), viaEdge2->high().x(), viaEdge2->high().y());
  gtl::rectangle_data<frCoord> markerRect(rect1);
  if(isCenterToCenter)
  {
    auto poly1 = viaEdge1->getPin()->getPolygon();
    auto poly2 = viaEdge2->getPin()->getPolygon();
    gtl::point_data<frCoord> centerPt1, centerPt2;
    gtl::center(centerPt1, *poly1);
    gtl::center(centerPt2, *poly2);
    dist = gtl::distance_squared(centerPt1, centerPt2);
    gtl::set_points(markerRect, centerPt1, centerPt2);
  } else {
    dist = gtl::square_euclidean_distance(rect1, rect2);
    gtl::generalized_intersect(markerRect, rect2);
  }
  if(dist < spcSqr)
  {
    //violation
    auto net1 = viaEdge1->getNet();
    auto net2 = viaEdge2->getNet();
    auto marker = make_unique<frMarker>();
    frBox box(gtl::xl(markerRect),
              gtl::yl(markerRect),
              gtl::xh(markerRect),
              gtl::yh(markerRect));
    marker->setBBox(box);
    marker->setLayerNum(layerNum);
    marker->setConstraint(con);
    marker->addSrc(net1->getOwner());
    frCoord llx = min(viaEdge1->getLowCorner()->x(), viaEdge1->getHighCorner()->x());
    frCoord lly = min(viaEdge1->getLowCorner()->y(), viaEdge1->getHighCorner()->y());
    frCoord urx = max(viaEdge1->getLowCorner()->x(), viaEdge1->getHighCorner()->x());
    frCoord ury = max(viaEdge1->getLowCorner()->y(), viaEdge1->getHighCorner()->y());
    marker->addVictim(
        net1->getOwner(),
        make_tuple(
            viaEdge1->getLayerNum(), frBox(llx, lly, urx, ury), viaEdge1->isFixed()));
    marker->addSrc(net2->getOwner());
    llx = min(viaEdge2->getLowCorner()->x(), viaEdge2->getHighCorner()->x());
    lly = min(viaEdge2->getLowCorner()->y(), viaEdge2->getHighCorner()->y());
    urx = max(viaEdge2->getLowCorner()->x(), viaEdge2->getHighCorner()->x());
    ury = max(viaEdge2->getLowCorner()->y(), viaEdge2->getHighCorner()->y());
    marker->addAggressor(
        net2->getOwner(),
        make_tuple(
            viaEdge2->getLayerNum(), frBox(llx, lly, urx, ury), viaEdge2->isFixed()));
    addMarker(std::move(marker));
  }
}

void FlexGCWorker::Impl::checkLef58CutSpacingTbl(gcSegment* viaEdge, frLef58CutSpacingTableConstraint* con)
{
  auto layerNum = viaEdge->getLayerNum();
  auto layer = getTech()->getLayer(layerNum);
  auto isSide = isSideEdge(viaEdge);
  auto cutClass = getCutClass(layer, viaEdge);
  auto dbRule = con->getODBRule();
  auto maxSpc = dbRule->getMaxSpacing(cutClass, isSide);
  box_t queryBox;
  gtl::rectangle_data<frCoord> queryRect;
  getLef58CutSpacingTblQueryBox(viaEdge, maxSpc, queryBox, queryRect);
  vector<pair<segment_t, gcSegment*>> results;
  auto& workerRegionQuery = getWorkerRegionQuery();
  workerRegionQuery.queryPolygonEdge(queryBox, layerNum, results);
  for(auto res : results)
  {
    auto ptr = res.second;
    if(ptr->isFixed() && viaEdge->isFixed())
      continue;
    if(ptr->getPin() == viaEdge->getPin())
      continue;
    checkLef58CutSpacingTbl_main(viaEdge, ptr, con);
  }
}