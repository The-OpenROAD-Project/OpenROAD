/* Authors: Lutong Wang and Bangqi Xu */
/* Updated version:  Zhiang Wang*/
/*
 * Copyright (c) 2025, The Regents of the University of California
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

#include <vector>

#include "gr/FlexGR.h"

namespace drt {

void FlexGRWorker::main_mt_init(std::vector<grNet*>& rerouteNets) {
  auto LLCorner = getRouteGCellIdxLL();
  for (auto net : rerouteNets) {
    mazeNetInit(net);
    net->updateAbsGridCoords(LLCorner);
  }
}


// reset gridGraph, remove routeObj and remove nodes
void FlexGRWorker::mazeNetInit(grNet* net)
{
  gridGraph_.resetStatus();
  if (ripupMode_ == RipUpMode::DRC) {
    mazeNetInit_decayHistCost(net);
  } else if (ripupMode_ == RipUpMode::ALL) {
    mazeNetInit_addHistCost(net);
  }
  mazeNetInit_removeNetObjs(net);
  mazeNetInit_removeNetNodes(net);
}


void FlexGRWorker::mazeNetInit_getRoutingTraces(
  grNet* net,
  std::set<FlexMazeIdx>& pts)
{
  for (auto& uptr : net->getRouteConnFigs()) {
    if (uptr->typeId() != grcPathSeg) {
      continue;
    }
    
    auto cptr = static_cast<grPathSeg*>(uptr.get());
    auto [bp, ep] = cptr->getPoints();
    const frLayerNum lNum = cptr->getLayerNum();
    FlexMazeIdx bi, ei;
    gridGraph_.getMazeIdx(bp, lNum, bi);
    gridGraph_.getMazeIdx(ep, lNum, ei);
    const int xStart = bi.x();
    const int yStart = bi.y();
    const int zIdx = bi.z();
    const int xEnd = ei.x();
    const int yEnd = ei.y();
    for (auto xIdx = xStart; xIdx <= xEnd; xIdx++) {
      for (auto yIdx = yStart; yIdx <= yEnd; yIdx++) {
        pts.insert(FlexMazeIdx(xIdx, yIdx, zIdx));
      }
    }
  }
}


void FlexGRWorker::mazeNetInit_addHistCost(grNet* net)
{
  // add history cost for all gcells that this net previously visited
  std::set<FlexMazeIdx> pts;
  mazeNetInit_getRoutingTraces(net, pts);
  for (const auto& pt : pts) {
    if (isGCellOverflow(pt.x(), pt.y(), pt.z())) {
      gridGraph_.addHistoryCost(pt.x(), pt.y(), pt.z(), 1);
    }
  }
}

void FlexGRWorker::mazeNetInit_decayHistCost(grNet* net)
{
  // decay history cost for all gcells that this net previously visited
  std::set<FlexMazeIdx> pts;
  mazeNetInit_getRoutingTraces(net, pts);
  for (const auto& pt : pts) {
    gridGraph_.decayHistoryCost(pt.x(), pt.y(), pt.z());
  }
}


// with vector, we can only erase all route objs
// if we need partial ripup, need to change routeConnfigs to list
void FlexGRWorker::mazeNetInit_removeNetObjs(grNet* net)
{
  auto& workerRegionQuery = getWorkerRegionQuery();
  // remove everything from region query
  for (auto& uptr : net->getRouteConnFigs()) {
    workerRegionQuery.remove(uptr.get());
  }
  
  // update congestion for the to-be-removed objs
  for (auto& uptr : net->getRouteConnFigs()) {
    if (uptr->typeId() == grcPathSeg) {
      auto cptr = static_cast<grPathSeg*>(uptr.get());
      modCong_pathSeg(cptr, false); // isAdd = false means remove congestion
    }
  }

  // remove the connFigs themselves
  net->clearRouteConnFigs();
}


void FlexGRWorker::modCong_pathSeg(grPathSeg* pathSeg, bool isAdd)
{
  FlexMazeIdx bi, ei;
  const frLayerNum lNum = pathSeg->getLayerNum();
  auto [bp, ep] = pathSeg->getPoints();
  gridGraph_.getMazeIdx(bp, lNum, bi);
  gridGraph_.getMazeIdx(ep, lNum, ei);

  // Only support colinear path segments (vertical or horizontal)
  if (bi.x() != ei.x() && bi.y() != ei.y()) {
    logger_->error(DRT, 377, "Error: non-colinear pathSeg in modCong_pathSeg!");
    return;
  }

  // Select axis, direction, and loop range
  const bool isVertical = (bi.x() == ei.x());
  const int from = isVertical ? bi.y() : bi.x();
  const int to   = isVertical ? ei.y() : ei.x();
  const int fixedIdx = isVertical ? bi.x() : bi.y();
  const int zIdx = bi.z();
  const frDirEnum dir = isVertical ? frDirEnum::N : frDirEnum::E;
  for (int idx = from; idx < to; ++idx) {
    if (isAdd) {
      if (isVertical) {
        gridGraph_.addRawDemand(fixedIdx, idx, zIdx, dir);
        gridGraph_.addRawDemand(fixedIdx, idx + 1, zIdx, dir);
      } else {
        gridGraph_.addRawDemand(idx, fixedIdx, zIdx, dir);
        gridGraph_.addRawDemand(idx + 1, fixedIdx, zIdx, dir);
      }
    } else {
      if (isVertical) {
        gridGraph_.subRawDemand(fixedIdx, idx, zIdx, dir);
        gridGraph_.subRawDemand(fixedIdx, idx + 1, zIdx, dir);
      } else {
        gridGraph_.subRawDemand(idx, fixedIdx, zIdx, dir);
        gridGraph_.subRawDemand(idx + 1, fixedIdx, zIdx, dir);
      }
    }
  }
}


//
void FlexGRWorker::mazeNetInit_removeNetNodes(grNet* net)
{
  // remove non-terminal gcell nodes (isDontMove is true)  
  std::set<grNode*> terminalNodes;
  grNode* root = nullptr;
  for (auto& [pinNode, gcellNode] : net->getPinGCellNodePairs()) {
    if (root == nullptr) {  root = gcellNode; }
    terminalNodes.insert(gcellNode);
  }

  if (root == nullptr) {
    logger_->error(utl::DRT, 378, "Error: root is nullptr in mazeNetInit_removeNetNodes!");
    return;
  }

  // For test only
  for (auto& node : terminalNodes) {
    if (node->isDontMove() == false) {
      logger_->error(utl::DRT, 376, "Error: terminal node isDontMove is false in mazeNetInit_removeNetNodes!");
    }
  }

  std::queue<grNode*> nodeQ;
  nodeQ.push(root);
  // bfs to remove non-terminal nodes
  // only steiner node can exist in queue
  bool isRoot = true; // This is important for restore the chip-level routing tree
  while (!nodeQ.empty()) {
    auto node = nodeQ.front();
    nodeQ.pop();
    // push steiner child
    for (auto child : node->getChildren()) {
      if (child->getType() == frNodeTypeEnum::frcSteiner) {
        nodeQ.push(child);
      }
    }
    // update connection
    // for root terminal node, clear all steiner children;
    // for non-root terminal nodes, clear all steiner children and reset parent
    if (node->isDontMove() == true) { // terminal nodes
      std::vector<grNode*> steinerChildNodes;
      for (auto child : node->getChildren()) {
        if (child->getType() == frNodeTypeEnum::frcSteiner) {
          steinerChildNodes.push_back(child);
        }
      }
      for (auto steinerChildNode : steinerChildNodes) {
        node->removeChild(steinerChildNode);
      }
      if (!isRoot) {  node->setParent(nullptr); }
    } else {
      net->removeNode(node);
    }
    isRoot = false;
  }
}



}  // namespace drt
