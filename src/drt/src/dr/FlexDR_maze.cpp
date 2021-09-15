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

#include <algorithm>
#include <boost/polygon/polygon.hpp>
#include <chrono>
#include <random>
#include <sstream>

#include "dr/FlexDR.h"
#include "dr/FlexDR_graphics.h"
#include "frProfileTask.h"
#include "gc/FlexGC.h"

using namespace std;
using namespace fr;
namespace gtl = boost::polygon;

static frSquaredDistance pt2boxDistSquare(const frPoint& pt, const frBox& box)
{
  frCoord dx = max(max(box.left() - pt.x(), pt.x() - box.right()), 0);
  frCoord dy = max(max(box.bottom() - pt.y(), pt.y() - box.top()), 0);
  return (frSquaredDistance) dx * dx + (frSquaredDistance) dy * dy;
}

static frSquaredDistance pt2ptDistSquare(const frPoint& pt1, const frPoint& pt2)
{
  frCoord dx = abs(pt1.x() - pt2.x());
  frCoord dy = abs(pt1.y() - pt2.y());
  return (frSquaredDistance) dx * dx + (frSquaredDistance) dy * dy;
}

// prlx = -dx, prly = -dy
// dx > 0 : disjoint in x; dx = 0 : touching in x; dx < 0 : overlap in x
static frSquaredDistance box2boxDistSquareNew(const frBox& box1,
                                              const frBox& box2,
                                              frCoord& dx,
                                              frCoord& dy)
{
  dx = max(box1.left(), box2.left()) - min(box1.right(), box2.right());
  dy = max(box1.bottom(), box2.bottom()) - min(box1.top(), box2.top());
  return (frSquaredDistance) max(dx, 0) * max(dx, 0)
         + (frSquaredDistance) max(dy, 0) * max(dy, 0);
}

void FlexDRWorker::modViaForbiddenThrough(const FlexMazeIdx& bi,
                                          const FlexMazeIdx& ei,
                                          int type)
{
  bool isHorz = (bi.y() == ei.y());

  bool isLowerViaForbidden
      = getTech()->isViaForbiddenThrough(bi.z(), true, isHorz);
  bool isUpperViaForbidden
      = getTech()->isViaForbiddenThrough(bi.z(), false, isHorz);

  if (isHorz) {
    for (int xIdx = bi.x(); xIdx < ei.x(); xIdx++) {
      if (isLowerViaForbidden) {
        switch (type) {
          case 0:
            gridGraph_.subRouteShapeCostVia(
                xIdx, bi.y(), bi.z() - 1);  // safe access
            break;
          case 1:
            gridGraph_.addRouteShapeCostVia(
                xIdx, bi.y(), bi.z() - 1);  // safe access
            break;
          case 2:
            gridGraph_.subFixedShapeCostVia(
                xIdx, bi.y(), bi.z() - 1);  // safe access
            break;
          case 3:
            gridGraph_.addFixedShapeCostVia(
                xIdx, bi.y(), bi.z() - 1);  // safe access
            break;
          default:;
        }
      }

      if (isUpperViaForbidden) {
        switch (type) {
          case 0:
            gridGraph_.subRouteShapeCostVia(
                xIdx, bi.y(), bi.z());  // safe access
            break;
          case 1:
            gridGraph_.addRouteShapeCostVia(
                xIdx, bi.y(), bi.z());  // safe access
            break;
          case 2:
            gridGraph_.subFixedShapeCostVia(
                xIdx, bi.y(), bi.z());  // safe access
            break;
          case 3:
            gridGraph_.addFixedShapeCostVia(
                xIdx, bi.y(), bi.z());  // safe access
            break;
          default:;
        }
      }
    }
  } else {
    for (int yIdx = bi.y(); yIdx < ei.y(); yIdx++) {
      if (isLowerViaForbidden) {
        switch (type) {
          case 0:
            gridGraph_.subRouteShapeCostVia(
                bi.x(), yIdx, bi.z() - 1);  // safe access
            break;
          case 1:
            gridGraph_.addRouteShapeCostVia(
                bi.x(), yIdx, bi.z() - 1);  // safe access
            break;
          case 2:
            gridGraph_.subFixedShapeCostVia(
                bi.x(), yIdx, bi.z() - 1);  // safe access
            break;
          case 3:
            gridGraph_.addFixedShapeCostVia(
                bi.x(), yIdx, bi.z() - 1);  // safe access
            break;
          default:;
        }
      }

      if (isUpperViaForbidden) {
        switch (type) {
          case 0:
            gridGraph_.subRouteShapeCostVia(
                bi.x(), yIdx, bi.z());  // safe access
            break;
          case 1:
            gridGraph_.addRouteShapeCostVia(
                bi.x(), yIdx, bi.z());  // safe access
            break;
          case 2:
            gridGraph_.subFixedShapeCostVia(
                bi.x(), yIdx, bi.z());  // safe access
            break;
          case 3:
            gridGraph_.addFixedShapeCostVia(
                bi.x(), yIdx, bi.z());  // safe access
            break;
          default:;
        }
      }
    }
  }
}

void FlexDRWorker::modBlockedPlanar(const frBox& box, frMIdx z, bool setBlock)
{
  FlexMazeIdx mIdx1;
  FlexMazeIdx mIdx2;
  gridGraph_.getIdxBox(mIdx1, mIdx2, box);
  for (int i = mIdx1.x(); i <= mIdx2.x(); i++) {
    for (int j = mIdx1.y(); j <= mIdx2.y(); j++) {
      if (setBlock) {
        gridGraph_.setBlocked(i, j, z, frDirEnum::E);
        gridGraph_.setBlocked(i, j, z, frDirEnum::N);
        gridGraph_.setBlocked(i, j, z, frDirEnum::W);
        gridGraph_.setBlocked(i, j, z, frDirEnum::S);
      } else {
        gridGraph_.resetBlocked(i, j, z, frDirEnum::E);
        gridGraph_.resetBlocked(i, j, z, frDirEnum::N);
        gridGraph_.resetBlocked(i, j, z, frDirEnum::W);
        gridGraph_.resetBlocked(i, j, z, frDirEnum::S);
      }
    }
  }
}

void FlexDRWorker::modBlockedVia(const frBox& box, frMIdx z, bool setBlock)
{
  FlexMazeIdx mIdx1;
  FlexMazeIdx mIdx2;
  gridGraph_.getIdxBox(mIdx1, mIdx2, box);
  for (int i = mIdx1.x(); i <= mIdx2.x(); i++) {
    for (int j = mIdx1.y(); j <= mIdx2.y(); j++) {
      if (setBlock) {
        gridGraph_.setBlocked(i, j, z, frDirEnum::U);
        gridGraph_.setBlocked(i, j, z, frDirEnum::D);
      } else {
        gridGraph_.resetBlocked(i, j, z, frDirEnum::U);
        gridGraph_.resetBlocked(i, j, z, frDirEnum::D);
      }
    }
  }
}

void FlexDRWorker::modCornerToCornerSpacing_helper(const frBox& box,
                                                   frMIdx z,
                                                   int type)
{
  FlexMazeIdx p1, p2;
  gridGraph_.getIdxBox(p1, p2, box, FlexGridGraph::isEnclosed);
  for (int i = p1.x(); i <= p2.x(); i++) {
    for (int j = p1.y(); j <= p2.y(); j++) {
      switch (type) {
        case 0:
          gridGraph_.subRouteShapeCostPlanar(i, j, z);
          break;
        case 1:
          gridGraph_.addRouteShapeCostPlanar(i, j, z);
          break;
        case 2:
          gridGraph_.subFixedShapeCostPlanar(i, j, z);
          break;
        case 3:
          gridGraph_.addFixedShapeCostPlanar(i, j, z);
          break;
        default:;
      }
    }
  }
}
void FlexDRWorker::modCornerToCornerSpacing(const frBox& box,
                                            frMIdx z,
                                            int type)
{
  auto lNum = gridGraph_.getLayerNum(z);
  frCoord halfwidth2 = getTech()->getLayer(lNum)->getWidth() / 2;
  // spacing value needed
  frCoord bloatDist = 0;
  auto& cons = getTech()->getLayer(lNum)->getLef58CornerSpacingConstraints();
  frBox bx;
  for (auto& c : cons) {
    bloatDist = c->findMax() + halfwidth2 - 1;
    bx.set(box.left() - bloatDist,
           box.bottom() - bloatDist,
           box.left(),
           box.bottom());  // ll box corner
    modCornerToCornerSpacing_helper(bx, z, type);
    bx.set(box.left() - bloatDist,
           box.top(),
           box.left(),
           box.top() + bloatDist);  // ul box corner
    modCornerToCornerSpacing_helper(bx, z, type);
    bx.set(box.right(),
           box.top(),
           box.right() + bloatDist,
           box.top() + bloatDist);  // ur box corner
    modCornerToCornerSpacing_helper(bx, z, type);
    bx.set(box.right(),
           box.bottom() - bloatDist,
           box.right() + bloatDist,
           box.bottom());  // lr box corner
    modCornerToCornerSpacing_helper(bx, z, type);
  }
}

/*inline*/ void FlexDRWorker::modMinSpacingCostPlanar(const frBox& box,
                                                      frMIdx z,
                                                      int type,
                                                      bool isBlockage,
                                                      frNonDefaultRule* ndr)
{
  auto lNum = gridGraph_.getLayerNum(z);
  frCoord width1 = box.width();
  frCoord length1 = box.length();
  // layer default width
  frCoord width2 = getTech()->getLayer(lNum)->getWidth();
  frCoord halfwidth2 = width2 / 2;
  // spacing value needed
  frCoord bloatDist = 0;
  auto con = getTech()->getLayer(lNum)->getMinSpacing();
  if (con) {
    if (con->typeId() == frConstraintTypeEnum::frcSpacingConstraint) {
      bloatDist = static_cast<frSpacingConstraint*>(con)->getMinSpacing();
    } else if (con->typeId()
               == frConstraintTypeEnum::frcSpacingTablePrlConstraint) {
      bloatDist
          = (isBlockage && USEMINSPACING_OBS)
                ? static_cast<frSpacingTablePrlConstraint*>(con)->findMin()
                : static_cast<frSpacingTablePrlConstraint*>(con)->find(
                    max(width1, width2), length1);
    } else if (con->typeId()
               == frConstraintTypeEnum::frcSpacingTableTwConstraint) {
      bloatDist = (isBlockage && USEMINSPACING_OBS)
                      ? static_cast<frSpacingTableTwConstraint*>(con)->findMin()
                      : static_cast<frSpacingTableTwConstraint*>(con)->find(
                          width1, width2, length1);
    } else {
      cout << "Warning: min spacing rule not supporterd" << endl;
      return;
    }
  } else {
    cout << "Warning: no min spacing rule" << endl;
    return;
  }
  if (ndr)
    bloatDist = max(bloatDist, ndr->getSpacing(z));
  frSquaredDistance bloatDistSquare = bloatDist;
  bloatDistSquare *= bloatDist;

  FlexMazeIdx mIdx1;
  FlexMazeIdx mIdx2;
  // assumes width always > 2
  frBox bx(box.left() - bloatDist - halfwidth2 + 1,
           box.bottom() - bloatDist - halfwidth2 + 1,
           box.right() + bloatDist + halfwidth2 - 1,
           box.top() + bloatDist + halfwidth2 - 1);
  gridGraph_.getIdxBox(mIdx1, mIdx2, bx);

  frPoint pt, pt1, pt2, pt3, pt4;
  frSquaredDistance distSquare = 0;
  int cnt = 0;
  for (int i = mIdx1.x(); i <= mIdx2.x(); i++) {
    for (int j = mIdx1.y(); j <= mIdx2.y(); j++) {
      gridGraph_.getPoint(pt, i, j);
      pt1.set(pt.x() + halfwidth2, pt.y() - halfwidth2);
      pt2.set(pt.x() + halfwidth2, pt.y() + halfwidth2);
      pt3.set(pt.x() - halfwidth2, pt.y() - halfwidth2);
      pt4.set(pt.x() - halfwidth2, pt.y() + halfwidth2);
      distSquare = min(pt2boxDistSquare(pt1, box), pt2boxDistSquare(pt2, box));
      distSquare = min(pt2boxDistSquare(pt3, box), distSquare);
      distSquare = min(pt2boxDistSquare(pt4, box), distSquare);
      if (distSquare < bloatDistSquare) {
        switch (type) {
          case 0:
            gridGraph_.subRouteShapeCostPlanar(i, j, z);  // safe access
            break;
          case 1:
            gridGraph_.addRouteShapeCostPlanar(i, j, z);  // safe access
            break;
          case 2:
            gridGraph_.subFixedShapeCostPlanar(i, j, z);  // safe access
            break;
          case 3:
            gridGraph_.addFixedShapeCostPlanar(i, j, z);  // safe access
            break;
          default:;
        }
        cnt++;
        // if (!isInitDR()) {
        //   cout <<" planer find viol mIdx (" <<i <<", " <<j <<") " <<pt
        //   <<endl;
        // }
      }
    }
  }
  // cout <<"planer mod " <<cnt <<" edges" <<endl;
}


void FlexDRWorker::modMinSpacingCostVia_eol_helper(const frBox& box,
                                                   const frBox& testBox,
                                                   int type,
                                                   bool isUpperVia,
                                                   frMIdx i,
                                                   frMIdx j,
                                                   frMIdx z)
{
  if (testBox.overlaps(box, false)) {
    if (isUpperVia) {
      switch (type) {
        case 0:
          gridGraph_.subRouteShapeCostVia(i, j, z);
          break;
        case 1:
          gridGraph_.addRouteShapeCostVia(i, j, z);
          break;
        case 2:
          gridGraph_.subFixedShapeCostVia(i, j, z);  // safe access
          break;
        case 3:
          gridGraph_.addFixedShapeCostVia(i, j, z);  // safe access
          break;
        default:;
      }
    } else {
      switch (type) {
        case 0:
          gridGraph_.subRouteShapeCostVia(i, j, z - 1);
          break;
        case 1:
          gridGraph_.addRouteShapeCostVia(i, j, z - 1);
          break;
        case 2:
          gridGraph_.subFixedShapeCostVia(i, j, z - 1);  // safe access
          break;
        case 3:
          gridGraph_.addFixedShapeCostVia(i, j, z - 1);  // safe access
          break;
        default:;
      }
    }
  }
}

