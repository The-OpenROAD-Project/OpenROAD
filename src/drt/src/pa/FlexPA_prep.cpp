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

#include <omp.h>

#include <chrono>
#include <iostream>
#include <sstream>

#include "FlexPA.h"
#include "FlexPA_graphics.h"
#include "db/infra/frTime.h"
#include "frProfileTask.h"
#include "gc/FlexGC.h"
#include "utl/exception.h"

using namespace std;
using namespace fr;

using utl::ThreadException;

int gcCallCnt = 0;

template <typename T>
void FlexPA::prepPoint_pin_mergePinShapes(
    vector<gtl::polygon_90_set_data<frCoord>>& pinShapes,
    T* pin,
    frInstTerm* instTerm,
    bool isShrink)
{
  frInst* inst = nullptr;
  if (instTerm) {
    inst = instTerm->getInst();
  }

  dbTransform xform;
  if (inst) {
    xform = inst->getUpdatedXform();
  }

  vector<frCoord> layerWidths;
  if (isShrink) {
    layerWidths.resize(getDesign()->getTech()->getLayers().size(), 0);
    for (int i = 0; i < int(layerWidths.size()); i++) {
      layerWidths[i] = getDesign()->getTech()->getLayer(i)->getWidth();
    }
  }

  pinShapes.clear();
  pinShapes.resize(getDesign()->getTech()->getLayers().size());
  for (auto& shape : pin->getFigs()) {
    if (shape->typeId() == frcRect) {
      auto obj = static_cast<frRect*>(shape.get());
      auto layerNum = obj->getLayerNum();
      if (getDesign()->getTech()->getLayer(layerNum)->getType()
          != dbTechLayerType::ROUTING) {
        continue;
      }
      Rect box = obj->getBBox();
      xform.apply(box);
      gtl::rectangle_data<frCoord> rect(
          box.xMin(), box.yMin(), box.xMax(), box.yMax());
      if (isShrink) {
        if (getDesign()->getTech()->getLayer(layerNum)->getDir()
            == dbTechLayerDir::HORIZONTAL) {
          gtl::shrink(rect, gtl::VERTICAL, layerWidths[layerNum] / 2);
        } else if (getDesign()->getTech()->getLayer(layerNum)->getDir()
                   == dbTechLayerDir::VERTICAL) {
          gtl::shrink(rect, gtl::HORIZONTAL, layerWidths[layerNum] / 2);
        }
      }
      using namespace boost::polygon::operators;
      pinShapes[layerNum] += rect;
    } else if (shape->typeId() == frcPolygon) {
      auto obj = static_cast<frPolygon*>(shape.get());
      auto layerNum = obj->getLayerNum();
      vector<gtl::point_data<frCoord>> points;
      // must be copied pts
      for (Point pt : obj->getPoints()) {
        xform.apply(pt);
        points.push_back(gtl::point_data<frCoord>(pt.x(), pt.y()));
      }
      gtl::polygon_90_data<frCoord> poly;
      poly.set(points.begin(), points.end());
      using namespace boost::polygon::operators;
      pinShapes[layerNum] += poly;
    } else {
      logger_->error(DRT, 67, "FlexPA mergePinShapes unsupported shape.");
      exit(1);
    }
  }
}

void FlexPA::prepPoint_pin_genPoints_rect_genGrid(
    map<frCoord, frAccessPointEnum>& coords,
    const map<frCoord, frAccessPointEnum>& trackCoords,
    frCoord low,
    frCoord high,
    bool useNearbyGrid)
{
  for (auto it = trackCoords.lower_bound(low); it != trackCoords.end(); it++) {
    auto& [coord, cost] = *it;
    if (coord > high) {
      break;
    }
    if (useNearbyGrid) {
      coords.insert({coord, frAccessPointEnum::NearbyGrid});
    } else {
      coords.insert(*it);
    }
  }
}

// will not generate center for wider edge
void FlexPA::prepPoint_pin_genPoints_rect_genCenter(
    map<frCoord, frAccessPointEnum>& coords,
    frLayerNum layerNum,
    frCoord low,
    frCoord high)
{
  // if touching two tracks, then no center??
  int cnt = 0;
  for (auto it = coords.lower_bound(low); it != coords.end(); it++) {
    auto& [c1, c2] = *it;
    if (c1 > high) {
      break;
    }
    if (c2 == frAccessPointEnum::OnGrid) {
      cnt++;
    }
  }
  if (cnt >= 2) {
    return;
  }

  frCoord manuGrid = getDesign()->getTech()->getManufacturingGrid();
  frCoord coord = (low + high) / 2 / manuGrid * manuGrid;
  auto it = coords.find(coord);
  if (it == coords.end()) {
    coords.insert(make_pair(coord, frAccessPointEnum::Center));
  } else {
    coords[coord] = std::min(coords[coord], frAccessPointEnum::Center);
  }
}

void FlexPA::prepPoint_pin_genPoints_rect_ap_helper(
    vector<unique_ptr<frAccessPoint>>& aps,
    set<pair<Point, frLayerNum>>& apset,
    const gtl::rectangle_data<frCoord>& maxrect,
    frCoord x,
    frCoord y,
    frLayerNum layerNum,
    bool allowPlanar,
    bool allowVia,
    frAccessPointEnum lowCost,
    frAccessPointEnum highCost)
{
  gtl::point_data<frCoord> pt(x, y);
  if (!gtl::contains(maxrect, pt) && lowCost != frAccessPointEnum::NearbyGrid
      && highCost != frAccessPointEnum::NearbyGrid) {
    return;
  }
  Point fpt(x, y);
  if (apset.find(make_pair(fpt, layerNum)) != apset.end()) {
    return;
  }
  auto ap = make_unique<frAccessPoint>(fpt, layerNum);
  if (allowPlanar) {
    auto lowerLayer = getDesign()->getTech()->getLayer(layerNum);
    ap->setAccess(frDirEnum::W, true);
    ap->setAccess(frDirEnum::E, true);
    ap->setAccess(frDirEnum::S, true);
    ap->setAccess(frDirEnum::N, true);
    // rectonly forbid wrongway planar access
    // rightway on grid only forbid off track rightway planar access
    // horz layer
    if (lowerLayer->getDir() == dbTechLayerDir::HORIZONTAL) {
      if (lowerLayer->isUnidirectional()) {
        ap->setAccess(frDirEnum::S, false);
        ap->setAccess(frDirEnum::N, false);
      }
      if (lowerLayer->getLef58RightWayOnGridOnlyConstraint()
          && lowCost != frAccessPointEnum::OnGrid) {
        ap->setAccess(frDirEnum::W, false);
        ap->setAccess(frDirEnum::E, false);
      }
    }
    // vert layer
    if (lowerLayer->getDir() == dbTechLayerDir::VERTICAL) {
      if (lowerLayer->isUnidirectional()) {
        ap->setAccess(frDirEnum::W, false);
        ap->setAccess(frDirEnum::E, false);
      }
      if (lowerLayer->getLef58RightWayOnGridOnlyConstraint()
          && lowCost != frAccessPointEnum::OnGrid) {
        ap->setAccess(frDirEnum::S, false);
        ap->setAccess(frDirEnum::N, false);
      }
    }
  } else {
    ap->setAccess(frDirEnum::W, false);
    ap->setAccess(frDirEnum::E, false);
    ap->setAccess(frDirEnum::S, false);
    ap->setAccess(frDirEnum::N, false);
  }
  ap->setAccess(frDirEnum::D, false);
  if (allowVia) {
    ap->setAccess(frDirEnum::U, true);
    // cout <<"@@@set U valid, check " <<ap->hasAccess(frDirEnum::U) <<endl;
  } else {
    ap->setAccess(frDirEnum::U, false);
  }
  ap->setType((frAccessPointEnum) lowCost, true);
  ap->setType((frAccessPointEnum) highCost, false);
  if ((lowCost == frAccessPointEnum::NearbyGrid
       || highCost == frAccessPointEnum::NearbyGrid)) {
    Point end;
    int halfWidth
        = design_->getTech()->getLayer(ap->getLayerNum())->getMinWidth() / 2;
    if (fpt.x() < gtl::xl(maxrect) + halfWidth) {
      end.setX(gtl::xl(maxrect) + halfWidth);
    } else if (fpt.x() > gtl::xh(maxrect) - halfWidth)
      end.setX(gtl::xh(maxrect) - halfWidth);
    else
      end.setX(fpt.x());
    if (fpt.y() < gtl::yl(maxrect) + halfWidth)
      end.setY(gtl::yl(maxrect) + halfWidth);
    else if (fpt.y() > gtl::yh(maxrect) - halfWidth)
      end.setY(gtl::yh(maxrect) - halfWidth);
    else
      end.setY(fpt.y());

    Point e = fpt;
    if (fpt.x() != end.x())
      e.setX(end.x());
    else if (fpt.y() != end.y())
      e.setY(end.y());
    if (!(e == fpt)) {
      frPathSeg ps;
      ps.setPoints_safe(fpt, e);
      if (ps.getBeginPoint() == end)
        ps.setBeginStyle(frEndStyle(frcTruncateEndStyle));
      else if (ps.getEndPoint() == end)
        ps.setEndStyle(frEndStyle(frcTruncateEndStyle));
      ap->addPathSeg(std::move(ps));
      if (!(e == end)) {
        fpt = e;
        ps.setPoints_safe(fpt, end);
        if (ps.getBeginPoint() == end)
          ps.setBeginStyle(frEndStyle(frcTruncateEndStyle));
        else
          ps.setEndStyle(frEndStyle(frcTruncateEndStyle));
        ap->addPathSeg(std::move(ps));
      }
    }
  }
  aps.push_back(std::move(ap));
  apset.insert(make_pair(fpt, layerNum));
}

void FlexPA::prepPoint_pin_genPoints_rect_ap(
    vector<unique_ptr<frAccessPoint>>& aps,
    set<pair<Point, frLayerNum>>& apset,
    const gtl::rectangle_data<frCoord>& rect,
    frLayerNum layerNum,
    bool allowPlanar,
    bool allowVia,
    bool isLayer1Horz,
    const map<frCoord, frAccessPointEnum>& xCoords,
    const map<frCoord, frAccessPointEnum>& yCoords,
    frAccessPointEnum lowerType,
    frAccessPointEnum upperType)
{
  // build points;
  for (auto& [xCoord, costX] : xCoords) {
    for (auto& [yCoord, costY] : yCoords) {
      // lower full/half/center
      auto& lowCost = isLayer1Horz ? costY : costX;
      auto& highCost = (!isLayer1Horz) ? costY : costX;
      if (lowCost == lowerType && highCost == upperType) {
        prepPoint_pin_genPoints_rect_ap_helper(aps,
                                               apset,
                                               rect,
                                               xCoord,
                                               yCoord,
                                               layerNum,
                                               allowPlanar,
                                               allowVia,
                                               lowCost,
                                               highCost);
      }
    }
  }
}

void FlexPA::prepPoint_pin_genPoints_rect_genEnc(
    map<frCoord, frAccessPointEnum>& coords,
    const gtl::rectangle_data<frCoord>& rect,
    frLayerNum layerNum,
    bool isCurrLayerHorz)
{
  auto rectWidth = gtl::delta(rect, gtl::HORIZONTAL);
  auto rectHeight = gtl::delta(rect, gtl::VERTICAL);
  int maxNumViaTrial = 2;
  if (layerNum + 1 > getDesign()->getTech()->getTopLayerNum()) {
    return;
  }
  // hardcode first two single vias
  vector<frViaDef*> viaDefs;
  int cnt = 0;
  for (auto& [tup, via] : layerNum2ViaDefs_[layerNum + 1][1]) {
    viaDefs.push_back(via);
    cnt++;
    if (cnt >= maxNumViaTrial) {
      break;
    }
  }
  for (auto& viaDef : viaDefs) {
    frVia via(viaDef);
    Rect box = via.getLayer1BBox();
    auto viaWidth = box.xMax() - box.xMin();
    auto viaHeight = box.yMax() - box.yMin();
    if (viaWidth > rectWidth || viaHeight > rectHeight) {
      // cout <<"@@@" <<viaDef->getName() <<" rect " <<rectWidth <<" "
      // <<rectHeight <<" via " <<viaWidth <<" " <<viaHeight <<endl;
      continue;
    }
    if (isCurrLayerHorz) {
      auto coord = gtl::yh(rect) - (box.yMax() - 0);
      auto it = coords.find(coord);
      if (it == coords.end()) {
        // cout << coord / 2000.0 <<endl;
        coords.insert(make_pair(coord, frAccessPointEnum::EncOpt));
      } else {
        coords[coord] = std::min(coords[coord], frAccessPointEnum::EncOpt);
      }
      coord = gtl::yl(rect) + (0 - box.yMin());
      it = coords.find(coord);
      if (it == coords.end()) {
        // cout << coord / 2000.0 <<endl;
        coords.insert(make_pair(coord, frAccessPointEnum::EncOpt));
      } else {
        coords[coord] = std::min(coords[coord], frAccessPointEnum::EncOpt);
      }
    } else {
      auto coord = gtl::xh(rect) - (box.xMax() - 0);
      auto it = coords.find(coord);
      if (it == coords.end()) {
        // cout << coord / 2000.0 <<endl;
        coords.insert(make_pair(coord, frAccessPointEnum::EncOpt));
      } else {
        coords[coord] = std::min(coords[coord], frAccessPointEnum::EncOpt);
      }
      coord = gtl::xl(rect) + (0 - box.xMin());
      it = coords.find(coord);
      if (it == coords.end()) {
        // cout << coord / 2000.0 <<endl;
        coords.insert(make_pair(coord, frAccessPointEnum::EncOpt));
      } else {
        coords[coord] = std::min(coords[coord], frAccessPointEnum::EncOpt);
      }
    }
  }
}

