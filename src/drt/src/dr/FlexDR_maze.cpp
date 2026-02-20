// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <queue>
#include <random>
#include <set>
#include <sstream>
#include <tuple>
#include <utility>
#include <vector>

#include "boost/polygon/polygon.hpp"
#include "db/drObj/drFig.h"
#include "db/drObj/drShape.h"
#include "db/gcObj/gcNet.h"
#include "db/gcObj/gcPin.h"
#include "db/infra/frBox.h"
#include "db/infra/frPoint.h"
#include "db/infra/frSegStyle.h"
#include "db/obj/frAccess.h"
#include "db/obj/frBTerm.h"
#include "db/obj/frBlockObject.h"
#include "db/obj/frInst.h"
#include "db/obj/frTrackPattern.h"
#include "db/obj/frVia.h"
#include "db/tech/frConstraint.h"
#include "db/tech/frViaDef.h"
#include "dr/FlexDR.h"
#include "dr/FlexMazeTypes.h"
#include "frBaseTypes.h"
#include "frDesign.h"
#include "frProfileTask.h"
#include "frRegionQuery.h"
#include "gc/FlexGC.h"
#include "odb/dbTransform.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "utl/Logger.h"

using odb::dbTechLayerDir;
using odb::dbTechLayerType;

namespace drt {

namespace gtl = boost::polygon;

const int beginDebugIter = std::numeric_limits<int>::max();
static frSquaredDistance pt2boxDistSquare(const odb::Point& pt,
                                          const odb::Rect& box)

{
  frCoord dx = std::max({box.xMin() - pt.x(), pt.x() - box.xMax(), 0});
  frCoord dy = std::max({box.yMin() - pt.y(), pt.y() - box.yMax(), 0});
  return (frSquaredDistance) dx * dx + (frSquaredDistance) dy * dy;
}

// prlx = -dx, prly = -dy
// dx > 0 : disjoint in x; dx = 0 : touching in x; dx < 0 : overlap in x
static frSquaredDistance box2boxDistSquareNew(const odb::Rect& box1,
                                              const odb::Rect& box2,
                                              frCoord& dx,
                                              frCoord& dy)
{
  dx = std::max(box1.xMin(), box2.xMin()) - std::min(box1.xMax(), box2.xMax());
  dy = std::max(box1.yMin(), box2.yMin()) - std::min(box1.yMax(), box2.yMax());
  return (frSquaredDistance) std::max(dx, 0) * std::max(dx, 0)
         + (frSquaredDistance) std::max(dy, 0) * std::max(dy, 0);
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
        frMIdx idx = gridGraph_.getIdx(xIdx, bi.y(), bi.z() - 1);
        switch (type) {
          case subRouteShape:
            gridGraph_.subRouteShapeCostVia(idx);  // safe access
            break;
          case addRouteShape:
            gridGraph_.addRouteShapeCostVia(idx);  // safe access
            break;
          case subFixedShape:
            gridGraph_.subFixedShapeCostVia(idx);  // safe access
            break;
          case addFixedShape:
            gridGraph_.addFixedShapeCostVia(idx);  // safe access
            break;
          default:;
        }
      }

      if (isUpperViaForbidden) {
        frMIdx idx = gridGraph_.getIdx(xIdx, bi.y(), bi.z());
        switch (type) {
          case subRouteShape:
            gridGraph_.subRouteShapeCostVia(idx);  // safe access
            break;
          case addRouteShape:
            gridGraph_.addRouteShapeCostVia(idx);  // safe access
            break;
          case subFixedShape:
            gridGraph_.subFixedShapeCostVia(idx);  // safe access
            break;
          case addFixedShape:
            gridGraph_.addFixedShapeCostVia(idx);  // safe access
            break;
          default:;
        }
      }
    }
  } else {
    for (int yIdx = bi.y(); yIdx < ei.y(); yIdx++) {
      if (isLowerViaForbidden) {
        frMIdx idx = gridGraph_.getIdx(bi.x(), yIdx, bi.z() - 1);
        switch (type) {
          case subRouteShape:
            gridGraph_.subRouteShapeCostVia(idx);  // safe access
            break;
          case addRouteShape:
            gridGraph_.addRouteShapeCostVia(idx);  // safe access
            break;
          case subFixedShape:
            gridGraph_.subFixedShapeCostVia(idx);  // safe access
            break;
          case addFixedShape:
            gridGraph_.addFixedShapeCostVia(idx);  // safe access
            break;
          default:;
        }
      }

      if (isUpperViaForbidden) {
        frMIdx idx = gridGraph_.getIdx(bi.x(), yIdx, bi.z());
        switch (type) {
          case subRouteShape:
            gridGraph_.subRouteShapeCostVia(idx);  // safe access
            break;
          case addRouteShape:
            gridGraph_.addRouteShapeCostVia(idx);  // safe access
            break;
          case subFixedShape:
            gridGraph_.subFixedShapeCostVia(idx);  // safe access
            break;
          case addFixedShape:
            gridGraph_.addFixedShapeCostVia(idx);  // safe access
            break;
          default:;
        }
      }
    }
  }
}

void FlexDRWorker::modBlockedPlanar(const odb::Rect& box,
                                    frMIdx z,
                                    bool setBlock)
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

void FlexDRWorker::modBlockedVia(const odb::Rect& box, frMIdx z, bool setBlock)
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

