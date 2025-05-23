/* Authors: Zhiang Wang */
/*
 * Copyright (c) 2024, The Regents of the University of California
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

#include "FlexGR.h"
#include "FlexGRWorker.h"

#include <omp.h>
#include <spdlog/common.h>
#include <sys/types.h>

#include <boost/polygon/rectangle_concept.hpp>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <queue>

#include "FlexGR_util_update.h"
#include "db/grObj/grShape.h"
#include "db/grObj/grVia.h"
#include "db/infra/frTime.h"
#include "db/obj/frGuide.h"
#include "odb/db.h"
#include "utl/exception.h"
#include "FlexGR_util.h"
#include "stt/SteinerTreeBuilder.h"

#include <cuda_runtime.h>
#include <cuda.h>
#include <thrust/device_vector.h>
#include <thrust/functional.h>
#include <thrust/host_vector.h>
#include <thrust/transform_reduce.h>
#include <string>
#include <functional>

namespace drt {


// From here we will implement the augmented L-shape routing graph
void FlexGR::getRipupRerouteNets_update(std::vector<frNet*>& nets2RR)
{
  // We collect all the nets that need to be ripup and reroute
  // We ripup and reroute all the nets with congestion more than 85%
  nets2RR.clear();
  nets2RR.reserve(nets_.size());

  // Should we use the average or maximum congestion or summation (demand / supply)
  // First calculate the congestion 
  auto& gCellPatterns = design_->getTopBlock()->getGCellPatterns();
  auto xgp = &(gCellPatterns.at(0));
  auto ygp = &(gCellPatterns.at(1));
  int gridXSize = xgp->getCount();
  int gridYSize = ygp->getCount();
  
  std::vector<std::vector<bool> > congestionMap(gridXSize, std::vector<bool>(gridYSize, false));  
 
  int numCongestedGCells = 0;
  for (int xIdx = 0; xIdx < gridXSize; xIdx++) {
    for (int yIdx = 0; yIdx < gridYSize; yIdx++) {
      int demand = cmap2D_->getDemand(xIdx, yIdx, 0, frDirEnum::E) + cmap2D_->getDemand(xIdx, yIdx, 0, frDirEnum::N);
      int supply = cmap2D_->getSupply(xIdx, yIdx, 0, frDirEnum::E) + cmap2D_->getSupply(xIdx, yIdx, 0, frDirEnum::N);
      if (demand * 1.0 / supply >= ripup_threshold_) {
        congestionMap[xIdx][yIdx] = true;
        numCongestedGCells++;
      }
    }
  }

  if (numCongestedGCells == 0) {
    logger_->report("[INFO][FlexGR] No congested GCells\n");
    return;
  }


  for (auto& net : nets_) {
    bool isCongested = false;
    for (auto& node : net->getNodes()) {
      frNode* parentNode = node->getParent();
      if (parentNode == nullptr) {
        continue;
      }

      if (node->getType() != frNodeTypeEnum::frcSteiner
          || parentNode->getType() != frNodeTypeEnum::frcSteiner) {
        continue;
      }

      Point childLocIdx = design_->getTopBlock()->getGCellIdx(node->getLoc());
      Point parentLocIdx = design_->getTopBlock()->getGCellIdx(parentNode->getLoc());

      if (childLocIdx.y() == parentLocIdx.y()) {
        const int xStart = std::min(childLocIdx.x(), parentLocIdx.x());
        const int xEnd = std::max(childLocIdx.x(), parentLocIdx.x());
        int yIdx = childLocIdx.y();
        for (int xIdx = xStart; xIdx <= xEnd; xIdx++) {
          if (congestionMap[xIdx][yIdx]) {
            isCongested = true;
            break;
          }
        }
      } else if (childLocIdx.x() == parentLocIdx.x()) {
        const int yStart = std::min(childLocIdx.y(), parentLocIdx.y());
        const int yEnd = std::max(childLocIdx.y(), parentLocIdx.y());
        int xIdx = childLocIdx.x();
        for (int yIdx = yStart; yIdx <= yEnd; yIdx++) {
          if (congestionMap[xIdx][yIdx]) {
            isCongested = true;
            break;
          }
        }
      } else {
        logger_->error(DRT, 286, "Error: non-root node does not have parent in getRipupRerouteNets_update\n");
      }      

      if (isCongested) {
        nets2RR.push_back(net);
        break; // stop traversing the net
      }
    }
  }

  logger_->report("[INFO][FlexGR] Number of congested GCells: {} ({}%)", 
    numCongestedGCells, 100.0 * numCongestedGCells / (gridXSize * gridYSize));
  
  logger_->report("[INFO][FlexGR] Number of nets to be ripup and reroute: {} ({}%)", 
    nets2RR.size(), 100.0 * nets2RR.size() / nets_.size());

  if (nets2RR.empty()) {
    logger_->report("[INFO][FlexGR] No nets to be ripup and reroute.");
    return;
  }

  for (auto& net : nets2RR) {
    initGR_ripup_update(net);
  }
}


void FlexGR::initGR_node_levelization_update(
  const std::vector<frNet*>& nets2RR,
  const std::vector<std::vector<int> >& netBatch,  
  int gridXSize,
  int gridYSize,
  int& totalNumNodes,
  int& maxNumNodes,
  int& maxNumChildren,
  int& maxNumLocs,
  std::vector<int>& nodeCntPtrVec,
  std::vector<int>& nodeLevel,
  std::vector<IntPair>& locVec,
  std::vector<IntPair>& nodeLocPtr,
  std::vector<int>& nodeParentIdx,  // store the parent and children relationship
  std::vector<int>& nodeEdgeIdx,
  std::vector<IntPair>& nodeEdgePtr,
  std::vector<int>& netBatchMaxDepth) 
{
  // Check the nodes
  totalNumNodes = 0;
  maxNumNodes = 0;
  maxNumChildren = 0;
  maxNumLocs = 0;
  
  // Shared resources for the thread pool
  nodeCntPtrVec.clear();  
  nodeCntPtrVec.reserve(nets2RR.size() + 1);
  for (auto& net : nets2RR) {
    nodeCntPtrVec.push_back(totalNumNodes);
    int numNodes = net->getNodes().size() - net->getRPins().size();  
    maxNumNodes = std::max(maxNumNodes, numNodes);
    totalNumNodes += numNodes;
  }  
  nodeCntPtrVec.push_back(totalNumNodes);

  nodeLevel.clear();
  nodeLevel.resize(totalNumNodes, -1);
  
  // Mark the location candidates for each steiner nodes 
  // If unknown errors occur, we can need to increase the reserved size
  locVec.clear();
  locVec.reserve(totalNumNodes * 5); 
  
  nodeLocPtr.clear();
  nodeLocPtr.resize(totalNumNodes);  

  nodeParentIdx.clear();
  nodeParentIdx.resize(totalNumNodes, -1);

  nodeEdgeIdx.clear();
  nodeEdgeIdx.reserve(totalNumNodes * maxNumNodes / 2.0);  

  nodeEdgePtr.clear();
  nodeEdgePtr.resize(totalNumNodes);  
 
  auto check_valid_loc = [&](int x, int y) {
    return x >= 0 && x < gridXSize && y >= 0 && y < gridYSize;
  };  
 
  auto getIdx = [&](int x, int y) {
    return y * gridXSize + x;
  };
  
  // Get the congestion cost
  std::vector<float> congestionCost;
  std::vector<float> overflowCost;
  congestionCost.reserve(gridXSize * gridYSize);
  overflowCost.reserve(gridXSize * gridYSize);  
  
  for (int yIdx = 0; yIdx < gridYSize; yIdx++) {
    for (int xIdx = 0; xIdx < gridXSize; xIdx++) {
      float vCost = getGridCost2D_update(xIdx, yIdx, frDirEnum::N);
      float hCost = getGridCost2D_update(xIdx, yIdx, frDirEnum::E);      
      congestionCost.push_back((vCost + hCost) / 2.0);
      overflowCost.push_back(getOverflow2D_update(xIdx, yIdx));
    }
  }

  relaxLimit_ = 2;

  // We only pick the gcell nodes that has lower cost than the current node
  auto addCandidateLoc = [&](int x, int y) {
    int lx = std::max(0, x - relaxLimit_);
    int ly = std::max(0, y - relaxLimit_);
    int ux = std::min(gridXSize - 1, x + relaxLimit_);
    int uy = std::min(gridYSize - 1, y + relaxLimit_);
    for (int x = lx; x <= ux; x++) {
      // need to update later
      for (int y = ly; y <= uy; y++) {
        locVec.emplace_back(x, y);
      }
    }
  };

  // Traverse the net in a pre-order DFS manner
  std::function<void(frNode*, frNet*, int&, int, int)> traverse_net_dfs_lambda = 
    [&](frNode* currNode, frNet* net, int& maxDepth, int baseIdx, int depth) {
    if (currNode->getType() != frNodeTypeEnum::frcSteiner) {
      return;
    }

    int currNodeIdx = distance(net->getFirstNonRPinNode()->getIter(), currNode->getIter()) + baseIdx;
    currNode->setIntProp(currNodeIdx);

    if (currNodeIdx >= nodeLevel.size()) {
      // We are going to remove this
      std::cout << "Distance : " << distance(net->getFirstNonRPinNode()->getIter(), currNode->getIter()) << std::endl;
      std::cout << "Distance of root: " << distance(net->getFirstNonRPinNode()->getIter(), net->getRootGCellNode()->getIter()) << std::endl;
      std::cout << "number of nodes : " << net->getNodes().size() - net->getRPins().size() << std::endl; 
      std::cout << "Invalid node index: " << currNodeIdx << std::endl;
      std::cout << "Base index: " << baseIdx << std::endl;
      std::cout << "Total number of nodes: " << nodeLevel.size() << std::endl;
      for (auto& tempNode : net->getNodes()) {
        std::cout << "node id : " << distance(net->getFirstNonRPinNode()->getIter(), tempNode->getIter()) << "  ";
        if (tempNode->getType() == frNodeTypeEnum::frcSteiner) {
          std::cout << "Steiner node" << std::endl;
        } else {
          std::cout << "RPin node" << std::endl;
        }
      }
      
      logger_->error(DRT, 358, "Invalid node index\n");
    }
    
    nodeLevel[currNodeIdx] = depth;
    maxDepth = std::max(maxDepth, depth);
    Point curLocIdx = design_->getTopBlock()->getGCellIdx(currNode->getLoc());
    nodeLocPtr[currNodeIdx].start = locVec.size();
    if (currNode->isDontMove() == true) {
      locVec.emplace_back(curLocIdx.x(), curLocIdx.y());
    } else {
      addCandidateLoc(curLocIdx.x(), curLocIdx.y());
    }

    // Get the parent and children information
    nodeLocPtr[currNodeIdx].end = locVec.size(); 
    maxNumLocs = std::max(maxNumLocs, nodeLocPtr[currNodeIdx].end - nodeLocPtr[currNodeIdx].start);
    nodeEdgePtr[currNodeIdx].start = nodeEdgeIdx.size();
    for (auto child : currNode->getChildren()) {
      if (child->getType() != frNodeTypeEnum::frcSteiner) {
        continue;
      }
      
      int childNodeIdx = distance(net->getFirstNonRPinNode()->getIter(), child->getIter()) + baseIdx;
      if (nodeLevel[childNodeIdx] != -1) {
        std::cout << "Error:  node already visited" << std::endl;
        exit(1);
      }
      
      nodeParentIdx[childNodeIdx] = currNodeIdx;      
      nodeEdgeIdx.push_back(childNodeIdx);
    }
    nodeEdgePtr[currNodeIdx].end = nodeEdgeIdx.size();
    maxNumChildren = std::max(maxNumChildren, nodeEdgePtr[currNodeIdx].end - nodeEdgePtr[currNodeIdx].start);
    // Traverse the children
    for (auto child : currNode->getChildren()) {
      traverse_net_dfs_lambda(child, net, maxDepth, baseIdx, depth + 1);
    }
  };

  // Traverse the nets
  netBatchMaxDepth.clear();
  netBatchMaxDepth.resize(netBatch.size(), 0);
  int batchId = 0;
  int depth = 0;
  int baseIdx = 0;
  // traverse the list in a DFS manner
  for (auto& batch : netBatch) {
    auto& maxDepth = netBatchMaxDepth[batchId++];
    for (auto netId : batch) {
      auto& net = nets2RR[netId];
      depth = 0;
      baseIdx = nodeCntPtrVec[netId];
      
      traverse_net_dfs_lambda(net->getRootGCellNode(), net, maxDepth, baseIdx, depth);
    }
  } 

  // Verify the correctness
  for (auto& net : nets2RR) {
    for (auto& node : net->getNodes()) {
      if (node->getType() != frNodeTypeEnum::frcSteiner) {
        continue;
      }
      int nodeIdx = node->getIntProp();
      //std::cout << "Node index: " << nodeIdx << std::endl;
      if (nodeIdx == -1) {
        logger_->error(DRT, 366, "Error: node not visited in initGR_node_levelization_update\n");
      }
    }
  }

  logger_->report("[INFO][FlexGR] Total number of nodes: {}", totalNumNodes);
  logger_->report("[INFO][FlexGR] Max number of nodes: {}", maxNumNodes);
  logger_->report("[INFO][FlexGR] Max number of children: {}", maxNumChildren);
  logger_->report("[INFO][FlexGR] Max number of locations: {}", maxNumLocs);
}


} // namespace drt