bool FlexPA::enclosesOnTrackPlanarAccess(
    const gtl::rectangle_data<frCoord>& rect,
    frLayerNum layerNum)
{
  frCoord low, high;
  frLayer* layer = getDesign()->getTech()->getLayer(layerNum);
  if (layer->isHorizontal()) {
    low = gtl::yl(rect);
    high = gtl::yh(rect);
  } else if (layer->isVertical()) {
    low = gtl::xl(rect);
    high = gtl::xh(rect);
  } else
    logger_->error(
        DRT,
        1003,
        "enclosesPlanarAccess: layer is neither vertical or horizontal");
  auto& tracks = trackCoords_[layerNum];
  auto lowTrack = tracks.lower_bound(low);
  if (lowTrack == tracks.end())
    logger_->error(DRT, 1004, "enclosesPlanarAccess: low track not found");
  if (lowTrack->first > high)
    return false;
  auto highTrack = tracks.lower_bound(high);
  if (highTrack != tracks.end()) {
    if (highTrack->first > high)
      highTrack--;
  } else
    logger_->error(DRT, 1005, "enclosesPlanarAccess: high track not found");
  if (highTrack->first - lowTrack->first > (int) layer->getPitch())
    return true;
  if (lowTrack->first - (int) layer->getWidth() / 2 < low)
    return false;
  if (highTrack->first + (int) layer->getWidth() / 2 > high)
    return false;
  return true;
}
void FlexPA::prepPoint_pin_genPoints_rect(
    vector<unique_ptr<frAccessPoint>>& aps,
    set<pair<Point, frLayerNum>>& apset,
    const gtl::rectangle_data<frCoord>& rect,
    frLayerNum layerNum,
    bool allowPlanar,
    bool allowVia,
    frAccessPointEnum lowerType,
    frAccessPointEnum upperType,
    bool isMacroCellPin)
{
  auto minWidthLayer1
      = getDesign()->getTech()->getLayer(layerNum)->getMinWidth();
  if (std::min(gtl::delta(rect, gtl::HORIZONTAL),
               gtl::delta(rect, gtl::VERTICAL))
      < minWidthLayer1) {
    return;
  }
  frLayerNum secondLayerNum = 0;
  if (layerNum + 2 <= getDesign()->getTech()->getTopLayerNum()) {
    secondLayerNum = layerNum + 2;
  } else if (layerNum - 2 >= getDesign()->getTech()->getBottomLayerNum()) {
    secondLayerNum = layerNum - 2;
  } else {
    logger_->error(
        DRT, 68, "prepPoint_pin_genPoints_rect cannot find secondLayerNum.");
  }
  auto minWidthLayer2
      = getDesign()->getTech()->getLayer(secondLayerNum)->getMinWidth();
  auto& layer1TrackCoords = trackCoords_[layerNum];
  auto& layer2TrackCoords = trackCoords_[secondLayerNum];
  bool isLayer1Horz = (getDesign()->getTech()->getLayer(layerNum)->getDir()
                       == dbTechLayerDir::HORIZONTAL);

  map<frCoord, frAccessPointEnum> xCoords;
  map<frCoord, frAccessPointEnum> yCoords;
  int hwidth = getDesign()->getTech()->getLayer(layerNum)->getWidth() / 2;
  bool useCenterLine = false;
  if (isMacroCellPin) {
    auto rectDir = gtl::guess_orientation(rect);
    if ((rectDir == gtl::HORIZONTAL && isLayer1Horz)
        || (rectDir == gtl::VERTICAL && !isLayer1Horz)) {
      auto layerWidth = getDesign()->getTech()->getLayer(layerNum)->getWidth();
      if ((rectDir == gtl::HORIZONTAL
           && gtl::delta(rect, gtl::VERTICAL) < 2 * layerWidth)
          || (rectDir == gtl::VERTICAL
              && gtl::delta(rect, gtl::HORIZONTAL) < 2 * layerWidth)) {
        useCenterLine = true;
      }
    }
  }

  // gen all full/half grid coords
  if (!isMacroCellPin || !useCenterLine) {
    if (isLayer1Horz) {
      prepPoint_pin_genPoints_rect_genGrid(
          yCoords, layer1TrackCoords, gtl::yl(rect), gtl::yh(rect));
      prepPoint_pin_genPoints_rect_genGrid(
          xCoords,
          layer2TrackCoords,
          gtl::xl(rect) + (isMacroCellPin ? hwidth : 0),
          gtl::xh(rect) - (isMacroCellPin ? hwidth : 0));
      if (lowerType >= frAccessPointEnum::Center) {
        prepPoint_pin_genPoints_rect_genCenter(
            yCoords, layerNum, gtl::yl(rect), gtl::yh(rect));
      }
      if (lowerType >= frAccessPointEnum::EncOpt) {
        prepPoint_pin_genPoints_rect_genEnc(
            yCoords, rect, layerNum, isLayer1Horz);
      }
      if (upperType >= frAccessPointEnum::Center) {
        prepPoint_pin_genPoints_rect_genCenter(
            xCoords,
            layerNum,
            gtl::xl(rect) + (isMacroCellPin ? hwidth : 0),
            gtl::xh(rect) - (isMacroCellPin ? hwidth : 0));
      }
      if (upperType >= frAccessPointEnum::EncOpt) {
        prepPoint_pin_genPoints_rect_genEnc(
            xCoords, rect, layerNum, !isLayer1Horz);
      }
      if (lowerType >= frAccessPointEnum::NearbyGrid) {
        prepPoint_pin_genPoints_rect_genGrid(yCoords,
                                             layer1TrackCoords,
                                             gtl::yh(rect),
                                             gtl::yh(rect) + minWidthLayer1,
                                             true);
        prepPoint_pin_genPoints_rect_genGrid(yCoords,
                                             layer1TrackCoords,
                                             gtl::yl(rect) - minWidthLayer1,
                                             gtl::yl(rect),
                                             true);
      }
      if (upperType >= frAccessPointEnum::NearbyGrid) {
        prepPoint_pin_genPoints_rect_genGrid(xCoords,
                                             layer2TrackCoords,
                                             gtl::xh(rect),
                                             gtl::xh(rect) + minWidthLayer2,
                                             true);
        prepPoint_pin_genPoints_rect_genGrid(xCoords,
                                             layer2TrackCoords,
                                             gtl::xl(rect) - minWidthLayer2,
                                             gtl::xl(rect),
                                             true);
      }
    } else {
      prepPoint_pin_genPoints_rect_genGrid(
          xCoords, layer1TrackCoords, gtl::xl(rect), gtl::xh(rect));
      prepPoint_pin_genPoints_rect_genGrid(
          yCoords,
          layer2TrackCoords,
          gtl::yl(rect) + (isMacroCellPin ? hwidth : 0),
          gtl::yh(rect) - (isMacroCellPin ? hwidth : 0));
      if (lowerType >= frAccessPointEnum::Center) {
        prepPoint_pin_genPoints_rect_genCenter(
            xCoords, layerNum, gtl::xl(rect), gtl::xh(rect));
      }
      if (lowerType >= frAccessPointEnum::EncOpt) {
        prepPoint_pin_genPoints_rect_genEnc(
            xCoords, rect, layerNum, isLayer1Horz);
      }
      if (upperType >= frAccessPointEnum::Center) {
        prepPoint_pin_genPoints_rect_genCenter(
            yCoords,
            layerNum,
            gtl::yl(rect) + (isMacroCellPin ? hwidth : 0),
            gtl::yh(rect) - (isMacroCellPin ? hwidth : 0));
      }
      if (upperType >= frAccessPointEnum::EncOpt) {
        prepPoint_pin_genPoints_rect_genEnc(
            yCoords, rect, layerNum, !isLayer1Horz);
      }
      if (lowerType >= frAccessPointEnum::NearbyGrid) {
        prepPoint_pin_genPoints_rect_genGrid(xCoords,
                                             layer1TrackCoords,
                                             gtl::xh(rect),
                                             gtl::xh(rect) + minWidthLayer1,
                                             true);
        prepPoint_pin_genPoints_rect_genGrid(xCoords,
                                             layer1TrackCoords,
                                             gtl::xl(rect) - minWidthLayer1,
                                             gtl::xl(rect),
                                             true);
      }
      if (upperType >= frAccessPointEnum::NearbyGrid) {
        prepPoint_pin_genPoints_rect_genGrid(yCoords,
                                             layer2TrackCoords,
                                             gtl::yh(rect),
                                             gtl::yh(rect) + minWidthLayer2,
                                             true);
        prepPoint_pin_genPoints_rect_genGrid(yCoords,
                                             layer2TrackCoords,
                                             gtl::yl(rect) - minWidthLayer2,
                                             gtl::yl(rect),
                                             true);
      }
    }
  } else {
    if (isLayer1Horz) {
      lowerType = frAccessPointEnum::OnGrid;
      prepPoint_pin_genPoints_rect_genGrid(
          xCoords, layer2TrackCoords, gtl::xl(rect), gtl::xh(rect));
      if (upperType >= frAccessPointEnum::Center) {
        prepPoint_pin_genPoints_rect_genCenter(
            xCoords, layerNum, gtl::xl(rect), gtl::xh(rect));
      }
      if (upperType >= frAccessPointEnum::EncOpt) {
        prepPoint_pin_genPoints_rect_genEnc(
            xCoords, rect, layerNum, !isLayer1Horz);
      }
      if (upperType >= frAccessPointEnum::NearbyGrid) {
        prepPoint_pin_genPoints_rect_genGrid(xCoords,
                                             layer2TrackCoords,
                                             gtl::xh(rect),
                                             gtl::xh(rect) + minWidthLayer2,
                                             true);
        prepPoint_pin_genPoints_rect_genGrid(xCoords,
                                             layer2TrackCoords,
                                             gtl::xl(rect) - minWidthLayer2,
                                             gtl::xl(rect),
                                             true);
      }
      prepPoint_pin_genPoints_rect_genCenter(
          yCoords, layerNum, gtl::yl(rect), gtl::yh(rect));
      for (auto& [yCoord, cost] : yCoords) {
        yCoords[yCoord] = frAccessPointEnum::OnGrid;
      }
    } else {
      prepPoint_pin_genPoints_rect_genGrid(
          yCoords, layer2TrackCoords, gtl::yl(rect), gtl::yh(rect));
      if (upperType >= frAccessPointEnum::Center) {
        prepPoint_pin_genPoints_rect_genCenter(
            yCoords, layerNum, gtl::yl(rect), gtl::yh(rect));
      }
      if (upperType >= frAccessPointEnum::EncOpt) {
        prepPoint_pin_genPoints_rect_genEnc(
            yCoords, rect, layerNum, !isLayer1Horz);
      }
      if (upperType >= frAccessPointEnum::NearbyGrid) {
        prepPoint_pin_genPoints_rect_genGrid(yCoords,
                                             layer2TrackCoords,
                                             gtl::yh(rect),
                                             gtl::yh(rect) + minWidthLayer2,
                                             true);
        prepPoint_pin_genPoints_rect_genGrid(yCoords,
                                             layer2TrackCoords,
                                             gtl::yl(rect) - minWidthLayer2,
                                             gtl::yl(rect),
                                             true);
      }
      prepPoint_pin_genPoints_rect_genCenter(
          xCoords, layerNum, gtl::xl(rect), gtl::xh(rect));
      for (auto& [xCoord, cost] : xCoords) {
        xCoords[xCoord] = frAccessPointEnum::OnGrid;
      }
    }
  }
  prepPoint_pin_genPoints_rect_ap(aps,
                                  apset,
                                  rect,
                                  layerNum,
                                  allowPlanar,
                                  allowVia,
                                  isLayer1Horz,
                                  xCoords,
                                  yCoords,
                                  lowerType,
                                  upperType);
}