void FlexDRWorker::modCornerToCornerSpacing_helper(const odb::Rect& box,
                                                   frMIdx z,
                                                   ModCostType type)
{
  FlexMazeIdx p1, p2;
  gridGraph_.getIdxBox(p1, p2, box, FlexGridGraph::isEnclosed);
  for (int i = p1.x(); i <= p2.x(); i++) {
    for (int j = p1.y(); j <= p2.y(); j++) {
      frMIdx idx = gridGraph_.getIdx(i, j, z);
      switch (type) {
        case subRouteShape:
          gridGraph_.subRouteShapeCostPlanar(idx);
          break;
        case addRouteShape:
          gridGraph_.addRouteShapeCostPlanar(idx);
          break;
        case subFixedShape:
          gridGraph_.subFixedShapeCostPlanar(idx);
          break;
        case addFixedShape:
          gridGraph_.addFixedShapeCostPlanar(idx);
          break;
        default:;
      }
    }
  }
}
void FlexDRWorker::modCornerToCornerSpacing(const odb::Rect& box,
                                            frMIdx z,
                                            ModCostType type)
{
  auto lNum = gridGraph_.getLayerNum(z);
  frCoord halfwidth2 = getTech()->getLayer(lNum)->getWidth() / 2;
  // spacing value needed
  frCoord bloatDist = 0;
  auto& cons = getTech()->getLayer(lNum)->getLef58CornerSpacingConstraints();
  odb::Rect bx;
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

void FlexDRWorker::modMinSpacingCostPlanar(const odb::Rect& box,
                                           frMIdx z,
                                           ModCostType type,
                                           bool isBlockage,
                                           frNonDefaultRule* ndr,
                                           bool isMacroPin,
                                           bool resetHorz,
                                           bool resetVert)
{
  // calculate costs for non-NDR nets
  frCoord default_width
      = getTech()->getLayer(gridGraph_.getLayerNum(z))->getWidth();
  frCoord default_spacing = 0;
  if (ndr) {
    default_spacing = ndr->getSpacing(z);
  }
  modMinSpacingCostPlanarHelper(box,
                                z,
                                type,
                                default_width,
                                default_spacing,
                                isBlockage,
                                isMacroPin,
                                resetHorz,
                                resetVert,
                                false);
  // caclulate costs for NDR nets
  // get all unique width,spacing pairs from ndrs
  for (auto ndr : ndrs_) {
    frCoord ndr_width = default_width;
    if (ndr->getWidth(z) != 0) {
      ndr_width = ndr->getWidth(z);
    }
    frCoord ndr_min_spc = std::max(default_spacing, ndr->getSpacing(z));
    modMinSpacingCostPlanarHelper(box,
                                  z,
                                  type,
                                  ndr_width,
                                  ndr_min_spc,
                                  isBlockage,
                                  isMacroPin,
                                  resetHorz,
                                  resetVert,
                                  true);
  }
}

void FlexDRWorker::modMinSpacingCostPlanarHelper(const odb::Rect& box,
                                                 frMIdx z,
                                                 ModCostType type,
                                                 frCoord width2,
                                                 frCoord minSpacing,
                                                 bool isBlockage,
                                                 bool isMacroPin,
                                                 bool resetHorz,
                                                 bool resetVert,
                                                 bool ndr)
{
  auto lNum = gridGraph_.getLayerNum(z);
  frCoord width1 = box.minDXDY();
  // layer default width
  frCoord halfwidth2 = width2 / 2;
  // spacing value needed
  bool use_min_spacing = isBlockage && router_cfg_->USEMINSPACING_OBS;
  frCoord bloatDist = getTech()->getLayer(lNum)->getMinSpacingValue(
      width1, width2, box.maxDXDY(), use_min_spacing);
  bloatDist = std::max(bloatDist, minSpacing);
  frSquaredDistance bloatDistSquare = bloatDist;
  bloatDistSquare *= bloatDist;

  FlexMazeIdx mIdx1, mPinLL;
  FlexMazeIdx mIdx2, mPinUR;
  // assumes width always > 2
  odb::Rect bx(box.xMin() - bloatDist - halfwidth2 + 1,
               box.yMin() - bloatDist - halfwidth2 + 1,
               box.xMax() + bloatDist + halfwidth2 - 1,
               box.yMax() + bloatDist + halfwidth2 - 1);
  gridGraph_.getIdxBox(mIdx1, mIdx2, bx);
  if (isMacroPin && type == ModCostType::resetBlocked) {
    odb::Rect sBox(box.xMin() + width2 / 2,
                   box.yMin() + width2 / 2,
                   box.xMax() - width2 / 2,
                   box.yMax() - width2 / 2);
    gridGraph_.getIdxBox(mPinLL, mPinUR, sBox);
  }
  odb::Point pt, pt1, pt2, pt3, pt4;
  frSquaredDistance distSquare = 0;
  for (int i = mIdx1.x(); i <= mIdx2.x(); i++) {
    for (int j = mIdx1.y(); j <= mIdx2.y(); j++) {
      gridGraph_.getPoint(pt, i, j);
      pt1 = {pt.x() + halfwidth2, pt.y() - halfwidth2};
      pt2 = {pt.x() + halfwidth2, pt.y() + halfwidth2};
      pt3 = {pt.x() - halfwidth2, pt.y() - halfwidth2};
      pt4 = {pt.x() - halfwidth2, pt.y() + halfwidth2};
      distSquare
          = std::min(pt2boxDistSquare(pt1, box), pt2boxDistSquare(pt2, box));
      distSquare = std::min(pt2boxDistSquare(pt3, box), distSquare);
      distSquare = std::min(pt2boxDistSquare(pt4, box), distSquare);
      if (distSquare < bloatDistSquare) {
        switch (type) {
          case subRouteShape:
            gridGraph_.subRouteShapeCostPlanar(gridGraph_.getIdx(i, j, z),
                                               ndr);  // safe access
            break;
          case addRouteShape:
            gridGraph_.addRouteShapeCostPlanar(gridGraph_.getIdx(i, j, z),
                                               ndr);  // safe access
            break;
          case subFixedShape:
            gridGraph_.subFixedShapeCostPlanar(gridGraph_.getIdx(i, j, z),
                                               ndr);  // safe access
            break;
          case addFixedShape:
            gridGraph_.addFixedShapeCostPlanar(gridGraph_.getIdx(i, j, z),
                                               ndr);  // safe access
            break;
          case resetFixedShape:
            if (resetHorz) {
              gridGraph_.setFixedShapeCostPlanarHorz(
                  gridGraph_.getIdx(i, j, z), 0, ndr);  // safe access
            }
            if (resetVert) {
              gridGraph_.setFixedShapeCostPlanarVert(
                  gridGraph_.getIdx(i, j, z), 0, ndr);  // safe access
            }
            break;
          case setFixedShape: {
            frMIdx idx = gridGraph_.getIdx(i, j, z);
            gridGraph_.setFixedShapeCostPlanarHorz(idx, 1, ndr);  // safe access
            gridGraph_.setFixedShapeCostPlanarVert(idx, 1, ndr);  // safe access
            break;
          }
          case resetBlocked:
            if (ndr) {
              return;
            }
            if (isMacroPin) {
              if (j >= mPinLL.y() && j <= mPinUR.y()) {
                gridGraph_.resetBlocked(i, j, z, frDirEnum::E);
                if (i == 0) {
                  gridGraph_.resetBlocked(i, j, z, frDirEnum::W);
                }
              }
              if (i >= mPinLL.x() && i <= mPinUR.x()) {
                gridGraph_.resetBlocked(i, j, z, frDirEnum::N);
                if (j == 0) {
                  gridGraph_.resetBlocked(i, j, z, frDirEnum::S);
                }
              }
            } else {
              gridGraph_.resetBlocked(i, j, z, frDirEnum::E);
              gridGraph_.resetBlocked(i, j, z, frDirEnum::N);
              if (i == 0) {
                gridGraph_.resetBlocked(i, j, z, frDirEnum::W);
              }
              if (j == 0) {
                gridGraph_.resetBlocked(i, j, z, frDirEnum::S);
              }
            }
            break;
          case setBlocked:  // set blocked
            if (ndr) {
              return;
            }
            gridGraph_.setBlocked(i, j, z, frDirEnum::E);
            gridGraph_.setBlocked(i, j, z, frDirEnum::N);
            if (i == 0) {
              gridGraph_.setBlocked(i, j, z, frDirEnum::W);
            }
            if (j == 0) {
              gridGraph_.setBlocked(i, j, z, frDirEnum::S);
            }
            break;
          default:;
        }
      }
    }
  }
}

void FlexDRWorker::modMinSpacingCostVia_eol_helper(const odb::Rect& box,
                                                   const odb::Rect& testBox,
                                                   ModCostType type,
                                                   frMIdx idx,
                                                   bool ndr)
{
  if (testBox.overlaps(box)) {
    switch (type) {
      case subRouteShape:
        gridGraph_.subRouteShapeCostVia(idx, ndr);
        break;
      case addRouteShape:
        gridGraph_.addRouteShapeCostVia(idx, ndr);
        break;
      case subFixedShape:
        gridGraph_.subFixedShapeCostVia(idx, ndr);  // safe access
        break;
      case addFixedShape:
        gridGraph_.addFixedShapeCostVia(idx, ndr);  // safe access
        break;
      default:;
    }
  }
}

void FlexDRWorker::modMinSpacingCostVia_eol(const odb::Rect& box,
                                            const odb::Rect& tmpBx,
                                            ModCostType type,
                                            const drEolSpacingConstraint& drCon,
                                            frMIdx idx,
                                            bool ndr)
{
  if (drCon.eolSpace == 0) {
    return;
  }
  odb::Rect testBox;
  frCoord eolSpace = drCon.eolSpace;
  frCoord eolWidth = drCon.eolWidth;
  frCoord eolWithin = drCon.eolWithin;
  // eol to up and down
  if (tmpBx.dx() <= eolWidth) {
    testBox.init(tmpBx.xMin() - eolWithin,
                 tmpBx.yMax(),
                 tmpBx.xMax() + eolWithin,
                 tmpBx.yMax() + eolSpace);
    modMinSpacingCostVia_eol_helper(box, testBox, type, idx, ndr);

    testBox.init(tmpBx.xMin() - eolWithin,
                 tmpBx.yMin() - eolSpace,
                 tmpBx.xMax() + eolWithin,
                 tmpBx.yMin());
    modMinSpacingCostVia_eol_helper(box, testBox, type, idx, ndr);
  }
  // eol to left and right
  if (tmpBx.dy() <= eolWidth) {
    testBox.init(tmpBx.xMax(),
                 tmpBx.yMin() - eolWithin,
                 tmpBx.xMax() + eolSpace,
                 tmpBx.yMax() + eolWithin);
    modMinSpacingCostVia_eol_helper(box, testBox, type, idx, ndr);

    testBox.init(tmpBx.xMin() - eolSpace,
                 tmpBx.yMin() - eolWithin,
                 tmpBx.xMin(),
                 tmpBx.yMax() + eolWithin);
    modMinSpacingCostVia_eol_helper(box, testBox, type, idx, ndr);
  }
}

void FlexDRWorker::modMinimumcutCostVia(const odb::Rect& box,
                                        frMIdx z,
                                        ModCostType type,
                                        bool isUpperVia)
{
  auto lNum = gridGraph_.getLayerNum(z);
  frCoord width1 = box.minDXDY();
  frCoord length1 = box.maxDXDY();
  // default via dimension
  const frViaDef* viaDef = nullptr;
  if (isUpperVia) {
    viaDef = (lNum < gridGraph_.getMaxLayerNum())
                 ? getTech()->getLayer(lNum + 1)->getDefaultViaDef()
                 : nullptr;
  } else {
    viaDef = (lNum > gridGraph_.getMinLayerNum())
                 ? getTech()->getLayer(lNum - 1)->getDefaultViaDef()
                 : nullptr;
  }
  if (viaDef == nullptr) {
    return;
  }
  frVia via(viaDef);
  odb::Rect viaBox(0, 0, 0, 0);
  viaBox = via.getCutBBox();

  FlexMazeIdx mIdx1, mIdx2;
  odb::Rect bx, tmpBx, sViaBox;
  odb::Point pt;
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
          odb::dbTransform xform(pt);
          tmpBx = viaBox;
          frMIdx idx = gridGraph_.getIdx(i, j, zIdx);
          if (gridGraph_.isSVia(idx)) {
            auto sViaDef = apSVia_[FlexMazeIdx(i, j, zIdx)]->getAccessViaDef();
            sVia.setViaDef(sViaDef);
            sViaBox = sVia.getCutBBox();
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
              gridGraph_.subRouteShapeCostVia(idx);  // safe access
              break;
            case addRouteShape:
              gridGraph_.addRouteShapeCostVia(idx);  // safe access
              break;
            case subFixedShape:
              gridGraph_.subFixedShapeCostVia(idx);  // safe access
              break;
            case addFixedShape:
              gridGraph_.addFixedShapeCostVia(idx);  // safe access
              break;
            case resetFixedShape:
              gridGraph_.setFixedShapeCostVia(gridGraph_.getIdx(i, j, z),
                                              0);  // safe access
              break;
            case setFixedShape:
              gridGraph_.setFixedShapeCostVia(gridGraph_.getIdx(i, j, z),
                                              1);  // safe access
            default:;
          }
        }
      }
    }
  }
}

void FlexDRWorker::modMinSpacingCostVia(const odb::Rect& box,
                                        frMIdx z,
                                        ModCostType type,
                                        bool isUpperVia,
                                        bool isCurrPs,
                                        bool isBlockage,
                                        frNonDefaultRule* ndr)
{
  // mod costs for non-NDR nets
  auto lNum = gridGraph_.getLayerNum(z);
  const frViaDef* defaultViaDef = nullptr;
  if (isUpperVia) {
    defaultViaDef = (lNum < gridGraph_.getMaxLayerNum())
                        ? getTech()->getLayer(lNum + 1)->getDefaultViaDef()
                        : nullptr;
  } else {
    defaultViaDef = (lNum > gridGraph_.getMinLayerNum())
                        ? getTech()->getLayer(lNum - 1)->getDefaultViaDef()
                        : nullptr;
  }
  frCoord defaultMinSpacing = 0;
  drEolSpacingConstraint drCon;
  if (ndr) {
    defaultMinSpacing = ndr->getSpacing(z);
    drCon = ndr->getDrEolSpacingConstraint(z);
  }
  frCoord defaultWidth = getTech()->getLayer(lNum)->getWidth();
  modMinSpacingCostViaHelper(box,
                             z,
                             type,
                             defaultWidth,
                             defaultMinSpacing,
                             defaultViaDef,
                             drCon,
                             isUpperVia,
                             isCurrPs,
                             isBlockage,
                             false);
  // mod costs for ndrs
  for (auto ndr : ndrs_) {
    frCoord width = defaultWidth;
    const frViaDef* viadef = defaultViaDef;
    frCoord minSpacing = defaultMinSpacing;
    drEolSpacingConstraint ndrDrCon = ndr->getDrEolSpacingConstraint(z);
    if (isUpperVia && lNum < gridGraph_.getMaxLayerNum()
        && ndr->getPrefVia(z) != nullptr) {
      viadef = ndr->getPrefVia(z);
    } else if (!isUpperVia && lNum > gridGraph_.getMinLayerNum()
               && ndr->getPrefVia(z - 1) != nullptr) {
      viadef = ndr->getPrefVia(z - 1);
    }
    if (ndr->getWidth(z) != 0) {
      width = ndr->getWidth(z);
    }
    minSpacing = std::max(minSpacing, ndr->getSpacing(z));
    modMinSpacingCostViaHelper(box,
                               z,
                               type,
                               width,
                               minSpacing,
                               viadef,
                               ndrDrCon,
                               isUpperVia,
                               isCurrPs,
                               isBlockage,
                               true);
  }
}