void FlexDRWorker::modMinSpacingCostVia_eol(const frBox& box,
                                            const frBox& tmpBx,
                                            int type,
                                            bool isUpperVia,
                                            frMIdx i,
                                            frMIdx j,
                                            frMIdx z)
{
  auto lNum = gridGraph_.getLayerNum(z);
  frBox testBox;
  if (getTech()->getLayer(lNum)->hasEolSpacing()) {
    for (auto eolCon : getTech()->getLayer(lNum)->getEolSpacing()) {
      auto eolSpace = eolCon->getMinSpacing();
      auto eolWidth = eolCon->getEolWidth();
      auto eolWithin = eolCon->getEolWithin();
      // eol to up and down
      if (tmpBx.right() - tmpBx.left() < eolWidth) {
        testBox.set(tmpBx.left() - eolWithin,
                    tmpBx.top(),
                    tmpBx.right() + eolWithin,
                    tmpBx.top() + eolSpace);
        modMinSpacingCostVia_eol_helper(
            box, testBox, type, isUpperVia, i, j, z);

        testBox.set(tmpBx.left() - eolWithin,
                    tmpBx.bottom() - eolSpace,
                    tmpBx.right() + eolWithin,
                    tmpBx.bottom());
        modMinSpacingCostVia_eol_helper(
            box, testBox, type, isUpperVia, i, j, z);
      }
      // eol to left and right
      if (tmpBx.top() - tmpBx.bottom() < eolWidth) {
        testBox.set(tmpBx.right(),
                    tmpBx.bottom() - eolWithin,
                    tmpBx.right() + eolSpace,
                    tmpBx.top() + eolWithin);
        modMinSpacingCostVia_eol_helper(
            box, testBox, type, isUpperVia, i, j, z);

        testBox.set(tmpBx.left() - eolSpace,
                    tmpBx.bottom() - eolWithin,
                    tmpBx.left(),
                    tmpBx.top() + eolWithin);
        modMinSpacingCostVia_eol_helper(
            box, testBox, type, isUpperVia, i, j, z);
      }
    }
  }
  for (auto eolCon : getTech()->getLayer(lNum)->getLef58SpacingEndOfLineConstraints()) {
      auto eolSpace = eolCon->getEolSpace();
      auto eolWidth = eolCon->getEolWidth();
      frCoord eolWithin = 0;
      if(eolCon->hasWithinConstraint())
        eolWithin = eolCon->getWithinConstraint()->getEolWithin();
      // eol to up and down
      if (tmpBx.right() - tmpBx.left() < eolWidth) {
        testBox.set(tmpBx.left() - eolWithin,
                    tmpBx.top(),
                    tmpBx.right() + eolWithin,
                    tmpBx.top() + eolSpace);
        modMinSpacingCostVia_eol_helper(
            box, testBox, type, isUpperVia, i, j, z);

        testBox.set(tmpBx.left() - eolWithin,
                    tmpBx.bottom() - eolSpace,
                    tmpBx.right() + eolWithin,
                    tmpBx.bottom());
        modMinSpacingCostVia_eol_helper(
            box, testBox, type, isUpperVia, i, j, z);
      }
      // eol to left and right
      if (tmpBx.top() - tmpBx.bottom() < eolWidth) {
        testBox.set(tmpBx.right(),
                    tmpBx.bottom() - eolWithin,
                    tmpBx.right() + eolSpace,
                    tmpBx.top() + eolWithin);
        modMinSpacingCostVia_eol_helper(
            box, testBox, type, isUpperVia, i, j, z);

        testBox.set(tmpBx.left() - eolSpace,
                    tmpBx.bottom() - eolWithin,
                    tmpBx.left(),
                    tmpBx.top() + eolWithin);
        modMinSpacingCostVia_eol_helper(
            box, testBox, type, isUpperVia, i, j, z);
      }
    }
}

void FlexDRWorker::modMinimumcutCostVia(const frBox& box,
                                        frMIdx z,
                                        int type,
                                        bool isUpperVia)
{
  auto lNum = gridGraph_.getLayerNum(z);
  frCoord width1 = box.width();
  frCoord length1 = box.length();
  // default via dimension
  frViaDef* viaDef = nullptr;
  if (isUpperVia) {
    viaDef = (lNum < getTech()->getTopLayerNum())
                 ? getTech()->getLayer(lNum + 1)->getDefaultViaDef()
                 : nullptr;
  } else {
    viaDef = (lNum > getTech()->getBottomLayerNum())
                 ? getTech()->getLayer(lNum - 1)->getDefaultViaDef()
                 : nullptr;
  }
  if (viaDef == nullptr) {
    return;
  }
  frVia via(viaDef);
  frBox viaBox(0, 0, 0, 0);
  if (isUpperVia) {
    via.getCutBBox(viaBox);
  } else {
    via.getCutBBox(viaBox);
  }

  FlexMazeIdx mIdx1, mIdx2;
  frBox bx, tmpBx, sViaBox;
  frTransform xform;
  frPoint pt;
  frCoord dx, dy;
  frVia sVia;
  for (auto& con : getTech()->getLayer(lNum)->getMinimumcutConstraints()) {
    // check via2cut to box
    // check whether via can be placed on the pin
    if ((!con->hasLength() || (con->hasLength() && length1 > con->getLength()))
        && width1 > con->getWidth()) {
      bool checkVia2 = false;
      if (!con->hasConnection()) {
        checkVia2 = true;
      } else {
        if (con->getConnection() == frMinimumcutConnectionEnum::FROMABOVE
            && isUpperVia) {
          checkVia2 = true;
        } else if (con->getConnection() == frMinimumcutConnectionEnum::FROMBELOW
                   && !isUpperVia) {
          checkVia2 = true;
        }
      }
      if (!checkVia2) {
        continue;
      }
      // block via on pin
      frCoord dist = 0;
      if (con->hasLength()) {
        dist = con->getDistance();
        // conservative for macro pin
        // TODO: revert the += to be more accurate and check qor change
        dist += getTech()->getLayer(lNum)->getPitch();
      }
      // assumes width always > 2
      bx.set(box.left() - dist - (viaBox.right() - 0) + 1,
             box.bottom() - dist - (viaBox.top() - 0) + 1,
             box.right() + dist + (0 - viaBox.left()) - 1,
             box.top() + dist + (0 - viaBox.bottom()) - 1);
      gridGraph_.getIdxBox(mIdx1, mIdx2, bx);

      for (int i = mIdx1.x(); i <= mIdx2.x(); i++) {
        for (int j = mIdx1.y(); j <= mIdx2.y(); j++) {
          gridGraph_.getPoint(pt, i, j);
          xform.set(pt);
          tmpBx.set(viaBox);
          if (gridGraph_.isSVia(i, j, isUpperVia ? z : z - 1)) {
            auto sViaDef = apSVia_[FlexMazeIdx(i, j, isUpperVia ? z : z - 1)]
                               ->getAccessViaDef();
            sVia.setViaDef(sViaDef);
            if (isUpperVia) {
              sVia.getCutBBox(sViaBox);
            } else {
              sVia.getCutBBox(sViaBox);
            }
            tmpBx.set(sViaBox);
          }
          tmpBx.transform(xform);
          box2boxDistSquareNew(box, tmpBx, dx, dy);
          if (!con->hasLength()) {
            if (dx <= 0 && dy <= 0) {
              ;
            } else {
              continue;
            }
          } else {
            if (dx > 0 && dy > 0 && dx + dy < dist) {
              ;
            } else {
              continue;
            }
          }
          if (isUpperVia) {
            switch (type) {
              case 0:
                gridGraph_.subRouteShapeCostVia(i, j, z);  // safe access
                break;
              case 1:
                gridGraph_.addRouteShapeCostVia(i, j, z);  // safe access
                break;
              case 2:
                gridGraph_.subFixedShapeCostVia(i, j, z);  // safe access
                break;
              case 3:
                gridGraph_.addFixedShapeCostVia(i, j, z);  // safe access
                break;
              default:;
            }
          } else {
            switch (type) {
              case 0:
                gridGraph_.subRouteShapeCostVia(i, j, z - 1);  // safe access
                break;
              case 1:
                gridGraph_.addRouteShapeCostVia(i, j, z - 1);  // safe access
                break;
              case 2:
                gridGraph_.subFixedShapeCostVia(i, j, z - 1);  // safe access
                break;
              case 3:
                gridGraph_.addFixedShapeCostVia(i, j, z - 1);  // safe access
                break;
              default:;
            }
          }
        }
      }
    }
  }
}

void FlexDRWorker::modMinSpacingCostVia(const frBox& box,
                                        frMIdx z,
                                        int type,
                                        bool isUpperVia,
                                        bool isCurrPs,
                                        bool isBlockage,
                                        frNonDefaultRule* ndr)
{
  auto lNum = gridGraph_.getLayerNum(z);
  frCoord width1 = box.width();
  frCoord length1 = box.length();
  // default via dimension
  frViaDef* viaDef = nullptr;
  if (isUpperVia) {
    viaDef = (lNum < getTech()->getTopLayerNum())
                 ? getTech()->getLayer(lNum + 1)->getDefaultViaDef()
                 : nullptr;
  } else {
    viaDef = (lNum > getTech()->getBottomLayerNum())
                 ? getTech()->getLayer(lNum - 1)->getDefaultViaDef()
                 : nullptr;
  }
  if (viaDef == nullptr) {
    return;
  }
  frVia via(viaDef);
  frBox viaBox(0, 0, 0, 0);
  if (isUpperVia) {
    via.getLayer1BBox(viaBox);
  } else {
    via.getLayer2BBox(viaBox);
  }
  frCoord width2 = viaBox.width();
  frCoord length2 = viaBox.length();

  // via prl should check min area patch metal if not fat via
  frCoord defaultWidth = getTech()->getLayer(lNum)->getWidth();
  bool isH = (getTech()->getLayer(lNum)->getDir()
              == frPrefRoutingDirEnum::frcHorzPrefRoutingDir);
  bool isFatVia = (isH) ? (viaBox.top() - viaBox.bottom() > defaultWidth)
                        : (viaBox.right() - viaBox.left() > defaultWidth);

  frCoord length2_mar = length2;
  frCoord patchLength = 0;
  if (!isFatVia) {
    auto minAreaConstraint = getTech()->getLayer(lNum)->getAreaConstraint();
    auto minArea = minAreaConstraint ? minAreaConstraint->getMinArea() : 0;
    patchLength = frCoord(ceil(1.0 * minArea / defaultWidth
                               / getTech()->getManufacturingGrid()))
                  * frCoord(getTech()->getManufacturingGrid());
    length2_mar = max(length2_mar, patchLength);
  }

  // spacing value needed
  frCoord bloatDist = 0;
  auto con = getTech()->getLayer(lNum)->getMinSpacing();
  if (con) {
    if (con->typeId() == frConstraintTypeEnum::frcSpacingConstraint) {
      bloatDist = static_cast<frSpacingConstraint*>(con)->getMinSpacing();
    } else if (con->typeId()
               == frConstraintTypeEnum::frcSpacingTablePrlConstraint) {
      bloatDist
          = (isBlockage && USEMINSPACING_OBS && !isFatVia)
                ? static_cast<frSpacingTablePrlConstraint*>(con)->findMin()
                : static_cast<frSpacingTablePrlConstraint*>(con)->find(
                    max(width1, width2),
                    isCurrPs ? (length2_mar) : min(length1, length2_mar));
    } else if (con->typeId()
               == frConstraintTypeEnum::frcSpacingTableTwConstraint) {
      bloatDist = (isBlockage && USEMINSPACING_OBS && !isFatVia)
                      ? static_cast<frSpacingTableTwConstraint*>(con)->findMin()
                      : static_cast<frSpacingTableTwConstraint*>(con)->find(
                          width1,
                          width2,
                          isCurrPs ? (length2_mar) : min(length1, length2_mar));
    } else {
      cout << "Warning: min spacing rule not supporterd" << endl;
      return;
    }
  } else {
    cout << "Warning: no min spacing rule" << endl;
    return;
  }
  if (ndr)
    bloatDist = max(bloatDist, ndr->getSpacing(z));
  // other obj eol spc to curr obj
  // no need to bloat eolWithin because eolWithin always < minSpacing
  frCoord bloatDistEolX = 0;
  frCoord bloatDistEolY = 0;
  for (auto con : getTech()->getLayer(lNum)->getEolSpacing()) {
    auto eolSpace = con->getMinSpacing();
    auto eolWidth = con->getEolWidth();
    // eol up and down
    if (viaBox.right() - viaBox.left() < eolWidth) {
      bloatDistEolY = max(bloatDistEolY, eolSpace);
    }
    // eol left and right
    if (viaBox.top() - viaBox.bottom() < eolWidth) {
      bloatDistEolX = max(bloatDistEolX, eolSpace);
    }
  }
  for (auto con : getTech()->getLayer(lNum)->getLef58SpacingEndOfLineConstraints()) {
    auto eolSpace = con->getEolSpace();
    auto eolWidth = con->getEolWidth();
    // eol up and down
    if (viaBox.right() - viaBox.left() < eolWidth) {
      bloatDistEolY = max(bloatDistEolY, eolSpace);
    }
    // eol left and right
    if (viaBox.top() - viaBox.bottom() < eolWidth) {
      bloatDistEolX = max(bloatDistEolX, eolSpace);
    }
  }

  FlexMazeIdx mIdx1;
  FlexMazeIdx mIdx2;
  // assumes width always > 2
  frBox bx(
      box.left() - max(bloatDist, bloatDistEolX) - (viaBox.right() - 0) + 1,
      box.bottom() - max(bloatDist, bloatDistEolY) - (viaBox.top() - 0) + 1,
      box.right() + max(bloatDist, bloatDistEolX) + (0 - viaBox.left()) - 1,
      box.top() + max(bloatDist, bloatDistEolY) + (0 - viaBox.bottom()) - 1);
  gridGraph_.getIdxBox(mIdx1, mIdx2, bx);
  frPoint pt;
  frBox tmpBx;
  frSquaredDistance distSquare = 0;
  frCoord dx, dy, prl;
  frTransform xform;
  frCoord reqDist = 0;
  frBox sViaBox;
  frVia sVia;
  for (int i = mIdx1.x(); i <= mIdx2.x(); i++) {
    for (int j = mIdx1.y(); j <= mIdx2.y(); j++) {
      gridGraph_.getPoint(pt, i, j);
      xform.set(pt);
      tmpBx.set(viaBox);
      if (gridGraph_.isSVia(i, j, isUpperVia ? z : z - 1)) {
        auto sViaDef = apSVia_[FlexMazeIdx(i, j, isUpperVia ? z : z - 1)]
                           ->getAccessViaDef();
        sVia.setViaDef(sViaDef);
        if (isUpperVia) {
          sVia.getLayer1BBox(sViaBox);
        } else {
          sVia.getLayer2BBox(sViaBox);
        }
        tmpBx.set(sViaBox);
      }
      tmpBx.transform(xform);
      distSquare = box2boxDistSquareNew(box, tmpBx, dx, dy);
      prl = max(-dx, -dy);
      // curr is ps
      if (isCurrPs) {
        if (-dy >= 0 && prl == -dy) {
          prl = viaBox.top() - viaBox.bottom();
          // ignore svia effect here...
          if (!isH && !isFatVia) {
            prl = max(prl, patchLength);
          }
        } else if (-dx >= 0 && prl == -dx) {
          prl = viaBox.right() - viaBox.left();
          if (isH && !isFatVia) {
            prl = max(prl, patchLength);
          }
        }
      } else {
        ;
      }
      if (con->typeId() == frConstraintTypeEnum::frcSpacingConstraint) {
        reqDist = static_cast<frSpacingConstraint*>(con)->getMinSpacing();
      } else if (con->typeId()
                 == frConstraintTypeEnum::frcSpacingTablePrlConstraint) {
        reqDist
            = (isBlockage && USEMINSPACING_OBS && !isFatVia)
                  ? static_cast<frSpacingTablePrlConstraint*>(con)->findMin()
                  : static_cast<frSpacingTablePrlConstraint*>(con)->find(
                      max(width1, width2), prl);
      } else if (con->typeId()
                 == frConstraintTypeEnum::frcSpacingTableTwConstraint) {
        reqDist = (isBlockage && USEMINSPACING_OBS && !isFatVia)
                      ? static_cast<frSpacingTableTwConstraint*>(con)->findMin()
                      : static_cast<frSpacingTableTwConstraint*>(con)->find(
                          width1, width2, prl);
      }
      if (ndr)
        reqDist = max(reqDist, ndr->getSpacing(z));
      if (distSquare < (frSquaredDistance) reqDist * reqDist) {
        if (isUpperVia) {
          switch (type) {
            case 0:
              gridGraph_.subRouteShapeCostVia(i, j, z);  // safe access
              break;
            case 1:
              gridGraph_.addRouteShapeCostVia(i, j, z);  // safe access
              break;
            case 2:
              gridGraph_.subFixedShapeCostVia(i, j, z);  // safe access
              break;
            case 3:
              gridGraph_.addFixedShapeCostVia(i, j, z);  // safe access
              break;
            default:;
          }
        } else {
          switch (type) {
            case 0:
              gridGraph_.subRouteShapeCostVia(i, j, z - 1);  // safe access
              break;
            case 1:
              gridGraph_.addRouteShapeCostVia(i, j, z - 1);  // safe access
              break;
            case 2:
              gridGraph_.subFixedShapeCostVia(i, j, z - 1);  // safe access
              break;
            case 3:
              gridGraph_.addFixedShapeCostVia(i, j, z - 1);  // safe access
              break;
            default:;
          }
        }
      }
      // eol, other obj to curr obj
      modMinSpacingCostVia_eol(box, tmpBx, type, isUpperVia, i, j, z);
    }
  }
}

