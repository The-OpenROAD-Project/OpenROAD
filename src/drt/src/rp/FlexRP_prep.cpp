// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <algorithm>
#include <cassert>
#include <climits>
#include <memory>
#include <utility>
#include <vector>

#include "boost/polygon/polygon.hpp"
#include "db/gcObj/gcNet.h"
#include "db/gcObj/gcPin.h"
#include "db/gcObj/gcShape.h"
#include "db/infra/frTime.h"
#include "db/obj/frVia.h"
#include "db/tech/frConstraint.h"
#include "db/tech/frLayer.h"
#include "db/tech/frViaDef.h"
#include "frBaseTypes.h"
#include "frProfileTask.h"
#include "gc/FlexGC.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "rp/FlexRP.h"
#include "utl/Logger.h"

using odb::dbTechLayerType;

namespace drt {

void FlexRP::prep()
{
  ProfileTask profile("RP:prep");
  prep_via2viaForbiddenLen();
  prep_viaForbiddenTurnLen();
  prep_viaForbiddenPlanarLen();
  prep_lineForbiddenLen();
  prep_eolForbiddenLen();
  prep_cutSpcTbl();
  prep_viaForbiddenThrough();
  for (const auto& ndr : tech_->nonDefaultRules_) {
    prep_via2viaForbiddenLen(ndr.get());
    prep_viaForbiddenTurnLen(ndr.get());
  }
  prep_minStepViasCheck();
}

void FlexRP::prep_minStepViasCheck()
{
  const auto bottomLayerNum = tech_->getBottomLayerNum();
  const auto topLayerNum = tech_->getTopLayerNum();
  for (auto lNum = bottomLayerNum; lNum <= topLayerNum; lNum++) {
    frLayer* layer = tech_->getLayer(lNum);
    if (layer->getType() != dbTechLayerType::ROUTING) {
      continue;
    }
    if (lNum - 2 < bottomLayerNum || lNum + 2 > topLayerNum) {
      continue;
    }
    const frViaDef* downVia = tech_->getLayer(lNum - 1)->getDefaultViaDef();
    const frViaDef* upVia = tech_->getLayer(lNum + 1)->getDefaultViaDef();
    if (!downVia || !upVia) {
      continue;
    }
    auto minStepCons = layer->getMinStepConstraint();
    if (!minStepCons) {
      continue;
    }

    const odb::Rect upViaBox = upVia->getLayer1ShapeBox();
    const odb::Rect downViaBox = downVia->getLayer2ShapeBox();
    const gtl::rectangle_data<frCoord> upViaRect(
        upViaBox.xMin(), upViaBox.yMin(), upViaBox.xMax(), upViaBox.yMax());
    const gtl::rectangle_data<frCoord> downViaRect(downViaBox.xMin(),
                                                   downViaBox.yMin(),
                                                   downViaBox.xMax(),
                                                   downViaBox.yMax());

    // joining the two via rects in one polygon
    gtl::polygon_90_set_data<frCoord> set;
    using boost::polygon::operators::operator+=;
    set += upViaRect;
    set += downViaRect;
    std::vector<gtl::polygon_90_with_holes_data<frCoord>> polys;
    set.get_polygons(polys);
    if (polys.size() != 1) {
      continue;
    }

    gtl::polygon_90_with_holes_data<frCoord> poly = *polys.begin();
    std::unique_ptr<gcNet> uTestNet = std::make_unique<gcNet>(0);
    gcNet* testNet = uTestNet.get();
    auto uTestPin = std::make_unique<gcPin>(poly, lNum, testNet);
    gcPin* testPin = uTestPin.get();
    testPin->setNet(testNet);

    bool first = true;
    std::vector<std::unique_ptr<gcSegment>> tmpEdges;
    gtl::point_data<frCoord> prev;
    for (const gtl::point_data<frCoord>& cur : poly) {
      if (first) {
        prev = cur;
        first = false;
      } else {
        auto edge = std::make_unique<gcSegment>();
        edge->setLayerNum(lNum);
        edge->addToPin(testPin);
        edge->addToNet(testNet);
        edge->setSegment(prev, cur);
        if (!tmpEdges.empty()) {
          edge->setPrevEdge(tmpEdges.back().get());
          tmpEdges.back()->setNextEdge(edge.get());
        }
        tmpEdges.push_back(std::move(edge));
        prev = cur;
      }
    }
    // last edge
    auto edge = std::make_unique<gcSegment>();
    edge->setLayerNum(lNum);
    edge->addToPin(testPin);
    edge->addToNet(testNet);
    edge->setSegment(prev, *poly.begin());
    edge->setPrevEdge(tmpEdges.back().get());
    tmpEdges.back()->setNextEdge(edge.get());
    // set first edge
    tmpEdges.front()->setPrevEdge(edge.get());
    edge->setNextEdge(tmpEdges.front().get());
    tmpEdges.push_back(std::move(edge));
    // add to polygon edges
    testPin->addPolygonEdges(tmpEdges);
    // check gc minstep violations
    FlexGCWorker worker(tech_, logger_, router_cfg_);
    worker.checkMinStep(testPin);
    const auto& markers = worker.getMarkers();
    if (!markers.empty()) {
      tech_->setVia2ViaMinStep(true);
      layer->setHasVia2ViaMinStepViol(true);
    }
  }
}

void FlexRP::prep_viaForbiddenThrough()
{
  const auto bottomLayerNum = tech_->getBottomLayerNum();
  const auto topLayerNum = tech_->getTopLayerNum();

  int i = 0;
  for (auto lNum = bottomLayerNum; lNum <= topLayerNum; lNum++) {
    if (tech_->getLayer(lNum)->getType() != dbTechLayerType::ROUTING) {
      continue;
    }
    const frViaDef* downVia = nullptr;
    const frViaDef* upVia = nullptr;
    if (tech_->getBottomLayerNum() <= lNum - 1) {
      downVia = tech_->getLayer(lNum - 1)->getDefaultViaDef();
    }
    if (tech_->getTopLayerNum() >= lNum + 1) {
      upVia = tech_->getLayer(lNum + 1)->getDefaultViaDef();
    }
    prep_viaForbiddenThrough_helper(lNum, i, 0, downVia, true);
    prep_viaForbiddenThrough_helper(lNum, i, 1, downVia, false);
    prep_viaForbiddenThrough_helper(lNum, i, 2, upVia, true);
    prep_viaForbiddenThrough_helper(lNum, i, 3, upVia, false);
    i++;
  }
}

void FlexRP::prep_viaForbiddenThrough_helper(const frLayerNum& lNum,
                                             const int& tableLayerIdx,
                                             const int& tableEntryIdx,
                                             const frViaDef* viaDef,
                                             const bool isCurrDirX)
{
  tech_->viaForbiddenThrough_[tableLayerIdx][tableEntryIdx]
      = prep_viaForbiddenThrough_minStep(lNum, viaDef, isCurrDirX);
}

bool FlexRP::prep_viaForbiddenThrough_minStep(const frLayerNum& lNum,
                                              const frViaDef* viaDef,
                                              const bool isCurrDirX)
{
  if (!viaDef) {
    return false;
  }

  return lNum == 10 && viaDef->getName() == "CK_23_28_0_26_VH_CK";
}

/**
 * Calculate the min end of line width in the eol rules.
 * If no eol rules found, consider default width.
 * @param[in] layer the layer we are calculating the min eol width for.
 * @return the min eol width from the eol rules -1 or the default width if no
 * eol rules found.
 */
frCoord getMinEol(const frLayer* layer, const frCoord minWidth)
{
  frCoord eol = INT_MAX;
  if (layer->hasEolSpacing()) {
    for (const auto con : layer->getEolSpacing()) {
      eol = std::min(eol, con->getEolWidth());
    }
  }
  for (const auto con : layer->getLef58SpacingEndOfLineConstraints()) {
    eol = std::min(eol, con->getEolWidth());
  }
  for (const auto con : layer->getLef58EolKeepOutConstraints()) {
    eol = std::min(eol, con->getEolWidth());
  }
  for (const auto con : layer->getLef58EolExtConstraints()) {
    eol = std::min(eol, con->getExtensionTable().getMinRow());
  }
  if (eol == INT_MAX) {
    eol = minWidth;
  } else {
    eol = std::max(eol - 1, minWidth);
  }
  return eol;
}

void FlexRP::prep_eolForbiddenLen_helper(const frLayer* layer,
                                         const frCoord eolWidth,
                                         frCoord& eolSpace,
                                         frCoord& eolWithin)
{
  if (layer->hasEolSpacing()) {
    for (const auto con : layer->getEolSpacing()) {
      if (eolWidth < con->getEolWidth()) {
        eolSpace = std::max(eolSpace, con->getMinSpacing());
        eolWithin = std::max(eolWithin, con->getEolWithin());
      }
    }
  }
  for (const auto con : layer->getLef58SpacingEndOfLineConstraints()) {
    if (eolWidth < con->getEolWidth()) {
      eolSpace = std::max(eolSpace, con->getEolSpace());
      if (con->hasWithinConstraint()) {
        const auto withinCon = con->getWithinConstraint();
        eolWithin = std::max(eolWithin, withinCon->getEolWithin());
        if (withinCon->hasEndToEndConstraint()) {
          const auto endToEndCon = withinCon->getEndToEndConstraint();
          eolSpace = std::max(eolSpace, endToEndCon->getEndToEndSpace());
        }
      }
    }
  }
  for (const auto con : layer->getLef58EolKeepOutConstraints()) {
    if (eolWidth < con->getEolWidth()) {
      eolSpace = std::max(eolSpace, con->getForwardExt());
      eolWithin = std::max(eolWithin, con->getSideExt());
    }
  }
  for (const auto con : layer->getLef58EolExtConstraints()) {
    if (eolWidth < con->getExtensionTable().getMaxRow()) {
      eolSpace = std::max(
          eolSpace,
          con->getExtensionTable().find(eolWidth) + con->getMinSpacing());
    }
  }
}

void FlexRP::prep_eolForbiddenLen()
{
  const auto bottomLayerNum = tech_->getBottomLayerNum();
  const auto topLayerNum = tech_->getTopLayerNum();

  for (auto lNum = bottomLayerNum; lNum <= topLayerNum; lNum++) {
    const auto layer = tech_->getLayer(lNum);
    if (layer->getType() != dbTechLayerType::ROUTING) {
      continue;
    }
    frCoord eolSpace = 0;
    frCoord eolWithin = 0;
    frCoord eolWidth = getMinEol(layer, layer->getWidth());
    prep_eolForbiddenLen_helper(layer, eolWidth, eolSpace, eolWithin);
    layer->setDrEolSpacingConstraint(eolWidth, eolSpace, eolWithin);
  }
  for (const auto& ndr : tech_->getNondefaultRules()) {
    for (auto lNum = bottomLayerNum; lNum <= topLayerNum; lNum++) {
      const auto layer = tech_->getLayer(lNum);
      if (layer->getType() != dbTechLayerType::ROUTING) {
        continue;
      }
      const auto z = lNum / 2 - 1;
      const frCoord minWidth = ndr->getWidth(z);
      if (minWidth == 0) {
        continue;
      }
      frCoord eolSpace = 0;
      frCoord eolWithin = 0;
      frCoord eolWidth = getMinEol(layer, minWidth);
      prep_eolForbiddenLen_helper(layer, eolWidth, eolSpace, eolWithin);
      drEolSpacingConstraint drCon(eolWidth, eolSpace, eolWithin);
      ndr->setDrEolConstraint(drCon, z);
    }
  }
}

void FlexRP::prep_cutSpcTbl()
{
  const auto bottomLayerNum = tech_->getBottomLayerNum();
  const auto topLayerNum = tech_->getTopLayerNum();

  for (auto lNum = bottomLayerNum; lNum <= topLayerNum; lNum++) {
    const auto layer = tech_->getLayer(lNum);
    if (layer->getType() == odb::dbTechLayerType::CUT) {
      auto viaDef = layer->getDefaultViaDef();
      if (viaDef == nullptr) {
        continue;
      }
      frVia via(viaDef);
      odb::Rect tmpBx = via.getCutBBox();
      frString cutClass1;
      const auto cutClassIdx1
          = layer->getCutClassIdx(tmpBx.minDXDY(), tmpBx.maxDXDY());
      if (cutClassIdx1 >= 0) {
        cutClass1 = layer->getCutClass(cutClassIdx1)->getName();
      }
      if (layer->hasLef58DiffNetCutSpcTblConstraint()) {
        const auto con = layer->getLef58DiffNetCutSpcTblConstraint();
        const auto dbRule = con->getODBRule();
        con->setDefaultSpacing(
            {dbRule->getMaxSpacing(
                 cutClass1,
                 cutClass1,
                 odb::dbTechLayerCutSpacingTableDefRule::FIRST),
             dbRule->getMaxSpacing(
                 cutClass1,
                 cutClass1,
                 odb::dbTechLayerCutSpacingTableDefRule::SECOND)});
        con->setDefaultCenterToCenter(
            dbRule->isCenterToCenter(cutClass1, cutClass1));
        con->setDefaultCenterAndEdge(
            dbRule->isCenterAndEdge(cutClass1, cutClass1));
      }
      if (layer->hasLef58DefaultInterCutSpcTblConstraint()) {
        const auto con = layer->getLef58DefaultInterCutSpcTblConstraint();
        const auto dbRule = con->getODBRule();
        const auto secondLayer
            = tech_->getLayer(dbRule->getSecondLayer()->getName());
        viaDef = secondLayer->getDefaultViaDef();
        if (viaDef != nullptr) {
          tmpBx = via.getCutBBox();
          frString cutClass2;
          const auto cutClassIdx2
              = secondLayer->getCutClassIdx(tmpBx.minDXDY(), tmpBx.maxDXDY());
          if (cutClassIdx2 >= 0) {
            cutClass2 = secondLayer->getCutClass(cutClassIdx2)->getName();
          }
          con->setDefaultSpacing(
              {dbRule->getMaxSpacing(
                   cutClass1,
                   cutClass2,
                   odb::dbTechLayerCutSpacingTableDefRule::FIRST),
               dbRule->getMaxSpacing(
                   cutClass1,
                   cutClass2,
                   odb::dbTechLayerCutSpacingTableDefRule::SECOND)});
          con->setDefaultCenterToCenter(
              dbRule->isCenterToCenter(cutClass1, cutClass2));
          con->setDefaultCenterAndEdge(
              dbRule->isCenterAndEdge(cutClass1, cutClass2));
        }
      }
    }
  }
}

void FlexRP::prep_lineForbiddenLen()
{
  const auto bottomLayerNum = tech_->getBottomLayerNum();
  const auto topLayerNum = tech_->getTopLayerNum();

  int i = 0;
  for (auto lNum = bottomLayerNum; lNum <= topLayerNum; lNum++) {
    if (tech_->getLayer(lNum)->getType() != dbTechLayerType::ROUTING) {
      continue;
    }
    prep_lineForbiddenLen_helper(lNum, i, 0, true, true);
    prep_lineForbiddenLen_helper(lNum, i, 1, true, false);
    prep_lineForbiddenLen_helper(lNum, i, 2, false, true);
    prep_lineForbiddenLen_helper(lNum, i, 3, false, false);

    i++;
  }
}

void FlexRP::prep_lineForbiddenLen_helper(const frLayerNum& lNum,
                                          const int& tableLayerIdx,
                                          const int& tableEntryIdx,
                                          const bool isZShape,
                                          const bool isCurrDirX)
{
  ForbiddenRanges forbiddenRanges;
  prep_lineForbiddenLen_minSpc(lNum, isZShape, isCurrDirX, forbiddenRanges);

  // merge ranges
  boost::icl::interval_set<frCoord> forbiddenIntvSet;
  for (auto& range : forbiddenRanges) {
    forbiddenIntvSet.insert(
        boost::icl::interval<frCoord>::closed(range.first, range.second));
  }

  forbiddenRanges.clear();
  for (const auto& interval : forbiddenIntvSet) {
    const auto beginCoord = interval.lower();
    const auto endCoord = interval.upper();
    forbiddenRanges.emplace_back(beginCoord + 1, endCoord - 1);
  }

  tech_->line2LineForbiddenLen_[tableLayerIdx][tableEntryIdx]
      = std::move(forbiddenRanges);
}

void FlexRP::prep_lineForbiddenLen_minSpc(const frLayerNum& lNum,
                                          const bool isZShape,
                                          const bool isCurrDirX,
                                          ForbiddenRanges& forbiddenRanges)
{
  const frCoord defaultWidth = tech_->getLayer(lNum)->getWidth();

  const frCoord minNonOverlapDist = defaultWidth;

  frCoord minReqDist = INT_MIN;
  const frCoord prl
      = isZShape ? defaultWidth : tech_->getLayer(lNum)->getPitch();
  const auto con = tech_->getLayer(lNum)->getMinSpacing();
  if (con) {
    if (con->typeId() == frConstraintTypeEnum::frcSpacingConstraint) {
      minReqDist = static_cast<frSpacingConstraint*>(con)->getMinSpacing();
    } else if (con->typeId()
               == frConstraintTypeEnum::frcSpacingTablePrlConstraint) {
      minReqDist = static_cast<frSpacingTablePrlConstraint*>(con)->find(
          defaultWidth, prl);
    } else if (con->typeId()
               == frConstraintTypeEnum::frcSpacingTableTwConstraint) {
      minReqDist = static_cast<frSpacingTableTwConstraint*>(con)->find(
          defaultWidth, defaultWidth, prl);
    }
  }
  if (minReqDist != INT_MIN) {
    minReqDist += minNonOverlapDist;
  }
  if (minReqDist != INT_MIN) {
    forbiddenRanges.emplace_back(minNonOverlapDist, minReqDist);
  }
}

void FlexRP::prep_viaForbiddenPlanarLen()
{
  const auto bottomLayerNum = tech_->getBottomLayerNum();
  const auto topLayerNum = tech_->getTopLayerNum();

  int i = 0;
  for (auto lNum = bottomLayerNum; lNum <= topLayerNum; lNum++) {
    if (tech_->getLayer(lNum)->getType() != dbTechLayerType::ROUTING) {
      continue;
    }
    const frViaDef* downVia = nullptr;
    const frViaDef* upVia = nullptr;
    if (tech_->getBottomLayerNum() <= lNum - 1) {
      downVia = tech_->getLayer(lNum - 1)->getDefaultViaDef();
    }
    if (tech_->getTopLayerNum() >= lNum + 1) {
      upVia = tech_->getLayer(lNum + 1)->getDefaultViaDef();
    }
    prep_viaForbiddenPlanarLen_helper(lNum, i, 0, downVia, true);
    prep_viaForbiddenPlanarLen_helper(lNum, i, 1, downVia, false);
    prep_viaForbiddenPlanarLen_helper(lNum, i, 2, upVia, true);
    prep_viaForbiddenPlanarLen_helper(lNum, i, 3, upVia, false);

    i++;
  }
}

void FlexRP::prep_viaForbiddenPlanarLen_helper(const frLayerNum& lNum,
                                               const int& tableLayerIdx,
                                               const int& tableEntryIdx,
                                               const frViaDef* viaDef,
                                               bool isCurrDirX)
{
  if (!viaDef) {
    return;
  }

  ForbiddenRanges forbiddenRanges;
  prep_viaForbiddenPlanarLen_minStep(lNum, viaDef, isCurrDirX, forbiddenRanges);

  // merge forbidden ranges
  boost::icl::interval_set<frCoord> forbiddenIntvSet;
  for (const auto& range : forbiddenRanges) {
    forbiddenIntvSet.insert(
        boost::icl::interval<frCoord>::closed(range.first, range.second));
  }

  forbiddenRanges.clear();
  for (const auto& interval : forbiddenIntvSet) {
    const auto beginCoord = interval.lower();
    const auto endCoord = interval.upper();
    forbiddenRanges.emplace_back(beginCoord + 1, endCoord - 1);
  }

  tech_->viaForbiddenPlanarLen_[tableLayerIdx][tableEntryIdx]
      = std::move(forbiddenRanges);
}

void FlexRP::prep_viaForbiddenPlanarLen_minStep(
    const frLayerNum& lNum,
    const frViaDef* viaDef,
    const bool isCurrDirX,
    ForbiddenRanges& forbiddenRanges)
{
}

void FlexRP::prep_viaForbiddenTurnLen(frNonDefaultRule* ndr)
{
  const auto bottomLayerNum = tech_->getBottomLayerNum();
  const auto topLayerNum = tech_->getTopLayerNum();
  const int bottom = router_cfg_->BOTTOM_ROUTING_LAYER;
  int i = 0;
  for (auto lNum = bottomLayerNum; lNum <= topLayerNum; lNum++) {
    if (tech_->getLayer(lNum)->getType() != dbTechLayerType::ROUTING) {
      continue;
    }
    const frViaDef* downVia = nullptr;
    const frViaDef* upVia = nullptr;
    if (ndr && bottom < lNum && ndr->getPrefVia((lNum - 2) / 2 - 1)) {
      downVia = ndr->getPrefVia((lNum - 2) / 2 - 1);
    } else if (tech_->getBottomLayerNum() <= lNum - 1) {
      downVia = tech_->getLayer(lNum - 1)->getDefaultViaDef();
    }

    if (tech_->getTopLayerNum() >= lNum + 1) {
      if (ndr && ndr->getPrefVia(lNum / 2 - 1)) {
        upVia = ndr->getPrefVia(lNum / 2 - 1);
      } else {
        upVia = tech_->getLayer(lNum + 1)->getDefaultViaDef();
      }
    }
    prep_viaForbiddenTurnLen_helper(lNum, i, 0, downVia, true, ndr);
    prep_viaForbiddenTurnLen_helper(lNum, i, 1, downVia, false, ndr);
    prep_viaForbiddenTurnLen_helper(lNum, i, 2, upVia, true, ndr);
    prep_viaForbiddenTurnLen_helper(lNum, i, 3, upVia, false, ndr);

    i++;
  }
}

// forbidden turn length range from via
void FlexRP::prep_viaForbiddenTurnLen_helper(const frLayerNum& lNum,
                                             const int& tableLayerIdx,
                                             const int& tableEntryIdx,
                                             const frViaDef* viaDef,
                                             const bool isCurrDirX,
                                             frNonDefaultRule* ndr)
{
  if (!viaDef) {
    return;
  }

  auto tech = tech_;

  ForbiddenRanges forbiddenRanges;
  prep_viaForbiddenTurnLen_minSpc(
      lNum, viaDef, isCurrDirX, forbiddenRanges, ndr);

  // merge forbidden ranges
  boost::icl::interval_set<frCoord> forbiddenIntvSet;
  for (auto& range : forbiddenRanges) {
    forbiddenIntvSet.insert(
        boost::icl::interval<frCoord>::closed(range.first, range.second));
  }

  forbiddenRanges.clear();
  for (const auto& interval : forbiddenIntvSet) {
    const auto beginCoord = interval.lower();
    const auto endCoord = interval.upper();
    forbiddenRanges.emplace_back(beginCoord + 1, endCoord - 1);
  }
  if (ndr) {
    ndr->viaForbiddenTurnLen_[tableLayerIdx][tableEntryIdx]
        = std::move(forbiddenRanges);
  } else {
    tech->viaForbiddenTurnLen_[tableLayerIdx][tableEntryIdx]
        = std::move(forbiddenRanges);
  }
}

void FlexRP::prep_viaForbiddenTurnLen_minSpc(const frLayerNum& lNum,
                                             const frViaDef* viaDef,
                                             const bool isCurrDirX,
                                             ForbiddenRanges& forbiddenRanges,
                                             frNonDefaultRule* ndr)
{
  if (!viaDef) {
    return;
  }

  const frCoord defaultWidth = tech_->getLayer(lNum)->getWidth();
  frCoord width = defaultWidth;
  if (ndr) {
    width = std::max(width, ndr->getWidth(lNum / 2 - 1));
  }

  const frVia via1(viaDef);
  odb::Rect viaBox1;
  if (viaDef->getLayer1Num() == lNum) {
    viaBox1 = via1.getLayer1BBox();
  } else {
    viaBox1 = via1.getLayer2BBox();
  }
  const int width1 = viaBox1.minDXDY();
  const bool isVia1Fat = isCurrDirX ? (viaBox1.dy() > defaultWidth)
                                    : (viaBox1.dx() > defaultWidth);
  const auto prl1 = isCurrDirX ? viaBox1.dy() : viaBox1.dx();

  const frCoord minNonOverlapDist = isCurrDirX ? ((viaBox1.dx() + width) / 2)
                                               : ((viaBox1.dy() + width) / 2);
  frCoord minReqDist = INT_MIN;
  if (isVia1Fat || ndr) {
    const auto con = tech_->getLayer(lNum)->getMinSpacing();
    if (con) {
      if (con->typeId() == frConstraintTypeEnum::frcSpacingConstraint) {
        minReqDist = static_cast<frSpacingConstraint*>(con)->getMinSpacing();
      } else if (con->typeId()
                 == frConstraintTypeEnum::frcSpacingTablePrlConstraint) {
        minReqDist = static_cast<frSpacingTablePrlConstraint*>(con)->find(
            std::max(width1, width), prl1);
      } else if (con->typeId()
                 == frConstraintTypeEnum::frcSpacingTableTwConstraint) {
        minReqDist = static_cast<frSpacingTableTwConstraint*>(con)->find(
            width1, width, prl1);
      }
    }
    if (ndr) {
      minReqDist = std::max(minReqDist, ndr->getSpacing(lNum / 2 - 1));
    }
    if (minReqDist != INT_MIN) {
      minReqDist += minNonOverlapDist;
    }
  }
  if (minReqDist != INT_MIN) {
    forbiddenRanges.emplace_back(minNonOverlapDist, minReqDist);
  }
}

void FlexRP::prep_via2viaForbiddenLen(frNonDefaultRule* ndr)
{
  const auto bottomLayerNum = tech_->getBottomLayerNum();
  const auto topLayerNum = tech_->getTopLayerNum();
  const int bottom = router_cfg_->BOTTOM_ROUTING_LAYER;
  int i = 0;
  for (auto lNum = bottomLayerNum; lNum <= topLayerNum; lNum++) {
    if (tech_->getLayer(lNum)->getType() != dbTechLayerType::ROUTING) {
      continue;
    }
    const frViaDef* downVia = nullptr;
    const frViaDef* upVia = nullptr;
    if (ndr && bottom < lNum && ndr->getPrefVia((lNum - 2) / 2 - 1)) {
      downVia = ndr->getPrefVia((lNum - 2) / 2 - 1);
    } else if (tech_->getBottomLayerNum() <= lNum - 1) {
      downVia = tech_->getLayer(lNum - 1)->getDefaultViaDef();
    }
    if (tech_->getTopLayerNum() >= lNum + 1) {
      if (ndr && ndr->getPrefVia(lNum / 2 - 1)) {
        upVia = ndr->getPrefVia(lNum / 2 - 1);
      } else {
        upVia = tech_->getLayer(lNum + 1)->getDefaultViaDef();
      }
    }
    prep_via2viaForbiddenLen_helper(lNum, i, 0, downVia, downVia, true, ndr);
    prep_via2viaForbiddenLen_helper(lNum, i, 1, downVia, downVia, false, ndr);
    prep_via2viaForbiddenLen_helper(lNum, i, 2, downVia, upVia, true, ndr);
    prep_via2viaForbiddenLen_helper(lNum, i, 3, downVia, upVia, false, ndr);
    prep_via2viaForbiddenLen_helper(lNum, i, 4, upVia, downVia, true, ndr);
    prep_via2viaForbiddenLen_helper(lNum, i, 5, upVia, downVia, false, ndr);
    prep_via2viaForbiddenLen_helper(lNum, i, 6, upVia, upVia, true, ndr);
    prep_via2viaForbiddenLen_helper(lNum, i, 7, upVia, upVia, false, ndr);

    i++;
  }
}

// assume via is always centered at (0,0) for shapes on all three layers
// currently only support single-cut via and min-square cut class
void FlexRP::prep_via2viaForbiddenLen_helper(const frLayerNum& lNum,
                                             const int& tableLayerIdx,
                                             const int& tableEntryIdx,
                                             const frViaDef* viaDef1,
                                             const frViaDef* viaDef2,
                                             const bool isHorizontal,
                                             frNonDefaultRule* ndr)
{
  auto tech = tech_;
  // non-shape-based rule
  ForbiddenRanges forbiddenRanges;
  prep_via2viaForbiddenLen_minSpc(
      lNum, viaDef1, viaDef2, isHorizontal, forbiddenRanges, ndr);
  prep_via2viaForbiddenLen_minimumCut(
      lNum, viaDef1, viaDef2, isHorizontal, forbiddenRanges);
  prep_via2viaForbiddenLen_widthViaMap(
      lNum, viaDef1, viaDef2, isHorizontal, forbiddenRanges);
  prep_via2viaForbiddenLen_cutSpc(
      lNum, viaDef1, viaDef2, isHorizontal, forbiddenRanges);
  prep_via2viaForbiddenLen_lef58CutSpc(
      lNum, viaDef1, viaDef2, isHorizontal, forbiddenRanges);
  prep_via2viaForbiddenLen_lef58CutSpcTbl(
      lNum, viaDef1, viaDef2, isHorizontal, forbiddenRanges);
  prep_via2viaForbiddenLen_minStep(
      lNum, viaDef1, viaDef2, !isHorizontal, forbiddenRanges);

  // merge forbidden ranges
  boost::icl::interval_set<frCoord> forbiddenIntvSet;
  for (auto& range : forbiddenRanges) {
    forbiddenIntvSet.insert(
        boost::icl::interval<frCoord>::closed(range.first, range.second));
  }

  forbiddenRanges.clear();
  for (const auto& interval : forbiddenIntvSet) {
    const auto beginCoord = interval.lower();
    const auto endCoord = interval.upper();
    forbiddenRanges.emplace_back(beginCoord, endCoord);
  }
  if (ndr) {
    ndr->via2ViaForbiddenLen_[tableLayerIdx][tableEntryIdx]
        = std::move(forbiddenRanges);
  } else {
    tech->via2ViaForbiddenLen_[tableLayerIdx][tableEntryIdx]
        = std::move(forbiddenRanges);
  }

  if (!ndr) {
    frCoord prl = 0;
    prep_via2viaPRL(lNum, viaDef1, viaDef2, isHorizontal, prl);
    tech->via2ViaPrlLen_[tableLayerIdx][tableEntryIdx] = prl;
  }
}

bool FlexRP::hasMinStepViol(const odb::Rect& r1,
                            const odb::Rect& r2,
                            const frLayerNum lNum)
{
  const gtl::rectangle_data<frCoord> rect1(
      r1.xMin(), r1.yMin(), r1.xMax(), r1.yMax());
  const gtl::rectangle_data<frCoord> rect2(
      r2.xMin(), r2.yMin(), r2.xMax(), r2.yMax());

  // joining the two via rects in one polygon
  gtl::polygon_90_set_data<frCoord> set;
  using boost::polygon::operators::operator+=;
  set += rect1;
  set += rect2;
  std::vector<gtl::polygon_90_with_holes_data<frCoord>> polys;
  set.get_polygons(polys);
  if (polys.size() != 1) {
    return false;
  }

  gtl::polygon_90_with_holes_data<frCoord> poly = *polys.begin();
  std::unique_ptr<gcNet> uTestNet = std::make_unique<gcNet>(0);
  gcNet* testNet = uTestNet.get();
  std::unique_ptr<gcPin> uTestPin
      = std::make_unique<gcPin>(poly, lNum, testNet);
  gcPin* testPin = uTestPin.get();
  testPin->setNet(testNet);

  bool first = true;
  std::vector<std::unique_ptr<gcSegment>> tmpEdges;
  gtl::point_data<frCoord> prev;
  for (const gtl::point_data<frCoord>& cur : poly) {
    if (first) {
      prev = cur;
      first = false;
    } else {
      auto edge = std::make_unique<gcSegment>();
      edge->setLayerNum(lNum);
      edge->addToPin(testPin);
      edge->addToNet(testNet);
      edge->setSegment(prev, cur);
      if (!tmpEdges.empty()) {
        edge->setPrevEdge(tmpEdges.back().get());
        tmpEdges.back()->setNextEdge(edge.get());
      }
      tmpEdges.push_back(std::move(edge));
      prev = cur;
    }
  }
  // last edge
  auto edge = std::make_unique<gcSegment>();
  edge->setLayerNum(lNum);
  edge->addToPin(testPin);
  edge->addToNet(testNet);
  edge->setSegment(prev, *poly.begin());
  edge->setPrevEdge(tmpEdges.back().get());
  tmpEdges.back()->setNextEdge(edge.get());
  // set first edge
  tmpEdges.front()->setPrevEdge(edge.get());
  edge->setNextEdge(tmpEdges.front().get());
  tmpEdges.push_back(std::move(edge));
  // add to polygon edges
  testPin->addPolygonEdges(tmpEdges);
  // check gc minstep violations
  FlexGCWorker worker(tech_, logger_, router_cfg_);
  worker.checkMinStep(testPin);
  return !worker.getMarkers().empty();
}

void FlexRP::prep_via2viaForbiddenLen_minStep(const frLayerNum& lNum,
                                              const frViaDef* viaDef1,
                                              const frViaDef* viaDef2,
                                              const bool isVertical,
                                              ForbiddenRanges& forbiddenRanges)
{
  if (!viaDef1 || !viaDef2) {
    return;
  }
  const frMinStepConstraint* con
      = tech_->getLayer(lNum)->getMinStepConstraint();
  if (!con) {
    return;
  }
  if (viaDef1->getLayer1Num() == viaDef2->getLayer1Num()) {
    return;
  }
  odb::Rect enclosureBox1, enclosureBox2;
  const frVia via1(viaDef1);
  const frVia via2(viaDef2);
  if (viaDef1->getLayer1Num() == lNum) {
    enclosureBox1 = via1.getLayer1BBox();
    enclosureBox2 = via2.getLayer2BBox();
  } else {
    enclosureBox1 = via1.getLayer2BBox();
    enclosureBox2 = via2.getLayer1BBox();
  }
  odb::Rect *shifting, *other;
  // get the rect with lesser width (the shifting one)
  if (isVertical) {
    if (enclosureBox1.dx() < enclosureBox2.dx()) {
      shifting = &enclosureBox1;
      other = &enclosureBox2;
    } else if (enclosureBox2.dx() < enclosureBox1.dx()) {
      shifting = &enclosureBox2;
      other = &enclosureBox1;
    } else {
      return;
    }
  } else {
    if (enclosureBox1.dy() < enclosureBox2.dy()) {
      shifting = &enclosureBox1;
      other = &enclosureBox2;
    } else if (enclosureBox2.dy() < enclosureBox1.dy()) {
      shifting = &enclosureBox2;
      other = &enclosureBox1;
    } else {
      return;
    }
  }
  // example where shifting is vertical and other is horizontal
  //      shiftingHigh
  //      _____
  //     |     | <-shiftingEdge (the vertical one)
  //   __|     |__ otherEdge (the horizontal one)
  //  |           |
  //  |__       __|
  //     |     |
  //     |_____|
  //     shiftingLow
  int shiftingEdge, otherEdge, shiftingLow, otherLow, otherHigh, minRange = 0;
  if (other->contains(*shifting)) {
    if (isVertical) {
      minRange = other->yMax() - shifting->yMax() + 1;
      shifting->moveDelta(0, minRange);
    } else {
      minRange = other->xMax() - shifting->xMax() + 1;
      shifting->moveDelta(minRange, 0);
    }
  }
  if (isVertical) {
    shiftingEdge = shifting->yMax() - other->yMax();
    otherEdge = other->xMax() - shifting->xMax();
    shiftingLow = shifting->yMin();
    otherLow = other->yMin();
    otherHigh = other->yMax();
  } else {
    shiftingEdge = shifting->xMax() - other->xMax();
    otherEdge = other->yMax() - shifting->yMax();
    shiftingLow = shifting->xMin();
    otherLow = other->xMin();
    otherHigh = other->xMax();
  }
  int shift;
  if (hasMinStepViol(*shifting, *other, lNum)) {
    if (shiftingEdge < con->getMinStepLength()) {
      shift = con->getMinStepLength() - shiftingEdge - 1;
      if (shiftingLow < otherLow) {
        shift = std::max(shift, otherLow - shiftingLow - 1);
      }
      if (isVertical) {
        shifting->moveDelta(0, shift + 1);
      } else {
        shifting->moveDelta(shift + 1, 0);
      }
      if (hasMinStepViol(*shifting, *other, lNum)) {
        shift = otherHigh - shiftingLow;
      }
    } else {
      assert(otherEdge < con->getMinStepLength());
      shift = otherHigh - shiftingLow;
    }
  } else {
    if (shiftingEdge < con->getMinStepLength()) {
      if (con->getMaxLength() > 0) {
        int div = 2;
        int length = shiftingEdge;
        const int topEdge_shifting
            = isVertical ? shifting->dx() : shifting->dy();
        const int topEdge_other = isVertical ? other->dy() : other->dx();
        if (topEdge_shifting < con->getMinStepLength()) {
          length += topEdge_shifting + shiftingEdge;
          if (otherEdge < con->getMinStepLength()) {
            length += 2 * otherEdge;
            if (topEdge_other < con->getMinStepLength()) {
              return;
            }
          }
        } else if (otherEdge < con->getMinStepLength()) {
          length += otherEdge;
          div = 1;
          if (topEdge_other < con->getMinStepLength()) {
            length += topEdge_other + otherEdge + shiftingEdge;
          }
        }
        shift = (con->getMaxLength() - length) / div + 1;
        if (shift < 0) {
          return;
        }
        if (isVertical) {
          shifting->moveDelta(0, shift);
        } else {
          shifting->moveDelta(shift, 0);
        }
        if (hasMinStepViol(*shifting, *other, lNum)) {
          minRange = shift;
          shift = otherHigh - shiftingLow;
        } else {
          return;
        }
      } else {
        return;
      }
    } else {
      minRange = otherLow - shiftingLow - con->getMinStepLength() + 1;
      if (isVertical) {
        shifting->moveDelta(0, minRange);
      } else {
        shifting->moveDelta(minRange, 0);
      }
      if (hasMinStepViol(*shifting, *other, lNum)) {
        shift = con->getMinStepLength() - 2;
        if (isVertical) {
          shifting->moveDelta(0, shift + 1);
        } else {
          shifting->moveDelta(shift + 1, 0);
        }
        if (hasMinStepViol(*shifting, *other, lNum)) {
          shift = otherHigh - shiftingLow;
        }
      } else {
        return;
      }
    }
  }
  forbiddenRanges.emplace_back(minRange - 1, minRange + shift + 1);
}

// only partial support of GF14
void FlexRP::prep_via2viaForbiddenLen_lef58CutSpc(
    const frLayerNum& lNum,
    const frViaDef* viaDef1,
    const frViaDef* viaDef2,
    const bool isCurrDirX,
    ForbiddenRanges& forbiddenRanges)
{
  if (!viaDef1 || !viaDef2) {
    return;
  }

  if (router_cfg_->DBPROCESSNODE != "GF14_13M_3Mx_2Cx_4Kx_2Hx_2Gx_LB") {
    return;
  }

  const bool isCurrDirY = !isCurrDirX;
  if (lNum != 10 || !isCurrDirY) {
    return;
  }

  const bool match12
      = (viaDef1->getLayer1Num() == lNum) && (viaDef2->getLayer2Num() == lNum);
  const bool match21
      = (viaDef1->getLayer2Num() == lNum) && (viaDef2->getLayer1Num() == lNum);
  if (!match12 && !match21) {
    return;
  }

  odb::Rect enclosureBox1;
  const frVia via1(viaDef1);
  const frVia via2(viaDef2);
  if (viaDef1->getLayer1Num() == lNum) {
    enclosureBox1 = via1.getLayer1BBox();
  } else {
    enclosureBox1 = via1.getLayer2BBox();
  }
  odb::Rect enclosureBox2;
  if (viaDef2->getLayer1Num() == lNum) {
    enclosureBox2 = via2.getLayer1BBox();
  } else {
    enclosureBox2 = via2.getLayer2BBox();
  }
  const odb::Rect cutBox1 = via1.getCutBBox();
  const odb::Rect cutBox2 = via2.getCutBBox();
  std::pair<frCoord, frCoord> range;
  frCoord reqSpcVal = 0;
  // check via1 cut layer to lNum
  const auto via1CutLNum = viaDef1->getCutLayerNum();
  if (!tech_->getLayer(via1CutLNum)
           ->getLef58CutSpacingConstraints(false)
           .empty()) {
    for (const auto con :
         tech_->getLayer(via1CutLNum)->getLef58CutSpacingConstraints(false)) {
      if (con->getSecondLayerNum() != lNum) {
        continue;
      }
      if (!con->isConcaveCorner()) {
        continue;
      }
      reqSpcVal = con->getCutSpacing();
      prep_via2viaForbiddenLen_lef58CutSpc_helper(
          enclosureBox1, enclosureBox2, cutBox1, reqSpcVal, range);
      forbiddenRanges.push_back(range);
    }
  } else {
    for (const auto con :
         tech_->getLayer(via1CutLNum)->getLef58CutSpacingConstraints(true)) {
      if (con->getSecondLayerNum() != lNum) {
        continue;
      }
      if (!con->isConcaveCorner()) {
        continue;
      }
      reqSpcVal = con->getCutSpacing();
      prep_via2viaForbiddenLen_lef58CutSpc_helper(
          enclosureBox1, enclosureBox2, cutBox1, reqSpcVal, range);
      forbiddenRanges.push_back(range);
    }
  }

  // check via2 cut layer to lNum
  const auto via2CutLNum = viaDef2->getCutLayerNum();
  if (!tech_->getLayer(via2CutLNum)
           ->getLef58CutSpacingConstraints(false)
           .empty()) {
    for (const auto con :
         tech_->getLayer(via2CutLNum)->getLef58CutSpacingConstraints(false)) {
      if (con->getSecondLayerNum() != lNum) {
        continue;
      }
      if (!con->isConcaveCorner()) {
        continue;
      }
      reqSpcVal = con->getCutSpacing();
      prep_via2viaForbiddenLen_lef58CutSpc_helper(
          enclosureBox1, enclosureBox2, cutBox2, reqSpcVal, range);
      forbiddenRanges.push_back(range);
    }
  } else {
    for (const auto con :
         tech_->getLayer(via2CutLNum)->getLef58CutSpacingConstraints(true)) {
      if (con->getSecondLayerNum() != lNum) {
        continue;
      }
      if (!con->isConcaveCorner()) {
        continue;
      }
      reqSpcVal = con->getCutSpacing();
      prep_via2viaForbiddenLen_lef58CutSpc_helper(
          enclosureBox1, enclosureBox2, cutBox2, reqSpcVal, range);
      forbiddenRanges.push_back(range);
    }
  }
}

void FlexRP::prep_via2viaForbiddenLen_lef58CutSpcTbl(
    const frLayerNum& lNum,
    const frViaDef* viaDef1,
    const frViaDef* viaDef2,
    const bool isCurrDirX,
    ForbiddenRanges& forbiddenRanges)
{
  if (!viaDef1 || !viaDef2) {
    return;
  }
  const bool swapped = viaDef2->getCutLayerNum() > viaDef1->getCutLayerNum();
  if (swapped) {
    std::swap(viaDef2, viaDef1);
  }
  const bool isCurrDirY = !isCurrDirX;
  const frVia via1(viaDef1);
  odb::Rect viaBox1;
  if (viaDef1->getLayer1Num() == lNum) {
    viaBox1 = via1.getLayer1BBox();
  } else {
    viaBox1 = via1.getLayer2BBox();
  }

  const frVia via2(viaDef2);
  odb::Rect viaBox2;
  if (viaDef2->getLayer1Num() == lNum) {
    viaBox2 = via2.getLayer1BBox();
  } else {
    viaBox2 = via2.getLayer2BBox();
  }
  const odb::Rect cutBox1 = via1.getCutBBox();
  const odb::Rect cutBox2 = via2.getCutBBox();
  const auto layer1 = tech_->getLayer(viaDef1->getCutLayerNum());
  const auto layer2 = tech_->getLayer(viaDef2->getCutLayerNum());
  const auto cutClassIdx1
      = layer1->getCutClassIdx(cutBox1.minDXDY(), cutBox1.maxDXDY());
  const auto cutClassIdx2
      = layer2->getCutClassIdx(cutBox2.minDXDY(), cutBox2.maxDXDY());
  frString cutClass1;
  if (cutClassIdx1 != -1) {
    cutClass1 = layer1->getCutClass(cutClassIdx1)->getName();
  }
  frString cutClass2;
  if (cutClassIdx2 != -1) {
    cutClass2 = layer2->getCutClass(cutClassIdx2)->getName();
  }
  bool isSide1;
  bool isSide2;
  if (isCurrDirY) {
    isSide1 = cutBox1.dx() > cutBox1.dy();
    isSide2 = cutBox2.dx() > cutBox2.dy();
  } else {
    isSide1 = cutBox1.dx() < cutBox1.dy();
    isSide2 = cutBox2.dx() < cutBox2.dy();
  }
  if (layer1->getLayerNum() == layer2->getLayerNum()) {
    const frLef58CutSpacingTableConstraint* lef58con = nullptr;
    if (layer1->hasLef58SameMetalCutSpcTblConstraint()) {
      lef58con = layer1->getLef58SameMetalCutSpcTblConstraint();
    } else if (layer1->hasLef58SameNetCutSpcTblConstraint()) {
      lef58con = layer1->getLef58SameNetCutSpcTblConstraint();
    } else if (layer1->hasLef58DiffNetCutSpcTblConstraint()) {
      lef58con = layer1->getLef58DiffNetCutSpcTblConstraint();
    }
    if (lef58con != nullptr) {
      const auto dbRule = lef58con->getODBRule();
      frCoord reqSpcVal
          = dbRule->getSpacing(cutClass1, isSide1, cutClass2, isSide2);
      if (!dbRule->isCenterToCenter(cutClass1, cutClass2)
          && !dbRule->isCenterAndEdge(cutClass1, cutClass2)) {
        if (!swapped) {
          reqSpcVal += isCurrDirY ? cutBox1.dy() : cutBox1.dx();
        } else {
          reqSpcVal += isCurrDirY ? cutBox2.dy() : cutBox2.dx();
        }
      }
      if (reqSpcVal != 0) {
        forbiddenRanges.emplace_back(0, reqSpcVal);
      }
    }
  } else {
    const frLef58CutSpacingTableConstraint* con;
    if (layer1->hasLef58SameMetalInterCutSpcTblConstraint()) {
      con = layer1->getLef58SameMetalInterCutSpcTblConstraint();
    } else if (layer1->hasLef58SameNetInterCutSpcTblConstraint()) {
      con = layer1->getLef58SameNetInterCutSpcTblConstraint();
    } else {
      return;
    }
    const auto dbRule = con->getODBRule();
    if (dbRule->isSameNet() || dbRule->isSameMetal()) {
      if (!dbRule->isNoStack()) {
        return;
      }
    }
    frCoord reqSpcVal
        = dbRule->getSpacing(cutClass1, isSide1, cutClass2, isSide2);
    if (reqSpcVal == 0) {
      return;
    }
    if (!dbRule->isCenterToCenter(cutClass1, cutClass2)
        && !dbRule->isCenterAndEdge(cutClass1, cutClass2)) {
      reqSpcVal += isCurrDirY ? ((cutBox1.dy() + cutBox2.dy()) / 2)
                              : ((cutBox1.dx() + cutBox2.dx()) / 2);
    }
    forbiddenRanges.emplace_back(0, reqSpcVal);
  }
}

void FlexRP::prep_via2viaForbiddenLen_lef58CutSpc_helper(
    const odb::Rect& enclosureBox1,
    const odb::Rect& enclosureBox2,
    const odb::Rect& cutBox,
    const frCoord reqSpcVal,
    std::pair<frCoord, frCoord>& range)
{
  frCoord overlapLen = std::min(enclosureBox1.dy(), enclosureBox2.dy());
  frCoord cutLen = cutBox.dy();
  frCoord forbiddenLowerBound, forbiddenUpperBound;
  forbiddenLowerBound = std::max(0, (overlapLen - cutLen) / 2 - reqSpcVal);
  forbiddenUpperBound = reqSpcVal + (overlapLen + cutLen) / 2;
  range = std::make_pair(forbiddenLowerBound, forbiddenUpperBound);
}

// If a via pad triggers MINIMUMCUT rules, we need to make sure any other via
// is sufficiently spaced so it isn't included in the rule. Rules of the form
// "LENGTH length WITHIN distance" need to separate the cut shape from the via
// pad by the specified distance, otherwise the cut shape just needs to not
// intersect the via pad.
void FlexRP::prep_via2viaForbiddenLen_minimumCut(
    const frLayerNum& lNum,
    const frViaDef* viaDef1,
    const frViaDef* viaDef2,
    const bool isCurrDirX,
    ForbiddenRanges& forbiddenRanges)
{
  if (!viaDef1 || !viaDef2) {
    return;
  }

  const bool isH
      = (tech_->getLayer(lNum)->getDir() == odb::dbTechLayerDir::HORIZONTAL);

  bool isVia1Above = false;
  const frVia via1(viaDef1);
  odb::Rect viaBox1;
  if (viaDef1->getLayer1Num() == lNum) {
    viaBox1 = via1.getLayer1BBox();
    isVia1Above = true;
  } else {
    viaBox1 = via1.getLayer2BBox();
    isVia1Above = false;
  }
  const odb::Rect cutBox1 = via1.getCutBBox();
  const int width1 = viaBox1.minDXDY();
  const int length1 = viaBox1.maxDXDY();

  bool isVia2Above = false;
  const frVia via2(viaDef2);
  odb::Rect viaBox2;
  if (viaDef2->getLayer1Num() == lNum) {
    viaBox2 = via2.getLayer1BBox();
    isVia2Above = true;
  } else {
    viaBox2 = via2.getLayer2BBox();
    isVia2Above = false;
  }
  const odb::Rect cutBox2 = via2.getCutBBox();
  const int width2 = viaBox2.minDXDY();
  const int length2 = viaBox2.maxDXDY();

  for (auto& con : tech_->getLayer(lNum)->getMinimumcutConstraints()) {
    frCoord minReqDist = INT_MIN;
    // check via2cut to via1metal
    // no length OR metal1 shape satisfies --> check via2
    if ((!con->hasLength() || (con->hasLength() && length1 > con->getLength()))
        && width1 > con->getWidth()) {
      bool checkVia2 = false;
      if (!con->hasConnection()) {
        checkVia2 = true;
      } else if (con->getConnection() == frMinimumcutConnectionEnum::FROMABOVE
                 && isVia2Above) {
        checkVia2 = true;
      } else if (con->getConnection() == frMinimumcutConnectionEnum::FROMBELOW
                 && !isVia2Above) {
        checkVia2 = true;
      }

      if (!checkVia2) {
        continue;
      }
      if (isH) {
        minReqDist
            = std::max(minReqDist,
                       (con->hasLength() ? con->getDistance() : 0)
                           + std::max(cutBox2.xMax() - 0 + 0 - viaBox1.xMin(),
                                      viaBox1.xMax() - 0 + 0 - cutBox2.xMin()));
      } else {
        minReqDist
            = std::max(minReqDist,
                       (con->hasLength() ? con->getDistance() : 0)
                           + std::max(cutBox2.yMax() - 0 + 0 - viaBox1.yMin(),
                                      viaBox1.yMax() - 0 + 0 - cutBox2.yMin()));
      }
      forbiddenRanges.emplace_back(0, minReqDist);
    }
    minReqDist = INT_MIN;
    // check via1cut to via2metal
    if ((!con->hasLength() || (con->hasLength() && length2 > con->getLength()))
        && width2 > con->getWidth()) {
      bool checkVia1 = false;
      if (!con->hasConnection()) {
        checkVia1 = true;
      } else if (con->getConnection() == frMinimumcutConnectionEnum::FROMABOVE
                 && isVia1Above) {
        checkVia1 = true;
      } else if (con->getConnection() == frMinimumcutConnectionEnum::FROMBELOW
                 && !isVia1Above) {
        checkVia1 = true;
      }

      if (!checkVia1) {
        continue;
      }
      if (isH) {
        minReqDist
            = std::max(minReqDist,
                       (con->hasLength() ? con->getDistance() : 0)
                           + std::max(cutBox1.xMax() - 0 + 0 - viaBox2.xMin(),
                                      viaBox2.xMax() - 0 + 0 - cutBox1.xMin()));
      } else {
        minReqDist
            = std::max(minReqDist,
                       (con->hasLength() ? con->getDistance() : 0)
                           + std::max(cutBox1.yMax() - 0 + 0 - viaBox2.yMin(),
                                      viaBox2.yMax() - 0 + 0 - cutBox1.yMin()));
      }
      forbiddenRanges.emplace_back(0, minReqDist);
    }
  }
}

void FlexRP::prep_via2viaForbiddenLen_widthViaMap(
    const frLayerNum& lNum,
    const frViaDef* viaDef1,
    const frViaDef* viaDef2,
    const bool isCurrDirX,
    ForbiddenRanges& forbiddenRanges)
{
  if (!viaDef1 || !viaDef2 || viaDef1 == viaDef2) {
    return;
  }

  const bool isVia1Above = (viaDef1->getLayer1Num() == lNum);
  const bool isVia2Above = (viaDef2->getLayer1Num() == lNum);
  assert(isVia1Above != isVia2Above);

  const odb::Rect viaBox1(isVia1Above ? viaDef1->getLayer1ShapeBox()
                                      : viaDef1->getLayer2ShapeBox());

  const odb::Rect viaBox2(isVia2Above ? viaDef2->getLayer1ShapeBox()
                                      : viaDef2->getLayer2ShapeBox());

  int lower_width;
  int upper_width;
  const frViaDef* lowerViaDef;
  if (isVia2Above) {
    lower_width = viaBox1.minDXDY();
    upper_width = viaBox2.minDXDY();
    lowerViaDef = viaDef1;
  } else {
    lower_width = viaBox2.minDXDY();
    upper_width = viaBox1.minDXDY();
    lowerViaDef = viaDef2;
  }

  const auto tech = tech_;
  const auto cutLayer = tech->getLayer(lowerViaDef->getCutLayerNum());
  bool allow_stacking = true;
  for (const auto rule : cutLayer->getMetalWidthViaConstraints()) {
    const auto con = rule->getDbRule();
    if (con->getViaName() != lowerViaDef->getName()) {
      if (con->getAboveLayerWidthLow() <= upper_width
          && upper_width <= con->getAboveLayerWidthHigh()
          && con->getBelowLayerWidthLow() <= lower_width
          && lower_width <= con->getBelowLayerWidthHigh()) {
        allow_stacking = false;
        break;
      }
    }
  }

  if (allow_stacking) {
    return;
  }

  const odb::Rect cutBox1 = viaDef1->getCutShapeBox();
  const odb::Rect cutBox2 = viaDef2->getCutShapeBox();

  frCoord minReqDist;
  if (isCurrDirX) {
    minReqDist = std::max({cutBox2.xMax() - viaBox1.xMin(),
                           viaBox1.xMax() - cutBox2.xMin(),
                           cutBox1.xMax() - viaBox2.xMin(),
                           viaBox2.xMax() - cutBox1.xMin()});
  } else {
    minReqDist = std::max({cutBox2.yMax() - viaBox1.yMin(),
                           viaBox1.yMax() - cutBox2.yMin(),
                           cutBox1.yMax() - viaBox2.yMin(),
                           viaBox2.yMax() - cutBox1.yMin()});
  }
  forbiddenRanges.emplace_back(0, minReqDist);

  debugPrint(logger_,
             utl::DRT,
             "widthViaMap",
             1,
             "widthViaMap: constrain {} to {} by {}",
             viaDef1->getName(),
             viaDef2->getName(),
             minReqDist);
}

void FlexRP::prep_via2viaForbiddenLen_cutSpc(const frLayerNum& lNum,
                                             const frViaDef* viaDef1,
                                             const frViaDef* viaDef2,
                                             const bool isCurrDirX,
                                             ForbiddenRanges& forbiddenRanges)
{
  if (!viaDef1 || !viaDef2) {
    return;
  }

  const bool isCurrDirY = !isCurrDirX;

  const frVia via1(viaDef1);
  odb::Rect viaBox1;
  if (viaDef1->getLayer1Num() == lNum) {
    viaBox1 = via1.getLayer1BBox();
  } else {
    viaBox1 = via1.getLayer2BBox();
  }
  const odb::Rect cutBox1 = via1.getCutBBox();

  const frVia via2(viaDef2);
  odb::Rect viaBox2;
  if (viaDef2->getLayer1Num() == lNum) {
    viaBox2 = via2.getLayer1BBox();
  } else {
    viaBox2 = via2.getLayer2BBox();
  }
  const odb::Rect cutBox2 = via2.getCutBBox();

  // same layer (use samenet rule if exist, otherwise use diffnet rule)
  if (viaDef1->getCutLayerNum() == viaDef2->getCutLayerNum()) {
    const auto& samenetCons
        = tech_->getLayer(viaDef1->getCutLayerNum())->getCutSpacing(true);
    const auto& diffnetCons
        = tech_->getLayer(viaDef1->getCutLayerNum())->getCutSpacing(false);
    if (!samenetCons.empty()) {
      // check samenet spacing rule if exists
      for (const auto con : samenetCons) {
        if (con == nullptr) {
          continue;
        }
        // filter rule, assuming default via will never trigger cutArea
        if (con->hasSecondLayer() || con->isAdjacentCuts()
            || con->isParallelOverlap() || con->isArea()
            || !con->hasSameNet()) {
          continue;
        }
        auto reqSpcVal = con->getCutSpacing();
        if (!con->hasCenterToCenter()) {
          reqSpcVal += isCurrDirY ? cutBox1.dy() : cutBox1.dx();
        }
        forbiddenRanges.emplace_back(0, reqSpcVal);
      }
    } else {
      // check diffnet spacing rule if samenet rule does not exist
      // filter rule, assuming default via will never trigger cutArea
      for (const auto con : diffnetCons) {
        if (con == nullptr) {
          continue;
        }
        if (con->hasSecondLayer() || con->isAdjacentCuts()
            || con->isParallelOverlap() || con->isArea() || con->hasSameNet()) {
          continue;
        }
        auto reqSpcVal = con->getCutSpacing();
        if (!con->hasCenterToCenter()) {
          reqSpcVal += isCurrDirY ? cutBox1.dy() : cutBox1.dx();
        }
        forbiddenRanges.emplace_back(0, reqSpcVal);
      }
    }
  } else {
    const auto layerNum1 = viaDef1->getCutLayerNum();
    const auto layerNum2 = viaDef2->getCutLayerNum();
    const frCutSpacingConstraint* samenetCon = nullptr;
    if (tech_->getLayer(layerNum1)->hasInterLayerCutSpacing(layerNum2, true)) {
      samenetCon = tech_->getLayer(layerNum1)->getInterLayerCutSpacing(
          layerNum2, true);
    }
    if (tech_->getLayer(layerNum2)->hasInterLayerCutSpacing(layerNum1, true)) {
      if (samenetCon) {
        logger_->warn(DRT,
                      92,
                      "Duplicate diff layer samenet cut spacing, skipping cut "
                      "spacing from {} to {}.",
                      layerNum2,
                      layerNum1);
      } else {
        samenetCon = tech_->getLayer(layerNum2)->getInterLayerCutSpacing(
            layerNum1, true);
      }
    }
    if (samenetCon == nullptr) {
      if (tech_->getLayer(layerNum1)->hasInterLayerCutSpacing(layerNum2,
                                                              false)) {
        samenetCon = tech_->getLayer(layerNum1)->getInterLayerCutSpacing(
            layerNum2, false);
      }
      if (tech_->getLayer(layerNum2)->hasInterLayerCutSpacing(layerNum1,
                                                              false)) {
        if (samenetCon) {
          logger_->warn(DRT,
                        93,
                        "Duplicate diff layer diffnet cut spacing, skipping "
                        "cut spacing from {} to {}.",
                        layerNum2,
                        layerNum1);
        } else {
          samenetCon = tech_->getLayer(layerNum2)->getInterLayerCutSpacing(
              layerNum1, false);
        }
      }
    }
    if (samenetCon) {
      // filter rule, assuming default via will never trigger cutArea
      auto reqSpcVal = samenetCon->getCutSpacing();
      if (reqSpcVal != 0 && !samenetCon->hasCenterToCenter()) {
        reqSpcVal += isCurrDirY ? ((cutBox1.dy() + cutBox2.dy()) / 2)
                                : ((cutBox1.dx() + cutBox2.dx()) / 2);
      }
      if (reqSpcVal != 0 && !samenetCon->hasStack()) {
        forbiddenRanges.emplace_back(0, reqSpcVal);
      }
    }
  }
}

void FlexRP::prep_via2viaForbiddenLen_minSpc(frLayerNum lNum,
                                             const frViaDef* viaDef1,
                                             const frViaDef* viaDef2,
                                             const bool isCurrDirX,
                                             ForbiddenRanges& forbiddenRanges,
                                             frNonDefaultRule* ndr)
{
  if (!viaDef1 || !viaDef2) {
    return;
  }

  // bool isCurrDirY = !isCurrDirX;
  const frCoord defaultWidth = tech_->getLayer(lNum)->getWidth();

  const frVia via1(viaDef1);
  odb::Rect viaBox1;
  if (viaDef1->getLayer1Num() == lNum) {
    viaBox1 = via1.getLayer1BBox();
  } else {
    viaBox1 = via1.getLayer2BBox();
  }
  auto width1 = viaBox1.minDXDY();
  const bool isVia1Fat = isCurrDirX ? (viaBox1.dy() > defaultWidth)
                                    : (viaBox1.dx() > defaultWidth);
  auto prl1 = isCurrDirX ? viaBox1.dy() : viaBox1.dx();

  const frVia via2(viaDef2);
  odb::Rect viaBox2;
  if (viaDef2->getLayer1Num() == lNum) {
    viaBox2 = via2.getLayer1BBox();
  } else {
    viaBox2 = via2.getLayer2BBox();
  }
  const auto width2 = viaBox2.minDXDY();
  const bool isVia2Fat = isCurrDirX ? (viaBox2.dy() > defaultWidth)
                                    : (viaBox2.dx() > defaultWidth);
  const auto prl2 = isCurrDirX ? viaBox2.dy() : viaBox2.dx();

  frCoord minNonOverlapDist = isCurrDirX ? ((viaBox1.dx() + viaBox2.dx()) / 2)
                                         : ((viaBox1.dy() + viaBox2.dy()) / 2);
  frCoord minReqDist = INT_MIN;

  // check minSpc rule
  if (isVia1Fat && isVia2Fat) {
    const auto con = tech_->getLayer(lNum)->getMinSpacing();
    if (con) {
      if (con->typeId() == frConstraintTypeEnum::frcSpacingConstraint) {
        minReqDist = static_cast<frSpacingConstraint*>(con)->getMinSpacing();
      } else if (con->typeId()
                 == frConstraintTypeEnum::frcSpacingTablePrlConstraint) {
        minReqDist = static_cast<frSpacingTablePrlConstraint*>(con)->find(
            std::max(width1, width2), std::min(prl1, prl2));
      } else if (con->typeId()
                 == frConstraintTypeEnum::frcSpacingTableTwConstraint) {
        minReqDist = static_cast<frSpacingTableTwConstraint*>(con)->find(
            width1, width2, std::min(prl1, prl2));
      }
    }
    if (ndr) {
      minReqDist = std::max(minReqDist, ndr->getSpacing(lNum / 2 - 1));
    }
    if (minReqDist != INT_MIN) {
      minReqDist += minNonOverlapDist;
    }
  }

  if (minReqDist != INT_MIN) {
    forbiddenRanges.emplace_back(minNonOverlapDist, minReqDist);
  }

  // check in layer2 if two vias are in same layer
  if (viaDef1 == viaDef2) {
    if (viaDef1->getLayer1Num() == lNum) {
      viaBox1 = via1.getLayer2BBox();
      lNum = lNum + 2;
    } else {
      viaBox1 = via1.getLayer1BBox();
      lNum = lNum - 2;
    }
    minNonOverlapDist = isCurrDirX ? viaBox1.dx() : viaBox1.dy();

    width1 = viaBox1.minDXDY();
    prl1 = isCurrDirX ? viaBox1.dy() : viaBox1.dx();
    minReqDist = INT_MIN;
    const auto con = tech_->getLayer(lNum)->getMinSpacing();
    if (con) {
      if (con->typeId() == frConstraintTypeEnum::frcSpacingConstraint) {
        minReqDist = static_cast<frSpacingConstraint*>(con)->getMinSpacing();
      } else if (con->typeId()
                 == frConstraintTypeEnum::frcSpacingTablePrlConstraint) {
        minReqDist = static_cast<frSpacingTablePrlConstraint*>(con)->find(
            width1, prl1);
      } else if (con->typeId()
                 == frConstraintTypeEnum::frcSpacingTableTwConstraint) {
        minReqDist = static_cast<frSpacingTableTwConstraint*>(con)->find(
            width1, width1, prl1);
      }
      if (ndr) {
        minReqDist = std::max(minReqDist, ndr->getSpacing(lNum / 2 - 1));
      }
      if (minReqDist != INT_MIN) {
        minReqDist += minNonOverlapDist;
      }
    }

    if (minReqDist != INT_MIN) {
      forbiddenRanges.emplace_back(minNonOverlapDist, minReqDist);
    }
  }
}

void FlexRP::prep_via2viaPRL(const frLayerNum lNum,
                             const frViaDef* viaDef1,
                             const frViaDef* viaDef2,
                             const bool isCurrDirX,
                             frCoord& prl)
{
  if (!viaDef1 || !viaDef2) {
    return;
  }
  const frVia via1(viaDef1);
  odb::Rect viaBox1;
  if (viaDef1->getLayer1Num() == lNum) {
    viaBox1 = via1.getLayer1BBox();
  } else {
    viaBox1 = via1.getLayer2BBox();
  }
  const frVia via2(viaDef2);
  odb::Rect viaBox2;
  if (viaDef2->getLayer1Num() == lNum) {
    viaBox2 = via2.getLayer1BBox();
  } else {
    viaBox2 = via2.getLayer2BBox();
  }
  if (isCurrDirX) {
    prl = (viaBox1.dx() + viaBox2.dx()) / 2;
  } else {
    prl = (viaBox1.dy() + viaBox2.dy()) / 2;
  }
}

}  // namespace drt
