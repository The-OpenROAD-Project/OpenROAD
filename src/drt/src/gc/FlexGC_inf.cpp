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

#include "frProfileTask.h"
#include "gc/FlexGC_impl.h"
using namespace std;
using namespace fr;

void FlexGCWorker::Impl::checkRectMetSpcTblInf_queryBox(
    const gtl::rectangle_data<frCoord>& rect,
    const frCoord dist,
    const frDirEnum dir,
    box_t& box)
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

void FlexGCWorker::Impl::checkOrthRectsMetSpcTblInf(
    const std::vector<gcRect*>& rects,
    const gtl::rectangle_data<frCoord>& queryRect,
    const frCoord spacing,
    const gtl::orientation_2d orient)
{
  frSquaredDistance spacingSq = spacing * (frSquaredDistance) spacing;
  /*
  for each i_th wire, check the next sz-i wires for violation
    if spacing is less than required, check violation
    else, then spacing is larger than required.
      Then no violation would be found for that wire and all the next
      wires.(because rects is sorted) Then stop checking the i_th wire and move
      on to i+1 wire.
  */
  for (size_t i = 0; i < rects.size() - 1; i++) {
    for (size_t j = i + 1; j < rects.size(); j++) {
      frSquaredDistance distSquared
          = gtl::square_euclidean_distance(*rects[i], *rects[j]);
      if (distSquared < spacingSq) {
        // Violation
        if (distSquared == 0)  // short!
          continue;  // shorts are already checked in checkMetalSpacing_main
        if (rects[i]->isFixed() && rects[j]->isFixed())
          continue;
        auto lNum = rects[i]->getLayerNum();
        gtl::rectangle_data<frCoord> rect1(queryRect);
        gtl::rectangle_data<frCoord> rect2(queryRect);
        gtl::intersect(rect1, *rects[i]);
        gtl::intersect(rect2, *rects[j]);
        gtl::rectangle_data<frCoord> markerRect(rect1);
        gtl::generalized_intersect(markerRect, rect2);
        auto marker = make_unique<frMarker>();
        frBox box(gtl::xl(markerRect),
                  gtl::yl(markerRect),
                  gtl::xh(markerRect),
                  gtl::yh(markerRect));
        marker->setBBox(box);
        marker->setLayerNum(lNum);
        marker->setConstraint(
            getTech()->getLayer(lNum)->getSpacingTableInfluence());
        marker->addSrc(rects[i]->getNet()->getOwner());
        marker->addVictim(rects[i]->getNet()->getOwner(),
                          make_tuple(lNum,
                                     frBox(gtl::xl(rect1),
                                           gtl::yl(rect1),
                                           gtl::xh(rect1),
                                           gtl::yh(rect1)),
                                     rects[i]->isFixed()));
        marker->addSrc(rects[j]->getNet()->getOwner());
        marker->addAggressor(rects[j]->getNet()->getOwner(),
                             make_tuple(lNum,
                                        frBox(gtl::xl(rect2),
                                              gtl::yl(rect2),
                                              gtl::xh(rect2),
                                              gtl::yh(rect2)),
                                        rects[j]->isFixed()));
        addMarker(std::move(marker));
      } else
        break;  // spacing is larger than required, no need to check other
                // wires.
    }
  }
}
bool compareVertical(gcRect* r1, gcRect* r2)
{
  return (gtl::yl(*r1) < gtl::yl(*r2));
}

bool compareHorizontal(gcRect* r1, gcRect* r2)
{
  return (gtl::xl(*r1) < gtl::xl(*r2));
}

void FlexGCWorker::Impl::checkRectMetSpcTblInf(
    gcRect* rect,
    frSpacingTableInfluenceConstraint* con)
{
  frCoord width = rect->width();
  frLayerNum lNum = rect->getLayerNum();
  auto owner = rect->getNet()->getOwner();
  if (owner != nullptr
      && (owner->typeId() == frcInstBlockage
          || owner->typeId() == frcBlockage)) {
    frBlockage* blkg;
    if (owner->typeId() == frcInstBlockage)
      blkg = static_cast<frInstBlockage*>(owner)->getBlockage();
    else
      blkg = static_cast<frBlockage*>(owner);
    if (blkg->getDesignRuleWidth() != -1)
      width = blkg->getDesignRuleWidth();
  }
  if (width < con->getMinWidth())
    return;
  auto val = con->find(width);
  frCoord within, spacing;
  within = val.first;
  spacing = val.second;
  auto dir = gtl::guess_orientation(*rect);
  box_t queryBox1, queryBox2;
  if (dir == gtl::HORIZONTAL) {
    // wide wire is horizontal check above and below for orth wires
    checkRectMetSpcTblInf_queryBox(*rect, within, frDirEnum::N, queryBox1);
    checkRectMetSpcTblInf_queryBox(*rect, within, frDirEnum::S, queryBox2);
  } else {
    // wide wire is vertical check left and right for orth wires
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
    vector<gcRect*> rects;
    for (auto& [objBox, objPtr] : result) {
      if (!gtl::intersects(*objPtr, queryRect, false))
        continue;  // ignore if it only touches the query region
      if (gtl::guess_orientation(*objPtr) == dir)
        continue;  // ignore if not orthogonal
      rects.push_back(objPtr);
    }
    if (rects.size() < 2)
      continue;  // At least two orthogonal rectangle are required for checking
    if (dir == gtl::HORIZONTAL)
      std::sort(rects.begin(), rects.end(), compareHorizontal);
    else
      std::sort(rects.begin(), rects.end(), compareVertical);
    // <rects> should be a sorted vector of all the wires found in the region
    // It should be sorted in the orientation we are checking
    checkOrthRectsMetSpcTblInf(rects, queryRect, spacing, dir);
  }
}
void FlexGCWorker::Impl::checkPinMetSpcTblInf(gcPin* pin)
{
  frLayerNum lNum = pin->getPolygon()->getLayerNum();
  auto con = getTech()->getLayer(lNum)->getSpacingTableInfluence();
  for (auto& rect : pin->getMaxRectangles()) {
    checkRectMetSpcTblInf(rect.get(), con);
  }
}
void FlexGCWorker::Impl::checkMetalSpacingTableInfluence()
{
  if (targetNet_) {
    for (int i
         = std::max((frLayerNum)(getTech()->getBottomLayerNum()), minLayerNum_);
         i <= std::min((frLayerNum)(getTech()->getTopLayerNum()), maxLayerNum_);
         i++) {
      auto currLayer = getTech()->getLayer(i);
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
         = std::max((frLayerNum)(getTech()->getBottomLayerNum()), minLayerNum_);
         i <= std::min((frLayerNum)(getTech()->getTopLayerNum()), maxLayerNum_);
         i++) {
      auto currLayer = getTech()->getLayer(i);
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