// eolType == 0: planer
// eolType == 1: down
// eolType == 2: up
/*inline*/ void FlexDRWorker::modEolSpacingCost_helper(const frBox& testbox,
                                                       frMIdx z,
                                                       int type,
                                                       int eolType)
{
  auto lNum = gridGraph_.getLayerNum(z);
  frBox bx;
  if (eolType == 0) {
    // layer default width
    frCoord width2 = getTech()->getLayer(lNum)->getWidth();
    frCoord halfwidth2 = width2 / 2;
    // assumes width always > 2
    bx.set(testbox.left() - halfwidth2 + 1,
           testbox.bottom() - halfwidth2 + 1,
           testbox.right() + halfwidth2 - 1,
           testbox.top() + halfwidth2 - 1);
  } else {
    // default via dimension
    frViaDef* viaDef = nullptr;
    if (eolType == 1) {
      viaDef = (lNum > getTech()->getBottomLayerNum())
                   ? getTech()->getLayer(lNum - 1)->getDefaultViaDef()
                   : nullptr;
    } else if (eolType == 2) {
      viaDef = (lNum < getTech()->getTopLayerNum())
                   ? getTech()->getLayer(lNum + 1)->getDefaultViaDef()
                   : nullptr;
    }
    if (viaDef == nullptr) {
      return;
    }
    frVia via(viaDef);
    frBox viaBox(0, 0, 0, 0);
    if (eolType == 2) {  // upper via
      via.getLayer1BBox(viaBox);
    } else {
      via.getLayer2BBox(viaBox);
    }
    // assumes via bbox always > 2
    bx.set(testbox.left() - (viaBox.right() - 0) + 1,
           testbox.bottom() - (viaBox.top() - 0) + 1,
           testbox.right() + (0 - viaBox.left()) - 1,
           testbox.top() + (0 - viaBox.bottom()) - 1);
  }

  FlexMazeIdx mIdx1;
  FlexMazeIdx mIdx2;
  gridGraph_.getIdxBox(mIdx1, mIdx2, bx);  // >= bx

  frVia sVia;
  frBox sViaBox;
  frTransform xform;
  frPoint pt;

  for (int i = mIdx1.x(); i <= mIdx2.x(); i++) {
    for (int j = mIdx1.y(); j <= mIdx2.y(); j++) {
      if (eolType == 0) {
        switch (type) {
          case 0:
            gridGraph_.subRouteShapeCostPlanar(i, j, z);  // safe access
            break;
          case 1:
            gridGraph_.addRouteShapeCostPlanar(i, j, z);  // safe access
            break;
          case 2:
            gridGraph_.subFixedShapeCostPlanar(i, j, z);  // safe access
            break;
          case 3:
            gridGraph_.addFixedShapeCostPlanar(i, j, z);  // safe access
            break;
          default:;
        }
      } else if (eolType == 1) {
        if (gridGraph_.isSVia(i, j, z - 1)) {
          gridGraph_.getPoint(pt, i, j);
          auto sViaDef = apSVia_[FlexMazeIdx(i, j, z - 1)]->getAccessViaDef();
          sVia.setViaDef(sViaDef);
          sVia.setOrigin(pt);
          sVia.getLayer2BBox(sViaBox);
          if (!sViaBox.overlaps(testbox, false)) {
            continue;
          }
        }
        switch (type) {
          case 0:
            gridGraph_.subRouteShapeCostVia(i, j, z - 1);  // safe access
            break;
          case 1:
            gridGraph_.addRouteShapeCostVia(i, j, z - 1);  // safe access
            break;
          case 2:
            gridGraph_.subFixedShapeCostVia(i, j, z - 1);  // safe access
            break;
          case 3:
            gridGraph_.addFixedShapeCostVia(i, j, z - 1);  // safe access
            break;
          default:;
        }
      } else if (eolType == 2) {
        if (gridGraph_.isSVia(i, j, z)) {
          gridGraph_.getPoint(pt, i, j);
          auto sViaDef = apSVia_[FlexMazeIdx(i, j, z)]->getAccessViaDef();
          sVia.setViaDef(sViaDef);
          sVia.setOrigin(pt);
          sVia.getLayer1BBox(sViaBox);
          if (!sViaBox.overlaps(testbox, false)) {
            continue;
          }
        }
        switch (type) {
          case 0:
            gridGraph_.subRouteShapeCostVia(i, j, z);  // safe access
            break;
          case 1:
            gridGraph_.addRouteShapeCostVia(i, j, z);  // safe access
            break;
          case 2:
            gridGraph_.subFixedShapeCostVia(i, j, z);  // safe access
            break;
          case 3:
            gridGraph_.addFixedShapeCostVia(i, j, z);  // safe access
            break;
          default:;
        }
      }
    }
  }
}

void FlexDRWorker::modEolSpacingCost(const frBox& box,
                                     frMIdx z,
                                     int type,
                                     frConstraint* con,
                                     bool isSkipVia)
{
  frCoord eolSpace, eolWidth, eolWithin;
  if (con->typeId()
      == frConstraintTypeEnum::frcLef58SpacingEndOfLineConstraint) {
    frLef58SpacingEndOfLineConstraint* constraint
        = (frLef58SpacingEndOfLineConstraint*) con;
    eolSpace = constraint->getEolSpace();
    eolWidth = constraint->getEolWidth();
    if (constraint->hasWithinConstraint())
      eolWithin = constraint->getWithinConstraint()->getEolWithin();
    else
      eolWithin = 0;
  } else if (con->typeId()
             == frConstraintTypeEnum::frcSpacingEndOfLineConstraint) {
    frSpacingEndOfLineConstraint* constraint
        = (frSpacingEndOfLineConstraint*) con;
    eolSpace = constraint->getMinSpacing();
    eolWidth = constraint->getEolWidth();
    eolWithin = constraint->getEolWithin();
  } else
    return;
  frBox testBox;
  if (box.right() - box.left() < eolWidth) {
    testBox.set(box.left() - eolWithin,
                box.top(),
                box.right() + eolWithin,
                box.top() + eolSpace);
    // if (!isInitDR()) {
    modEolSpacingCost_helper(testBox, z, type, 0);
    if (!isSkipVia) {
      modEolSpacingCost_helper(testBox, z, type, 1);
      modEolSpacingCost_helper(testBox, z, type, 2);
    }
    testBox.set(box.left() - eolWithin,
                box.bottom() - eolSpace,
                box.right() + eolWithin,
                box.bottom());
    modEolSpacingCost_helper(testBox, z, type, 0);
    if (!isSkipVia) {
      modEolSpacingCost_helper(testBox, z, type, 1);
      modEolSpacingCost_helper(testBox, z, type, 2);
    }
  }
  // eol to left and right
  if (box.top() - box.bottom() < eolWidth) {
    testBox.set(box.right(),
                box.bottom() - eolWithin,
                box.right() + eolSpace,
                box.top() + eolWithin);
    modEolSpacingCost_helper(testBox, z, type, 0);
    if (!isSkipVia) {
      modEolSpacingCost_helper(testBox, z, type, 1);
      modEolSpacingCost_helper(testBox, z, type, 2);
    }
    testBox.set(box.left() - eolSpace,
                box.bottom() - eolWithin,
                box.left(),
                box.top() + eolWithin);
    modEolSpacingCost_helper(testBox, z, type, 0);
    if (!isSkipVia) {
      modEolSpacingCost_helper(testBox, z, type, 1);
      modEolSpacingCost_helper(testBox, z, type, 2);
    }
  }
}
void FlexDRWorker::modEolSpacingRulesCost(const frBox& box,
                                          frMIdx z,
                                          int type,
                                          bool isSkipVia)
{
  auto layer = getTech()->getLayer(gridGraph_.getLayerNum(z));
  frBox testBox;
  if (layer->hasEolSpacing())
    for (auto con : layer->getEolSpacing())
      modEolSpacingCost(box, z, type, con, isSkipVia);
  for (auto con : layer->getLef58SpacingEndOfLineConstraints())
    modEolSpacingCost(box, z, type, con.get(), isSkipVia);
}

// forbid via if it would trigger violation
void FlexDRWorker::modAdjCutSpacingCost_fixedObj(const frDesign* design,
                                                 const frBox& origCutBox,
                                                 frVia* origVia)
{
  if (origVia->getNet()->getType() != frNetEnum::frcPowerNet
      && origVia->getNet()->getType() != frNetEnum::frcGroundNet) {
    return;
  }
  auto lNum = origVia->getViaDef()->getCutLayerNum();
  for (auto con : getTech()->getLayer(lNum)->getCutSpacing()) {
    if (con->getAdjacentCuts() == -1) {
      continue;
    }
    bool hasFixedViol = false;

    gtl::point_data<frCoord> origCenter(
        (origCutBox.left() + origCutBox.right()) / 2,
        (origCutBox.bottom() + origCutBox.top()) / 2);
    gtl::rectangle_data<frCoord> origCutRect(origCutBox.left(),
                                             origCutBox.bottom(),
                                             origCutBox.right(),
                                             origCutBox.top());

    frBox viaBox;
    origVia->getCutBBox(viaBox);

    frSquaredDistance reqDistSquare = con->getCutSpacing();
    reqDistSquare *= reqDistSquare;

    auto cutWithin = con->getCutWithin();
    frBox queryBox;
    viaBox.bloat(cutWithin, queryBox);

    frRegionQuery::Objects<frBlockObject> result;
    design->getRegionQuery()->query(queryBox, lNum, result);

    for (auto& [box, obj] : result) {
      if (obj->typeId() == frcVia) {
        auto via = static_cast<frVia*>(obj);
        if (via->getNet()->getType() != frNetEnum::frcPowerNet
            && via->getNet()->getType() != frNetEnum::frcGroundNet) {
          continue;
        }
        if (origCutBox == box) {
          continue;
        }

        gtl::rectangle_data<frCoord> cutRect(
            box.left(), box.bottom(), box.right(), box.top());
        gtl::point_data<frCoord> cutCenterPt((box.left() + box.right()) / 2,
                                             (box.bottom() + box.top()) / 2);

        frSquaredDistance distSquare = 0;
        if (con->hasCenterToCenter()) {
          distSquare = gtl::distance_squared(origCenter, cutCenterPt);
        } else {
          distSquare = gtl::square_euclidean_distance(origCutRect, cutRect);
        }

        if (distSquare < reqDistSquare) {
          hasFixedViol = true;
          break;
        }
      }
    }

    // block adjacent via idx if will trigger violation
    // pessimistic since block a box
    if (hasFixedViol) {
      FlexMazeIdx mIdx1, mIdx2;
      frBox spacingBox;
      auto reqDist = con->getCutSpacing();
      auto cutWidth = getTech()->getLayer(lNum)->getWidth();
      if (con->hasCenterToCenter()) {
        spacingBox.set(origCenter.x() - reqDist,
                       origCenter.y() - reqDist,
                       origCenter.x() + reqDist,
                       origCenter.y() + reqDist);
      } else {
        origCutBox.bloat(reqDist + cutWidth / 2, spacingBox);
      }
      // cout << "  @@@ debug: blocking for adj (" << spacingBox.left() / 2000.0
      // << ", " << spacingBox.bottom() / 2000.0
      //      << ") -- (" << spacingBox.right() / 2000.0 << ", " <<
      //      spacingBox.top() / 2000.0 << ")\n";
      gridGraph_.getIdxBox(mIdx1, mIdx2, spacingBox);

      frMIdx zIdx
          = gridGraph_.getMazeZIdx(origVia->getViaDef()->getLayer1Num());
      for (int i = mIdx1.x(); i <= mIdx2.x(); i++) {
        for (int j = mIdx1.y(); j <= mIdx2.y(); j++) {
          gridGraph_.setBlocked(i, j, zIdx, frDirEnum::U);
        }
      }
    }
  }
}

bool checkLef58CutSpacingViol(const frBox& box1,
                              const frBox& box2,
                              frString cutClass1,
                              frString cutClass2,
                              frCoord prl,
                              frSquaredDistance distSquare,
                              frSquaredDistance c2cSquare,
                              bool isCurrDirY,
                              odb::dbTechLayerCutSpacingTableDefRule* dbRule)
{
  frSquaredDistance currDistSquare, reqDistSquare;
  bool isSide1, isSide2;
  if (isCurrDirY) {
    isSide1 = (box1.right() - box1.left()) > (box1.top() - box1.bottom());
    isSide2 = (box2.right() - box2.left()) > (box2.top() - box2.bottom());
  } else {
    isSide1 = (box1.right() - box1.left()) < (box1.top() - box1.bottom());
    isSide2 = (box2.right() - box2.left()) < (box2.top() - box2.bottom());
  }
  auto reqPrl = dbRule->getPrlEntry(cutClass1, cutClass2);
  if (dbRule->isCenterAndEdge(cutClass1, cutClass2) && dbRule->isNoPrl()) {
    reqDistSquare
        = dbRule->getSpacing(cutClass1,
                             isSide1,
                             cutClass2,
                             isSide2,
                             odb::dbTechLayerCutSpacingTableDefRule::MAX);
    reqDistSquare *= reqDistSquare;
    if (c2cSquare < reqDistSquare)
      return true;

    reqDistSquare
        = dbRule->getSpacing(cutClass1,
                             isSide1,
                             cutClass2,
                             isSide2,
                             odb::dbTechLayerCutSpacingTableDefRule::MIN);
    reqDistSquare *= reqDistSquare;
    if (distSquare < reqDistSquare)
      return true;
    return false;
  }
  if (cutClass1 == cutClass2 && box1.width() == box1.length()) {
    bool exactlyAligned = false;
    if (box2.left() == box1.left() && box2.right() == box1.right()
        && !dbRule->isHorizontal())
      exactlyAligned = true;
    else if (box2.bottom() == box1.bottom() && box2.top() == box1.top()
             && !dbRule->isVertical())
      exactlyAligned = true;

    auto exAlSpc = dbRule->getExactAlignedSpacing(cutClass1);
    if (exactlyAligned && exAlSpc != -1) {
      reqDistSquare = (frSquaredDistance) exAlSpc * exAlSpc;
      if (distSquare < reqDistSquare) {
        return true;
      }
    }
  }
  if (prl > reqPrl)
    reqDistSquare
        = dbRule->getSpacing(cutClass1,
                             isSide1,
                             cutClass2,
                             isSide2,
                             odb::dbTechLayerCutSpacingTableDefRule::SECOND);
  else
    reqDistSquare
        = dbRule->getSpacing(cutClass1,
                             isSide1,
                             cutClass2,
                             isSide2,
                             odb::dbTechLayerCutSpacingTableDefRule::FIRST);

  if (dbRule->isCenterToCenter(cutClass1, cutClass2))
    currDistSquare = c2cSquare;
  else if (dbRule->isCenterAndEdge(cutClass1, cutClass2)
           && (frCoord) reqDistSquare
                  == dbRule->getSpacing(
                      cutClass1,
                      isSide1,
                      cutClass2,
                      isSide2,
                      odb::dbTechLayerCutSpacingTableDefRule::MAX))
    currDistSquare = c2cSquare;
  else
    currDistSquare = distSquare;

  reqDistSquare *= reqDistSquare;
  if (currDistSquare < reqDistSquare)
    return true;
  return false;
}

