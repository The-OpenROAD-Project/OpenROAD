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

__global__
void layerAssignNodeCompute__kernel(  
  NodeStruct* d_nodes,
  unsigned* d_bestLayerCombs,
  unsigned* d_bestLayerCosts,
  int nodeStartIdx, int nodeEndIdx, int depth,
  uint64_t* d_cmap, bool* d_layerDir,
  int xDim, int yDim, int numLayers,
  unsigned VIACOST, unsigned VIA_ACCESS_LAYERNUM,
  unsigned BLOCKCOST, unsigned MARKERCOST,
  unsigned LA_PIN_LAYER_COST_FACTOR)
{
  int tIdx = blockIdx.x * blockDim.x + threadIdx.x;
  int nodeId = tIdx + nodeStartIdx;
  if (nodeId >= nodeEndIdx) { return;  } // Out of bounds 

  // Copy to register
  NodeStruct node = d_nodes[nodeId];
  if (node.level != depth) { return; }

  int numChild = node.childCnt;
  int numComb = 1;
  #pragma unroll
  for (int i = 0; i < 4; i++) // Unroll if max 4 children
    if (i < numChild) numComb *= numLayers;
  
  // cache node-specific parameters outside the combination loop
  const int minPinLayer = node.minLayerNum;
  const int maxPinLayer = node.maxLayerNum;
  const int curLocX = node.x;
  const int curLocY = node.y;
  int parentIdx = node.parentIdx;
  // Preload children into local array (max 4)
  int children[4];  
  #pragma unroll
  for (int i = 0; i < 4; i++)
    children[i] = node.children[i];

  // iterate over all combinations and get the combination with lowest overall cost  
  for (int layerNum = 0; layerNum < numLayers; layerNum++) {
    unsigned currLayerBestCost = UINT_MAX;
    unsigned currLayerBestComb = 0;
    // Iterate over each combination
    for (unsigned comb = 0; comb < numComb; comb++) {
      unsigned currComb = comb; // current combination index
      unsigned downStreamCost = 0;
      int downstreamMinLayerNum = INT_MAX;
      int downstreamMaxLayerNum = INT_MIN;
      // Iterate through all children for the current combination
      // For each child, determine its layer in this combination and add cost
      #pragma unroll
      for (int i = 0; i < 4; i++) {
        if (i < numChild) {
          int childIdx = children[i];
          int childLayerNum = currComb % numLayers;  // determine child layer
          downstreamMinLayerNum = min(downstreamMinLayerNum, childLayerNum);
          downstreamMaxLayerNum = max(downstreamMaxLayerNum, childLayerNum);
          currComb /= numLayers; // update combination index for next child
          // Accumulate downstream cost for this child
          downStreamCost += d_bestLayerCosts[childIdx * numLayers + childLayerNum];
        }
      }
      
      downstreamMinLayerNum = min(downstreamMinLayerNum, minPinLayer);
      downstreamMaxLayerNum = max(downstreamMaxLayerNum, maxPinLayer);
      const unsigned downstreamViaCost =
        (max(layerNum, maxPinLayer) - min(layerNum, minPinLayer)) * VIACOST;

      // Compute upstream edge congestion cost
      // Pin layer routing penalty
      unsigned congestionCost = 0;
      if (layerNum <= (VIA_ACCESS_LAYERNUM / 2 - 1)) { // Pin layer routing adjustment
        congestionCost += VIACOST * LA_PIN_LAYER_COST_FACTOR;
      }

      // If the node has a parent, compute congestion cost along the connecting edge
      if (parentIdx != -1) {
        int parentX = d_nodes[parentIdx].x;
        int parentY = d_nodes[parentIdx].y;

        if (curLocX == parentX) { // vertical segment
          congestionCost += 100;
        } else if (curLocY == parentY) { // horizontal segment
          congestionCost += 100;
        } else {
          printf("Node %d: current node and parent node are not aligned collinearly\n", nodeId);
        }
      }

      unsigned currLayerCost = downStreamCost + downstreamViaCost + congestionCost;
      if (currLayerCost < currLayerBestCost) {
        currLayerBestCost = currLayerCost;
        currLayerBestComb = comb;
      }
    } // end of combination loop
    
    d_bestLayerCosts[nodeId * numLayers + layerNum] = currLayerBestCost;
    d_bestLayerCombs[nodeId * numLayers + layerNum] = currLayerBestComb;
  } // end of layer loop     
}