void FlexDRWorker::modMinSpacingCostViaHelper(const odb::Rect& box,
                                              frMIdx z,
                                              ModCostType type,
                                              frCoord width,
                                              frCoord minSpacing,
                                              const frViaDef* viaDef,
                                              drEolSpacingConstraint drCon,
                                              bool isUpperVia,
                                              bool isCurrPs,
                                              bool isBlockage,
                                              bool ndr)
{
  if (viaDef == nullptr) {
    return;
  }
  frVia via(viaDef);
  odb::Rect viaBox(0, 0, 0, 0);
  if (isUpperVia) {
    viaBox = via.getLayer1BBox();
  } else {
    viaBox = via.getLayer2BBox();
  }
  frCoord width2 = viaBox.minDXDY();
  frCoord length2 = viaBox.maxDXDY();

  // via prl should check min area patch metal if not fat via
  auto lNum = gridGraph_.getLayerNum(z);
  bool isH
      = (getTech()->getLayer(lNum)->getDir() == dbTechLayerDir::HORIZONTAL);
  bool isFatVia = (isH) ? (viaBox.dy() > width) : (viaBox.dx() > width);

  frCoord length2_mar = length2;
  frCoord patchLength = 0;
  if (!isFatVia) {
    auto minAreaConstraint = getTech()->getLayer(lNum)->getAreaConstraint();
    auto minArea = minAreaConstraint ? minAreaConstraint->getMinArea() : 0;
    patchLength = frCoord(ceil(1.0 * minArea / width
                               / getTech()->getManufacturingGrid()))
                  * frCoord(getTech()->getManufacturingGrid());
    length2_mar = std::max(length2_mar, patchLength);
  }

  // spacing value needed
  frCoord width1 = box.minDXDY();
  frCoord length1 = box.maxDXDY();
  frCoord prl = isCurrPs ? (length2_mar) : std::min(length1, length2_mar);
  bool use_min_spacing
      = isBlockage && router_cfg_->USEMINSPACING_OBS && !isFatVia;
  frCoord bloatDist = getTech()->getLayer(lNum)->getMinSpacingValue(
      width1, width2, prl, use_min_spacing);
  bloatDist = std::max(minSpacing, bloatDist);
  frCoord bloatDistEolX = 0;
  frCoord bloatDistEolY = 0;
  if (drCon.eolWidth == 0) {
    drCon = getTech()->getLayer(lNum)->getDrEolSpacingConstraint();
  }
  if (viaBox.dx() <= drCon.eolWidth) {
    bloatDistEolY = std::max(bloatDistEolY, drCon.eolSpace);
  }
  // eol left and right
  if (viaBox.dy() <= drCon.eolWidth) {
    bloatDistEolX = std::max(bloatDistEolX, drCon.eolSpace);
  }

  FlexMazeIdx mIdx1;
  FlexMazeIdx mIdx2;
  // assumes width always > 2
  odb::Rect bx(
      box.xMin() - std::max(bloatDist, bloatDistEolX) - (viaBox.xMax() - 0) + 1,
      box.yMin() - std::max(bloatDist, bloatDistEolY) - (viaBox.yMax() - 0) + 1,
      box.xMax() + std::max(bloatDist, bloatDistEolX) + (0 - viaBox.xMin()) - 1,
      box.yMax() + std::max(bloatDist, bloatDistEolY) + (0 - viaBox.yMin())
          - 1);
  gridGraph_.getIdxBox(mIdx1, mIdx2, bx);
  odb::Point pt;
  odb::Rect tmpBx;
  frSquaredDistance distSquare = 0;
  frCoord dx, dy;
  frVia sVia;
  frMIdx zIdx = isUpperVia ? z : z - 1;
  for (int i = mIdx1.x(); i <= mIdx2.x(); i++) {
    for (int j = mIdx1.y(); j <= mIdx2.y(); j++) {
      gridGraph_.getPoint(pt, i, j);
      odb::dbTransform xform(pt);
      tmpBx = viaBox;
      frMIdx idx = gridGraph_.getIdx(i, j, zIdx);
      if (gridGraph_.isSVia(idx)) {
        auto sViaDef = apSVia_[FlexMazeIdx(i, j, zIdx)]->getAccessViaDef();
        sVia.setViaDef(sViaDef);
        odb::Rect sViaBox;
        if (isUpperVia) {
          sViaBox = sVia.getLayer1BBox();
        } else {
          sViaBox = sVia.getLayer2BBox();
        }
        tmpBx = sViaBox;
      }
      xform.apply(tmpBx);
      distSquare = box2boxDistSquareNew(box, tmpBx, dx, dy);
      prl = std::max(-dx, -dy);
      // curr is ps
      if (isCurrPs) {
        if (-dy >= 0 && prl == -dy) {
          prl = viaBox.dy();
          // ignore svia effect here...
          if (!isH && !isFatVia) {
            prl = std::max(prl, patchLength);
          }
        } else if (-dx >= 0 && prl == -dx) {
          prl = viaBox.dx();
          if (isH && !isFatVia) {
            prl = std::max(prl, patchLength);
          }
        }
      }
      bool use_min_spacing
          = isBlockage && router_cfg_->USEMINSPACING_OBS && !isFatVia;
      frCoord reqDist = getTech()->getLayer(lNum)->getMinSpacingValue(
          width1, width2, prl, use_min_spacing);
      reqDist = std::max(reqDist, minSpacing);
      if (distSquare < (frSquaredDistance) reqDist * reqDist) {
        switch (type) {
          case subRouteShape:
            gridGraph_.subRouteShapeCostVia(idx, ndr);  // safe access
            break;
          case addRouteShape:
            gridGraph_.addRouteShapeCostVia(idx, ndr);  // safe access
            break;
          case subFixedShape:
            gridGraph_.subFixedShapeCostVia(idx, ndr);  // safe access
            break;
          case addFixedShape:
            gridGraph_.addFixedShapeCostVia(idx, ndr);  // safe access
            break;
          default:;
        }
      }
      // eol, other obj to curr obj
      modMinSpacingCostVia_eol(box, tmpBx, type, drCon, idx, ndr);
    }
  }
}

// eolType == 0: planer
// eolType == 1: down
// eolType == 2: up
void FlexDRWorker::modEolSpacingCost_helper(const odb::Rect& testbox,
                                            frMIdx z,
                                            ModCostType type,
                                            int eolType,
                                            bool resetHorz,
                                            bool resetVert)
{
  auto lNum = gridGraph_.getLayerNum(z);
  odb::Rect bx;
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
    const frViaDef* viaDef = nullptr;
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
    odb::Rect viaBox(0, 0, 0, 0);
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
  odb::Rect sViaBox;
  odb::Point pt;

  for (int i = mIdx1.x(); i <= mIdx2.x(); i++) {
    for (int j = mIdx1.y(); j <= mIdx2.y(); j++) {
      if (eolType == 0) {
        frMIdx idx = gridGraph_.getIdx(i, j, z);
        switch (type) {
          case subRouteShape:
            gridGraph_.subRouteShapeCostPlanar(idx);  // safe access
            break;
          case addRouteShape:
            gridGraph_.addRouteShapeCostPlanar(idx);  // safe access
            break;
          case subFixedShape:
            gridGraph_.subFixedShapeCostPlanar(idx);  // safe access
            break;
          case addFixedShape:
            gridGraph_.addFixedShapeCostPlanar(idx);  // safe access
            break;
          case resetFixedShape:
            if (resetHorz) {
              gridGraph_.setFixedShapeCostPlanarHorz(idx, 0);  // safe access
            }
            if (resetVert) {
              gridGraph_.setFixedShapeCostPlanarVert(idx, 0);  // safe access
            }
            break;
          case setFixedShape:
            gridGraph_.setFixedShapeCostPlanarHorz(idx, 1);  // safe access
            gridGraph_.setFixedShapeCostPlanarVert(idx, 1);  // safe access
            break;
          default:;
        }
      } else if (eolType == 1) {
        frMIdx idx = gridGraph_.getIdx(i, j, z - 1);
        if (gridGraph_.isSVia(idx)) {
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
            gridGraph_.subRouteShapeCostVia(idx);  // safe access
            break;
          case addRouteShape:
            gridGraph_.addRouteShapeCostVia(idx);  // safe access
            break;
          case subFixedShape:
            gridGraph_.subFixedShapeCostVia(idx);  // safe access
            break;
          case addFixedShape:
            gridGraph_.addFixedShapeCostVia(idx);  // safe access
            break;
          default:;
        }
      } else if (eolType == 2) {
        frMIdx idx = gridGraph_.getIdx(i, j, z);
        if (gridGraph_.isSVia(idx)) {
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
            gridGraph_.subRouteShapeCostVia(idx);  // safe access
            break;
          case addRouteShape:
            gridGraph_.addRouteShapeCostVia(idx);  // safe access
            break;
          case subFixedShape:
            gridGraph_.subFixedShapeCostVia(idx);  // safe access
            break;
          case addFixedShape:
            gridGraph_.addFixedShapeCostVia(idx);  // safe access
            break;
          default:;
        }
      }
    }
  }
}

void FlexDRWorker::modEolSpacingRulesCost(const odb::Rect& box,
                                          frMIdx z,
                                          ModCostType type,
                                          bool isSkipVia,
                                          frNonDefaultRule* ndr,
                                          bool resetHorz,
                                          bool resetVert)
{
  auto layer = getTech()->getLayer(gridGraph_.getLayerNum(z));
  drEolSpacingConstraint drCon;
  if (ndr != nullptr) {
    drCon = ndr->getDrEolSpacingConstraint(z);
  }
  if (drCon.eolWidth == 0) {
    drCon = layer->getDrEolSpacingConstraint();
  }
  frCoord eolSpace, eolWidth, eolWithin;
  eolSpace = drCon.eolSpace;
  eolWithin = drCon.eolWithin;
  eolWidth = drCon.eolWidth;
  if (eolSpace == 0) {
    return;
  }
  odb::Rect testBox;
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
                                                 const odb::Rect& origCutBox,
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

    odb::Rect viaBox = origVia->getCutBBox();

    frSquaredDistance reqDistSquare = con->getCutSpacing();
    reqDistSquare *= reqDistSquare;

    auto cutWithin = con->getCutWithin();
    odb::Rect queryBox;
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
      odb::Rect spacingBox;
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

/*inline*/ void FlexDRWorker::modCutSpacingCost(const odb::Rect& box,
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
  const frViaDef* viaDef = cutLayer->getDefaultViaDef();
  frVia via(viaDef);
  odb::Rect viaBox = via.getCutBBox();

  // spacing value needed
  frCoord bloatDist = 0;
  for (auto con : cutLayer->getCutSpacing()) {
    bloatDist = std::max(bloatDist, con->getCutSpacing());
    if (con->getAdjacentCuts() != -1 && isBlockage) {
      bloatDist = std::max(bloatDist, con->getCutWithin());
    }
  }
  frLef58CutSpacingTableConstraint* lef58con = nullptr;
  std::pair<frCoord, frCoord> lef58conSpc;
  if (cutLayer->hasLef58DiffNetCutSpcTblConstraint()) {
    lef58con = cutLayer->getLef58DiffNetCutSpcTblConstraint();
  }

  if (lef58con != nullptr) {
    lef58conSpc = lef58con->getDefaultSpacing();
    bloatDist = std::max({bloatDist, lef58conSpc.first, lef58conSpc.second});
  }

  FlexMazeIdx mIdx1;
  FlexMazeIdx mIdx2;
  // assumes width always > 2
  odb::Rect bx(box.xMin() - bloatDist - (viaBox.xMax() - 0) + 1,
               box.yMin() - bloatDist - (viaBox.yMax() - 0) + 1,
               box.xMax() + bloatDist + (0 - viaBox.xMin()) - 1,
               box.yMax() + bloatDist + (0 - viaBox.yMin()) - 1);
  gridGraph_.getIdxBox(mIdx1, mIdx2, bx);

  odb::Point pt;
  frSquaredDistance distSquare = 0;
  frSquaredDistance c2cSquare = 0;
  frCoord dx, dy, prl;
  frSquaredDistance reqDistSquare = 0;
  odb::Point boxCenter, tmpBxCenter;
  boxCenter = {(box.xMin() + box.xMax()) / 2, (box.yMin() + box.yMax()) / 2};
  frSquaredDistance currDistSquare = 0;
  bool hasViol;
  for (int i = mIdx1.x(); i <= mIdx2.x(); i++) {
    for (int j = mIdx1.y(); j <= mIdx2.y(); j++) {
      if (i == avoidI && j == avoidJ) {
        continue;
      }
      for (auto& uFig : via.getViaDef()->getCutFigs()) {
        auto obj = static_cast<frRect*>(uFig.get());
        gridGraph_.getPoint(pt, i, j);
        odb::dbTransform xform(pt);
        odb::Rect tmpBx = obj->getBBox();
        xform.apply(tmpBx);
        tmpBxCenter = {(tmpBx.xMin() + tmpBx.xMax()) / 2,
                       (tmpBx.yMin() + tmpBx.yMax()) / 2};
        distSquare = box2boxDistSquareNew(box, tmpBx, dx, dy);
        c2cSquare = odb::Point::squaredDistance(boxCenter, tmpBxCenter);
        prl = std::max(-dx, -dy);
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
            auto currArea = std::max(box.area(), tmpBx.area());
            if (currArea >= con->getCutArea()
                && currDistSquare < reqDistSquare) {
              hasViol = true;
            }
          } else if (currDistSquare < reqDistSquare) {
            hasViol = true;
          }
          if (hasViol) {
            break;
          }
        }
        if (!hasViol && lef58con != nullptr) {
          bool center2center = false;
          if (prl > 0) {
            reqDistSquare = lef58conSpc.second;
          } else {
            reqDistSquare = lef58conSpc.first;
          }
          if (lef58con->getDefaultCenterAndEdge()) {
            if ((frCoord) reqDistSquare
                == std::max(lef58conSpc.first, lef58conSpc.second)) {
              center2center = true;
            }
          }
          if (lef58con->getDefaultCenterToCenter()) {
            center2center = true;
          }
          reqDistSquare *= reqDistSquare;
          if (center2center) {
            currDistSquare = c2cSquare;
          } else {
            currDistSquare = distSquare;
          }
          if (currDistSquare < reqDistSquare) {
            hasViol = true;
          }
        }

        if (hasViol) {
          frMIdx idx = gridGraph_.getIdx(i, j, z);
          switch (type) {
            case subRouteShape:
              gridGraph_.subRouteShapeCostVia(idx);  // safe access
              break;
            case addRouteShape:
              gridGraph_.addRouteShapeCostVia(idx);  // safe access
              break;
            case subFixedShape:
              gridGraph_.subFixedShapeCostVia(idx);  // safe access
              break;
            case addFixedShape:
              gridGraph_.addFixedShapeCostVia(idx);  // safe access
              break;
            default:;
          }
          break;
        }
      }
    }
  }
}