/*inline*/ void FlexDRWorker::modCutSpacingCost(const frBox& box,
                                                frMIdx z,
                                                int type,
                                                bool isBlockage)
{
  auto lNum = gridGraph_.getLayerNum(z) + 1;
  auto cutLayer = getTech()->getLayer(lNum);
  if (!cutLayer->hasCutSpacing()
      && !cutLayer->hasLef58SameMetalCutSpcTblConstraint()
      && !cutLayer->hasLef58SameNetCutSpcTblConstraint()
      && !cutLayer->hasLef58DiffNetCutSpcTblConstraint()) {
    return;
  }
  // obj1 = curr obj
  // obj2 = other obj
  // default via dimension
  frViaDef* viaDef = cutLayer->getDefaultViaDef();
  frVia via(viaDef);
  frBox viaBox(0, 0, 0, 0);
  via.getCutBBox(viaBox);

  frString cutClass1 = "";
  frString cutClass2 = "";
  int cutClassIdx1, cutClassIdx2;
  cutClassIdx1 = cutLayer->getCutClassIdx(box.width(), box.length());
  cutClassIdx2 = cutLayer->getCutClassIdx(viaBox.width(), viaBox.length());
  if (cutClassIdx1 != -1)
    cutClass1 = cutLayer->getCutClass(cutClassIdx1)->getName();
  if (cutClassIdx2 != -1)
    cutClass2 = cutLayer->getCutClass(cutClassIdx2)->getName();

  // spacing value needed
  frCoord bloatDist = 0;
  for (auto con : cutLayer->getCutSpacing()) {
    bloatDist = max(bloatDist, con->getCutSpacing());
    if (con->getAdjacentCuts() != -1 && isBlockage) {
      bloatDist = max(bloatDist, con->getCutWithin());
    }
  }
  frLef58CutSpacingTableConstraint* lef58con = nullptr;
  if (cutLayer->hasLef58SameMetalCutSpcTblConstraint())
    lef58con = cutLayer->getLef58SameMetalCutSpcTblConstraint();
  else if (cutLayer->hasLef58SameNetCutSpcTblConstraint())
    lef58con = cutLayer->getLef58SameNetCutSpcTblConstraint();
  else if (cutLayer->hasLef58DiffNetCutSpcTblConstraint())
    lef58con = cutLayer->getLef58DiffNetCutSpcTblConstraint();

  if (lef58con != nullptr) {
    bloatDist = max(
        bloatDist, lef58con->getODBRule()->getMaxSpacing(cutClass1, cutClass2));
    if (cutClass1 == cutClass2)
      bloatDist = max(
          bloatDist, lef58con->getODBRule()->getExactAlignedSpacing(cutClass1));
  }

  FlexMazeIdx mIdx1;
  FlexMazeIdx mIdx2;
  // assumes width always > 2
  frBox bx(box.left() - bloatDist - (viaBox.right() - 0) + 1,
           box.bottom() - bloatDist - (viaBox.top() - 0) + 1,
           box.right() + bloatDist + (0 - viaBox.left()) - 1,
           box.top() + bloatDist + (0 - viaBox.bottom()) - 1);
  gridGraph_.getIdxBox(mIdx1, mIdx2, bx);

  frPoint pt;
  frBox tmpBx;
  frSquaredDistance distSquare = 0;
  frSquaredDistance c2cSquare = 0;
  frCoord dx, dy, prl;
  frTransform xform;
  // frCoord reqDist = 0;
  frSquaredDistance reqDistSquare = 0;
  frPoint boxCenter, tmpBxCenter;
  boxCenter.set((box.left() + box.right()) / 2, (box.bottom() + box.top()) / 2);
  frSquaredDistance currDistSquare = 0;
  bool hasViol;
  for (int i = mIdx1.x(); i <= mIdx2.x(); i++) {
    for (int j = mIdx1.y(); j <= mIdx2.y(); j++) {
      for (auto& uFig : via.getViaDef()->getCutFigs()) {
        auto obj = static_cast<frRect*>(uFig.get());
        gridGraph_.getPoint(pt, i, j);
        xform.set(pt);
        obj->getBBox(tmpBx);
        tmpBx.transform(xform);
        tmpBxCenter.set((tmpBx.left() + tmpBx.right()) / 2,
                        (tmpBx.bottom() + tmpBx.top()) / 2);
        distSquare = box2boxDistSquareNew(box, tmpBx, dx, dy);
        c2cSquare = pt2ptDistSquare(boxCenter, tmpBxCenter);
        prl = max(-dx, -dy);
        hasViol = false;
        for (auto con : cutLayer->getCutSpacing()) {
          reqDistSquare = con->getCutSpacing();
          reqDistSquare *= con->getCutSpacing();
          currDistSquare = con->hasCenterToCenter() ? c2cSquare : distSquare;
          if (con->hasSameNet()) {
            continue;
          }
          if (con->isLayer()) {
            ;
          } else if (con->isAdjacentCuts()) {
            // OBS always count as within distance instead of cut spacing
            if (isBlockage) {
              reqDistSquare = con->getCutWithin();
              reqDistSquare *= con->getCutWithin();
            }
            if (currDistSquare < reqDistSquare) {
              hasViol = true;
              // should disable hasViol and modify this part to new grid graph
            }
          } else if (con->isParallelOverlap()) {
            if (prl > 0 && currDistSquare < reqDistSquare) {
              hasViol = true;
            }
          } else if (con->isArea()) {
            auto currArea = max(box.length() * box.width(),
                                tmpBx.length() * tmpBx.width());
            if (currArea >= con->getCutArea()
                && currDistSquare < reqDistSquare) {
              hasViol = true;
            }
          } else if (currDistSquare < reqDistSquare) {
            hasViol = true;
          }
          if (hasViol)
            break;
        }
        if (!hasViol && lef58con != nullptr) {
          auto dbRule = lef58con->getODBRule();
          bool checkVertical
              = (tmpBx.bottom() > box.top()) || (tmpBx.top() < box.bottom());
          bool checkHorizontal
              = (tmpBx.left() > box.right()) || (tmpBx.right() < box.left());
          if (!checkHorizontal && !checkVertical)
            hasViol = true;
          else {
            if (checkVertical)
              hasViol = checkLef58CutSpacingViol(box,
                                                 tmpBx,
                                                 cutClass1,
                                                 cutClass2,
                                                 prl,
                                                 distSquare,
                                                 c2cSquare,
                                                 true,
                                                 dbRule);
            if (!hasViol && checkHorizontal)
              hasViol = checkLef58CutSpacingViol(box,
                                                 tmpBx,
                                                 cutClass1,
                                                 cutClass2,
                                                 prl,
                                                 distSquare,
                                                 c2cSquare,
                                                 false,
                                                 dbRule);
          }
        }

        if (hasViol) {
          switch (type) {
            case 0:
              gridGraph_.subRouteShapeCostVia(i, j, z);  // safe access
              break;
            case 1:
              gridGraph_.addRouteShapeCostVia(i, j, z);  // safe access
              break;
            case 2:
              gridGraph_.subFixedShapeCostVia(i, j, z);  // safe access
              break;
            case 3:
              gridGraph_.addFixedShapeCostVia(i, j, z);  // safe access
              break;
            default:;
          }
        }
      }
    }
  }
}

void FlexDRWorker::modInterLayerCutSpacingCost(const frBox& box,
                                               frMIdx z,
                                               int type,
                                               bool isUpperVia,
                                               bool isBlockage)
{
  auto cutLayerNum1 = gridGraph_.getLayerNum(z) + 1;
  auto cutLayerNum2 = isUpperVia ? cutLayerNum1 + 2 : cutLayerNum1 - 2;
  auto z2 = isUpperVia ? z + 1 : z - 1;

  frViaDef* viaDef = nullptr;
  if (isUpperVia) {
    viaDef = (cutLayerNum2 <= getTech()->getTopLayerNum())
                 ? getTech()->getLayer(cutLayerNum2)->getDefaultViaDef()
                 : nullptr;
  } else {
    viaDef = (cutLayerNum2 >= getTech()->getBottomLayerNum())
                 ? getTech()->getLayer(cutLayerNum2)->getDefaultViaDef()
                 : nullptr;
  }
  if (viaDef == nullptr) {
    return;
  }
  frCutSpacingConstraint* con
      = getTech()
            ->getLayer(cutLayerNum1)
            ->getInterLayerCutSpacing(cutLayerNum2, false);
  if (con == nullptr) {
    con = getTech()
              ->getLayer(cutLayerNum2)
              ->getInterLayerCutSpacing(cutLayerNum1, false);
  }
  if (con == nullptr) {
    return;
  }
  // obj1 = curr obj
  // obj2 = other obj
  // default via dimension
  frVia via(viaDef);
  frBox viaBox(0, 0, 0, 0);
  via.getCutBBox(viaBox);

  // spacing value needed
  frCoord bloatDist = con->getCutSpacing();

  FlexMazeIdx mIdx1;
  FlexMazeIdx mIdx2;
  // assumes width always > 2
  frBox bx(box.left() - bloatDist - (viaBox.right() - 0) + 1,
           box.bottom() - bloatDist - (viaBox.top() - 0) + 1,
           box.right() + bloatDist + (0 - viaBox.left()) - 1,
           box.top() + bloatDist + (0 - viaBox.bottom()) - 1);
  gridGraph_.getIdxBox(mIdx1, mIdx2, bx);

  frPoint pt;
  frBox tmpBx;
  frSquaredDistance distSquare = 0;
  frSquaredDistance c2cSquare = 0;
  frCoord dx, dy;
  frTransform xform;
  frSquaredDistance reqDistSquare = 0;
  frPoint boxCenter, tmpBxCenter;
  boxCenter.set((box.left() + box.right()) / 2, (box.bottom() + box.top()) / 2);
  frSquaredDistance currDistSquare = 0;
  bool hasViol = false;
  for (int i = mIdx1.x(); i <= mIdx2.x(); i++) {
    for (int j = mIdx1.y(); j <= mIdx2.y(); j++) {
      for (auto& uFig : via.getViaDef()->getCutFigs()) {
        auto obj = static_cast<frRect*>(uFig.get());
        gridGraph_.getPoint(pt, i, j);
        xform.set(pt);
        obj->getBBox(tmpBx);
        tmpBx.transform(xform);
        tmpBxCenter.set((tmpBx.left() + tmpBx.right()) / 2,
                        (tmpBx.bottom() + tmpBx.top()) / 2);
        distSquare = box2boxDistSquareNew(box, tmpBx, dx, dy);
        c2cSquare = pt2ptDistSquare(boxCenter, tmpBxCenter);
        hasViol = false;
        reqDistSquare = con->getCutSpacing();
        reqDistSquare *= con->getCutSpacing();
        currDistSquare = con->hasCenterToCenter() ? c2cSquare : distSquare;
        if (currDistSquare < reqDistSquare) {
          hasViol = true;
        }
        if (hasViol) {
          switch (type) {
            case 0:
              gridGraph_.subRouteShapeCostVia(i, j, z2);  // safe access
              break;
            case 1:
              gridGraph_.addRouteShapeCostVia(i, j, z2);  // safe access
              break;
            case 2:
              gridGraph_.subFixedShapeCostVia(i, j, z2);  // safe access
              break;
            case 3:
              gridGraph_.addFixedShapeCostVia(i, j, z2);  // safe access
              break;
            default:;
          }
          break;
        }
      }
    }
  }
}

void FlexDRWorker::modLef58InterLayerCutSpacingCost(const frBox& box,
                                                    frMIdx z,
                                                    int type,
                                                    bool isUpperVia,
                                                    bool isBlockage)
{
  auto cutLayerNum1 = gridGraph_.getLayerNum(z) + 1;
  auto cutLayerNum2 = isUpperVia ? cutLayerNum1 + 2 : cutLayerNum1 - 2;

  auto z2 = isUpperVia ? z + 1 : z - 1;

  frViaDef* viaDef = nullptr;
  if (isUpperVia) {
    viaDef = (cutLayerNum2 <= getTech()->getTopLayerNum())
                 ? getTech()->getLayer(cutLayerNum2)->getDefaultViaDef()
                 : nullptr;
  } else {
    viaDef = (cutLayerNum2 >= getTech()->getBottomLayerNum())
                 ? getTech()->getLayer(cutLayerNum2)->getDefaultViaDef()
                 : nullptr;
  }
  if (viaDef == nullptr)
    return;

  frLayer* higherLayer;
  frLayer* lowerLayer;
  if (isUpperVia) {
    higherLayer = getTech()->getLayer(cutLayerNum2);
    lowerLayer = getTech()->getLayer(cutLayerNum1);
  } else {
    lowerLayer = getTech()->getLayer(cutLayerNum2);
    higherLayer = getTech()->getLayer(cutLayerNum1);
  }
  frLef58CutSpacingTableConstraint* con;
  if (higherLayer->hasLef58DefaultInterCutSpcTblConstraint())
    con = higherLayer->getLef58DefaultInterCutSpcTblConstraint();
  else if (higherLayer->hasLef58SameNetInterCutSpcTblConstraint())
    con = higherLayer->getLef58SameNetInterCutSpcTblConstraint();
  else if (higherLayer->hasLef58SameMetalInterCutSpcTblConstraint())
    con = higherLayer->getLef58SameMetalInterCutSpcTblConstraint();
  else
    return;

  auto dbRule = con->getODBRule();
  if (dbRule->getSecondLayer()->getName() != lowerLayer->getName())
    return;
  // obj1 = curr obj
  // obj2 = other obj
  // default via dimension
  frVia via(viaDef);
  frBox viaBox(0, 0, 0, 0);
  via.getCutBBox(viaBox);

  // spacing value needed

  FlexMazeIdx mIdx1;
  FlexMazeIdx mIdx2;
  frString cutClass1 = "";
  frString cutClass2 = "";
  int cutClassIdx1, cutClassIdx2;
  frCoord bloatDist = 0;
  if (isUpperVia) {
    cutClassIdx1 = higherLayer->getCutClassIdx(viaBox.width(), viaBox.length());
    cutClassIdx2 = lowerLayer->getCutClassIdx(box.width(), box.length());
  } else {
    cutClassIdx1 = higherLayer->getCutClassIdx(box.width(), box.length());
    cutClassIdx2 = lowerLayer->getCutClassIdx(viaBox.width(), viaBox.length());
  }
  if (cutClassIdx1 != -1)
    cutClass1 = higherLayer->getCutClass(cutClassIdx1)->getName();
  if (cutClassIdx2 != -1)
    cutClass2 = lowerLayer->getCutClass(cutClassIdx2)->getName();

  bloatDist = dbRule->getMaxSpacing(cutClass1, cutClass2);
  if (bloatDist == 0)
    return;
  // assumes width always > 2
  frBox bx(box.left() - bloatDist - (viaBox.right() - 0) + 1,
           box.bottom() - bloatDist - (viaBox.top() - 0) + 1,
           box.right() + bloatDist + (0 - viaBox.left()) - 1,
           box.top() + bloatDist + (0 - viaBox.bottom()) - 1);
  gridGraph_.getIdxBox(mIdx1, mIdx2, bx);

  frPoint pt;
  frBox tmpBx;
  frSquaredDistance distSquare = 0;
  frSquaredDistance c2cSquare = 0;
  frCoord dx, dy, prl;
  frTransform xform;
  frPoint boxCenter, tmpBxCenter;
  boxCenter.set((box.left() + box.right()) / 2, (box.bottom() + box.top()) / 2);
  bool hasViol;
  for (int i = mIdx1.x(); i <= mIdx2.x(); i++) {
    for (int j = mIdx1.y(); j <= mIdx2.y(); j++) {
      for (auto& uFig : via.getViaDef()->getCutFigs()) {
        auto obj = static_cast<frRect*>(uFig.get());
        gridGraph_.getPoint(pt, i, j);
        xform.set(pt);
        obj->getBBox(tmpBx);
        tmpBx.transform(xform);
        tmpBxCenter.set((tmpBx.left() + tmpBx.right()) / 2,
                        (tmpBx.bottom() + tmpBx.top()) / 2);
        distSquare = box2boxDistSquareNew(box, tmpBx, dx, dy);
        c2cSquare = pt2ptDistSquare(boxCenter, tmpBxCenter);
        prl = max(-dx, -dy);
        hasViol = false;

        bool checkVertical
            = (tmpBx.bottom() > box.top()) || (tmpBx.top() < box.bottom());
        bool checkHorizontal
            = (tmpBx.left() > box.right()) || (tmpBx.right() < box.left());
        if (checkVertical)
          hasViol = checkLef58CutSpacingViol(box,
                                             tmpBx,
                                             cutClass1,
                                             cutClass2,
                                             prl,
                                             distSquare,
                                             c2cSquare,
                                             true,
                                             dbRule);
        if (!hasViol && checkHorizontal)
          hasViol = checkLef58CutSpacingViol(box,
                                             tmpBx,
                                             cutClass1,
                                             cutClass2,
                                             prl,
                                             distSquare,
                                             c2cSquare,
                                             false,
                                             dbRule);
        if (hasViol) {
          switch (type) {
            case 0:
              gridGraph_.subRouteShapeCostVia(i, j, z2);  // safe access
              break;
            case 1:
              gridGraph_.addRouteShapeCostVia(i, j, z2);  // safe access
              break;
            case 2:
              gridGraph_.subFixedShapeCostVia(i, j, z2);  // safe access
              break;
            case 3:
              gridGraph_.addFixedShapeCostVia(i, j, z2);  // safe access
              break;
            default:;
          }
        }
      }
    }
  }
}

