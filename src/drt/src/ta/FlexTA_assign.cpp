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

#include <cmath>

#include "ta/FlexTA.h"

using namespace std;
using namespace fr;

frSquaredDistance FlexTAWorker::box2boxDistSquare(const Rect& box1,
                                                  const Rect& box2,
                                                  frCoord& dx,
                                                  frCoord& dy)
{
  dx = max(max(box1.xMin(), box2.xMin()) - min(box1.xMax(), box2.xMax()), 0);
  dy = max(max(box1.yMin(), box2.yMin()) - min(box1.yMax(), box2.yMax()), 0);
  return (frSquaredDistance) dx * dx + (frSquaredDistance) dy * dy;
}

// must be current TA layer
void FlexTAWorker::modMinSpacingCostPlanar(const Rect& box,
                                           frLayerNum lNum,
                                           taPinFig* fig,
                                           bool isAddCost,
                                           set<taPin*, frBlockObjectComp>* pinS)
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
  if (fig->getNet()->getNondefaultRule())
    bloatDist
        = max(bloatDist,
              fig->getNet()->getNondefaultRule()->getSpacing(lNum / 2 - 1));

  frSquaredDistance bloatDistSquare = (frSquaredDistance) bloatDist * bloatDist;

  bool isH = (getDir() == dbTechLayerDir::HORIZONTAL);

  // now assume track in H direction
  frCoord boxLow = isH ? box.yMin() : box.xMin();
  frCoord boxHigh = isH ? box.yMax() : box.xMax();
  frCoord boxLeft = isH ? box.xMin() : box.yMin();
  frCoord boxRight = isH ? box.xMax() : box.yMax();
  Rect box1(boxLeft, boxLow, boxRight, boxHigh);

  int idx1, idx2;
  getTrackIdx(boxLow - bloatDist - halfwidth2 + 1,
              boxHigh + bloatDist + halfwidth2 - 1,
              lNum,
              idx1,
              idx2);

  Rect box2(-halfwidth2, -halfwidth2, halfwidth2, halfwidth2);
  dbTransform xform;
  frCoord dx, dy;
  auto& trackLocs = getTrackLocs(lNum);
  auto& workerRegionQuery = getWorkerRegionQuery();
  for (int i = idx1; i <= idx2; i++) {
    auto trackLoc = trackLocs[i];
    xform.setOffset(Point(boxLeft, trackLoc));
    xform.apply(box2);
    box2boxDistSquare(box1, box2, dx, dy);
    if (dy >= bloatDist) {
      continue;
    }
    frCoord maxX = (frCoord) (sqrt(1.0 * bloatDistSquare
                                   - 1.0 * (frSquaredDistance) dy * dy));
    if ((frSquaredDistance) maxX * maxX + (frSquaredDistance) dy * dy
        == bloatDistSquare) {
      maxX = max(0, maxX - 1);
    }
    frCoord blockLeft = boxLeft - maxX - halfwidth2;
    frCoord blockRight = boxRight + maxX + halfwidth2;
    Rect tmpBox;
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
void FlexTAWorker::modMinSpacingCostVia(const Rect& box,
                                        frLayerNum lNum,
                                        taPinFig* fig,
                                        bool isAddCost,
                                        bool isUpperVia,
                                        bool isCurrPs,
                                        set<taPin*, frBlockObjectComp>* pinS)
{
  // obj1 = curr obj
  frCoord width1 = box.minDXDY();
  frCoord length1 = box.maxDXDY();
  // obj2 = other obj
  // default via dimension
  frViaDef* viaDef = nullptr;
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
  Rect viaBox(0, 0, 0, 0);
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
    cout << "Warning: via layer connected to non-routing layer, skipped in "
            "modMinSpacingCostVia"
         << endl;
    return;
  }

  // spacing value needed
  auto layer = getTech()->getLayer(lNum);
  frCoord bloatDist = layer->getMinSpacingValue(
      width1, width2, isCurrPs ? length2 : min(length1, length2), false);
  if (fig->getNet()->getNondefaultRule())
    bloatDist
        = max(bloatDist,
              fig->getNet()->getNondefaultRule()->getSpacing(lNum / 2 - 1));
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
  Rect tmpBx;
  dbTransform xform;
  frCoord dx, dy, prl;
  frCoord maxX, blockLeft, blockRight;
  Rect blockBox;
  for (int i = idx1; i <= idx2; i++) {
    auto trackLoc = trackLocs[i];
    if (isH) {
      xform.setOffset(Point(box.xMin(), trackLoc));
    } else {
      xform.setOffset(Point(trackLoc, box.yMin()));
    }
    tmpBx = viaBox;
    xform.apply(tmpBx);
    box2boxDistSquare(box, tmpBx, dx, dy);
    if (isH) {           // track is horizontal
      if (dy > 0) {      // via at the bottom of box
        if (isCurrPs) {  // prl maxed out to be viaBox
          prl = viaBox.xMax() - viaBox.xMin();
        } else {  // prl maxed out to be smaller of box and viaBox
          prl = min(box.xMax() - box.xMin(), viaBox.xMax() - viaBox.xMin());
        }
        // via at the side of box
      } else {
        if (isCurrPs) {  // prl maxed out to be viaBox
          prl = viaBox.yMax() - viaBox.yMin();
        } else {  // prl maxed out to be smaller of box and viaBox
          prl = min(box.yMax() - box.yMin(), viaBox.yMax() - viaBox.yMin());
        }
      }
    } else {             // track is vertical
      if (dx > 0) {      // via at the bottom of box
        if (isCurrPs) {  // prl maxed out to be viaBox
          prl = viaBox.yMax() - viaBox.yMin();
        } else {  // prl maxed out to be smaller of box and viaBox
          prl = min(box.yMax() - box.yMin(), viaBox.yMax() - viaBox.yMin());
        }
        // via at the side of box
      } else {
        if (isCurrPs) {  // prl maxed out to be viaBox
          prl = viaBox.xMax() - viaBox.xMin();
        } else {  // prl maxed out to be smaller of box and viaBox
          prl = min(box.xMax() - box.xMin(), viaBox.xMax() - viaBox.xMin());
        }
      }
    }

    frCoord reqDist = layer->getMinSpacingValue(width1, width2, prl, false);
    if (fig->getNet()->getNondefaultRule())
      reqDist
          = max(reqDist,
                fig->getNet()->getNondefaultRule()->getSpacing(lNum / 2 - 1));

    if (isH) {
      if (dy >= reqDist) {
        continue;
      }
      maxX = (frCoord) (sqrt(1.0 * reqDist * reqDist - 1.0 * dy * dy));
      if (maxX * maxX + dy * dy == reqDist * reqDist) {
        maxX = max(0, maxX - 1);
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
        maxX = max(0, maxX - 1);
      }
      blockLeft = box.yMin() - maxX - (viaBox.yMax() - 0);
      blockRight = box.yMax() + maxX + (0 - viaBox.yMin());

      blockBox.init(trackLoc, blockLeft, trackLoc, blockRight);
    }

    auto con = layer->getMinSpacing();
    if (isAddCost) {
      workerRegionQuery.addCost(blockBox, cutLNum, fig, con);
      if (pinS) {
        workerRegionQuery.query(blockBox, cutLNum, *pinS);
      }
    } else {
      workerRegionQuery.removeCost(blockBox, cutLNum, fig, con);
      if (pinS) {
        workerRegionQuery.query(blockBox, cutLNum, *pinS);
      }
    }
  }
}

