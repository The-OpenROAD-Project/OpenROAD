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




#include "gr/FlexGR.h"

#include <vector>
#include <omp.h>
#include <spdlog/common.h>
#include <sys/types.h>
#include <climits>
#include <cmath>
#include <fstream>
#include <iostream>
#include <iterator>
#include <mutex>
#include <queue>

namespace drt {


void FlexGRWorker::main_mt_prep(std::vector<grNet*>& rerouteNets, int iter) {
  // set local grid graph dimensions
  gridGraph_.getDim(xDim_, yDim_, zDim_);
  // For incremental RRR (DRC mode), we need to update the history at the beginning of each iteration
  route_addHistCost_update();
  routePrep_update(rerouteNets, iter);
}


// Setup the history cost for the nets 
void FlexGRWorker::route_addHistCost_update()
{
  if (ripupMode_ == RipUpMode::DRC) {
    route_addHistCost();
  }
}


void FlexGRWorker::route_addHistCost()
{
  frMIdx xDim = 0;
  frMIdx yDim = 0;
  frMIdx zDim = 0;
  gridGraph_.getDim(xDim, yDim, zDim); 
  for (int zIdx = 0; zIdx < zDim; zIdx++) {
    for (int xIdx = 0; xIdx < xDim; xIdx++) {
      for (int yIdx = 0; yIdx < yDim; yIdx++) {
        const int rawDemandE = gridGraph_.getRawDemand(xIdx, yIdx, zIdx, frDirEnum::E);
        const int rawDemandN = gridGraph_.getRawDemand(xIdx, yIdx, zIdx, frDirEnum::N);
        const int rawSupplyE = gridGraph_.getRawSupply(xIdx, yIdx, zIdx, frDirEnum::E) * congThresh_;
        const int rawSupplyN = gridGraph_.getRawSupply(xIdx, yIdx, zIdx, frDirEnum::N) * congThresh_;
        if (rawDemandE > rawSupplyE || rawDemandN > rawSupplyN) {
          // has congestion, need to add history cost
          int overflowH = std::max(0, rawDemandE - rawSupplyE);
          int overflowV = std::max(0, rawDemandN - rawSupplyN);
          gridGraph_.addHistoryCost(
              xIdx, yIdx, zIdx, (overflowH + overflowV) / 2 + 1);
        }
      }
    }
  }
}


// Original code has a bug: we should not reset the ripup flag here
// Because we need to keep the ripup flag for the nets with loops should be rerouted
void FlexGRWorker::routePrep_update(std::vector<grNet*> &rerouteNets, 
  int iter)
{
  if (iter == 0) {
    init_pinGCellIdxs(); // Identify the locations for pinGCell nodes (fixed nodes during RRR)
  }  
  
  if (ripupMode_ == RipUpMode::DRC) {
    route_mazeIterInit();
  }

  // Get all the candidates nets to be rerouted
  route_getRerouteNets(rerouteNets); 
}


// Identify the fixed pinGCell nodes for the nets (expressed as the local gcell indices)
void FlexGRWorker::init_pinGCellIdxs()
{
  const auto LL = getRouteGCellIdxLL();
  const auto UR = getRouteGCellIdxUR();
  const int xDim = UR.x() - LL.x() + 1;
  const int yDim = UR.y() - LL.y() + 1;  
  const int workerId = getWorkerId();
  for (auto &net : nets_) {
    net->clearPinGCellIdxs();
    for (auto pinGCellNode : net->getPinGCellNodes()) {
      const auto loc = pinGCellNode->getLoc();
      const auto lNum = pinGCellNode->getLayerNum();
      FlexMazeIdx mi;
      gridGraph_.getMazeIdx(loc, lNum, mi);
      net->addPinGCellIdx(mi);
    }

    // Check if the net is a feedthrough net
    const std::vector<FlexMazeIdx>& pinGCellIdxs = net->getPinGCellIdxs();
    if (pinGCellIdxs.size() == 2 && (pinGCellIdxs[0].x() == pinGCellIdxs[1].x() || pinGCellIdxs[0].y() == pinGCellIdxs[1].y())) {
      net->setFeedThrough(true);
    } else {
      net->setFeedThrough(false);
    }

    // Set the bounding box
    FlexMazeIdx ll(std::numeric_limits<frMIdx>::max(), std::numeric_limits<frMIdx>::max(), std::numeric_limits<frMIdx>::max());
    FlexMazeIdx ur(std::numeric_limits<frMIdx>::min(), std::numeric_limits<frMIdx>::min(), std::numeric_limits<frMIdx>::min());
    for (auto &idx : pinGCellIdxs) {
      ll.set(std::min(ll.x(), idx.x()), std::min(ll.y(), idx.y()), std::min(ll.z(), idx.z()));
      ur.set(std::max(ur.x(), idx.x()), std::max(ur.y(), idx.y()), std::max(ur.z(), idx.z()));
    }
      
    // Bounding box relaxation
    // In this future, we hope to use a dynamic bounding box based on the routing congestion
    // Currently just use a fixed delta
    // Currently, we assume the RRR can use all the layers
    const int fixedBBDelta = 10;
    const int deltaX = std::min(ur.x() - ll.x(), fixedBBDelta);
    const int deltaY = std::min(ur.y() - ll.y(), fixedBBDelta);
    const int delta = std::max(deltaX, deltaY);
    ll.set(std::max(0, ll.x() - delta), std::max(0, ll.y() - delta), ll.z());
    ur.set(std::min(xDim - 1, ur.x() + delta), std::min(yDim - 1, ur.y() + delta), ur.z());     
    net->setRouteBBox(Rect(ll.x(), ll.y(), ur.x(), ur.y())); 
    // Set the routed wirelength as HPWL: we will use this to estimate the routed wirelength
    int hpwl = (ur.x() - ll.x()) + (ur.y() - ll.y());
    net->setHPWL(hpwl);
    net->setWorkerId(workerId);
    // net->setCPUFlag(true); // This is only for testing purpose
  }
}


// The route_mazeIterInit function is only used in the DRC mode
// At this moment, the DRC mode is the same as the ALL mode
// In the future, we want to implement the routing tree as the a graph structure
// So we have only remove the segments that passing through the congested gcell
// instead of removing the whole routing tree
void FlexGRWorker::route_mazeIterInit()
{
  // set ripup based on congestion
  auto& workerRegionQuery = getWorkerRegionQuery();

  frMIdx xDim = 0, yDim = 0, zDim = 0;
  gridGraph_.getDim(xDim, yDim, zDim);

  std::vector<grConnFig*> result;
  for (int zIdx = 0; zIdx < zDim; zIdx++) {
    const frLayerNum lNum = (zIdx + 1) * 2;
    for (int xIdx = 0; xIdx < xDim; xIdx++) {
      for (int yIdx = 0; yIdx < yDim; yIdx++) {
        if (!isGCellOverflow(xIdx, yIdx, zIdx)) {
          continue; // no congestion
        }
        
        Point gcellCenter;
        gridGraph_.getPoint(xIdx, yIdx, gcellCenter);
        Rect gcellCenterBox(gcellCenter, gcellCenter);
          
        result.clear();
        workerRegionQuery.query(gcellCenterBox, lNum, result);
        for (auto rptr : result) {
          if (rptr->typeId() != grcPathSeg) {
            continue; // Only check path segments
          }
          
          auto cptr = static_cast<grPathSeg*>(rptr);
          if (cptr->hasGrNet()) {
            cptr->getGrNet()->setRipup(true);
          } else {
            logger_->error(utl::DRT, 372, "Error: route_mazeIterInit hasGrNet() empty!");
          }
        } // end result loop
      } // end yIdx loop
    } // end xIdx loop
  } // end zIdx loop
}


// In this implementation, DRC mode mainly for removing loops
void FlexGRWorker::route_getRerouteNets(std::vector<grNet*>& rerouteNets)
{
  rerouteNets.clear(); 
  switch (ripupMode_) {
    case RipUpMode::DRC:
      for (auto& net : nets_) {
        if (net->isRipup() && !net->isTrivial()) {
          rerouteNets.push_back(net.get());
          net->setModified(true);
          net->getFrNet()->setModified(true);
        }
      }
      break;
    
    case RipUpMode::ALL:
      for (auto& net : nets_) {
        if (!net->isTrivial() && (net->isRipup() || mazeNetHasCong(net.get()))) {
          rerouteNets.push_back(net.get());
        }
      }
      break;
    
    default:
      logger_->error(utl::DRT, 374, "Unknown ripup mode !!!");
      break;
  }
}


bool FlexGRWorker::isGCellOverflow(int xIdx, int yIdx, int zIdx) const  
{
  const int rawDemandE = gridGraph_.getRawDemand(xIdx, yIdx, zIdx, frDirEnum::E);
  const int rawDemandN = gridGraph_.getRawDemand(xIdx, yIdx, zIdx, frDirEnum::N);
  const int rawSupplyE = gridGraph_.getRawSupply(xIdx, yIdx, zIdx, frDirEnum::E) * congThresh_;
  const int rawSupplyN = gridGraph_.getRawSupply(xIdx, yIdx, zIdx, frDirEnum::N) * congThresh_;
  if (rawDemandE > rawSupplyE || rawDemandN > rawSupplyN) {
    return true;
  }
  return false;
}


bool FlexGRWorker::mazeNetHasCong(grNet* net)
{
  // In this implementation, we express the via demand into segment demand
  // Similar to the ISPD 24/25 Global Routing Contest  
  // define the lambda for congestion gcell
  auto markIfCongestedGCell = [&](int xIdx, int yIdx, int zIdx) -> bool {
    if (isGCellOverflow(xIdx, yIdx, zIdx)) {
      net->setModified(true);
      net->getFrNet()->setModified(true);
      net->setRipup(true);
      return true;
    }
    return false;
  };
  
  for (auto& uptr : net->getRouteConnFigs()) {
    if (uptr->typeId() != grcPathSeg) {
      continue; // Only check path segments
    }
    
    auto cptr = static_cast<grPathSeg*>(uptr.get());
    auto [bp, ep] = cptr->getPoints();
    const  frLayerNum lNum = cptr->getLayerNum();
    
    FlexMazeIdx bi, ei;
    gridGraph_.getMazeIdx(bp, lNum, bi);
    gridGraph_.getMazeIdx(ep, lNum, ei);
    
    const int xStart = std::min(bi.x(), ei.x());
    const int xEnd = std::max(bi.x(), ei.x());
    const int yStart = std::min(bi.y(), ei.y());
    const int yEnd = std::max(bi.y(), ei.y());

    if (xStart == xEnd) { // vertical path segment
      for (int yIdx = yStart; yIdx <= yEnd; yIdx++) {
        if (markIfCongestedGCell(bi.x(), yIdx, bi.z()))  return true;        
      }
    } else if (yStart == yEnd) { // horizontal path segment
      for (int xIdx = xStart; xIdx <= xEnd; xIdx++) {
        if (markIfCongestedGCell(xIdx, bi.y(), bi.z())) return true;
      }
    } else {
      logger_->report("Warning: Non-colinear path segment in mazeNetHasCong");
    }
  }  
  
  return false;
}



void FlexGRWorker::grNet2NetTree(
  std::vector<grNet*>& sortedNets,
  std::vector<std::unique_ptr<NetStruct> >& netTrees)
{
  netTrees.clear();
  netTrees.resize(sortedNets.size());
  int numNets = static_cast<int>(sortedNets.size());
  
  // Use OpenMP to parallelize the net tree generation
  int numThreads = std::max(1, static_cast<int>(omp_get_max_threads()));
  #pragma omp parallel for num_threads(numThreads) schedule(dynamic)
  for (int netId = 0; netId < numNets; netId++) {
    auto& net = sortedNets[netId];
    std::unique_ptr<NetStruct> netTree = std::make_unique<NetStruct>();
    netTree->netId = netId;  
    auto& points = netTree->points;
    auto& vSegments = netTree->vSegments;
    auto& hSegments = netTree->hSegments;

    // Use queue to traverse the net's GCell nodes
    auto& pinGCellNodePairs = net->getPinGCellNodePairs();
    if (pinGCellNodePairs.empty()) {
      logger_->error(utl::DRT, 375, "Error: pinGCellNodePairs is empty in grNet2NetTree!");
    }
    grNode* root = pinGCellNodePairs.front().second;
    std::queue<grNode*> nodeQ;
    nodeQ.push(root);

    while (!nodeQ.empty()) {
      auto node = nodeQ.front();
      nodeQ.pop();
      // add the children nodes to the queue (we just need to consider Steiner nodes)
      for (auto& child : node->getChildren()) {
        if (child->getType() == frNodeTypeEnum::frcSteiner) {
          nodeQ.push(child);
        }
      }
   
      FlexMazeIdx nodeMIdx;
      gridGraph_.getMazeIdx(node->getLoc(), node->getLayerNum(), nodeMIdx);
      const int nodeLocIdx = getGCellIdx1D(nodeMIdx.x(), nodeMIdx.y(), nodeMIdx.z());
      points.push_back(nodeLocIdx);

      // get the segment associated with the gcell node
      auto parent = node->getParent();
      if (parent == nullptr || node->getType() != frNodeTypeEnum::frcSteiner) {
        continue; // skip root gcellNode, boundary node, or pin node
      }

      // get the parent node's location index
      FlexMazeIdx parentMIdx;
      gridGraph_.getMazeIdx(parent->getLoc(), parent->getLayerNum(), parentMIdx);
      const int parentLocIdx = getGCellIdx1D(parentMIdx.x(), parentMIdx.y(), parentMIdx.z());
      if (nodeMIdx.x() == parentMIdx.x()) { // vertical segment
        nodeLocIdx > parentLocIdx
          ? vSegments.push_back(std::make_pair(parentLocIdx, nodeLocIdx))
          : vSegments.push_back(std::make_pair(nodeLocIdx, parentLocIdx));
      } else if (nodeMIdx.y() == parentMIdx.y()) { // horizontal segment
        nodeLocIdx > parentLocIdx
          ? hSegments.push_back(std::make_pair(parentLocIdx, nodeLocIdx))
          : hSegments.push_back(std::make_pair(nodeLocIdx, parentLocIdx));
      } else {
        logger_->error(DRT, 373, "current node and parent node are are not aligned collinearly.");
      }  
    }

    // Push the net tree into the vector
    netTrees[netId] = std::move(netTree);
  }
}


void FlexGRWorker::batchGenerationRelax(
  std::vector<grNet*>& rerouteNets,
  std::vector<std::vector<grNet*> >& batches)
{
  batches.clear();
  batches.reserve(rerouteNets.size());
  const int numNets = static_cast<int>(rerouteNets.size()); 
  // Use mask to track the occupied gcells for each batch to detect conflicts
  // batchMask[i] is a 2D vector with the same size as the gcell grid of size (xGrids_ x yGrids_)
  std::vector<std::vector<bool> > batchMask; 
  batchMask.reserve(rerouteNets.size());

  std::sort(rerouteNets.begin(), rerouteNets.end(), 
    [](const grNet* a, const grNet* b) {
      return a->getHPWL() > b->getHPWL();
    });

  // Construct the netTrees
  std::vector<std::unique_ptr<NetStruct> > netTrees;
  netTrees.reserve(numNets);
  grNet2NetTree(rerouteNets, netTrees);

  // Define the lambda function to check if the net is in some batch
  // Here we use the representative point exhaustion, for non-exact overlap checking.
  // Only checks the two end points of a query segment
  // The checking may fail is the segment is too long 
  // and the two end points cover all the existing segments
  // For speedup, no worry, we need to use atomic operations when updating the demand in congestion map
  auto hasConflict_lambda = [&](std::vector<std::vector<bool> >::iterator maskIter, int netId) -> bool {
    for (auto& point : netTrees[netId]->points) {
      if ((*maskIter)[point]) { return true; }
    }
    return false;
  };

  auto findBatch_lambda = [&](int netId) -> int {
    std::vector<std::vector<bool> >::iterator maskIter = batchMask.begin();
    while (maskIter != batchMask.end()) {
      if (!hasConflict_lambda(maskIter, netId)) {
        return std::distance(batchMask.begin(), maskIter);
      }
      maskIter++;
    }
    return -1; 
  };    
  
  auto maskExactRegion_lambda = [&](int netId, std::vector<bool>& mask) -> void {    
    for (auto& vSeg : netTrees[netId]->vSegments) {
      for (int id = vSeg.first; id <= vSeg.second; id += xDim_) {
        mask[id] = true;
      }
    }

    for (auto& hSeg : netTrees[netId]->hSegments) {
      for (int id = hSeg.first; id <= hSeg.second; id++) {
        mask[id] = true;
      }
    }
  };

  // We limit the maximum batch size to the number of grids
  int numGrids = xDim_ * yDim_;
  for (int netId = 0; netId < numNets; netId++) {
    int batchId = findBatch_lambda(netId);  
    // always create a new batch if no batch is found
    if (batchId == -1 || batches[batchId].size() >= numGrids) {
      batchId = batches.size();
      batches.push_back(std::vector<grNet*>());
      batchMask.push_back(std::vector<bool>(static_cast<size_t>(numGrids), false));
    }  

    batches[batchId].push_back(rerouteNets[netId]);
    maskExactRegion_lambda(netId, batchMask[batchId]);      
  } 
}



}  // namespace drt