void FlexPA::prepPoint_pin_genPoints_layerShapes(
    vector<unique_ptr<frAccessPoint>>& aps,
    set<pair<Point, frLayerNum>>& apset,
    frInstTerm* instTerm,
    const gtl::polygon_90_set_data<frCoord>& layerShapes,
    frLayerNum layerNum,
    bool allowVia,
    frAccessPointEnum lowerType,
    frAccessPointEnum upperType)
{
  if (getDesign()->getTech()->getLayer(layerNum)->getType()
      != dbTechLayerType::ROUTING) {
    return;
  }
  bool allowPlanar = true;
  bool isMacroCellPin = false;
  if (instTerm) {
    dbMasterType masterType = instTerm->getInst()->getMaster()->getMasterType();
    if (masterType == dbMasterType::CORE
        || masterType == dbMasterType::CORE_TIEHIGH
        || masterType == dbMasterType::CORE_TIELOW
        || masterType == dbMasterType::CORE_ANTENNACELL) {
      if ((layerNum >= VIAINPIN_BOTTOMLAYERNUM
           && layerNum <= VIAINPIN_TOPLAYERNUM)
          || layerNum <= VIA_ACCESS_LAYERNUM) {
        allowPlanar = false;
      }
    } else if (masterType.isBlock() || masterType.isPad()
               || masterType == dbMasterType::RING) {
      isMacroCellPin = true;
    }
  } else {
    // IO term is treated as the MacroCellPin as the top block
    isMacroCellPin = true;
    allowPlanar = true;
    allowVia = false;
  }
  // lower layer is current layer
  // righway on grid only forbid off track up via access on upper layer
  auto upperLayer = (layerNum + 2 <= getDesign()->getTech()->getTopLayerNum())
                        ? getDesign()->getTech()->getLayer(layerNum + 2)
                        : nullptr;
  if (!isMacroCellPin && upperLayer
      && upperLayer->getLef58RightWayOnGridOnlyConstraint()
      && upperType != frAccessPointEnum::OnGrid) {
    return;
  }
  vector<gtl::rectangle_data<frCoord>> maxrects;
  gtl::get_max_rectangles(maxrects, layerShapes);
  for (auto& bboxRect : maxrects) {
    prepPoint_pin_genPoints_rect(aps,
                                 apset,
                                 bboxRect,
                                 layerNum,
                                 allowPlanar,
                                 allowVia,
                                 lowerType,
                                 upperType,
                                 isMacroCellPin);
  }
}

// filter off-grid coordinate
// lower on-grid 0, upper on-grid 0 = 0
// lower 1/2     1, upper on-grid 0 = 1
// lower center  2, upper on-grid 0 = 2
// lower center  2, upper center  2 = 4
template <typename T>
void FlexPA::prepPoint_pin_genPoints(
    vector<unique_ptr<frAccessPoint>>& aps,
    set<pair<Point, frLayerNum>>& apset,
    T* pin,
    frInstTerm* instTerm,
    const vector<gtl::polygon_90_set_data<frCoord>>& pinShapes,
    frAccessPointEnum lowerType,
    frAccessPointEnum upperType)
{
  //  only VIA_ACCESS_LAYERNUM layer can have via access
  bool allowVia = true;
  frLayerNum layerNum = (int) pinShapes.size() - 1;
  for (auto it = pinShapes.rbegin(); it != pinShapes.rend(); it++) {
    if (!it->empty()
        && getDesign()->getTech()->getLayer(layerNum)->getType()
               == dbTechLayerType::ROUTING) {
      // cout <<"via layernum = " <<layerNum <<endl;
      prepPoint_pin_genPoints_layerShapes(
          aps, apset, instTerm, *it, layerNum, allowVia, lowerType, upperType);
    }
    layerNum--;
  }
}

bool FlexPA::prepPoint_pin_checkPoint_planar_ep(
    Point& ep,
    const vector<gtl::polygon_90_data<frCoord>>& layerPolys,
    const Point& bp,
    frLayerNum layerNum,
    frDirEnum dir,
    bool isBlock)
{
  const int stepSizeMultiplier = 3;
  frCoord x = bp.x();
  frCoord y = bp.y();
  frCoord width = getDesign()->getTech()->getLayer(layerNum)->getWidth();
  frCoord stepSize = stepSizeMultiplier * width;
  frCoord pitch = getDesign()->getTech()->getLayer(layerNum)->getPitch();
  gtl::rectangle_data<frCoord> rect;
  if (isBlock) {
    gtl::extents(rect, layerPolys[0]);
    if (layerPolys.size() > 1)
      logger_->warn(DRT, 6000, "Macro pin has more than 1 polygon");
  }
  switch (dir) {
    case (frDirEnum::W):
      if (isBlock)
        x = gtl::xl(rect) - pitch;
      else
        x -= stepSize;
      break;
    case (frDirEnum::E):
      if (isBlock)
        x = gtl::xh(rect) + pitch;
      else
        x += stepSize;
      break;
    case (frDirEnum::S):
      if (isBlock)
        y = gtl::yl(rect) - pitch;
      else
        y -= stepSize;
      break;
    case (frDirEnum::N):
      if (isBlock)
        y = gtl::yh(rect) + pitch;
      else
        y += stepSize;
      break;
    default:
      logger_->error(DRT, 70, "Unexpected direction in getPlanarEP.");
  }
  ep = {x, y};
  gtl::point_data<frCoord> pt(x, y);
  bool outside = true;
  for (auto& layerPoly : layerPolys) {
    if (gtl::contains(layerPoly, pt)) {
      outside = false;
      break;
    }
  }

  return outside;
}

template <typename T>
void FlexPA::prepPoint_pin_checkPoint_planar(
    frAccessPoint* ap,
    const vector<gtl::polygon_90_data<frCoord>>& layerPolys,
    frDirEnum dir,
    T* pin,
    frInstTerm* instTerm)
{
  Point bp = ap->getPoint();
  // skip viaonly access
  if (!ap->hasAccess(dir)) {
    return;
  }
  bool isBlock
      = instTerm && instTerm->getInst()->getMaster()->getMasterType().isBlock();
  Point ep;
  bool isOutSide = prepPoint_pin_checkPoint_planar_ep(
      ep, layerPolys, bp, ap->getLayerNum(), dir, isBlock);
  // skip if two width within shape for standard cell
  if (!isOutSide) {
    ap->setAccess(dir, false);
    return;
  }
  // TODO: EDIT HERE Wrongdirection segments
  auto layer = getDesign()->getTech()->getLayer(ap->getLayerNum());
  auto ps = make_unique<frPathSeg>();
  auto style = layer->getDefaultSegStyle();
  if (dir == frDirEnum::W || dir == frDirEnum::S) {
    ps->setPoints(ep, bp);
    style.setEndStyle(frcTruncateEndStyle, 0);
    if (layer->getDir() == dbTechLayerDir::VERTICAL)
      style.setWidth(layer->getWrongDirWidth());
  } else {
    ps->setPoints(bp, ep);
    style.setBeginStyle(frcTruncateEndStyle, 0);
    if (layer->getDir() == dbTechLayerDir::HORIZONTAL)
      style.setWidth(layer->getWrongDirWidth());
  }
  ps->setLayerNum(ap->getLayerNum());
  ps->setStyle(style);
  if (instTerm && instTerm->hasNet()) {
    ps->addToNet(instTerm->getNet());
  } else {
    ps->addToPin(pin);
  }

  // new gcWorker
  FlexGCWorker gcWorker(getTech(), logger_);
  gcWorker.setIgnoreMinArea();
  gcWorker.setIgnoreCornerSpacing();
  Rect extBox(bp.x() - 3000, bp.y() - 3000, bp.x() + 3000, bp.y() + 3000);
  gcWorker.setExtBox(extBox);
  gcWorker.setDrcBox(extBox);
  if (instTerm) {
    gcWorker.addTargetObj(instTerm->getInst());
  } else {
    gcWorker.addTargetObj(pin->getTerm());
  }
  gcWorker.initPA0(getDesign());
  frBlockObject* owner;
  if (instTerm) {
    if (instTerm->hasNet()) {
      owner = instTerm->getNet();
    } else {
      owner = instTerm;
    }
  } else {
    if (pin->getTerm()->hasNet()) {
      owner = pin->getTerm()->getNet();
    } else {
      owner = pin->getTerm();
    }
  }
  gcWorker.addPAObj(ps.get(), owner);
  for (auto& apPs : ap->getPathSegs())
    gcWorker.addPAObj(&apPs, owner);
  gcWorker.initPA1();
  gcWorker.main();
  gcWorker.end();

  if (gcWorker.getMarkers().empty()) {
    ap->setAccess(dir, true);
  } else {
    ap->setAccess(dir, false);
  }

  if (graphics_) {
    graphics_->setPlanarAP(ap, ps.get(), gcWorker.getMarkers());
  }
}

void FlexPA::getViasFromMetalWidthMap(
    const Point& pt,
    const frLayerNum layerNum,
    const gtl::polygon_90_set_data<frCoord>& polyset,
    vector<pair<int, frViaDef*>>& viaDefs)
{
  const auto tech = getTech();
  if (layerNum == tech->getTopLayerNum()) {
    return;
  }
  const auto cutLayer = tech->getLayer(layerNum + 1)->getDbLayer();
  // If the upper layer has an NDR special handling will be needed
  // here. Assuming normal min-width routing for now.
  const frCoord top_width = tech->getLayer(layerNum + 2)->getMinWidth();
  const auto width_orient
      = tech->isHorizontalLayer(layerNum) ? gtl::VERTICAL : gtl::HORIZONTAL;
  frCoord bottom_width = -1;
  auto viaMap = cutLayer->getTech()->getMetalWidthViaMap();
  for (auto entry : viaMap) {
    if (entry->getCutLayer() != cutLayer) {
      continue;
    }

    if (entry->isPgVia()) {
      continue;
    }

    if (entry->isViaCutClass()) {
      logger_->warn(
          DRT,
          519,
          "Via cut classes in LEF58_METALWIDTHVIAMAP are not supported.");
      continue;
    }

    if (entry->getAboveLayerWidthLow() > top_width
        || entry->getAboveLayerWidthHigh() < top_width) {
      continue;
    }

    if (bottom_width < 0) {  // compute bottom_width once
      vector<gtl::rectangle_data<frCoord>> maxrects;
      gtl::get_max_rectangles(maxrects, polyset);
      for (auto& rect : maxrects) {
        if (contains(rect, gtl::point_data<frCoord>(pt.x(), pt.y()))) {
          const frCoord width = delta(rect, width_orient);
          bottom_width = std::max(bottom_width, width);
        }
      }
    }

    if (entry->getBelowLayerWidthLow() > bottom_width
        || entry->getBelowLayerWidthHigh() < bottom_width) {
      continue;
    }

    viaDefs.push_back({viaDefs.size(), tech->getVia(entry->getViaName())});
  }
}

template <typename T>
void FlexPA::prepPoint_pin_checkPoint_via(
    frAccessPoint* ap,
    const gtl::polygon_90_set_data<frCoord>& polyset,
    frDirEnum dir,
    T* pin,
    frInstTerm* instTerm)
{
  Point bp = ap->getPoint();
  auto layerNum = ap->getLayerNum();
  // skip planar only access
  if (!ap->hasAccess(dir)) {
    // cout <<"no via access, check " <<ap->hasAccess(frDirEnum::U) <<endl;
    return;
  }

  bool viainpin = false;
  auto lower_type = ap->getType(true);
  auto upper_type = ap->getType(false);
  if (layerNum >= VIAINPIN_BOTTOMLAYERNUM && layerNum <= VIAINPIN_TOPLAYERNUM) {
    viainpin = true;
  } else if ((lower_type == frAccessPointEnum::EncOpt
              && upper_type != frAccessPointEnum::NearbyGrid)
             || (upper_type == frAccessPointEnum::EncOpt
                 && lower_type != frAccessPointEnum::NearbyGrid)) {
    viainpin = true;
  }

  int maxNumViaTrial = 2;
  // use std:pair to ensure deterministic behavior
  vector<pair<int, frViaDef*>> viaDefs;
  getViasFromMetalWidthMap(bp, layerNum, polyset, viaDefs);

  if (viaDefs.empty()) {  // no via map entry
    // hardcode first two single vias
    int cnt = 0;
    for (auto& [tup, viaDef] : layerNum2ViaDefs_[layerNum + 1][1]) {
      viaDefs.push_back(make_pair(viaDefs.size(), viaDef));
      cnt++;
      if (cnt >= maxNumViaTrial) {
        break;
      }
    }
  }

  // check if ap is on the left/right boundary of the cell
  Rect boundaryBBox;
  bool isLRBound = false;
  if (instTerm) {
    boundaryBBox = instTerm->getInst()->getBoundaryBBox();
    frCoord width = getDesign()->getTech()->getLayer(layerNum)->getWidth();
    if (bp.x() <= boundaryBBox.xMin() + 3 * width
        || bp.x() >= boundaryBBox.xMax() - 3 * width) {
      isLRBound = true;
    }
  }

  set<tuple<frCoord, int, frViaDef*>> validViaDefs;
  for (auto& [idx, viaDef] : viaDefs) {
    auto via = make_unique<frVia>(viaDef);
    via->setOrigin(bp);
    Rect box = via->getLayer1BBox();
    if (instTerm) {
      if (!boundaryBBox.contains(box))
        continue;
      Rect layer2BBox = via->getLayer2BBox();
      if (!boundaryBBox.contains(layer2BBox))
        continue;
    }

    frCoord maxExt = 0;
    gtl::rectangle_data<frCoord> viarect(
        box.xMin(), box.yMin(), box.xMax(), box.yMax());
    using namespace boost::polygon::operators;
    gtl::polygon_90_set_data<frCoord> intersection;
    intersection += viarect;
    intersection &= polyset;
    // via ranking criteria: max extension distance beyond pin shape
    vector<gtl::rectangle_data<frCoord>> intRects;
    intersection.get_rectangles(intRects, gtl::orientation_2d_enum::HORIZONTAL);
    for (auto& r : intRects) {
      maxExt = max(maxExt, box.xMax() - gtl::xh(r));
      maxExt = max(maxExt, gtl::xl(r) - box.xMin());
    }
    if (!isLRBound) {
      if (intRects.size() > 1) {
        intRects.clear();
        intersection.get_rectangles(intRects,
                                    gtl::orientation_2d_enum::VERTICAL);
      }
      for (auto& r : intRects) {
        maxExt = max(maxExt, box.yMax() - gtl::yh(r));
        maxExt = max(maxExt, gtl::yl(r) - box.yMin());
      }
    }
    if (viainpin && maxExt)
      continue;
    if (prepPoint_pin_checkPoint_via_helper(ap, via.get(), pin, instTerm)) {
      validViaDefs.insert(make_tuple(maxExt, idx, viaDef));
    }
  }
  if (validViaDefs.empty()) {
    ap->setAccess(dir, false);
  }
  for (auto& [area, idx, viaDef] : validViaDefs) {
    ap->addViaDef(viaDef);
  }
}