void FlexTAWorker::modCutSpacingCost(const Rect& box,
                                     frLayerNum lNum,
                                     taPinFig* fig,
                                     bool isAddCost,
                                     set<taPin*, frBlockObjectComp>* pinS)
{
  if (!getDesign()->getTech()->getLayer(lNum)->hasCutSpacing()) {
    return;
  }
  // obj1 = curr obj
  // obj2 = other obj
  // default via dimension
  frViaDef* viaDef = getDesign()->getTech()->getLayer(lNum)->getDefaultViaDef();
  frVia via(viaDef);
  Rect viaBox = via.getCutBBox();

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
    cout << "Warning: via layer connected to non-routing layer, skipped in "
            "modMinSpacingCostVia"
         << endl;
    return;
  }

  // spacing value needed
  frCoord bloatDist = 0;
  for (auto con : getDesign()->getTech()->getLayer(lNum)->getCutSpacing()) {
    bloatDist = max(bloatDist, con->getCutSpacing());
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
  Rect tmpBx;
  dbTransform xform;
  frCoord dx, dy, c2ctrackdist;
  frCoord reqDist = 0;
  frCoord maxX, blockLeft, blockRight;
  Rect blockBox;
  Point boxCenter, tmpBxCenter;
  boxCenter = {(box.xMin() + box.xMax()) / 2, (box.yMin() + box.yMax()) / 2};
  bool hasViol = false;
  for (int i = idx1; i <= idx2; i++) {
    auto trackLoc = trackLocs[i];
    if (isH) {
      xform.setOffset(Point(box.xMin(), trackLoc));
    } else {
      xform.setOffset(Point(trackLoc, box.yMin()));
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
          maxX = max(0, maxX - 1);
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
          maxX = max(0, maxX - 1);
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
            blockBox.init(max(box.xMin() - (viaBox.xMax() - 0) + 1, blockLeft),
                          trackLoc,
                          min(box.xMax() + (0 - viaBox.xMin()) - 1, blockRight),
                          trackLoc);
          }
        } else {
          if (dx > 0) {
            blockBox.init(
                trackLoc,
                max(box.yMin() - (viaBox.yMax() - 0) + 1, blockLeft),
                trackLoc,
                min(box.yMax() + (0 - viaBox.yMin()) - 1, blockRight));
          }
        }
        if (blockBox.xMin() <= blockBox.xMax()
            && blockBox.yMin() <= blockBox.yMax()) {
          hasViol = true;
        }
      } else if (con->isArea()) {
        auto currArea = max(box.area(), tmpBx.area());
        if (currArea >= con->getCutArea()) {
          hasViol = true;
        }
      } else {
        hasViol = true;
      }
      if (hasViol) {
        if (isAddCost) {
          workerRegionQuery.addCost(blockBox, lNum, fig, con);
          if (pinS) {
            workerRegionQuery.query(blockBox, lNum, *pinS);
          }
        } else {
          workerRegionQuery.removeCost(blockBox, lNum, fig, con);
          if (pinS) {
            workerRegionQuery.query(blockBox, lNum, *pinS);
          }
        }
      }
    }
  }
}

