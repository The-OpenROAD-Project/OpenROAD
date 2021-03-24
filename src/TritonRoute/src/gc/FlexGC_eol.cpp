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
bool FlexGCWorker::Impl::checkMetalEndOfLine_eol_isEolEdge(
    gcSegment* edge,
    frConstraint* constraint)
{
  frCoord eolWidth;
  switch (constraint->typeId()) {
    case frConstraintTypeEnum::frcSpacingEndOfLineConstraint: {
      auto con = (frSpacingEndOfLineConstraint*) constraint;
      eolWidth = con->getEolWidth();
    } break;
    case frConstraintTypeEnum::frcLef58SpacingEndOfLineConstraint: {
      auto con = (frLef58SpacingEndOfLineConstraint*) constraint;
      eolWidth = con->getEolWidth();
    } break;
    default:
      logger_->error(DRT, 223, "Unsupported endofline spacing rule");
      break;
  }
  // skip if >= eolWidth
  if (gtl::length(*edge) >= eolWidth) {
    return false;
  }
  // skip if non convex edge
  auto prevEdge = edge->getPrevEdge();
  auto nextEdge = edge->getNextEdge();
  if (!(gtl::orientation(*prevEdge, *edge) == 1
        && gtl::orientation(*edge, *nextEdge) == 1)) {
    return false;
  }
  return true;
}

bool FlexGCWorker::Impl::checkMetalEndOfLine_eol_hasMinMaxLength(
    gcSegment* edge,
    frConstraint* constraint)
{
  if (constraint->typeId()
      != frConstraintTypeEnum::frcLef58SpacingEndOfLineConstraint)
    return true;
  auto con = (frLef58SpacingEndOfLineConstraint*) constraint;
  if (!con->getWithinConstraint()->hasMaxMinLengthConstraint())
    return true;
  auto minMaxCon = con->getWithinConstraint()->getMaxMinLengthConstraint();
  auto prevEdgeLen = gtl::length(*edge->getPrevEdge());
  auto nextEdgeLen = gtl::length(*edge->getNextEdge());
  if (minMaxCon->isMaxLength())
    if (minMaxCon->isTwoSides())
      return prevEdgeLen <= minMaxCon->getLength()
             && nextEdgeLen <= minMaxCon->getLength();
    else
      return prevEdgeLen <= minMaxCon->getLength()
             || nextEdgeLen <= minMaxCon->getLength();
  else if (minMaxCon->isTwoSides())
    return prevEdgeLen >= minMaxCon->getLength()
           && nextEdgeLen >= minMaxCon->getLength();
  else
    return prevEdgeLen >= minMaxCon->getLength()
           || nextEdgeLen >= minMaxCon->getLength();
}

// TODO check for hasRoute
bool FlexGCWorker::Impl::checkMetalEndOfLine_eol_hasEncloseCut(
    gcSegment* edge1,
    gcSegment* edge2,
    frConstraint* constraint)
{
  if (constraint->typeId()
      != frConstraintTypeEnum::frcLef58SpacingEndOfLineConstraint)
    return true;
  auto con = (frLef58SpacingEndOfLineConstraint*) constraint;
  if (!con->getWithinConstraint()->hasEncloseCutConstraint())
    return true;
  auto encCutCon = con->getWithinConstraint()->getEncloseCutConstraint();
  frSquaredDistance cutToMetalSpace
      = encCutCon->getCutToMetalSpace() * encCutCon->getCutToMetalSpace();
  box_t queryBox;  // query region is encloseDist from edge1
  checkMetalEndOfLine_eol_hasEncloseCut_getQueryBox(
      edge1, encCutCon.get(), queryBox);

  std::vector<int> layers;  // cutLayers to search for the vias
  bool aboveAndBelow
      = !(encCutCon->isBelow()
          || encCutCon->isAbove());  // if not defined (means both)
  if (aboveAndBelow || encCutCon->isAbove())
    if (edge1->getLayerNum() + 1 <= maxLayerNum_)
      layers.push_back(edge1->getLayerNum() + 1);
  if (aboveAndBelow || encCutCon->isBelow())
    if (edge1->getLayerNum() - 1 >= minLayerNum_)
      layers.push_back(edge1->getLayerNum() - 1);
  // edge2 segment as a rectangle for getting distance
  gtl::rectangle_data<frCoord> metRect(edge2->getLowCorner()->x(),
                                       edge2->getLowCorner()->y(),
                                       edge2->getHighCorner()->x(),
                                       edge2->getHighCorner()->y());
  bool found = false;
  // allCuts=false --> return true if you find one cut segment that satisfies
  // the enclose cut allCuts=true  --> return true if all cut segment satisfies
  // the enclose cut (at least one has to be found)
  for (auto layerNum : layers) {
    vector<pair<segment_t, gcSegment*>> results;
    auto& workerRegionQuery = getWorkerRegionQuery();
    workerRegionQuery.queryPolygonEdge(queryBox, layerNum, results);
    for (auto& [boostSeg, ptr] : results) {
      if (edge1->getDir() != ptr->getDir())
        continue;
      gtl::rectangle_data<frCoord> rect(ptr->getLowCorner()->x(),
                                        ptr->getLowCorner()->y(),
                                        ptr->getHighCorner()->x(),
                                        ptr->getHighCorner()->y());
      frSquaredDistance dist = gtl::square_euclidean_distance(metRect, rect);
      if (dist < cutToMetalSpace) {
        if (encCutCon->isAllCuts())
          found = true;
        else
          return true;
      } else if (encCutCon->isAllCuts())
        return false;
    }
  }
  return found;
}

