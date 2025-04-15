// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "gr/FlexGRGridGraph.h"

#include <iostream>
#include <map>

#include "gr/FlexGR.h"

namespace drt {

void FlexGRGridGraph::init()
{
  auto gCellPatterns = getDesign()->getTopBlock()->getGCellPatterns();
  xgp_ = &(gCellPatterns.at(0));
  ygp_ = &(gCellPatterns.at(1));

  initCoords();
  initGrids();
  initEdges();
}

void FlexGRGridGraph::initCoords()
{
  std::map<frLayerNum, dbTechLayerDir> zMap;
  Point gcellIdxLL = getGRWorker()->getRouteGCellIdxLL();
  Point gcellIdxUR = getGRWorker()->getRouteGCellIdxUR();
  // xCoords
  for (int xIdx = gcellIdxLL.x(); xIdx <= gcellIdxUR.x(); xIdx++) {
    Rect gcellBox = getDesign()->getTopBlock()->getGCellBox(Point(xIdx, 0));
    xCoords_.push_back((gcellBox.xMin() + gcellBox.xMax()) / 2);
  }
  // yCoords
  for (int yIdx = gcellIdxLL.y(); yIdx <= gcellIdxUR.y(); yIdx++) {
    Rect gcellBox = getDesign()->getTopBlock()->getGCellBox(Point(0, yIdx));
    yCoords_.push_back((gcellBox.yMin() + gcellBox.yMax()) / 2);
  }
  // z
  if (!is2DRouting_) {
    for (auto& layer : getTech()->getLayers()) {
      if (layer->getType() != dbTechLayerType::ROUTING) {
        continue;
      }
      frLayerNum lNum = layer->getLayerNum();
      dbTechLayerDir prefRouteDir = layer->getDir();
      zMap[lNum] = prefRouteDir;
    }
  } else {
    // 2D routing only has one layer on layerNum == 2
    zMap[2] = dbTechLayerDir::NONE;
  }

  frCoord zHeight = 0;
  for (auto& [k, v] : zMap) {
    zCoords_.push_back(k);
    zHeight += getTech()->getLayer(k)->getPitch() * router_cfg_->VIACOST;
    zHeights_.push_back(zHeight);
    zDirs_.push_back((v == dbTechLayerDir::HORIZONTAL));
  }
}

void FlexGRGridGraph::initGrids()
{
  frMIdx xDim, yDim, zDim;
  getDim(xDim, yDim, zDim);
  bits_.clear();
  bits_.resize(xDim * yDim * zDim, 0);
  prevDirs_.clear();
  srcs_.clear();
  dsts_.clear();
  prevDirs_.resize(xDim * yDim * zDim * 3, false);
  srcs_.resize(xDim * yDim * zDim, false);
  dsts_.resize(xDim * yDim * zDim, false);
}

void FlexGRGridGraph::initEdges()
{
  for (frMIdx zIdx = 0; zIdx < (int) zCoords_.size(); zIdx++) {
    dbTechLayerDir dir = dbTechLayerDir::NONE;
    if (!is2DRouting_) {
      dir = zDirs_[zIdx] ? dbTechLayerDir::HORIZONTAL
                         : dbTechLayerDir::VERTICAL;
    }
    for (frMIdx xIdx = 0; xIdx < (int) xCoords_.size(); xIdx++) {
      for (frMIdx yIdx = 0; yIdx < (int) yCoords_.size(); yIdx++) {
        // horz
        if ((dir == dbTechLayerDir::NONE || dir == dbTechLayerDir::HORIZONTAL)
            && ((xIdx + 1) != (int) xCoords_.size())) {
          addEdge(xIdx, yIdx, zIdx, frDirEnum::E);
        }
        // vert
        if ((dir == dbTechLayerDir::NONE || dir == dbTechLayerDir::VERTICAL)
            && ((yIdx + 1) != (int) yCoords_.size())) {
          addEdge(xIdx, yIdx, zIdx, frDirEnum::N);
        }
        // via
        if ((zIdx + 1) != (int) zCoords_.size()) {
          addEdge(xIdx, yIdx, zIdx, frDirEnum::U);
        }
      }
    }
  }
}

void FlexGRGridGraph::resetStatus()
{
  resetSrc();
  resetDst();
  resetPrevNodeDir();
}

void FlexGRGridGraph::resetSrc()
{
  srcs_.assign(srcs_.size(), false);
}

void FlexGRGridGraph::resetDst()
{
  dsts_.assign(dsts_.size(), false);
}

void FlexGRGridGraph::resetPrevNodeDir()
{
  prevDirs_.assign(prevDirs_.size(), false);
}

}  // namespace drt