template <typename T>
bool FlexPA::prepPoint_pin_checkPoint_via_helper(frAccessPoint* ap,
                                                 frVia* via,
                                                 T* pin,
                                                 frInstTerm* instTerm)
{
  Point bp = ap->getPoint();

  if (instTerm && instTerm->hasNet()) {
    via->addToNet(instTerm->getNet());
  } else {
    via->addToPin(pin);
  }

  // new gcWorker
  FlexGCWorker gcWorker(getTech(), logger_);
  gcWorker.setIgnoreMinArea();
  gcWorker.setIgnoreLongSideEOL();
  gcWorker.setIgnoreCornerSpacing();
  Rect extBox(bp.x() - 3000, bp.y() - 3000, bp.x() + 3000, bp.y() + 3000);
  gcWorker.setExtBox(extBox);
  gcWorker.setDrcBox(extBox);
  if (instTerm) {
    if (!instTerm->getNet() || !instTerm->getNet()->getNondefaultRule()
        || AUTO_TAPER_NDR_NETS)
      gcWorker.addTargetObj(instTerm->getInst());
  } else {
    if (!pin->getTerm()->getNet()
        || !pin->getTerm()->getNet()->getNondefaultRule()
        || AUTO_TAPER_NDR_NETS)
      gcWorker.addTargetObj(pin->getTerm());
  }

  gcWorker.initPA0(getDesign());
  frBlockObject* owner;
  if (instTerm) {
    if (instTerm->hasNet()) {
      owner = instTerm->getNet();
    } else {
      owner = instTerm;
    }
  } else {
    owner = pin->getTerm();
  }
  gcWorker.addPAObj(via, owner);
  for (auto& apPs : ap->getPathSegs())
    gcWorker.addPAObj(&apPs, owner);
  gcWorker.initPA1();
  gcWorker.main();
  gcWorker.end();

  bool sol = false;
  if (gcWorker.getMarkers().empty()) {
    sol = true;
  }
  if (graphics_) {
    graphics_->setViaAP(ap, via, gcWorker.getMarkers());
  }
  return sol;
}

template <typename T>
void FlexPA::prepPoint_pin_checkPoint(
    frAccessPoint* ap,
    const gtl::polygon_90_set_data<frCoord>& polyset,
    const vector<gtl::polygon_90_data<frCoord>>& polys,
    T* pin,
    frInstTerm* instTerm)
{
  prepPoint_pin_checkPoint_planar(ap, polys, frDirEnum::W, pin, instTerm);
  prepPoint_pin_checkPoint_planar(ap, polys, frDirEnum::E, pin, instTerm);
  prepPoint_pin_checkPoint_planar(ap, polys, frDirEnum::S, pin, instTerm);
  prepPoint_pin_checkPoint_planar(ap, polys, frDirEnum::N, pin, instTerm);
  prepPoint_pin_checkPoint_via(ap, polyset, frDirEnum::U, pin, instTerm);
}

template <typename T>
void FlexPA::prepPoint_pin_checkPoints(
    vector<unique_ptr<frAccessPoint>>& aps,
    const vector<gtl::polygon_90_set_data<frCoord>>& layerPolysets,
    T* pin,
    frInstTerm* instTerm)
{
  vector<vector<gtl::polygon_90_data<frCoord>>> layerPolys(
      layerPolysets.size());
  for (int i = 0; i < (int) layerPolysets.size(); i++) {
    layerPolysets[i].get_polygons(layerPolys[i]);
  }
  for (auto& ap : aps) {
    auto layerNum = ap->getLayerNum();
    Point pt = ap->getPoint();
    prepPoint_pin_checkPoint(
        ap.get(), layerPolysets[layerNum], layerPolys[layerNum], pin, instTerm);
  }
}

template <typename T>
void FlexPA::prepPoint_pin_updateStat(
    const vector<unique_ptr<frAccessPoint>>& tmpAps,
    T* pin,
    frInstTerm* instTerm)
{
  bool isStdCellPin = false;
  bool isMacroCellPin = false;
  if (instTerm) {
    // TODO there should be a better way to get this info by getting the master
    // terms from OpenDB
    dbMasterType masterType = instTerm->getInst()->getMaster()->getMasterType();
    isStdCellPin = masterType == dbMasterType::CORE
                   || masterType == dbMasterType::CORE_TIEHIGH
                   || masterType == dbMasterType::CORE_TIELOW
                   || masterType == dbMasterType::CORE_ANTENNACELL;

    isMacroCellPin = masterType.isBlock() || masterType.isPad()
                     || masterType == dbMasterType::RING;
  }
  for (auto& ap : tmpAps) {
    if (ap->hasAccess(frDirEnum::W) || ap->hasAccess(frDirEnum::E)
        || ap->hasAccess(frDirEnum::S) || ap->hasAccess(frDirEnum::N)) {
      if (isStdCellPin) {
#pragma omp atomic
        stdCellPinValidPlanarApCnt_++;
      }
      if (isMacroCellPin) {
#pragma omp atomic
        macroCellPinValidPlanarApCnt_++;
      }
    }
    if (ap->hasAccess(frDirEnum::U)) {
      if (isStdCellPin) {
#pragma omp atomic
        stdCellPinValidViaApCnt_++;
      }
      if (isMacroCellPin) {
#pragma omp atomic
        macroCellPinValidViaApCnt_++;
      }
    }
  }
}

template <typename T>
bool FlexPA::prepPoint_pin_helper(
    vector<unique_ptr<frAccessPoint>>& aps,
    set<pair<Point, frLayerNum>>& apset,
    vector<gtl::polygon_90_set_data<frCoord>>& pinShapes,
    T* pin,
    frInstTerm* instTerm,
    frAccessPointEnum lowerType,
    frAccessPointEnum upperType)
{
  bool isStdCellPin = false;
  bool isMacroCellPin = false;
  if (instTerm) {
    // TODO there should be a better way to get this info by getting the master
    // terms from OpenDB
    dbMasterType masterType = instTerm->getInst()->getMaster()->getMasterType();
    isStdCellPin = masterType == dbMasterType::CORE
                   || masterType == dbMasterType::CORE_TIEHIGH
                   || masterType == dbMasterType::CORE_TIELOW
                   || masterType == dbMasterType::CORE_ANTENNACELL;

    isMacroCellPin = masterType.isBlock() || masterType.isPad()
                     || masterType == dbMasterType::RING;
  }
  bool isIOPin = (instTerm == nullptr);
  vector<unique_ptr<frAccessPoint>> tmpAps;
  prepPoint_pin_genPoints(
      tmpAps, apset, pin, instTerm, pinShapes, lowerType, upperType);
  prepPoint_pin_checkPoints(tmpAps, pinShapes, pin, instTerm);
  if (isStdCellPin) {
#pragma omp atomic
    stdCellPinGenApCnt_ += tmpAps.size();
  }
  if (isMacroCellPin) {
#pragma omp atomic
    macroCellPinGenApCnt_ += tmpAps.size();
  }
  if (graphics_) {
    graphics_->setAPs(tmpAps, lowerType, upperType);
  }
  for (auto& ap : tmpAps) {
    // for stdcell, add (i) planar access if layerNum != VIA_ACCESS_LAYERNUM,
    // and (ii) access if exist access for macro, allow pure planar ap
    if (isStdCellPin) {
      auto layerNum = ap->getLayerNum();
      if ((layerNum == VIA_ACCESS_LAYERNUM && ap->hasAccess(frDirEnum::U))
          || (layerNum != VIA_ACCESS_LAYERNUM && ap->hasAccess())) {
        aps.push_back(std::move(ap));
      }
    } else if ((isMacroCellPin || isIOPin) && ap->hasAccess()) {
      aps.push_back(std::move(ap));
    }
  }
  int nSparseAPs = (int) aps.size();
  Rect tbx;
  for (int i = 0; i < (int) aps.size();
       i++) {  // not perfect but will do the job
    int r = design_->getTech()->getLayer(aps[i]->getLayerNum())->getWidth() / 2;
    tbx.init(
        aps[i]->x() - r, aps[i]->y() - r, aps[i]->x() + r, aps[i]->y() + r);
    for (int j = i + 1; j < (int) aps.size(); j++) {
      if (aps[i]->getLayerNum() == aps[j]->getLayerNum()
          && tbx.intersects(aps[j]->getPoint())) {
        nSparseAPs--;
        break;
      }
    }
  }
  if (isStdCellPin && nSparseAPs >= MINNUMACCESSPOINT_STDCELLPIN) {
    prepPoint_pin_updateStat(aps, pin, instTerm);
    // write to pa
    auto it = unique2paidx_.find(instTerm->getInst());
    if (it == unique2paidx_.end()) {
      logger_->error(DRT, 71, "prepPoint_pin_helper unique2paidx not found.");
    } else {
      for (auto& ap : aps) {
        pin->getPinAccess(it->second)->addAccessPoint(std::move(ap));
      }
    }
    return true;
  }
  if (isMacroCellPin && nSparseAPs >= MINNUMACCESSPOINT_MACROCELLPIN) {
    prepPoint_pin_updateStat(aps, pin, instTerm);
    // write to pa
    auto it = unique2paidx_.find(instTerm->getInst());
    if (it == unique2paidx_.end()) {
      logger_->error(DRT, 72, "prepPoint_pin_helper unique2paidx not found.");
    } else {
      for (auto& ap : aps) {
        pin->getPinAccess(it->second)->addAccessPoint(std::move(ap));
      }
    }
    return true;
  }
  if (isIOPin && (int) aps.size() > 0) {
    // IO term pin always only have one access
    for (auto& ap : aps) {
      pin->getPinAccess(0)->addAccessPoint(std::move(ap));
    }
    return true;
  }
  return false;
}