void FlexDRWorker::addPathCost(drConnFig* connFig)
{
  modPathCost(connFig, 1);
}

void FlexDRWorker::subPathCost(drConnFig* connFig)
{
  modPathCost(connFig, 0);
}

void FlexDRWorker::modPathCost(drConnFig* connFig, int type)
{
  frNonDefaultRule* ndr = nullptr;
  if (connFig->typeId() == drcPathSeg) {
    auto obj = static_cast<drPathSeg*>(connFig);
    FlexMazeIdx bi, ei;
    obj->getMazeIdx(bi, ei);
    // new
    frBox box;
    obj->getBBox(box);
    ndr = !obj->isTapered() ? connFig->getNet()->getFrNet()->getNondefaultRule()
                            : nullptr;
    modMinSpacingCostPlanar(box, bi.z(), type, false, ndr);
    modMinSpacingCostVia(box, bi.z(), type, true, true, false, ndr);
    modMinSpacingCostVia(box, bi.z(), type, false, true, false, ndr);
    modViaForbiddenThrough(bi, ei, type);
    // wrong way wire cannot have eol problem: (1) with via at end, then via
    // will add eol cost; (2) with pref-dir wire, then not eol edge
    bool isHLayer
        = (getTech()->getLayer(gridGraph_.getLayerNum(bi.z()))->getDir()
           == frPrefRoutingDirEnum::frcHorzPrefRoutingDir);
    if (isHLayer == (bi.y() == ei.y())) {
      modEolSpacingRulesCost(box, bi.z(), type);
    }
  } else if (connFig->typeId() == drcPatchWire) {
    auto obj = static_cast<drPatchWire*>(connFig);
    frMIdx zIdx = gridGraph_.getMazeZIdx(obj->getLayerNum());
    frBox box;
    obj->getBBox(box);
    ndr = connFig->getNet()->getFrNet()->getNondefaultRule();
    modMinSpacingCostPlanar(box, zIdx, type, false, ndr);
    modMinSpacingCostVia(box, zIdx, type, true, true, false, ndr);
    modMinSpacingCostVia(box, zIdx, type, false, true, false, ndr);
    modEolSpacingRulesCost(box, zIdx, type);
  } else if (connFig->typeId() == drcVia) {
    auto obj = static_cast<drVia*>(connFig);
    FlexMazeIdx bi, ei;
    obj->getMazeIdx(bi, ei);
    // new

    frBox box;
    obj->getLayer1BBox(box);  // assumes enclosure for via is always rectangle
    ndr = connFig->getNet()->getFrNet()->getNondefaultRule();
    modMinSpacingCostPlanar(box, bi.z(), type, false, ndr);
    modMinSpacingCostVia(box, bi.z(), type, true, false, false, ndr);
    modMinSpacingCostVia(box, bi.z(), type, false, false, false, ndr);
    modEolSpacingRulesCost(box, bi.z(), type);

    obj->getLayer2BBox(box);  // assumes enclosure for via is always rectangle

    modMinSpacingCostPlanar(box, ei.z(), type, false, ndr);
    modMinSpacingCostVia(box, ei.z(), type, true, false, false, ndr);
    modMinSpacingCostVia(box, ei.z(), type, false, false, false, ndr);
    modEolSpacingRulesCost(box, ei.z(), type);

    frTransform xform;
    frPoint pt;
    obj->getOrigin(pt);
    xform.set(pt);
    for (auto& uFig : obj->getViaDef()->getCutFigs()) {
      auto rect = static_cast<frRect*>(uFig.get());
      rect->getBBox(box);
      box.transform(xform);
      modCutSpacingCost(box, bi.z(), type);
      modInterLayerCutSpacingCost(box, bi.z(), type, true);
      modInterLayerCutSpacingCost(box, bi.z(), type, false);
      modLef58InterLayerCutSpacingCost(box, bi.z(), type, true);
      modLef58InterLayerCutSpacingCost(box, bi.z(), type, false);
    }
  }
}

bool FlexDRWorker::mazeIterInit_sortRerouteNets(int mazeIter,
                                                vector<drNet*>& rerouteNets)
{
  auto rerouteNetsComp = [](drNet* const& a, drNet* const& b) {
    if (a->getFrNet()->getNondefaultRule()
        && !b->getFrNet()->getNondefaultRule())
      return true;
    if (!a->getFrNet()->getNondefaultRule()
        && b->getFrNet()->getNondefaultRule())
      return false;
    frBox boxA, boxB;
    a->getPinBox(boxA);
    b->getPinBox(boxB);
    auto areaA = boxA.area();
    auto areaB = boxB.area();
    return (a->getNumPinsIn() == b->getNumPinsIn()
                ? (areaA == areaB ? a->getId() < b->getId() : areaA < areaB)
                : a->getNumPinsIn() < b->getNumPinsIn());
  };
  // sort
  if (mazeIter == 0) {
    sort(rerouteNets.begin(), rerouteNets.end(), rerouteNetsComp);
    // to be removed
    if (OR_SEED != -1 && rerouteNets.size() >= 2) {
      uniform_int_distribution<int> distribution(0, rerouteNets.size() - 1);
      default_random_engine generator(OR_SEED);
      int numSwap = (double) (rerouteNets.size()) * OR_K;
      for (int i = 0; i < numSwap; i++) {
        int idx = distribution(generator);
        swap(rerouteNets[idx], rerouteNets[(idx + 1) % rerouteNets.size()]);
      }
    }
  }
  return true;
}

void FlexDRWorker::mazeNetInit(drNet* net)
{
  gridGraph_.resetStatus();
  gridGraph_.setNDR(net->getFrNet()->getNondefaultRule());
  // sub term / instterm cost when net is about to route
  initMazeCost_terms(net->getFrNetTerms(), false, true);
  // sub via access cost when net is about to route
  // route_queue does not need to reserve
  if (isFollowGuide()) {
    initMazeCost_guide_helper(net, true);
  }
  // add minimum cut cost from objs in ext ring when the net is about to route
  initMazeCost_minCut_helper(net, true);
  initMazeCost_ap_helper(net, false);
  initMazeCost_boundary_helper(net, false);
}

void FlexDRWorker::mazeNetEnd(drNet* net)
{
  // add term / instterm cost back when net is about to end
  initMazeCost_terms(net->getFrNetTerms(), true, true);
  if (isFollowGuide()) {
    initMazeCost_guide_helper(net, false);
  }
  // sub minimum cut cost from vias in ext ring when the net is about to end
  initMazeCost_minCut_helper(net, false);
  initMazeCost_ap_helper(net, true);
  initMazeCost_boundary_helper(net, true);
  gridGraph_.setNDR(nullptr);
  gridGraph_.setDstTaperBox(nullptr);
}

void FlexDRWorker::route_queue(FlexGCWorker& gcWorker)
{
  queue<RouteQueueEntry> rerouteQueue;

  if (needRecheck_) {
    gcWorker.main();
    setMarkers(gcWorker.getMarkers());
  }

  setGCWorker(&gcWorker);

  // init net status
  route_queue_resetRipup();
  // init marker cost
  route_queue_addMarkerCost();
  // init reroute queue
  // route_queue_init_queue(rerouteNets);
  route_queue_init_queue(rerouteQueue);

  if (graphics_ && !rerouteQueue.empty()) {
    graphics_->startWorker(this);
  }

  // route
  route_queue_main(rerouteQueue);

  // end
  gcWorker.resetTargetNet();
  gcWorker.setEnableSurgicalFix(true);
  gcWorker.main();
  // write back GC patches
  for (auto& pwire : gcWorker.getPWires()) {
    auto net = pwire->getNet();
    if (!net) {
      cout << "Error: pwire with no net\n";
      exit(1);
    }
    auto tmpPWire = make_unique<drPatchWire>();
    tmpPWire->setLayerNum(pwire->getLayerNum());
    frPoint origin;
    pwire->getOrigin(origin);
    tmpPWire->setOrigin(origin);
    frBox box;
    pwire->getOffsetBox(box);
    tmpPWire->setOffsetBox(box);
    tmpPWire->addToNet(net);

    unique_ptr<drConnFig> tmp(std::move(tmpPWire));
    auto& workerRegionQuery = getWorkerRegionQuery();
    workerRegionQuery.add(tmp.get());
    net->addRoute(std::move(tmp));
  }

  gcWorker.end();

  setMarkers(gcWorker.getMarkers());

  for (auto& net : nets_) {
    net->setBestRouteConnFigs();
  }
  setBestMarkers();
  if (graphics_) {
    graphics_->show(true);
  }
}

void FlexDRWorker::route_queue_main(queue<RouteQueueEntry>& rerouteQueue)
{
  auto& workerRegionQuery = getWorkerRegionQuery();
  while (!rerouteQueue.empty()) {
    // cout << "rerouteQueue size = " << rerouteQueue.size() << endl;
    auto& entry = rerouteQueue.front();
    frBlockObject* obj = entry.block;
    bool doRoute = entry.doRoute;
    int numReroute = entry.numReroute;

    rerouteQueue.pop();
    bool didRoute = false;
    bool didCheck = false;

    if (obj->typeId() == drcNet && doRoute) {
      auto net = static_cast<drNet*>(obj);
      if (numReroute != net->getNumReroutes()) {
        continue;
      }
      // init
      net->setModified(true);
      if (net->getFrNet()) {
        net->getFrNet()->setModified(true);
      }
      net->setNumMarkers(0);
      for (auto& uConnFig : net->getRouteConnFigs()) {
        subPathCost(uConnFig.get());
        workerRegionQuery.remove(uConnFig.get());  // worker region query
      }
      // route_queue need to unreserve via access if all nets are ripupped
      // (i.e., not routed) see route_queue_init_queue this
      // is unreserve via via is reserved only when drWorker starts from nothing
      // and via is reserved
      if (net->getNumReroutes() == 0 && getRipupMode() == 1) {
        initMazeCost_via_helper(net, false);
      }
      net->clear();

      // route
      mazeNetInit(net);
      bool isRouted = routeNet(net);
      if (isRouted == false) {
        if (OUT_MAZE_FILE == string("")) {
          if (VERBOSE > 0) {
            cout << "Waring: no output maze log specified, skipped writing "
                    "maze log"
                 << endl;
          }
        } else {
          gridGraph_.print();
        }
        if (graphics_) {
            graphics_->show(false);
        }
        logger_->error(DRT,
                       255,
                       "Maze Route cannot find path of net {} in "
                       "worker of routeBox {}.",
                       net->getFrNet()->getName(),
                       getRouteBox());
      }
      mazeNetEnd(net);
      net->addNumReroutes();
      didRoute = true;

      // gc
      if (gcWorker_->setTargetNet(net->getFrNet())) {
        gcWorker_->updateDRNet(net);
        gcWorker_->setEnableSurgicalFix(true);
        gcWorker_->main();

        // write back GC patches
        for (auto& pwire : gcWorker_->getPWires()) {
          auto net = pwire->getNet();
          auto tmpPWire = make_unique<drPatchWire>();
          tmpPWire->setLayerNum(pwire->getLayerNum());
          frPoint origin;
          pwire->getOrigin(origin);
          tmpPWire->setOrigin(origin);
          frBox box;
          pwire->getOffsetBox(box);
          tmpPWire->setOffsetBox(box);
          tmpPWire->addToNet(net);

          unique_ptr<drConnFig> tmp(std::move(tmpPWire));
          auto& workerRegionQuery = getWorkerRegionQuery();
          workerRegionQuery.add(tmp.get());
          net->addRoute(std::move(tmp));
        }

        didCheck = true;
      } else {
        cout << "Error: fail to setTargetNet\n";
      }
    } else {
      // if (isRouteSkipped == false) {
      gcWorker_->setEnableSurgicalFix(false);
      if (obj->typeId() == frcNet) {
        auto net = static_cast<frNet*>(obj);
        if (gcWorker_->setTargetNet(net)) {
          gcWorker_->main();
          didCheck = true;
          // cout << "do check " << net->getName() << "\n";
        }
      } else {
        if (gcWorker_->setTargetNet(obj)) {
          gcWorker_->main();
          didCheck = true;
          // cout << "do check\n";
        }
      }
      // }
    }

    // end
    if (didCheck) {
      route_queue_update_queue(gcWorker_->getMarkers(), rerouteQueue);
    }
    if (didRoute) {
      route_queue_markerCostDecay();
    }
    if (didCheck) {
      route_queue_addMarkerCost(gcWorker_->getMarkers());
    }

    if (graphics_) {
      if (obj->typeId() == drcNet && doRoute) {
        auto net = static_cast<drNet*>(obj);
        graphics_->endNet(net);
      }
    }
  }
}

