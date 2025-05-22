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


#include <iostream>
#include <cuda_runtime.h>
#include <cuda.h>
#include <thrust/device_vector.h>
#include <thrust/functional.h>
#include <thrust/host_vector.h>
#include <thrust/transform_reduce.h>
#include <string>
#include <functional>

#include "db/grObj/grShape.h"
#include "db/grObj/grVia.h"
#include "db/infra/frTime.h"
#include "db/obj/frGuide.h"
#include "odb/db.h"
#include "utl/exception.h"
#include "stt/SteinerTreeBuilder.h"
#include "gr/FlexGRCMap.h"
#include "gr/FlexGR.h"
#include "frBaseTypes.h"
#include "frDesign.h"
#include "gr/FlexGR_util.h"


namespace drt {


__device__
void addRawDemandSegment__device(
  uint64_t* d_cmap, 
  int xDim, int yDim, int numLayers,
  int parentX, int parentY, int childX, int childY, 
  int childLayerNum)
{


}



__global__
void layerAssignNodeCompute__kernel(  
  NodeStruct* d_nodes,
  unsigned* d_bestLayerCombs,
  unsigned* d_bestLayerCosts,
  uint64_t* d_costMap,
  bool* d_layerDir,
  int xDim, int yDim, int numLayers,
  int nodeStartIdx, int nodeEndIdx, int depth,
  unsigned VIACOST_DEVICE,
  unsigned VIA_ACCESS_LAYERNUM_DEVICE,
  unsigned BLOCKCOST_DEVICE,
  unsigned MARKERCOST_DEVICE)
{
  int tIdx = threadIdx.x + blockIdx.x * blockDim.x;
  int nodeId = tIdx + nodeStartIdx;
  if (nodeId >= nodeEndIdx) {
    return;
  }

  NodeStruct& node = d_nodes[nodeId];
  if (node.level != depth) {
    return;
  }

  int numChild = node.childCnt;
  int numComb = 1;
  for (int i = 0; i < numChild; i++) numComb *= numLayers;
  
  printf("Node %d, numComb: %d\n", nodeId, numComb);

  // iterate over all combinations and get the combination with lowest overall cost  
  for (int layerNum = 0; layerNum < numLayers; layerNum++) {
    unsigned currLayerBestCost = UINT_MAX;
    unsigned currLayerBestComb = 0;
    
    // cache node-specific parameters outside the combination loop
    int minPinLayer = node.minLayerNum;
    int maxPinLayer = node.maxLayerNum;
    int parentIdx = node.parentIdx;
    int curLocX = node.x;
    int curLocY = node.y;

    // Iterate over each combination
    for (unsigned comb = 0; comb < numComb; comb++) {
      unsigned currComb = comb; // current combination index
      unsigned downStreamCost = 0;
      unsigned downstreamViaCost = 0;
      int downstreamMinLayerNum = INT_MAX;
      int downstreamMaxLayerNum = INT_MIN;
       
      // Iterate through all children for the current combination
      for (int i = 0; node.childCnt; i++) {
        int childIdx = node.children[i];
        int childLayerNum = currComb % numLayers;  // determine child layer
        downstreamMinLayerNum = min(downstreamMinLayerNum, childLayerNum);
        downstreamMaxLayerNum = max(downstreamMaxLayerNum, childLayerNum);
        currComb /= numLayers; // update combination index for next child
        // Accumulate downstream cost for this child
        downStreamCost += d_bestLayerCosts[childIdx * numLayers + childLayerNum];
      }
      
      downstreamMinLayerNum = min(downstreamMinLayerNum, minPinLayer);
      downstreamMaxLayerNum = max(downstreamMaxLayerNum, maxPinLayer);
      // Compute the number of vias (tune the via cost as needed)
      const unsigned numVias = max(layerNum, maxPinLayer) - min(layerNum, minPinLayer);

      // Compute upstream edge congestion cost
      unsigned congestionCost = 0;
      if (layerNum <= (VIA_ACCESS_LAYERNUM_DEVICE / 2 - 1)) { // Pin layer routing adjustment
        congestionCost += VIACOST_DEVICE * 8;
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


// Update the layer assignment for each net
// from top (parent) to bottom (child)
__global__
void layerAssignNodeCommit_kernel(
  NodeStruct* d_nodes,
  const unsigned* d_bestLayerCombs,
  const unsigned* d_bestLayerCosts,
  int xDim, int yDim, int numLayers,
  int nodeStartIdx, int nodeEndIdx,
  int depth)
{
  int tIdx = threadIdx.x + blockIdx.x * blockDim.x;
  int nodeId = tIdx + nodeStartIdx;
  if (nodeId >= nodeEndIdx) {
    return;
  }

  NodeStruct& node = d_nodes[nodeId];
  if (node.level != depth) {
    return;
  }

  if (node.parentIdx == -1) { // root node
    frLayerNum minCostLayerNum = 0;
    unsigned minCost = UINT_MAX;
    for (frLayerNum layerNum = 0; layerNum < numLayers; layerNum++) {
      if (d_bestLayerCosts[nodeId * numLayers + layerNum]  < minCost) {
        minCostLayerNum = layerNum;
        minCost = d_bestLayerCosts[nodeId * numLayers + layerNum];
      }
    }

    node.layerNum = minCostLayerNum;
  }

  // Update the layer assignment for each child
  int currLayerNum = node.layerNum;
  int comb = d_bestLayerCombs[nodeId * numLayers + currLayerNum];
  // Iterate through all children for the current combination
  for (int i = 0; node.childCnt; i++) {
    int childIdx = node.children[i];
    d_nodes[childIdx].layerNum = comb % numLayers; // determine child layer
    comb /= numLayers; // update combination index for next child
  }
}
    

// update the congestion map
// We use atomicAdd to update the congestion map
// update all the children segment
__global__
void layerAssignSegmentCommit__kernel(
  NodeStruct* d_nodes,
  uint64_t* d_cmap,
  bool* d_layerDir,
  int xDim, int yDim, int numLayers,
  int nodeStartIdx, int nodeEndIdx, 
  int depth)
{  
  int tIdx = threadIdx.x + blockIdx.x * blockDim.x;
  int nodeId = tIdx + nodeStartIdx;
  if (nodeId >= nodeEndIdx) {
    return;
  }

  NodeStruct& node = d_nodes[nodeId];
  if (node.level != depth) {
    return;
  }

  // update the demand for the current node and its children
  int parentX = node.x;
  int parentY = node.y;
  int numChild = node.childCnt; 
  for (int i = 0; node.childCnt; i++) {
    int childIdx = node.children[i];
    NodeStruct& childNode = d_nodes[childIdx];
    int childLayerNum = childNode.layerNum;
    int childX = childNode.x;
    int childY = childNode.y;
    // To be implemented 
    // addRawDemand__device(d_cmap, xDim, yDim, numLayers, parentX, parentY, childX, childY, childLayerNum);
  }
}



// Perform the node level parallelization for the update of the GR layer assignment
void FlexGR::layerAssign_node_compute_CUDA(
  std::vector<unsigned>& bestLayerCosts,
  std::vector<unsigned>& bestLayerCombs,
  std::vector<int>& netBatchNodePtr,
  std::vector<int>& netBatchMaxDepth,
  std::vector<NodeStruct>& nodes)
{    
  int xDim, yDim, zDim;
  cmap_->getDim(xDim, yDim, zDim);
  auto& h_costMap = cmap_->getBits();
  bool* layerDir = new bool[zDim];  // 0 for horizontal, 1 for vertical
  for (int layerNum = 0; layerNum < zDim; layerNum++) {
    auto dir = design_->getTech()->getLayer((layerNum + 1) * 2)->getDir();
    layerDir[layerNum] = (dir == dbTechLayerDir::HORIZONTAL) ? 0 : 1;
  }

  for (int layerNum = 0; layerNum < zDim; layerNum++) {
    std::cout << "Layer " << layerNum << " is " << (layerDir[layerNum] ? "vertical" : "horizontal") << std::endl;
  }

  unsigned VIACOST_DEVICE = router_cfg_->VIACOST;
  unsigned VIA_ACCESS_LAYERNUM_DEVICE = router_cfg_->VIA_ACCESS_LAYERNUM;
  unsigned BLOCKCOST_DEVICE = router_cfg_->BLOCKCOST;
  unsigned MARKERCOST_DEVICE = router_cfg_->MARKERCOST;

  std::cout << "[INFO] VIACOST_DEVICE: " << VIACOST_DEVICE << std::endl;
  std::cout << "[INFO] VIA_ACCESS_LAYERNUM_DEVICE: " << VIA_ACCESS_LAYERNUM_DEVICE << std::endl;
  std::cout << "[INFO] BLOCKCOST_DEVICE: " << BLOCKCOST_DEVICE << std::endl;
  std::cout << "[INFO] MARKERCOST_DEVICE: " << MARKERCOST_DEVICE << std::endl;

  // Allocate memory for the device
  unsigned* d_bestLayerCosts;
  unsigned* d_bestLayerCombs;
  NodeStruct* d_nodes;
  uint64_t* d_costMap;
  bool* d_layerDir;

  cudaMalloc(&d_bestLayerCosts, bestLayerCosts.size() * sizeof(unsigned));
  cudaMalloc(&d_bestLayerCombs, bestLayerCombs.size() * sizeof(unsigned));
  cudaMalloc(&d_nodes, nodes.size() * sizeof(NodeStruct));
  cudaMalloc(&d_costMap, h_costMap.size() * sizeof(uint64_t));
  cudaMalloc(&d_layerDir, zDim * sizeof(bool));

  cudaMemcpy(d_bestLayerCosts, bestLayerCosts.data(), bestLayerCosts.size() * sizeof(unsigned), cudaMemcpyHostToDevice);
  cudaMemcpy(d_bestLayerCombs, bestLayerCombs.data(), bestLayerCombs.size() * sizeof(unsigned), cudaMemcpyHostToDevice);
  cudaMemcpy(d_nodes, nodes.data(), nodes.size() * sizeof(NodeStruct), cudaMemcpyHostToDevice);
  cudaMemcpy(d_costMap, h_costMap.data(), h_costMap.size() * sizeof(uint64_t), cudaMemcpyHostToDevice);
  cudaMemcpy(d_layerDir, layerDir, zDim * sizeof(bool), cudaMemcpyHostToDevice);


  int numBatch = netBatchMaxDepth.size();
  // Launch the kernel
  for (int batchId = 0; batchId < numBatch; batchId++) {
    int maxDepth = netBatchMaxDepth[batchId];
    int nodeStartIdx = netBatchNodePtr[batchId];
    int nodeEndIdx = netBatchNodePtr[batchId + 1];
    int numNodes = nodeEndIdx - nodeStartIdx;
    int numThreads = 256;
    int numBlocks = (numNodes + numThreads - 1) / numThreads;
    
    // node-level compute
    for (int depth = maxDepth; depth >= 0; depth--) { 
      layerAssignNodeCompute__kernel<<<numBlocks, numThreads>>>(
        d_nodes,
        d_bestLayerCombs, 
        d_bestLayerCosts,
        d_costMap,
        d_layerDir,
        xDim, yDim, zDim, 
        nodeStartIdx, nodeEndIdx, depth,
        VIACOST_DEVICE, 
        VIA_ACCESS_LAYERNUM_DEVICE,
        BLOCKCOST_DEVICE, 
        MARKERCOST_DEVICE);
    }
    
    cudaDeviceSynchronize();  // Wait for the kernel to finish
    cudaCheckError();
  }

  // Copy the results back to the host
  cudaMemcpy(bestLayerCosts.data(), d_bestLayerCosts, bestLayerCosts.size() * sizeof(unsigned), cudaMemcpyDeviceToHost);
  cudaMemcpy(bestLayerCombs.data(), d_bestLayerCombs, bestLayerCombs.size() * sizeof(unsigned), cudaMemcpyDeviceToHost);

  // Free the memory
  cudaFree(d_bestLayerCosts);
  cudaFree(d_bestLayerCombs);
  cudaFree(d_nodes);
  cudaFree(d_costMap);
  cudaFree(d_layerDir);

  delete[] layerDir;
}

} // namespace drt