// first create all access points with costs
template <typename T>
int FlexPA::prepPoint_pin(T* pin, frInstTerm* instTerm)
{
  // aps are after xform
  // before checkPoints, ap->hasAccess(dir) indicates whether to check drc
  vector<unique_ptr<frAccessPoint>> aps;
  set<pair<Point, frLayerNum>> apset;
  bool isStdCellPin = false;
  bool isMacroCellPin = false;
  if (instTerm) {
    // TODO there should be a better way to get this info by getting the master
    // terms from OpenDB
    dbMasterType masterType = instTerm->getInst()->getMaster()->getMasterType();
    isStdCellPin = masterType == dbMasterType::CORE
                   || masterType == dbMasterType::CORE_TIEHIGH
                   || masterType == dbMasterType::CORE_TIELOW
                   || masterType == dbMasterType::CORE_ANTENNACELL;

    isMacroCellPin = masterType.isBlock() || masterType.isPad()
                     || masterType == dbMasterType::RING;
  }

  if (graphics_) {
    set<frInst*, frBlockObjectComp>* instClass = nullptr;
    if (instTerm) {
      instClass = inst2Class_[instTerm->getInst()];
    }
    graphics_->startPin(pin, instTerm, instClass);
  }

  vector<gtl::polygon_90_set_data<frCoord>> pinShapes;
  prepPoint_pin_mergePinShapes(pinShapes, pin, instTerm);

  for (auto upper : {frAccessPointEnum::OnGrid,
                     frAccessPointEnum::HalfGrid,
                     frAccessPointEnum::Center,
                     frAccessPointEnum::EncOpt,
                     frAccessPointEnum::NearbyGrid}) {
    for (auto lower : {frAccessPointEnum::OnGrid,
                       frAccessPointEnum::HalfGrid,
                       frAccessPointEnum::Center,
                       frAccessPointEnum::EncOpt}) {
      if (upper == frAccessPointEnum::NearbyGrid && !aps.empty()) {
        // Only use NearbyGrid as a last resort (at least until
        // nangate45/aes is resolved).
        continue;
      }
      if (prepPoint_pin_helper(
              aps, apset, pinShapes, pin, instTerm, lower, upper)) {
        return aps.size();
      }
    }
  }

  // instTerm aps are written back here if not early stopped
  // IO term aps are are written back in prepPoint_pin_helper and always early
  // stopped
  prepPoint_pin_updateStat(aps, pin, instTerm);
  int nAps = aps.size();
  if (nAps == 0) {
    if (isStdCellPin) {
      stdCellPinNoApCnt_++;
    }
    if (isMacroCellPin) {
      macroCellPinNoApCnt_++;
    }
  } else {
    // write to pa
    auto it = unique2paidx_.find(instTerm->getInst());
    if (it == unique2paidx_.end()) {
      logger_->error(DRT, 75, "prepPoint_pin unique2paidx not found.");
    } else {
      for (auto& ap : aps) {
        pin->getPinAccess(it->second)->addAccessPoint(std::move(ap));
      }
    }
  }
  return nAps;
}

void FlexPA::prepPoint()
{
  ProfileTask profile("PA:point");
  int cnt = 0;

  omp_set_num_threads(MAX_THREADS);
  ThreadException exception;
#pragma omp parallel for schedule(dynamic)
  for (int i = 0; i < (int) uniqueInstances_.size(); i++) {
    try {
      auto& inst = uniqueInstances_[i];
      // only do for core and block cells
      dbMasterType masterType = inst->getMaster()->getMasterType();
      if (masterType != dbMasterType::CORE
          && masterType != dbMasterType::CORE_TIEHIGH
          && masterType != dbMasterType::CORE_TIELOW
          && masterType != dbMasterType::CORE_ANTENNACELL
          && !masterType.isBlock() && !masterType.isPad()
          && masterType != dbMasterType::RING) {
        continue;
      }
      ProfileTask profile("PA:uniqueInstance");
      for (auto& instTerm : inst->getInstTerms()) {
        // only do for normal and clock terms
        if (isSkipInstTerm(instTerm.get())) {
          continue;
        }
        int nAps = 0;
        for (auto& pin : instTerm->getTerm()->getPins()) {
          nAps += prepPoint_pin(pin.get(), instTerm.get());
        }
        if (!nAps) {
          logger_->error(DRT,
                         73,
                         "No access point for {}/{}.",
                         instTerm->getInst()->getName(),
                         instTerm->getTerm()->getName());
        }
#pragma omp critical
        {
          cnt++;
          if (VERBOSE > 0) {
            if (cnt < 1000) {
              if (cnt % 100 == 0) {
                logger_->info(DRT, 76, "  Complete {} pins.", cnt);
              }
            } else {
              if (cnt % 1000 == 0) {
                logger_->info(DRT, 77, "  Complete {} pins.", cnt);
              }
            }
          }
        }
      }
    } catch (...) {
      exception.capture();
    }
  }
  exception.rethrow();

  // cout << "PA for IO terms\n" << flush;

  // PA for IO terms
  if (target_insts_.empty()) {
    omp_set_num_threads(MAX_THREADS);
#pragma omp parallel for schedule(dynamic)
    for (unsigned i = 0; i < getDesign()->getTopBlock()->getTerms().size();
         i++) {
      try {
        auto& term = getDesign()->getTopBlock()->getTerms()[i];
        if (term.get()->getType().isSupply()) {
          continue;
        }
        if (term->getNet() == nullptr) {
          continue;
        }
        int nAps = 0;
        for (auto& pin : term->getPins()) {
          nAps += prepPoint_pin(pin.get(), nullptr);
        }
        if (!nAps) {
          logger_->error(
              DRT, 74, "No access point for PIN/{}.", term->getName());
        }
      } catch (...) {
        exception.capture();
      }
    }
    exception.rethrow();
  }

  if (VERBOSE > 0) {
    logger_->info(DRT, 78, "  Complete {} pins.", cnt);
  }
}

void FlexPA::prepPattern()
{
  ProfileTask profile("PA:pattern");

  // revert access points to origin
  uniqueInstPatterns_.resize(uniqueInstances_.size());

  int cnt = 0;

  omp_set_num_threads(MAX_THREADS);
  ThreadException exception;
#pragma omp parallel for schedule(dynamic)
  for (int currUniqueInstIdx = 0;
       currUniqueInstIdx < (int) uniqueInstances_.size();
       currUniqueInstIdx++) {
    try {
      auto& inst = uniqueInstances_[currUniqueInstIdx];
      // only do for core and block cells
      // TODO the above comment says "block cells" but that's not what the code
      // does?
      dbMasterType masterType = inst->getMaster()->getMasterType();
      if (masterType != dbMasterType::CORE
          && masterType != dbMasterType::CORE_TIEHIGH
          && masterType != dbMasterType::CORE_TIELOW
          && masterType != dbMasterType::CORE_ANTENNACELL) {
        continue;
      }

      int numValidPattern = prepPattern_inst(inst, currUniqueInstIdx, 1.0);

      if (numValidPattern == 0) {
        // In FAx1_ASAP7_75t_R (in asap7) the pins are mostly horizontal
        // and sorting in X works poorly.  So we try again sorting in Y.
        numValidPattern = prepPattern_inst(inst, currUniqueInstIdx, 0.0);
        if (numValidPattern == 0) {
          logger_->warn(
              DRT,
              87,
              "No valid pattern for unique instance {}, master is {}.",
              inst->getName(),
              inst->getMaster()->getName());
        }
      }
#pragma omp critical
      {
        cnt++;
        if (VERBOSE > 0) {
          if (cnt < 1000) {
            if (cnt % 100 == 0) {
              logger_->info(
                  DRT, 79, "  Complete {} unique inst patterns.", cnt);
            }
          } else {
            if (cnt % 1000 == 0) {
              logger_->info(
                  DRT, 80, "  Complete {} unique inst patterns.", cnt);
            }
          }
        }
      }
    } catch (...) {
      exception.capture();
    }
  }
  exception.rethrow();
  if (VERBOSE > 0) {
    logger_->info(DRT, 81, "  Complete {} unique inst patterns.", cnt);
  }

  // prep pattern for each row
  std::vector<frInst*> insts;
  std::vector<std::vector<frInst*>> instRows;
  std::vector<frInst*> rowInsts;

  auto instLocComp = [](frInst* const& a, frInst* const& b) {
    Point originA = a->getOrigin();
    Point originB = b->getOrigin();
    if (originA.y() == originB.y()) {
      return (originA.x() < originB.x());
    } else {
      return (originA.y() < originB.y());
    }
  };

  getInsts(insts);
  std::sort(insts.begin(), insts.end(), instLocComp);

  // gen rows of insts
  int prevYCoord = INT_MIN;
  int prevXEndCoord = INT_MIN;
  for (auto inst : insts) {
    Point origin = inst->getOrigin();
    // cout << inst->getName() << " (" << origin.x() << ", " << origin.y() << ")
    // -- ";
    if (origin.y() != prevYCoord || origin.x() > prevXEndCoord) {
      // cout << "\n";
      if (!rowInsts.empty()) {
        instRows.push_back(rowInsts);
        rowInsts.clear();
      }
    }
    rowInsts.push_back(inst);
    prevYCoord = origin.y();
    Rect instBoundaryBox = inst->getBoundaryBBox();
    prevXEndCoord = instBoundaryBox.xMax();
  }
  if (!rowInsts.empty()) {
    instRows.push_back(rowInsts);
  }

  // choose access pattern of a row of insts
  int rowIdx = 0;
  cnt = 0;
  // for (auto &instRow: instRows) {
  omp_set_num_threads(MAX_THREADS);
#pragma omp parallel for schedule(dynamic)
  for (int i = 0; i < (int) instRows.size(); i++) {
    try {
      auto& instRow = instRows[i];
      genInstRowPattern(instRow);
#pragma omp critical
      {
        rowIdx++;
        cnt++;
        if (VERBOSE > 0) {
          if (cnt < 10000) {
            if (cnt % 1000 == 0) {
              logger_->info(DRT, 82, "  Complete {} groups.", cnt);
            }
          } else {
            if (cnt % 10000 == 0) {
              logger_->info(DRT, 83, "  Complete {} groups.", cnt);
            }
          }
        }
      }
    } catch (...) {
      exception.capture();
    }
  }
  exception.rethrow();
  if (VERBOSE > 0) {
    logger_->info(DRT, 84, "  Complete {} groups.", cnt);
  }
}

void FlexPA::revertAccessPoints()
{
  for (auto& inst : uniqueInstances_) {
    dbTransform xform = inst->getTransform();
    Point offset(xform.getOffset());
    dbTransform revertXform;
    revertXform.setOffset(Point(-offset.getX(), -offset.getY()));
    revertXform.setOrient(dbOrientType::R0);

    auto paIdx = unique2paidx_[inst];
    for (auto& instTerm : inst->getInstTerms()) {
      // if (isSkipInstTerm(instTerm.get())) {
      //   continue;
      // }

      for (auto& pin : instTerm->getTerm()->getPins()) {
        auto pinAccess = pin->getPinAccess(paIdx);
        for (auto& accessPoint : pinAccess->getAccessPoints()) {
          Point uniqueAPPoint(accessPoint->getPoint());
          revertXform.apply(uniqueAPPoint);
          accessPoint->setPoint(uniqueAPPoint);
          for (auto& ps : accessPoint->getPathSegs()) {
            Point begin = ps.getBeginPoint();
            Point end = ps.getEndPoint();
            revertXform.apply(begin);
            revertXform.apply(end);
            if (end < begin) {
              Point tmp = begin;
              begin = end;
              end = tmp;
            }
            ps.setPoints(begin, end);
          }
        }
      }
    }
  }
}

// calculate which pattern to be used for each inst
// the insts must be in the same row and sorted from left to right
void FlexPA::genInstRowPattern(std::vector<frInst*>& insts)
{
  if (insts.empty()) {
    return;
  }

  int numNode = (insts.size() + 2) * ACCESS_PATTERN_END_ITERATION_NUM;

  std::vector<FlexDPNode> nodes(numNode);

  genInstRowPattern_init(nodes, insts);
  genInstRowPattern_perform(nodes, insts);
  genInstRowPattern_commit(nodes, insts);
}

// init dp node array for valid access patterns
void FlexPA::genInstRowPattern_init(std::vector<FlexDPNode>& nodes,
                                    const std::vector<frInst*>& insts)
{
  // init virtual nodes
  int startNodeIdx = getFlatIdx(-1, 0, ACCESS_PATTERN_END_ITERATION_NUM);
  int endNodeIdx
      = getFlatIdx(insts.size(), 0, ACCESS_PATTERN_END_ITERATION_NUM);
  nodes[startNodeIdx].setNodeCost(0);
  nodes[startNodeIdx].setPathCost(0);
  nodes[endNodeIdx].setNodeCost(0);

  // init inst nodes
  for (int idx1 = 0; idx1 < (int) insts.size(); idx1++) {
    auto& inst = insts[idx1];
    auto& uniqueInst = inst2unique_[inst];
    auto uniqueInstIdx = unique2Idx_[uniqueInst];
    auto& instPatterns = uniqueInstPatterns_[uniqueInstIdx];
    for (int idx2 = 0; idx2 < (int) instPatterns.size(); idx2++) {
      int nodeIdx = getFlatIdx(idx1, idx2, ACCESS_PATTERN_END_ITERATION_NUM);
      auto accessPattern = instPatterns[idx2].get();
      nodes[nodeIdx].setNodeCost(accessPattern->getCost());
    }
  }
}

