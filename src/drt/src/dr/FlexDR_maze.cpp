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

#include "db/gcObj/gcNet.h"
#include "db/gcObj/gcPin.h"
#include "dr/FlexDR.h"
#include "dr/FlexDR_graphics.h"
#include "frProfileTask.h"
#include "gc/FlexGC.h"

using namespace std;
using namespace fr;
namespace gtl = boost::polygon;

int beginDebugIter = std::numeric_limits<int>().max();
static frSquaredDistance pt2boxDistSquare(const Point& pt, const Rect& box)

{
  frCoord dx = max(max(box.xMin() - pt.x(), pt.x() - box.xMax()), 0);
  frCoord dy = max(max(box.yMin() - pt.y(), pt.y() - box.yMax()), 0);
  return (frSquaredDistance) dx * dx + (frSquaredDistance) dy * dy;
}

// prlx = -dx, prly = -dy
// dx > 0 : disjoint in x; dx = 0 : touching in x; dx < 0 : overlap in x
static frSquaredDistance box2boxDistSquareNew(const Rect& box1,
                                              const Rect& box2,
                                              frCoord& dx,
                                              frCoord& dy)
{
  dx = max(box1.xMin(), box2.xMin()) - min(box1.xMax(), box2.xMax());
  dy = max(box1.yMin(), box2.yMin()) - min(box1.yMax(), box2.yMax());
  return (frSquaredDistance) max(dx, 0) * max(dx, 0)
         + (frSquaredDistance) max(dy, 0) * max(dy, 0);
}

void FlexDRWorker::modViaForbiddenThrough(const FlexMazeIdx& bi,
                                          const FlexMazeIdx& ei,
                                          ModCostType type)
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
          case subRouteShape:
            gridGraph_.subRouteShapeCostVia(
                xIdx, bi.y(), bi.z() - 1);  // safe access
            break;
          case addRouteShape:
            gridGraph_.addRouteShapeCostVia(
                xIdx, bi.y(), bi.z() - 1);  // safe access
            break;
          case subFixedShape:
            gridGraph_.subFixedShapeCostVia(
                xIdx, bi.y(), bi.z() - 1);  // safe access
            break;
          case addFixedShape:
            gridGraph_.addFixedShapeCostVia(
                xIdx, bi.y(), bi.z() - 1);  // safe access
            break;
          default:;
        }
      }

      if (isUpperViaForbidden) {
        switch (type) {
          case subRouteShape:
            gridGraph_.subRouteShapeCostVia(
                xIdx, bi.y(), bi.z());  // safe access
            break;
          case addRouteShape:
            gridGraph_.addRouteShapeCostVia(
                xIdx, bi.y(), bi.z());  // safe access
            break;
          case subFixedShape:
            gridGraph_.subFixedShapeCostVia(
                xIdx, bi.y(), bi.z());  // safe access
            break;
          case addFixedShape:
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
          case subRouteShape:
            gridGraph_.subRouteShapeCostVia(
                bi.x(), yIdx, bi.z() - 1);  // safe access
            break;
          case addRouteShape:
            gridGraph_.addRouteShapeCostVia(
                bi.x(), yIdx, bi.z() - 1);  // safe access
            break;
          case subFixedShape:
            gridGraph_.subFixedShapeCostVia(
                bi.x(), yIdx, bi.z() - 1);  // safe access
            break;
          case addFixedShape:
            gridGraph_.addFixedShapeCostVia(
                bi.x(), yIdx, bi.z() - 1);  // safe access
            break;
          default:;
        }
      }

      if (isUpperViaForbidden) {
        switch (type) {
          case subRouteShape:
            gridGraph_.subRouteShapeCostVia(
                bi.x(), yIdx, bi.z());  // safe access
            break;
          case addRouteShape:
            gridGraph_.addRouteShapeCostVia(
                bi.x(), yIdx, bi.z());  // safe access
            break;
          case subFixedShape:
            gridGraph_.subFixedShapeCostVia(
                bi.x(), yIdx, bi.z());  // safe access
            break;
          case addFixedShape:
            gridGraph_.addFixedShapeCostVia(
                bi.x(), yIdx, bi.z());  // safe access
            break;
          default:;
        }
      }
    }
  }
}

void FlexDRWorker::modBlockedPlanar(const Rect& box, frMIdx z, bool setBlock)
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

void FlexDRWorker::modBlockedVia(const Rect& box, frMIdx z, bool setBlock)
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

void FlexDRWorker::modCornerToCornerSpacing_helper(const Rect& box,
                                                   frMIdx z,
                                                   ModCostType type)
{
  FlexMazeIdx p1, p2;
  gridGraph_.getIdxBox(p1, p2, box, FlexGridGraph::isEnclosed);
  for (int i = p1.x(); i <= p2.x(); i++) {
    for (int j = p1.y(); j <= p2.y(); j++) {
      switch (type) {
        case subRouteShape:
          gridGraph_.subRouteShapeCostPlanar(i, j, z);
          break;
        case addRouteShape:
          gridGraph_.addRouteShapeCostPlanar(i, j, z);
          break;
        case subFixedShape:
          gridGraph_.subFixedShapeCostPlanar(i, j, z);
          break;
        case addFixedShape:
          gridGraph_.addFixedShapeCostPlanar(i, j, z);
          break;
        default:;
      }
    }
  }
}
void FlexDRWorker::modCornerToCornerSpacing(const Rect& box,
                                            frMIdx z,
                                            ModCostType type)
{
  auto lNum = gridGraph_.getLayerNum(z);
  frCoord halfwidth2 = getTech()->getLayer(lNum)->getWidth() / 2;
  // spacing value needed
  frCoord bloatDist = 0;
  auto& cons = getTech()->getLayer(lNum)->getLef58CornerSpacingConstraints();
  Rect bx;
  for (auto& c : cons) {
    bloatDist = c->findMax() + halfwidth2 - 1;
    bx.init(box.xMin() - bloatDist,
            box.yMin() - bloatDist,
            box.xMin(),
            box.yMin());  // ll box corner
    modCornerToCornerSpacing_helper(bx, z, type);
    bx.init(box.xMin() - bloatDist,
            box.yMax(),
            box.xMin(),
            box.yMax() + bloatDist);  // ul box corner
    modCornerToCornerSpacing_helper(bx, z, type);
    bx.init(box.xMax(),
            box.yMax(),
            box.xMax() + bloatDist,
            box.yMax() + bloatDist);  // ur box corner
    modCornerToCornerSpacing_helper(bx, z, type);
    bx.init(box.xMax(),
            box.yMin() - bloatDist,
            box.xMax() + bloatDist,
            box.yMin());  // lr box corner
    modCornerToCornerSpacing_helper(bx, z, type);
  }
}

void FlexDRWorker::modMinSpacingCostPlanar(const Rect& box,
                                           frMIdx z,
                                           ModCostType type,
                                           bool isBlockage,
                                           frNonDefaultRule* ndr,
                                           bool isMacroPin,
                                           bool resetHorz,
                                           bool resetVert)
{
  auto lNum = gridGraph_.getLayerNum(z);
  frCoord width1 = box.minDXDY();
  frCoord length1 = box.maxDXDY();
  // layer default width
  frCoord width2 = getTech()->getLayer(lNum)->getWidth();
  frCoord halfwidth2 = width2 / 2;
  // spacing value needed
  bool use_min_spacing = isBlockage && USEMINSPACING_OBS;
  frCoord bloatDist = getTech()->getLayer(lNum)->getMinSpacingValue(
      width1, width2, length1, use_min_spacing);

  if (ndr)
    bloatDist = max(bloatDist, ndr->getSpacing(z));
  frSquaredDistance bloatDistSquare = bloatDist;
  bloatDistSquare *= bloatDist;

  FlexMazeIdx mIdx1, mPinLL;
  FlexMazeIdx mIdx2, mPinUR;
  // assumes width always > 2
  Rect bx(box.xMin() - bloatDist - halfwidth2 + 1,
          box.yMin() - bloatDist - halfwidth2 + 1,
          box.xMax() + bloatDist + halfwidth2 - 1,
          box.yMax() + bloatDist + halfwidth2 - 1);
  gridGraph_.getIdxBox(mIdx1, mIdx2, bx);
  if (isMacroPin && type == ModCostType::resetBlocked) {
    Rect sBox(box.xMin() + width2 / 2,
              box.yMin() + width2 / 2,
              box.xMax() - width2 / 2,
              box.yMax() - width2 / 2);
    gridGraph_.getIdxBox(mPinLL, mPinUR, sBox);
  }
  Point pt, pt1, pt2, pt3, pt4;
  frSquaredDistance distSquare = 0;
  for (int i = mIdx1.x(); i <= mIdx2.x(); i++) {
    for (int j = mIdx1.y(); j <= mIdx2.y(); j++) {
      gridGraph_.getPoint(pt, i, j);
      pt1 = {pt.x() + halfwidth2, pt.y() - halfwidth2};
      pt2 = {pt.x() + halfwidth2, pt.y() + halfwidth2};
      pt3 = {pt.x() - halfwidth2, pt.y() - halfwidth2};
      pt4 = {pt.x() - halfwidth2, pt.y() + halfwidth2};
      distSquare = min(pt2boxDistSquare(pt1, box), pt2boxDistSquare(pt2, box));
      distSquare = min(pt2boxDistSquare(pt3, box), distSquare);
      distSquare = min(pt2boxDistSquare(pt4, box), distSquare);
      if (distSquare < bloatDistSquare) {
        switch (type) {
          case subRouteShape:
            gridGraph_.subRouteShapeCostPlanar(i, j, z);  // safe access
            break;
          case addRouteShape:
            gridGraph_.addRouteShapeCostPlanar(i, j, z);  // safe access
            break;
          case subFixedShape:
            gridGraph_.subFixedShapeCostPlanar(i, j, z);  // safe access
            break;
          case addFixedShape:
            gridGraph_.addFixedShapeCostPlanar(i, j, z);  // safe access
            break;
          case resetFixedShape:
            if (resetHorz) {
              gridGraph_.setFixedShapeCostPlanarHorz(
                  i, j, z, 0);  // safe access
            }
            if (resetVert) {
              gridGraph_.setFixedShapeCostPlanarVert(
                  i, j, z, 0);  // safe access
            }
            break;
          case setFixedShape:
            gridGraph_.setFixedShapeCostPlanarHorz(i, j, z, 1);  // safe access
            gridGraph_.setFixedShapeCostPlanarVert(i, j, z, 1);  // safe access
            break;
          case resetBlocked:
            if (isMacroPin) {
              if (j >= mPinLL.y() && j <= mPinUR.y()) {
                gridGraph_.resetBlocked(i, j, z, frDirEnum::E);
                if (i == 0)
                  gridGraph_.resetBlocked(i, j, z, frDirEnum::W);
              }
              if (i >= mPinLL.x() && i <= mPinUR.x()) {
                gridGraph_.resetBlocked(i, j, z, frDirEnum::N);
                if (j == 0)
                  gridGraph_.resetBlocked(i, j, z, frDirEnum::S);
              }
            } else {
              gridGraph_.resetBlocked(i, j, z, frDirEnum::E);
              gridGraph_.resetBlocked(i, j, z, frDirEnum::N);
              if (i == 0)
                gridGraph_.resetBlocked(i, j, z, frDirEnum::W);
              if (j == 0)
                gridGraph_.resetBlocked(i, j, z, frDirEnum::S);
            }
            break;
          case setBlocked:  // set blocked
            gridGraph_.setBlocked(i, j, z, frDirEnum::E);
            gridGraph_.setBlocked(i, j, z, frDirEnum::N);
            if (i == 0)
              gridGraph_.setBlocked(i, j, z, frDirEnum::W);
            if (j == 0)
              gridGraph_.setBlocked(i, j, z, frDirEnum::S);
            break;
          default:;
        }
      }
    }
  }
}

