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

// The file is designed for GPU-accelerated Layer Assignment

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
#include "gr/FlexGR_util.h"

namespace drt {


void FlexGR::layerAssign_gpu()
{
  // Perform layer assignment in a GPU-accelerated manner  
  // Step 1: sort all the nets based on the HPWL / numPins ratio
  std::vector<frNet*>& sortedNets = gpuDB_->sortedNets;
  sortedNets.reserve(design_->getTopBlock()->getNets().size());
  for (auto& uNet : design_->getTopBlock()->getNets()) {
    if (uNet->isGRValid() == false) {
      continue; // skip invalid nets
    }
    sortedNets.push_back(uNet.get());
  }

  // Sort the nets based on the HPWL / numPins ratio with lambda function
  std::sort(sortedNets.begin(), sortedNets.end(),
            [](const frNet* net_a, const frNet* net_b) {
              return (net_a->getNetPriority() < net_b->getNetPriority());
            });

  // To do list: please remove this part when the GPU-accelerated layer assignment is fully functional
  if (debugMode_) {
    // Just for debugging purpose, print the sorted nets
    // put the net priority into a file
    std::ofstream netPriorityFile("net_priority.txt");
    if (netPriorityFile.is_open()) {
      for (const auto& net : sortedNets) {
        netPriorityFile << net->getName() << " " << net->getNetPriority() << "\n";
      }
      netPriorityFile.close();
    } else {
      logger_->error(utl::DRT, 75, "Unable to open net_priority.txt for writing");
    }
  }

  // Step 2: preprocessing all the nets in a multi-threaded manner
  layerAssign_preproces(sortedNets);

  // Step 2: divide the nets into chunks
  // multi-threading is used
  std::vector<std::vector<int> > batches;   
  gpuDB_->generate2DBatch(sortedNets, batches);

  // Step 3: perform layer assignment in parallel for each chunk
  // Perform node levelization
  // Arrange node based on the batch size
  // multi-threading is used
  std::vector<NodeStruct> nodes;  
  std::vector<int> netBatchMaxDepth; // the maximum depth of the net batch
  std::vector<int> netBatchNodePtr;  // the pointer to the first node of the net batch
  gpuDB_->levelizeNodes(sortedNets, batches, nodes, netBatchMaxDepth, netBatchNodePtr);

  // Move the GPU side
  gpuDB_->layerAssign_CUDA(sortedNets, batches, nodes, netBatchMaxDepth, netBatchNodePtr);

  // Step 4: push the layer assignment results back to the router 
  // (1) remove the original 2D GR shapes
  // (2) construct the 2D GR shapes based on the layer assignment results
  // (4) remove unnecessary loops
}


void FlexGR::layerAssign_preproces(std::vector<frNet*>& sortedNets)
{
  // use multi-threading to preprocess the nets
  // use OpenMP to parallelize the preprocessing
  int numThreads = std::max(1, static_cast<int>(omp_get_max_threads()));
  int numNets = static_cast<int>(sortedNets.size());
  
#pragma omp parallel for num_threads(numThreads) schedule(dynamic)
  for (int i = 0; i < numNets; i++) {
    auto net = sortedNets[i];
    // Clear all the GR Shapes
    net->clearGRShapes();
    
    // break connections between rpin and gcell nodes
    unsigned rpinNodeSize = net->getRPins().size();
    auto& nodes = net->getNodes();
    auto endIter = nodes.begin();
    std::advance(endIter, rpinNodeSize);

    // Not sure why we have both firstNonRPinNode and rootGCellNode
    // We always set the rootGCellNode as the firstNonRPinNode
    std::string errMsg = "";
    // only loop over rpin nodes
    for (auto iter = nodes.begin(); iter != endIter; ++iter) {
      auto rpinNode = iter->get();
      if (rpinNode == net->getRoot()) {
        auto firstGCellNode = rpinNode->getChildren().front();   
        firstGCellNode->setParent(nullptr);      
        rpinNode->clearChildren(); // disconnect the root ripin node with the first gcell node
        // update the min and max pin layer number for the first gcell node
        // We assume the pin metal layer is M1 and above
        int pinLayerNum = rpinNode->getLayerNum() / 2 - 1;
        firstGCellNode->updateMinPinLayerNum(pinLayerNum);
        firstGCellNode->updateMaxPinLayerNum(pinLayerNum);
        if (pinLayerNum < 0) {
          errMsg = "Error: pinLayerNum is negative for net " + net->getName();
          break;
        }

        if (firstGCellNode->isDontMove() == false) {
          errMsg = "Error: firstGCellNode is not a pinGCellNode for net " + net->getName();
          break;
        }
      } else {
        auto parent = rpinNode->getParent();
        // disconnect the ripin node with its parent
        parent->removeChild(rpinNode);
        rpinNode->setParent(nullptr);
        // update the min and max pin layer number for the ripin node
        int pinLayerNum = rpinNode->getLayerNum() / 2 - 1;
        parent->updateMinPinLayerNum(pinLayerNum);
        parent->updateMaxPinLayerNum(pinLayerNum);
        if (pinLayerNum < 0) {
          errMsg = "Error: pinLayerNum is negative for net " + net->getName();
          break;
        }
      
        if (parent->isDontMove() == false) {
          // If the parent is not a pinGCellNode, we have an error
          errMsg = "Error: parent node is not a pinGCellNode for net " + net->getName();
          break;
        }
      }
    
      if (!errMsg.empty()) {
        logger_->error(utl::DRT, 80, "{}", errMsg);
        continue; // skip this net if there is an error
      }

      // clear all connFig
      for (auto& node : net->getNodes()) {
        node->setConnFig(nullptr);
      }    
    }
  }
}

























































  
} // namespace drt