void FlexPA::genInstRowPattern_perform(std::vector<FlexDPNode>& nodes,
                                       const std::vector<frInst*>& insts)
{
  for (int currIdx1 = 0; currIdx1 <= (int) insts.size(); currIdx1++) {
    bool isSet = false;
    for (int currIdx2 = 0; currIdx2 < ACCESS_PATTERN_END_ITERATION_NUM;
         currIdx2++) {
      auto currNodeIdx
          = getFlatIdx(currIdx1, currIdx2, ACCESS_PATTERN_END_ITERATION_NUM);
      auto& currNode = nodes[currNodeIdx];
      if (currNode.getNodeCost() == std::numeric_limits<int>::max()) {
        continue;
      }
      int prevIdx1 = currIdx1 - 1;
      for (int prevIdx2 = 0; prevIdx2 < ACCESS_PATTERN_END_ITERATION_NUM;
           prevIdx2++) {
        int prevNodeIdx
            = getFlatIdx(prevIdx1, prevIdx2, ACCESS_PATTERN_END_ITERATION_NUM);
        auto& prevNode = nodes[prevNodeIdx];
        if (prevNode.getPathCost() == std::numeric_limits<int>::max()) {
          continue;
        }

        int edgeCost = getEdgeCost(prevNodeIdx, currNodeIdx, nodes, insts);
        if (currNode.getPathCost() == std::numeric_limits<int>::max()
            || currNode.getPathCost() > prevNode.getPathCost() + edgeCost) {
          currNode.setPathCost(prevNode.getPathCost() + edgeCost);
          currNode.setPrevNodeIdx(prevNodeIdx);
          isSet = true;
        }
      }
    }
    if (!isSet) {
      cout << "@@@ dead end inst\n";
    }
  }
}

void FlexPA::genInstRowPattern_commit(std::vector<FlexDPNode>& nodes,
                                      const std::vector<frInst*>& insts)
{
  // bool isDebugMode = true;
  bool isDebugMode = false;
  int currNodeIdx
      = getFlatIdx(insts.size(), 0, ACCESS_PATTERN_END_ITERATION_NUM);
  auto currNode = &(nodes[currNodeIdx]);
  int instCnt = insts.size();
  std::vector<int> instAccessPatternIdx(insts.size(), -1);
  while (currNode->getPrevNodeIdx() != -1) {
    // non-virtual node
    if (instCnt != (int) insts.size()) {
      int currIdx1, currIdx2;
      getNestedIdx(
          currNodeIdx, currIdx1, currIdx2, ACCESS_PATTERN_END_ITERATION_NUM);
      instAccessPatternIdx[currIdx1] = currIdx2;

      auto& inst = insts[currIdx1];
      int accessPointIdx = 0;
      auto& uniqueInst = inst2unique_[inst];
      auto& uniqueInstIdx = unique2Idx_[uniqueInst];
      auto accessPattern = uniqueInstPatterns_[uniqueInstIdx][currIdx2].get();
      auto& accessPoints = accessPattern->getPattern();

      // update instTerm ap
      for (auto& instTerm : inst->getInstTerms()) {
        if (isSkipInstTerm(instTerm.get())) {
          continue;
        }

        int pinIdx = 0;
        // to avoid unused variable warning in GCC
        for (int i = 0; i < (int) (instTerm->getTerm()->getPins().size());
             i++) {
          auto& accessPoint = accessPoints[accessPointIdx];
          instTerm->setAccessPoint(pinIdx, accessPoint);
          pinIdx++;
          accessPointIdx++;
        }
      }
    }
    currNodeIdx = currNode->getPrevNodeIdx();
    currNode = &(nodes[currNode->getPrevNodeIdx()]);
    instCnt--;
  }

  if (instCnt != -1) {
    std::string inst_names;
    for (frInst* inst : insts) {
      inst_names += '\n' + inst->getName();
    }
    logger_->error(DRT,
                   85,
                   "Valid access pattern combination not found for {}",
                   inst_names);
  }

  // for (int i = 0; i < (int)instAccessPatternIdx.size(); i++) {
  //   cout << instAccessPatternIdx[i] << " ";
  // }
  // cout << "\n";

  if (isDebugMode) {
    genInstRowPattern_print(nodes, insts);
  }
}

void FlexPA::genInstRowPattern_print(std::vector<FlexDPNode>& nodes,
                                     const std::vector<frInst*>& insts)
{
  int currNodeIdx
      = getFlatIdx(insts.size(), 0, ACCESS_PATTERN_END_ITERATION_NUM);
  auto currNode = &(nodes[currNodeIdx]);
  int instCnt = insts.size();
  std::vector<int> instAccessPatternIdx(insts.size(), -1);

  while (currNode->getPrevNodeIdx() != -1) {
    // non-virtual node
    if (instCnt != (int) insts.size()) {
      int currIdx1, currIdx2;
      getNestedIdx(
          currNodeIdx, currIdx1, currIdx2, ACCESS_PATTERN_END_ITERATION_NUM);
      instAccessPatternIdx[currIdx1] = currIdx2;

      // print debug information
      auto& inst = insts[currIdx1];
      int accessPointIdx = 0;
      auto& uniqueInst = inst2unique_[inst];
      auto& uniqueInstIdx = unique2Idx_[uniqueInst];
      auto accessPattern = uniqueInstPatterns_[uniqueInstIdx][currIdx2].get();
      auto& accessPoints = accessPattern->getPattern();

      for (auto& instTerm : inst->getInstTerms()) {
        if (isSkipInstTerm(instTerm.get())) {
          continue;
        }

        // for (auto &pin: instTerm->getTerm()->getPins()) {
        //  to avoid unused variable warning in GCC
        for (int i = 0; i < (int) (instTerm->getTerm()->getPins().size());
             i++) {
          auto& accessPoint = accessPoints[accessPointIdx];
          if (accessPoint) {
            Point pt(accessPoint->getPoint());
            if (instTerm->hasNet()) {
              cout << " gcclean2via " << inst->getName() << " "
                   << instTerm->getTerm()->getName() << " "
                   << accessPoint->getViaDef()->getName() << " " << pt.x()
                   << " " << pt.y() << " " << inst->getOrient().getString()
                   << "\n";
              instTermValidViaApCnt_++;
            }
          }
          accessPointIdx++;
        }
      }
    }
    currNodeIdx = currNode->getPrevNodeIdx();
    currNode = &(nodes[currNode->getPrevNodeIdx()]);
    instCnt--;
  }

  cout << flush;

  if (instCnt != -1) {
    logger_->error(DRT, 276, "Valid access pattern combination not found.");
  }
}

int FlexPA::getEdgeCost(int prevNodeIdx,
                        int currNodeIdx,
                        std::vector<FlexDPNode>& nodes,
                        const std::vector<frInst*>& insts)
{
  int edgeCost = 0;
  int prevIdx1, prevIdx2, currIdx1, currIdx2;
  getNestedIdx(
      prevNodeIdx, prevIdx1, prevIdx2, ACCESS_PATTERN_END_ITERATION_NUM);
  getNestedIdx(
      currNodeIdx, currIdx1, currIdx2, ACCESS_PATTERN_END_ITERATION_NUM);
  if (prevIdx1 == -1 || currIdx1 == (int) insts.size()) {
    return edgeCost;
  }

  bool hasVio = false;

  // check DRC
  std::vector<std::unique_ptr<frVia>> tempVias;
  std::vector<std::pair<frConnFig*, frBlockObject*>> objs;
  // push the vias from prev inst access pattern and curr inst access pattern
  auto prevInst = insts[prevIdx1];
  auto prevUniqueInstIdx = unique2Idx_[inst2unique_[prevInst]];
  auto currInst = insts[currIdx1];
  auto currUniqueInstIdx = unique2Idx_[inst2unique_[currInst]];
  auto prevPinAccessPattern
      = uniqueInstPatterns_[prevUniqueInstIdx][prevIdx2].get();
  auto currPinAccessPattern
      = uniqueInstPatterns_[currUniqueInstIdx][currIdx2].get();
  addAccessPatternObj(prevInst, prevPinAccessPattern, objs, tempVias, true);
  addAccessPatternObj(currInst, currPinAccessPattern, objs, tempVias, false);

  hasVio = !genPatterns_gc({prevInst, currInst}, objs, Edge);
  if (!hasVio) {
    int prevNodeCost = nodes[prevNodeIdx].getNodeCost();
    int currNodeCost = nodes[currNodeIdx].getNodeCost();
    edgeCost = (prevNodeCost + currNodeCost) / 2;
  } else {
    edgeCost = 1000;
  }

  return edgeCost;
}

void FlexPA::addAccessPatternObj(
    frInst* inst,
    FlexPinAccessPattern* accessPattern,
    std::vector<std::pair<frConnFig*, frBlockObject*>>& objs,
    std::vector<std::unique_ptr<frVia>>& vias,
    bool isPrev)
{
  dbTransform xform = inst->getUpdatedXform(true);
  int accessPointIdx = 0;
  auto& accessPoints = accessPattern->getPattern();

  for (auto& instTerm : inst->getInstTerms()) {
    if (isSkipInstTerm(instTerm.get())) {
      continue;
    }

    // to avoid unused variable warning in GCC
    for (int i = 0; i < (int) (instTerm->getTerm()->getPins().size()); i++) {
      auto& accessPoint = accessPoints[accessPointIdx];
      // cout <<"@@@test: isPrev = " <<isPrev <<", apoint "
      //      <<accessPoint <<", leftAPoint "
      //      <<accessPattern->getBoundaryAP(true) <<", rightAPoint "
      //      <<accessPattern->getBoundaryAP(false) <<endl <<flush;
      if (!accessPoint
          || (isPrev && accessPoint != accessPattern->getBoundaryAP(false))) {
        accessPointIdx++;
        continue;
      }
      if ((!isPrev) && accessPoint != accessPattern->getBoundaryAP(true)) {
        accessPointIdx++;
        continue;
      }
      if (accessPoint->hasAccess(frDirEnum::U)) {
        std::unique_ptr<frVia> via
            = std::make_unique<frVia>(accessPoint->getViaDef());
        Point pt(accessPoint->getPoint());
        xform.apply(pt);
        via->setOrigin(pt);
        auto rvia = via.get();
        if (instTerm->hasNet()) {
          objs.push_back(std::make_pair(rvia, instTerm->getNet()));
        } else {
          objs.push_back(std::make_pair(rvia, instTerm.get()));
        }
        vias.push_back(std::move(via));
      }
      accessPointIdx++;
    }
  }
}

void FlexPA::getInsts(std::vector<frInst*>& insts)
{
  std::set<frInst*> target_frinsts;
  for (auto inst : target_insts_)
    target_frinsts.insert(design_->getTopBlock()->findInst(inst->getName()));
  for (auto& inst : design_->getTopBlock()->getInsts()) {
    if (!target_insts_.empty()
        && target_frinsts.find(inst.get()) == target_frinsts.end())
      continue;
    if (inst2unique_.find(inst.get()) == inst2unique_.end())
      continue;
    dbMasterType masterType = inst->getMaster()->getMasterType();
    if (masterType != dbMasterType::CORE
        && masterType != dbMasterType::CORE_TIEHIGH
        && masterType != dbMasterType::CORE_TIELOW
        && masterType != dbMasterType::CORE_ANTENNACELL) {
      continue;
    }
    bool isSkip = true;
    for (auto& instTerm : inst->getInstTerms()) {
      if (!isSkipInstTerm(instTerm.get())) {
        isSkip = false;
        break;
      }
    }
    if (!isSkip) {
      insts.push_back(inst.get());
    }
  }
}

bool FlexPA::isSkipInstTerm(frInstTerm* in)
{
  if (in->getTerm()->getType().isSupply())
    return true;
  if (!in->getNet() || in->getNet()->isSpecial()) {
    auto instClass = inst2Class_[in->getInst()];
    if (instClass != nullptr) {
      for (auto& inst : *instClass) {
        frInstTerm* it = inst->getInstTerm(in->getTerm()->getName());
        if (!in->getNet()) {
          if (it->getNet()) {
            return false;
          }
        } else if (in->getNet()->isSpecial()) {
          if (it->getNet() && !it->getNet()->isSpecial()) {
            return false;
          }
        }
      }
    }
    return true;
  }
  return false;
}

// the input inst must be unique instance
int FlexPA::prepPattern_inst(frInst* inst,
                             const int currUniqueInstIdx,
                             const double xWeight)
{
  std::vector<std::pair<frCoord, std::pair<frMPin*, frInstTerm*>>> pins;
  // TODO: add assert in case input inst is not unique inst
  int paIdx = unique2paidx_[inst];
  for (auto& instTerm : inst->getInstTerms()) {
    if (isSkipInstTerm(instTerm.get())) {
      continue;
    }
    int nAps = 0;
    for (auto& pin : instTerm->getTerm()->getPins()) {
      // container of access points
      auto pinAccess = pin->getPinAccess(paIdx);
      int sumXCoord = 0;
      int sumYCoord = 0;
      int cnt = 0;
      // get avg x coord for sort
      for (auto& accessPoint : pinAccess->getAccessPoints()) {
        sumXCoord += accessPoint->getPoint().x();
        sumYCoord += accessPoint->getPoint().y();
        cnt++;
      }
      nAps += cnt;
      if (cnt != 0) {
        const double coord
            = (xWeight * sumXCoord + (1.0 - xWeight) * sumYCoord) / cnt;
        pins.push_back({(int) std::round(coord), {pin.get(), instTerm.get()}});
      }
    }
    if (nAps == 0 && instTerm->getTerm()->getPins().size())
      logger_->error(DRT, 86, "Pin does not have an access point.");
  }
  std::sort(pins.begin(),
            pins.end(),
            [](const std::pair<frCoord, std::pair<frMPin*, frInstTerm*>>& lhs,
               const std::pair<frCoord, std::pair<frMPin*, frInstTerm*>>& rhs) {
              return lhs.first < rhs.first;
            });

  std::vector<std::pair<frMPin*, frInstTerm*>> pinInstTermPairs;
  for (auto& [x, m] : pins) {
    pinInstTermPairs.push_back(m);
  }

  return genPatterns(pinInstTermPairs, currUniqueInstIdx);
}