void FlexDRWorker::modMinSpacingCostVia_eol_helper(const Rect& box,
                                                   const Rect& testBox,
                                                   ModCostType type,
                                                   bool isUpperVia,
                                                   frMIdx i,
                                                   frMIdx j,
                                                   frMIdx z)
{
  frMIdx zIdx = isUpperVia ? z : z - 1;
  if (testBox.overlaps(box)) {
    switch (type) {
      case subRouteShape:
        gridGraph_.subRouteShapeCostVia(i, j, zIdx);
        break;
      case addRouteShape:
        gridGraph_.addRouteShapeCostVia(i, j, zIdx);
        break;
      case subFixedShape:
        gridGraph_.subFixedShapeCostVia(i, j, zIdx);  // safe access
        break;
      case addFixedShape:
        gridGraph_.addFixedShapeCostVia(i, j, zIdx);  // safe access
        break;
      default:;
    }
  }
}

void FlexDRWorker::modMinSpacingCostVia_eol(const Rect& box,
                                            const Rect& tmpBx,
                                            ModCostType type,
                                            bool isUpperVia,
                                            const drEolSpacingConstraint& drCon,
                                            frMIdx i,
                                            frMIdx j,
                                            frMIdx z)
{
  if (drCon.eolSpace == 0)
    return;
  Rect testBox;
  frCoord eolSpace = drCon.eolSpace;
  frCoord eolWidth = drCon.eolWidth;
  frCoord eolWithin = drCon.eolWithin;
  // eol to up and down
  if (tmpBx.dx() <= eolWidth) {
    testBox.init(tmpBx.xMin() - eolWithin,
                 tmpBx.yMax(),
                 tmpBx.xMax() + eolWithin,
                 tmpBx.yMax() + eolSpace);
    modMinSpacingCostVia_eol_helper(box, testBox, type, isUpperVia, i, j, z);

    testBox.init(tmpBx.xMin() - eolWithin,
                 tmpBx.yMin() - eolSpace,
                 tmpBx.xMax() + eolWithin,
                 tmpBx.yMin());
    modMinSpacingCostVia_eol_helper(box, testBox, type, isUpperVia, i, j, z);
  }
  // eol to left and right
  if (tmpBx.dy() <= eolWidth) {
    testBox.init(tmpBx.xMax(),
                 tmpBx.yMin() - eolWithin,
                 tmpBx.xMax() + eolSpace,
                 tmpBx.yMax() + eolWithin);
    modMinSpacingCostVia_eol_helper(box, testBox, type, isUpperVia, i, j, z);

    testBox.init(tmpBx.xMin() - eolSpace,
                 tmpBx.yMin() - eolWithin,
                 tmpBx.xMin(),
                 tmpBx.yMax() + eolWithin);
    modMinSpacingCostVia_eol_helper(box, testBox, type, isUpperVia, i, j, z);
  }
}

void FlexDRWorker::modMinimumcutCostVia(const Rect& box,
                                        frMIdx z,
                                        ModCostType type,
                                        bool isUpperVia)
{
  auto lNum = gridGraph_.getLayerNum(z);
  frCoord width1 = box.minDXDY();
  frCoord length1 = box.maxDXDY();
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
  Rect viaBox(0, 0, 0, 0);
  if (isUpperVia) {
    viaBox = via.getCutBBox();
  } else {
    viaBox = via.getCutBBox();
  }

  FlexMazeIdx mIdx1, mIdx2;
  Rect bx, tmpBx, sViaBox;
  dbTransform xform;
  Point pt;
  frCoord dx, dy;
  frVia sVia;
  frMIdx zIdx = isUpperVia ? z : z - 1;
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
      bx.init(box.xMin() - dist - (viaBox.xMax() - 0) + 1,
              box.yMin() - dist - (viaBox.yMax() - 0) + 1,
              box.xMax() + dist + (0 - viaBox.xMin()) - 1,
              box.yMax() + dist + (0 - viaBox.yMin()) - 1);
      gridGraph_.getIdxBox(mIdx1, mIdx2, bx);

      for (int i = mIdx1.x(); i <= mIdx2.x(); i++) {
        for (int j = mIdx1.y(); j <= mIdx2.y(); j++) {
          gridGraph_.getPoint(pt, i, j);
          xform.setOffset(pt);
          tmpBx = viaBox;
          if (gridGraph_.isSVia(i, j, zIdx)) {
            auto sViaDef = apSVia_[FlexMazeIdx(i, j, zIdx)]->getAccessViaDef();
            sVia.setViaDef(sViaDef);
            if (isUpperVia) {
              sViaBox = sVia.getCutBBox();
            } else {
              sViaBox = sVia.getCutBBox();
            }
            tmpBx = sViaBox;
          }
          xform.apply(tmpBx);
          box2boxDistSquareNew(box, tmpBx, dx, dy);
          if (!con->hasLength()) {
            if (dx <= 0 && dy <= 0) {
              ;
            } else {
              continue;
            }
          } else {
            dx = std::max(dx, 0);
            dy = std::max(dy, 0);
            if (((dx > 0) ^ (dy > 0)) && dx + dy < dist) {
              ;
            } else {
              continue;
            }
          }
          switch (type) {
            case subRouteShape:
              gridGraph_.subRouteShapeCostVia(i, j, zIdx);  // safe access
              break;
            case addRouteShape:
              gridGraph_.addRouteShapeCostVia(i, j, zIdx);  // safe access
              break;
            case subFixedShape:
              gridGraph_.subFixedShapeCostVia(i, j, zIdx);  // safe access
              break;
            case addFixedShape:
              gridGraph_.addFixedShapeCostVia(i, j, zIdx);  // safe access
              break;
            case resetFixedShape:
              gridGraph_.setFixedShapeCostVia(i, j, z, 0);  // safe access
              break;
            case setFixedShape:
              gridGraph_.setFixedShapeCostVia(i, j, z, 1);  // safe access
            default:;
          }
        }
      }
    }
  }
}

void FlexDRWorker::modMinSpacingCostVia(const Rect& box,
                                        frMIdx z,
                                        ModCostType type,
                                        bool isUpperVia,
                                        bool isCurrPs,
                                        bool isBlockage,
                                        frNonDefaultRule* ndr)
{
  auto lNum = gridGraph_.getLayerNum(z);
  frCoord width1 = box.minDXDY();
  frCoord length1 = box.maxDXDY();
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
  Rect viaBox(0, 0, 0, 0);
  if (isUpperVia) {
    viaBox = via.getLayer1BBox();
  } else {
    viaBox = via.getLayer2BBox();
  }
  frCoord width2 = viaBox.minDXDY();
  frCoord length2 = viaBox.maxDXDY();

  // via prl should check min area patch metal if not fat via
  frCoord defaultWidth = getTech()->getLayer(lNum)->getWidth();
  bool isH
      = (getTech()->getLayer(lNum)->getDir() == dbTechLayerDir::HORIZONTAL);
  bool isFatVia
      = (isH) ? (viaBox.dy() > defaultWidth) : (viaBox.dx() > defaultWidth);

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
  frCoord prl = isCurrPs ? (length2_mar) : min(length1, length2_mar);
  bool use_min_spacing = isBlockage && USEMINSPACING_OBS && !isFatVia;
  frCoord bloatDist = getTech()->getLayer(lNum)->getMinSpacingValue(
      width1, width2, prl, use_min_spacing);

  drEolSpacingConstraint drCon;
  if (ndr) {
    bloatDist = max(bloatDist, ndr->getSpacing(z));
    drCon = ndr->getDrEolSpacingConstraint(z);
  }
  // other obj eol spc to curr obj
  // no need to bloat eolWithin because eolWithin always < minSpacing
  frCoord bloatDistEolX = 0;
  frCoord bloatDistEolY = 0;
  if (drCon.eolWidth == 0)
    drCon = getTech()->getLayer(lNum)->getDrEolSpacingConstraint();
  if (viaBox.dx() <= drCon.eolWidth) {
    bloatDistEolY = max(bloatDistEolY, drCon.eolSpace);
  }
  // eol left and right
  if (viaBox.dy() <= drCon.eolWidth) {
    bloatDistEolX = max(bloatDistEolX, drCon.eolSpace);
  }

  FlexMazeIdx mIdx1;
  FlexMazeIdx mIdx2;
  // assumes width always > 2
  Rect bx(box.xMin() - max(bloatDist, bloatDistEolX) - (viaBox.xMax() - 0) + 1,
          box.yMin() - max(bloatDist, bloatDistEolY) - (viaBox.yMax() - 0) + 1,
          box.xMax() + max(bloatDist, bloatDistEolX) + (0 - viaBox.xMin()) - 1,
          box.yMax() + max(bloatDist, bloatDistEolY) + (0 - viaBox.yMin()) - 1);
  gridGraph_.getIdxBox(mIdx1, mIdx2, bx);
  Point pt;
  Rect tmpBx;
  frSquaredDistance distSquare = 0;
  frCoord dx, dy;
  dbTransform xform;
  frVia sVia;
  frMIdx zIdx = isUpperVia ? z : z - 1;
  for (int i = mIdx1.x(); i <= mIdx2.x(); i++) {
    for (int j = mIdx1.y(); j <= mIdx2.y(); j++) {
      gridGraph_.getPoint(pt, i, j);
      xform.setOffset(pt);
      tmpBx = viaBox;
      if (gridGraph_.isSVia(i, j, zIdx)) {
        auto sViaDef = apSVia_[FlexMazeIdx(i, j, zIdx)]->getAccessViaDef();
        sVia.setViaDef(sViaDef);
        Rect sViaBox;
        if (isUpperVia) {
          sViaBox = sVia.getLayer1BBox();
        } else {
          sViaBox = sVia.getLayer2BBox();
        }
        tmpBx = sViaBox;
      }
      xform.apply(tmpBx);
      distSquare = box2boxDistSquareNew(box, tmpBx, dx, dy);
      prl = max(-dx, -dy);
      // curr is ps
      if (isCurrPs) {
        if (-dy >= 0 && prl == -dy) {
          prl = viaBox.dy();
          // ignore svia effect here...
          if (!isH && !isFatVia) {
            prl = max(prl, patchLength);
          }
        } else if (-dx >= 0 && prl == -dx) {
          prl = viaBox.dx();
          if (isH && !isFatVia) {
            prl = max(prl, patchLength);
          }
        }
      }
      bool use_min_spacing = isBlockage && USEMINSPACING_OBS && !isFatVia;
      frCoord reqDist = getTech()->getLayer(lNum)->getMinSpacingValue(
          width1, width2, prl, use_min_spacing);

      if (ndr)
        reqDist = max(reqDist, ndr->getSpacing(z));
      if (distSquare < (frSquaredDistance) reqDist * reqDist) {
        switch (type) {
          case subRouteShape:
            gridGraph_.subRouteShapeCostVia(i, j, zIdx);  // safe access
            break;
          case addRouteShape:
            gridGraph_.addRouteShapeCostVia(i, j, zIdx);  // safe access
            break;
          case subFixedShape:
            gridGraph_.subFixedShapeCostVia(i, j, zIdx);  // safe access
            break;
          case addFixedShape:
            gridGraph_.addFixedShapeCostVia(i, j, zIdx);  // safe access
            break;
          default:;
        }
      }
      // eol, other obj to curr obj
      modMinSpacingCostVia_eol(box, tmpBx, type, isUpperVia, drCon, i, j, z);
    }
  }
}