void FlexGCWorker::Impl::checkMetalEndOfLine_eol_hasEncloseCut_getQueryBox(
    gcSegment* edge,
    frLef58SpacingEndOfLineWithinEncloseCutConstraint* constraint,
    box_t& queryBox)
{
  frCoord encDist = constraint->getEncloseDist();
  switch (edge->getDir()) {
    case frDirEnum::W:
      bg::set<bg::min_corner, 0>(queryBox, edge->high().x());
      bg::set<bg::min_corner, 1>(queryBox, edge->high().y() - encDist);
      bg::set<bg::max_corner, 0>(queryBox, edge->low().x());
      bg::set<bg::max_corner, 1>(queryBox, edge->low().y());
      break;
    case frDirEnum::E:
      bg::set<bg::min_corner, 0>(queryBox, edge->low().x());
      bg::set<bg::min_corner, 1>(queryBox, edge->low().y());
      bg::set<bg::max_corner, 0>(queryBox, edge->high().x());
      bg::set<bg::max_corner, 1>(queryBox, edge->high().y() + encDist);
      break;
    case frDirEnum::S:
      bg::set<bg::min_corner, 0>(queryBox, edge->high().x());
      bg::set<bg::min_corner, 1>(queryBox, edge->high().y());
      bg::set<bg::max_corner, 0>(queryBox, edge->low().x() + encDist);
      bg::set<bg::max_corner, 1>(queryBox, edge->low().y());
      break;
    case frDirEnum::N:
      bg::set<bg::min_corner, 0>(queryBox, edge->low().x() - encDist);
      bg::set<bg::min_corner, 1>(queryBox, edge->low().y());
      bg::set<bg::max_corner, 0>(queryBox, edge->high().x());
      bg::set<bg::max_corner, 1>(queryBox, edge->high().y());
      break;
    default:
      break;
  }
}
// bbox on the gcSegment->low() side
void FlexGCWorker::Impl::
    checkMetalEndOfLine_eol_hasParallelEdge_oneDir_getQueryBox(
        gcSegment* edge,
        frConstraint* constraint,
        bool isSegLow,
        box_t& queryBox,
        gtl::rectangle_data<frCoord>& queryRect)
{
  frCoord eolWithin, parWithin, parSpace;
  switch (constraint->typeId()) {
    case frConstraintTypeEnum::frcSpacingEndOfLineConstraint: {
      auto con = (frSpacingEndOfLineConstraint*) constraint;
      eolWithin = con->getEolWithin();
      parWithin = con->getParWithin();
      parSpace = con->getParSpace();
    } break;
    case frConstraintTypeEnum::frcLef58SpacingEndOfLineConstraint: {
      auto con = (frLef58SpacingEndOfLineConstraint*) constraint;
      eolWithin = con->getWithinConstraint()->getEolWithin();
      parWithin = con->getWithinConstraint()
                      ->getParallelEdgeConstraint()
                      ->getParWithin();
      parSpace = con->getWithinConstraint()
                     ->getParallelEdgeConstraint()
                     ->getParSpace();
    } break;
    default:
      logger_->error(DRT, 224, "Unsupported endofline spacing rule");
      break;
  }

  frCoord ptX, ptY;
  if (isSegLow) {
    ptX = edge->low().x();
    ptY = edge->low().y();
    if (edge->getDir() == frDirEnum::E) {
      bg::set<bg::min_corner, 0>(queryBox, ptX - parSpace);
      bg::set<bg::min_corner, 1>(queryBox, ptY - eolWithin);
      bg::set<bg::max_corner, 0>(queryBox, ptX);
      bg::set<bg::max_corner, 1>(queryBox, ptY + parWithin);
    } else if (edge->getDir() == frDirEnum::W) {
      bg::set<bg::min_corner, 0>(queryBox, ptX);
      bg::set<bg::min_corner, 1>(queryBox, ptY - parWithin);
      bg::set<bg::max_corner, 0>(queryBox, ptX + parSpace);
      bg::set<bg::max_corner, 1>(queryBox, ptY + eolWithin);
    } else if (edge->getDir() == frDirEnum::N) {
      bg::set<bg::min_corner, 0>(queryBox, ptX - parWithin);
      bg::set<bg::min_corner, 1>(queryBox, ptY - parSpace);
      bg::set<bg::max_corner, 0>(queryBox, ptX + eolWithin);
      bg::set<bg::max_corner, 1>(queryBox, ptY);
    } else {  // S
      bg::set<bg::min_corner, 0>(queryBox, ptX - eolWithin);
      bg::set<bg::min_corner, 1>(queryBox, ptY);
      bg::set<bg::max_corner, 0>(queryBox, ptX + parWithin);
      bg::set<bg::max_corner, 1>(queryBox, ptY + parSpace);
    }
  } else {
    ptX = edge->high().x();
    ptY = edge->high().y();
    if (edge->getDir() == frDirEnum::E) {
      bg::set<bg::min_corner, 0>(queryBox, ptX);
      bg::set<bg::min_corner, 1>(queryBox, ptY - eolWithin);
      bg::set<bg::max_corner, 0>(queryBox, ptX + parSpace);
      bg::set<bg::max_corner, 1>(queryBox, ptY + parWithin);
    } else if (edge->getDir() == frDirEnum::W) {
      bg::set<bg::min_corner, 0>(queryBox, ptX - parSpace);
      bg::set<bg::min_corner, 1>(queryBox, ptY - parWithin);
      bg::set<bg::max_corner, 0>(queryBox, ptX);
      bg::set<bg::max_corner, 1>(queryBox, ptY + eolWithin);
    } else if (edge->getDir() == frDirEnum::N) {
      bg::set<bg::min_corner, 0>(queryBox, ptX - parWithin);
      bg::set<bg::min_corner, 1>(queryBox, ptY);
      bg::set<bg::max_corner, 0>(queryBox, ptX + eolWithin);
      bg::set<bg::max_corner, 1>(queryBox, ptY + parSpace);
    } else {  // S
      bg::set<bg::min_corner, 0>(queryBox, ptX - eolWithin);
      bg::set<bg::min_corner, 1>(queryBox, ptY - parSpace);
      bg::set<bg::max_corner, 0>(queryBox, ptX + parWithin);
      bg::set<bg::max_corner, 1>(queryBox, ptY);
    }
  }
  gtl::xl(queryRect, queryBox.min_corner().x());
  gtl::yl(queryRect, queryBox.min_corner().y());
  gtl::xh(queryRect, queryBox.max_corner().x());
  gtl::yh(queryRect, queryBox.max_corner().y());
}