int FlexPA::genPatterns(
    const std::vector<std::pair<frMPin*, frInstTerm*>>& pins,
    int currUniqueInstIdx)
{
  if (pins.empty()) {
    return -1;
  }

  int maxAccessPointSize = 0;
  int paIdx = unique2paidx_[pins[0].second->getInst()];
  for (auto& [pin, instTerm] : pins) {
    maxAccessPointSize = std::max(
        maxAccessPointSize, pin->getPinAccess(paIdx)->getNumAccessPoints());
  }
  int numNode = (pins.size() + 2) * maxAccessPointSize;
  int numEdge = numNode * maxAccessPointSize;

  std::vector<FlexDPNode> nodes(numNode);
  std::vector<int> vioEdge(numEdge, -1);
  // moved for mt
  std::set<std::vector<int>> instAccessPatterns;
  std::set<std::pair<int, int>> usedAccessPoints;
  std::set<std::pair<int, int>> violAccessPoints;

  genPatterns_init(nodes,
                   pins,
                   instAccessPatterns,
                   usedAccessPoints,
                   violAccessPoints,
                   maxAccessPointSize);
  int numValidPattern = 0;
  for (int i = 0; i < ACCESS_PATTERN_END_ITERATION_NUM; i++) {
    genPatterns_reset(nodes, pins, maxAccessPointSize);
    genPatterns_perform(nodes,
                        pins,
                        vioEdge,
                        usedAccessPoints,
                        violAccessPoints,
                        currUniqueInstIdx,
                        maxAccessPointSize);
    bool isValid = false;
    if (genPatterns_commit(nodes,
                           pins,
                           isValid,
                           instAccessPatterns,
                           usedAccessPoints,
                           violAccessPoints,
                           currUniqueInstIdx,
                           maxAccessPointSize)) {
      if (isValid) {
        numValidPattern++;
      } else {
      }
    } else {
      break;
    }
  }

  // try reverse order if no valid pattern
  if (numValidPattern == 0) {
    auto reversedPins = pins;
    reverse(reversedPins.begin(), reversedPins.end());

    std::vector<FlexDPNode> nodes(numNode);
    std::vector<int> vioEdge(numEdge, -1);

    genPatterns_init(nodes,
                     reversedPins,
                     instAccessPatterns,
                     usedAccessPoints,
                     violAccessPoints,
                     maxAccessPointSize);
    for (int i = 0; i < ACCESS_PATTERN_END_ITERATION_NUM; i++) {
      genPatterns_reset(nodes, reversedPins, maxAccessPointSize);
      genPatterns_perform(nodes,
                          reversedPins,
                          vioEdge,
                          usedAccessPoints,
                          violAccessPoints,
                          currUniqueInstIdx,
                          maxAccessPointSize);
      bool isValid = false;
      if (genPatterns_commit(nodes,
                             reversedPins,
                             isValid,
                             instAccessPatterns,
                             usedAccessPoints,
                             violAccessPoints,
                             currUniqueInstIdx,
                             maxAccessPointSize)) {
        if (isValid) {
          numValidPattern++;
        } else {
        }
      } else {
        break;
      }
    }
  }

  return numValidPattern;
}

// init dp node array for valid access points
void FlexPA::genPatterns_init(
    std::vector<FlexDPNode>& nodes,
    const std::vector<std::pair<frMPin*, frInstTerm*>>& pins,
    std::set<std::vector<int>>& instAccessPatterns,
    std::set<std::pair<int, int>>& usedAccessPoints,
    std::set<std::pair<int, int>>& violAccessPoints,
    int maxAccessPointSize)
{
  // clear temp storage and flag
  instAccessPatterns.clear();
  usedAccessPoints.clear();
  violAccessPoints.clear();

  // init virtual nodes
  int startNodeIdx = getFlatIdx(-1, 0, maxAccessPointSize);
  int endNodeIdx = getFlatIdx(pins.size(), 0, maxAccessPointSize);
  nodes[startNodeIdx].setNodeCost(0);
  nodes[startNodeIdx].setPathCost(0);
  nodes[endNodeIdx].setNodeCost(0);
  // init pin nodes
  int pinIdx = 0;
  int apIdx = 0;
  int paIdx = unique2paidx_[pins[0].second->getInst()];

  for (auto& [pin, instTerm] : pins) {
    apIdx = 0;
    for (auto& ap : pin->getPinAccess(paIdx)->getAccessPoints()) {
      int nodeIdx = getFlatIdx(pinIdx, apIdx, maxAccessPointSize);
      nodes[nodeIdx].setNodeCost(ap->getCost());
      apIdx++;
    }
    pinIdx++;
  }
}

void FlexPA::genPatterns_reset(
    std::vector<FlexDPNode>& nodes,
    const std::vector<std::pair<frMPin*, frInstTerm*>>& pins,
    int maxAccessPointSize)
{
  for (int i = 0; i < (int) nodes.size(); i++) {
    auto node = &nodes[i];
    node->setPathCost(std::numeric_limits<int>::max());
    node->setPrevNodeIdx(-1);
  }

  int startNodeIdx = getFlatIdx(-1, 0, maxAccessPointSize);
  int endNodeIdx = getFlatIdx(pins.size(), 0, maxAccessPointSize);
  nodes[startNodeIdx].setNodeCost(0);
  nodes[startNodeIdx].setPathCost(0);
  nodes[endNodeIdx].setNodeCost(0);
}

// objs must hold at least 1 obj
bool FlexPA::genPatterns_gc(std::set<frBlockObject*> targetObjs,
                            vector<pair<frConnFig*, frBlockObject*>>& objs,
                            const PatternType patternType,
                            std::set<frBlockObject*>* owners)
{
  gcCallCnt++;
  if (objs.empty()) {
    if (VERBOSE > 1) {
      logger_->warn(DRT, 89, "genPattern_gc objs empty.");
    }
    return false;
  }

  FlexGCWorker gcWorker(getTech(), logger_);
  gcWorker.setIgnoreMinArea();
  gcWorker.setIgnoreLongSideEOL();
  gcWorker.setIgnoreCornerSpacing();

  frCoord llx = std::numeric_limits<frCoord>::max();
  frCoord lly = std::numeric_limits<frCoord>::max();
  frCoord urx = std::numeric_limits<frCoord>::min();
  frCoord ury = std::numeric_limits<frCoord>::min();
  for (auto& [connFig, owner] : objs) {
    Rect bbox = connFig->getBBox();
    llx = std::min(llx, bbox.xMin());
    lly = std::min(llx, bbox.yMin());
    urx = std::max(llx, bbox.xMax());
    ury = std::max(llx, bbox.yMax());
  }
  Rect extBox(llx - 3000, lly - 3000, urx + 3000, ury + 3000);
  // Rect extBox(llx - 1000, lly - 1000, urx + 1000, ury + 1000);
  gcWorker.setExtBox(extBox);
  gcWorker.setDrcBox(extBox);

  gcWorker.setTargetObjs(targetObjs);
  if (targetObjs.empty())
    gcWorker.setIgnoreDB();
  // cout <<flush;
  gcWorker.initPA0(getDesign());
  for (auto& [connFig, owner] : objs) {
    gcWorker.addPAObj(connFig, owner);
  }
  gcWorker.initPA1();
  gcWorker.main();
  gcWorker.end();

  bool sol = false;
  if (gcWorker.getMarkers().empty()) {
    sol = true;
  }
  if (owners) {
    for (auto& marker : gcWorker.getMarkers()) {
      for (auto& src : marker->getSrcs()) {
        owners->insert(src);
      }
    }
  }
  if (graphics_) {
    graphics_->setObjsAndMakers(objs, gcWorker.getMarkers(), patternType);
  }
  return sol;
}

void FlexPA::genPatterns_perform(
    std::vector<FlexDPNode>& nodes,
    const std::vector<std::pair<frMPin*, frInstTerm*>>& pins,
    std::vector<int>& vioEdges,
    const std::set<std::pair<int, int>>& usedAccessPoints,
    const std::set<std::pair<int, int>>& violAccessPoints,
    int currUniqueInstIdx,
    int maxAccessPointSize)
{
  for (int currIdx1 = 0; currIdx1 <= (int) pins.size(); currIdx1++) {
    for (int currIdx2 = 0; currIdx2 < maxAccessPointSize; currIdx2++) {
      auto currNodeIdx = getFlatIdx(currIdx1, currIdx2, maxAccessPointSize);
      auto& currNode = nodes[currNodeIdx];
      if (currNode.getNodeCost() == std::numeric_limits<int>::max()) {
        continue;
      }
      int prevIdx1 = currIdx1 - 1;
      for (int prevIdx2 = 0; prevIdx2 < maxAccessPointSize; prevIdx2++) {
        int prevNodeIdx = getFlatIdx(prevIdx1, prevIdx2, maxAccessPointSize);
        auto& prevNode = nodes[prevNodeIdx];
        if (prevNode.getPathCost() == std::numeric_limits<int>::max()) {
          continue;
        }

        int edgeCost = getEdgeCost(prevNodeIdx,
                                   currNodeIdx,
                                   nodes,
                                   pins,
                                   vioEdges,
                                   usedAccessPoints,
                                   violAccessPoints,
                                   currUniqueInstIdx,
                                   maxAccessPointSize);
        if (currNode.getPathCost() == std::numeric_limits<int>::max()
            || currNode.getPathCost() > prevNode.getPathCost() + edgeCost) {
          currNode.setPathCost(prevNode.getPathCost() + edgeCost);
          currNode.setPrevNodeIdx(prevNodeIdx);
        }
      }
    }
  }
}

int FlexPA::getEdgeCost(
    int prevNodeIdx,
    int currNodeIdx,
    const std::vector<FlexDPNode>& nodes,
    const std::vector<std::pair<frMPin*, frInstTerm*>>& pins,
    std::vector<int>& vioEdges,
    const std::set<std::pair<int, int>>& usedAccessPoints,
    const std::set<std::pair<int, int>>& violAccessPoints,
    int currUniqueInstIdx,
    int maxAccessPointSize)
{
  int edgeCost = 0;
  int prevIdx1, prevIdx2, currIdx1, currIdx2;
  getNestedIdx(prevNodeIdx, prevIdx1, prevIdx2, maxAccessPointSize);
  getNestedIdx(currNodeIdx, currIdx1, currIdx2, maxAccessPointSize);
  if (prevIdx1 == -1 || currIdx1 == (int) pins.size()) {
    return edgeCost;
  }

  bool hasVio = false;
  // check if the edge has been calculated
  int edgeIdx
      = getFlatEdgeIdx(prevIdx1, prevIdx2, currIdx2, maxAccessPointSize);
  if (vioEdges[edgeIdx] != -1) {
    hasVio = (vioEdges[edgeIdx] == 1);
  } else {
    auto& currUniqueInst = uniqueInstances_[currUniqueInstIdx];
    dbTransform xform = currUniqueInst->getUpdatedXform(true);
    // check DRC
    vector<pair<frConnFig*, frBlockObject*>> objs;
    auto& [pin1, instTerm1] = pins[prevIdx1];
    auto targetObj = instTerm1->getInst();
    int paIdx = unique2paidx_[targetObj];
    auto pa1 = pin1->getPinAccess(paIdx);
    unique_ptr<frVia> via1;
    if (pa1->getAccessPoint(prevIdx2)->hasAccess(frDirEnum::U)) {
      via1 = make_unique<frVia>(pa1->getAccessPoint(prevIdx2)->getViaDef());
      Point pt1(pa1->getAccessPoint(prevIdx2)->getPoint());
      xform.apply(pt1);
      via1->setOrigin(pt1);
      if (instTerm1->hasNet()) {
        objs.push_back(make_pair(via1.get(), instTerm1->getNet()));
      } else {
        objs.push_back(make_pair(via1.get(), instTerm1));
      }
    }

    auto& [pin2, instTerm2] = pins[currIdx1];
    auto pa2 = pin2->getPinAccess(paIdx);
    unique_ptr<frVia> via2;
    if (pa2->getAccessPoint(currIdx2)->hasAccess(frDirEnum::U)) {
      via2 = make_unique<frVia>(pa2->getAccessPoint(currIdx2)->getViaDef());
      Point pt2(pa2->getAccessPoint(currIdx2)->getPoint());
      xform.apply(pt2);
      via2->setOrigin(pt2);
      if (instTerm2->hasNet()) {
        objs.push_back(make_pair(via2.get(), instTerm2->getNet()));
      } else {
        objs.push_back(make_pair(via2.get(), instTerm2));
      }
    }

    hasVio = !genPatterns_gc({targetObj}, objs, Edge);
    vioEdges[edgeIdx] = hasVio;

    // look back for GN14
    if (!hasVio && prevNodeIdx != -1) {
      // check one more back
      auto prevPrevNodeIdx = nodes[prevNodeIdx].getPrevNodeIdx();
      if (prevPrevNodeIdx != -1) {
        int prevPrevIdx1, prevPrevIdx2;
        getNestedIdx(
            prevPrevNodeIdx, prevPrevIdx1, prevPrevIdx2, maxAccessPointSize);
        if (prevPrevIdx1 != -1) {
          auto& [pin3, instTerm3] = pins[prevPrevIdx1];
          auto pa3 = pin3->getPinAccess(paIdx);
          unique_ptr<frVia> via3;
          if (pa3->getAccessPoint(prevPrevIdx2)->hasAccess(frDirEnum::U)) {
            via3 = make_unique<frVia>(
                pa3->getAccessPoint(prevPrevIdx2)->getViaDef());
            Point pt3(pa3->getAccessPoint(prevPrevIdx2)->getPoint());
            xform.apply(pt3);
            via3->setOrigin(pt3);
            if (instTerm3->hasNet()) {
              objs.push_back(make_pair(via3.get(), instTerm3->getNet()));
            } else {
              objs.push_back(make_pair(via3.get(), instTerm3));
            }
          }

          hasVio = !genPatterns_gc({targetObj}, objs, Edge);
        }
      }
    }
  }

  if (!hasVio) {
    if ((prevIdx1 == 0
         && usedAccessPoints.find(std::make_pair(prevIdx1, prevIdx2))
                != usedAccessPoints.end())
        || (currIdx1 == (int) pins.size() - 1
            && usedAccessPoints.find(std::make_pair(currIdx1, currIdx2))
                   != usedAccessPoints.end())) {
      edgeCost = 100;
    } else if (violAccessPoints.find(make_pair(prevIdx1, prevIdx2))
                   != violAccessPoints.end()
               || violAccessPoints.find(make_pair(currIdx1, currIdx2))
                      != violAccessPoints.end()) {
      edgeCost = 1000;
    } else {
      int prevNodeCost = nodes[prevNodeIdx].getNodeCost();
      int currNodeCost = nodes[currNodeIdx].getNodeCost();
      edgeCost = (prevNodeCost + currNodeCost) / 2;
    }
  } else {
    edgeCost = 1000 /*violation cost*/;
  }

  return edgeCost;
}

