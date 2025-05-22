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


#include "db/grObj/grShape.h"
#include "db/grObj/grVia.h"
#include "db/infra/frTime.h"
#include "db/obj/frGuide.h"
#include "odb/db.h"
#include "utl/exception.h"
#include "FlexGR_CUDA_object.h"


namespace drt {


void FlexGR::updateNetAttribute(
  std::vector<std::pair<int, frNet*> >& sortedNets,
  int mode, // Mode 0 for HWPL and 1 for rHPWL
  frNet* net)
{
  if (mode == 0) {
    frCoord llx = INT_MAX;
    frCoord lly = INT_MAX;
    frCoord urx = INT_MIN;
    frCoord ury = INT_MIN;    
    for (auto& rpin : net->getRPins()) {
      Rect bbox = rpin->getBBox();
      llx = std::min(bbox.xMin(), llx);
      lly = std::min(bbox.yMin(), lly);
      urx = std::max(bbox.xMax(), urx);
      ury = std::max(bbox.yMax(), ury);
    }
    int numRPins = net->getRPins().size();
    int ratio = ((urx - llx) + (ury - lly)) / (numRPins);
    sortedNets.emplace_back(ratio, net);
    return;
  }    

  // Use the rWL as the metric
  // Number of Steiner points
  int numSteinerNodes = 0;
  int rWL = 0;
  int numPins = 0;

  auto root = net->getRoot();
  std::queue<frNode*> nodeQ;  
  nodeQ.push(root);

  // if a node is connected to a frcPin, then the steiner node is a pinGCellNode
  while (!nodeQ.empty()) {
    auto node = nodeQ.front();
    nodeQ.pop();    
    if (node->getType() == frNodeTypeEnum::frcSteiner) {
      numSteinerNodes++;
    } else if (node->getType() == frNodeTypeEnum::frcPin) {
      numPins++;
    }
    
    Point loc = node->getLoc();
    frLayerNum lNum = node->getLayerNum();  

    // set the dontMove flag for the pin node
    if (node->getType() == frNodeTypeEnum::frcPin) {
      if (node->getChildren().size() > 1) {
        logger_->error(utl::DRT, 49, "Error: pin node has more than one child");
      } else if (node->getChildren().size() == 1) {
        auto child = node->getChildren().front();
        child->setDontMove();
      }     
    } else if (node->getType() == frNodeTypeEnum::frcSteiner) {
      bool dontMoveFlag = false;
      for (auto& child : node->getChildren()) {
        if (child->getType() == frNodeTypeEnum::frcPin) {
          dontMoveFlag = true;
          break;
        }
      }
      if (dontMoveFlag) {
        node->setDontMove();
      }
    }

    for (auto child : node->getChildren()) {
      nodeQ.push(child);
      if (child->getType() == frNodeTypeEnum::frcSteiner && node->getType() == frNodeTypeEnum::frcSteiner) {
        Point childLoc = child->getLoc();
        frLayerNum childLNum = child->getLayerNum();
        if (childLNum != lNum) {
          logger_->error(utl::DRT, 25, "Error: child and parent nodes are on different layers");
        }
        rWL += abs(childLoc.x() - loc.x()) + abs(childLoc.y() - loc.y());
      }
    }
  }  

  int ratio = rWL / numPins;
  net->setNumSteinerNodes(numSteinerNodes); // Some nodes are pinGCellNodes (which cannot be moved)
  sortedNets.emplace_back(ratio, net);
  //std::cout << "Net " << net->getName() << "  ratio = " << ratio << std::endl;
}


void FlexGR::layerAssign_update()
{
  if (gpuFlag_ == false) {
    return; // This function is only for GPU-accelerated Layer Assignment
  }

  logger_->report("[INFO] Starting GPU-accelerated Layer assignment...");

  std::vector<std::pair<int, frNet*> > sortedNets;
  for (auto& uNet : design_->getTopBlock()->getNets()) {
    auto net = uNet.get();
    if (net2GCellNodes_.find(net) == net2GCellNodes_.end()
        || net2GCellNodes_[net].size() <= 1) {
      continue;
    }
    updateNetAttribute(sortedNets, 1, net);
  }

  // sort
  struct sort_net
  {
    bool operator()(const std::pair<int, frNet*>& left,
                    const std::pair<int, frNet*>& right)
    {
      if (left.first == right.first) {
        return (left.second->getId() < right.second->getId());
      }
      return (left.first < right.first);
    }
  };
  
  sort(sortedNets.begin(), sortedNets.end(), sort_net()); 
  
  // Perform layer assignment in a chunk manner
  // i.e., assign layerAssignChunkSize_ nets in parallel at one time
  // This is to avoid memory overflow for large designs
  int numChunks = (sortedNets.size() + layerAssignChunkSize_ - 1) / layerAssignChunkSize_;
  for (int chunkId = 0; chunkId < numChunks; chunkId++) {
    int chunkStartIdx = chunkId * layerAssignChunkSize_;
    int chunkEndIdx = std::min((chunkId + 1) * layerAssignChunkSize_, (int) sortedNets.size());
    layerAssign_chunk(sortedNets, chunkStartIdx, chunkEndIdx);
  }
}


void FlexGR::layerAssign_chunk(
  std::vector<std::pair<int, frNet*> >& sortedNets,
  int chunkStartIdx, int chunkEndIdx)
{
  // The parallel update is done in a node-based manner
  int totalNumNodes = 0;
  for (int netId = chunkStartIdx; netId < chunkEndIdx; netId++) {
    totalNumNodes += sortedNets[netId].second->getNumSteinerNodes();
  }
  int numLayers = cmap_->getNumLayers();

  logger_->report("[INFO] Layer assignment for {} nets ...", chunkEndIdx - chunkStartIdx);
  logger_->report("[INFO] Layer assignment for {} nodes ...", totalNumNodes);
  logger_->report("[INFO] Layer assignment for {} layers ...", numLayers);


  // use openmp to parallelize the computation
#pragma omp parallel for schedule(dynamic)
  for (auto netId = chunkStartIdx; netId < chunkEndIdx; netId++) {
    auto net = sortedNets[netId].second;
    // Clear all the GR Shapes
    net->clearGRShapes();
    // break connections between rpin and gcell nodes
    unsigned rpinNodeSize = net->getRPins().size();
    auto iter = net->getNodes().begin();
    //  update the first non-RPin node
    auto endIter = net->getNodes().begin();
    std::advance(endIter, rpinNodeSize);
    net->setFirstNonRPinNode(endIter->get());
    // set the endIter to the last node
    for(; iter != endIter; iter++) {
      auto node = iter->get();
      if (node == net->getRoot()) {
        auto firstGCellNode = node->getChildren().front();   
        firstGCellNode->setParent(nullptr);      
        net->setRootGCellNode(firstGCellNode);
        node->clearChildren(); // disconnect the root ripin node with the first gcell node
        // update the min and max pin layer number for the first gcell node
        auto pinLayerNum = node->getLayerNum() / 2 - 1;
        firstGCellNode->updateMinPinLayerNum(pinLayerNum);
        firstGCellNode->updateMaxPinLayerNum(pinLayerNum);
        if (firstGCellNode->isDontMove() == false) {
          logger_->error(utl::DRT, 64, "Error: firstGCellNode is not a pinGCellNode");
        }
      } else {
        auto parent = node->getParent();
        // disconnect the ripin node with its parent
        parent->removeChild(node);
        node->setParent(nullptr);
        // update the min and max pin layer number for the ripin node
        auto pinLayerNum = node->getLayerNum() / 2 - 1;
        parent->updateMinPinLayerNum(pinLayerNum);
        parent->updateMaxPinLayerNum(pinLayerNum);
        if (parent->isDontMove() == false) {
          logger_->error(utl::DRT, 71, "Error: parent node is not a pinGCellNode");
        }
      }
    }
  
    for (auto& node : net->getNodes()) {
      node->setConnFig(nullptr);
      // for testing
      // if (node->getType() == frNodeTypeEnum::frcSteiner && node->isDontMove() == true) {
      //  if (node->getMinPinLayerNum() != node->getMaxPinLayerNum()) {
      //    std::cout << "Node minPinLayerNum = " << node->getMinPinLayerNum() << ", maxPinLayerNum = " << node->getMaxPinLayerNum() << std::endl;
      //  }
      //}
    }
  }



  // Step 2: the parallel update
  // Step 2.1: Perform batch generation for nets
  std::vector<std::vector<int> > batchNets;
  layerAssign_batchGeneration(sortedNets, batchNets, chunkStartIdx, chunkEndIdx);

  // Step 2.2: Convert the net-level to the node-level parallelization
  std::vector<NodeStruct> nodes;
  std::vector<int> netBatchMaxDepth; // the maximum depth of the net batch
  std::vector<int> netBatchNodePtr;  // the pointer to the first node of the net batch

  netBatchNodePtr.push_back(0);
  nodes.reserve(totalNumNodes);
  netBatchMaxDepth.resize(batchNets.size(), 0);

  int batchId = 0;
  int depth = 0;
  // traverse the list in a DFS manner
  for (auto& batch : batchNets) {
    auto& maxDepth = netBatchMaxDepth[batchId++];
    for (auto netId : batch) {
      auto& net = sortedNets[chunkStartIdx + netId].second;
      depth = 0;
      layerAssign_nodeLevelization(nodes, net->getRootGCellNode(), netId, depth, maxDepth);
    }
    netBatchNodePtr.push_back(nodes.size());    
  }

  // print the depth
  for (int i = 0; i < netBatchMaxDepth.size(); i++) {
    logger_->report("[INFO] Net batch {} has max depth = {}", i, netBatchMaxDepth[i]);
  }

  std::vector<unsigned> bestLayerCosts(static_cast<size_t>(totalNumNodes) * numLayers, UINT_MAX);
  std::vector<unsigned> bestLayerCombs(static_cast<size_t>(totalNumNodes) * numLayers, 0);
  layerAssign_node_compute_CUDA(bestLayerCosts, bestLayerCombs, netBatchNodePtr, netBatchMaxDepth, nodes);

  return;
}



void FlexGR::layerAssign_batchGeneration(
  std::vector<std::pair<int, frNet*> >& sortedNets,
  std::vector<std::vector<int> >& batches,
  int chunkStartIdx, int chunkEndIdx)
{
  int numNets = chunkEndIdx - chunkStartIdx; 
  batches.clear();
  batches.reserve(numNets);
 
  // Use mask to track the occupied gcells for each batch to detect conflicts
  // batchMask[i] is a 2D vector with the same size as the gcell grid of size (xGrids_ x yGrids_)
  std::vector<std::vector<bool> > batchMask; 
  batchMask.reserve(numNets);

  int xDim, yDim, zDim;
  cmap_->getDim(xDim, yDim, zDim);
  logger_->report("[INFO] xDim = {}, yDim = {}, zDim = {}", xDim, yDim, zDim);

  // Define the lambda function to get the idx for each gcell
  // We use the row-major order to index the gcells
  auto getGCellIdx1D = [xDim](int x, int y) {
    return y * xDim + x;
  };

  // Construct the netTrees
  std::vector<NetStruct> netTrees;
  netTrees.reserve(numNets);
  for (int netId = chunkStartIdx; netId < chunkEndIdx; netId++) {
    auto& net = sortedNets[netId].second;
    NetStruct netTree;
    netTree.netId = static_cast<int>(netTrees.size());
    auto& points = netTree.points;
    auto& vSegments = netTree.vSegments;
    auto& hSegments = netTree.hSegments;

    auto root = net->getRootGCellNode();
    std::queue<frNode*> nodeQ;  
    nodeQ.push(root);
  
    // if a node is connected to a frcPin, then the steiner node is a pinGCellNode
    while (!nodeQ.empty()) {
      auto node = nodeQ.front();
      nodeQ.pop();    
      if (node->getType() == frNodeTypeEnum::frcPin) {
        continue;
      }

      Point locIdx = design_->getTopBlock()->getGCellIdx(node->getLoc());
      int nodeLocIdx = getGCellIdx1D(locIdx.x(), locIdx.y());
      points.push_back(nodeLocIdx);
      for (auto& child : node->getChildren()) {
        nodeQ.push(child);
        Point childLocIdx = design_->getTopBlock()->getGCellIdx(child->getLoc());
        int childLocIdx1D = getGCellIdx1D(childLocIdx.x(), childLocIdx.y());
        if (childLocIdx.x() == locIdx.x()) {
          nodeLocIdx > childLocIdx1D 
            ? vSegments.push_back(std::make_pair(childLocIdx1D, nodeLocIdx)) 
            : vSegments.push_back(std::make_pair(nodeLocIdx, childLocIdx1D));
        } else if (childLocIdx.y() == locIdx.y()) {
          nodeLocIdx > childLocIdx1D 
            ? hSegments.push_back(std::make_pair(childLocIdx1D, nodeLocIdx)) 
            : hSegments.push_back(std::make_pair(nodeLocIdx, childLocIdx1D));
        } else {
          logger_->error(DRT, 265, "current node and parent node are are not aligned collinearly.");
        } 
      }
    }
    netTrees.push_back(netTree);
  }

  // Define the lambda function to check if the net is in some batch
  // Here we use the representative point exhaustion, for non-exact overlap checking.
  // Only checks the two end points of a query segment
  // The checking may fail is the segment is too long 
  // and the two end points cover all the existing segments
  auto hasConflict = [&](std::vector<std::vector<bool> >::iterator maskIter, int netId) -> bool {
    for (auto& point : netTrees[netId].points) {
      if ((*maskIter)[point]) {
        return true;
      }
    }
    return false;
  };

  auto findBatch = [&](int netId) -> int {
    std::vector<std::vector<bool> >::iterator maskIter = batchMask.begin();
    while (maskIter != batchMask.end()) {
      if (!hasConflict(maskIter, netId)) {
        return std::distance(batchMask.begin(), maskIter);
      }
      maskIter++;
    }
    return -1; 
  };    
    
  auto maskExactRegion = [&](int netId, std::vector<bool>& mask) {    
    for (auto& vSeg : netTrees[netId].vSegments) {
      for (int id = vSeg.first; id <= vSeg.second; id += xDim) {
        mask[id] = true;
      }
    }

    for (auto& hSeg : netTrees[netId].hSegments) {
      for (int id = hSeg.first; id <= hSeg.second; id++) {
        mask[id] = true;
      }
    }
  };

  int numGrids = xDim * yDim;
  for (int netId = 0; netId < numNets; netId++) {
    int batchId = findBatch(netId);  
    // always create a new batch if no batch is found
    if (batchId == -1 || batches[batchId].size() >= numGrids) {
      batchId = batches.size();
      batches.push_back(std::vector<int>());
      batchMask.push_back(std::vector<bool>(static_cast<size_t>(numGrids), false));
    }  
    batches[batchId].push_back(netId);
    maskExactRegion(netId, batchMask[batchId]);      
  }

  // For testing
  int batchIdx = 0;
  for (auto& batch : batches) {
    logger_->report("[INFO] BatchId: {}  Batch size: {}", batchIdx++, batch.size());
  }
}


// At this stage, all the rpin nodes are disconnected from the gcell nodes
// Postorder DFS
void FlexGR::layerAssign_nodeLevelization(
  std::vector<NodeStruct>& nodes,
  frNode* currNode, 
  int netId, int depth, int& maxDepth)
{ 
  for (auto child : currNode->getChildren()) {
    layerAssign_nodeLevelization(nodes, child, netId, depth + 1, maxDepth);        
  }
  
  int nodeIdx = static_cast<int>(nodes.size());
  currNode->setIntProp(nodeIdx);
  Point curLocIdx = design_->getTopBlock()->getGCellIdx(currNode->getLoc());
  maxDepth = std::max(maxDepth, depth);

  NodeStruct node_s;
  node_s.netId = netId;
  node_s.nodeIdx = nodeIdx;
  node_s.level = depth; 
  node_s.x = curLocIdx.x();
  node_s.y = curLocIdx.y();
  node_s.minLayerNum = currNode->getMinPinLayerNum();
  node_s.maxLayerNum = currNode->getMaxPinLayerNum();
  node_s.childCnt = 0;
  node_s.layerNum = 0xFF;
  for (auto child : currNode->getChildren()) {
    node_s.children[node_s.childCnt++] = child->getIntProp();
  }
  nodes.push_back(node_s);
}

  
} // namespace drt