void FlexDRWorker::modInterLayerCutSpacingCost(const odb::Rect& box,
                                               frMIdx z,
                                               ModCostType type,
                                               bool isUpperVia,
                                               bool isBlockage)
{
  auto cutLayerNum1 = gridGraph_.getLayerNum(z) + 1;
  auto cutLayerNum2 = isUpperVia ? cutLayerNum1 + 2 : cutLayerNum1 - 2;
  auto z2 = isUpperVia ? z + 1 : z - 1;
  if (cutLayerNum2 > getTech()->getTopLayerNum()
      || cutLayerNum2 < getTech()->getBottomLayerNum()) {
    return;
  }
  frLayer* layer1 = getTech()->getLayer(cutLayerNum1);
  frLayer* layer2 = getTech()->getLayer(cutLayerNum2);

  const frViaDef* viaDef = layer2->getDefaultViaDef();

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
  if (!isUpperVia) {
    lef58con = layer1->getLef58DefaultInterCutSpcTblConstraint();
  } else {
    lef58con = layer2->getLef58DefaultInterCutSpcTblConstraint();
  }

  if (lef58con != nullptr) {
    auto dbRule = lef58con->getODBRule();
    if (!isUpperVia
        && dbRule->getSecondLayer()->getName() != layer2->getName()) {
      lef58con = nullptr;
    }
    if (isUpperVia
        && dbRule->getSecondLayer()->getName() != layer1->getName()) {
      lef58con = nullptr;
    }
  }
  // LEF58_SPACINGTABLE END
  if (con == nullptr && lef58con == nullptr) {
    return;
  }

  // obj1 = curr obj
  // obj2 = other obj
  // default via dimension
  frVia via(viaDef);
  odb::Rect viaBox = via.getCutBBox();

  // spacing value needed
  frCoord bloatDist = 0;
  if (con != nullptr) {
    bloatDist = con->getCutSpacing();
  }
  if (lef58con != nullptr) {
    lef58conSpc = lef58con->getDefaultSpacing();
    bloatDist = std::max({bloatDist, lef58conSpc.first, lef58conSpc.second});
  }

  FlexMazeIdx mIdx1;
  FlexMazeIdx mIdx2;
  // assumes width always > 2
  odb::Rect bx(box.xMin() - bloatDist - (viaBox.xMax() - 0) + 1,
               box.yMin() - bloatDist - (viaBox.yMax() - 0) + 1,
               box.xMax() + bloatDist + (0 - viaBox.xMin()) - 1,
               box.yMax() + bloatDist + (0 - viaBox.yMin()) - 1);
  gridGraph_.getIdxBox(mIdx1, mIdx2, bx);

  odb::Point pt;
  frSquaredDistance distSquare = 0;
  frSquaredDistance c2cSquare = 0;
  frCoord prl, dx, dy;
  frSquaredDistance reqDistSquare = 0;
  odb::Point boxCenter, tmpBxCenter;
  boxCenter = {(box.xMin() + box.xMax()) / 2, (box.yMin() + box.yMax()) / 2};
  frSquaredDistance currDistSquare = 0;
  bool hasViol = false;
  for (int i = mIdx1.x(); i <= mIdx2.x(); i++) {
    for (int j = mIdx1.y(); j <= mIdx2.y(); j++) {
      for (auto& uFig : via.getViaDef()->getCutFigs()) {
        auto obj = static_cast<frRect*>(uFig.get());
        gridGraph_.getPoint(pt, i, j);
        odb::dbTransform xform(pt);
        odb::Rect tmpBx = obj->getBBox();
        xform.apply(tmpBx);
        tmpBxCenter = {(tmpBx.xMin() + tmpBx.xMax()) / 2,
                       (tmpBx.yMin() + tmpBx.yMax()) / 2};
        distSquare = box2boxDistSquareNew(box, tmpBx, dx, dy);
        c2cSquare = odb::Point::squaredDistance(boxCenter, tmpBxCenter);
        prl = std::max(-dx, -dy);
        hasViol = false;
        if (con != nullptr) {
          reqDistSquare = con->getCutSpacing();
          reqDistSquare *= reqDistSquare;
          currDistSquare = con->hasCenterToCenter() ? c2cSquare : distSquare;
          if (currDistSquare < reqDistSquare) {
            hasViol = true;
          }
        }
        if (!hasViol && lef58con != nullptr) {
          bool center2center = false;
          if (prl > 0) {
            reqDistSquare = lef58conSpc.second;
          } else {
            reqDistSquare = lef58conSpc.first;
          }
          if (lef58con->getDefaultCenterAndEdge()) {
            if ((frCoord) reqDistSquare
                == std::max(lef58conSpc.first, lef58conSpc.second)) {
              center2center = true;
            }
          }
          if (lef58con->getDefaultCenterToCenter()) {
            center2center = true;
          }
          reqDistSquare *= reqDistSquare;
          currDistSquare = center2center ? c2cSquare : distSquare;
          if (currDistSquare < reqDistSquare) {
            hasViol = true;
          }
        }

        if (hasViol) {
          frMIdx idx = gridGraph_.getIdx(i, j, z2);
          switch (type) {
            case subRouteShape:
              gridGraph_.subRouteShapeCostVia(idx);  // safe access
              break;
            case addRouteShape:
              gridGraph_.addRouteShapeCostVia(idx);  // safe access
              break;
            case subFixedShape:
              gridGraph_.subFixedShapeCostVia(idx);  // safe access
              break;
            case addFixedShape:
              gridGraph_.addFixedShapeCostVia(idx);  // safe access
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
    odb::Rect box = obj->getBBox();
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
    odb::Rect box = obj->getBBox();
    ndr = connFig->getNet()->getFrNet()->getNondefaultRule();
    modMinSpacingCostPlanar(box, zIdx, type, false, ndr);
    modMinSpacingCostVia(box, zIdx, type, true, true, false, ndr);
    modMinSpacingCostVia(box, zIdx, type, false, true, false, ndr);
    if (modEol) {
      modEolSpacingRulesCost(box, zIdx, type);
    }
  } else if (connFig->typeId() == drcVia) {
    auto obj = static_cast<drVia*>(connFig);
    auto [bi, ei] = obj->getMazeIdx();
    // new

    // assumes enclosure for via is always rectangle
    odb::Rect box = obj->getLayer1BBox();
    ndr = connFig->getNet()->getFrNet()->getNondefaultRule();
    modMinSpacingCostPlanar(box, bi.z(), type, false, ndr);
    modMinSpacingCostVia(box, bi.z(), type, true, false, false, ndr);
    modMinSpacingCostVia(box, bi.z(), type, false, false, false, ndr);
    if (modEol) {
      modEolSpacingRulesCost(box, bi.z(), type, false, ndr);
    }

    // assumes enclosure for via is always rectangle
    box = obj->getLayer2BBox();

    modMinSpacingCostPlanar(box, ei.z(), type, false, ndr);
    modMinSpacingCostVia(box, ei.z(), type, true, false, false, ndr);
    modMinSpacingCostVia(box, ei.z(), type, false, false, false, ndr);
    if (modEol) {
      modEolSpacingRulesCost(box, ei.z(), type, false, ndr);
    }

    odb::Point pt = obj->getOrigin();
    odb::dbTransform xform(pt);
    for (auto& uFig : obj->getViaDef()->getCutFigs()) {
      auto rect = static_cast<frRect*>(uFig.get());
      box = rect->getBBox();
      xform.apply(box);
      if (modCutSpc) {
        modCutSpacingCost(box, bi.z(), type, false, bi.x(), bi.y());
      }
      modInterLayerCutSpacingCost(box, bi.z(), type, true);
      modInterLayerCutSpacingCost(box, bi.z(), type, false);
    }
  }
}

bool FlexDRWorker::mazeIterInit_sortRerouteNets(
    int mazeIter,
    std::vector<drNet*>& rerouteNets)
{
  auto rerouteNetsComp = [](drNet* const& a, drNet* const& b) {
    if (a->getPriority() > b->getPriority()) {
      return true;
    }
    if (a->getPriority() < b->getPriority()) {
      return false;
    }
    if (a->getFrNet()->getAbsPriorityLvl()
        > b->getFrNet()->getAbsPriorityLvl()) {
      return true;
    }
    if (a->getFrNet()->getAbsPriorityLvl()
        < b->getFrNet()->getAbsPriorityLvl()) {
      return false;
    }
    const odb::Rect boxA = a->getPinBox();
    const odb::Rect boxB = b->getPinBox();
    const auto areaA = boxA.area();
    const auto areaB = boxB.area();
    const int pinsA = a->getNumPinsIn();
    const int pinsB = b->getNumPinsIn();
    const auto idA = a->getId();
    const auto idB = b->getId();
    return std::tie(pinsA, areaA, idA) < std::tie(pinsB, areaB, idB);
  };
  // sort
  if (mazeIter == 0) {
    std::ranges::sort(rerouteNets, rerouteNetsComp);
    // to be removed
    if (router_cfg_->OR_SEED != -1 && rerouteNets.size() >= 2) {
      std::uniform_int_distribution<int> distribution(0,
                                                      rerouteNets.size() - 1);
      std::default_random_engine generator(router_cfg_->OR_SEED);
      int numSwap = (double) (rerouteNets.size()) * router_cfg_->OR_K;
      for (int i = 0; i < numSwap; i++) {
        int idx = distribution(generator);
        std::swap(rerouteNets[idx],
                  rerouteNets[(idx + 1) % rerouteNets.size()]);
      }
    }
  }
  return true;
}

bool FlexDRWorker::mazeIterInit_sortRerouteQueue(
    int mazeIter,
    std::vector<RouteQueueEntry>& rerouteNets)
{
  auto rerouteNetsComp
      = [](RouteQueueEntry const& a, RouteQueueEntry const& b) {
          auto block1 = a.block;
          auto block2 = b.block;
          if (block1->typeId() == block2->typeId()) {
            return block1->getId() < block2->getId();
          }
          return block1->typeId() < block2->typeId();
        };
  // sort
  if (mazeIter == 0) {
    std::ranges::sort(rerouteNets, rerouteNetsComp);
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

void FlexDRWorker::writeGCPatchesToDRWorker(
    drNet* target_net,
    const std::vector<FlexMazeIdx>& valid_indices)
{
  for (auto& pwire : gcWorker_->getPWires()) {
    auto net = pwire->getNet();
    if (!net) {
      net = target_net;
    }
    if (!net) {
      logger_->error(utl::DRT, 407, "pwire with no net");
    }
    net->setModified(true);
    auto tmp_pwire = std::make_unique<drPatchWire>();
    tmp_pwire->setLayerNum(pwire->getLayerNum());
    tmp_pwire->setOrigin(pwire->getOrigin());
    tmp_pwire->setOffsetBox(pwire->getOffsetBox());
    if (!valid_indices.empty()) {
      // Find closest path point
      odb::Point closest;
      frSquaredDistance closest_dist
          = std::numeric_limits<frSquaredDistance>::max();
      auto origin = pwire->getOrigin();
      auto box = pwire->getOffsetBox();
      for (auto p : valid_indices) {
        odb::Point pp;
        gridGraph_.getPoint(pp, p.x(), p.y());
        frSquaredDistance path_length = odb::Point::squaredDistance(origin, pp);
        if (path_length < closest_dist) {
          closest_dist = path_length;
          closest = pp;
        }
      }
      tmp_pwire->setOrigin(closest);
      tmp_pwire->setOffsetBox(
          odb::Rect(origin.getX() - closest.getX() + box.xMin(),
                    origin.getY() - closest.getY() + box.yMin(),
                    origin.getX() - closest.getX() + box.xMax(),
                    origin.getY() - closest.getY() + box.yMax()));
    }
    tmp_pwire->addToNet(net);
    std::unique_ptr<drConnFig> tmp(std::move(tmp_pwire));
    getWorkerRegionQuery().add(tmp.get());
    net->addRoute(std::move(tmp));
  }
}

void FlexDRWorker::route_queue()
{
  std::queue<RouteQueueEntry> rerouteQueue;

  if (needRecheck_) {
    gcWorker_->setEnableSurgicalFix(true);
    gcWorker_->main();
    writeGCPatchesToDRWorker();
    gcWorker_->clearPWires();
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
      std::cout << marker << "\n";
    }
    if (needRecheck_) {
      std::cout << "(Needs recheck)\n";
    }
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
  writeGCPatchesToDRWorker();

  setMarkers(gcWorker_->getMarkers());

  for (auto& net : nets_) {
    net->setBestRouteConnFigs();
  }
  setBestMarkers();
  if (graphics_) {
    graphics_->endWorker(drIter_);
    graphics_->show(true);
  }

  if (getDRIter() >= 7 && getDRIter() <= 30) {
    identifyCongestionLevel();
  }
}

void FlexDRWorker::identifyCongestionLevel()
{
  std::vector<drNet*> bpNets;
  for (auto& uNet : nets_) {
    drNet* net = uNet.get();
    bool bpLow = false, bpHigh = false;
    for (auto& uPin : net->getPins()) {
      drPin* pin = uPin.get();
      if (pin->hasFrTerm()) {
        continue;
      }
      for (auto& uAP : pin->getAccessPatterns()) {
        drAccessPattern* ap = uAP.get();
        if (design_->isVerticalLayer(ap->getBeginLayerNum())) {
          if (ap->getPoint().y() == getRouteBox().yMin()) {
            bpLow = true;
          } else if (ap->getPoint().y() == getRouteBox().yMax()) {
            bpHigh = true;
          }
        } else {
          if (ap->getPoint().x() == getRouteBox().xMin()) {
            bpLow = true;
          } else if (ap->getPoint().x() == getRouteBox().xMax()) {
            bpHigh = true;
          }
        }
      }
    }
    if (bpLow && bpHigh) {
      bpNets.push_back(net);
    }
  }
  std::vector<int> nLowBorderCross(gridGraph_.getLayerCount(), 0);
  std::vector<int> nHighBorderCross(gridGraph_.getLayerCount(), 0);
  for (drNet* net : bpNets) {
    for (auto& uPin : net->getPins()) {
      drPin* pin = uPin.get();
      if (pin->hasFrTerm()) {
        continue;
      }
      for (auto& uAP : pin->getAccessPatterns()) {
        drAccessPattern* ap = uAP.get();
        frMIdx z = gridGraph_.getMazeZIdx(ap->getBeginLayerNum());
        if (z < 4) {
          continue;
        }
        if (design_->isVerticalLayer(ap->getBeginLayerNum())) {
          if (ap->getPoint().y() == getRouteBox().yMin()) {
            nLowBorderCross[z]++;
          } else if (ap->getPoint().y() == getRouteBox().yMax()) {
            nHighBorderCross[z]++;
          }
        } else {
          if (ap->getPoint().x() == getRouteBox().xMin()) {
            nLowBorderCross[z]++;
          } else if (ap->getPoint().x() == getRouteBox().xMax()) {
            nHighBorderCross[z]++;
          }
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
        if (!utp->isHorizontal()) {  // reminder: trackPattern isHorizontal
                                     // means vertical tracks
          tp = utp.get();
        }
      }
      workerSize = getRouteBox().dy();
    } else {
      for (auto& utp : trackPatterns) {
        if (utp->isHorizontal()) {
          tp = utp.get();
        }
      }
      workerSize = getRouteBox().dx();
    }
    if (tp == nullptr) {
      continue;
    }
    int nTracks = workerSize / tp->getTrackSpacing();  // 1 track error margin
    float congestionFactorLow = nLowBorderCross[z] / (float) nTracks;
    float congestionFactorHigh = nHighBorderCross[z] / (float) nTracks;
    float finalFactor = std::max(congestionFactorLow, congestionFactorHigh);
    if (finalFactor >= router_cfg_->CONGESTION_THRESHOLD) {
      isCongested_ = true;
      return;
    }
  }
}

void FlexDRWorker::route_queue_main(std::queue<RouteQueueEntry>& rerouteQueue)
{
  int gc_version = 1;
  std::map<frBlockObject*, std::pair<int, int>> obj_gc_version;
  auto& workerRegionQuery = getWorkerRegionQuery();
  while (!rerouteQueue.empty()) {
    auto& entry = rerouteQueue.front();
    frBlockObject* obj = entry.block;
    bool doRoute = entry.doRoute;
    int numReroute = entry.numReroute;
    frBlockObject* checking_obj = entry.checkingObj;

    rerouteQueue.pop();
    bool didRoute = false;
    bool didCheck = false;

    if (obj->typeId() == drcNet && doRoute) {
      auto net = static_cast<drNet*>(obj);
      if (numReroute != net->getNumReroutes()) {
        continue;
      }
      if (ripupMode_ == RipUpMode::DRC && checking_obj != nullptr
          && obj_gc_version.find(net->getFrNet()) != obj_gc_version.end()
          && obj_gc_version.find(checking_obj) != obj_gc_version.end()
          && obj_gc_version[net->getFrNet()] == std::make_pair(gc_version, 0)
          && obj_gc_version[checking_obj] == std::make_pair(gc_version, 0)) {
        continue;
      }
      // init
      net->setModified(true);
      net->setNumMarkers(0);
      if (graphics_) {
        graphics_->startNet(net);
      }
      for (auto& uConnFig : net->getRouteConnFigs()) {
        subPathCost(uConnFig.get(), false, true);
        workerRegionQuery.remove(uConnFig.get());  // worker region query
      }
      modEolCosts_poly(gcWorker_->getNet(net->getFrNet()),
                       ModCostType::subRouteShape);
      // route_queue need to unreserve via access if all nets are ripped up
      // (i.e., not routed) see route_queue_init_queue this
      // is unreserve via via is reserved only when drWorker starts from nothing
      // and via is reserved
      if (net->getNumReroutes() == 0
          && (getRipupMode() == RipUpMode::ALL
              || getRipupMode() == RipUpMode::INCR)) {
        initMazeCost_via_helper(net, false);
      }
      net->clear();
      if (getDRIter() >= beginDebugIter) {
        logger_->info(DRT, 2002, "Routing net {}", net->getFrNet()->getName());
      }
      // route
      mazeNetInit(net);
      std::vector<FlexMazeIdx> paths;
      bool isRouted = routeNet(net, paths);
      if (isRouted == false) {
        if (router_cfg_->OUT_MAZE_FILE == std::string("")) {
          if (router_cfg_->VERBOSE > 0) {
            std::cout
                << "Warning: no output maze log specified, skipped writing "
                   "maze log\n";
          }
        } else {
          gridGraph_.print();
        }
        if (graphics_) {
          graphics_->show(false);
        }
        // TODO odb::Rect can't be logged directly
        std::stringstream routeBoxStringStream;
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
      gc_version++;
      net->addNumReroutes();
      didRoute = true;
      // gc
      if (gcWorker_->setTargetNet(net)) {
        gcWorker_->updateDRNet(net);
        gcWorker_->setEnableSurgicalFix(true);
        gcWorker_->main();
        modEolCosts_poly(gcWorker_->getTargetNet(), ModCostType::addRouteShape);
        // write back GC patches
        drNet* currNet = net;
        writeGCPatchesToDRWorker(currNet, paths);
        gcWorker_->clearPWires();
        if (getDRIter() >= beginDebugIter
            && !getGCWorker()->getMarkers().empty()) {
          logger_->info(DRT,
                        2003,
                        "Ending net {} with markers:",
                        net->getFrNet()->getName());
          for (auto& marker : getGCWorker()->getMarkers()) {
            std::cout << *marker << "\n";
          }
        }
        didCheck = true;
        obj_gc_version[net->getFrNet()]
            = {gc_version, gcWorker_->getMarkers().size()};

      } else {
        logger_->error(DRT, 1006, "failed to setTargetNet");
      }
    } else {
      if (getRipupMode() == RipUpMode::VIASWAP) {
        auto net = static_cast<drNet*>(obj);
        bool no_solution_found = false;
        for (auto& conn_fig : net->getRouteConnFigs()) {
          if (conn_fig->typeId() != drcVia) {
            continue;
          }
          auto old_via = static_cast<drVia*>(conn_fig.get());
          if (old_via->isLonely()) {
            auto cutLayer
                = getTech()->getLayer(old_via->getViaDef()->getCutLayerNum());
            const frViaDef* replacement_via_def = nullptr;
            if (cutLayer->getSecondaryViaDefs().size()
                <= numReroute)  // no more secViaDefs to try
            {
              no_solution_found = true;
              // fall back to the original viadef
              replacement_via_def = cutLayer->getDefaultViaDef();
            } else {
              replacement_via_def = cutLayer->getSecondaryViaDef(numReroute);
            }
            if (replacement_via_def == old_via->getViaDef()) {
              continue;
            }
            // replace via with a secondaryVia at index numReroute
            std::unique_ptr<drConnFig> new_via_connfig
                = std::make_unique<drVia>(*old_via);
            auto new_via_ptr = static_cast<drVia*>(new_via_connfig.get());
            new_via_ptr->setViaDef(replacement_via_def);
            // remove old via
            getWorkerRegionQuery().remove(old_via);
            net->removeShape(old_via);
            // add new via
            net->addRoute(std::move(new_via_connfig));
            getWorkerRegionQuery().add(new_via_ptr);
          }
        }
        if (!no_solution_found) {
          gcWorker_->setTargetNet(net->getFrNet());
          gcWorker_->updateDRNet(net);
          gcWorker_->setEnableSurgicalFix(true);
          gcWorker_->main();
          if (gcWorker_->getMarkers().empty()) {
            net->setModified(true);
            writeGCPatchesToDRWorker();
          } else {
            rerouteQueue.emplace(net, numReroute + 1, false, net);
          }
          gcWorker_->clearPWires();
        }
      } else {
        gcWorker_->setEnableSurgicalFix(false);
        if (obj_gc_version.find(obj) != obj_gc_version.end()
            && obj_gc_version[obj].first == gc_version) {
          continue;
        }
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
        obj_gc_version[obj] = {gc_version, gcWorker_->getMarkers().size()};
      }
    }
    // end
    if (didCheck) {
      route_queue_update_queue(gcWorker_->getMarkers(), rerouteQueue, obj);
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
  if (eol.eolSpace == 0) {
    return;
  }
  for (auto& edges : shape->getPolygonEdges()) {
    for (auto& edge : edges) {
      if (edge->length() >= eol.eolWidth) {
        continue;
      }
      frCoord low, high, line;
      bool innerDirIsIncreasing;  // x: increases to the east, y: increases to
                                  // the north
      if (edge->isVertical()) {
        low = std::min(edge->low().y(), edge->high().y());
        high = std::max(edge->low().y(), edge->high().y());
        line = edge->low().x();
        innerDirIsIncreasing = edge->getInnerDir() == frDirEnum::E;
      } else {
        low = std::min(edge->low().x(), edge->high().x());
        high = std::max(edge->low().x(), edge->high().x());
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
  odb::Rect testBox;
  auto eol = layer->getDrEolSpacingConstraint();
  if (isVertical) {
    if (innerDirIsIncreasing) {
      testBox.init(
          line - eol.eolSpace, low - eol.eolWithin, line, high + eol.eolWithin);
    } else {
      testBox.init(
          line, low - eol.eolWithin, line + eol.eolSpace, high + eol.eolWithin);
    }
  } else {
    if (innerDirIsIncreasing) {
      testBox.init(
          low - eol.eolWithin, line - eol.eolSpace, high + eol.eolWithin, line);
    } else {
      testBox.init(
          low - eol.eolWithin, line, high + eol.eolWithin, line + eol.eolSpace);
    }
  }
  frMIdx z = gridGraph_.getMazeZIdx(layer->getLayerNum());
  modEolSpacingCost_helper(testBox, z, modType, 0);
  modEolSpacingCost_helper(testBox, z, modType, 1);
  modEolSpacingCost_helper(testBox, z, modType, 2);
}

void FlexDRWorker::cleanUnneededPatches_poly(gcNet* drcNet, drNet* net)
{
  std::vector<std::vector<float>> areaMap(getTech()->getTopLayerNum() + 1);
  std::vector<drPatchWire*> patchesToRemove;
  for (auto& shape : net->getRouteConnFigs()) {
    if (shape->typeId() != frBlockObjectEnum::drcPatchWire) {
      continue;
    }
    drPatchWire* patch = static_cast<drPatchWire*>(shape.get());
    gtl::point_data<frCoord> pt(patch->getOrigin().x(), patch->getOrigin().y());
    frLayerNum lNum = patch->getLayerNum();
    frCoord minArea
        = getTech()->getLayer(lNum)->getAreaConstraint()->getMinArea();
    if (areaMap[lNum].empty()) {
      areaMap[lNum].assign(drcNet->getPins(lNum).size(), -1);
    }
    for (int i = 0; i < drcNet->getPins(lNum).size(); i++) {
      auto& pin = drcNet->getPins(lNum)[i];
      if (!gtl::contains(*pin->getPolygon(), pt)) {
        continue;
      }
      frCoord area;
      if (areaMap[lNum][i] == -1) {
        areaMap[lNum][i] = gtl::area(*pin->getPolygon());
      }
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
    if (layer->getType() != dbTechLayerType::ROUTING) {
      continue;
    }
    for (auto& pin : net->getPins(lNum)) {
      modEolCosts_poly(pin.get(), layer, modType);
    }
  }
}

void FlexDRWorker::routeNet_prep(
    drNet* net,
    frOrderedIdSet<drPin*>& unConnPins,
    std::map<FlexMazeIdx, frOrderedIdSet<drPin*>>& mazeIdx2unConnPins,
    std::set<FlexMazeIdx>& apMazeIdx,
    std::set<FlexMazeIdx>& realPinAPMazeIdx,
    std::map<FlexMazeIdx, frBox3D*>& mazeIdx2TaperBox,
    std::list<std::pair<drPin*, frBox3D>>& pinTaperBoxes)
{
  frBox3D* tbx = nullptr;
  if (getDRIter() >= beginDebugIter) {
    logger_->info(DRT, 2005, "Creating dest search points from pins:");
  }
  for (auto& pin : net->getPins()) {
    if (getDRIter() >= beginDebugIter) {
      logger_->info(DRT, 2006, "Pin {}", pin->getName());
    }
    unConnPins.insert(pin.get());
    if (gridGraph_.getNDR()) {
      if (router_cfg_->AUTO_TAPER_NDR_NETS
          && pin->isInstPin()) {  // create a taper box for each pin
        auto [l, h] = pin->getAPBbox();
        frCoord pitch
            = getTech()->getLayer(gridGraph_.getLayerNum(l.z()))->getPitch(),
            r;
        r = router_cfg_->TAPERBOX_RADIUS;
        l.set(gridGraph_.getMazeXIdx(gridGraph_.xCoord(l.x()) - r * pitch),
              gridGraph_.getMazeYIdx(gridGraph_.yCoord(l.y()) - r * pitch),
              l.z());
        h.set(gridGraph_.getMazeXIdx(gridGraph_.xCoord(h.x()) + r * pitch),
              gridGraph_.getMazeYIdx(gridGraph_.yCoord(h.y()) + r * pitch),
              h.z());
        frMIdx z = l.z() == 0 ? 1 : h.z();
        pinTaperBoxes.emplace_back(
            pin.get(), frBox3D(l.x(), l.y(), h.x(), h.y(), l.z(), z));
        tbx = &std::prev(pinTaperBoxes.end())->second;
        for (z = tbx->zLow(); z <= tbx->zHigh();
             z++) {  // populate the map from points to taper boxes
          for (int x = tbx->xMin(); x <= tbx->xMax(); x++) {
            for (int y = tbx->yMin(); y <= tbx->yMax(); y++) {
              mazeIdx2TaperBox[FlexMazeIdx(x, y, z)] = tbx;
            }
          }
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
    frOrderedIdSet<drPin*>& unConnPins,
    std::map<FlexMazeIdx, frOrderedIdSet<drPin*>>& mazeIdx2unConnPins,
    std::vector<FlexMazeIdx>& connComps,
    FlexMazeIdx& ccMazeIdx1,
    FlexMazeIdx& ccMazeIdx2,
    odb::Point& centerPt)
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
      odb::Point bp = ap->getPoint();
      totX += bp.x();
      totY += bp.y();
      totZ += gridGraph_.getZHeight(mi.z());
      totAPCnt++;
      break;
    }
  }
  if (totAPCnt == 0) {
    logger_->error(DRT, 423, "No access points found");
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
      odb::Point bp = ap->getPoint();
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
    ccMazeIdx1.set(std::min(ccMazeIdx1.x(), mi.x()),
                   std::min(ccMazeIdx1.y(), mi.y()),
                   std::min(ccMazeIdx1.z(), mi.z()));
    ccMazeIdx2.set(std::max(ccMazeIdx2.x(), mi.x()),
                   std::max(ccMazeIdx2.y(), mi.y()),
                   std::max(ccMazeIdx2.z(), mi.z()));
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
    std::map<FlexMazeIdx, frOrderedIdSet<drPin*>>& mazeIdx2unConnPins,
    std::list<std::pair<drPin*, frBox3D>>& pinTaperBoxes)
{
  odb::Point pt;
  odb::Point ll, ur;
  gridGraph_.getPoint(ll, ccMazeIdx1.x(), ccMazeIdx1.y());
  gridGraph_.getPoint(ur, ccMazeIdx2.x(), ccMazeIdx2.y());
  frCoord currDist = std::numeric_limits<frCoord>::max();
  drPin* nextDst = nullptr;
  // Find the next dst pin nearest to the src
  for (auto& [mazeIdx, setS] : mazeIdx2unConnPins) {
    gridGraph_.getPoint(pt, mazeIdx.x(), mazeIdx.y());
    frCoord dx = std::max({ll.x() - pt.x(), pt.x() - ur.x(), 0});
    frCoord dy = std::max({ll.y() - pt.y(), pt.y() - ur.y(), 0});
    frCoord dz = std::max({gridGraph_.getZHeight(ccMazeIdx1.z())
                               - gridGraph_.getZHeight(mazeIdx.z()),
                           gridGraph_.getZHeight(mazeIdx.z())
                               - gridGraph_.getZHeight(ccMazeIdx2.z()),
                           0});
    if (dx + dy + dz < currDist) {
      currDist = dx + dy + dz;
      nextDst = *(setS.begin());
    }
    if (currDist == 0) {
      break;
    }
  }
  if (gridGraph_.getNDR()) {
    if (router_cfg_->AUTO_TAPER_NDR_NETS) {
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
    std::vector<FlexMazeIdx>& path,
    std::vector<FlexMazeIdx>& connComps,
    frOrderedIdSet<drPin*>& unConnPins,
    std::map<FlexMazeIdx, frOrderedIdSet<drPin*>>& mazeIdx2unConnPins,
    bool isFirstConn)
{
  // first point is dst
  std::set<FlexMazeIdx> localConnComps;
  if (!path.empty()) {
    auto mi = path[0];
    std::vector<drPin*> tmpPins;
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
        if (router_cfg_->ALLOW_PIN_AS_FEEDTHROUGH) {
          localConnComps.insert(mi);
          gridGraph_.setSrc(mi);
        }
      }
    }
  } else {
    std::cout << "Error: routeNet_postAstarUpdate path is empty\n";
  }
  // must be before comment line ABC so that the used actual src is set in
  // gridgraph
  if (isFirstConn && (!router_cfg_->ALLOW_PIN_AS_FEEDTHROUGH)) {
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
    if (isFirstConn && !router_cfg_->ALLOW_PIN_AS_FEEDTHROUGH) {
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
    std::vector<FlexMazeIdx>& points,
    const std::set<FlexMazeIdx>& realPinApMazeIdx,
    std::map<FlexMazeIdx, frBox3D*>& mazeIdx2TaperBox,
    const std::set<FlexMazeIdx>& apMazeIdx)
{
  if (points.empty()) {
    return;
  }
  auto& workerRegionQuery = getWorkerRegionQuery();
  frBox3D *srcBox = nullptr, *dstBox = nullptr;
  auto it = mazeIdx2TaperBox.find(points[0]);
  if (it != mazeIdx2TaperBox.end()) {
    dstBox = it->second;
  }
  it = mazeIdx2TaperBox.find(points.back());
  if (it != mazeIdx2TaperBox.end()) {
    srcBox = it->second;
  }
  if (points.size() == 1) {
    if (net->getFrAccessPoint(gridGraph_.xCoord(points[0].x()),
                              gridGraph_.yCoord(points[0].y()),
                              gridGraph_.getLayerNum(points[0].z()))) {
      if (!addApPathSegs(points[0], net)) {
        addApExtFigUpdate(net, points[0]);
      }
    }
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
        odb::Point loc;
        frLayerNum startLayerNum = gridGraph_.getLayerNum(currZ);
        gridGraph_.getPoint(loc, startX, startY);
        FlexMazeIdx mi(startX, startY, currZ);
        auto via = getTech()->getLayer(startLayerNum + 1)->getDefaultViaDef();
        auto it = apSVia_.find(mi);
        if (gridGraph_.isSVia(startX, startY, currZ) && it != apSVia_.end()) {
          via = it->second->getAccessViaDef();
        }
        auto net_ndr = net->getFrNet()->getNondefaultRule();
        if (net_ndr != nullptr && net_ndr->getPrefVia(startLayerNum / 2 - 1)) {
          via = net_ndr->getPrefVia(startLayerNum / 2 - 1);
        }
        auto currVia = std::make_unique<drVia>(via);
        if (net->hasNDR() && router_cfg_->AUTO_TAPER_NDR_NETS) {
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
          if (!addApPathSegs(mzIdxBot, net)) {
            currVia->setBottomConnected(true);
          }
        } else {
          checkViaConnectivityToAP(
              currVia.get(), true, net->getFrNet(), apMazeIdx, mzIdxBot);
        }
        if (realPinApMazeIdx.find(mzIdxTop) != realPinApMazeIdx.end()) {
          if (!addApPathSegs(mzIdxTop, net)) {
            currVia->setTopConnected(true);
          }
        } else {
          checkViaConnectivityToAP(
              currVia.get(), false, net->getFrNet(), apMazeIdx, mzIdxTop);
        }
        std::unique_ptr<drConnFig> tmp(std::move(currVia));
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
void FlexDRWorker::addApExtFigUpdate(drNet* net,
                                     const FlexMazeIdx& ap_idx) const
{
  frLayerNum layer_num = gridGraph_.getLayerNum(ap_idx.z());
  Point3D real_point;
  gridGraph_.getPoint(real_point, ap_idx.x(), ap_idx.y());
  real_point.setZ(layer_num);
  for (const auto& ext_obj : net->getExtConnFigs()) {
    if (ext_obj->typeId() == drcPathSeg) {
      const drPathSeg* path_seg = static_cast<const drPathSeg*>(ext_obj.get());
      const auto [bp, ep] = path_seg->getPoints();
      if (path_seg->getLayerNum() != layer_num) {
        continue;
      }
      frSegStyle style = path_seg->getStyle();
      if (bp == real_point) {
        style.setBeginStyle(frcTruncateEndStyle, 0);
        net->updateExtFigStyle(real_point, style);
        return;
      }
      if (ep == real_point) {
        style.setEndStyle(frcTruncateEndStyle, 0);
        net->updateExtFigStyle(real_point, style);
        return;
      }
    } else if (ext_obj->typeId() == drcVia) {
      auto via = static_cast<const drVia*>(ext_obj.get());
      odb::Point via_point = via->getOrigin();
      if (via_point != real_point) {
        continue;
      }
      auto via_def = via->getViaDef();
      if (via_def->getLayer1Num() == layer_num) {
        net->updateExtFigConnected(real_point, true, via->isTopConnected());
        return;
      }
      if (via_def->getLayer2Num() == layer_num) {
        net->updateExtFigConnected(real_point, via->isBottomConnected(), true);
        return;
      }
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
  if (!ap) {  // on-the-fly ap
    return false;
  }
  assert(owner != nullptr);
  frInst* inst = nullptr;
  if (owner->typeId() == frBlockObjectEnum::frcInstTerm) {
    inst = static_cast<frInstTerm*>(owner)->getInst();
  }
  assert(ap != nullptr);
  if (ap->getPathSegs().empty()) {
    return false;
  }
  for (auto& ps : ap->getPathSegs()) {
    std::unique_ptr<drPathSeg> drPs = std::make_unique<drPathSeg>();
    drPs->setApPathSeg({x, y});
    odb::Point begin = ps.getBeginPoint();
    odb::Point end = ps.getEndPoint();
    odb::Point* connecting = nullptr;
    if (ps.getBeginStyle() == frEndStyle(frcTruncateEndStyle)) {
      connecting = &begin;
    } else if (ps.getEndStyle() == frEndStyle(frcTruncateEndStyle)) {
      connecting = &end;
    }
    if (inst) {
      odb::dbTransform trans = inst->getNoRotationTransform();
      trans.apply(begin);
      trans.apply(end);
      if (end < begin) {  // if rotation swapped order, correct it
        if (connecting == &begin) {
          connecting = &end;
        } else {
          connecting = &begin;
        }
        odb::Point tmp = begin;
        begin = end;
        end = tmp;
      }
    }
    drPs->setPoints(begin, end);
    drPs->setLayerNum(lNum);
    drPs->addToNet(net);
    auto currStyle = getTech()->getLayer(lNum)->getDefaultSegStyle();
    if (connecting == &begin) {
      currStyle.setBeginStyle(frcTruncateEndStyle, 0);
    } else if (connecting == &end) {
      currStyle.setEndStyle(frcTruncateEndStyle, 0);
    }

    if (net->getFrNet()->getNondefaultRule()) {
      drPs->setTapered(
          true);  // these tiny access pathsegs should all be tapered
    }
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
  if (!net->hasNDR() || !router_cfg_->AUTO_TAPER_NDR_NETS) {
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
    }
    if (startX == endX) {
      midX = startX;
      midY = bx->yMax() + 1;
    } else {
      midX = bx->xMax() + 1;
      midY = startY;
    }
    return true;
  }
  if (srcBox && srcBox->contains(endX, endY, z)) {
    bx = srcBox;
  } else if (dstBox && dstBox->contains(endX, endY, z)) {
    bx = dstBox;
  }
  if (bx) {
    if (bx->contains(startX, startY, z, 1, 1)) {
      taperFirstPiece = true;
      return false;
    }
    if (startX == endX) {
      midX = startX;
      midY = bx->yMin() - 1;
    } else {
      midX = bx->xMin() - 1;
      midY = startY;
    }
    return true;
  }
  return false;
}
void FlexDRWorker::processPathSeg(frMIdx startX,
                                  frMIdx startY,
                                  frMIdx endX,
                                  frMIdx endY,
                                  frMIdx z,
                                  const std::set<FlexMazeIdx>& realApMazeIdx,
                                  drNet* net,
                                  bool segIsVertical,
                                  bool taper,
                                  int i,
                                  std::vector<FlexMazeIdx>& points,
                                  const std::set<FlexMazeIdx>& apMazeIdx)
{
  odb::Point startLoc, endLoc;
  frLayerNum currLayerNum = gridGraph_.getLayerNum(z);
  gridGraph_.getPoint(startLoc, startX, startY);
  gridGraph_.getPoint(endLoc, endX, endY);
  auto currPathSeg = std::make_unique<drPathSeg>();
  currPathSeg->setPoints(startLoc, endLoc);
  currPathSeg->setLayerNum(currLayerNum);
  currPathSeg->addToNet(net);
  FlexMazeIdx start(startX, startY, z), end(endX, endY, z);
  auto layer = getTech()->getLayer(currLayerNum);
  auto currStyle = layer->getDefaultSegStyle();
  if (realApMazeIdx.find(start) != realApMazeIdx.end()) {
    if (!addApPathSegs(start, net)) {
      currStyle.setBeginStyle(frcTruncateEndStyle, 0);
    }
  } else {
    checkPathSegStyle(currPathSeg.get(), true, currStyle, apMazeIdx, start);
  }
  if (realApMazeIdx.find(end) != realApMazeIdx.end()) {
    if (!addApPathSegs(end, net)) {
      currStyle.setEndStyle(frcTruncateEndStyle, 0);
    }
  } else {
    checkPathSegStyle(currPathSeg.get(), false, currStyle, apMazeIdx, end);
  }
  if (net->getFrNet()->getNondefaultRule()) {
    if (taper) {
      currPathSeg->setTapered(true);
    } else {
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
  std::unique_ptr<drConnFig> tmp(std::move(currPathSeg));
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
                                     const std::set<FlexMazeIdx>& apMazeIdx,
                                     const FlexMazeIdx& idx)
{
  const odb::Point& pt = (isBegin ? ps->getBeginPoint() : ps->getEndPoint());
  if (apMazeIdx.find(idx) == apMazeIdx.end()
      && !isInWorkerBorder(pt.x(), pt.y())) {
    return;
  }
  if (hasAccessPoint(pt, ps->getLayerNum(), ps->getNet()->getFrNet())) {
    if (!addApPathSegs(idx, ps->getNet())) {
      if (isBegin) {
        style.setBeginStyle(frEndStyle(frEndStyleEnum::frcTruncateEndStyle), 0);
      } else {
        style.setEndStyle(frEndStyle(frEndStyleEnum::frcTruncateEndStyle), 0);
      }
    }
  }
}

bool FlexDRWorker::hasAccessPoint(const odb::Point& pt,
                                  frLayerNum lNum,
                                  frNet* net)
{
  frRegionQuery::Objects<frBlockObject> result;
  odb::Rect bx(pt.x(), pt.y(), pt.x(), pt.y());
  design_->getRegionQuery()->query(bx, lNum, result);
  for (auto& rqObj : result) {
    switch (rqObj.second->typeId()) {
      case frcInstTerm: {
        auto instTerm = static_cast<frInstTerm*>(rqObj.second);
        if (instTerm->getNet() == net
            && instTerm->hasAccessPoint(pt.x(), pt.y(), lNum)) {
          return true;
        }
        break;
      }
      case frcBTerm: {
        auto term = static_cast<frBTerm*>(rqObj.second);
        if (term->getNet() == net
            && term->hasAccessPoint(pt.x(), pt.y(), lNum, 0)) {
          return true;
        }
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
void FlexDRWorker::checkViaConnectivityToAP(
    drVia* via,
    bool isBottom,
    frNet* net,
    const std::set<FlexMazeIdx>& apMazeIdx,
    const FlexMazeIdx& idx)
{
  if (apMazeIdx.find(idx) == apMazeIdx.end()
      && !isInWorkerBorder(via->getOrigin().x(), via->getOrigin().y())) {
    return;
  }
  if (isBottom) {
    if (hasAccessPoint(
            via->getOrigin(), via->getViaDef()->getLayer1Num(), net)) {
      if (!addApPathSegs(idx, via->getNet())) {
        via->setBottomConnected(true);
      }
    }
  } else {
    if (hasAccessPoint(
            via->getOrigin(), via->getViaDef()->getLayer2Num(), net)) {
      if (!addApPathSegs(idx, via->getNet())) {
        via->setTopConnected(true);
      }
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
          < abs(next->x() - endX) + abs(next->y() - endY)) {
        hasEndExt = true;
      } else {
        hasBeginExt = true;
      }
    } else if (!next) {
      if (abs(prev->x() - startX) + abs(prev->y() - startY)
          < abs(prev->x() - endX) + abs(prev->y() - endY)) {
        hasEndExt = true;
      } else {
        hasBeginExt = true;
      }
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
    if (hasBeginExt) {
      currStyle.setBeginStyle(es,
                              std::max((int) currStyle.getBeginExt(),
                                       (int) ndr->getWireExtension(z)));
    }
    if (hasEndExt) {
      currStyle.setEndStyle(es,
                            std::max((int) currStyle.getEndExt(),
                                     (int) ndr->getWireExtension(z)));
    }
  }
}

inline bool segmentIsOrthogonal(FlexMazeIdx* idx,
                                frMIdx z,
                                frMIdx x,
                                bool isVertical)
{
  if (idx == nullptr) {
    return false;
  }
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
  if (layer->getWrongDirWidth() >= layer->getWidth()) {
    return;
  }
  bool is_vertical = startX == endX;
  if (layer->isVertical() != is_vertical) {
    return;
  }
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
    std::map<FlexMazeIdx, frBox3D*>& mazeIdx2TaperBox)
{
  FlexMazeIdx idx(x, y, startZ);
  auto it = mazeIdx2TaperBox.find(idx);
  if (it != mazeIdx2TaperBox.end()) {
    return true;
  }
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

void FlexDRWorker::routeNet_AddCutSpcCost(std::vector<FlexMazeIdx>& path)
{
  if (path.size() <= 1) {
    return;
  }
  for (uint64_t i = 1; i < path.size(); i++) {
    if (path[i].z() != path[i - 1].z()) {
      frMIdx z = std::min(path[i].z(), path[i - 1].z());
      const frViaDef* viaDef = design_->getTech()
                                   ->getLayer(gridGraph_.getLayerNum(z) + 1)
                                   ->getDefaultViaDef();
      int x = gridGraph_.xCoord(path[i].x());
      int y = gridGraph_.yCoord(path[i].y());
      odb::dbTransform xform(odb::Point(x, y));
      for (auto& uFig : viaDef->getCutFigs()) {
        auto rect = static_cast<frRect*>(uFig.get());
        odb::Rect box = rect->getBBox();
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
                                        std::map<FlexMazeIdx, frCoord>& areaMap)
{
  for (auto& pin : net->getPins()) {
    for (auto& ap : pin->getAccessPatterns()) {
      FlexMazeIdx mIdx = ap->getMazeIdx();
      auto it = areaMap.find(mIdx);
      if (it != areaMap.end()) {
        it->second = std::max(it->second, ap->getBeginArea());
      } else {
        areaMap[mIdx] = ap->getBeginArea();
      }
    }
  }
}

bool FlexDRWorker::routeNet(drNet* net, std::vector<FlexMazeIdx>& paths)
{
  //  ProfileTask profile("DR:routeNet");

  // Verify if net has jumpers
  const bool route_with_jumpers = net->getFrNet()->hasJumpers();

  if (net->getPins().size() <= 1) {
    return true;
  }
  if (graphics_) {
    graphics_->show(true);
  }
  frOrderedIdSet<drPin*> unConnPins;
  std::map<FlexMazeIdx, frOrderedIdSet<drPin*>> mazeIdx2unConnPins;
  std::map<FlexMazeIdx, frBox3D*>
      mazeIdx2TaperBox;  // access points -> taper box: used to efficiently know
                         // what points are in what taper boxes
  std::list<std::pair<drPin*, frBox3D>> pinTaperBoxes;
  std::set<FlexMazeIdx> apMazeIdx;
  std::set<FlexMazeIdx> realPinAPMazeIdx;
  routeNet_prep(net,
                unConnPins,
                mazeIdx2unConnPins,
                apMazeIdx,
                realPinAPMazeIdx,
                mazeIdx2TaperBox,
                pinTaperBoxes);
  // prep for area map
  std::map<FlexMazeIdx, frCoord> areaMap;
  if (router_cfg_->ENABLE_BOUNDARY_MAR_FIX) {
    routeNet_prepAreaMap(net, areaMap);
  }

  FlexMazeIdx ccMazeIdx1, ccMazeIdx2;  // connComps ll, ur flexmazeidx
  odb::Point centerPt;
  std::vector<FlexMazeIdx> connComps;
  routeNet_setSrc(unConnPins,
                  mazeIdx2unConnPins,
                  connComps,
                  ccMazeIdx1,
                  ccMazeIdx2,
                  centerPt);

  std::vector<FlexMazeIdx> path;  // astar must return with >= 1 idx
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
                          mazeIdx2TaperBox,
                          route_with_jumpers)) {
      routeNet_postAstarUpdate(
          path, connComps, unConnPins, mazeIdx2unConnPins, isFirstConn);
      routeNet_postAstarWritePath(
          net, path, realPinAPMazeIdx, mazeIdx2TaperBox, apMazeIdx);
      routeNet_postAstarPatchMinAreaVio(net, path, areaMap);
      routeNet_AddCutSpcCost(path);
      isFirstConn = false;
      // Add current pin path point to drNet paths
      paths.insert(paths.end(), path.begin(), path.end());
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
    if (router_cfg_->CLEAN_PATCHES) {
      gcWorker_->setTargetNet(net);
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
    const std::vector<FlexMazeIdx>& path,
    const std::map<FlexMazeIdx, frCoord>& areaMap)
{
  if (path.empty()) {
    return;
  }
  // get path with separated (stacked vias)
  std::vector<FlexMazeIdx> points;
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
  if (router_cfg_->ENABLE_BOUNDARY_MAR_FIX) {
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
        routeNet_postAstarPatchMinAreaVio_helper(net,
                                                 getTech()->getLayer(layerNum),
                                                 reqArea,
                                                 currArea,
                                                 startViaHalfEncArea,
                                                 endViaHalfEncArea,
                                                 points,
                                                 i,
                                                 prev_i);
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
      odb::Point begin_point, end_point;
      gridGraph_.getPoint(begin_point, currIdx.x(), currIdx.y());
      gridGraph_.getPoint(end_point, nextIdx.x(), nextIdx.y());
      frCoord pathLength
          = odb::Point::manhattanDistance(begin_point, end_point);
      if (currArea < reqArea) {
        if (!prev_is_wire) {
          currArea /= 2;
        }
        currArea += static_cast<frArea>(pathLength) * pathWidth;
      }
      prev_is_wire = true;
    }
    currIdx = nextIdx;
  }
  // add boundary area for last segment
  if (router_cfg_->ENABLE_BOUNDARY_MAR_FIX) {
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
      routeNet_postAstarPatchMinAreaVio_helper(net,
                                               getTech()->getLayer(layerNum),
                                               reqArea,
                                               currArea,
                                               startViaHalfEncArea,
                                               endViaHalfEncArea,
                                               points,
                                               i,
                                               prev_i);
    }
  }
}

void FlexDRWorker::routeNet_postAstarPatchMinAreaVio_helper(
    drNet* net,
    drt::frLayer* curr_layer,
    frArea reqArea,
    frArea currArea,
    frCoord startViaHalfEncArea,
    frCoord endViaHalfEncArea,
    std::vector<FlexMazeIdx>& points,
    int point_idx,
    int prev_point_idx)
{
  FlexMazeIdx begin_point, end_point;
  frArea gapArea = reqArea
                   - (currArea - startViaHalfEncArea - endViaHalfEncArea)
                   - std::min(startViaHalfEncArea, endViaHalfEncArea);
  // bp = begin point, ep = endpoint
  bool is_bp_patch_style_left = true;  // style 1: left only; 0: right only
  bool is_ep_patch_style_right = false;
  // stack via
  if (point_idx - 1 == prev_point_idx) {
    begin_point = points[point_idx - 1];
    end_point = points[point_idx - 1];
    is_bp_patch_style_left = true;
    is_ep_patch_style_right = false;
    // planar
  } else {
    begin_point = points[prev_point_idx];
    end_point = points[point_idx - 1];
    FlexMazeIdx begin_point_successor = points[prev_point_idx + 1],
                end_point_predecessor = points[point_idx - 2];
    if (curr_layer->getDir() == dbTechLayerDir::HORIZONTAL) {
      is_bp_patch_style_left
          = (begin_point.x() == begin_point_successor.x())
                ? (begin_point.x() < end_point.x())
                : (begin_point.x() < begin_point_successor.x());
      is_ep_patch_style_right
          = (end_point.x() == end_point_predecessor.x())
                ? (end_point.x() <= begin_point.x())
                : (end_point.x() < end_point_predecessor.x());
    } else {
      is_bp_patch_style_left
          = (begin_point.y() == begin_point_successor.y())
                ? (begin_point.y() < end_point.y())
                : (begin_point.y() < begin_point_successor.y());
      is_ep_patch_style_right
          = (end_point.y() == end_point_predecessor.y())
                ? (end_point.y() <= begin_point.y())
                : (end_point.y() < end_point_predecessor.y());
    }
  }
  auto patchWidth = curr_layer->getWidth();
  routeNet_postAstarAddPatchMetal(net,
                                  begin_point,
                                  end_point,
                                  gapArea,
                                  patchWidth,
                                  is_bp_patch_style_left,
                                  is_ep_patch_style_right);
}

frCoord FlexDRWorker::getHalfViaEncArea(frMIdx z,
                                        bool isLayer1,
                                        frNonDefaultRule* ndr)
{
  if (!ndr || !ndr->getPrefVia(z)) {
    return gridGraph_.getHalfViaEncArea(z, isLayer1);
  }
  frVia via(ndr->getPrefVia(z));
  odb::Rect box;
  if (isLayer1) {
    box = via.getLayer1BBox();
  } else {
    box = via.getLayer2BBox();
  }
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
  odb::Point origin, patchEnd;
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
  const odb::Point& patchLL = std::min(origin, patchEnd);
  const odb::Point& patchUR = std::max(origin, patchEnd);
  if (!getRouteBox().intersects(patchEnd)) {
    cost = std::numeric_limits<int>::max();
  } else {
    FlexMazeIdx startIdx, endIdx;
    startIdx.set(0, 0, layerNum);
    endIdx.set(0, 0, layerNum);
    odb::Rect patchBox(patchLL, patchUR);
    gridGraph_.getIdxBox(startIdx, endIdx, patchBox, FlexGridGraph::enclose);
    if (isPatchHorz) {
      // in gridgraph, the planar cost is checked for xIdx + 1
      for (auto xIdx = std::max(0, startIdx.x() - 1); xIdx < endIdx.x();
           ++xIdx) {
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
      for (auto yIdx = std::max(0, startIdx.y() - 1); yIdx < endIdx.y();
           ++yIdx) {
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
  odb::Point origin;
  gridGraph_.getPoint(origin, bpIdx.x(), bpIdx.y());
  frLayerNum layerNum = gridGraph_.getLayerNum(bpIdx.z());
  // actual offsetbox
  odb::Point patchLL, patchUR;
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

  auto tmpPatch = std::make_unique<drPatchWire>();
  tmpPatch->setLayerNum(layerNum);
  tmpPatch->setOrigin(origin);
  tmpPatch->setOffsetBox(odb::Rect(patchLL, patchUR));
  tmpPatch->addToNet(net);
  std::unique_ptr<drConnFig> tmp(std::move(tmpPatch));
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

}  // namespace drt