bool FlexPA::genPatterns_commit(
    std::vector<FlexDPNode>& nodes,
    const std::vector<std::pair<frMPin*, frInstTerm*>>& pins,
    bool& isValid,
    std::set<std::vector<int>>& instAccessPatterns,
    std::set<std::pair<int, int>>& usedAccessPoints,
    std::set<std::pair<int, int>>& violAccessPoints,
    int currUniqueInstIdx,
    int maxAccessPointSize)
{
  bool hasNewPattern = false;
  int currNodeIdx = getFlatIdx(pins.size(), 0, maxAccessPointSize);
  auto currNode = &(nodes[currNodeIdx]);
  int pinCnt = pins.size();
  std::vector<int> accessPattern(pinCnt, -1);
  while (currNode->getPrevNodeIdx() != -1) {
    // non-virtual node
    if (pinCnt != (int) pins.size()) {
      int currIdx1, currIdx2;
      getNestedIdx(currNodeIdx, currIdx1, currIdx2, maxAccessPointSize);
      accessPattern[currIdx1] = currIdx2;
      usedAccessPoints.insert(std::make_pair(currIdx1, currIdx2));
    }

    currNodeIdx = currNode->getPrevNodeIdx();
    currNode = &(nodes[currNode->getPrevNodeIdx()]);
    pinCnt--;
  }

  if (pinCnt != -1) {
    logger_->error(DRT, 90, "Valid access pattern not found.");
  }

  // add to pattern set if unique
  if (instAccessPatterns.find(accessPattern) == instAccessPatterns.end()) {
    instAccessPatterns.insert(accessPattern);
    // create new access pattern and push to uniqueInstances
    auto pinAccessPattern = std::make_unique<FlexPinAccessPattern>();
    std::map<frMPin*, frAccessPoint*> pin2AP;
    // check DRC for the whole pattern
    vector<pair<frConnFig*, frBlockObject*>> objs;
    vector<unique_ptr<frVia>> tempVias;
    frInst* targetObj = nullptr;
    for (int idx1 = 0; idx1 < (int) pins.size(); idx1++) {
      auto idx2 = accessPattern[idx1];
      auto& [pin, instTerm] = pins[idx1];
      auto inst = instTerm->getInst();
      targetObj = inst;
      int paIdx = unique2paidx_[inst];
      auto pa = pin->getPinAccess(paIdx);
      auto accessPoint = pa->getAccessPoint(idx2);
      pin2AP[pin] = accessPoint;

      // add objs
      unique_ptr<frVia> via;
      if (accessPoint->hasAccess(frDirEnum::U)) {
        via = make_unique<frVia>(accessPoint->getViaDef());
        auto rvia = via.get();
        tempVias.push_back(std::move(via));

        dbTransform xform = inst->getUpdatedXform(true);
        Point pt(accessPoint->getPoint());
        xform.apply(pt);
        rvia->setOrigin(pt);
        if (instTerm->hasNet()) {
          objs.push_back(make_pair(rvia, instTerm->getNet()));
        } else {
          objs.push_back(make_pair(rvia, instTerm));
        }
      }
    }

    frAccessPoint* leftAP = nullptr;
    frAccessPoint* rightAP = nullptr;
    frCoord leftPt = std::numeric_limits<frCoord>::max();
    frCoord rightPt = std::numeric_limits<frCoord>::min();

    auto& [pin, instTerm] = pins[0];
    auto inst = instTerm->getInst();
    for (auto& instTerm : inst->getInstTerms()) {
      if (isSkipInstTerm(instTerm.get())) {
        continue;
      }
      long unsigned int nNoApPins = 0;
      for (auto& pin : instTerm->getTerm()->getPins()) {
        if (pin2AP.find(pin.get()) == pin2AP.end()) {
          nNoApPins++;
          pinAccessPattern->addAccessPoint(nullptr);
        } else {
          auto& ap = pin2AP[pin.get()];
          Point tmpPt = ap->getPoint();
          if (tmpPt.x() < leftPt) {
            leftAP = ap;
            leftPt = tmpPt.x();
          }
          if (tmpPt.x() > rightPt) {
            rightAP = ap;
            rightPt = tmpPt.x();
          }
          pinAccessPattern->addAccessPoint(ap);
        }
      }
      if (nNoApPins == instTerm->getTerm()->getPins().size())
        logger_->error(DRT, 91, "Pin does not have valid ap.");
    }
    // if (leftAP == nullptr) {
    //   cout <<"@@@Warning leftAP == nullptr " <<instTerm->getInst()->getName()
    //   <<endl;
    // }
    // if (rightAP == nullptr) {
    //   cout <<"@@@Warning rightAP == nullptr "
    //   <<instTerm->getInst()->getName() <<endl;
    // }
    pinAccessPattern->setBoundaryAP(true, leftAP);
    pinAccessPattern->setBoundaryAP(false, rightAP);

    set<frBlockObject*> owners;
    if (targetObj != nullptr
        && genPatterns_gc({targetObj}, objs, Commit, &owners)) {
      pinAccessPattern->updateCost();
      // cout <<"commit ap:";
      // for (auto &ap: pinAccessPattern->getPattern()) {
      //   cout <<" " <<ap;
      // }
      // cout <<endl <<"l/r = "
      //      <<pinAccessPattern->getBoundaryAP(true) <<"/"
      //      <<pinAccessPattern->getBoundaryAP(false) <<", "
      //      <<currUniqueInstIdx <<", "
      //      <<uniqueInstances[currUniqueInstIdx]->getName() <<endl;
      uniqueInstPatterns_[currUniqueInstIdx].push_back(
          std::move(pinAccessPattern));
      // genPatterns_print(nodes, pins, maxAccessPointSize);
      isValid = true;
    } else {
      for (int idx1 = 0; idx1 < (int) pins.size(); idx1++) {
        auto idx2 = accessPattern[idx1];
        auto& [pin, instTerm] = pins[idx1];
        if (instTerm->hasNet()) {
          if (owners.find(instTerm->getNet()) != owners.end()) {
            violAccessPoints.insert(make_pair(idx1, idx2));  // idx ;
          }
        } else {
          if (owners.find(instTerm) != owners.end()) {
            violAccessPoints.insert(make_pair(idx1, idx2));  // idx ;
          }
        }
      }
    }

    hasNewPattern = true;
  } else {
    hasNewPattern = false;
  }

  return hasNewPattern;
}

void FlexPA::genPatterns_print_debug(
    std::vector<FlexDPNode>& nodes,
    const std::vector<std::pair<frMPin*, frInstTerm*>>& pins,
    int maxAccessPointSize)
{
  int currNodeIdx = getFlatIdx(pins.size(), 0, maxAccessPointSize);
  auto currNode = &(nodes[currNodeIdx]);
  int pinCnt = pins.size();

  dbTransform xform;
  auto& [pin, instTerm] = pins[0];
  if (instTerm) {
    frInst* inst = instTerm->getInst();
    xform = inst->getTransform();
    xform.setOrient(dbOrientType::R0);
  }

  cout << "failed pattern:";

  double dbu = getDesign()->getTopBlock()->getDBUPerUU();
  while (currNode->getPrevNodeIdx() != -1) {
    // non-virtual node
    if (pinCnt != (int) pins.size()) {
      auto& [pin, instTerm] = pins[pinCnt];
      auto inst = instTerm->getInst();
      cout << " " << instTerm->getTerm()->getName();
      int paIdx = unique2paidx_[inst];
      auto pa = pin->getPinAccess(paIdx);
      int currIdx1, currIdx2;
      getNestedIdx(currNodeIdx, currIdx1, currIdx2, maxAccessPointSize);
      Point pt(pa->getAccessPoint(currIdx2)->getPoint());
      xform.apply(pt);
      cout << " (" << pt.x() / dbu << ", " << pt.y() / dbu << ")";
    }

    currNodeIdx = currNode->getPrevNodeIdx();
    currNode = &(nodes[currNode->getPrevNodeIdx()]);
    pinCnt--;
  }
  cout << endl;
  if (pinCnt != -1) {
    logger_->error(DRT, 277, "Valid access pattern not found.");
  }
}

void FlexPA::genPatterns_print(
    std::vector<FlexDPNode>& nodes,
    const std::vector<std::pair<frMPin*, frInstTerm*>>& pins,
    int maxAccessPointSize)
{
  int currNodeIdx = getFlatIdx(pins.size(), 0, maxAccessPointSize);
  auto currNode = &(nodes[currNodeIdx]);
  int pinCnt = pins.size();

  cout << "new pattern\n";

  while (currNode->getPrevNodeIdx() != -1) {
    // non-virtual node
    if (pinCnt != (int) pins.size()) {
      auto& [pin, instTerm] = pins[pinCnt];
      auto inst = instTerm->getInst();
      int paIdx = unique2paidx_[inst];
      auto pa = pin->getPinAccess(paIdx);
      int currIdx1, currIdx2;
      getNestedIdx(currNodeIdx, currIdx1, currIdx2, maxAccessPointSize);
      unique_ptr<frVia> via
          = make_unique<frVia>(pa->getAccessPoint(currIdx2)->getViaDef());
      Point pt(pa->getAccessPoint(currIdx2)->getPoint());
      cout << " gccleanvia " << inst->getMaster()->getName() << " "
           << instTerm->getTerm()->getName() << " "
           << via->getViaDef()->getName() << " " << pt.x() << " " << pt.y()
           << " " << inst->getOrient().getString() << "\n";
    }

    currNodeIdx = currNode->getPrevNodeIdx();
    currNode = &(nodes[currNode->getPrevNodeIdx()]);
    pinCnt--;
  }
  if (pinCnt != -1) {
    logger_->error(DRT, 278, "Valid access pattern not found.");
  }
}

// get flat index
// idx1 is outer index and idx2 is inner index dpNodes[idx1][idx2]
int FlexPA::getFlatIdx(int idx1, int idx2, int idx2Dim)
{
  return ((idx1 + 1) * idx2Dim + idx2);
}

// get idx1 and idx2 from flat index
void FlexPA::getNestedIdx(int flatIdx, int& idx1, int& idx2, int idx2Dim)
{
  idx1 = flatIdx / idx2Dim - 1;
  idx2 = flatIdx % idx2Dim;
}

// get flat edge index
int FlexPA::getFlatEdgeIdx(int prevIdx1,
                           int prevIdx2,
                           int currIdx2,
                           int idx2Dim)
{
  return (((prevIdx1 + 1) * idx2Dim + prevIdx2) * idx2Dim + currIdx2);
}