void FlexTAWorker::addCost(taPinFig* fig, set<taPin*, frBlockObjectComp>* pinS)
{
  modCost(fig, true, pinS);
}

void FlexTAWorker::subCost(taPinFig* fig, set<taPin*, frBlockObjectComp>* pinS)
{
  modCost(fig, false, pinS);
}

void FlexTAWorker::modCost(taPinFig* fig,
                           bool isAddCost,
                           set<taPin*, frBlockObjectComp>* pinS)
{
  if (fig->typeId() == tacPathSeg) {
    auto obj = static_cast<taPathSeg*>(fig);
    auto layerNum = obj->getLayerNum();
    Rect box = obj->getBBox();
    modMinSpacingCostPlanar(
        box, layerNum, obj, isAddCost, pinS);  // must be current TA layer
    modMinSpacingCostVia(box, layerNum, obj, isAddCost, true, true, pinS);
    modMinSpacingCostVia(box, layerNum, obj, isAddCost, false, true, pinS);
  } else if (fig->typeId() == tacVia) {
    auto obj = static_cast<taVia*>(fig);
    // assumes enclosure for via is always rectangle
    Rect box = obj->getLayer1BBox();
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

    dbTransform xform;
    Point pt = obj->getOrigin();
    xform.setOffset(pt);
    for (auto& uFig : obj->getViaDef()->getCutFigs()) {
      auto rect = static_cast<frRect*>(uFig.get());
      box = rect->getBBox();
      xform.apply(box);
      layerNum = obj->getViaDef()->getCutLayerNum();
      modCutSpacingCost(box, layerNum, obj, isAddCost, pinS);
    }
  } else {
    cout << "Error: unsupported region query add" << endl;
  }
}

