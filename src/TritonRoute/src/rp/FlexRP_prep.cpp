/* Authors: Lutong Wang and Bangqi Xu */
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
#include <sstream>

#include "FlexRP.h"
#include "db/infra/frTime.h"
#include "frProfileTask.h"
#include "gc/FlexGC.h"
#include "opendb/db.h"
using namespace std;
using namespace fr;

void FlexRP::prep()
{
  ProfileTask profile("RP:prep");
  prep_via2viaForbiddenLen();
  prep_viaForbiddenTurnLen();
  prep_viaForbiddenPlanarLen();
  prep_lineForbiddenLen();
  prep_viaForbiddenThrough();
  for (auto& ndr : tech_->nonDefaultRules) {
    prep_via2viaForbiddenLen(ndr.get());
    prep_viaForbiddenTurnLen(ndr.get());
  }
}

void FlexRP::prep_viaForbiddenThrough()
{
  auto bottomLayerNum = getDesign()->getTech()->getBottomLayerNum();
  auto topLayerNum = getDesign()->getTech()->getTopLayerNum();

  int i = 0;
  for (auto lNum = bottomLayerNum; lNum <= topLayerNum; lNum++) {
    if (tech_->getLayer(lNum)->getType() != frLayerTypeEnum::ROUTING) {
      continue;
    }
    frViaDef* downVia = nullptr;
    frViaDef* upVia = nullptr;
    if (getDesign()->getTech()->getBottomLayerNum() <= lNum - 1) {
      downVia = getDesign()->getTech()->getLayer(lNum - 1)->getDefaultViaDef();
    }
    if (getDesign()->getTech()->getTopLayerNum() >= lNum + 1) {
      upVia = getDesign()->getTech()->getLayer(lNum + 1)->getDefaultViaDef();
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
                                             frViaDef* viaDef,
                                             bool isCurrDirX)
{
  bool isThroughAllowed = true;

  if (prep_viaForbiddenThrough_minStep(lNum, viaDef, isCurrDirX)) {
    isThroughAllowed = false;
  }

  tech_->viaForbiddenThrough[tableLayerIdx][tableEntryIdx] = !isThroughAllowed;
}

bool FlexRP::prep_viaForbiddenThrough_minStep(const frLayerNum& lNum,
                                              frViaDef* viaDef,
                                              bool isCurrDirX)
{
  if (!viaDef) {
    return false;
  }

  if (lNum == 10 && viaDef->getName() == "CK_23_28_0_26_VH_CK") {
    return true;
  } else {
    return false;
  }
}

void FlexRP::prep_lineForbiddenLen()
{
  auto bottomLayerNum = getDesign()->getTech()->getBottomLayerNum();
  auto topLayerNum = getDesign()->getTech()->getTopLayerNum();

  int i = 0;
  for (auto lNum = bottomLayerNum; lNum <= topLayerNum; lNum++) {
    if (tech_->getLayer(lNum)->getType() != frLayerTypeEnum::ROUTING) {
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
  vector<pair<frCoord, frCoord>> forbiddenRanges;
  prep_lineForbiddenLen_minSpc(lNum, isZShape, isCurrDirX, forbiddenRanges);

  // merge ranges
  boost::icl::interval_set<frCoord> forbiddenIntvSet;
  for (auto& range : forbiddenRanges) {
    forbiddenIntvSet.insert(
        boost::icl::interval<frCoord>::closed(range.first, range.second));
  }

  forbiddenRanges.clear();
  for (auto it = forbiddenIntvSet.begin(); it != forbiddenIntvSet.end(); it++) {
    auto beginCoord = it->lower();
    auto endCoord = it->upper();
    forbiddenRanges.push_back(make_pair(beginCoord + 1, endCoord - 1));
  }

  tech_->line2LineForbiddenLen[tableLayerIdx][tableEntryIdx] = forbiddenRanges;
}

void FlexRP::prep_lineForbiddenLen_minSpc(
    const frLayerNum& lNum,
    const bool isZShape,
    const bool isCurrDirX,
    vector<pair<frCoord, frCoord>>& forbiddenRanges)
{
  frCoord defaultWidth = tech_->getLayer(lNum)->getWidth();

  frCoord minNonOverlapDist = defaultWidth;

  frCoord minReqDist = INT_MIN;
  frCoord prl = isZShape ? defaultWidth : tech_->getLayer(lNum)->getPitch();
  auto con = tech_->getLayer(lNum)->getMinSpacing();
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
    forbiddenRanges.push_back(make_pair(minNonOverlapDist, minReqDist));
  }
}

void FlexRP::prep_viaForbiddenPlanarLen()
{
  auto bottomLayerNum = getDesign()->getTech()->getBottomLayerNum();
  auto topLayerNum = getDesign()->getTech()->getTopLayerNum();

  int i = 0;
  for (auto lNum = bottomLayerNum; lNum <= topLayerNum; lNum++) {
    if (getDesign()->getTech()->getLayer(lNum)->getType()
        != frLayerTypeEnum::ROUTING) {
      continue;
    }
    frViaDef* downVia = nullptr;
    frViaDef* upVia = nullptr;
    if (getDesign()->getTech()->getBottomLayerNum() <= lNum - 1) {
      downVia = getDesign()->getTech()->getLayer(lNum - 1)->getDefaultViaDef();
    }
    if (getDesign()->getTech()->getTopLayerNum() >= lNum + 1) {
      upVia = getDesign()->getTech()->getLayer(lNum + 1)->getDefaultViaDef();
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
                                               frViaDef* viaDef,
                                               bool isCurrDirX)
{
  if (!viaDef) {
    return;
  }

  vector<pair<frCoord, frCoord>> forbiddenRanges;
  prep_viaForbiddenPlanarLen_minStep(lNum, viaDef, isCurrDirX, forbiddenRanges);

  // merge forbidden ranges
  boost::icl::interval_set<frCoord> forbiddenIntvSet;
  for (auto& range : forbiddenRanges) {
    forbiddenIntvSet.insert(
        boost::icl::interval<frCoord>::closed(range.first, range.second));
  }

  forbiddenRanges.clear();
  for (auto it = forbiddenIntvSet.begin(); it != forbiddenIntvSet.end(); it++) {
    auto beginCoord = it->lower();
    auto endCoord = it->upper();
    forbiddenRanges.push_back(make_pair(beginCoord + 1, endCoord - 1));
  }

  tech_->viaForbiddenPlanarLen[tableLayerIdx][tableEntryIdx] = forbiddenRanges;
}

void FlexRP::prep_viaForbiddenPlanarLen_minStep(
    const frLayerNum& lNum,
    frViaDef* viaDef,
    bool isCurrDirX,
    vector<pair<frCoord, frCoord>>& forbiddenRanges)
{
  return;
}

void FlexRP::prep_viaForbiddenTurnLen(frNonDefaultRule* ndr)
{
  auto bottomLayerNum = getDesign()->getTech()->getBottomLayerNum();
  auto topLayerNum = getDesign()->getTech()->getTopLayerNum();
  int bottom = BOTTOM_ROUTING_LAYER;
  int i = 0;
  for (auto lNum = bottomLayerNum; lNum <= topLayerNum; lNum++) {
    if (getDesign()->getTech()->getLayer(lNum)->getType()
        != frLayerTypeEnum::ROUTING) {
      continue;
    }
    frViaDef* downVia = nullptr;
    frViaDef* upVia = nullptr;
    if (ndr && bottom < lNum && ndr->getPrefVia((lNum - 2) / 2 - 1))
      downVia = ndr->getPrefVia((lNum - 2) / 2 - 1);
    else if (getDesign()->getTech()->getBottomLayerNum() <= lNum - 1)
      downVia = getDesign()->getTech()->getLayer(lNum - 1)->getDefaultViaDef();

    if (getDesign()->getTech()->getTopLayerNum() >= lNum + 1) {
      if (ndr && ndr->getPrefVia((lNum + 2) / 2 - 1))
        upVia = ndr->getPrefVia((lNum + 2) / 2 - 1);
      else
        upVia = getDesign()->getTech()->getLayer(lNum + 1)->getDefaultViaDef();
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
                                             frViaDef* viaDef,
                                             bool isCurrDirX,
                                             frNonDefaultRule* ndr)
{
  if (!viaDef) {
    return;
  }

  auto tech = getDesign()->getTech();

  vector<pair<frCoord, frCoord>> forbiddenRanges;
  prep_viaForbiddenTurnLen_minSpc(
      lNum, viaDef, isCurrDirX, forbiddenRanges, ndr);

  // merge forbidden ranges
  boost::icl::interval_set<frCoord> forbiddenIntvSet;
  for (auto& range : forbiddenRanges) {
    forbiddenIntvSet.insert(
        boost::icl::interval<frCoord>::closed(range.first, range.second));
  }

  forbiddenRanges.clear();
  for (auto it = forbiddenIntvSet.begin(); it != forbiddenIntvSet.end(); it++) {
    auto beginCoord = it->lower();
    auto endCoord = it->upper();
    forbiddenRanges.push_back(make_pair(beginCoord + 1, endCoord - 1));
  }
  if (ndr)
    ndr->viaForbiddenTurnLen[tableLayerIdx][tableEntryIdx] = forbiddenRanges;
  else
    tech->viaForbiddenTurnLen[tableLayerIdx][tableEntryIdx] = forbiddenRanges;
}

void FlexRP::prep_viaForbiddenTurnLen_minSpc(
    const frLayerNum& lNum,
    frViaDef* viaDef,
    bool isCurrDirX,
    vector<pair<frCoord, frCoord>>& forbiddenRanges,
    frNonDefaultRule* ndr)
{
  if (!viaDef) {
    return;
  }

  frCoord defaultWidth = tech_->getLayer(lNum)->getWidth();
  frCoord width = defaultWidth;
  if (ndr)
    width = max(width, ndr->getWidth(lNum / 2 - 1));

  frVia via1(viaDef);
  frBox viaBox1;
  if (viaDef->getLayer1Num() == lNum) {
    via1.getLayer1BBox(viaBox1);
  } else {
    via1.getLayer2BBox(viaBox1);
  }
  auto width1 = viaBox1.width();
  bool isVia1Fat = isCurrDirX
                       ? (viaBox1.top() - viaBox1.bottom() > defaultWidth)
                       : (viaBox1.right() - viaBox1.left() > defaultWidth);
  auto prl1 = isCurrDirX ? (viaBox1.top() - viaBox1.bottom())
                         : (viaBox1.right() - viaBox1.left());

  frCoord minNonOverlapDist
      = isCurrDirX ? ((viaBox1.right() - viaBox1.left() + width) / 2)
                   : ((viaBox1.top() - viaBox1.bottom() + width) / 2);
  frCoord minReqDist = INT_MIN;
  if (isVia1Fat || ndr) {
    auto con = getDesign()->getTech()->getLayer(lNum)->getMinSpacing();
    if (con) {
      if (con->typeId() == frConstraintTypeEnum::frcSpacingConstraint) {
        minReqDist = static_cast<frSpacingConstraint*>(con)->getMinSpacing();
      } else if (con->typeId()
                 == frConstraintTypeEnum::frcSpacingTablePrlConstraint) {
        minReqDist = static_cast<frSpacingTablePrlConstraint*>(con)->find(
            max(width1, width), prl1);
      } else if (con->typeId()
                 == frConstraintTypeEnum::frcSpacingTableTwConstraint) {
        minReqDist = static_cast<frSpacingTableTwConstraint*>(con)->find(
            width1, width, prl1);
      }
    }
    if (ndr)
      minReqDist = max(minReqDist, ndr->getSpacing(lNum / 2 - 1));
    if (minReqDist != INT_MIN) {
      minReqDist += minNonOverlapDist;
    }
  }
  if (minReqDist != INT_MIN) {
    forbiddenRanges.push_back(make_pair(minNonOverlapDist, minReqDist));
  }
}

void FlexRP::prep_via2viaForbiddenLen(frNonDefaultRule* ndr)
{
  auto bottomLayerNum = getDesign()->getTech()->getBottomLayerNum();
  auto topLayerNum = getDesign()->getTech()->getTopLayerNum();
  int bottom = BOTTOM_ROUTING_LAYER;
  int i = 0;
  for (auto lNum = bottomLayerNum; lNum <= topLayerNum; lNum++) {
    if (getDesign()->getTech()->getLayer(lNum)->getType()
        != frLayerTypeEnum::ROUTING) {
      continue;
    }
    frViaDef* downVia = nullptr;
    frViaDef* upVia = nullptr;
    if (ndr && bottom < lNum && ndr->getPrefVia((lNum - 2) / 2 - 1))
      downVia = ndr->getPrefVia((lNum - 2) / 2 - 1);
    else if (getDesign()->getTech()->getBottomLayerNum() <= lNum - 1)
      downVia = getDesign()->getTech()->getLayer(lNum - 1)->getDefaultViaDef();
    if (getDesign()->getTech()->getTopLayerNum() >= lNum + 1) {
      if (ndr && ndr->getPrefVia((lNum + 2) / 2 - 1))
        upVia = ndr->getPrefVia((lNum + 2) / 2 - 1);
      else
        upVia = getDesign()->getTech()->getLayer(lNum + 1)->getDefaultViaDef();
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
                                             frViaDef* viaDef1,
                                             frViaDef* viaDef2,
                                             bool isCurrDirX,
                                             frNonDefaultRule* ndr)
{
  auto tech = getDesign()->getTech();
  // non-shape-based rule
  vector<pair<frCoord, frCoord>> forbiddenRanges;
  prep_via2viaForbiddenLen_minSpc(
      lNum, viaDef1, viaDef2, isCurrDirX, forbiddenRanges, ndr);
  prep_via2viaForbiddenLen_minimumCut(
      lNum, viaDef1, viaDef2, isCurrDirX, forbiddenRanges);
  prep_via2viaForbiddenLen_cutSpc(
      lNum, viaDef1, viaDef2, isCurrDirX, forbiddenRanges);
  prep_via2viaForbiddenLen_lef58CutSpc(
      lNum, viaDef1, viaDef2, isCurrDirX, forbiddenRanges);
  prep_via2viaForbiddenLen_lef58CutSpcTbl(
      lNum, viaDef1, viaDef2, isCurrDirX, forbiddenRanges);

  // merge forbidden ranges
  boost::icl::interval_set<frCoord> forbiddenIntvSet;
  for (auto& range : forbiddenRanges) {
    forbiddenIntvSet.insert(
        boost::icl::interval<frCoord>::closed(range.first, range.second));
  }

  forbiddenRanges.clear();
  for (auto it = forbiddenIntvSet.begin(); it != forbiddenIntvSet.end(); it++) {
    auto beginCoord = it->lower();
    auto endCoord = it->upper();
    forbiddenRanges.push_back(make_pair(beginCoord + 1, endCoord - 1));
  }
  if (ndr)
    ndr->via2ViaForbiddenLen[tableLayerIdx][tableEntryIdx] = forbiddenRanges;
  else {
    tech->via2ViaForbiddenLen[tableLayerIdx][tableEntryIdx] = forbiddenRanges;

    // shape-base rule
    forbiddenRanges.clear();
    forbiddenIntvSet = boost::icl::interval_set<frCoord>();
    prep_via2viaForbiddenLen_minStep(
        lNum, viaDef1, viaDef2, isCurrDirX, forbiddenRanges);

    // merge forbidden ranges
    for (auto& range : forbiddenRanges) {
      forbiddenIntvSet.insert(
          boost::icl::interval<frCoord>::closed(range.first, range.second));
    }

    forbiddenRanges.clear();
    for (auto it = forbiddenIntvSet.begin(); it != forbiddenIntvSet.end();
         it++) {
      auto beginCoord = it->lower();
      auto endCoord = it->upper();
      forbiddenRanges.push_back(make_pair(beginCoord, endCoord - 1));
    }

    tech->via2ViaForbiddenOverlapLen[tableLayerIdx][tableEntryIdx]
        = forbiddenRanges;
  }
}

// only partial support of GF14
void FlexRP::prep_via2viaForbiddenLen_lef58CutSpc(
    const frLayerNum& lNum,
    frViaDef* viaDef1,
    frViaDef* viaDef2,
    bool isCurrDirX,
    vector<pair<frCoord, frCoord>>& forbiddenRanges)
{
  if (!viaDef1 || !viaDef2) {
    return;
  }

  if (DBPROCESSNODE != "GF14_13M_3Mx_2Cx_4Kx_2Hx_2Gx_LB") {
    return;
  }

  bool isCurrDirY = !isCurrDirX;
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

  frBox enclosureBox1, enclosureBox2, cutBox1, cutBox2;
  frVia via1(viaDef1);
  frVia via2(viaDef2);
  if (viaDef1->getLayer1Num() == lNum) {
    via1.getLayer1BBox(enclosureBox1);
  } else {
    via1.getLayer2BBox(enclosureBox1);
  }
  if (viaDef2->getLayer1Num() == lNum) {
    via2.getLayer1BBox(enclosureBox2);
  } else {
    via2.getLayer2BBox(enclosureBox2);
  }
  via1.getCutBBox(cutBox1);
  via2.getCutBBox(cutBox2);
  pair<frCoord, frCoord> range;
  frCoord reqSpcVal = 0;
  // check via1 cut layer to lNum
  auto via1CutLNum = viaDef1->getCutLayerNum();
  if (!tech_->getLayer(via1CutLNum)
           ->getLef58CutSpacingConstraints(false)
           .empty()) {
    for (auto con :
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
    for (auto con :
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
  auto via2CutLNum = viaDef2->getCutLayerNum();
  if (!tech_->getLayer(via2CutLNum)
           ->getLef58CutSpacingConstraints(false)
           .empty()) {
    for (auto con :
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
    for (auto con :
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
    frViaDef* viaDef1,
    frViaDef* viaDef2,
    bool isCurrDirX,
    vector<pair<frCoord, frCoord>>& forbiddenRanges)
{
  if (!viaDef1 || !viaDef2) {
    return;
  }
  if (viaDef1->getCutLayerNum() != viaDef2->getCutLayerNum())
    return;
  bool isCurrDirY = !isCurrDirX;
  frVia via1(viaDef1);
  frBox viaBox1, viaBox2, cutBox1, cutBox2;
  if (viaDef1->getLayer1Num() == lNum) {
    via1.getLayer1BBox(viaBox1);
  } else {
    via1.getLayer2BBox(viaBox1);
  }

  frVia via2(viaDef2);
  if (viaDef2->getLayer1Num() == lNum) {
    via2.getLayer1BBox(viaBox2);
  } else {
    via2.getLayer2BBox(viaBox2);
  }
  via1.getCutBBox(cutBox1);
  via2.getCutBBox(cutBox2);
  pair<frCoord, frCoord> range;
  frCoord reqSpcVal = 0;
  auto layer = tech_->getLayer(viaDef1->getCutLayerNum());
  for(auto con : layer->getLef58CutSpacingTableConstraints())
  {
    auto cutClass1 = layer->getCutClass(cutBox1.width(), cutBox1.length())->getName();
    auto cutClass2 = layer->getCutClass(cutBox2.width(), cutBox2.length())->getName();
    for(auto con : layer->getLef58CutSpacingTableConstraints())
    {
      auto dbRule = con->getODBRule();
      reqSpcVal = dbRule->getMaxSpacing(cutClass1, cutClass2);
      if (!dbRule->isCenterToCenter(cutClass1, cutClass2)) {
        reqSpcVal += isCurrDirY ? (cutBox1.top() - cutBox1.bottom())
                                : (cutBox1.right() - cutBox1.left());
      }
      prep_via2viaForbiddenLen_lef58CutSpc_helper(
          viaBox1, viaBox2, cutBox2, reqSpcVal, range);
      forbiddenRanges.push_back(range);
    }
  }
}

void FlexRP::prep_via2viaForbiddenLen_lef58CutSpc_helper(
    const frBox& enclosureBox1,
    const frBox& enclosureBox2,
    const frBox& cutBox,
    frCoord reqSpcVal,
    pair<frCoord, frCoord>& range)
{
  frCoord overlapLen = min((enclosureBox1.top() - enclosureBox1.bottom()),
                           (enclosureBox2.top() - enclosureBox2.bottom()));
  frCoord cutLen = cutBox.top() - cutBox.bottom();
  frCoord forbiddenLowerBound, forbiddenUpperBound;
  forbiddenLowerBound = max(0, (overlapLen - cutLen) / 2 - reqSpcVal);
  forbiddenUpperBound = reqSpcVal + (overlapLen + cutLen) / 2;
  range = make_pair(forbiddenLowerBound, forbiddenUpperBound);
}

// only partial support of GF14
void FlexRP::prep_via2viaForbiddenLen_minStep(
    const frLayerNum& lNum,
    frViaDef* viaDef1,
    frViaDef* viaDef2,
    bool isCurrDirX,
    vector<pair<frCoord, frCoord>>& forbiddenRanges)
{
  if (!viaDef1 || !viaDef2) {
    return;
  }

  if (DBPROCESSNODE != "GF14_13M_3Mx_2Cx_4Kx_2Hx_2Gx_LB") {
    return;
  }

  bool isCurrDirY = !isCurrDirX;
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
  frBox enclosureBox1, enclosureBox2;
  frVia via1(viaDef1);
  frVia via2(viaDef2);
  if (viaDef1->getLayer1Num() == lNum) {
    via1.getLayer1BBox(enclosureBox1);
  } else {
    via1.getLayer2BBox(enclosureBox1);
  }
  if (viaDef2->getLayer1Num() == lNum) {
    via2.getLayer1BBox(enclosureBox2);
  } else {
    via2.getLayer2BBox(enclosureBox2);
  }

  frCoord enclosureBox1Span = enclosureBox1.top() - enclosureBox1.bottom();
  frCoord enclosureBox2Span = enclosureBox2.top() - enclosureBox2.bottom();
  frCoord maxForbiddenLen = (enclosureBox1Span > enclosureBox2Span)
                                ? (enclosureBox1Span - enclosureBox2Span)
                                : (enclosureBox2Span - enclosureBox1Span);
  maxForbiddenLen /= 2;

  forbiddenRanges.push_back(make_pair(0, maxForbiddenLen));
}

void FlexRP::prep_via2viaForbiddenLen_minimumCut(
    const frLayerNum& lNum,
    frViaDef* viaDef1,
    frViaDef* viaDef2,
    bool isCurrDirX,
    vector<pair<frCoord, frCoord>>& forbiddenRanges)
{
  if (!viaDef1 || !viaDef2) {
    return;
  }

  bool isH = (getDesign()->getTech()->getLayer(lNum)->getDir()
              == frPrefRoutingDirEnum::frcHorzPrefRoutingDir);

  bool isVia1Above = false;
  frVia via1(viaDef1);
  frBox viaBox1, cutBox1;
  if (viaDef1->getLayer1Num() == lNum) {
    via1.getLayer1BBox(viaBox1);
    isVia1Above = true;
  } else {
    via1.getLayer2BBox(viaBox1);
    isVia1Above = false;
  }
  via1.getCutBBox(cutBox1);
  auto width1 = viaBox1.width();
  auto length1 = viaBox1.length();

  bool isVia2Above = false;
  frVia via2(viaDef2);
  frBox viaBox2, cutBox2;
  if (viaDef2->getLayer1Num() == lNum) {
    via2.getLayer1BBox(viaBox2);
    isVia2Above = true;
  } else {
    via2.getLayer2BBox(viaBox2);
    isVia2Above = false;
  }
  via2.getCutBBox(cutBox2);
  auto width2 = viaBox2.width();
  auto length2 = viaBox2.length();

  for (auto& con :
       getDesign()->getTech()->getLayer(lNum)->getMinimumcutConstraints()) {
    frCoord minReqDist = INT_MIN;
    // check via2cut to via1metal
    // no length OR metal1 shape satisfies --> check via2
    if ((!con->hasLength() || (con->hasLength() && length1 > con->getLength()))
        && width1 > con->getWidth()) {
      bool checkVia2 = false;
      if (!con->hasConnection()) {
        checkVia2 = true;
      } else {
        if (con->getConnection() == frMinimumcutConnectionEnum::FROMABOVE
            && isVia2Above) {
          checkVia2 = true;
        } else if (con->getConnection() == frMinimumcutConnectionEnum::FROMBELOW
                   && !isVia2Above) {
          checkVia2 = true;
        }
      }
      if (!checkVia2) {
        continue;
      }
      if (isH) {
        minReqDist = max(minReqDist,
                         (con->hasLength() ? con->getDistance() : 0)
                             + max(cutBox2.right() - 0 + 0 - viaBox1.left(),
                                   viaBox1.right() - 0 + 0 - cutBox2.left()));
      } else {
        minReqDist = max(minReqDist,
                         (con->hasLength() ? con->getDistance() : 0)
                             + max(cutBox2.top() - 0 + 0 - viaBox1.bottom(),
                                   viaBox1.top() - 0 + 0 - cutBox2.bottom()));
      }
      forbiddenRanges.push_back(make_pair(0, minReqDist));
    }
    minReqDist = INT_MIN;
    // check via1cut to via2metal
    if ((!con->hasLength() || (con->hasLength() && length2 > con->getLength()))
        && width2 > con->getWidth()) {
      bool checkVia1 = false;
      if (!con->hasConnection()) {
        checkVia1 = true;
      } else {
        if (con->getConnection() == frMinimumcutConnectionEnum::FROMABOVE
            && isVia1Above) {
          checkVia1 = true;
        } else if (con->getConnection() == frMinimumcutConnectionEnum::FROMBELOW
                   && !isVia1Above) {
          checkVia1 = true;
        }
      }
      if (!checkVia1) {
        continue;
      }
      if (isH) {
        minReqDist = max(minReqDist,
                         (con->hasLength() ? con->getDistance() : 0)
                             + max(cutBox1.right() - 0 + 0 - viaBox2.left(),
                                   viaBox2.right() - 0 + 0 - cutBox1.left()));
      } else {
        minReqDist = max(minReqDist,
                         (con->hasLength() ? con->getDistance() : 0)
                             + max(cutBox1.top() - 0 + 0 - viaBox2.bottom(),
                                   viaBox2.top() - 0 + 0 - cutBox1.bottom()));
      }
      forbiddenRanges.push_back(make_pair(0, minReqDist));
    }
  }
}

void FlexRP::prep_via2viaForbiddenLen_cutSpc(
    const frLayerNum& lNum,
    frViaDef* viaDef1,
    frViaDef* viaDef2,
    bool isCurrDirX,
    vector<pair<frCoord, frCoord>>& forbiddenRanges)
{
  if (!viaDef1 || !viaDef2) {
    return;
  }

  bool isCurrDirY = !isCurrDirX;

  frVia via1(viaDef1);
  frBox viaBox1, cutBox1;
  if (viaDef1->getLayer1Num() == lNum) {
    via1.getLayer1BBox(viaBox1);
  } else {
    via1.getLayer2BBox(viaBox1);
  }
  via1.getCutBBox(cutBox1);

  frVia via2(viaDef2);
  frBox viaBox2, cutBox2;
  if (viaDef2->getLayer1Num() == lNum) {
    via2.getLayer1BBox(viaBox2);
  } else {
    via2.getLayer2BBox(viaBox2);
  }
  via2.getCutBBox(cutBox2);

  // same layer (use samenet rule if exist, otherwise use diffnet rule)
  if (viaDef1->getCutLayerNum() == viaDef2->getCutLayerNum()) {
    auto samenetCons = getDesign()
                           ->getTech()
                           ->getLayer(viaDef1->getCutLayerNum())
                           ->getCutSpacing(true);
    auto diffnetCons = getDesign()
                           ->getTech()
                           ->getLayer(viaDef1->getCutLayerNum())
                           ->getCutSpacing(false);
    if (!samenetCons.empty()) {
      // check samenet spacing rule if exists
      for (auto con : samenetCons) {
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
          reqSpcVal += isCurrDirY ? (cutBox1.top() - cutBox1.bottom())
                                  : (cutBox1.right() - cutBox1.left());
        }
        forbiddenRanges.push_back(make_pair(0, reqSpcVal));
      }
    } else {
      // check diffnet spacing rule if samenet rule does not exist
      // filter rule, assuming default via will never trigger cutArea
      for (auto con : diffnetCons) {
        if (con == nullptr) {
          continue;
        }
        if (con->hasSecondLayer() || con->isAdjacentCuts()
            || con->isParallelOverlap() || con->isArea() || con->hasSameNet()) {
          continue;
        }
        auto reqSpcVal = con->getCutSpacing();
        if (!con->hasCenterToCenter()) {
          reqSpcVal += isCurrDirY ? (cutBox1.top() - cutBox1.bottom())
                                  : (cutBox1.right() - cutBox1.left());
        }
        forbiddenRanges.push_back(make_pair(0, reqSpcVal));
      }
    }
  } else {
    auto layerNum1 = viaDef1->getCutLayerNum();
    auto layerNum2 = viaDef2->getCutLayerNum();
    frCutSpacingConstraint* samenetCon = nullptr;
    if (getDesign()->getTech()->getLayer(layerNum1)->hasInterLayerCutSpacing(
            layerNum2, true)) {
      samenetCon = getDesign()
                       ->getTech()
                       ->getLayer(layerNum1)
                       ->getInterLayerCutSpacing(layerNum2, true);
    }
    if (getDesign()->getTech()->getLayer(layerNum2)->hasInterLayerCutSpacing(
            layerNum1, true)) {
      if (samenetCon) {
        logger_->warn(DRT,
                      92,
                      "duplicate diff layer samenet cut spacing, skipping cut "
                      "spacing from {} to {}.",
                      layerNum2,
                      layerNum1);
      } else {
        samenetCon = getDesign()
                         ->getTech()
                         ->getLayer(layerNum2)
                         ->getInterLayerCutSpacing(layerNum1, true);
      }
    }
    if (samenetCon == nullptr) {
      if (getDesign()->getTech()->getLayer(layerNum1)->hasInterLayerCutSpacing(
              layerNum2, false)) {
        samenetCon = getDesign()
                         ->getTech()
                         ->getLayer(layerNum1)
                         ->getInterLayerCutSpacing(layerNum2, false);
      }
      if (getDesign()->getTech()->getLayer(layerNum2)->hasInterLayerCutSpacing(
              layerNum1, false)) {
        if (samenetCon) {
          logger_->warn(DRT,
                        93,
                        "duplicate diff layer diffnet cut spacing, skipping "
                        "cut spacing from {} to {}.",
                        layerNum2,
                        layerNum1);
        } else {
          samenetCon = getDesign()
                           ->getTech()
                           ->getLayer(layerNum2)
                           ->getInterLayerCutSpacing(layerNum1, false);
        }
      }
    }
    if (samenetCon) {
      // filter rule, assuming default via will never trigger cutArea
      auto reqSpcVal = samenetCon->getCutSpacing();
      if (reqSpcVal == 0) {
        ;
      } else {
        if (!samenetCon->hasCenterToCenter()) {
          reqSpcVal += isCurrDirY ? ((cutBox1.top() - cutBox1.bottom()
                                      + cutBox2.top() - cutBox2.bottom())
                                     / 2)
                                  : ((cutBox1.right() - cutBox1.left()
                                      + cutBox2.right() - cutBox2.left())
                                     / 2);
        }
      }
      if (reqSpcVal != 0 && !samenetCon->hasStack()) {
        forbiddenRanges.push_back(make_pair(0, reqSpcVal));
      }
    }
  }
}

void FlexRP::prep_via2viaForbiddenLen_minSpc(
    frLayerNum lNum,
    frViaDef* viaDef1,
    frViaDef* viaDef2,
    bool isCurrDirX,
    vector<pair<frCoord, frCoord>>& forbiddenRanges,
    frNonDefaultRule* ndr)
{
  if (!viaDef1 || !viaDef2) {
    return;
  }

  // bool isCurrDirY = !isCurrDirX;
  frCoord defaultWidth = getDesign()->getTech()->getLayer(lNum)->getWidth();

  frVia via1(viaDef1);
  frBox viaBox1;
  if (viaDef1->getLayer1Num() == lNum) {
    via1.getLayer1BBox(viaBox1);
  } else {
    via1.getLayer2BBox(viaBox1);
  }
  auto width1 = viaBox1.width();
  bool isVia1Fat = isCurrDirX
                       ? (viaBox1.top() - viaBox1.bottom() > defaultWidth)
                       : (viaBox1.right() - viaBox1.left() > defaultWidth);
  auto prl1 = isCurrDirX ? (viaBox1.top() - viaBox1.bottom())
                         : (viaBox1.right() - viaBox1.left());

  frVia via2(viaDef2);
  frBox viaBox2;
  if (viaDef2->getLayer1Num() == lNum) {
    via2.getLayer1BBox(viaBox2);
  } else {
    via2.getLayer2BBox(viaBox2);
  }
  auto width2 = viaBox2.width();
  bool isVia2Fat = isCurrDirX
                       ? (viaBox2.top() - viaBox2.bottom() > defaultWidth)
                       : (viaBox2.right() - viaBox2.left() > defaultWidth);
  auto prl2 = isCurrDirX ? (viaBox2.top() - viaBox2.bottom())
                         : (viaBox2.right() - viaBox2.left());

  frCoord minNonOverlapDist = isCurrDirX ? ((viaBox1.right() - viaBox1.left()
                                             + viaBox2.right() - viaBox2.left())
                                            / 2)
                                         : ((viaBox1.top() - viaBox1.bottom()
                                             + viaBox2.top() - viaBox2.bottom())
                                            / 2);
  frCoord minReqDist = INT_MIN;

  // check minSpc rule
  if (isVia1Fat && isVia2Fat) {
    auto con = getDesign()->getTech()->getLayer(lNum)->getMinSpacing();
    if (con) {
      if (con->typeId() == frConstraintTypeEnum::frcSpacingConstraint) {
        minReqDist = static_cast<frSpacingConstraint*>(con)->getMinSpacing();
      } else if (con->typeId()
                 == frConstraintTypeEnum::frcSpacingTablePrlConstraint) {
        minReqDist = static_cast<frSpacingTablePrlConstraint*>(con)->find(
            max(width1, width2), min(prl1, prl2));
      } else if (con->typeId()
                 == frConstraintTypeEnum::frcSpacingTableTwConstraint) {
        minReqDist = static_cast<frSpacingTableTwConstraint*>(con)->find(
            width1, width2, min(prl1, prl2));
      }
    }
    if (ndr)
      minReqDist = max(minReqDist, ndr->getSpacing(lNum / 2 - 1));
    if (minReqDist != INT_MIN) {
      minReqDist += minNonOverlapDist;
    }
  }

  if (minReqDist != INT_MIN) {
    forbiddenRanges.push_back(make_pair(minNonOverlapDist, minReqDist));
  }

  // check in layer2 if two vias are in same layer
  if (viaDef1 == viaDef2) {
    if (viaDef1->getLayer1Num() == lNum) {
      via1.getLayer2BBox(viaBox1);
      lNum = lNum + 2;
    } else {
      via1.getLayer1BBox(viaBox1);
      lNum = lNum - 2;
    }
    minNonOverlapDist = isCurrDirX ? (viaBox1.right() - viaBox1.left())
                                   : (viaBox2.right() - viaBox2.left());

    width1 = viaBox1.width();
    prl1 = isCurrDirX ? (viaBox1.top() - viaBox1.bottom())
                      : (viaBox1.right() - viaBox1.left());
    minReqDist = INT_MIN;
    auto con = getDesign()->getTech()->getLayer(lNum)->getMinSpacing();
    if (con) {
      if (con->typeId() == frConstraintTypeEnum::frcSpacingConstraint) {
        minReqDist = static_cast<frSpacingConstraint*>(con)->getMinSpacing();
      } else if (con->typeId()
                 == frConstraintTypeEnum::frcSpacingTablePrlConstraint) {
        minReqDist = static_cast<frSpacingTablePrlConstraint*>(con)->find(
            max(width1, width2), prl1);
      } else if (con->typeId()
                 == frConstraintTypeEnum::frcSpacingTableTwConstraint) {
        minReqDist = static_cast<frSpacingTableTwConstraint*>(con)->find(
            width1, width2, prl1);
      }
      if (ndr)
        minReqDist = max(minReqDist, ndr->getSpacing(lNum / 2 - 1));
      if (minReqDist != INT_MIN) {
        minReqDist += minNonOverlapDist;
      }
    }

    if (minReqDist != INT_MIN) {
      forbiddenRanges.push_back(make_pair(minNonOverlapDist, minReqDist));
    }
  }
}