// eolType == 0: planer
// eolType == 1: down
// eolType == 2: up
void FlexDRWorker::modEolSpacingCost_helper(const Rect& testbox,
                                            frMIdx z,
                                            ModCostType type,
                                            int eolType,
                                            bool resetHorz,
                                            bool resetVert)
{
  auto lNum = gridGraph_.getLayerNum(z);
  Rect bx;
  if (eolType == 0) {
    // layer default width
    frCoord width2 = getTech()->getLayer(lNum)->getWidth();
    frCoord halfwidth2 = width2 / 2;
    // assumes width always > 2
    bx.init(testbox.xMin() - halfwidth2 + 1,
            testbox.yMin() - halfwidth2 + 1,
            testbox.xMax() + halfwidth2 - 1,
            testbox.yMax() + halfwidth2 - 1);
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
    Rect viaBox(0, 0, 0, 0);
    if (eolType == 2) {  // upper via
      viaBox = via.getLayer1BBox();
    } else {
      viaBox = via.getLayer2BBox();
    }
    // assumes via bbox always > 2
    bx.init(testbox.xMin() - (viaBox.xMax() - 0) + 1,
            testbox.yMin() - (viaBox.yMax() - 0) + 1,
            testbox.xMax() + (0 - viaBox.xMin()) - 1,
            testbox.yMax() + (0 - viaBox.yMin()) - 1);
  }

  FlexMazeIdx mIdx1;
  FlexMazeIdx mIdx2;
  gridGraph_.getIdxBox(mIdx1, mIdx2, bx);  // >= bx

  frVia sVia;
  Rect sViaBox;
  Point pt;

  for (int i = mIdx1.x(); i <= mIdx2.x(); i++) {
    for (int j = mIdx1.y(); j <= mIdx2.y(); j++) {
      if (eolType == 0) {
        switch (type) {
          case subRouteShape:
            gridGraph_.subRouteShapeCostPlanar(i, j, z);  // safe access
            break;
          case addRouteShape:
            gridGraph_.addRouteShapeCostPlanar(i, j, z);  // safe access
            break;
          case subFixedShape:
            gridGraph_.subFixedShapeCostPlanar(i, j, z);  // safe access
            break;
          case addFixedShape:
            gridGraph_.addFixedShapeCostPlanar(i, j, z);  // safe access
            break;
          case resetFixedShape:
            if (resetHorz) {
              gridGraph_.setFixedShapeCostPlanarHorz(
                  i, j, z, 0);  // safe access
            }
            if (resetVert) {
              gridGraph_.setFixedShapeCostPlanarVert(
                  i, j, z, 0);  // safe access
            }
            break;
          case setFixedShape:
            gridGraph_.setFixedShapeCostPlanarHorz(i, j, z, 1);  // safe access
            gridGraph_.setFixedShapeCostPlanarVert(i, j, z, 1);  // safe access
            break;
          default:;
        }
      } else if (eolType == 1) {
        if (gridGraph_.isSVia(i, j, z - 1)) {
          gridGraph_.getPoint(pt, i, j);
          auto sViaDef = apSVia_[FlexMazeIdx(i, j, z - 1)]->getAccessViaDef();
          sVia.setViaDef(sViaDef);
          sVia.setOrigin(pt);
          sViaBox = sVia.getLayer2BBox();
          if (!sViaBox.overlaps(testbox)) {
            continue;
          }
        }
        switch (type) {
          case subRouteShape:
            gridGraph_.subRouteShapeCostVia(i, j, z - 1);  // safe access
            break;
          case addRouteShape:
            gridGraph_.addRouteShapeCostVia(i, j, z - 1);  // safe access
            break;
          case subFixedShape:
            gridGraph_.subFixedShapeCostVia(i, j, z - 1);  // safe access
            break;
          case addFixedShape:
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
          sViaBox = sVia.getLayer1BBox();
          if (!sViaBox.overlaps(testbox)) {
            continue;
          }
        }
        switch (type) {
          case subRouteShape:
            gridGraph_.subRouteShapeCostVia(i, j, z);  // safe access
            break;
          case addRouteShape:
            gridGraph_.addRouteShapeCostVia(i, j, z);  // safe access
            break;
          case subFixedShape:
            gridGraph_.subFixedShapeCostVia(i, j, z);  // safe access
            break;
          case addFixedShape:
            gridGraph_.addFixedShapeCostVia(i, j, z);  // safe access
            break;
          default:;
        }
      }
    }
  }
}

void FlexDRWorker::modEolSpacingRulesCost(const Rect& box,
                                          frMIdx z,
                                          ModCostType type,
                                          bool isSkipVia,
                                          frNonDefaultRule* ndr,
                                          bool resetHorz,
                                          bool resetVert)
{
  auto layer = getTech()->getLayer(gridGraph_.getLayerNum(z));
  drEolSpacingConstraint drCon;
  if (ndr != nullptr)
    drCon = ndr->getDrEolSpacingConstraint(z);
  if (drCon.eolWidth == 0)
    drCon = layer->getDrEolSpacingConstraint();
  frCoord eolSpace, eolWidth, eolWithin;
  eolSpace = drCon.eolSpace;
  eolWithin = drCon.eolWithin;
  eolWidth = drCon.eolWidth;
  if (eolSpace == 0)
    return;
  Rect testBox;
  if (box.dx() <= eolWidth) {
    testBox.init(box.xMin() - eolWithin,
                 box.yMax(),
                 box.xMax() + eolWithin,
                 box.yMax() + eolSpace);
    // if (!isInitDR()) {
    modEolSpacingCost_helper(testBox, z, type, 0, resetHorz, resetVert);
    if (!isSkipVia) {
      modEolSpacingCost_helper(testBox, z, type, 1, resetHorz, resetVert);
      modEolSpacingCost_helper(testBox, z, type, 2, resetHorz, resetVert);
    }
    testBox.init(box.xMin() - eolWithin,
                 box.yMin() - eolSpace,
                 box.xMax() + eolWithin,
                 box.yMin());
    modEolSpacingCost_helper(testBox, z, type, 0, resetHorz, resetVert);
    if (!isSkipVia) {
      modEolSpacingCost_helper(testBox, z, type, 1, resetHorz, resetVert);
      modEolSpacingCost_helper(testBox, z, type, 2, resetHorz, resetVert);
    }
  }
  // eol to left and right
  if (box.dy() <= eolWidth) {
    testBox.init(box.xMax(),
                 box.yMin() - eolWithin,
                 box.xMax() + eolSpace,
                 box.yMax() + eolWithin);
    modEolSpacingCost_helper(testBox, z, type, 0, resetHorz, resetVert);
    if (!isSkipVia) {
      modEolSpacingCost_helper(testBox, z, type, 1, resetHorz, resetVert);
      modEolSpacingCost_helper(testBox, z, type, 2, resetHorz, resetVert);
    }
    testBox.init(box.xMin() - eolSpace,
                 box.yMin() - eolWithin,
                 box.xMin(),
                 box.yMax() + eolWithin);
    modEolSpacingCost_helper(testBox, z, type, 0, resetHorz, resetVert);
    if (!isSkipVia) {
      modEolSpacingCost_helper(testBox, z, type, 1, resetHorz, resetVert);
      modEolSpacingCost_helper(testBox, z, type, 2, resetHorz, resetVert);
    }
  }
}