void FlexTAWorker::assignIroute_availTracks(taPin* iroute,
                                            frLayerNum& lNum,
                                            int& idx1,
                                            int& idx2)
{
  lNum = iroute->getGuide()->getBeginLayerNum();
  auto [gbp, gep] = iroute->getGuide()->getPoints();
  Point gIdx = getDesign()->getTopBlock()->getGCellIdx(gbp);
  Rect gBox = getDesign()->getTopBlock()->getGCellBox(gIdx);
  bool isH = (getDir() == dbTechLayerDir::HORIZONTAL);
  frCoord coordLow = isH ? gBox.yMin() : gBox.xMin();
  frCoord coordHigh = isH ? gBox.yMax() : gBox.xMax();
  coordHigh--;  // to avoid higher track == guide top/right
  if (getTech()->getLayer(lNum)->isUnidirectional()) {
    const Rect& dieBx = design_->getTopBlock()->getDieBox();
    frViaDef* via = nullptr;
    Rect testBox;
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
    if (diffLow > 0)
      coordLow += diffLow;
    if (diffHigh > 0)
      coordHigh -= diffHigh;
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

frUInt4 FlexTAWorker::assignIroute_getWlenCost(taPin* iroute, frCoord trackLoc)
{
  auto guide = iroute->getGuide();
  bool isH = (getDir() == dbTechLayerDir::HORIZONTAL);
  auto [begin, end] = guide->getPoints();
  Point idx = getDesign()->getTopBlock()->getGCellIdx(end);
  Rect endBox = getDesign()->getTopBlock()->getGCellBox(idx);
  int wlen = 0;
  auto wlen_helper = iroute->getWlenHelper();
  if (wlen_helper <= 0) {
    if (isH) {
      wlen = abs(wlen_helper) * (trackLoc - endBox.yMin());
    } else {
      wlen = abs(wlen_helper) * (trackLoc - endBox.xMin());
    }
  } else {
    if (isH) {
      wlen = abs(wlen_helper) * (endBox.yMax() - trackLoc);
    } else {
      wlen = abs(wlen_helper) * (endBox.xMax() - trackLoc);
    }
  }
  if (wlen < 0) {
    double dbu = getDesign()->getTopBlock()->getDBUPerUU();
    cout << "Error: getWlenCost has wlenCost < 0"
         << ", trackLoc@" << trackLoc / dbu << " box (" << endBox.xMin() / dbu
         << ", " << endBox.yMin() / dbu << ") (" << endBox.xMax() / dbu << ", "
         << endBox.yMax() / dbu << ")" << endl;
    return (frUInt4) 0;
  } else {
    return (frUInt4) wlen;
  }
}

frUInt4 FlexTAWorker::assignIroute_getPinCost(taPin* iroute, frCoord trackLoc)
{
  frUInt4 sol = 0;
  if (iroute->hasPinCoord()) {
    sol = abs(trackLoc - iroute->getPinCoord());
    if (DBPROCESSNODE == "GF14_13M_3Mx_2Cx_4Kx_2Hx_2Gx_LB") {
      bool isH = (getDir() == dbTechLayerDir::HORIZONTAL);
      auto layerNum = iroute->getGuide()->getBeginLayerNum();
      int zIdx = layerNum / 2 - 1;
      if (sol) {
        if (isH) {
          // if cannot use bottom or upper layer to bridge, then add cost
          if ((getTech()->isVia2ViaForbiddenLen(
                   zIdx, false, false, false, sol, nullptr)
               || layerNum - 2 < BOTTOM_ROUTING_LAYER)
              && (getTech()->isVia2ViaForbiddenLen(
                      zIdx, true, true, false, sol, nullptr)
                  || layerNum + 2 > getTech()->getTopLayerNum())) {
            sol += TADRCCOST;
          }
        } else {
          if ((getTech()->isVia2ViaForbiddenLen(
                   zIdx, false, false, true, sol, nullptr)
               || layerNum - 2 < BOTTOM_ROUTING_LAYER)
              && (getTech()->isVia2ViaForbiddenLen(
                      zIdx, true, true, true, sol, nullptr)
                  || layerNum + 2 > getTech()->getTopLayerNum())) {
            sol += TADRCCOST;
          }
        }
      }
    }
  }
  return sol;
}

frUInt4 FlexTAWorker::assignIroute_getDRCCost_helper(taPin* iroute,
                                                     Rect& box,
                                                     frLayerNum lNum)
{
  auto& workerRegionQuery = getWorkerRegionQuery();
  vector<rq_box_value_t<std::pair<frBlockObject*, frConstraint*>>> result;
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
  bool isCut = false;
  for (auto& [bounds, pr] : result) {
    auto& [obj, con] = pr;
    frCoord tmpOvlp
        = -max(box.xMin(), bounds.xMin()) + min(box.xMax(), bounds.xMax())
          - max(box.yMin(), bounds.yMin()) + min(box.yMax(), bounds.yMax()) + 1;
    if (tmpOvlp <= 0) {
      cout << "Error: assignIroute_getDRCCost_helper overlap < 0" << endl;
      exit(1);
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
      cout << "Warning: assignIroute_getDRCCost_helper unsupported type"
           << endl;
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
    cout << "Error: assignIroute_getDRCCost_helper unknown layer type" << endl;
    exit(1);
  }
  // always penalize two pitch per cut, regardless of cnts
  return (overlap == 0) ? 0 : (isCut ? pitch * 2 : max(pitch * 2, overlap));
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
      Rect bbox(bp, ep);
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
      Rect bbox(bp, bp);
      frUInt4 viaCost = assignIroute_getDRCCost_helper(
          iroute, bbox, obj->getViaDef()->getCutLayerNum());
      cost += viaCost;
    } else {
      cout << "Error: assignIroute_updateIroute unsupported pinFig" << endl;
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
      set<taPin*, frBlockObjectComp> result;
      Rect box;
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
  int drcCost = (isInitTA()) ? (0.05 * outDrcCost) : (TADRCCOST * outDrcCost);
  int wlenCost = assignIroute_getWlenCost(iroute, trackLoc);
  // int pinCost    = TAPINCOST * assignIroute_getPinCost(iroute, trackLoc);
  int tmpPinCost = assignIroute_getPinCost(iroute, trackLoc);
  int pinCost
      = (tmpPinCost == 0) ? 0 : TAPINCOST * irouteLayerPitch + tmpPinCost;
  int tmpAlignCost = assignIroute_getAlignCost(iroute, trackLoc);
  int alignCost
      = (tmpAlignCost == 0) ? 0 : TAALIGNCOST * irouteLayerPitch + tmpAlignCost;
  return max(drcCost + wlenCost + pinCost - alignCost, 0);
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
  // while (1) {
  //  if pinCoord, then try from  pinCoord
  //  else try from wlen1 dir
  if (iroute->hasPinCoord()) {
    // cout <<"if" <<endl;
    frCoord pinCoord = iroute->getPinCoord();
    if (iroute->getWlenHelper() > 0) {
      int startTrackIdx
          = int(std::lower_bound(
                    trackLocs_[lNum].begin(), trackLocs_[lNum].end(), pinCoord)
                - trackLocs_[lNum].begin());
      startTrackIdx = min(startTrackIdx, idx2);
      startTrackIdx = max(startTrackIdx, idx1);
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
    } else if (iroute->getWlenHelper() == 0) {
      int startTrackIdx
          = int(std::lower_bound(
                    trackLocs_[lNum].begin(), trackLocs_[lNum].end(), pinCoord)
                - trackLocs_[lNum].begin());
      startTrackIdx = min(startTrackIdx, idx2);
      startTrackIdx = max(startTrackIdx, idx1);
      // cout <<"startTrackIdx " <<startTrackIdx <<endl;
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
      startTrackIdx = min(startTrackIdx, idx2);
      startTrackIdx = max(startTrackIdx, idx1);
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
    // cout <<"else" <<endl;
    if (iroute->getWlenHelper() > 0) {
      for (int i = idx2; i >= idx1; i--) {
        assignIroute_bestTrack_helper(
            iroute, lNum, i, bestCost, bestTrackLoc, bestTrackIdx, drcCost);
        if (!drcCost) {
          break;
        }
      }
    } else if (iroute->getWlenHelper() == 0) {
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
    Rect box = guide->getBBox();
    cout << "Error: assignIroute_bestTrack select no track for "
         << guide->getNet()->getName() << " @(" << box.xMin() / dbu << ", "
         << box.yMin() / dbu << ") (" << box.xMax() / dbu << ", "
         << box.yMax() / dbu << " "
         << getDesign()->getTech()->getLayer(lNum)->getName()
         << " idx1/2=" << idx1 << "/" << idx2 << endl;
    exit(1);
  }
  totCost_ += drcCost;
  iroute->setCost(drcCost);
  return bestTrackLoc;
}

void FlexTAWorker::assignIroute_updateIroute(
    taPin* iroute,
    frCoord bestTrackLoc,
    set<taPin*, frBlockObjectComp>* pinS)
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
      cout << "Error: assignIroute_updateIroute unsupported pinFig" << endl;
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
                                     set<taPin*, frBlockObjectComp>* pinS)
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

void FlexTAWorker::assignIroute_updateOthers(
    set<taPin*, frBlockObjectComp>& pinS)
{
  bool isH = (getDir() == dbTechLayerDir::HORIZONTAL);
  if (isInitTA()) {
    return;
  }
  for (auto& iroute : pinS) {
    if (iroute->getGuide()->getNet()->isClock() && !hardIroutesMode)
      continue;
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
      cout << "Error: FlexTAWorker::assignIroute_updateOthers does not find "
              "trackLoc"
           << endl;
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
  set<taPin*, frBlockObjectComp> pinS;
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
  vector<taPin*> buffers(maxBufferSize, nullptr);
  int currBufferIdx = 0;
  auto iroute = popFromReassignIroutes();
  while (iroute != nullptr) {
    auto it = find(buffers.begin(), buffers.end(), iroute);
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