void FlexDRWorker::routeNet_prep(drNet* net, set<drPin*, frBlockObjectComp> &unConnPins, 
                                 map<FlexMazeIdx, set<drPin*, frBlockObjectComp> > &mazeIdx2unConnPins,
                                 set<FlexMazeIdx> &apMazeIdx,
                                 set<FlexMazeIdx> &realPinAPMazeIdx,
                                 map<FlexMazeIdx, frBox3D*>& mazeIdx2Tbox,
                                 list<pair<drPin*, frBox3D>>& pinTaperBoxes/*,
                                 map<FlexMazeIdx, frViaDef*> &apSVia*/)
{
  frBox3D* tbx = nullptr;
  for (auto& pin : net->getPins()) {
    unConnPins.insert(pin.get());
    if (gridGraph_.getNDR()) {
      if (AUTO_TAPER_NDR_NETS
          && pin->isInstPin()) {  // create a taper box for each pin
        FlexMazeIdx l, h;
        pin->getAPBbox(l, h);
        frMIdx z;
        frCoord pitch
            = getTech()->getLayer(gridGraph_.getLayerNum(l.z()))->getPitch(),
            r;
        r = TAPERBOX_RADIUS;
        l.set(gridGraph_.getMazeXIdx(gridGraph_.xCoord(l.x()) - r * pitch),
              gridGraph_.getMazeYIdx(gridGraph_.yCoord(l.y()) - r * pitch),
              l.z());
        h.set(gridGraph_.getMazeXIdx(gridGraph_.xCoord(h.x()) + r * pitch),
              gridGraph_.getMazeYIdx(gridGraph_.yCoord(h.y()) + r * pitch),
              h.z());
        z = l.z() == 0 ? 1 : h.z();
        pinTaperBoxes.push_back(std::make_pair(
            pin.get(), frBox3D(l.x(), l.y(), h.x(), h.y(), l.z(), z)));
        tbx = &std::prev(pinTaperBoxes.end())->second;
        for (z = tbx->zLow(); z <= tbx->zHigh();
             z++) {  // populate the map from points to taper boxes
          for (int x = tbx->left(); x <= tbx->right(); x++)
            for (int y = tbx->bottom(); y <= tbx->top(); y++)
              mazeIdx2Tbox[FlexMazeIdx(x, y, z)] = tbx;
        }
      }
    }
    for (auto& ap : pin->getAccessPatterns()) {
      FlexMazeIdx mi;
      ap->getMazeIdx(mi);
      mazeIdx2unConnPins[mi].insert(pin.get());
      if (pin->hasFrTerm()) {
        realPinAPMazeIdx.insert(mi);
      }
      apMazeIdx.insert(mi);
      gridGraph_.setDst(mi);
    }
  }
}

void FlexDRWorker::routeNet_setSrc(
    set<drPin*, frBlockObjectComp>& unConnPins,
    map<FlexMazeIdx, set<drPin*, frBlockObjectComp>>& mazeIdx2unConnPins,
    vector<FlexMazeIdx>& connComps,
    FlexMazeIdx& ccMazeIdx1,
    FlexMazeIdx& ccMazeIdx2,
    frPoint& centerPt)
{
  frMIdx xDim, yDim, zDim;
  gridGraph_.getDim(xDim, yDim, zDim);
  ccMazeIdx1.set(xDim - 1, yDim - 1, zDim - 1);
  ccMazeIdx2.set(0, 0, 0);
  // first pin selection algorithm goes here
  // choose the center pin
  centerPt.set(0, 0);
  int totAPCnt = 0;
  frCoord totX = 0;
  frCoord totY = 0;
  frCoord totZ = 0;
  FlexMazeIdx mi;
  frPoint bp;
  for (auto& pin : unConnPins) {
    for (auto& ap : pin->getAccessPatterns()) {
      ap->getMazeIdx(mi);
      ap->getPoint(bp);
      totX += bp.x();
      totY += bp.y();
      centerPt.set(centerPt.x() + bp.x(), centerPt.y() + bp.y());
      totZ += gridGraph_.getZHeight(mi.z());
      totAPCnt++;
      break;
    }
  }
  totX /= totAPCnt;
  totY /= totAPCnt;
  totZ /= totAPCnt;
  centerPt.set(centerPt.x() / totAPCnt, centerPt.y() / totAPCnt);

  // select the farmost pin
  drPin* currPin = nullptr;

  frCoord currDist = 0;
  for (auto& pin : unConnPins) {
    for (auto& ap : pin->getAccessPatterns()) {
      ap->getMazeIdx(mi);
      ap->getPoint(bp);
      frCoord dist = abs(totX - bp.x()) + abs(totY - bp.y())
                     + abs(totZ - gridGraph_.getZHeight(mi.z()));
      if (dist >= currDist) {
        currDist = dist;
        currPin = pin;
      }
    }
  }

  unConnPins.erase(currPin);

  // first pin selection algorithm ends here
  for (auto& ap : currPin->getAccessPatterns()) {
    ap->getMazeIdx(mi);
    connComps.push_back(mi);
    ccMazeIdx1.set(min(ccMazeIdx1.x(), mi.x()),
                   min(ccMazeIdx1.y(), mi.y()),
                   min(ccMazeIdx1.z(), mi.z()));
    ccMazeIdx2.set(max(ccMazeIdx2.x(), mi.x()),
                   max(ccMazeIdx2.y(), mi.y()),
                   max(ccMazeIdx2.z(), mi.z()));
    auto it = mazeIdx2unConnPins.find(mi);
    if (it == mazeIdx2unConnPins.end()) {
      continue;
    }
    auto it2 = it->second.find(currPin);
    if (it2 == it->second.end()) {
      continue;
    }
    it->second.erase(it2);

    gridGraph_.setSrc(mi);
    // remove dst label only when no other pins share the same loc
    if (it->second.empty()) {
      mazeIdx2unConnPins.erase(it);
      gridGraph_.resetDst(mi);
    }
  }
}

drPin* FlexDRWorker::routeNet_getNextDst(
    FlexMazeIdx& ccMazeIdx1,
    FlexMazeIdx& ccMazeIdx2,
    map<FlexMazeIdx, set<drPin*, frBlockObjectComp>>& mazeIdx2unConnPins,
    list<pair<drPin*, frBox3D>>& pinTaperBoxes)
{
  frPoint pt;
  frPoint ll, ur;
  gridGraph_.getPoint(ll, ccMazeIdx1.x(), ccMazeIdx1.y());
  gridGraph_.getPoint(ur, ccMazeIdx2.x(), ccMazeIdx2.y());
  frCoord currDist = std::numeric_limits<frCoord>::max();
  drPin* nextDst = nullptr;
  if (!nextDst)
    for (auto& [mazeIdx, setS] : mazeIdx2unConnPins) {
      gridGraph_.getPoint(pt, mazeIdx.x(), mazeIdx.y());
      frCoord dx = max(max(ll.x() - pt.x(), pt.x() - ur.x()), 0);
      frCoord dy = max(max(ll.y() - pt.y(), pt.y() - ur.y()), 0);
      frCoord dz = max(max(gridGraph_.getZHeight(ccMazeIdx1.z())
                               - gridGraph_.getZHeight(mazeIdx.z()),
                           gridGraph_.getZHeight(mazeIdx.z())
                               - gridGraph_.getZHeight(ccMazeIdx2.z())),
                       0);
      if (dx + dy + dz < currDist) {
        currDist = dx + dy + dz;
        nextDst = *(setS.begin());
      }
      if (currDist == 0) {
        break;
      }
    }
  if (gridGraph_.getNDR()) {
    if (AUTO_TAPER_NDR_NETS) {
      for (auto& a : pinTaperBoxes) {
        if (a.first == nextDst) {
          gridGraph_.setDstTaperBox(&a.second);
          break;
        }
      }
    }
  }
  return nextDst;
}

void FlexDRWorker::mazePinInit()
{
  gridGraph_.resetPrevNodeDir();
}

void FlexDRWorker::routeNet_postAstarUpdate(
    vector<FlexMazeIdx>& path,
    vector<FlexMazeIdx>& connComps,
    set<drPin*, frBlockObjectComp>& unConnPins,
    map<FlexMazeIdx, set<drPin*, frBlockObjectComp>>& mazeIdx2unConnPins,
    bool isFirstConn)
{
  // first point is dst
  set<FlexMazeIdx> localConnComps;
  if (!path.empty()) {
    auto mi = path[0];
    vector<drPin*> tmpPins;
    for (auto pin : mazeIdx2unConnPins[mi]) {
      tmpPins.push_back(pin);
    }
    for (auto pin : tmpPins) {
      unConnPins.erase(pin);
      for (auto& ap : pin->getAccessPatterns()) {
        FlexMazeIdx mi;
        ap->getMazeIdx(mi);
        auto it = mazeIdx2unConnPins.find(mi);
        if (it == mazeIdx2unConnPins.end()) {
          continue;
        }
        auto it2 = it->second.find(pin);
        if (it2 == it->second.end()) {
          continue;
        }
        it->second.erase(it2);
        if (it->second.empty()) {
          mazeIdx2unConnPins.erase(it);
          gridGraph_.resetDst(mi);
        }
        if (ALLOW_PIN_AS_FEEDTHROUGH) {
          localConnComps.insert(mi);
          gridGraph_.setSrc(mi);
        }
      }
    }
  } else {
    cout << "Error: routeNet_postAstarUpdate path is empty" << endl;
  }
  // must be before comment line ABC so that the used actual src is set in
  // gridgraph
  if (isFirstConn && (!ALLOW_PIN_AS_FEEDTHROUGH)) {
    for (auto& mi : connComps) {
      gridGraph_.resetSrc(mi);
    }
    connComps.clear();
    if ((int) path.size() == 1) {
      connComps.push_back(path[0]);
      gridGraph_.setSrc(path[0]);
    }
  }
  // line ABC
  // must have >0 length
  for (int i = 0; i < (int) path.size() - 1; ++i) {
    auto start = path[i];
    auto end = path[i + 1];
    auto startX = start.x(), startY = start.y(), startZ = start.z();
    auto endX = end.x(), endY = end.y(), endZ = end.z();
    // horizontal wire
    if (startX != endX && startY == endY && startZ == endZ) {
      for (auto currX = std::min(startX, endX); currX <= std::max(startX, endX);
           ++currX) {
        localConnComps.insert(FlexMazeIdx(currX, startY, startZ));
        gridGraph_.setSrc(currX, startY, startZ);
      }
      // vertical wire
    } else if (startX == endX && startY != endY && startZ == endZ) {
      for (auto currY = std::min(startY, endY); currY <= std::max(startY, endY);
           ++currY) {
        localConnComps.insert(FlexMazeIdx(startX, currY, startZ));
        gridGraph_.setSrc(startX, currY, startZ);
      }
      // via
    } else if (startX == endX && startY == endY && startZ != endZ) {
      for (auto currZ = std::min(startZ, endZ); currZ <= std::max(startZ, endZ);
           ++currZ) {
        localConnComps.insert(FlexMazeIdx(startX, startY, currZ));
        gridGraph_.setSrc(startX, startY, currZ);
      }
      // zero length
    } else if (startX == endX && startY == endY && startZ == endZ) {
      std::cout << "Warning: zero-length path in updateFlexPin\n";
    } else {
      std::cout << "Error: non-colinear path in updateFlexPin\n";
    }
  }
  for (auto& mi : localConnComps) {
    if (isFirstConn && !ALLOW_PIN_AS_FEEDTHROUGH) {
      connComps.push_back(mi);
    } else {
      if (!(mi == *(path.cbegin()))) {
        connComps.push_back(mi);
      }
    }
  }
}

