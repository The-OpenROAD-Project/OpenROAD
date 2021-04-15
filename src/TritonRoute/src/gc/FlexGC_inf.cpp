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

#include <iostream>

#include "frProfileTask.h"
#include "gc/FlexGC_impl.h"
using namespace std;
using namespace fr;

void FlexGCWorker::Impl::checkRectMetSpcTblInf_queryBox(
    const gtl::rectangle_data<frCoord>& rect,
    frCoord dist,
    frDirEnum dir,
    box_t& box)  // east and north
{
  switch (dir) {
    case frDirEnum::N:
      bg::set<bg::min_corner, 0>(box, gtl::xl(rect));
      bg::set<bg::min_corner, 1>(box, gtl::yh(rect));
      bg::set<bg::max_corner, 0>(box, gtl::xh(rect));
      bg::set<bg::max_corner, 1>(box, gtl::yh(rect) + dist);
      break;
    case frDirEnum::S:
      bg::set<bg::min_corner, 0>(box, gtl::xl(rect));
      bg::set<bg::min_corner, 1>(box, gtl::yl(rect) - dist);
      bg::set<bg::max_corner, 0>(box, gtl::xh(rect));
      bg::set<bg::max_corner, 1>(box, gtl::yl(rect));
      break;
    case frDirEnum::E:
      bg::set<bg::min_corner, 0>(box, gtl::xh(rect));
      bg::set<bg::min_corner, 1>(box, gtl::yl(rect));
      bg::set<bg::max_corner, 0>(box, gtl::xh(rect) + dist);
      bg::set<bg::max_corner, 1>(box, gtl::yh(rect));
      break;
    case frDirEnum::W:
      bg::set<bg::min_corner, 0>(box, gtl::xl(rect) - dist);
      bg::set<bg::min_corner, 1>(box, gtl::yl(rect));
      bg::set<bg::max_corner, 0>(box, gtl::xl(rect));
      bg::set<bg::max_corner, 1>(box, gtl::yh(rect));
      break;

    default:
      break;
  }
}
void FlexGCWorker::Impl::check90RectMetSpcTblInf_queryBox(
    const gtl::rectangle_data<frCoord>& rect,
    frUInt4 spacing,
    gtl::orientation_2d orient,
    box_t& box)
{
  if (orient == gtl::HORIZONTAL)  // query north neighbors
  {
    bg::set<bg::min_corner, 0>(box, gtl::xl(rect));
    bg::set<bg::min_corner, 1>(box, gtl::yh(rect));
    bg::set<bg::max_corner, 0>(box, gtl::xh(rect));
    bg::set<bg::max_corner, 1>(box, gtl::yh(rect) + spacing);
  } else  // query easter neighbors
  {
    bg::set<bg::min_corner, 0>(box, gtl::xh(rect));
    bg::set<bg::min_corner, 1>(box, gtl::yl(rect));
    bg::set<bg::max_corner, 0>(box, gtl::xh(rect) + spacing);
    bg::set<bg::max_corner, 1>(box, gtl::yh(rect));
  }
}
void FlexGCWorker::Impl::check90RectMetSpcTblInf(
    gcRect* rect,
    gtl::rectangle_data<frCoord> intersection,
    frUInt4 spacing,
    gtl::orientation_2d required_orient)
{
  frLayerNum lNum = rect->getLayerNum();
  box_t queryBox;
  check90RectMetSpcTblInf_queryBox(
      intersection, spacing, required_orient, queryBox);
  auto& workerRegionQuery = getWorkerRegionQuery();
  vector<rq_box_value_t<gcRect*>> result;
  workerRegionQuery.queryMaxRectangle(queryBox, lNum, result);
  gtl::rectangle_data<frCoord> queryRect(queryBox.min_corner().x(),
                                         queryBox.min_corner().y(),
                                         queryBox.max_corner().x(),
                                         queryBox.max_corner().y());

  for (auto& [objBox, objPtr] : result) {
    if (gtl::guess_orientation(*objPtr) != required_orient)
      continue;
    if (!gtl::intersects(*objPtr, queryRect, false))
      continue;
    if(rect->isFixed() && objPtr->isFixed())
      continue;
    gtl::rectangle_data<frCoord> secondIntersection(queryRect);
    gtl::intersect(secondIntersection, *objPtr);
    gtl::rectangle_data<frCoord> markerRect(intersection);
    gtl::generalized_intersect(markerRect, secondIntersection);
    auto marker = make_unique<frMarker>();
    frBox box(gtl::xl(markerRect),
              gtl::yl(markerRect),
              gtl::xh(markerRect),
              gtl::yh(markerRect));
    marker->setBBox(box);
    marker->setLayerNum(lNum);
    marker->setConstraint(
        design_->getTech()->getLayer(lNum)->getSpacingTableInfluence());
    marker->addSrc(rect->getNet()->getOwner());
    marker->addVictim(rect->getNet()->getOwner(),
                      make_tuple(lNum,
                                 frBox(gtl::xl(intersection),
                                       gtl::yl(intersection),
                                       gtl::xh(intersection),
                                       gtl::yh(intersection)),
                                 rect->isFixed()));
    marker->addSrc(objPtr->getNet()->getOwner());
    marker->addAggressor(objPtr->getNet()->getOwner(),
                         make_tuple(lNum,
                                    frBox(gtl::xl(secondIntersection),
                                          gtl::yl(secondIntersection),
                                          gtl::xh(secondIntersection),
                                          gtl::yh(secondIntersection)),
                                    objPtr->isFixed()));
    if (addMarker(std::move(marker))) {
      // true marker
    }
  }
}
void FlexGCWorker::Impl::checkRectMetSpcTblInf(
    gcRect* rect,
    frSpacingTableInfluenceConstraint* con)
{
  frUInt4 width = rect->width();
  frLayerNum lNum = rect->getLayerNum();
  if (width < con->getLookupTbl().getMin())
    return;
  auto val = con->getLookupTbl().find(width);
  frUInt4 within, spacing;
  within = val.first;
  spacing = val.second;
  auto dir = gtl::guess_orientation(*rect);
  box_t queryBox1, queryBox2;
  if (dir == gtl::HORIZONTAL) {
    checkRectMetSpcTblInf_queryBox(*rect, within, frDirEnum::N, queryBox1);
    checkRectMetSpcTblInf_queryBox(*rect, within, frDirEnum::S, queryBox2);
  } else {
    checkRectMetSpcTblInf_queryBox(*rect, within, frDirEnum::E, queryBox1);
    checkRectMetSpcTblInf_queryBox(*rect, within, frDirEnum::W, queryBox2);
  }
  for (auto queryBox : {queryBox1, queryBox2}) {
    gtl::rectangle_data<frCoord> queryRect(queryBox.min_corner().x(),
                                           queryBox.min_corner().y(),
                                           queryBox.max_corner().x(),
                                           queryBox.max_corner().y());
    auto& workerRegionQuery = getWorkerRegionQuery();
    vector<rq_box_value_t<gcRect*>> result;
    workerRegionQuery.queryMaxRectangle(queryBox, lNum, result);
    if (result.size() < 2)
      continue;
    for (auto& [objBox, objPtr] : result) {
      if (!gtl::intersects(*objPtr, queryRect, false))
        continue;
      if (gtl::guess_orientation(*objPtr) == dir)
        continue;  // not perpendicular
      gtl::rectangle_data<frCoord> intersection(queryRect);
      gtl::intersect(intersection, *objPtr);
      check90RectMetSpcTblInf(
          objPtr, intersection, spacing, dir.get_perpendicular());
    }
  }
}
void FlexGCWorker::Impl::checkPinMetSpcTblInf(gcPin* pin)
{
  frLayerNum lNum = pin->getPolygon()->getLayerNum();
  auto con = design_->getTech()->getLayer(lNum)->getSpacingTableInfluence();
  for (auto& rect : pin->getMaxRectangles()) {
    checkRectMetSpcTblInf(rect.get(), con);
  }
}
void FlexGCWorker::Impl::checkMetalSpacingTableInfluence()
{
  if (targetNet_) {
    for (int i
         = std::max((frLayerNum)(getDesign()->getTech()->getBottomLayerNum()),
                    minLayerNum_);
         i <= std::min((frLayerNum)(getDesign()->getTech()->getTopLayerNum()),
                       maxLayerNum_);
         i++) {
      auto currLayer = getDesign()->getTech()->getLayer(i);
      if (currLayer->getType() != frLayerTypeEnum::ROUTING) {
        continue;
      }
      if (!currLayer->hasSpacingTableInfluence())
        continue;
      for (auto& pin : targetNet_->getPins(i)) {
        checkPinMetSpcTblInf(pin.get());
      }
    }
  } else {
    // layer --> net --> polygon
    for (int i
         = std::max((frLayerNum)(getDesign()->getTech()->getBottomLayerNum()),
                    minLayerNum_);
         i <= std::min((frLayerNum)(getDesign()->getTech()->getTopLayerNum()),
                       maxLayerNum_);
         i++) {
      auto currLayer = getDesign()->getTech()->getLayer(i);
      if (currLayer->getType() != frLayerTypeEnum::ROUTING) {
        continue;
      }
      if (!currLayer->hasSpacingTableInfluence())
        continue;
      for (auto& uNet : getNets())
        for (auto& pin : uNet.get()->getPins(i)) {
          checkPinMetSpcTblInf(pin.get());
        }
    }
  }
}