// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <utility>
#include <vector>

#include "db/obj/frBlockObject.h"
#include "db/obj/frShape.h"
#include "db/obj/frVia.h"
#include "db/taObj/taFig.h"
#include "db/taObj/taPin.h"
#include "db/taObj/taShape.h"
#include "db/taObj/taVia.h"
#include "db/tech/frConstraint.h"
#include "db/tech/frViaDef.h"
#include "frBaseTypes.h"
#include "odb/dbTransform.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "ta/FlexTA.h"

using odb::dbTechLayerDir;
using odb::dbTechLayerType;

namespace drt {

frSquaredDistance FlexTAWorker::box2boxDistSquare(const odb::Rect& box1,
                                                  const odb::Rect& box2,
                                                  frCoord& dx,
                                                  frCoord& dy)
{
  dx = std::max(
      std::max(box1.xMin(), box2.xMin()) - std::min(box1.xMax(), box2.xMax()),
      0);
  dy = std::max(
      std::max(box1.yMin(), box2.yMin()) - std::min(box1.yMax(), box2.yMax()),
      0);
  return (frSquaredDistance) dx * dx + (frSquaredDistance) dy * dy;
}

// must be current TA layer
void FlexTAWorker::modMinSpacingCostPlanar(const odb::Rect& box,
                                           frLayerNum lNum,
                                           taPinFig* fig,
                                           bool isAddCost,
                                           frOrderedIdSet<taPin*>* pinS)
{
  // obj1 = curr obj
  frCoord width1 = box.minDXDY();
  frCoord length1 = box.maxDXDY();
  // obj2 = other obj
  auto layer = getDesign()->getTech()->getLayer(lNum);
  // layer default width
  frCoord width2 = layer->getWidth();
  frCoord halfwidth2 = width2 / 2;
  // spacing value needed
  frCoord bloatDist = layer->getMinSpacingValue(width1, width2, length1, false);
  if (fig->getNet()->getNondefaultRule()) {
    bloatDist = std::max(
        bloatDist,
        fig->getNet()->getNondefaultRule()->getSpacing(lNum / 2 - 1));
  }

  frSquaredDistance bloatDistSquare = (frSquaredDistance) bloatDist * bloatDist;

  bool isH = (getDir() == dbTechLayerDir::HORIZONTAL);

  // now assume track in H direction
  frCoord boxLow = isH ? box.yMin() : box.xMin();
  frCoord boxHigh = isH ? box.yMax() : box.xMax();
  frCoord boxLeft = isH ? box.xMin() : box.yMin();
  frCoord boxRight = isH ? box.xMax() : box.yMax();
  odb::Rect box1(boxLeft, boxLow, boxRight, boxHigh);

  int idx1, idx2;
  getTrackIdx(boxLow - bloatDist - halfwidth2 + 1,
              boxHigh + bloatDist + halfwidth2 - 1,
              lNum,
              idx1,
              idx2);

  odb::Rect box2(-halfwidth2, -halfwidth2, halfwidth2, halfwidth2);
  frCoord dx, dy;
  auto& trackLocs = getTrackLocs(lNum);
  auto& workerRegionQuery = getWorkerRegionQuery();
  for (int i = idx1; i <= idx2; i++) {
    auto trackLoc = trackLocs[i];
    odb::dbTransform xform(odb::Point(boxLeft, trackLoc));
    xform.apply(box2);
    box2boxDistSquare(box1, box2, dx, dy);
    if (dy >= bloatDist) {
      continue;
    }
    frCoord maxX = (frCoord) (sqrt(1.0 * bloatDistSquare
                                   - 1.0 * (frSquaredDistance) dy * dy));
    if ((frSquaredDistance) maxX * maxX + (frSquaredDistance) dy * dy
        == bloatDistSquare) {
      maxX = std::max(0, maxX - 1);
    }
    frCoord blockLeft = boxLeft - maxX - halfwidth2;
    frCoord blockRight = boxRight + maxX + halfwidth2;
    odb::Rect tmpBox;
    if (isH) {
      tmpBox.init(blockLeft, trackLoc, blockRight, trackLoc);
    } else {
      tmpBox.init(trackLoc, blockLeft, trackLoc, blockRight);
    }
    auto con = layer->getMinSpacing();
    if (isAddCost) {
      workerRegionQuery.addCost(tmpBox, lNum, fig, con);
      if (pinS) {
        workerRegionQuery.query(tmpBox, lNum, *pinS);
      }
    } else {
      workerRegionQuery.removeCost(tmpBox, lNum, fig, con);
      if (pinS) {
        workerRegionQuery.query(tmpBox, lNum, *pinS);
      }
    }
  }
}

// given a shape on any routing layer n, block via @(n+1) if isUpperVia is true
void FlexTAWorker::modMinSpacingCostVia(const odb::Rect& box,
                                        frLayerNum lNum,
                                        taPinFig* fig,
                                        bool isAddCost,
                                        bool isUpperVia,
                                        bool isCurrPs,
                                        frOrderedIdSet<taPin*>* pinS)
{
  // obj1 = curr obj
  frCoord width1 = box.minDXDY();
  frCoord length1 = box.maxDXDY();
  // obj2 = other obj
  // default via dimension
  const frViaDef* viaDef = nullptr;
  frLayerNum cutLNum = 0;
  if (isUpperVia) {
    viaDef
        = (lNum < getDesign()->getTech()->getTopLayerNum())
              ? getDesign()->getTech()->getLayer(lNum + 1)->getDefaultViaDef()
              : nullptr;
    cutLNum = lNum + 1;
  } else {
    viaDef
        = (lNum > getDesign()->getTech()->getBottomLayerNum())
              ? getDesign()->getTech()->getLayer(lNum - 1)->getDefaultViaDef()
              : nullptr;
    cutLNum = lNum - 1;
  }
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

  bool isH = (getDir() == dbTechLayerDir::HORIZONTAL);
  frLayerNum followTrackLNum;
  if (cutLNum - 1 >= getDesign()->getTech()->getBottomLayerNum()
      && getDesign()->getTech()->getLayer(cutLNum - 1)->getType()
             == dbTechLayerType::ROUTING
      && getDesign()->getTech()->getLayer(cutLNum - 1)->getDir() == getDir()) {
    followTrackLNum = cutLNum - 1;
  } else if (cutLNum + 1 <= getDesign()->getTech()->getTopLayerNum()
             && getDesign()->getTech()->getLayer(cutLNum + 1)->getType()
                    == dbTechLayerType::ROUTING
             && getDesign()->getTech()->getLayer(cutLNum + 1)->getDir()
                    == getDir()) {
    followTrackLNum = cutLNum + 1;
  } else {
    std::cout
        << "Warning: via layer connected to non-routing layer, skipped in "
           "modMinSpacingCostVia\n";
    return;
  }

  // spacing value needed
  auto layer = getTech()->getLayer(lNum);
  frCoord bloatDist = layer->getMinSpacingValue(
      width1, width2, isCurrPs ? length2 : std::min(length1, length2), false);
  if (fig->getNet()->getNondefaultRule()) {
    bloatDist = std::max(
        bloatDist,
        fig->getNet()->getNondefaultRule()->getSpacing(lNum / 2 - 1));
  }
  int idx1, idx2;
  if (isH) {
    getTrackIdx(box.yMin() - bloatDist - (viaBox.yMax() - 0) + 1,
                box.yMax() + bloatDist + (0 - viaBox.yMin()) - 1,
                followTrackLNum,
                idx1,
                idx2);
  } else {
    getTrackIdx(box.xMin() - bloatDist - (viaBox.xMax() - 0) + 1,
                box.xMax() + bloatDist + (0 - viaBox.xMin()) - 1,
                followTrackLNum,
                idx1,
                idx2);
  }

  auto& trackLocs = getTrackLocs(followTrackLNum);
  auto& workerRegionQuery = getWorkerRegionQuery();
  odb::Rect tmpBx;
  odb::dbTransform xform;
  frCoord dx, dy, prl;
  frCoord maxX, blockLeft, blockRight;
  odb::Rect blockBox;
  for (int i = idx1; i <= idx2; i++) {
    auto trackLoc = trackLocs[i];
    if (isH) {
      xform.setOffset(odb::Point(box.xMin(), trackLoc));
    } else {
      xform.setOffset(odb::Point(trackLoc, box.yMin()));
    }
    tmpBx = viaBox;
    xform.apply(tmpBx);
    box2boxDistSquare(box, tmpBx, dx, dy);
    if (isH) {           // track is horizontal
      if (dy > 0) {      // via at the bottom of box
        if (isCurrPs) {  // prl maxed out to be viaBox
          prl = viaBox.dx();
        } else {  // prl maxed out to be smaller of box and viaBox
          prl = std::min(box.dx(), viaBox.dx());
        }
        // via at the side of box
      } else {
        if (isCurrPs) {  // prl maxed out to be viaBox
          prl = viaBox.dy();
        } else {  // prl maxed out to be smaller of box and viaBox
          prl = std::min(box.dy(), viaBox.dy());
        }
      }
    } else {             // track is vertical
      if (dx > 0) {      // via at the bottom of box
        if (isCurrPs) {  // prl maxed out to be viaBox
          prl = viaBox.dy();
        } else {  // prl maxed out to be smaller of box and viaBox
          prl = std::min(box.dy(), viaBox.dy());
        }
        // via at the side of box
      } else {
        if (isCurrPs) {  // prl maxed out to be viaBox
          prl = viaBox.dx();
        } else {  // prl maxed out to be smaller of box and viaBox
          prl = std::min(box.dx(), viaBox.dx());
        }
      }
    }

    frCoord reqDist = layer->getMinSpacingValue(width1, width2, prl, false);
    if (fig->getNet()->getNondefaultRule()) {
      reqDist = std::max(
          reqDist,
          fig->getNet()->getNondefaultRule()->getSpacing(lNum / 2 - 1));
    }

    if (isH) {
      if (dy >= reqDist) {
        continue;
      }
      maxX = (frCoord) (sqrt(1.0 * reqDist * reqDist - 1.0 * dy * dy));
      if (maxX * maxX + dy * dy == reqDist * reqDist) {
        maxX = std::max(0, maxX - 1);
      }
      blockLeft = box.xMin() - maxX - (viaBox.xMax() - 0);
      blockRight = box.xMax() + maxX + (0 - viaBox.xMin());

      blockBox.init(blockLeft, trackLoc, blockRight, trackLoc);
    } else {
      if (dx >= reqDist) {
        continue;
      }
      maxX = (frCoord) (sqrt(1.0 * reqDist * reqDist - 1.0 * dx * dx));
      if (maxX * maxX + dx * dx == reqDist * reqDist) {
        maxX = std::max(0, maxX - 1);
      }
      blockLeft = box.yMin() - maxX - (viaBox.yMax() - 0);
      blockRight = box.yMax() + maxX + (0 - viaBox.yMin());

      blockBox.init(trackLoc, blockLeft, trackLoc, blockRight);
    }

    auto con = layer->getMinSpacing();
    if (isAddCost) {
      workerRegionQuery.addViaCost(blockBox, cutLNum, fig, con);
      if (pinS) {
        workerRegionQuery.query(blockBox, cutLNum, *pinS);
      }
    } else {
      workerRegionQuery.removeViaCost(blockBox, cutLNum, fig, con);
      if (pinS) {
        workerRegionQuery.query(blockBox, cutLNum, *pinS);
      }
    }
  }
}

void FlexTAWorker::modCutSpacingCost(const odb::Rect& box,
                                     frLayerNum lNum,
                                     taPinFig* fig,
                                     bool isAddCost,
                                     frOrderedIdSet<taPin*>* pinS)
{
  if (!getDesign()->getTech()->getLayer(lNum)->hasCutSpacing()) {
    return;
  }
  // obj1 = curr obj
  // obj2 = other obj
  // default via dimension
  const frViaDef* viaDef
      = getDesign()->getTech()->getLayer(lNum)->getDefaultViaDef();
  frVia via(viaDef);
  odb::Rect viaBox = via.getCutBBox();

  bool isH = (getDir() == dbTechLayerDir::HORIZONTAL);
  frLayerNum followTrackLNum;
  if (lNum - 1 >= getDesign()->getTech()->getBottomLayerNum()
      && getDesign()->getTech()->getLayer(lNum - 1)->getType()
             == dbTechLayerType::ROUTING
      && getDesign()->getTech()->getLayer(lNum - 1)->getDir() == getDir()) {
    followTrackLNum = lNum - 1;
  } else if (lNum + 1 <= getDesign()->getTech()->getTopLayerNum()
             && getDesign()->getTech()->getLayer(lNum + 1)->getType()
                    == dbTechLayerType::ROUTING
             && getDesign()->getTech()->getLayer(lNum + 1)->getDir()
                    == getDir()) {
    followTrackLNum = lNum + 1;
  } else {
    std::cout
        << "Warning: via layer connected to non-routing layer, skipped in "
           "modMinSpacingCostVia\n";
    return;
  }

  // spacing value needed
  frCoord bloatDist = 0;
  for (auto con : getDesign()->getTech()->getLayer(lNum)->getCutSpacing()) {
    bloatDist = std::max(bloatDist, con->getCutSpacing());
  }

  int idx1, idx2;
  if (isH) {
    getTrackIdx(box.yMin() - bloatDist - (viaBox.yMax() - 0) + 1,
                box.yMax() + bloatDist + (0 - viaBox.yMin()) - 1,
                followTrackLNum,
                idx1,
                idx2);
  } else {
    getTrackIdx(box.xMin() - bloatDist - (viaBox.xMax() - 0) + 1,
                box.xMax() + bloatDist + (0 - viaBox.xMin()) - 1,
                followTrackLNum,
                idx1,
                idx2);
  }

  auto& trackLocs = getTrackLocs(followTrackLNum);
  auto& workerRegionQuery = getWorkerRegionQuery();
  odb::Rect tmpBx;
  odb::dbTransform xform;
  frCoord dx, dy, c2ctrackdist;
  frCoord reqDist = 0;
  frCoord maxX, blockLeft, blockRight;
  odb::Rect blockBox;
  odb::Point boxCenter;
  boxCenter = {(box.xMin() + box.xMax()) / 2, (box.yMin() + box.yMax()) / 2};
  bool hasViol = false;
  for (int i = idx1; i <= idx2; i++) {
    auto trackLoc = trackLocs[i];
    if (isH) {
      xform.setOffset(odb::Point(box.xMin(), trackLoc));
    } else {
      xform.setOffset(odb::Point(trackLoc, box.yMin()));
    }
    tmpBx = viaBox;
    xform.apply(tmpBx);
    box2boxDistSquare(box, tmpBx, dx, dy);

    for (auto con : getDesign()->getTech()->getLayer(lNum)->getCutSpacing()) {
      hasViol = false;
      reqDist = con->getCutSpacing();
      bool isC2C = con->hasCenterToCenter();
      if (isH) {
        c2ctrackdist = abs(boxCenter.y() - trackLoc);
      } else {
        c2ctrackdist = abs(boxCenter.x() - trackLoc);
      }

      if (isH) {
        if (isC2C) {
          if (c2ctrackdist >= reqDist) {
            continue;
          }
        } else {
          if (dy >= reqDist) {
            continue;
          }
        }
        if (isC2C) {
          maxX = (frCoord) (sqrt(1.0 * reqDist * reqDist
                                 - 1.0 * c2ctrackdist * c2ctrackdist));
        } else {
          maxX = (frCoord) (sqrt(1.0 * reqDist * reqDist - 1.0 * dy * dy));
        }
        if (maxX * maxX + dy * dy == reqDist * reqDist) {
          maxX = std::max(0, maxX - 1);
        }
        if (isC2C) {
          blockLeft = boxCenter.x() - maxX;
          blockRight = boxCenter.x() + maxX;
        } else {
          blockLeft = box.xMin() - maxX - (viaBox.xMax() - 0);
          blockRight = box.xMax() + maxX + (0 - viaBox.xMin());
        }
        blockBox.init(blockLeft, trackLoc, blockRight, trackLoc);
      } else {
        if (isC2C) {
          if (c2ctrackdist >= reqDist) {
            continue;
          }
        } else {
          if (dx >= reqDist) {
            continue;
          }
        }
        if (isC2C) {
          maxX = (frCoord) (sqrt(1.0 * reqDist * reqDist
                                 - 1.0 * c2ctrackdist * c2ctrackdist));
        } else {
          maxX = (frCoord) (sqrt(1.0 * reqDist * reqDist - 1.0 * dx * dx));
        }
        if (maxX * maxX + dx * dx == reqDist * reqDist) {
          maxX = std::max(0, maxX - 1);
        }
        if (isC2C) {
          blockLeft = boxCenter.y() - maxX;
          blockRight = boxCenter.y() + maxX;
        } else {
          blockLeft = box.yMin() - maxX - (viaBox.yMax() - 0);
          blockRight = box.yMax() + maxX + (0 - viaBox.yMin());
        }

        blockBox.init(trackLoc, blockLeft, trackLoc, blockRight);
      }
      if (con->hasSameNet()) {
        continue;
      }
      if (con->isLayer()) {
        ;
      } else if (con->isAdjacentCuts()) {
        hasViol = true;
        // should disable hasViol and modify this part to new grid graph
      } else if (con->isParallelOverlap()) {
        if (isH) {
          if (dy > 0) {
            blockBox.init(
                std::max(box.xMin() - (viaBox.xMax() - 0) + 1, blockLeft),
                trackLoc,
                std::min(box.xMax() + (0 - viaBox.xMin()) - 1, blockRight),
                trackLoc);
          }
        } else {
          if (dx > 0) {
            blockBox.init(
                trackLoc,
                std::max(box.yMin() - (viaBox.yMax() - 0) + 1, blockLeft),
                trackLoc,
                std::min(box.yMax() + (0 - viaBox.yMin()) - 1, blockRight));
          }
        }
        if (blockBox.xMin() <= blockBox.xMax()
            && blockBox.yMin() <= blockBox.yMax()) {
          hasViol = true;
        }
      } else if (con->isArea()) {
        auto currArea = std::max(box.area(), tmpBx.area());
        if (currArea >= con->getCutArea()) {
          hasViol = true;
        }
      } else {
        hasViol = true;
      }
      if (hasViol) {
        if (isAddCost) {
          workerRegionQuery.addViaCost(blockBox, lNum, fig, con);
          if (pinS) {
            workerRegionQuery.query(blockBox, lNum, *pinS);
          }
        } else {
          workerRegionQuery.removeViaCost(blockBox, lNum, fig, con);
          if (pinS) {
            workerRegionQuery.query(blockBox, lNum, *pinS);
          }
        }
      }
    }
  }
}

void FlexTAWorker::addCost(taPinFig* fig, frOrderedIdSet<taPin*>* pinS)
{
  modCost(fig, true, pinS);
}

void FlexTAWorker::subCost(taPinFig* fig, frOrderedIdSet<taPin*>* pinS)
{
  modCost(fig, false, pinS);
}

void FlexTAWorker::modCost(taPinFig* fig,
                           bool isAddCost,
                           frOrderedIdSet<taPin*>* pinS)
{
  if (fig->typeId() == tacPathSeg) {
    auto obj = static_cast<taPathSeg*>(fig);
    auto layerNum = obj->getLayerNum();
    odb::Rect box = obj->getBBox();
    modMinSpacingCostPlanar(
        box, layerNum, obj, isAddCost, pinS);  // must be current TA layer
    modMinSpacingCostVia(box, layerNum, obj, isAddCost, true, true, pinS);
    modMinSpacingCostVia(box, layerNum, obj, isAddCost, false, true, pinS);
  } else if (fig->typeId() == tacVia) {
    auto obj = static_cast<taVia*>(fig);
    // assumes enclosure for via is always rectangle
    odb::Rect box = obj->getLayer1BBox();
    auto layerNum = obj->getViaDef()->getLayer1Num();
    // current TA layer
    if (getDir() == getDesign()->getTech()->getLayer(layerNum)->getDir()) {
      modMinSpacingCostPlanar(box, layerNum, obj, isAddCost, pinS);
    }
    modMinSpacingCostVia(box, layerNum, obj, isAddCost, true, false, pinS);
    modMinSpacingCostVia(box, layerNum, obj, isAddCost, false, false, pinS);

    // assumes enclosure for via is always rectangle
    box = obj->getLayer2BBox();
    layerNum = obj->getViaDef()->getLayer2Num();
    // current TA layer
    if (getDir() == getDesign()->getTech()->getLayer(layerNum)->getDir()) {
      modMinSpacingCostPlanar(box, layerNum, obj, isAddCost, pinS);
    }
    modMinSpacingCostVia(box, layerNum, obj, isAddCost, true, false, pinS);
    modMinSpacingCostVia(box, layerNum, obj, isAddCost, false, false, pinS);

    odb::Point pt = obj->getOrigin();
    odb::dbTransform xform(pt);
    for (auto& uFig : obj->getViaDef()->getCutFigs()) {
      auto rect = static_cast<frRect*>(uFig.get());
      box = rect->getBBox();
      xform.apply(box);
      layerNum = obj->getViaDef()->getCutLayerNum();
      modCutSpacingCost(box, layerNum, obj, isAddCost, pinS);
    }
  } else {
    std::cout << "Error: unsupported region query add\n";
  }
}

void FlexTAWorker::assignIroute_availTracks(taPin* iroute,
                                            frLayerNum& lNum,
                                            int& idx1,
                                            int& idx2)
{
  lNum = iroute->getGuide()->getBeginLayerNum();
  auto [gbp, gep] = iroute->getGuide()->getPoints();
  odb::Point gIdx = getDesign()->getTopBlock()->getGCellIdx(gbp);
  odb::Rect gBox = getDesign()->getTopBlock()->getGCellBox(gIdx);
  bool isH = (getDir() == dbTechLayerDir::HORIZONTAL);
  frCoord coordLow = isH ? gBox.yMin() : gBox.xMin();
  frCoord coordHigh = isH ? gBox.yMax() : gBox.xMax();
  coordHigh--;  // to avoid higher track == guide top/right
  if (getTech()->getLayer(lNum)->isUnidirectional()) {
    const odb::Rect& dieBx = design_->getTopBlock()->getDieBox();
    const frViaDef* via = nullptr;
    odb::Rect testBox;
    if (lNum + 1 <= getTech()->getTopLayerNum()) {
      via = getTech()->getLayer(lNum + 1)->getDefaultViaDef();
      testBox = via->getLayer1ShapeBox();
      testBox.merge(via->getLayer2ShapeBox());
    } else {
      via = getTech()->getLayer(lNum - 1)->getDefaultViaDef();
      testBox = via->getLayer1ShapeBox();
      testBox.merge(via->getLayer2ShapeBox());
    }
    int diffLow, diffHigh;
    if (isH) {
      diffLow = dieBx.yMin() - (coordLow - testBox.dy() / 2);
      diffHigh = coordHigh + testBox.dy() / 2 - dieBx.yMax();
    } else {
      diffLow = dieBx.xMin() - (coordLow - testBox.dx() / 2);
      diffHigh = coordHigh + testBox.dx() / 2 - dieBx.xMax();
    }
    if (diffLow > 0) {
      coordLow += diffLow;
    }
    if (diffHigh > 0) {
      coordHigh -= diffHigh;
    }
  }
  getTrackIdx(coordLow, coordHigh, lNum, idx1, idx2);
  if (idx2 < idx1) {
    const double dbu = getDesign()->getTopBlock()->getDBUPerUU();
    logger_->error(DRT,
                   406,
                   "No {} tracks found in ({}, {}) for layer {}",
                   isH ? "horizontal" : "vertical",
                   coordLow / dbu,
                   coordHigh / dbu,
                   getTech()->getLayer(lNum)->getName());
  }
}

// This adds a cost based on the connected iroutes. For a vertical iroute if
// there are more connected iroutes to the right, we should force this iroute
// towards the right of the gcell (and vice versa). The cost is scaled based on
// the net number of iroutes in that direction.
frUInt4 FlexTAWorker::assignIroute_getNextIrouteDirCost(taPin* iroute,
                                                        frCoord trackLoc)
{
  auto guide = iroute->getGuide();
  bool isH = (getDir() == dbTechLayerDir::HORIZONTAL);
  auto [begin, end] = guide->getPoints();
  odb::Point idx = getDesign()->getTopBlock()->getGCellIdx(end);
  odb::Rect endBox = getDesign()->getTopBlock()->getGCellBox(idx);
  int nextIrouteDirCost = 0;
  auto nextIrouteDir = iroute->getNextIrouteDir();
  if (nextIrouteDir <= 0) {
    if (isH) {
      nextIrouteDirCost = abs(nextIrouteDir) * (trackLoc - endBox.yMin());
    } else {
      nextIrouteDirCost = abs(nextIrouteDir) * (trackLoc - endBox.xMin());
    }
  } else {
    if (isH) {
      nextIrouteDirCost = abs(nextIrouteDir) * (endBox.yMax() - trackLoc);
    } else {
      nextIrouteDirCost = abs(nextIrouteDir) * (endBox.xMax() - trackLoc);
    }
  }
  if (nextIrouteDirCost < 0) {
    double dbu = getDesign()->getTopBlock()->getDBUPerUU();
    std::cout << "Error: nextIrouteDirCost < 0" << ", trackLoc@"
              << trackLoc / dbu << " box (" << endBox.xMin() / dbu << ", "
              << endBox.yMin() / dbu << ") (" << endBox.xMax() / dbu << ", "
              << endBox.yMax() / dbu << ")\n";
    return (frUInt4) 0;
  }
  return (frUInt4) nextIrouteDirCost;
}

frUInt4 FlexTAWorker::assignIroute_getPinCost(taPin* iroute, frCoord trackLoc)
{
  frUInt4 sol = 0;
  if (iroute->hasPinCoord()) {
    sol = abs(trackLoc - iroute->getPinCoord());

    // add cost to locations that will cause forbidden via spacing to
    // boundary pin
    auto layerNum = iroute->getGuide()->getBeginLayerNum();
    auto layer = getTech()->getLayer(layerNum);

    if (layer->isUnidirectional()) {
      bool isH = (getDir() == dbTechLayerDir::HORIZONTAL);
      int zIdx = layerNum / 2 - 1;
      if (sol) {
        if (isH) {
          // if cannot use bottom or upper layer to bridge, then add cost
          if ((getTech()->isVia2ViaForbiddenLen(
                   zIdx, false, false, false, sol, nullptr)
               || layerNum - 2 < router_cfg_->BOTTOM_ROUTING_LAYER)
              && (getTech()->isVia2ViaForbiddenLen(
                      zIdx, true, true, false, sol, nullptr)
                  || layerNum + 2 > getTech()->getTopLayerNum())) {
            sol += router_cfg_->TADRCCOST;
          }
        } else {
          if ((getTech()->isVia2ViaForbiddenLen(
                   zIdx, false, false, true, sol, nullptr)
               || layerNum - 2 < router_cfg_->BOTTOM_ROUTING_LAYER)
              && (getTech()->isVia2ViaForbiddenLen(
                      zIdx, true, true, true, sol, nullptr)
                  || layerNum + 2 > getTech()->getTopLayerNum())) {
            sol += router_cfg_->TADRCCOST;
          }
        }
      }
    }
  }
  return sol;
}

frUInt4 FlexTAWorker::assignIroute_getDRCCost_helper(taPin* iroute,
                                                     odb::Rect& box,
                                                     frLayerNum lNum)
{
  auto layer = getDesign()->getTech()->getLayer(lNum);
  auto& workerRegionQuery = getWorkerRegionQuery();
  std::vector<rq_box_value_t<std::pair<frBlockObject*, frConstraint*>>> result;
  int overlap = 0;
  if (iroute->getGuide()->getNet()->getNondefaultRule()) {
    int r = iroute->getGuide()->getNet()->getNondefaultRule()->getWidth(lNum / 2
                                                                        - 1)
            / 2;
    r += iroute->getGuide()->getNet()->getNondefaultRule()->getSpacing(lNum / 2
                                                                       - 1);
    box.bloat(r, box);
  }
  workerRegionQuery.queryCost(box, lNum, result);

  auto getPartialBox = [this, lNum](odb::Rect box, bool begin) {
    auto layer = getTech()->getLayer(lNum);
    odb::Rect result;
    frCoord addHorz = 0;
    frCoord addVert = 0;
    if (layer->isHorizontal()) {
      addHorz = getDesign()->getTopBlock()->getGCellSizeHorizontal() / 2;
    } else {
      addVert = getDesign()->getTopBlock()->getGCellSizeVertical() / 2;
    }
    if (begin) {
      result.reset(box.xMin(),
                   box.yMin(),
                   std::min(box.xMax(), box.xMin() + addHorz),
                   std::min(box.yMax(), box.yMin() + addVert));
    } else {
      result.reset(std::max(box.xMin(), box.xMax() - addHorz),
                   std::max(box.yMin(), box.yMax() - addVert),
                   box.xMax(),
                   box.yMax());
    }
    return result;
  };
  std::vector<rq_box_value_t<std::pair<frBlockObject*, frConstraint*>>>
      tmpResult;
  if (layer->getType() == dbTechLayerType::CUT) {
    workerRegionQuery.queryViaCost(box, lNum, tmpResult);
    result.insert(result.end(), tmpResult.begin(), tmpResult.end());
  } else {
    odb::Rect tmpBox = getPartialBox(box, true);
    workerRegionQuery.queryViaCost(tmpBox, lNum, tmpResult);
    result.insert(result.end(), tmpResult.begin(), tmpResult.end());
    tmpResult.clear();
    tmpBox = getPartialBox(box, false);
    workerRegionQuery.queryViaCost(tmpBox, lNum, tmpResult);
    result.insert(result.end(), tmpResult.begin(), tmpResult.end());
  }
  bool isCut = false;

  // save same net overlaps
  std::vector<odb::Rect> sameNetOverlaps;
  for (auto& [bounds, pr] : result) {
    auto& [obj, con] = pr;
    if (obj != nullptr && obj->typeId() == frcNet) {
      if (iroute->getGuide()->getNet() == obj) {
        sameNetOverlaps.push_back(bounds);
      }
    }
  }

  for (auto& [bounds, pr] : result) {
    // if the overlap bounds intersect with the pin connection, do not add drc
    // cost
    bool pinConn = false;
    for (const odb::Rect& sameNetOverlap : sameNetOverlaps) {
      if (sameNetOverlap.intersects(bounds)) {
        pinConn = true;
        break;
      }
    }
    if (pinConn) {
      continue;
    }

    auto& [obj, con] = pr;
    frCoord tmpOvlp = -std::max(box.xMin(), bounds.xMin())
                      + std::min(box.xMax(), bounds.xMax())
                      - std::max(box.yMin(), bounds.yMin())
                      + std::min(box.yMax(), bounds.yMax()) + 1;
    if (tmpOvlp <= 0) {
      logger_->error(DRT,
                     412,
                     "assignIroute_getDRCCost_helper overlap value is {}.",
                     tmpOvlp);
    }
    // unknown obj, always add cost
    if (obj == nullptr) {
      overlap += tmpOvlp;
      // only add cost for diff-net
    } else if (obj->typeId() == frcNet) {
      if (iroute->getGuide()->getNet() != obj) {
        overlap += tmpOvlp;
      }
      // two taObjs
    } else if (obj->typeId() == tacPathSeg || obj->typeId() == tacVia) {
      auto taObj = static_cast<taPinFig*>(obj);
      // can exclude same iroute objs also
      if (taObj->getPin() == iroute) {
        continue;
      }
      if (iroute->getGuide()->getNet()
          != taObj->getPin()->getGuide()->getNet()) {
        overlap += tmpOvlp;
      }
    } else {
      std::cout << "Warning: assignIroute_getDRCCost_helper unsupported type"
                << '\n';
    }
  }
  frCoord pitch = 0;
  if (getDesign()->getTech()->getLayer(lNum)->getType()
      == dbTechLayerType::ROUTING) {
    pitch = getDesign()->getTech()->getLayer(lNum)->getPitch();
    isCut = false;
  } else if (getDesign()->getTech()->getTopLayerNum() >= lNum + 1
             && getDesign()->getTech()->getLayer(lNum + 1)->getType()
                    == dbTechLayerType::ROUTING) {
    pitch = getDesign()->getTech()->getLayer(lNum + 1)->getPitch();
    isCut = true;
  } else if (getDesign()->getTech()->getBottomLayerNum() <= lNum - 1
             && getDesign()->getTech()->getLayer(lNum - 1)->getType()
                    == dbTechLayerType::ROUTING) {
    pitch = getDesign()->getTech()->getLayer(lNum - 1)->getPitch();
    isCut = true;
  } else {
    std::cout << "Error: assignIroute_getDRCCost_helper unknown layer type"
              << '\n';
    exit(1);
  }
  // always penalize two pitch per cut, regardless of cnts
  if (overlap == 0) {
    return 0;
  }
  return isCut ? pitch * 2 : std::max(pitch * 2, overlap);
}

frUInt4 FlexTAWorker::assignIroute_getDRCCost(taPin* iroute, frCoord trackLoc)
{
  frUInt4 cost = 0;
  bool isH = (getDir() == dbTechLayerDir::HORIZONTAL);
  for (auto& uPinFig : iroute->getFigs()) {
    if (uPinFig->typeId() == tacPathSeg) {
      auto obj = static_cast<taPathSeg*>(uPinFig.get());
      auto [bp, ep] = obj->getPoints();
      if (isH) {
        bp = {bp.x(), trackLoc};
        ep = {ep.x(), trackLoc};
      } else {
        bp = {trackLoc, bp.y()};
        ep = {trackLoc, ep.y()};
      }
      odb::Rect bbox(bp, ep);
      frUInt4 wireCost
          = assignIroute_getDRCCost_helper(iroute, bbox, obj->getLayerNum());
      cost += wireCost;
    } else if (uPinFig->typeId() == tacVia) {
      auto obj = static_cast<taVia*>(uPinFig.get());
      auto bp = obj->getOrigin();
      if (isH) {
        bp = {bp.x(), trackLoc};
      } else {
        bp = {trackLoc, bp.y()};
      }
      odb::Rect bbox(bp, bp);
      frUInt4 viaCost = assignIroute_getDRCCost_helper(
          iroute, bbox, obj->getViaDef()->getCutLayerNum());
      cost += viaCost;
    } else {
      std::cout << "Error: assignIroute_updateIroute unsupported pinFig"
                << '\n';
      exit(1);
    }
  }
  return cost;
}

frUInt4 FlexTAWorker::assignIroute_getAlignCost(taPin* iroute, frCoord trackLoc)
{
  frUInt4 sol = 0;
  frCoord pitch = 0;
  bool isH = (getDir() == dbTechLayerDir::HORIZONTAL);
  for (auto& uPinFig : iroute->getFigs()) {
    if (uPinFig->typeId() == tacPathSeg) {
      auto obj = static_cast<taPathSeg*>(uPinFig.get());
      auto [bp, ep] = obj->getPoints();
      auto lNum = obj->getLayerNum();
      pitch = getDesign()->getTech()->getLayer(lNum)->getPitch();
      auto& workerRegionQuery = getWorkerRegionQuery();
      frOrderedIdSet<taPin*> result;
      odb::Rect box;
      if (isH) {
        box.init(bp.x(), trackLoc, ep.x(), trackLoc);
      } else {
        box.init(trackLoc, bp.y(), trackLoc, ep.y());
      }
      workerRegionQuery.query(box, lNum, result);
      for (auto& iroute2 : result) {
        if (iroute2->getGuide()->getNet() == iroute->getGuide()->getNet()) {
          sol = 1;
          break;
        }
      }
    }
    if (sol == 1) {
      break;
    }
  }
  return pitch * sol;
}

frUInt4 FlexTAWorker::assignIroute_getCost(taPin* iroute,
                                           frCoord trackLoc,
                                           frUInt4& outDrcCost)
{
  frCoord irouteLayerPitch
      = getTech()->getLayer(iroute->getGuide()->getBeginLayerNum())->getPitch();
  outDrcCost = assignIroute_getDRCCost(iroute, trackLoc);
  int drcCost = (isInitTA()) ? (0.05 * outDrcCost)
                             : (router_cfg_->TADRCCOST * outDrcCost);
  int nextIrouteDirCost = assignIroute_getNextIrouteDirCost(iroute, trackLoc);
  // int pinCost    = TAPINCOST * assignIroute_getPinCost(iroute, trackLoc);
  int tmpPinCost = assignIroute_getPinCost(iroute, trackLoc);
  int pinCost = (tmpPinCost == 0)
                    ? 0
                    : router_cfg_->TAPINCOST * irouteLayerPitch + tmpPinCost;
  int tmpAlignCost = assignIroute_getAlignCost(iroute, trackLoc);
  int alignCost
      = (tmpAlignCost == 0)
            ? 0
            : router_cfg_->TAALIGNCOST * irouteLayerPitch + tmpAlignCost;
  return std::max(drcCost + nextIrouteDirCost + pinCost - alignCost, 0);
}

void FlexTAWorker::assignIroute_bestTrack_helper(taPin* iroute,
                                                 frLayerNum lNum,
                                                 int trackIdx,
                                                 frUInt4& bestCost,
                                                 frCoord& bestTrackLoc,
                                                 int& bestTrackIdx,
                                                 frUInt4& drcCost)
{
  auto trackLoc = getTrackLocs(lNum)[trackIdx];
  auto currCost = assignIroute_getCost(iroute, trackLoc, drcCost);
  if (isInitTA()) {
    if (currCost < bestCost) {
      bestCost = currCost;
      bestTrackLoc = trackLoc;
      bestTrackIdx = trackIdx;
    }
  } else {
    if (drcCost < bestCost) {
      bestCost = drcCost;
      bestTrackLoc = trackLoc;
      bestTrackIdx = trackIdx;
    }
  }
}

int FlexTAWorker::assignIroute_bestTrack(taPin* iroute,
                                         frLayerNum lNum,
                                         int idx1,
                                         int idx2)
{
  double dbu = getDesign()->getTopBlock()->getDBUPerUU();
  frCoord bestTrackLoc = 0;
  int bestTrackIdx = -1;
  frUInt4 bestCost = std::numeric_limits<frUInt4>::max();
  frUInt4 drcCost = 0;
  if (iroute->hasPinCoord()) {
    // std::cout <<"if" <<std::endl;
    frCoord pinCoord = iroute->getPinCoord();
    if (iroute->getNextIrouteDir() > 0) {
      int startTrackIdx
          = int(std::lower_bound(
                    trackLocs_[lNum].begin(), trackLocs_[lNum].end(), pinCoord)
                - trackLocs_[lNum].begin());
      startTrackIdx = std::min(startTrackIdx, idx2);
      startTrackIdx = std::max(startTrackIdx, idx1);
      for (int i = startTrackIdx; i <= idx2; i++) {
        assignIroute_bestTrack_helper(
            iroute, lNum, i, bestCost, bestTrackLoc, bestTrackIdx, drcCost);
        if (!drcCost) {
          break;
        }
      }
      if (drcCost) {
        for (int i = startTrackIdx - 1; i >= idx1; i--) {
          assignIroute_bestTrack_helper(
              iroute, lNum, i, bestCost, bestTrackLoc, bestTrackIdx, drcCost);
          if (!drcCost) {
            break;
          }
        }
      }
    } else if (iroute->getNextIrouteDir() == 0) {
      int startTrackIdx
          = int(std::lower_bound(
                    trackLocs_[lNum].begin(), trackLocs_[lNum].end(), pinCoord)
                - trackLocs_[lNum].begin());
      startTrackIdx = std::min(startTrackIdx, idx2);
      startTrackIdx = std::max(startTrackIdx, idx1);
      // std::cout <<"startTrackIdx " <<startTrackIdx <<std::endl;
      for (int i = 0; i <= idx2 - idx1; i++) {
        int currTrackIdx = startTrackIdx + i;
        if (currTrackIdx >= idx1 && currTrackIdx <= idx2) {
          assignIroute_bestTrack_helper(iroute,
                                        lNum,
                                        currTrackIdx,
                                        bestCost,
                                        bestTrackLoc,
                                        bestTrackIdx,
                                        drcCost);
        }
        if (!drcCost) {
          break;
        }
        currTrackIdx = startTrackIdx - i - 1;
        if (currTrackIdx >= idx1 && currTrackIdx <= idx2) {
          assignIroute_bestTrack_helper(iroute,
                                        lNum,
                                        currTrackIdx,
                                        bestCost,
                                        bestTrackLoc,
                                        bestTrackIdx,
                                        drcCost);
        }
        if (!drcCost) {
          break;
        }
      }
    } else {
      int startTrackIdx
          = int(std::lower_bound(
                    trackLocs_[lNum].begin(), trackLocs_[lNum].end(), pinCoord)
                - trackLocs_[lNum].begin());
      startTrackIdx = std::min(startTrackIdx, idx2);
      startTrackIdx = std::max(startTrackIdx, idx1);
      for (int i = startTrackIdx; i >= idx1; i--) {
        assignIroute_bestTrack_helper(
            iroute, lNum, i, bestCost, bestTrackLoc, bestTrackIdx, drcCost);
        if (!drcCost) {
          break;
        }
      }
      if (drcCost) {
        for (int i = startTrackIdx + 1; i <= idx2; i++) {
          assignIroute_bestTrack_helper(
              iroute, lNum, i, bestCost, bestTrackLoc, bestTrackIdx, drcCost);
          if (!drcCost) {
            break;
          }
        }
      }
    }
  } else {
    if (iroute->getNextIrouteDir() > 0) {
      for (int i = idx2; i >= idx1; i--) {
        assignIroute_bestTrack_helper(
            iroute, lNum, i, bestCost, bestTrackLoc, bestTrackIdx, drcCost);
        if (!drcCost) {
          break;
        }
      }
    } else if (iroute->getNextIrouteDir() == 0) {
      for (int i = (idx1 + idx2) / 2; i <= idx2; i++) {
        assignIroute_bestTrack_helper(
            iroute, lNum, i, bestCost, bestTrackLoc, bestTrackIdx, drcCost);
        if (!drcCost) {
          break;
        }
      }
      if (drcCost) {
        for (int i = (idx1 + idx2) / 2 - 1; i >= idx1; i--) {
          assignIroute_bestTrack_helper(
              iroute, lNum, i, bestCost, bestTrackLoc, bestTrackIdx, drcCost);
          if (!drcCost) {
            break;
          }
        }
      }
    } else {
      for (int i = idx1; i <= idx2; i++) {
        assignIroute_bestTrack_helper(
            iroute, lNum, i, bestCost, bestTrackLoc, bestTrackIdx, drcCost);
        if (!drcCost) {
          break;
        }
      }
    }
  }
  if (bestTrackIdx == -1) {
    auto guide = iroute->getGuide();
    odb::Rect box = guide->getBBox();
    std::cout << "Error: assignIroute_bestTrack select no track for "
              << guide->getNet()->getName() << " @(" << box.xMin() / dbu << ", "
              << box.yMin() / dbu << ") (" << box.xMax() / dbu << ", "
              << box.yMax() / dbu << " "
              << getDesign()->getTech()->getLayer(lNum)->getName()
              << " idx1/2=" << idx1 << "/" << idx2 << '\n';
    exit(1);
  }
  totCost_ += drcCost;
  iroute->setCost(drcCost);
  return bestTrackLoc;
}

void FlexTAWorker::assignIroute_updateIroute(taPin* iroute,
                                             frCoord bestTrackLoc,
                                             frOrderedIdSet<taPin*>* pinS)
{
  auto& workerRegionQuery = getWorkerRegionQuery();
  bool isH = (getDir() == dbTechLayerDir::HORIZONTAL);

  // update coord
  for (auto& uPinFig : iroute->getFigs()) {
    if (uPinFig->typeId() == tacPathSeg) {
      auto obj = static_cast<taPathSeg*>(uPinFig.get());
      auto [bp, ep] = obj->getPoints();
      if (isH) {
        bp = {bp.x(), bestTrackLoc};
        ep = {ep.x(), bestTrackLoc};
      } else {
        bp = {bestTrackLoc, bp.y()};
        ep = {bestTrackLoc, ep.y()};
      }
      obj->setPoints(bp, ep);
    } else if (uPinFig->typeId() == tacVia) {
      auto obj = static_cast<taVia*>(uPinFig.get());
      auto bp = obj->getOrigin();
      if (isH) {
        bp = {bp.x(), bestTrackLoc};
      } else {
        bp = {bestTrackLoc, bp.y()};
      }
      obj->setOrigin(bp);
    } else {
      std::cout << "Error: assignIroute_updateIroute unsupported pinFig"
                << '\n';
      exit(1);
    }
  }
  // addCost
  for (auto& uPinFig : iroute->getFigs()) {
    addCost(uPinFig.get(), isInitTA() ? nullptr : pinS);
    workerRegionQuery.add(uPinFig.get());
  }
  iroute->addNumAssigned();
}

void FlexTAWorker::assignIroute_init(taPin* iroute,
                                     frOrderedIdSet<taPin*>* pinS)
{
  auto& workerRegionQuery = getWorkerRegionQuery();
  // subCost
  if (!isInitTA()) {
    for (auto& uPinFig : iroute->getFigs()) {
      workerRegionQuery.remove(uPinFig.get());
      subCost(uPinFig.get(), pinS);
    }
    totCost_ -= iroute->getCost();
  }
}

void FlexTAWorker::assignIroute_updateOthers(frOrderedIdSet<taPin*>& pinS)
{
  bool isH = (getDir() == dbTechLayerDir::HORIZONTAL);
  if (isInitTA()) {
    return;
  }
  for (auto& iroute : pinS) {
    if (iroute->getGuide()->getNet()->isClock() && !hardIroutesMode_) {
      continue;
    }
    removeFromReassignIroutes(iroute);
    // recalculate cost
    frUInt4 drcCost = 0;
    frCoord trackLoc = std::numeric_limits<frCoord>::max();
    for (auto& uPinFig : iroute->getFigs()) {
      if (uPinFig->typeId() == tacPathSeg) {
        auto [bp, ep] = static_cast<taPathSeg*>(uPinFig.get())->getPoints();
        if (isH) {
          trackLoc = bp.y();
        } else {
          trackLoc = bp.x();
        }
        break;
      }
    }
    if (trackLoc == std::numeric_limits<frCoord>::max()) {
      std::cout
          << "Error: FlexTAWorker::assignIroute_updateOthers does not find "
             "trackLoc"
          << '\n';
      exit(1);
    }
    totCost_ -= iroute->getCost();
    assignIroute_getCost(iroute, trackLoc, drcCost);
    iroute->setCost(drcCost);
    totCost_ += iroute->getCost();
    if (drcCost && iroute->getNumAssigned() < maxRetry_) {
      addToReassignIroutes(iroute);
    }
  }
}

void FlexTAWorker::assignIroute(taPin* iroute)
{
  frOrderedIdSet<taPin*> pinS;
  assignIroute_init(iroute, &pinS);
  frLayerNum lNum;
  int idx1, idx2;
  assignIroute_availTracks(iroute, lNum, idx1, idx2);
  auto bestTrackLoc = assignIroute_bestTrack(iroute, lNum, idx1, idx2);

  assignIroute_updateIroute(iroute, bestTrackLoc, &pinS);
  assignIroute_updateOthers(pinS);
}

void FlexTAWorker::assign()
{
  int maxBufferSize = 20;
  std::vector<taPin*> buffers(maxBufferSize, nullptr);
  int currBufferIdx = 0;
  auto iroute = popFromReassignIroutes();
  while (iroute != nullptr) {
    auto it = std::ranges::find(buffers, iroute);
    // in the buffer, skip
    if (it != buffers.end() || iroute->getNumAssigned() >= maxRetry_) {
      ;
      // not in the buffer, re-assign
    } else {
      assignIroute(iroute);
      // re add last buffer item to reassigniroutes if drccost > 0
      // if (buffers[currBufferIdx]) {
      //  if (buffers[currBufferIdx]->getDrcCost()) {
      //    addToReassignIroutes(buffers[currBufferIdx]);
      //  }
      //}
      buffers[currBufferIdx] = iroute;
      currBufferIdx = (currBufferIdx + 1) % maxBufferSize;
      numAssigned_++;
    }
    iroute = popFromReassignIroutes();
  }
}

}  // namespace drt