// To do: large-net with highest depth should be done in CPU mode
// instead of GPU mode

void FlexGRGPUDB::layerAssign_CUDA(
  std::vector<frNet*>& sortedNets,
  std::vector<std::vector<int> >& batches,
  std::vector<NodeStruct>& nodes,
  std::vector<int>& netBatchMaxDepth,
  std::vector<int>& netBatchNodePtr)
{
  int totNumNodes = nodes.size();

  NodeStruct* d_nodes;
  unsigned* d_bestLayerCombs;
  unsigned* d_bestLayerCosts;

  cudaMalloc((void**)&d_nodes, nodes.size() * sizeof(NodeStruct));
  cudaMalloc((void**)&d_bestLayerCombs, totNumNodes * sizeof(unsigned));
  cudaMalloc((void**)&d_bestLayerCosts, totNumNodes * sizeof(unsigned));

  // Copy the data from the host to the device
  cudaMemcpy(d_nodes, nodes.data(), nodes.size() * sizeof(NodeStruct), cudaMemcpyHostToDevice);
  cudaMemset(d_bestLayerCombs, 0, totNumNodes * sizeof(unsigned));
  cudaMemset(d_bestLayerCosts, UINT_MAX, totNumNodes * sizeof(unsigned));

  // sync CMap
  syncCMapHostToDevice();

  cudaCheckError();

  // Node-level layer assignment kernel
  int numBatches = netBatchMaxDepth.size();
  // launch the kernel with one thread per node
  for (int batchId = 0; batchId < numBatches; batchId++) {
    int maxDepth = netBatchMaxDepth[batchId];
    int nodeStartIdx = netBatchNodePtr[batchId];
    int nodeEndIdx = netBatchNodePtr[batchId + 1];
    int numNodes = nodeEndIdx - nodeStartIdx;
    int numThreads = 256;
    int numBlocks = (numNodes + numThreads - 1) / numThreads;
   
    if (debugMode_) {
      std::string msg = std::string("[INFO] ")
                      + std::string("FlexGRGPUDB::layerAssign_CUDA: ")
                      + "batchId: " + std::to_string(batchId) + ", "
                      + "maxDepth: " + std::to_string(maxDepth) + ", "
                      + "nodeStartIdx: " + std::to_string(nodeStartIdx) + ", "
                      + "nodeEndIdx: " + std::to_string(nodeEndIdx) + ", "
                      + "numNodes: " + std::to_string(numNodes) + ", "
                      + "numBlocks: " + std::to_string(numBlocks) + ", "
                      + "numThreads: " + std::to_string(numThreads);
      logger_->report(msg);
    }    

    // node-level compute
    for (int depth = maxDepth; depth >= 0; depth--) { 
      layerAssignNodeCompute__kernel<<<numBlocks, numThreads>>>(
        d_nodes, d_bestLayerCombs, d_bestLayerCosts,
        nodeStartIdx, nodeEndIdx, depth,
        d_cmap_bits_3D, d_layerDir, xDim, yDim, zDim, 
        router_cfg_->VIACOST,
        router_cfg_->VIA_ACCESS_LAYERNUM,
        router_cfg_->BLOCKCOST,
        router_cfg_->MARKERCOST,
        LA_PIN_LAYER_COST_FACTOR);
    }
    
    cudaDeviceSynchronize();  // Wait for the kernel to finish
    cudaCheckError();
  }


  cudaCheckError();  

  // copy the solution from the device to the host
  cudaMemcpy(nodes.data(), d_nodes, nodes.size() * sizeof(NodeStruct), cudaMemcpyDeviceToHost);

  // Free device memory
  cudaFree(d_nodes);
  cudaFree(d_bestLayerCombs);
  cudaFree(d_bestLayerCosts);
  cudaCheckError();
}





}  // namespace drt