void FlexDRWorker::routeNet_postAstarWritePath(
    drNet* net,
    vector<FlexMazeIdx>& points,
    const set<FlexMazeIdx>& realPinApMazeIdx,
    map<FlexMazeIdx, frBox3D*>& mazeIdx2TaperBox,
    const set<FlexMazeIdx>& apMazeIdx)
{
  if (points.empty()) {
    return;
  }
  auto& workerRegionQuery = getWorkerRegionQuery();
  frBox3D *srcBox = nullptr, *dstBox = nullptr;
  auto it = mazeIdx2TaperBox.find(points[0]);
  if (it != mazeIdx2TaperBox.end())
    dstBox = it->second;
  it = mazeIdx2TaperBox.find(points.back());
  if (it != mazeIdx2TaperBox.end())
    srcBox = it->second;
  for (int i = 0; i < (int) points.size() - 1; ++i) {
    FlexMazeIdx start, end;
    if (points[i + 1] < points[i]) {
      start = points[i + 1];
      end = points[i];
    } else {
      start = points[i];
      end = points[i + 1];
    }
    auto startX = start.x(), startY = start.y(), startZ = start.z();
    auto endX = end.x(), endY = end.y(), endZ = end.z();
    if (startZ == endZ
        && ((startX != endX && startY == endY)
            || (startX == endX && startY != endY))) {
      frMIdx midX, midY;
      bool taper = false;
      if (splitPathSeg(midX,
                       midY,
                       taper,
                       startX,
                       startY,
                       endX,
                       endY,
                       startZ,
                       srcBox,
                       dstBox,
                       net)) {
        processPathSeg(startX,
                       startY,
                       midX,
                       midY,
                       startZ,
                       realPinApMazeIdx,
                       net,
                       startX == endX,
                       taper,
                       i,
                       points,
                       apMazeIdx);
        startX = midX;
        startY = midY;
        if (splitPathSeg(midX,
                         midY,
                         taper,
                         startX,
                         startY,
                         endX,
                         endY,
                         startZ,
                         srcBox,
                         dstBox,
                         net)) {
          processPathSeg(startX,
                         startY,
                         midX,
                         midY,
                         startZ,
                         realPinApMazeIdx,
                         net,
                         startX == endX,
                         taper,
                         i,
                         points,
                         apMazeIdx);
          startX = midX;
          startY = midY;
          taper = true;
        }
      }
      processPathSeg(startX,
                     startY,
                     endX,
                     endY,
                     startZ,
                     realPinApMazeIdx,
                     net,
                     startX == endX,
                     taper,
                     i,
                     points,
                     apMazeIdx);
    } else if (startX == endX && startY == endY && startZ != endZ) {  // via
      for (auto currZ = startZ; currZ < endZ; ++currZ) {
        frPoint loc;
        frLayerNum startLayerNum = gridGraph_.getLayerNum(currZ);
        // frLayerNum endLayerNum = gridGraph_.getLayerNum(currZ + 1);
        gridGraph_.getPoint(loc, startX, startY);
        FlexMazeIdx mi(startX, startY, currZ);
        auto via = getTech()->getLayer(startLayerNum + 1)->getDefaultViaDef();
        if (net->getFrNet()->getNondefaultRule()
            && net->getFrNet()->getNondefaultRule()->getPrefVia(currZ))
          via = net->getFrNet()->getNondefaultRule()->getPrefVia(currZ);
        if (gridGraph_.isSVia(startX, startY, currZ)) {
          via = apSVia_.find(mi)->second->getAccessViaDef();
        }
        auto currVia = make_unique<drVia>(via);
        if (net->hasNDR() && AUTO_TAPER_NDR_NETS) {
          if (isInsideTaperBox(endX, endY, startZ, endZ, mazeIdx2TaperBox)) {
            currVia->setTapered(true);
          }
        }
        currVia->setOrigin(loc);
        FlexMazeIdx mzIdxBot(startX, startY, currZ);
        FlexMazeIdx mzIdxTop(startX, startY, currZ + 1);
        currVia->setMazeIdx(mzIdxBot, mzIdxTop);
        /*update access point (AP) connectivity info. If it is over a boundary
        pin may still be over an unseen AP (this is checked by
        checkViaConnectivity) */
        if (realPinApMazeIdx.find(mzIdxBot) != realPinApMazeIdx.end()) {
          currVia->setBottomConnected(true);
        } else if (apMazeIdx.find(mzIdxBot) != apMazeIdx.end()) {
          checkViaConnectivityToAP(currVia.get(), true, net->getFrNet());
        }
        if (realPinApMazeIdx.find(mzIdxTop) != realPinApMazeIdx.end()) {
          currVia->setTopConnected(true);
        } else if (apMazeIdx.find(mzIdxTop) != apMazeIdx.end()) {
          checkViaConnectivityToAP(currVia.get(), false, net->getFrNet());
        }
        unique_ptr<drConnFig> tmp(std::move(currVia));
        workerRegionQuery.add(tmp.get());
        net->addRoute(std::move(tmp));
        if (gridGraph_.hasRouteShapeCost(startX, startY, currZ, frDirEnum::U)) {
          net->addMarker();
        }
      }
      // zero length
    } else if (startX == endX && startY == endY && startZ == endZ) {
      std::cout << "Warning: zero-length path in updateFlexPin\n";
    } else {
      std::cout << "Error: non-colinear path in updateFlexPin\n";
    }
  }
}
bool FlexDRWorker::splitPathSeg(frMIdx& midX,
                                frMIdx& midY,
                                bool& taperFirstPiece,
                                frMIdx startX,
                                frMIdx startY,
                                frMIdx endX,
                                frMIdx endY,
                                frMIdx z,
                                frBox3D* srcBox,
                                frBox3D* dstBox,
                                drNet* net)
{
  taperFirstPiece = false;
  if (!net->hasNDR() || !AUTO_TAPER_NDR_NETS) {
    return false;
  }
  frBox3D* bx = nullptr;
  if (srcBox && srcBox->contains(startX, startY, z)) {
    bx = srcBox;
  } else if (dstBox && dstBox->contains(startX, startY, z)) {
    bx = dstBox;
  }
  if (bx) {
    taperFirstPiece = true;
    if (bx->contains(endX, endY, z, 1, 1)) {
      return false;
    } else {
      if (startX == endX) {
        midX = startX;
        midY = bx->top() + 1;
      } else {
        midX = bx->right() + 1;
        midY = startY;
      }
      return true;
    }
  } else {
    if (srcBox && srcBox->contains(endX, endY, z)) {
      bx = srcBox;
    } else if (dstBox && dstBox->contains(endX, endY, z)) {
      bx = dstBox;
    }
    if (bx) {
      if (bx->contains(startX, startY, z, 1, 1)) {
        taperFirstPiece = true;
        return false;
      } else {
        if (startX == endX) {
          midX = startX;
          midY = bx->bottom() - 1;
        } else {
          midX = bx->left() - 1;
          midY = startY;
        }
        return true;
      }
    }
  }
  return false;
}
void FlexDRWorker::processPathSeg(frMIdx startX,
                                  frMIdx startY,
                                  frMIdx endX,
                                  frMIdx endY,
                                  frMIdx z,
                                  const set<FlexMazeIdx>& realApMazeIdx,
                                  drNet* net,
                                  bool vertical,
                                  bool taper,
                                  int i,
                                  vector<FlexMazeIdx>& points,
                                  const set<FlexMazeIdx>& apMazeIdx)
{
  frPoint startLoc, endLoc;
  frLayerNum currLayerNum = gridGraph_.getLayerNum(z);
  gridGraph_.getPoint(startLoc, startX, startY);
  gridGraph_.getPoint(endLoc, endX, endY);
  auto currPathSeg = make_unique<drPathSeg>();
  currPathSeg->setPoints(startLoc, endLoc);
  currPathSeg->setLayerNum(currLayerNum);
  currPathSeg->addToNet(net);
  FlexMazeIdx start(startX, startY, z), end(endX, endY, z);
  auto currStyle = getTech()->getLayer(currLayerNum)->getDefaultSegStyle();
  if (realApMazeIdx.find(start) != realApMazeIdx.end()) {
    currStyle.setBeginStyle(frcTruncateEndStyle, 0);
  } else if (apMazeIdx.find(start) != apMazeIdx.end()) {
    checkPathSegStyle(currPathSeg.get(), true, currStyle);
  }
  if (realApMazeIdx.find(end) != realApMazeIdx.end()) {
    currStyle.setEndStyle(frcTruncateEndStyle, 0);
  } else if (apMazeIdx.find(end) != apMazeIdx.end()) {
    checkPathSegStyle(currPathSeg.get(), false, currStyle);
  }
  if (net->getFrNet()->getNondefaultRule()) {
    if (taper)
      currPathSeg->setTapered(true);
    else
      setNDRStyle(net,
                  currStyle,
                  startX,
                  endX,
                  startY,
                  endY,
                  z,
                  i - 1 >= 0 ? &points[i - 1] : nullptr,
                  i + 2 < (int) points.size() ? &points[i + 2] : nullptr);
  }
  currPathSeg->setStyle(currStyle);
  currPathSeg->setMazeIdx(start, end);
  unique_ptr<drConnFig> tmp(std::move(currPathSeg));
  getWorkerRegionQuery().add(tmp.get());
  net->addRoute(std::move(tmp));

  // quick drc cnt
  bool prevHasCost = false;
  int endI = vertical ? endY : endX;
  for (int i = (vertical ? startY : startX); i < endI; i++) {
    if ((vertical && gridGraph_.hasRouteShapeCost(startX, i, z, frDirEnum::E))
        || (!vertical
            && gridGraph_.hasRouteShapeCost(i, startY, z, frDirEnum::N))) {
      if (!prevHasCost) {
        net->addMarker();
        prevHasCost = true;
      }
    } else {
      prevHasCost = false;
    }
  }
}
// checks whether the path segment is connected to an access point and update
// connectivity info (stored in frSegStyle)
void FlexDRWorker::checkPathSegStyle(drPathSeg* ps,
                                     bool isBegin,
                                     frSegStyle& style)
{
  const frPoint& pt = (isBegin ? ps->getBeginPoint() : ps->getEndPoint());
  if (hasAccessPoint(pt, ps->getLayerNum(), ps->getNet()->getFrNet())) {
    if (isBegin)
      style.setBeginStyle(frEndStyle(frEndStyleEnum::frcTruncateEndStyle), 0);
    else
      style.setEndStyle(frEndStyle(frEndStyleEnum::frcTruncateEndStyle), 0);
  }
}

bool FlexDRWorker::hasAccessPoint(const frPoint& pt,
                                  frLayerNum lNum,
                                  frNet* net)
{
  frRegionQuery::Objects<frBlockObject> result;
  frBox bx(pt.x(), pt.y(), pt.x(), pt.y());
  design_->getRegionQuery()->query(bx, lNum, result);
  for (auto& rqObj : result) {
    if (rqObj.second->typeId() == frcInstTerm) {
      auto instTerm = static_cast<frInstTerm*>(rqObj.second);
      if (instTerm->getNet() == net
          && instTerm->hasAccessPoint(pt.x(), pt.y(), lNum))
        return true;
    } else if (rqObj.second->typeId() == frcTerm) {
      auto term = static_cast<frTerm*>(rqObj.second);
      if (term->getNet() == net
          && term->hasAccessPoint(pt.x(), pt.y(), lNum, 0))
        return true;
    }
  }
  return false;
}
// checks whether the via is connected to an access point and update
// connectivity info
void FlexDRWorker::checkViaConnectivityToAP(drVia* via,
                                            bool isBottom,
                                            frNet* net)
{
  if (isBottom) {
    if (hasAccessPoint(via->getOrigin(), via->getViaDef()->getLayer1Num(), net))
      via->setBottomConnected(true);
  } else {
    if (hasAccessPoint(via->getOrigin(), via->getViaDef()->getLayer2Num(), net))
      via->setTopConnected(true);
  }
}
void FlexDRWorker::setNDRStyle(drNet* net,
                               frSegStyle& currStyle,
                               frMIdx startX,
                               frMIdx endX,
                               frMIdx startY,
                               frMIdx endY,
                               frMIdx z,
                               FlexMazeIdx* prev,
                               FlexMazeIdx* next)
{
  frNonDefaultRule* ndr = net->getFrNet()->getNondefaultRule();
  if (ndr->getWidth(z) > (int) currStyle.getWidth()) {
    currStyle.setWidth(ndr->getWidth(z));
    currStyle.setBeginExt(ndr->getWidth(z) / 2);
    currStyle.setEndExt(ndr->getWidth(z) / 2);
  }
  if (ndr->getWireExtension(z) > 0) {
    bool hasBeginExt = false, hasEndExt = false;
    if (!prev && !next) {
      hasBeginExt = hasEndExt = true;
    } else if (!prev) {
      if (abs(next->x() - startX) + abs(next->y() - startY)
          < abs(next->x() - endX) + abs(next->y() - endY))
        hasEndExt = true;
      else
        hasBeginExt = true;
    } else if (!next) {
      if (abs(prev->x() - startX) + abs(prev->y() - startY)
          < abs(prev->x() - endX) + abs(prev->y() - endY))
        hasEndExt = true;
      else
        hasBeginExt = true;
    }
    if (prev && prev->z() != z && prev->x() == startX && prev->y() == startY) {
      hasBeginExt = true;
    } else if (next && next->z() != z && next->x() == startX
               && next->y() == startY) {
      hasBeginExt = true;
    }
    if (prev && prev->z() != z && prev->x() == endX && prev->y() == endY) {
      hasEndExt = true;
    } else if (next && next->z() != z && next->x() == endX
               && next->y() == endY) {
      hasEndExt = true;
    }
    frEndStyle es(frEndStyleEnum::frcVariableEndStyle);
    if (hasBeginExt)
      currStyle.setBeginStyle(
          es,
          max((int) currStyle.getBeginExt(), (int) ndr->getWireExtension(z)));
    if (hasEndExt)
      currStyle.setEndStyle(
          es, max((int) currStyle.getEndExt(), (int) ndr->getWireExtension(z)));
  }
}

bool FlexDRWorker::isInsideTaperBox(
    frMIdx x,
    frMIdx y,
    frMIdx startZ,
    frMIdx endZ,
    map<FlexMazeIdx, frBox3D*>& mazeIdx2TaperBox)
{
  FlexMazeIdx idx(x, y, startZ);
  auto it = mazeIdx2TaperBox.find(idx);
  if (it != mazeIdx2TaperBox.end())
    return true;
  idx.setZ(endZ);
  it = mazeIdx2TaperBox.find(idx);
  return it != mazeIdx2TaperBox.end();
}

void FlexDRWorker::routeNet_postRouteAddPathCost(drNet* net)
{
  int cnt = 0;
  for (auto& connFig : net->getRouteConnFigs()) {
    addPathCost(connFig.get());
    cnt++;
  }
  // cout <<"updated " <<cnt <<" connfig costs" <<endl;
}

void FlexDRWorker::routeNet_prepAreaMap(drNet* net,
                                        map<FlexMazeIdx, frCoord>& areaMap)
{
  FlexMazeIdx mIdx;
  for (auto& pin : net->getPins()) {
    for (auto& ap : pin->getAccessPatterns()) {
      ap->getMazeIdx(mIdx);
      auto it = areaMap.find(mIdx);
      if (it != areaMap.end()) {
        it->second = max(it->second, ap->getBeginArea());
      } else {
        areaMap[mIdx] = ap->getBeginArea();
      }
    }
  }
}

bool FlexDRWorker::routeNet(drNet* net)
{
  ProfileTask profile("DR:routeNet");
  if (graphics_) {
    graphics_->startNet(net);
  }

  if (net->getPins().size() <= 1) {
    return true;
  }

  set<drPin*, frBlockObjectComp> unConnPins;
  map<FlexMazeIdx, set<drPin*, frBlockObjectComp>> mazeIdx2unConnPins;
  map<FlexMazeIdx, frBox3D*>
      mazeIdx2TaperBox;  // access points -> taper box: used to efficiently know
                         // what points are in what taper boxes
  list<pair<drPin*, frBox3D>> pinTaperBoxes;
  set<FlexMazeIdx> apMazeIdx;
  set<FlexMazeIdx> realPinAPMazeIdx;
  routeNet_prep(net,
                unConnPins,
                mazeIdx2unConnPins,
                apMazeIdx,
                realPinAPMazeIdx,
                mazeIdx2TaperBox,
                pinTaperBoxes);
  // prep for area map
  map<FlexMazeIdx, frCoord> areaMap;
  if (ENABLE_BOUNDARY_MAR_FIX) {
    routeNet_prepAreaMap(net, areaMap);
  }

  FlexMazeIdx ccMazeIdx1, ccMazeIdx2;  // connComps ll, ur flexmazeidx
  frPoint centerPt;
  vector<FlexMazeIdx> connComps;
  routeNet_setSrc(unConnPins,
                  mazeIdx2unConnPins,
                  connComps,
                  ccMazeIdx1,
                  ccMazeIdx2,
                  centerPt);

  vector<FlexMazeIdx> path;  // astar must return with >= 1 idx
  bool isFirstConn = true;
  bool searchSuccess = true;
  while (!unConnPins.empty()) {
    mazePinInit();
    auto nextPin = routeNet_getNextDst(
        ccMazeIdx1, ccMazeIdx2, mazeIdx2unConnPins, pinTaperBoxes);
    path.clear();
    if (gridGraph_.search(connComps,
                          nextPin,
                          path,
                          ccMazeIdx1,
                          ccMazeIdx2,
                          centerPt,
                          mazeIdx2TaperBox)) {
      routeNet_postAstarUpdate(
          path, connComps, unConnPins, mazeIdx2unConnPins, isFirstConn);
      routeNet_postAstarWritePath(
          net, path, realPinAPMazeIdx, mazeIdx2TaperBox, apMazeIdx);
      routeNet_postAstarPatchMinAreaVio(net, path, areaMap);
      isFirstConn = false;
    } else {
      searchSuccess = false;
      logger_->report("Failed to find a path between pin " + nextPin->getName()
                      + " and source aps:");
      for (FlexMazeIdx& mi : connComps) {
        logger_->report("( {} {} {} ) (Idx) / ( {} {} ) (coords)",
                        mi.x(),
                        mi.y(),
                        mi.z(),
                        gridGraph_.xCoord(mi.x()),
                        gridGraph_.yCoord(mi.y()));
      }
      break;
    }
  }

  if (searchSuccess) {
    routeNet_postRouteAddPathCost(net);
  }
  return searchSuccess;
}