void FlexGCWorker::Impl::
    checkMetalEndOfLine_eol_hasParallelEdge_oneDir_getParallelEdgeRect(
        gcSegment* edge,
        gtl::rectangle_data<frCoord>& rect)
{
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
  } else {  // S
    gtl::xl(rect, edge->high().x());
    gtl::yl(rect, edge->high().y());
    gtl::xh(rect, edge->low().x() + 1);
    gtl::yh(rect, edge->low().y());
  }
}

bool FlexGCWorker::Impl::checkMetalEndOfLine_eol_hasParallelEdge_oneDir(
    gcSegment* edge,
    frConstraint* constraint,
    bool isSegLow,
    bool& hasRoute)
{
  bool sol = false;
  auto layerNum = edge->getLayerNum();
  box_t queryBox;                          // (shrink by one, disabled)
  gtl::rectangle_data<frCoord> queryRect;  // original size
  checkMetalEndOfLine_eol_hasParallelEdge_oneDir_getQueryBox(
      edge, constraint, isSegLow, queryBox, queryRect);
  gtl::rectangle_data<frCoord> triggerRect;

  vector<pair<segment_t, gcSegment*>> results;
  auto& workerRegionQuery = getWorkerRegionQuery();
  workerRegionQuery.queryPolygonEdge(queryBox, edge->getLayerNum(), results);
  gtl::polygon_90_set_data<frCoord> tmpPoly;
  for (auto& [boostSeg, ptr] : results) {
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
    // (skip if no area, done by reducing queryBox... skipped here, --> not
    // skipped)
    checkMetalEndOfLine_eol_hasParallelEdge_oneDir_getParallelEdgeRect(
        ptr, triggerRect);
    if (!gtl::intersects(queryRect, triggerRect, false)) {
      continue;
    }
    // check whether parallel edge is route or not
    sol = true;
    if (!hasRoute && !ptr->isFixed()) {
      tmpPoly.clear();
      // checkMetalEndOfLine_eol_hasParallelEdge_oneDir_getParallelEdgeRect(ptr,
      // triggerRect);
      //  tmpPoly is the intersection of queryRect and minimum paralleledge rect
      using namespace boost::polygon::operators;
      tmpPoly += queryRect;
      tmpPoly &= triggerRect;
      auto& polys = ptr->getNet()->getPolygons(layerNum, false);
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

bool FlexGCWorker::Impl::checkMetalEndOfLine_eol_hasParallelEdge(
    gcSegment* edge,
    frConstraint* constraint,
    bool& hasRoute)
{
  bool hasTwoEdges = false;
  bool hasParallelEdge = false;
  switch (constraint->typeId()) {
    case frConstraintTypeEnum::frcSpacingEndOfLineConstraint: {
      auto con = (frSpacingEndOfLineConstraint*) constraint;
      hasParallelEdge = con->hasParallelEdge();
      hasTwoEdges = con->hasTwoEdges();
    } break;
    case frConstraintTypeEnum::frcLef58SpacingEndOfLineConstraint: {
      auto con = (frLef58SpacingEndOfLineConstraint*) constraint;
      hasParallelEdge = con->getWithinConstraint()->hasParallelEdgeConstraint();
      if (hasParallelEdge)
        hasTwoEdges = con->getWithinConstraint()
                          ->getParallelEdgeConstraint()
                          ->hasTwoEdges();
    } break;
    default:
      logger_->error(DRT, 225, "Unsupported endofline spacing rule");
      break;
  }

  if (!hasParallelEdge) {
    return true;
  }
  bool left = checkMetalEndOfLine_eol_hasParallelEdge_oneDir(
      edge, constraint, true, hasRoute);
  bool right = checkMetalEndOfLine_eol_hasParallelEdge_oneDir(
      edge, constraint, false, hasRoute);
  if ((!hasTwoEdges) && (left || right)) {
    return true;
  }
  if (hasTwoEdges && left && right) {
    return true;
  }
  return false;
}

void FlexGCWorker::Impl::checkMetalEndOfLine_eol_hasEol_getQueryBox(
    gcSegment* edge,
    frConstraint* constraint,
    box_t& queryBox,
    gtl::rectangle_data<frCoord>& queryRect)
{
  frCoord eolWithin, eolSpace;
  switch (constraint->typeId()) {
    case frConstraintTypeEnum::frcSpacingEndOfLineConstraint: {
      auto con = (frSpacingEndOfLineConstraint*) constraint;
      eolWithin = con->getEolWithin();
      eolSpace = con->getMinSpacing();
    } break;
    case frConstraintTypeEnum::frcLef58SpacingEndOfLineConstraint: {
      auto con = (frLef58SpacingEndOfLineConstraint*) constraint;
      eolWithin = con->getWithinConstraint()->getEolWithin();
      eolSpace = con->getEolSpace();
    } break;
    default:
      logger_->error(DRT, 226, "Unsupported endofline spacing rule");
      break;
  }

  if (edge->getDir() == frDirEnum::E) {
    bg::set<bg::min_corner, 0>(queryBox, edge->low().x() - eolWithin);
    bg::set<bg::min_corner, 1>(queryBox, edge->low().y() - eolSpace);
    bg::set<bg::max_corner, 0>(queryBox, edge->high().x() + eolWithin);
    bg::set<bg::max_corner, 1>(queryBox, edge->high().y());
  } else if (edge->getDir() == frDirEnum::W) {
    bg::set<bg::min_corner, 0>(queryBox, edge->high().x() - eolWithin);
    bg::set<bg::min_corner, 1>(queryBox, edge->high().y());
    bg::set<bg::max_corner, 0>(queryBox, edge->low().x() + eolWithin);
    bg::set<bg::max_corner, 1>(queryBox, edge->low().y() + eolSpace);
  } else if (edge->getDir() == frDirEnum::N) {
    bg::set<bg::min_corner, 0>(queryBox, edge->low().x());
    bg::set<bg::min_corner, 1>(queryBox, edge->low().y() - eolWithin);
    bg::set<bg::max_corner, 0>(queryBox, edge->high().x() + eolSpace);
    bg::set<bg::max_corner, 1>(queryBox, edge->high().y() + eolWithin);
  } else {  // S
    bg::set<bg::min_corner, 0>(queryBox, edge->high().x() - eolSpace);
    bg::set<bg::min_corner, 1>(queryBox, edge->high().y() - eolWithin);
    bg::set<bg::max_corner, 0>(queryBox, edge->low().x());
    bg::set<bg::max_corner, 1>(queryBox, edge->low().y() + eolWithin);
  }
  gtl::xl(queryRect, queryBox.min_corner().x());
  gtl::yl(queryRect, queryBox.min_corner().y());
  gtl::xh(queryRect, queryBox.max_corner().x());
  gtl::yh(queryRect, queryBox.max_corner().y());
}

void FlexGCWorker::Impl::checkMetalEndOfLine_eol_hasEol_helper(
    gcSegment* edge1,
    gcSegment* edge2,
    frConstraint* constraint)
{
  bool enableOutput = printMarker_;
  auto layerNum = edge1->getLayerNum();
  auto net1 = edge1->getNet();
  auto net2 = edge2->getNet();
  gtl::rectangle_data<frCoord> markerRect, rect2;
  checkMetalEndOfLine_eol_hasParallelEdge_oneDir_getParallelEdgeRect(
      edge1, markerRect);
  checkMetalEndOfLine_eol_hasParallelEdge_oneDir_getParallelEdgeRect(edge2,
                                                                     rect2);
  gtl::generalized_intersect(markerRect, rect2);
  // skip if markerRect contains anything
  auto& workerRegionQuery = getWorkerRegionQuery();
  vector<rq_box_value_t<gcRect*>> result;
  gtl::rectangle_data<frCoord> bloatMarkerRect(markerRect);
  if (gtl::area(markerRect) == 0) {
    if (edge1->getDir() == frDirEnum::W || edge1->getDir() == frDirEnum::E) {
      gtl::bloat(bloatMarkerRect, gtl::HORIZONTAL, 1);
    } else if (edge1->getDir() == frDirEnum::S
               || edge1->getDir() == frDirEnum::N) {
      gtl::bloat(bloatMarkerRect, gtl::VERTICAL, 1);
    }
  }
  box_t queryBox(point_t(gtl::xl(bloatMarkerRect), gtl::yl(bloatMarkerRect)),
                 point_t(gtl::xh(bloatMarkerRect), gtl::yh(bloatMarkerRect)));
  workerRegionQuery.queryMaxRectangle(queryBox, layerNum, result);
  for (auto& [objBox, objPtr] : result) {
    if (gtl::intersects(bloatMarkerRect, *objPtr, false)) {
      return;
    }
  }
  if (!checkMetalEndOfLine_eol_hasEncloseCut(edge1, edge2, constraint))
    return;

  auto marker = make_unique<frMarker>();
  frBox box(gtl::xl(markerRect),
            gtl::yl(markerRect),
            gtl::xh(markerRect),
            gtl::yh(markerRect));
  marker->setBBox(box);
  marker->setLayerNum(layerNum);
  marker->setConstraint(constraint);
  marker->addSrc(net1->getOwner());
  frCoord llx = min(edge1->getLowCorner()->x(), edge1->getHighCorner()->x());
  frCoord lly = min(edge1->getLowCorner()->y(), edge1->getHighCorner()->y());
  frCoord urx = max(edge1->getLowCorner()->x(), edge1->getHighCorner()->x());
  frCoord ury = max(edge1->getLowCorner()->y(), edge1->getHighCorner()->y());
  marker->addVictim(
      net1->getOwner(),
      make_tuple(
          edge1->getLayerNum(), frBox(llx, lly, urx, ury), edge1->isFixed()));
  marker->addSrc(net2->getOwner());
  llx = min(edge2->getLowCorner()->x(), edge2->getHighCorner()->x());
  lly = min(edge2->getLowCorner()->y(), edge2->getHighCorner()->y());
  urx = max(edge2->getLowCorner()->x(), edge2->getHighCorner()->x());
  ury = max(edge2->getLowCorner()->y(), edge2->getHighCorner()->y());
  marker->addAggressor(
      net2->getOwner(),
      make_tuple(
          edge2->getLayerNum(), frBox(llx, lly, urx, ury), edge2->isFixed()));
  if (addMarker(std::move(marker))) {
    // true marker
    if (enableOutput) {
      double dbu = getDesign()->getTopBlock()->getDBUPerUU();
      cout << "EOLSpc@(" << gtl::xl(markerRect) / dbu << ", "
           << gtl::yl(markerRect) / dbu << ") (" << gtl::xh(markerRect) / dbu
           << ", " << gtl::yh(markerRect) / dbu << ") "
           << getDesign()->getTech()->getLayer(layerNum)->getName() << " ";
      auto owner = net1->getOwner();
      if (owner == nullptr) {
        cout << "FLOATING";
      } else {
        if (owner->typeId() == frcNet) {
          cout << static_cast<frNet*>(owner)->getName();
        } else if (owner->typeId() == frcInstTerm) {
          cout << static_cast<frInstTerm*>(owner)->getInst()->getName() << "/"
               << static_cast<frInstTerm*>(owner)->getTerm()->getName();
        } else if (owner->typeId() == frcTerm) {
          cout << "PIN/" << static_cast<frTerm*>(owner)->getName();
        } else if (owner->typeId() == frcInstBlockage) {
          cout << static_cast<frInstBlockage*>(owner)->getInst()->getName()
               << "/OBS";
        } else if (owner->typeId() == frcBlockage) {
          cout << "PIN/OBS";
        } else {
          cout << "UNKNOWN";
        }
      }
      cout << " ";
      owner = net2->getOwner();
      if (owner == nullptr) {
        cout << "FLOATING";
      } else {
        if (owner->typeId() == frcNet) {
          cout << static_cast<frNet*>(owner)->getName();
        } else if (owner->typeId() == frcInstTerm) {
          cout << static_cast<frInstTerm*>(owner)->getInst()->getName() << "/"
               << static_cast<frInstTerm*>(owner)->getTerm()->getName();
        } else if (owner->typeId() == frcTerm) {
          cout << "PIN/" << static_cast<frTerm*>(owner)->getName();
        } else if (owner->typeId() == frcInstBlockage) {
          cout << static_cast<frInstBlockage*>(owner)->getInst()->getName()
               << "/OBS";
        } else if (owner->typeId() == frcBlockage) {
          cout << "PIN/OBS";
        } else {
          cout << "UNKNOWN";
        }
      }
      cout << endl;
    }
  }
}

void FlexGCWorker::Impl::checkMetalEndOfLine_eol_hasEol(
    gcSegment* edge,
    frConstraint* constraint,
    bool hasRoute)
{
  auto layerNum = edge->getLayerNum();
  box_t queryBox;
  gtl::rectangle_data<frCoord> queryRect;  // original size
  checkMetalEndOfLine_eol_hasEol_getQueryBox(
      edge, constraint, queryBox, queryRect);

  gtl::rectangle_data<frCoord> triggerRect;
  vector<pair<segment_t, gcSegment*>> results;
  auto& workerRegionQuery = getWorkerRegionQuery();
  workerRegionQuery.queryPolygonEdge(queryBox, edge->getLayerNum(), results);
  gtl::polygon_90_set_data<frCoord> tmpPoly;
  for (auto& [boostSeg, ptr] : results) {
    // skip if non oppo-dir edge
    if ((int) edge->getDir() + (int) ptr->getDir() != OPPOSITEDIR) {
      continue;
    }
    checkMetalEndOfLine_eol_hasParallelEdge_oneDir_getParallelEdgeRect(
        ptr, triggerRect);
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
      auto& polys = ptr->getNet()->getPolygons(layerNum, false);
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
    checkMetalEndOfLine_eol_hasEol_helper(edge, ptr, constraint);
  }
}

void FlexGCWorker::Impl::checkMetalEndOfLine_eol(gcSegment* edge,
                                                 frConstraint* constraint)
{
  if (!checkMetalEndOfLine_eol_isEolEdge(edge, constraint)) {
    return;
  }
  auto layerNum = edge->getLayerNum();
  // check left/right parallel edge
  // auto hasRoute = edge->isFixed() ? false : true;
  // check if current eol edge has route shapes
  bool hasRoute = false;
  if (!edge->isFixed()) {
    gtl::polygon_90_set_data<frCoord> tmpPoly;
    gtl::rectangle_data<frCoord> triggerRect;
    checkMetalEndOfLine_eol_hasParallelEdge_oneDir_getParallelEdgeRect(
        edge, triggerRect);
    // tmpPoly is the intersection of queryRect and minimum paralleledge rect
    using namespace boost::polygon::operators;
    tmpPoly += triggerRect;
    auto& polys = edge->getNet()->getPolygons(layerNum, false);
    // tmpPoly should have route shapes in order to be considered route
    tmpPoly &= polys;
    if (!gtl::empty(tmpPoly)) {
      hasRoute = true;
    }
  }
  auto triggered
      = checkMetalEndOfLine_eol_hasParallelEdge(edge, constraint, hasRoute);
  if (!triggered) {
    return;
  }
  triggered = checkMetalEndOfLine_eol_hasMinMaxLength(edge, constraint);
  if (!triggered) {
    return;
  }
  // check eol
  checkMetalEndOfLine_eol_hasEol(edge, constraint, hasRoute);
}

void FlexGCWorker::Impl::checkMetalEndOfLine_main(gcPin* pin)
{
  // bool enableOutput = true;

  auto poly = pin->getPolygon();
  auto layerNum = poly->getLayerNum();
  // auto net = poly->getNet();

  auto& cons = design_->getTech()->getLayer(layerNum)->getEolSpacing();
  auto lef58Cons = design_->getTech()
                       ->getLayer(layerNum)
                       ->getLef58SpacingEndOfLineConstraints();
  if (cons.empty() && lef58Cons.empty()) {
    return;
  }

  for (auto& edges : pin->getPolygonEdges()) {
    for (auto& edge : edges) {
      for (auto con : cons) {
        checkMetalEndOfLine_eol(edge.get(), con);
      }
      for (auto con : lef58Cons) {
        checkMetalEndOfLine_eol(edge.get(), con.get());
      }
    }
  }
}

void FlexGCWorker::Impl::checkMetalEndOfLine()
{
  if (targetNet_) {
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
      for (auto& pin : targetNet_->getPins(i)) {
        checkMetalEndOfLine_main(pin.get());
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
      for (auto& net : getNets()) {
        for (auto& pin : net->getPins(i)) {
          checkMetalEndOfLine_main(pin.get());
        }
      }
    }
  }
}