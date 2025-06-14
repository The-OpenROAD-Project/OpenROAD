/* Authors: Zhiang Wang */
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

#include "FlexGR_GPUDB.h"

#include <omp.h>
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>

#include "db/grObj/grShape.h"
#include "db/grObj/grVia.h"
#include "db/infra/frTime.h"
#include "db/obj/frGuide.h"
#include "odb/db.h"
#include "utl/exception.h"

namespace drt {




// Define utility functions here
void FlexGRGPUDB::frNet2NetTree(
  std::vector<frNet*>& sortedNets,
  std::vector<std::unique_ptr<NetStruct> >& netTrees)
{
  netTrees.clear();
  netTrees.resize(sortedNets.size());
  int numNets = static_cast<int>(sortedNets.size());
  
  // Use OpenMP to parallelize the net tree generation
  int numThreads = std::max(1, static_cast<int>(omp_get_max_threads()));
  #pragma omp parallel for num_threads(numThreads) schedule(dynamic)
  for (int i = 0; i < numNets; i++) {
    auto& net = sortedNets[i];
    std::unique_ptr<NetStruct> netTree = std::make_unique<NetStruct>();
    netTree->netId = i;  
    auto& points = netTree->points;
    auto& vSegments = netTree->vSegments;
    auto& hSegments = netTree->hSegments;

    // Use iterator to traverse the net's GCell nodes
    auto& nodes = net->getNodes();
    auto iter = nodes.begin();
    // skip all the rpin nodes
    std::advance(iter, net->getRPins().size());    
    // Iterate through the GCell nodes
    for (; iter != nodes.end(); ++iter) {
      auto gcellNode = iter->get();
      Point locIdx = design_->getTopBlock()->getGCellIdx(gcellNode->getLoc());
      int nodeLocIdx = getGCellIdx1D(locIdx.x(), locIdx.y());
      points.push_back(nodeLocIdx);

      // get the segment associated with the gcell node
      auto parent = gcellNode->getParent();
      if (parent == nullptr || parent->getType() == frNodeTypeEnum::frcPin) {
        continue; // skip root gcellNode
      }

      // get the parent node's location index
      Point parentLocIdx = design_->getTopBlock()->getGCellIdx(parent->getLoc());
      int parentLocIdx1D = getGCellIdx1D(parentLocIdx.x(), parentLocIdx.y());
      if (locIdx.x() == parentLocIdx.x()) {
        // vertical segment
        nodeLocIdx > parentLocIdx1D
          ? vSegments.push_back(std::make_pair(parentLocIdx1D, nodeLocIdx))
          : vSegments.push_back(std::make_pair(nodeLocIdx, parentLocIdx1D));
      } else if (locIdx.y() == parentLocIdx.y()) {
        // horizontal segment
        nodeLocIdx > parentLocIdx1D
          ? hSegments.push_back(std::make_pair(parentLocIdx1D, nodeLocIdx))
          : hSegments.push_back(std::make_pair(nodeLocIdx, parentLocIdx1D));
      } else {
        logger_->error(DRT, 287, "current node and parent node are are not aligned collinearly.");
      }  
    }

    // Push the net tree into the vector
    netTrees[i] = std::move(netTree);
  }
}


void FlexGRGPUDB::generate2DBatch(
  std::vector<frNet*>& sortedNets,
  std::vector<std::vector<int> >& batches)
{
  int numNets = static_cast<int>(sortedNets.size());
  batches.clear();
  batches.reserve(numNets);
 
  // Use mask to track the occupied gcells for each batch to detect conflicts
  // batchMask[i] is a 2D vector with the same size as the gcell grid of size (xGrids_ x yGrids_)
  std::vector<std::vector<bool> > batchMask; 
  batchMask.reserve(numNets);

  // Construct the netTrees
  std::vector<std::unique_ptr<NetStruct> > netTrees;
  netTrees.reserve(numNets);
  frNet2NetTree(sortedNets, netTrees);

  // Define the lambda function to check if the net is in some batch
  // Here we use the representative point exhaustion, for non-exact overlap checking.
  
  // Only checks the two end points of a query segment
  // The checking may fail is the segment is too long 
  // and the two end points cover all the existing segments
  // For speedup, no worry, we need to use atomic operations when updating the demand in congestion map
  auto hasConflict = [&](std::vector<std::vector<bool> >::iterator maskIter, int netId) -> bool {
    for (auto& point : netTrees[netId]->points) {
      if ((*maskIter)[point]) { return true; }
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
  
  auto maskExactRegion = [&](int netId, std::vector<bool>& mask) -> void {    
    for (auto& vSeg : netTrees[netId]->vSegments) {
      for (int id = vSeg.first; id <= vSeg.second; id += xDim) {
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
  if (debugMode_) {
    int batchIdx = 0;
    for (const auto& batch : batches) {
      logger_->report("[INFO] Batch {} has {} nets", batchIdx, batch.size());
      batchIdx++;
    }
  }
}



void FlexGRGPUDB::levelizeNodes(
  std::vector<frNet*>& sortedNets,
  std::vector<std::vector<int> >& batches,      
  std::vector<NodeStruct>& nodes,
  std::vector<int>& netBatchMaxDepth,
  std::vector<int>& netBatchNodePtr)
{
  netBatchNodePtr.clear();
  netBatchNodePtr.reserve(batches.size() + 1);
  netBatchNodePtr.push_back(0);

  std::vector<int> netNodePtr;
  netNodePtr.reserve(sortedNets.size() + 1);
  
  netNodePtr.push_back(0);
  int totNumNodes = 0;
  for (const auto& batch : batches) {
    for (auto netId : batch) {
      auto& net = sortedNets[netId];
      totNumNodes += net->getNodes().size() - net->getRPins().size(); // exclude the rpin nodes          
      netNodePtr.push_back(totNumNodes);
    }
    netBatchNodePtr.push_back(totNumNodes);
  }

  nodes.clear();
  nodes.resize(totNumNodes);


  // Create NodeStruct in parallel 
  int numNets = static_cast<int>(sortedNets.size());
  int numThreads = std::max(1, static_cast<int>(omp_get_max_threads()));
  #pragma omp parallel for num_threads(numThreads) schedule(dynamic)

  




  



































}


}  // namespace drt