void FlexDRWorker::routeNet_postAstarPatchMinAreaVio(
    drNet* net,
    const vector<FlexMazeIdx>& path,
    const map<FlexMazeIdx, frCoord>& areaMap)
{
  if (path.empty()) {
    return;
  }
  // get path with separated (stacked vias)
  vector<FlexMazeIdx> points;
  for (int i = 0; i < (int) path.size() - 1; ++i) {
    auto currIdx = path[i];
    auto nextIdx = path[i + 1];
    if (currIdx.z() == nextIdx.z()) {
      points.push_back(currIdx);
    } else {
      if (currIdx.z() < nextIdx.z()) {
        for (auto z = currIdx.z(); z < nextIdx.z(); ++z) {
          FlexMazeIdx tmpIdx(currIdx.x(), currIdx.y(), z);
          points.push_back(tmpIdx);
        }
      } else {
        for (auto z = currIdx.z(); z > nextIdx.z(); --z) {
          FlexMazeIdx tmpIdx(currIdx.x(), currIdx.y(), z);
          points.push_back(tmpIdx);
        }
      }
    }
  }
  points.push_back(path.back());

  auto layerNum = gridGraph_.getLayerNum(points.front().z());
  auto minAreaConstraint = getTech()->getLayer(layerNum)->getAreaConstraint();

  frArea currArea = 0;
  if (ENABLE_BOUNDARY_MAR_FIX) {
    if (areaMap.find(points[0]) != areaMap.end()) {
      currArea = areaMap.find(points[0])->second;
    } else {
      currArea = (minAreaConstraint) ? minAreaConstraint->getMinArea() : 0;
    }
  } else {
    currArea = (minAreaConstraint) ? minAreaConstraint->getMinArea() : 0;
  }
  frCoord startViaHalfEncArea = 0, endViaHalfEncArea = 0;
  FlexMazeIdx prevIdx = points[0], currIdx;
  int i;
  int prev_i = 0;  // path start point
  for (i = 1; i < (int) points.size(); ++i) {
    currIdx = points[i];
    // check minAreaViolation when change layer, or last segment
    if (currIdx.z() != prevIdx.z()) {
      layerNum = gridGraph_.getLayerNum(prevIdx.z());
      minAreaConstraint = getTech()->getLayer(layerNum)->getAreaConstraint();
      frArea reqArea
          = (minAreaConstraint) ? minAreaConstraint->getMinArea() : 0;
      // add next via enclosure
      if (currIdx.z() < prevIdx.z()) {
        currArea += getHalfViaEncArea(prevIdx.z() - 1, false, net);
        endViaHalfEncArea = getHalfViaEncArea(prevIdx.z() - 1, false, net);
      } else {
        currArea += getHalfViaEncArea(prevIdx.z(), true, net);
        endViaHalfEncArea = getHalfViaEncArea(prevIdx.z(), true, net);
      }
      // push to minArea violation
      if (currArea < reqArea) {
        FlexMazeIdx bp, ep;
        frArea gapArea = reqArea
                         - (currArea - startViaHalfEncArea - endViaHalfEncArea)
                         - std::min(startViaHalfEncArea, endViaHalfEncArea);
        // new
        bool bpPatchStyle = true;  // style 1: left only; 0: right only
        bool epPatchStyle = false;
        // stack via
        if (i - 1 == prev_i) {
          bp = points[i - 1];
          ep = points[i - 1];
          bpPatchStyle = true;
          epPatchStyle = false;
          // planar
        } else {
          bp = points[prev_i];
          ep = points[i - 1];
          if (getTech()->getLayer(layerNum)->getDir()
              == frPrefRoutingDirEnum::frcHorzPrefRoutingDir) {
            if (points[prev_i].x() < points[prev_i + 1].x()) {
              bpPatchStyle = true;
            } else if (points[prev_i].x() > points[prev_i + 1].x()) {
              bpPatchStyle = false;
            } else {
              if (points[prev_i].x() < points[i - 1].x()) {
                bpPatchStyle = true;
              } else {
                bpPatchStyle = false;
              }
            }
            if (points[i - 1].x() < points[i - 2].x()) {
              epPatchStyle = true;
            } else if (points[i - 1].x() > points[i - 2].x()) {
              epPatchStyle = false;
            } else {
              if (points[i - 1].x() < points[prev_i].x()) {
                epPatchStyle = true;
              } else {
                epPatchStyle = false;
              }
            }
          } else {
            if (points[prev_i].y() < points[prev_i + 1].y()) {
              bpPatchStyle = true;
            } else if (points[prev_i].y() > points[prev_i + 1].y()) {
              bpPatchStyle = false;
            } else {
              if (points[prev_i].y() < points[i - 1].y()) {
                bpPatchStyle = true;
              } else {
                bpPatchStyle = false;
              }
            }
            if (points[i - 1].y() < points[i - 2].y()) {
              epPatchStyle = true;
            } else if (points[i - 1].y() > points[i - 2].y()) {
              epPatchStyle = false;
            } else {
              if (points[i - 1].y() < points[prev_i].y()) {
                epPatchStyle = true;
              } else {
                epPatchStyle = false;
              }
            }
          }
        }
        auto patchWidth = getTech()->getLayer(layerNum)->getWidth();
        routeNet_postAstarAddPatchMetal(
            net, bp, ep, gapArea, patchWidth, bpPatchStyle, epPatchStyle);
      }
      // init for next path
      if (currIdx.z() < prevIdx.z()) {
        currArea = getHalfViaEncArea(prevIdx.z() - 1, true, net);
        startViaHalfEncArea = getHalfViaEncArea(prevIdx.z() - 1, true, net);
      } else {
        currArea = getHalfViaEncArea(
            prevIdx.z(),
            false,
            net);  // gridGraph_.getHalfViaEncArea(prevIdx.z(), false);
        startViaHalfEncArea = gridGraph_.getHalfViaEncArea(prevIdx.z(), false);
      }
      prev_i = i;
    }
    // add the wire area
    else {
      layerNum = gridGraph_.getLayerNum(prevIdx.z());
      minAreaConstraint = getTech()->getLayer(layerNum)->getAreaConstraint();
      frArea reqArea
          = (minAreaConstraint) ? minAreaConstraint->getMinArea() : 0;
      auto pathWidth = getTech()->getLayer(layerNum)->getWidth();
      frPoint bp, ep;
      gridGraph_.getPoint(bp, prevIdx.x(), prevIdx.y());
      gridGraph_.getPoint(ep, currIdx.x(), currIdx.y());
      frCoord pathLength = abs(bp.x() - ep.x()) + abs(bp.y() - ep.y());
      if (currArea < reqArea) {
        currArea += pathLength * pathWidth;
      }
    }
    prevIdx = currIdx;
  }
  // add boundary area for last segment
  if (ENABLE_BOUNDARY_MAR_FIX) {
    layerNum = gridGraph_.getLayerNum(prevIdx.z());
    minAreaConstraint = getTech()->getLayer(layerNum)->getAreaConstraint();
    frArea reqArea = (minAreaConstraint) ? minAreaConstraint->getMinArea() : 0;
    if (areaMap.find(prevIdx) != areaMap.end()) {
      currArea += areaMap.find(prevIdx)->second;
    }
    endViaHalfEncArea = 0;
    if (currArea < reqArea) {
      FlexMazeIdx bp, ep;
      frArea gapArea = reqArea
                       - (currArea - startViaHalfEncArea - endViaHalfEncArea)
                       - std::min(startViaHalfEncArea, endViaHalfEncArea);
      // new
      bool bpPatchStyle = true;  // style 1: left only; 0: right only
      bool epPatchStyle = false;
      // stack via
      if (i - 1 == prev_i) {
        bp = points[i - 1];
        ep = points[i - 1];
        bpPatchStyle = true;
        epPatchStyle = false;
        // planar
      } else {
        bp = points[prev_i];
        ep = points[i - 1];
        if (getTech()->getLayer(layerNum)->getDir()
            == frPrefRoutingDirEnum::frcHorzPrefRoutingDir) {
          if (points[prev_i].x() < points[prev_i + 1].x()) {
            bpPatchStyle = true;
          } else if (points[prev_i].x() > points[prev_i + 1].x()) {
            bpPatchStyle = false;
          } else {
            if (points[prev_i].x() < points[i - 1].x()) {
              bpPatchStyle = true;
            } else {
              bpPatchStyle = false;
            }
          }
          if (points[i - 1].x() < points[i - 2].x()) {
            epPatchStyle = true;
          } else if (points[i - 1].x() > points[i - 2].x()) {
            epPatchStyle = false;
          } else {
            if (points[i - 1].x() < points[prev_i].x()) {
              epPatchStyle = true;
            } else {
              epPatchStyle = false;
            }
          }
        } else {
          if (points[prev_i].y() < points[prev_i + 1].y()) {
            bpPatchStyle = true;
          } else if (points[prev_i].y() > points[prev_i + 1].y()) {
            bpPatchStyle = false;
          } else {
            if (points[prev_i].y() < points[i - 1].y()) {
              bpPatchStyle = true;
            } else {
              bpPatchStyle = false;
            }
          }
          if (points[i - 1].y() < points[i - 2].y()) {
            epPatchStyle = true;
          } else if (points[i - 1].y() > points[i - 2].y()) {
            epPatchStyle = false;
          } else {
            if (points[i - 1].y() < points[prev_i].y()) {
              epPatchStyle = true;
            } else {
              epPatchStyle = false;
            }
          }
        }
      }
      auto patchWidth = getTech()->getLayer(layerNum)->getWidth();
      routeNet_postAstarAddPatchMetal(
          net, bp, ep, gapArea, patchWidth, bpPatchStyle, epPatchStyle);
    }
  }
}

frCoord FlexDRWorker::getHalfViaEncArea(frMIdx z, bool isLayer1, drNet* net)
{
  if (!net || !net->getFrNet()->getNondefaultRule()
      || !net->getFrNet()->getNondefaultRule()->getPrefVia(z))
    return gridGraph_.getHalfViaEncArea(z, isLayer1);
  frVia via(net->getFrNet()->getNondefaultRule()->getPrefVia(z));
  frBox box;
  if (isLayer1)
    via.getLayer1BBox(box);
  else
    via.getLayer2BBox(box);
  return box.width() * box.length() / 2;
}
// assumes patchWidth == defaultWidth
// the cost checking part is sensitive to how cost is stored (1) planar + via;
// or (2) N;E;U
int FlexDRWorker::routeNet_postAstarAddPathMetal_isClean(
    const FlexMazeIdx& bpIdx,
    bool isPatchHorz,
    bool isPatchLeft,
    frCoord patchLength)
{
  int cost = 0;
  frPoint origin, patchEnd;
  gridGraph_.getPoint(origin, bpIdx.x(), bpIdx.y());
  frLayerNum layerNum = gridGraph_.getLayerNum(bpIdx.z());
  if (isPatchHorz) {
    if (isPatchLeft) {
      patchEnd.set(origin.x() - patchLength, origin.y());
    } else {
      patchEnd.set(origin.x() + patchLength, origin.y());
    }
  } else {
    if (isPatchLeft) {
      patchEnd.set(origin.x(), origin.y() - patchLength);
    } else {
      patchEnd.set(origin.x(), origin.y() + patchLength);
    }
  }
  // for wire, no need to bloat width
  frPoint patchLL = min(origin, patchEnd);
  frPoint patchUR = max(origin, patchEnd);
  if (!getRouteBox().contains(patchEnd)) {
    cost = std::numeric_limits<int>::max();
  } else {
    FlexMazeIdx startIdx, endIdx;
    startIdx.set(0, 0, layerNum);
    endIdx.set(0, 0, layerNum);
    frBox patchBox(patchLL, patchUR);
    gridGraph_.getIdxBox(startIdx, endIdx, patchBox, FlexGridGraph::enclose);
    if (isPatchHorz) {
      // in gridgraph, the planar cost is checked for xIdx + 1
      for (auto xIdx = max(0, startIdx.x() - 1); xIdx < endIdx.x(); ++xIdx) {
        if (gridGraph_.hasRouteShapeCost(
                xIdx, bpIdx.y(), bpIdx.z(), frDirEnum::E)) {
          cost += gridGraph_.getEdgeLength(
                      xIdx, bpIdx.y(), bpIdx.z(), frDirEnum::E)
                  * workerDRCCost_;
        }
        if (gridGraph_.hasFixedShapeCost(
                xIdx, bpIdx.y(), bpIdx.z(), frDirEnum::E)) {
          cost += gridGraph_.getEdgeLength(
                      xIdx, bpIdx.y(), bpIdx.z(), frDirEnum::E)
                  * FIXEDSHAPECOST;
        }
        if (gridGraph_.hasMarkerCost(
                xIdx, bpIdx.y(), bpIdx.z(), frDirEnum::E)) {
          cost += gridGraph_.getEdgeLength(
                      xIdx, bpIdx.y(), bpIdx.z(), frDirEnum::E)
                  * workerMarkerCost_;
        }
      }
    } else {
      // in gridgraph, the planar cost is checked for yIdx + 1
      for (auto yIdx = max(0, startIdx.y() - 1); yIdx < endIdx.y(); ++yIdx) {
        if (gridGraph_.hasRouteShapeCost(
                bpIdx.x(), yIdx, bpIdx.z(), frDirEnum::N)) {
          cost += gridGraph_.getEdgeLength(
                      bpIdx.x(), yIdx, bpIdx.z(), frDirEnum::N)
                  * workerDRCCost_;
        }
        if (gridGraph_.hasFixedShapeCost(
                bpIdx.x(), yIdx, bpIdx.z(), frDirEnum::N)) {
          cost += gridGraph_.getEdgeLength(
                      bpIdx.x(), yIdx, bpIdx.z(), frDirEnum::N)
                  * FIXEDSHAPECOST;
        }
        if (gridGraph_.hasMarkerCost(
                bpIdx.x(), yIdx, bpIdx.z(), frDirEnum::N)) {
          cost += gridGraph_.getEdgeLength(
                      bpIdx.x(), yIdx, bpIdx.z(), frDirEnum::N)
                  * workerMarkerCost_;
        }
      }
    }
  }
  return cost;
}

void FlexDRWorker::routeNet_postAstarAddPatchMetal_addPWire(
    drNet* net,
    const FlexMazeIdx& bpIdx,
    bool isPatchHorz,
    bool isPatchLeft,
    frCoord patchLength,
    frCoord patchWidth)
{
  frPoint origin, patchEnd;
  gridGraph_.getPoint(origin, bpIdx.x(), bpIdx.y());
  frLayerNum layerNum = gridGraph_.getLayerNum(bpIdx.z());
  // actual offsetbox
  frPoint patchLL, patchUR;
  if (isPatchHorz) {
    if (isPatchLeft) {
      patchLL.set(0 - patchLength, 0 - patchWidth / 2);
      patchUR.set(0, 0 + patchWidth / 2);
    } else {
      patchLL.set(0, 0 - patchWidth / 2);
      patchUR.set(0 + patchLength, 0 + patchWidth / 2);
    }
  } else {
    if (isPatchLeft) {
      patchLL.set(0 - patchWidth / 2, 0 - patchLength);
      patchUR.set(0 + patchWidth / 2, 0);
    } else {
      patchLL.set(0 - patchWidth / 2, 0);
      patchUR.set(0 + patchWidth / 2, 0 + patchLength);
    }
  }

  auto tmpPatch = make_unique<drPatchWire>();
  tmpPatch->setLayerNum(layerNum);
  tmpPatch->setOrigin(origin);
  tmpPatch->setOffsetBox(frBox(patchLL, patchUR));
  tmpPatch->addToNet(net);
  unique_ptr<drConnFig> tmp(std::move(tmpPatch));
  auto& workerRegionQuery = getWorkerRegionQuery();
  workerRegionQuery.add(tmp.get());
  net->addRoute(std::move(tmp));
}

void FlexDRWorker::routeNet_postAstarAddPatchMetal(drNet* net,
                                                   const FlexMazeIdx& bpIdx,
                                                   const FlexMazeIdx& epIdx,
                                                   frCoord gapArea,
                                                   frCoord patchWidth,
                                                   bool bpPatchStyle,
                                                   bool epPatchStyle)
{
  bool isPatchHorz;
  // bool isLeftClean = true;
  frLayerNum layerNum = gridGraph_.getLayerNum(bpIdx.z());
  frCoord patchLength = frCoord(ceil(1.0 * gapArea / patchWidth
                                     / getTech()->getManufacturingGrid()))
                        * getTech()->getManufacturingGrid();

  // always patch to pref dir
  if (getTech()->getLayer(layerNum)->getDir() == frcHorzPrefRoutingDir) {
    isPatchHorz = true;
  } else {
    isPatchHorz = false;
  }

  auto costL = routeNet_postAstarAddPathMetal_isClean(
      bpIdx, isPatchHorz, bpPatchStyle, patchLength);
  auto costR = routeNet_postAstarAddPathMetal_isClean(
      epIdx, isPatchHorz, epPatchStyle, patchLength);
  if (costL <= costR) {
    routeNet_postAstarAddPatchMetal_addPWire(
        net, bpIdx, isPatchHorz, bpPatchStyle, patchLength, patchWidth);
  } else {
    routeNet_postAstarAddPatchMetal_addPWire(
        net, epIdx, isPatchHorz, epPatchStyle, patchLength, patchWidth);
  }
}