// forbid via if it would trigger violation
void FlexDRWorker::modAdjCutSpacingCost_fixedObj(const frDesign* design,
                                                 const Rect& origCutBox,
                                                 frVia* origVia)
{
  if (!origVia->getNet()->getType().isSupply()) {
    return;
  }
  auto lNum = origVia->getViaDef()->getCutLayerNum();
  for (auto con : getTech()->getLayer(lNum)->getCutSpacing()) {
    if (con->getAdjacentCuts() == -1) {
      continue;
    }
    bool hasFixedViol = false;

    gtl::point_data<frCoord> origCenter(
        (origCutBox.xMin() + origCutBox.xMax()) / 2,
        (origCutBox.yMin() + origCutBox.yMax()) / 2);
    gtl::rectangle_data<frCoord> origCutRect(origCutBox.xMin(),
                                             origCutBox.yMin(),
                                             origCutBox.xMax(),
                                             origCutBox.yMax());

    Rect viaBox = origVia->getCutBBox();

    frSquaredDistance reqDistSquare = con->getCutSpacing();
    reqDistSquare *= reqDistSquare;

    auto cutWithin = con->getCutWithin();
    Rect queryBox;
    viaBox.bloat(cutWithin, queryBox);

    frRegionQuery::Objects<frBlockObject> result;
    design->getRegionQuery()->query(queryBox, lNum, result);

    for (auto& [box, obj] : result) {
      if (obj->typeId() == frcVia) {
        auto via = static_cast<frVia*>(obj);
        if (!via->getNet()->getType().isSupply()) {
          continue;
        }
        if (origCutBox == box) {
          continue;
        }

        gtl::rectangle_data<frCoord> cutRect(
            box.xMin(), box.yMin(), box.xMax(), box.yMax());
        gtl::point_data<frCoord> cutCenterPt((box.xMin() + box.xMax()) / 2,
                                             (box.yMin() + box.yMax()) / 2);

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
      Rect spacingBox;
      auto reqDist = con->getCutSpacing();
      auto cutWidth = getTech()->getLayer(lNum)->getWidth();
      if (con->hasCenterToCenter()) {
        spacingBox.init(origCenter.x() - reqDist,
                        origCenter.y() - reqDist,
                        origCenter.x() + reqDist,
                        origCenter.y() + reqDist);
      } else {
        origCutBox.bloat(reqDist + cutWidth / 2, spacingBox);
      }
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

/*inline*/ void FlexDRWorker::modCutSpacingCost(const Rect& box,
                                                frMIdx z,
                                                ModCostType type,
                                                bool isBlockage,
                                                int avoidI,
                                                int avoidJ)
{
  auto lNum = gridGraph_.getLayerNum(z) + 1;
  auto cutLayer = getTech()->getLayer(lNum);
  if (!cutLayer->hasCutSpacing()
      && !cutLayer->hasLef58DiffNetCutSpcTblConstraint()) {
    return;
  }
  // obj1 = curr obj
  // obj2 = other obj
  // default via dimension
  frViaDef* viaDef = cutLayer->getDefaultViaDef();
  frVia via(viaDef);
  Rect viaBox = via.getCutBBox();

  // spacing value needed
  frCoord bloatDist = 0;
  for (auto con : cutLayer->getCutSpacing()) {
    bloatDist = max(bloatDist, con->getCutSpacing());
    if (con->getAdjacentCuts() != -1 && isBlockage) {
      bloatDist = max(bloatDist, con->getCutWithin());
    }
  }
  frLef58CutSpacingTableConstraint* lef58con = nullptr;
  std::pair<frCoord, frCoord> lef58conSpc;
  if (cutLayer->hasLef58DiffNetCutSpcTblConstraint())
    lef58con = cutLayer->getLef58DiffNetCutSpcTblConstraint();

  if (lef58con != nullptr) {
    lef58conSpc = lef58con->getDefaultSpacing();
    bloatDist = max(bloatDist, std::max(lef58conSpc.first, lef58conSpc.second));
  }

  FlexMazeIdx mIdx1;
  FlexMazeIdx mIdx2;
  // assumes width always > 2
  Rect bx(box.xMin() - bloatDist - (viaBox.xMax() - 0) + 1,
          box.yMin() - bloatDist - (viaBox.yMax() - 0) + 1,
          box.xMax() + bloatDist + (0 - viaBox.xMin()) - 1,
          box.yMax() + bloatDist + (0 - viaBox.yMin()) - 1);
  gridGraph_.getIdxBox(mIdx1, mIdx2, bx);

  Point pt;
  frSquaredDistance distSquare = 0;
  frSquaredDistance c2cSquare = 0;
  frCoord dx, dy, prl;
  dbTransform xform;
  frSquaredDistance reqDistSquare = 0;
  Point boxCenter, tmpBxCenter;
  boxCenter = {(box.xMin() + box.xMax()) / 2, (box.yMin() + box.yMax()) / 2};
  frSquaredDistance currDistSquare = 0;
  bool hasViol;
  for (int i = mIdx1.x(); i <= mIdx2.x(); i++) {
    for (int j = mIdx1.y(); j <= mIdx2.y(); j++) {
      if (i == avoidI && j == avoidJ)
        continue;
      for (auto& uFig : via.getViaDef()->getCutFigs()) {
        auto obj = static_cast<frRect*>(uFig.get());
        gridGraph_.getPoint(pt, i, j);
        xform.setOffset(pt);
        Rect tmpBx = obj->getBBox();
        xform.apply(tmpBx);
        tmpBxCenter = {(tmpBx.xMin() + tmpBx.xMax()) / 2,
                       (tmpBx.yMin() + tmpBx.yMax()) / 2};
        distSquare = box2boxDistSquareNew(box, tmpBx, dx, dy);
        c2cSquare = Point::squaredDistance(boxCenter, tmpBxCenter);
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
            auto currArea = max(box.area(), tmpBx.area());
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
          bool center2center = false;
          if (prl > 0)
            reqDistSquare = lef58conSpc.second;
          else
            reqDistSquare = lef58conSpc.first;
          if (lef58con->getDefaultCenterAndEdge())
            if ((frCoord) reqDistSquare
                == std::max(lef58conSpc.first, lef58conSpc.second))
              center2center = true;
          if (lef58con->getDefaultCenterToCenter())
            center2center = true;
          reqDistSquare *= reqDistSquare;
          if (center2center)
            currDistSquare = c2cSquare;
          else
            currDistSquare = distSquare;
          if (currDistSquare < reqDistSquare)
            hasViol = true;
        }

        if (hasViol) {
          switch (type) {
            case subRouteShape:
              gridGraph_.subRouteShapeCostVia(i, j, z);  // safe access
              break;
            case addRouteShape:
              gridGraph_.addRouteShapeCostVia(i, j, z);  // safe access
              break;
            case subFixedShape:
              gridGraph_.subFixedShapeCostVia(i, j, z);  // safe access
              break;
            case addFixedShape:
              gridGraph_.addFixedShapeCostVia(i, j, z);  // safe access
              break;
            default:;
          }
          break;
        }
      }
    }
  }
}

void FlexDRWorker::modInterLayerCutSpacingCost(const Rect& box,
                                               frMIdx z,
                                               ModCostType type,
                                               bool isUpperVia,
                                               bool isBlockage)
{
  auto cutLayerNum1 = gridGraph_.getLayerNum(z) + 1;
  auto cutLayerNum2 = isUpperVia ? cutLayerNum1 + 2 : cutLayerNum1 - 2;
  auto z2 = isUpperVia ? z + 1 : z - 1;
  if (cutLayerNum2 > getTech()->getTopLayerNum()
      || cutLayerNum2 < getTech()->getBottomLayerNum())
    return;
  frLayer* layer1 = getTech()->getLayer(cutLayerNum1);
  frLayer* layer2 = getTech()->getLayer(cutLayerNum2);

  frViaDef* viaDef = nullptr;
  viaDef = layer2->getDefaultViaDef();

  if (viaDef == nullptr) {
    return;
  }
  frCutSpacingConstraint* con
      = layer1->getInterLayerCutSpacing(cutLayerNum2, false);
  if (con == nullptr) {
    con = layer2->getInterLayerCutSpacing(cutLayerNum1, false);
  }
  // LEF58_SPACINGTABLE START
  frLef58CutSpacingTableConstraint* lef58con;
  std::pair<frCoord, frCoord> lef58conSpc;
  if (!isUpperVia)
    lef58con = layer1->getLef58DefaultInterCutSpcTblConstraint();
  else
    lef58con = layer2->getLef58DefaultInterCutSpcTblConstraint();

  if (lef58con != nullptr) {
    auto dbRule = lef58con->getODBRule();
    if (!isUpperVia && dbRule->getSecondLayer()->getName() != layer2->getName())
      lef58con = nullptr;
    if (isUpperVia && dbRule->getSecondLayer()->getName() != layer1->getName())
      lef58con = nullptr;
  }
  // LEF58_SPACINGTABLE END
  if (con == nullptr && lef58con == nullptr)
    return;

  // obj1 = curr obj
  // obj2 = other obj
  // default via dimension
  frVia via(viaDef);
  Rect viaBox = via.getCutBBox();

  // spacing value needed
  frCoord bloatDist = 0;
  if (con != nullptr)
    bloatDist = con->getCutSpacing();
  if (lef58con != nullptr) {
    lef58conSpc = lef58con->getDefaultSpacing();
    bloatDist
        = std::max(bloatDist, std::max(lef58conSpc.first, lef58conSpc.second));
  }

  FlexMazeIdx mIdx1;
  FlexMazeIdx mIdx2;
  // assumes width always > 2
  Rect bx(box.xMin() - bloatDist - (viaBox.xMax() - 0) + 1,
          box.yMin() - bloatDist - (viaBox.yMax() - 0) + 1,
          box.xMax() + bloatDist + (0 - viaBox.xMin()) - 1,
          box.yMax() + bloatDist + (0 - viaBox.yMin()) - 1);
  gridGraph_.getIdxBox(mIdx1, mIdx2, bx);

  Point pt;
  frSquaredDistance distSquare = 0;
  frSquaredDistance c2cSquare = 0;
  frCoord prl, dx, dy;
  dbTransform xform;
  frSquaredDistance reqDistSquare = 0;
  Point boxCenter, tmpBxCenter;
  boxCenter = {(box.xMin() + box.xMax()) / 2, (box.yMin() + box.yMax()) / 2};
  frSquaredDistance currDistSquare = 0;
  bool hasViol = false;
  for (int i = mIdx1.x(); i <= mIdx2.x(); i++) {
    for (int j = mIdx1.y(); j <= mIdx2.y(); j++) {
      for (auto& uFig : via.getViaDef()->getCutFigs()) {
        auto obj = static_cast<frRect*>(uFig.get());
        gridGraph_.getPoint(pt, i, j);
        xform.setOffset(pt);
        Rect tmpBx = obj->getBBox();
        xform.apply(tmpBx);
        tmpBxCenter = {(tmpBx.xMin() + tmpBx.xMax()) / 2,
                       (tmpBx.yMin() + tmpBx.yMax()) / 2};
        distSquare = box2boxDistSquareNew(box, tmpBx, dx, dy);
        c2cSquare = Point::squaredDistance(boxCenter, tmpBxCenter);
        prl = max(-dx, -dy);
        hasViol = false;
        if (con != nullptr) {
          reqDistSquare = con->getCutSpacing();
          reqDistSquare *= reqDistSquare;
          currDistSquare = con->hasCenterToCenter() ? c2cSquare : distSquare;
          if (currDistSquare < reqDistSquare)
            hasViol = true;
        }
        if (!hasViol && lef58con != nullptr) {
          bool center2center = false;
          if (prl > 0)
            reqDistSquare = lef58conSpc.second;
          else
            reqDistSquare = lef58conSpc.first;
          if (lef58con->getDefaultCenterAndEdge())
            if ((frCoord) reqDistSquare
                == std::max(lef58conSpc.first, lef58conSpc.second))
              center2center = true;
          if (lef58con->getDefaultCenterToCenter())
            center2center = true;
          reqDistSquare *= reqDistSquare;
          currDistSquare = center2center ? c2cSquare : distSquare;
          if (currDistSquare < reqDistSquare)
            hasViol = true;
        }

        if (hasViol) {
          switch (type) {
            case subRouteShape:
              gridGraph_.subRouteShapeCostVia(i, j, z2);  // safe access
              break;
            case addRouteShape:
              gridGraph_.addRouteShapeCostVia(i, j, z2);  // safe access
              break;
            case subFixedShape:
              gridGraph_.subFixedShapeCostVia(i, j, z2);  // safe access
              break;
            case addFixedShape:
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

void FlexDRWorker::addPathCost(drConnFig* connFig, bool modEol, bool modCutSpc)
{
  modPathCost(connFig, ModCostType::addRouteShape, modEol, modCutSpc);
}

void FlexDRWorker::subPathCost(drConnFig* connFig, bool modEol, bool modCutSpc)
{
  modPathCost(connFig, ModCostType::subRouteShape, modEol, modCutSpc);
}

void FlexDRWorker::modPathCost(drConnFig* connFig,
                               ModCostType type,
                               bool modEol,
                               bool modCutSpc)
{
  frNonDefaultRule* ndr = nullptr;
  if (connFig->typeId() == drcPathSeg) {
    auto obj = static_cast<drPathSeg*>(connFig);
    auto [bi, ei] = obj->getMazeIdx();
    // new
    Rect box = obj->getBBox();
    ndr = !obj->isTapered() ? connFig->getNet()->getFrNet()->getNondefaultRule()
                            : nullptr;
    modMinSpacingCostPlanar(box, bi.z(), type, false, ndr);
    modMinSpacingCostVia(box, bi.z(), type, true, true, false, ndr);
    modMinSpacingCostVia(box, bi.z(), type, false, true, false, ndr);
    modViaForbiddenThrough(bi, ei, type);
    if (modEol) {
      // wrong way wire cannot have eol problem: (1) with via at end, then via
      // will add eol cost; (2) with pref-dir wire, then not eol edge
      bool isHLayer
          = (getTech()->getLayer(gridGraph_.getLayerNum(bi.z()))->getDir()
             == dbTechLayerDir::HORIZONTAL);
      if (isHLayer == (bi.y() == ei.y())) {
        modEolSpacingRulesCost(box, bi.z(), type, false, ndr);
      }
    }
  } else if (connFig->typeId() == drcPatchWire) {
    auto obj = static_cast<drPatchWire*>(connFig);
    frMIdx zIdx = gridGraph_.getMazeZIdx(obj->getLayerNum());
    Rect box = obj->getBBox();
    ndr = connFig->getNet()->getFrNet()->getNondefaultRule();
    modMinSpacingCostPlanar(box, zIdx, type, false, ndr);
    modMinSpacingCostVia(box, zIdx, type, true, true, false, ndr);
    modMinSpacingCostVia(box, zIdx, type, false, true, false, ndr);
    if (modEol)
      modEolSpacingRulesCost(box, zIdx, type);
  } else if (connFig->typeId() == drcVia) {
    auto obj = static_cast<drVia*>(connFig);
    auto [bi, ei] = obj->getMazeIdx();
    // new

    // assumes enclosure for via is always rectangle
    Rect box = obj->getLayer1BBox();
    ndr = connFig->getNet()->getFrNet()->getNondefaultRule();
    modMinSpacingCostPlanar(box, bi.z(), type, false, ndr);
    modMinSpacingCostVia(box, bi.z(), type, true, false, false, ndr);
    modMinSpacingCostVia(box, bi.z(), type, false, false, false, ndr);
    if (modEol)
      modEolSpacingRulesCost(box, bi.z(), type, false, ndr);

    // assumes enclosure for via is always rectangle
    box = obj->getLayer2BBox();

    modMinSpacingCostPlanar(box, ei.z(), type, false, ndr);
    modMinSpacingCostVia(box, ei.z(), type, true, false, false, ndr);
    modMinSpacingCostVia(box, ei.z(), type, false, false, false, ndr);
    if (modEol)
      modEolSpacingRulesCost(box, ei.z(), type, false, ndr);

    dbTransform xform;
    Point pt = obj->getOrigin();
    xform.setOffset(pt);
    for (auto& uFig : obj->getViaDef()->getCutFigs()) {
      auto rect = static_cast<frRect*>(uFig.get());
      box = rect->getBBox();
      xform.apply(box);
      if (modCutSpc)
        modCutSpacingCost(box, bi.z(), type);
      modInterLayerCutSpacingCost(box, bi.z(), type, true);
      modInterLayerCutSpacingCost(box, bi.z(), type, false);
    }
  }
}

bool FlexDRWorker::mazeIterInit_sortRerouteNets(int mazeIter,
                                                vector<drNet*>& rerouteNets)
{
  auto rerouteNetsComp = [](drNet* const& a, drNet* const& b) {
    if (a->getPriority() > b->getPriority())
      return true;
    if (a->getPriority() < b->getPriority())
      return false;
    if (a->getFrNet()->getAbsPriorityLvl() > b->getFrNet()->getAbsPriorityLvl())
      return true;
    if (a->getFrNet()->getAbsPriorityLvl() < b->getFrNet()->getAbsPriorityLvl())
      return false;
    Rect boxA = a->getPinBox();
    Rect boxB = b->getPinBox();
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

bool FlexDRWorker::mazeIterInit_sortRerouteQueue(
    int mazeIter,
    vector<RouteQueueEntry>& rerouteNets)
{
  auto rerouteNetsComp
      = [](RouteQueueEntry const& a, RouteQueueEntry const& b) {
          auto block1 = a.block;
          auto block2 = b.block;
          if (block1->typeId() == block2->typeId())
            return block1->getId() < block2->getId();
          else
            return block1->typeId() < block2->typeId();
        };
  // sort
  if (mazeIter == 0) {
    sort(rerouteNets.begin(), rerouteNets.end(), rerouteNetsComp);
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

void FlexDRWorker::route_queue()
{
  queue<RouteQueueEntry> rerouteQueue;

  if (needRecheck_) {
    gcWorker_->main();
    setMarkers(gcWorker_->getMarkers());
  }
  if (getDRIter() >= beginDebugIter) {
    logger_->info(DRT,
                  2001,
                  "Starting worker ({} {}) ({} {}) with {} markers",
                  getRouteBox().ll().x(),
                  getRouteBox().ll().y(),
                  getRouteBox().ur().x(),
                  getRouteBox().ur().y(),
                  markers_.size());
    for (auto& marker : markers_) {
      cout << marker << "\n";
    }
    if (needRecheck_)
      cout << "(Needs recheck)\n";
  }

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
  gcWorker_->resetTargetNet();
  gcWorker_->setEnableSurgicalFix(true);
  gcWorker_->main();
  // write back GC patches
  for (auto& pwire : gcWorker_->getPWires()) {
    auto net = pwire->getNet();
    if (!net) {
      cout << "Error: pwire with no net\n";
      exit(1);
    }
    net->setModified(true);
    auto tmpPWire = make_unique<drPatchWire>();
    tmpPWire->setLayerNum(pwire->getLayerNum());
    Point origin = pwire->getOrigin();
    tmpPWire->setOrigin(origin);
    Rect box = pwire->getOffsetBox();
    tmpPWire->setOffsetBox(box);
    tmpPWire->addToNet(net);
    unique_ptr<drConnFig> tmp(std::move(tmpPWire));
    auto& workerRegionQuery = getWorkerRegionQuery();
    workerRegionQuery.add(tmp.get());
    net->addRoute(std::move(tmp));
  }

  gcWorker_->end();

  setMarkers(gcWorker_->getMarkers());

  for (auto& net : nets_) {
    net->setBestRouteConnFigs();
  }
  setBestMarkers();
  if (graphics_) {
    graphics_->endWorker(drIter_);
    graphics_->show(true);
  }

  if (getDRIter() >= 7 && getDRIter() <= 30)
    identifyCongestionLevel();
}

void FlexDRWorker::identifyCongestionLevel()
{
  vector<drNet*> bpNets;
  for (auto& uNet : nets_) {
    drNet* net = uNet.get();
    bool bpLow = false, bpHigh = false;
    for (auto& uPin : net->getPins()) {
      drPin* pin = uPin.get();
      if (pin->hasFrTerm())
        continue;
      for (auto& uAP : pin->getAccessPatterns()) {
        drAccessPattern* ap = uAP.get();
        if (design_->isVerticalLayer(ap->getBeginLayerNum())) {
          if (ap->getPoint().y() == getRouteBox().yMin())
            bpLow = true;
          else if (ap->getPoint().y() == getRouteBox().yMax())
            bpHigh = true;
        } else {
          if (ap->getPoint().x() == getRouteBox().xMin())
            bpLow = true;
          else if (ap->getPoint().x() == getRouteBox().xMax())
            bpHigh = true;
        }
      }
    }
    if (bpLow && bpHigh)
      bpNets.push_back(net);
  }
  vector<int> nLowBorderCross(gridGraph_.getLayerCount(), 0);
  vector<int> nHighBorderCross(gridGraph_.getLayerCount(), 0);
  for (drNet* net : bpNets) {
    for (auto& uPin : net->getPins()) {
      drPin* pin = uPin.get();
      if (pin->hasFrTerm())
        continue;
      for (auto& uAP : pin->getAccessPatterns()) {
        drAccessPattern* ap = uAP.get();
        frMIdx z = gridGraph_.getMazeZIdx(ap->getBeginLayerNum());
        if (z < 4)
          continue;
        if (design_->isVerticalLayer(ap->getBeginLayerNum())) {
          if (ap->getPoint().y() == getRouteBox().yMin())
            nLowBorderCross[z]++;
          else if (ap->getPoint().y() == getRouteBox().yMax())
            nHighBorderCross[z]++;
        } else {
          if (ap->getPoint().x() == getRouteBox().xMin())
            nLowBorderCross[z]++;
          else if (ap->getPoint().x() == getRouteBox().xMax())
            nHighBorderCross[z]++;
        }
      }
    }
  }
  for (int z = 0; z < gridGraph_.getLayerCount(); z++) {
    frLayerNum lNum = gridGraph_.getLayerNum(z);
    auto& trackPatterns = design_->getTopBlock()->getTrackPatterns(lNum);
    frTrackPattern* tp = nullptr;
    int workerSize;
    if (design_->isHorizontalLayer(lNum)) {
      for (auto& utp : trackPatterns) {
        if (!utp->isHorizontal())  // reminder: trackPattern isHorizontal means
                                   // vertical tracks
          tp = utp.get();
      }
      workerSize = getRouteBox().dy();
    } else {
      for (auto& utp : trackPatterns) {
        if (utp->isHorizontal())
          tp = utp.get();
      }
      workerSize = getRouteBox().dx();
    }
    if (tp == nullptr)
      continue;
    int nTracks = workerSize / tp->getTrackSpacing();  // 1 track error margin
    float congestionFactorLow = nLowBorderCross[z] / (float) nTracks;
    float congestionFactorHigh = nHighBorderCross[z] / (float) nTracks;
    float finalFactor = max(congestionFactorLow, congestionFactorHigh);
    if (finalFactor >= CONGESTION_THRESHOLD) {
      isCongested_ = true;
      return;
    }
  }
}

void FlexDRWorker::route_queue_main(queue<RouteQueueEntry>& rerouteQueue)
{
  auto& workerRegionQuery = getWorkerRegionQuery();
  while (!rerouteQueue.empty()) {
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
      if (graphics_)
        graphics_->startNet(net);
      for (auto& uConnFig : net->getRouteConnFigs()) {
        subPathCost(uConnFig.get());
        workerRegionQuery.remove(uConnFig.get());  // worker region query
      }
      modEolCosts_poly(gcWorker_->getNet(net->getFrNet()),
                       ModCostType::subRouteShape);
      // route_queue need to unreserve via access if all nets are ripped up
      // (i.e., not routed) see route_queue_init_queue this
      // is unreserve via via is reserved only when drWorker starts from nothing
      // and via is reserved
      if (net->getNumReroutes() == 0 && getRipupMode() == RipUpMode::ALL) {
        initMazeCost_via_helper(net, false);
      }
      net->clear();
      if (getDRIter() >= beginDebugIter)
        logger_->info(DRT, 2002, "Routing net {}", net->getFrNet()->getName());
      // route
      mazeNetInit(net);
      bool isRouted = routeNet(net);
      if (isRouted == false) {
        if (OUT_MAZE_FILE == string("")) {
          if (VERBOSE > 0) {
            cout << "Warning: no output maze log specified, skipped writing "
                    "maze log"
                 << endl;
          }
        } else {
          gridGraph_.print();
        }
        if (graphics_) {
          graphics_->show(false);
        }
        // TODO Rect can't be logged directly
        stringstream routeBoxStringStream;
        routeBoxStringStream << getRouteBox();
        logger_->error(DRT,
                       255,
                       "Maze Route cannot find path of net {} in "
                       "worker of routeBox {}.",
                       net->getFrNet()->getName(),
                       routeBoxStringStream.str());
      }
      if (graphics_) {
        graphics_->midNet(net);
      }
      mazeNetEnd(net);
      net->addNumReroutes();
      didRoute = true;
      // gc
      if (gcWorker_->setTargetNet(net->getFrNet())) {
        gcWorker_->updateDRNet(net);
        gcWorker_->setEnableSurgicalFix(true);
        gcWorker_->main();
        modEolCosts_poly(gcWorker_->getTargetNet(), ModCostType::addRouteShape);
        // write back GC patches
        drNet* currNet = net;
        for (auto& pwire : gcWorker_->getPWires()) {
          auto net = pwire->getNet();
          if (!net)
            net = currNet;
          auto tmpPWire = make_unique<drPatchWire>();
          tmpPWire->setLayerNum(pwire->getLayerNum());
          Point origin = pwire->getOrigin();
          tmpPWire->setOrigin(origin);
          Rect box = pwire->getOffsetBox();
          tmpPWire->setOffsetBox(box);
          tmpPWire->addToNet(net);
          pwire->addToNet(net);

          unique_ptr<drConnFig> tmp(std::move(tmpPWire));
          auto& workerRegionQuery = getWorkerRegionQuery();
          workerRegionQuery.add(tmp.get());
          net->addRoute(std::move(tmp));
        }
        gcWorker_->clearPWires();
        if (getDRIter() >= beginDebugIter
            && !getGCWorker()->getMarkers().empty()) {
          logger_->info(DRT,
                        2003,
                        "Ending net {} with markers:",
                        net->getFrNet()->getName());
          for (auto& marker : getGCWorker()->getMarkers()) {
            cout << *marker << "\n";
          }
        }
        didCheck = true;
      } else {
        logger_->error(DRT, 1006, "failed to setTargetNet");
      }
    } else {
      gcWorker_->setEnableSurgicalFix(false);
      if (obj->typeId() == frcNet) {
        auto net = static_cast<frNet*>(obj);
        if (gcWorker_->setTargetNet(net)) {
          gcWorker_->main();
          didCheck = true;
        }
      } else {
        if (gcWorker_->setTargetNet(obj)) {
          gcWorker_->main();
          didCheck = true;
        }
      }
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

void FlexDRWorker::modEolCosts_poly(gcPin* shape,
                                    frLayer* layer,
                                    ModCostType modType)
{
  auto eol = layer->getDrEolSpacingConstraint();
  if (eol.eolSpace == 0)
    return;
  for (auto& edges : shape->getPolygonEdges()) {
    for (auto& edge : edges) {
      if (edge->length() >= eol.eolWidth)
        continue;
      frCoord low, high, line;
      bool innerDirIsIncreasing;  // x: increases to the east, y: increases to
                                  // the north
      if (edge->isVertical()) {
        low = min(edge->low().y(), edge->high().y());
        high = max(edge->low().y(), edge->high().y());
        line = edge->low().x();
        innerDirIsIncreasing = edge->getInnerDir() == frDirEnum::E;
      } else {
        low = min(edge->low().x(), edge->high().x());
        high = max(edge->low().x(), edge->high().x());
        line = edge->low().y();
        innerDirIsIncreasing = edge->getInnerDir() == frDirEnum::N;
      }
      modEolCost(low,
                 high,
                 line,
                 edge->isVertical(),
                 innerDirIsIncreasing,
                 layer,
                 modType);
    }
  }
}
// mods eol cost for an eol edge
void FlexDRWorker::modEolCost(frCoord low,
                              frCoord high,
                              frCoord line,
                              bool isVertical,
                              bool innerDirIsIncreasing,
                              frLayer* layer,
                              ModCostType modType)
{
  Rect testBox;
  auto eol = layer->getDrEolSpacingConstraint();
  if (isVertical) {
    if (innerDirIsIncreasing)
      testBox.init(
          line - eol.eolSpace, low - eol.eolWithin, line, high + eol.eolWithin);
    else
      testBox.init(
          line, low - eol.eolWithin, line + eol.eolSpace, high + eol.eolWithin);
  } else {
    if (innerDirIsIncreasing)
      testBox.init(
          low - eol.eolWithin, line - eol.eolSpace, high + eol.eolWithin, line);
    else
      testBox.init(
          low - eol.eolWithin, line, high + eol.eolWithin, line + eol.eolSpace);
  }
  frMIdx z = gridGraph_.getMazeZIdx(layer->getLayerNum());
  modEolSpacingCost_helper(testBox, z, modType, 0);
  modEolSpacingCost_helper(testBox, z, modType, 1);
  modEolSpacingCost_helper(testBox, z, modType, 2);
}

void FlexDRWorker::cleanUnneededPatches_poly(gcNet* drcNet, drNet* net)
{
  vector<vector<float>> areaMap(getTech()->getTopLayerNum() + 1);
  vector<drPatchWire*> patchesToRemove;
  for (auto& shape : net->getRouteConnFigs()) {
    if (shape->typeId() != frBlockObjectEnum::drcPatchWire)
      continue;
    drPatchWire* patch = static_cast<drPatchWire*>(shape.get());
    gtl::point_data<frCoord> pt(patch->getOrigin().x(), patch->getOrigin().y());
    frLayerNum lNum = patch->getLayerNum();
    frCoord minArea
        = getTech()->getLayer(lNum)->getAreaConstraint()->getMinArea();
    if (areaMap[lNum].empty())
      areaMap[lNum].assign(drcNet->getPins(lNum).size(), -1);
    for (int i = 0; i < drcNet->getPins(lNum).size(); i++) {
      auto& pin = drcNet->getPins(lNum)[i];
      if (!gtl::contains(*pin->getPolygon(), pt))
        continue;
      frCoord area;
      if (areaMap[lNum][i] == -1)
        areaMap[lNum][i] = gtl::area(*pin->getPolygon());
      area = areaMap[lNum][i];
      if (area - patch->getOffsetBox().area() >= minArea) {
        patchesToRemove.push_back(patch);
        areaMap[lNum][i] -= patch->getOffsetBox().area();
      }
    }
  }
  for (auto patch : patchesToRemove) {
    getWorkerRegionQuery().remove(patch);
    net->removeShape(patch);
  }
}

void FlexDRWorker::modEolCosts_poly(gcNet* net, ModCostType modType)
{
  for (int lNum = getTech()->getBottomLayerNum();
       lNum <= getTech()->getTopLayerNum();
       lNum++) {
    auto layer = getTech()->getLayer(lNum);
    if (layer->getType() != dbTechLayerType::ROUTING)
      continue;
    for (auto& pin : net->getPins(lNum)) {
      modEolCosts_poly(pin.get(), layer, modType);
    }
  }
}

void FlexDRWorker::routeNet_prep(
    drNet* net,
    set<drPin*, frBlockObjectComp>& unConnPins,
    map<FlexMazeIdx, set<drPin*, frBlockObjectComp>>& mazeIdx2unConnPins,
    set<FlexMazeIdx>& apMazeIdx,
    set<FlexMazeIdx>& realPinAPMazeIdx,
    map<FlexMazeIdx, frBox3D*>& mazeIdx2Tbox,
    list<pair<drPin*, frBox3D>>& pinTaperBoxes)
{
  frBox3D* tbx = nullptr;
  if (getDRIter() >= beginDebugIter)
    logger_->info(DRT, 2005, "Creating dest search points from pins:");
  for (auto& pin : net->getPins()) {
    if (getDRIter() >= beginDebugIter)
      logger_->info(DRT, 2006, "Pin {}", pin->getName());
    unConnPins.insert(pin.get());
    if (gridGraph_.getNDR()) {
      if (AUTO_TAPER_NDR_NETS
          && pin->isInstPin()) {  // create a taper box for each pin
        auto [l, h] = pin->getAPBbox();
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
        frMIdx z = l.z() == 0 ? 1 : h.z();
        pinTaperBoxes.push_back(std::make_pair(
            pin.get(), frBox3D(l.x(), l.y(), h.x(), h.y(), l.z(), z)));
        tbx = &std::prev(pinTaperBoxes.end())->second;
        for (z = tbx->zLow(); z <= tbx->zHigh();
             z++) {  // populate the map from points to taper boxes
          for (int x = tbx->xMin(); x <= tbx->xMax(); x++)
            for (int y = tbx->yMin(); y <= tbx->yMax(); y++)
              mazeIdx2Tbox[FlexMazeIdx(x, y, z)] = tbx;
        }
      }
    }
    for (auto& ap : pin->getAccessPatterns()) {
      FlexMazeIdx mi = ap->getMazeIdx();
      if (getDRIter() >= beginDebugIter) {
        logger_->info(DRT,
                      2007,
                      "({} {} {} coords: {} {} {}\n",
                      mi.x(),
                      mi.y(),
                      mi.z(),
                      ap->getPoint().x(),
                      ap->getPoint().y(),
                      ap->getBeginLayerNum());
      }
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
    Point& centerPt)
{
  frMIdx xDim, yDim, zDim;
  gridGraph_.getDim(xDim, yDim, zDim);
  ccMazeIdx1.set(xDim - 1, yDim - 1, zDim - 1);
  ccMazeIdx2.set(0, 0, 0);
  // first pin selection algorithm goes here
  // find the center point of all pins
  int totAPCnt = 0;
  frCoord totX = 0;
  frCoord totY = 0;
  frCoord totZ = 0;
  FlexMazeIdx mi;
  for (auto& pin : unConnPins) {
    for (auto& ap : pin->getAccessPatterns()) {
      mi = ap->getMazeIdx();
      Point bp = ap->getPoint();
      totX += bp.x();
      totY += bp.y();
      totZ += gridGraph_.getZHeight(mi.z());
      totAPCnt++;
      break;
    }
  }
  const frCoord centerX = totX / totAPCnt;
  const frCoord centerY = totY / totAPCnt;
  const frCoord centerZ = totZ / totAPCnt;
  centerPt = {centerX, centerY};

  // select the farmost pin from the center point as the src
  drPin* currPin = nullptr;

  frCoord currDist = 0;
  for (auto& pin : unConnPins) {
    for (auto& ap : pin->getAccessPatterns()) {
      mi = ap->getMazeIdx();
      Point bp = ap->getPoint();
      frCoord dist = abs(centerX - bp.x()) + abs(centerY - bp.y())
                     + abs(centerZ - gridGraph_.getZHeight(mi.z()));
      if (dist >= currDist) {
        currDist = dist;
        currPin = pin;
      }
    }
  }

  unConnPins.erase(currPin);
  // first pin selection algorithm ends here
  for (auto& ap : currPin->getAccessPatterns()) {
    mi = ap->getMazeIdx();
    connComps.push_back(mi);
    if (getDRIter() >= beginDebugIter) {
      logger_->info(DRT,
                    2000,
                    "({} {} {} coords: {} {} {}\n",
                    mi.x(),
                    mi.y(),
                    mi.z(),
                    ap->getPoint().x(),
                    ap->getPoint().y(),
                    ap->getBeginLayerNum());
    }
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
  Point pt;
  Point ll, ur;
  gridGraph_.getPoint(ll, ccMazeIdx1.x(), ccMazeIdx1.y());
  gridGraph_.getPoint(ur, ccMazeIdx2.x(), ccMazeIdx2.y());
  frCoord currDist = std::numeric_limits<frCoord>::max();
  drPin* nextDst = nullptr;
  // Find the next dst pin nearest to the src
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
        FlexMazeIdx mi = ap->getMazeIdx();
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
  if (points.size() == 1) {
    if (net->getFrAccessPoint(gridGraph_.xCoord(points[0].x()),
                              gridGraph_.yCoord(points[0].y()),
                              gridGraph_.getLayerNum(points[0].z())))
      addApPathSegs(points[0], net);
  }
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
        Point loc;
        frLayerNum startLayerNum = gridGraph_.getLayerNum(currZ);
        gridGraph_.getPoint(loc, startX, startY);
        FlexMazeIdx mi(startX, startY, currZ);
        auto via = getTech()->getLayer(startLayerNum + 1)->getDefaultViaDef();
        if (gridGraph_.isSVia(startX, startY, currZ)) {
          via = apSVia_.find(mi)->second->getAccessViaDef();
        }
        auto net_ndr = net->getFrNet()->getNondefaultRule();
        if (net_ndr != nullptr
            && net_ndr->getPrefVia((startLayerNum + 1) / 2)) {
          via = net_ndr->getPrefVia((startLayerNum + 1) / 2);
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
        currVia->addToNet(net);
        /*update access point (AP) connectivity info. If it is over a boundary
        pin may still be over an unseen AP (this is checked by
        checkViaConnectivity) */
        if (realPinApMazeIdx.find(mzIdxBot) != realPinApMazeIdx.end()) {
          if (!addApPathSegs(mzIdxBot, net))
            currVia->setBottomConnected(true);
        } else {
          checkViaConnectivityToAP(
              currVia.get(), true, net->getFrNet(), apMazeIdx, mzIdxBot);
        }
        if (realPinApMazeIdx.find(mzIdxTop) != realPinApMazeIdx.end()) {
          if (!addApPathSegs(mzIdxTop, net))
            currVia->setTopConnected(true);
        } else {
          checkViaConnectivityToAP(
              currVia.get(), false, net->getFrNet(), apMazeIdx, mzIdxTop);
        }
        unique_ptr<drConnFig> tmp(std::move(currVia));
        workerRegionQuery.add(tmp.get());
        net->addRoute(std::move(tmp));
        if (gridGraph_.hasRouteShapeCostAdj(
                startX, startY, currZ, frDirEnum::U)) {
          net->addMarker();
        }
      }
      // zero length
    } else if (startX == endX && startY == endY && startZ == endZ) {
      std::cout << "Warning: zero-length path in updateFlexPin\n";
    } else {
      std::cout << "Error: non-collinear path in updateFlexPin\n";
    }
  }
}
bool FlexDRWorker::addApPathSegs(const FlexMazeIdx& apIdx, drNet* net)
{
  frCoord x = gridGraph_.xCoord(apIdx.x());
  frCoord y = gridGraph_.yCoord(apIdx.y());
  frLayerNum lNum = gridGraph_.getLayerNum(apIdx.z());
  frBlockObject* owner = nullptr;
  frAccessPoint* ap = net->getFrAccessPoint(x, y, lNum, &owner);
  if (!ap)  // on-the-fly ap
    return false;
  assert(owner != nullptr);
  frInst* inst = nullptr;
  if (owner->typeId() == frBlockObjectEnum::frcInstTerm)
    inst = static_cast<frInstTerm*>(owner)->getInst();
  assert(ap != nullptr);
  if (ap->getPathSegs().empty())
    return false;
  for (auto& ps : ap->getPathSegs()) {
    unique_ptr<drPathSeg> drPs = make_unique<drPathSeg>();
    Point begin = ps.getBeginPoint();
    Point end = ps.getEndPoint();
    Point* connecting = nullptr;
    if (ps.getBeginStyle() == frEndStyle(frcTruncateEndStyle))
      connecting = &begin;
    else if (ps.getEndStyle() == frEndStyle(frcTruncateEndStyle))
      connecting = &end;
    if (inst) {
      dbTransform trans = inst->getTransform();
      trans.setOrient(dbOrientType(dbOrientType::R0));
      trans.apply(begin);
      trans.apply(end);
      if (end < begin) {  // if rotation swapped order, correct it
        if (connecting == &begin)
          connecting = &end;
        else
          connecting = &begin;
        Point tmp = begin;
        begin = end;
        end = tmp;
      }
    }
    drPs->setPoints(begin, end);
    drPs->setLayerNum(lNum);
    drPs->addToNet(net);
    auto currStyle = getTech()->getLayer(lNum)->getDefaultSegStyle();
    if (connecting == &begin)
      currStyle.setBeginStyle(frcTruncateEndStyle, 0);
    else if (connecting == &end)
      currStyle.setEndStyle(frcTruncateEndStyle, 0);

    if (net->getFrNet()->getNondefaultRule())
      drPs->setTapered(
          true);  // these tiny access pathsegs should all be tapered
    drPs->setStyle(currStyle);
    FlexMazeIdx startIdx, endIdx;
    gridGraph_.getMazeIdx(startIdx, begin, lNum);
    gridGraph_.getMazeIdx(endIdx, end, lNum);
    drPs->setMazeIdx(startIdx, endIdx);
    getWorkerRegionQuery().add(drPs.get());
    net->addRoute(std::move(drPs));
  }
  return true;
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
        midY = bx->yMax() + 1;
      } else {
        midX = bx->xMax() + 1;
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
          midY = bx->yMin() - 1;
        } else {
          midX = bx->xMin() - 1;
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
                                  bool segIsVertical,
                                  bool taper,
                                  int i,
                                  vector<FlexMazeIdx>& points,
                                  const set<FlexMazeIdx>& apMazeIdx)
{
  Point startLoc, endLoc;
  frLayerNum currLayerNum = gridGraph_.getLayerNum(z);
  gridGraph_.getPoint(startLoc, startX, startY);
  gridGraph_.getPoint(endLoc, endX, endY);
  auto currPathSeg = make_unique<drPathSeg>();
  currPathSeg->setPoints(startLoc, endLoc);
  currPathSeg->setLayerNum(currLayerNum);
  currPathSeg->addToNet(net);
  FlexMazeIdx start(startX, startY, z), end(endX, endY, z);
  auto layer = getTech()->getLayer(currLayerNum);
  auto currStyle = layer->getDefaultSegStyle();
  if (realApMazeIdx.find(start) != realApMazeIdx.end()) {
    if (!addApPathSegs(start, net))
      currStyle.setBeginStyle(frcTruncateEndStyle, 0);
  } else {
    checkPathSegStyle(currPathSeg.get(), true, currStyle, apMazeIdx, start);
  }
  if (realApMazeIdx.find(end) != realApMazeIdx.end()) {
    if (!addApPathSegs(end, net))
      currStyle.setEndStyle(frcTruncateEndStyle, 0);
  } else {
    checkPathSegStyle(currPathSeg.get(), false, currStyle, apMazeIdx, end);
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
  } else if (layer->isVertical() != segIsVertical) {  // wrong way segment
    currStyle.setWidth(layer->getWrongDirWidth());
  } else {
    editStyleExt(currStyle,
                 startX,
                 endX,
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
  int endI = segIsVertical ? endY : endX;
  for (int i = (segIsVertical ? startY : startX); i < endI; i++) {
    if ((segIsVertical
         && gridGraph_.hasRouteShapeCostAdj(startX, i, z, frDirEnum::E))
        || (!segIsVertical
            && gridGraph_.hasRouteShapeCostAdj(i, startY, z, frDirEnum::N))) {
      if (!prevHasCost) {
        net->addMarker();
        prevHasCost = true;
      }
    } else {
      prevHasCost = false;
    }
  }
}
bool FlexDRWorker::isInWorkerBorder(frCoord x, frCoord y) const
{
  return (x == getRouteBox().xMin() &&  // left
          y <= getRouteBox().yMax() && y >= getRouteBox().yMin())
         || (x == getRouteBox().xMax() &&  // right
             y <= getRouteBox().yMax() && y >= getRouteBox().yMin())
         || (y == getRouteBox().yMin() &&  // bottom
             x <= getRouteBox().xMax() && x >= getRouteBox().xMin())
         || (y == getRouteBox().yMax() &&  // top
             x <= getRouteBox().xMax() && x >= getRouteBox().xMin());
}
// checks whether the path segment is connected to an access point and update
// connectivity info (stored in frSegStyle)
void FlexDRWorker::checkPathSegStyle(drPathSeg* ps,
                                     bool isBegin,
                                     frSegStyle& style,
                                     const set<FlexMazeIdx>& apMazeIdx,
                                     const FlexMazeIdx& idx)
{
  const Point& pt = (isBegin ? ps->getBeginPoint() : ps->getEndPoint());
  if (apMazeIdx.find(idx) == apMazeIdx.end()
      && !isInWorkerBorder(pt.x(), pt.y()))
    return;
  if (hasAccessPoint(pt, ps->getLayerNum(), ps->getNet()->getFrNet())) {
    if (!addApPathSegs(idx, ps->getNet())) {
      if (isBegin)
        style.setBeginStyle(frEndStyle(frEndStyleEnum::frcTruncateEndStyle), 0);
      else
        style.setEndStyle(frEndStyle(frEndStyleEnum::frcTruncateEndStyle), 0);
    }
  }
}

bool FlexDRWorker::hasAccessPoint(const Point& pt, frLayerNum lNum, frNet* net)
{
  frRegionQuery::Objects<frBlockObject> result;
  Rect bx(pt.x(), pt.y(), pt.x(), pt.y());
  design_->getRegionQuery()->query(bx, lNum, result);
  for (auto& rqObj : result) {
    switch (rqObj.second->typeId()) {
      case frcInstTerm: {
        auto instTerm = static_cast<frInstTerm*>(rqObj.second);
        if (instTerm->getNet() == net
            && instTerm->hasAccessPoint(pt.x(), pt.y(), lNum))
          return true;
        break;
      }
      case frcBTerm: {
        auto term = static_cast<frBTerm*>(rqObj.second);
        if (term->getNet() == net
            && term->hasAccessPoint(pt.x(), pt.y(), lNum, 0))
          return true;
        break;
      }
      default:
        break;
    }
  }
  return false;
}
// checks whether the via is connected to an access point and update
// connectivity info
void FlexDRWorker::checkViaConnectivityToAP(drVia* via,
                                            bool isBottom,
                                            frNet* net,
                                            const set<FlexMazeIdx>& apMazeIdx,
                                            const FlexMazeIdx& idx)
{
  if (apMazeIdx.find(idx) == apMazeIdx.end()
      && !isInWorkerBorder(via->getOrigin().x(), via->getOrigin().y()))
    return;
  if (isBottom) {
    if (hasAccessPoint(
            via->getOrigin(), via->getViaDef()->getLayer1Num(), net)) {
      if (!addApPathSegs(idx, via->getNet()))
        via->setBottomConnected(true);
    }
  } else {
    if (hasAccessPoint(
            via->getOrigin(), via->getViaDef()->getLayer2Num(), net)) {
      if (!addApPathSegs(idx, via->getNet()))
        via->setTopConnected(true);
    }
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

inline bool segmentIsOrthogonal(FlexMazeIdx* idx,
                                frMIdx z,
                                frMIdx x,
                                bool isVertical)
{
  if (idx == nullptr)
    return false;
  bool seg_is_vertical = idx->x() == x;
  return idx->z() == z && (isVertical != seg_is_vertical);
}

void FlexDRWorker::editStyleExt(frSegStyle& currStyle,
                                frMIdx startX,
                                frMIdx endX,
                                frMIdx z,
                                FlexMazeIdx* prev,
                                FlexMazeIdx* next)
{
  auto layer = getTech()->getLayer(gridGraph_.getLayerNum(z));
  if (layer->getWrongDirWidth() >= layer->getWidth())
    return;
  bool is_vertical = startX == endX;
  if (layer->isVertical() != is_vertical)
    return;
  if (segmentIsOrthogonal(next, z, endX, is_vertical)
      && currStyle.getEndStyle() == frcExtendEndStyle) {
    currStyle.setEndExt(layer->getWrongDirWidth() / 2);
  }
  if (segmentIsOrthogonal(prev, z, startX, is_vertical)
      && currStyle.getBeginStyle() == frcExtendEndStyle) {
    currStyle.setBeginExt(layer->getWrongDirWidth() / 2);
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
  for (auto& connFig : net->getRouteConnFigs()) {
    addPathCost(connFig.get());
  }
}

void FlexDRWorker::routeNet_AddCutSpcCost(vector<FlexMazeIdx>& path)
{
  if (path.size() <= 1)
    return;
  for (unsigned long i = 1; i < path.size(); i++) {
    if (path[i].z() != path[i - 1].z()) {
      frMIdx z = min(path[i].z(), path[i - 1].z());
      frViaDef* viaDef = design_->getTech()
                             ->getLayer(gridGraph_.getLayerNum(z) + 1)
                             ->getDefaultViaDef();
      int x = gridGraph_.xCoord(path[i].x());
      int y = gridGraph_.yCoord(path[i].y());
      dbTransform xform(Point(x, y));
      for (auto& uFig : viaDef->getCutFigs()) {
        auto rect = static_cast<frRect*>(uFig.get());
        Rect box = rect->getBBox();
        xform.apply(box);
        modCutSpacingCost(box,
                          z,
                          ModCostType::addRouteShape,
                          false,
                          path[i].x(),
                          path[i].y());
      }
    }
  }
}

void FlexDRWorker::routeNet_prepAreaMap(drNet* net,
                                        map<FlexMazeIdx, frCoord>& areaMap)
{
  for (auto& pin : net->getPins()) {
    for (auto& ap : pin->getAccessPatterns()) {
      FlexMazeIdx mIdx = ap->getMazeIdx();
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
  //  ProfileTask profile("DR:routeNet");

  if (net->getPins().size() <= 1) {
    return true;
  }
  if (graphics_)
    graphics_->show(true);
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
  Point centerPt;
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
      routeNet_AddCutSpcCost(path);
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
    if (CLEAN_PATCHES) {
      gcWorker_->setTargetNet(net->getFrNet());
      gcWorker_->updateDRNet(net);
      gcWorker_->setEnableSurgicalFix(true);
      gcWorker_->updateGCWorker();
      cleanUnneededPatches_poly(gcWorker_->getTargetNet(), net);
    }
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
  FlexMazeIdx currIdx = points[0], nextIdx;
  int i;
  int prev_i = 0;  // path start point
  bool prev_is_wire = true;
  for (i = 1; i < (int) points.size(); ++i) {
    nextIdx = points[i];
    // check minAreaViolation when change layer, or last segment
    if (nextIdx.z() != currIdx.z()) {
      layerNum = gridGraph_.getLayerNum(currIdx.z());
      minAreaConstraint = getTech()->getLayer(layerNum)->getAreaConstraint();
      frArea reqArea
          = (minAreaConstraint) ? minAreaConstraint->getMinArea() : 0;
      // add curr via enclosure
      frMIdx z = (nextIdx.z() < currIdx.z()) ? currIdx.z() - 1 : currIdx.z();
      bool isLayer1 = (nextIdx.z() < currIdx.z()) ? false : true;
      if (prev_is_wire) {
        currArea += getHalfViaEncArea(
            z, isLayer1, net->getFrNet()->getNondefaultRule());
      } else {
        currArea
            = std::max((frArea) getHalfViaEncArea(
                           z, isLayer1, net->getFrNet()->getNondefaultRule())
                           * 2,
                       currArea);
      }
      endViaHalfEncArea = getHalfViaEncArea(
          z, isLayer1, net->getFrNet()->getNondefaultRule());

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
              == dbTechLayerDir::HORIZONTAL) {
            if (points[prev_i].x() < points[prev_i + 1].x()) {
              bpPatchStyle = true;
            } else if (points[prev_i].x() > points[prev_i + 1].x()) {
              bpPatchStyle = false;
            } else {
              if (points[prev_i].x() < points[i - 1].x()) {
                bpPatchStyle = true;
              } else if (points[prev_i].x() > points[i - 1].x()) {
                bpPatchStyle = false;
              } else {
                // if fully vertical, bpPatch left and epPatch right
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
              } else if (points[i - 1].x() > points[prev_i].x()) {
                epPatchStyle = false;
              } else {
                // if fully vertical, bpPatch left and epPatch right
                epPatchStyle = true;
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
              } else if (points[prev_i].y() > points[i - 1].y()) {
                bpPatchStyle = false;
              } else {
                // if fully horizontal, bpPatch left and epPatch right
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
              } else if (points[i - 1].y() > points[prev_i].y()) {
                epPatchStyle = false;
              } else {
                // if fully horizontal, bpPatch left and epPatch right
                epPatchStyle = true;
              }
            }
          }
        }
        auto patchWidth = getTech()->getLayer(layerNum)->getWidth();
        routeNet_postAstarAddPatchMetal(
            net, bp, ep, gapArea, patchWidth, bpPatchStyle, epPatchStyle);
      }
      // init for next path
      if (nextIdx.z() < currIdx.z()) {
        // get the bottom layer box of the current via to initialize the area
        // for the next shape
        currArea
            = getHalfViaEncArea(
                  currIdx.z() - 1, true, net->getFrNet()->getNondefaultRule())
              * 2;
        startViaHalfEncArea = getHalfViaEncArea(
            currIdx.z() - 1, true, net->getFrNet()->getNondefaultRule());
      } else {
        // get the top layer box of the current via to initialize the area
        // for the next shape
        currArea = getHalfViaEncArea(
                       currIdx.z(), false, net->getFrNet()->getNondefaultRule())
                   * 2;
        startViaHalfEncArea = gridGraph_.getHalfViaEncArea(nextIdx.z(), false);
        startViaHalfEncArea = gridGraph_.getHalfViaEncArea(currIdx.z(), false);
      }
      prev_i = i;
      prev_is_wire = false;
    }
    // add the wire area
    else {
      layerNum = gridGraph_.getLayerNum(currIdx.z());
      minAreaConstraint = getTech()->getLayer(layerNum)->getAreaConstraint();
      frArea reqArea
          = (minAreaConstraint) ? minAreaConstraint->getMinArea() : 0;
      auto pathWidth = getTech()->getLayer(layerNum)->getWidth();
      Point bp, ep;
      gridGraph_.getPoint(bp, currIdx.x(), currIdx.y());
      gridGraph_.getPoint(ep, nextIdx.x(), nextIdx.y());
      frCoord pathLength = Point::manhattanDistance(bp, ep);
      if (currArea < reqArea) {
        if (!prev_is_wire) {
          currArea /= 2;
        }
        currArea += pathLength * pathWidth;
      }
      prev_is_wire = true;
    }
    currIdx = nextIdx;
  }
  // add boundary area for last segment
  if (ENABLE_BOUNDARY_MAR_FIX) {
    layerNum = gridGraph_.getLayerNum(currIdx.z());
    minAreaConstraint = getTech()->getLayer(layerNum)->getAreaConstraint();
    frArea reqArea = (minAreaConstraint) ? minAreaConstraint->getMinArea() : 0;
    if (currArea < reqArea && areaMap.find(currIdx) != areaMap.end()) {
      if (!prev_is_wire) {
        currArea /= 2;
      }
      currArea += areaMap.find(currIdx)->second;
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
            == dbTechLayerDir::HORIZONTAL) {
          if (points[prev_i].x() < points[prev_i + 1].x()) {
            bpPatchStyle = true;
          } else if (points[prev_i].x() > points[prev_i + 1].x()) {
            bpPatchStyle = false;
          } else {
            if (points[prev_i].x() < points[i - 1].x()) {
              bpPatchStyle = true;
            } else if (points[prev_i].x() > points[i - 1].x()) {
              bpPatchStyle = false;
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
            } else if (points[i - 1].x() > points[prev_i].x()) {
              epPatchStyle = false;
            } else {
              epPatchStyle = true;
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
            } else if (points[prev_i].y() > points[i - 1].y()) {
              bpPatchStyle = false;
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
            } else if (points[i - 1].y() > points[prev_i].y()) {
              epPatchStyle = false;
            } else {
              epPatchStyle = true;
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

frCoord FlexDRWorker::getHalfViaEncArea(frMIdx z,
                                        bool isLayer1,
                                        frNonDefaultRule* ndr)
{
  if (!ndr || !ndr->getPrefVia(z))
    return gridGraph_.getHalfViaEncArea(z, isLayer1);
  frVia via(ndr->getPrefVia(z));
  Rect box;
  if (isLayer1)
    box = via.getLayer1BBox();
  else
    box = via.getLayer2BBox();
  return box.area() / 2;
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
  Point origin, patchEnd;
  gridGraph_.getPoint(origin, bpIdx.x(), bpIdx.y());
  frLayerNum layerNum = gridGraph_.getLayerNum(bpIdx.z());
  if (isPatchHorz) {
    if (isPatchLeft) {
      patchEnd = {origin.x() - patchLength, origin.y()};
    } else {
      patchEnd = {origin.x() + patchLength, origin.y()};
    }
  } else {
    if (isPatchLeft) {
      patchEnd = {origin.x(), origin.y() - patchLength};
    } else {
      patchEnd = {origin.x(), origin.y() + patchLength};
    }
  }
  // for wire, no need to bloat width
  Point patchLL = min(origin, patchEnd);
  Point patchUR = max(origin, patchEnd);
  if (!getRouteBox().intersects(patchEnd)) {
    cost = std::numeric_limits<int>::max();
  } else {
    FlexMazeIdx startIdx, endIdx;
    startIdx.set(0, 0, layerNum);
    endIdx.set(0, 0, layerNum);
    Rect patchBox(patchLL, patchUR);
    gridGraph_.getIdxBox(startIdx, endIdx, patchBox, FlexGridGraph::enclose);
    if (isPatchHorz) {
      // in gridgraph, the planar cost is checked for xIdx + 1
      for (auto xIdx = max(0, startIdx.x() - 1); xIdx < endIdx.x(); ++xIdx) {
        if (gridGraph_.hasRouteShapeCostAdj(
                xIdx, bpIdx.y(), bpIdx.z(), frDirEnum::E)) {
          cost += gridGraph_.getEdgeLength(
                      xIdx, bpIdx.y(), bpIdx.z(), frDirEnum::E)
                  * workerDRCCost_;
        }
        if (gridGraph_.hasFixedShapeCostAdj(
                xIdx, bpIdx.y(), bpIdx.z(), frDirEnum::E)) {
          cost += gridGraph_.getEdgeLength(
                      xIdx, bpIdx.y(), bpIdx.z(), frDirEnum::E)
                  * workerFixedShapeCost_;
        }
        if (gridGraph_.hasMarkerCostAdj(
                xIdx, bpIdx.y(), bpIdx.z(), frDirEnum::E)) {
          cost += gridGraph_.getEdgeLength(
                      xIdx, bpIdx.y(), bpIdx.z(), frDirEnum::E)
                  * workerMarkerCost_;
        }
      }
    } else {
      // in gridgraph, the planar cost is checked for yIdx + 1
      for (auto yIdx = max(0, startIdx.y() - 1); yIdx < endIdx.y(); ++yIdx) {
        if (gridGraph_.hasRouteShapeCostAdj(
                bpIdx.x(), yIdx, bpIdx.z(), frDirEnum::N)) {
          cost += gridGraph_.getEdgeLength(
                      bpIdx.x(), yIdx, bpIdx.z(), frDirEnum::N)
                  * workerDRCCost_;
        }
        if (gridGraph_.hasFixedShapeCostAdj(
                bpIdx.x(), yIdx, bpIdx.z(), frDirEnum::N)) {
          cost += gridGraph_.getEdgeLength(
                      bpIdx.x(), yIdx, bpIdx.z(), frDirEnum::N)
                  * workerFixedShapeCost_;
        }
        if (gridGraph_.hasMarkerCostAdj(
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
  Point origin;
  gridGraph_.getPoint(origin, bpIdx.x(), bpIdx.y());
  frLayerNum layerNum = gridGraph_.getLayerNum(bpIdx.z());
  // actual offsetbox
  Point patchLL, patchUR;
  if (isPatchHorz) {
    if (isPatchLeft) {
      patchLL = {0 - patchLength, 0 - patchWidth / 2};
      patchUR = {0, 0 + patchWidth / 2};
    } else {
      patchLL = {0, 0 - patchWidth / 2};
      patchUR = {0 + patchLength, 0 + patchWidth / 2};
    }
  } else {
    if (isPatchLeft) {
      patchLL = {0 - patchWidth / 2, 0 - patchLength};
      patchUR = {0 + patchWidth / 2, 0};
    } else {
      patchLL = {0 - patchWidth / 2, 0};
      patchUR = {0 + patchWidth / 2, 0 + patchLength};
    }
  }

  auto tmpPatch = make_unique<drPatchWire>();
  tmpPatch->setLayerNum(layerNum);
  tmpPatch->setOrigin(origin);
  tmpPatch->setOffsetBox(Rect(patchLL, patchUR));
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
                                                   bool bpPatchLeft,
                                                   bool epPatchLeft)
{
  bool isPatchHorz;
  // bool isLeftClean = true;
  frLayerNum layerNum = gridGraph_.getLayerNum(bpIdx.z());
  frCoord patchLength = frCoord(ceil(1.0 * gapArea / patchWidth
                                     / getTech()->getManufacturingGrid()))
                        * getTech()->getManufacturingGrid();

  // always patch to pref dir
  if (getTech()->getLayer(layerNum)->getDir() == dbTechLayerDir::HORIZONTAL) {
    isPatchHorz = true;
  } else {
    isPatchHorz = false;
  }

  auto costL = routeNet_postAstarAddPathMetal_isClean(
      bpIdx, isPatchHorz, bpPatchLeft, patchLength);
  auto costR = routeNet_postAstarAddPathMetal_isClean(
      epIdx, isPatchHorz, epPatchLeft, patchLength);
  if (costL <= costR) {
    routeNet_postAstarAddPatchMetal_addPWire(
        net, bpIdx, isPatchHorz, bpPatchLeft, patchLength, patchWidth);
  } else {
    routeNet_postAstarAddPatchMetal_addPWire(
        net, epIdx, isPatchHorz, epPatchLeft, patchLength, patchWidth);
  }
}
