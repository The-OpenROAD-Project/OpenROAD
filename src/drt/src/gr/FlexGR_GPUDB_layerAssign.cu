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


/*
__global__ void layerAssignNodeCompute__kernel(  
  NodeStruct* d_nodes,
  unsigned* d_bestLayerCombs,
  unsigned* d_bestLayerCosts,
  int nodeStartIdx, int nodeEndIdx, int depth,
  int xDim, int yDim, int numLayers,
  unsigned VIACOST, unsigned VIA_ACCESS_LAYERNUM,
  unsigned LA_PIN_LAYER_COST_FACTOR,
  int maxChild)
{
  extern __shared__ unsigned s_data[];
  int nodeId = nodeStartIdx + blockIdx.x;
  if (nodeId >= nodeEndIdx) return;

  // Shared memory pointers setup
  unsigned* s_childCosts = s_data;
  unsigned* s_fixedCostPerLayer = s_data + maxChild * numLayers;
  unsigned* s_reductionCosts = s_fixedCostPerLayer + numLayers;
  unsigned* s_reductionCombs = s_reductionCosts + blockDim.x;

  // Load node info to shared memory
  __shared__ int s_childCnt, s_minPinLayer, s_maxPinLayer, s_parentIdx, s_curLocX, s_curLocY;
  if (threadIdx.x == 0) {
    NodeStruct node = d_nodes[nodeId];
    s_childCnt = node.childCnt;
    s_minPinLayer = node.minLayerNum;
    s_maxPinLayer = node.maxLayerNum;
    s_parentIdx = node.parentIdx;
    s_curLocX = node.x;
    s_curLocY = node.y;
    
    // Handle invalid nodes immediately
    if (node.level != depth || s_childCnt > maxChild) {
      s_childCnt = -1; // Mark as invalid
    }
  }
  __syncthreads();

  // Skip invalid nodes
  if (s_childCnt == -1) return;
  if (s_childCnt == 0) {  // Leaf node special case
    if (threadIdx.x == 0) {
      for (int layerNum = 0; layerNum < numLayers; layerNum++) {
        unsigned numVias = max(layerNum, s_maxPinLayer) - min(layerNum, s_minPinLayer);
        unsigned viaCost = VIACOST * numVias;
        unsigned congestionCost = 0;
        if (layerNum <= (VIA_ACCESS_LAYERNUM / 2 - 1)) {
          congestionCost += VIACOST * LA_PIN_LAYER_COST_FACTOR;
        }
        if (s_parentIdx != -1) {
          congestionCost += 100; // Simplified alignment cost
        }
        d_bestLayerCosts[nodeId * numLayers + layerNum] = viaCost + congestionCost;
        d_bestLayerCombs[nodeId * numLayers + layerNum] = 0;
      }
    }
    return;
  }

  // Load child costs to shared memory
  if (threadIdx.x == 0) {
    NodeStruct node = d_nodes[nodeId];
    for (int i = 0; i < s_childCnt; i++) {
      int childIdx = node.children[i];
      for (int l = 0; l < numLayers; l++) {
        s_childCosts[i * numLayers + l] = d_bestLayerCosts[childIdx * numLayers + l];
      }
    }
  }
  __syncthreads();

  // Precompute fixed costs per layer
  if (threadIdx.x < numLayers) {
    int layerNum = threadIdx.x;
    unsigned numVias = max(layerNum, s_maxPinLayer) - min(layerNum, s_minPinLayer);
    unsigned viaCost = VIACOST * numVias;
    unsigned congestionCost = 0;
    
    if (layerNum <= (VIA_ACCESS_LAYERNUM / 2 - 1)) {
      congestionCost += VIACOST * LA_PIN_LAYER_COST_FACTOR;
    }
    if (s_parentIdx != -1) {
      congestionCost += 100; // Simplified alignment cost
    }
    s_fixedCostPerLayer[layerNum] = viaCost + congestionCost;
  }
  __syncthreads();

  // Process each layer combination
  for (int layerNum = 0; layerNum < numLayers; layerNum++) {
    unsigned fixedCost = s_fixedCostPerLayer[layerNum];
    unsigned long long numComb = 1;
    for (int i = 0; i < s_childCnt; i++) {
      numComb *= numLayers;
    }

    // Parallelize combination evaluation
    unsigned long long combPerThread = (numComb + blockDim.x - 1) / blockDim.x;
    unsigned long long start = threadIdx.x * combPerThread;
    unsigned long long end = min(start + combPerThread, numComb);

    unsigned myMinCost = UINT_MAX;
    unsigned myBestComb = 0;

    // Evaluate combinations in parallel
    for (unsigned long long comb = start; comb < end; comb++) {
      unsigned temp = comb;
      unsigned cost = 0;
      for (int i = 0; i < s_childCnt; i++) {
        int layer_i = temp % numLayers;
        temp /= numLayers;
        cost += s_childCosts[i * numLayers + layer_i];
      }
      if (cost < myMinCost) {
        myMinCost = cost;
        myBestComb = comb;
      }
    }

    // Parallel reduction for best combination
    s_reductionCosts[threadIdx.x] = myMinCost;
    s_reductionCombs[threadIdx.x] = myBestComb;
    __syncthreads();

    for (int stride = blockDim.x / 2; stride > 0; stride >>= 1) {
      if (threadIdx.x < stride) {
        if (s_reductionCosts[threadIdx.x + stride] < s_reductionCosts[threadIdx.x]) {
          s_reductionCosts[threadIdx.x] = s_reductionCosts[threadIdx.x + stride];
          s_reductionCombs[threadIdx.x] = s_reductionCombs[threadIdx.x + stride];
        }
      }
      __syncthreads();
    }

    // Store results
    if (threadIdx.x == 0) {
      d_bestLayerCosts[nodeId * numLayers + layerNum] = s_reductionCosts[0] + fixedCost;
      d_bestLayerCombs[nodeId * numLayers + layerNum] = s_reductionCombs[0];
    }
    __syncthreads();
  }
}
*/








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
          bool isLayerBlocked = d_layerDir[layerNum] == false; // false means horizontal
          congestionCost += getSegmentCostV__device(
            d_cmap, curLocX, min(curLocY, parentY), max(curLocY, parentY), layerNum,
            xDim, yDim, numLayers, isLayerBlocked, BLOCKCOST, MARKERCOST);
        } else if (curLocY == parentY) { // horizontal segment
          bool isLayerBlocked = d_layerDir[layerNum] == true; // true means vertical
          congestionCost += getSegmentCostH__device(
            d_cmap, min(curLocX, parentX), max(curLocX, parentX), curLocY, layerNum,
            xDim, yDim, numLayers, isLayerBlocked, BLOCKCOST, MARKERCOST);
        } else {
          printf("LANodeCompute Error Node %d: current node and parent node are not aligned collinearly\n", nodeId);
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



__global__
void layerAssignNodeCommit__kernel(  
  NodeStruct* d_nodes,
  unsigned* d_bestLayerCombs,
  unsigned* d_bestLayerCosts,
  int nodeStartIdx, int nodeEndIdx, int depth,
  uint64_t* d_cmap, int xDim, int yDim, int numLayers)
{
  int tIdx = blockIdx.x * blockDim.x + threadIdx.x;
  int nodeId = tIdx + nodeStartIdx;
  if (nodeId >= nodeEndIdx) { return;  } // Out of bounds 

  NodeStruct& node = d_nodes[nodeId];
  if (node.level != depth) { return; }
  
  if (node.level == 0) { // root node
    unsigned minCost = UINT_MAX;
    int bestLayerNum = -1;
    for (int l = 0; l < numLayers; l++) {
      unsigned cost = d_bestLayerCosts[nodeId * numLayers + l];
      if (cost < minCost) {
        minCost = cost;
        bestLayerNum = l;
      }
    }
    node.layerNum = bestLayerNum;
  }

  // Decode the child's layer from bestComb
  int numChild = node.childCnt;
  unsigned comb = d_bestLayerCombs[nodeId * numLayers + node.layerNum];
  for (int i = 0; i < numChild; i++) { 
    int myLayer = comb % numLayers;
    comb /= numLayers;
    int childId = node.children[i];
    d_nodes[childId].layerNum = myLayer; // Assign layer to child node  
    // update cmap
    int childX = d_nodes[childId].x;
    int childY = d_nodes[childId].y;
    int parentX = node.x;
    int parentY = node.y;
    if (childX == parentX) { // vertical segment
      addSegmentV__device(d_cmap, min(childY, parentY), max(childY, parentY), childX, myLayer,
        xDim, yDim, numLayers);
    } else if (childY == parentY) { // horizontal segment
      addSegmentH__device(d_cmap, min(childX, parentX), max(childX, parentX), childY, myLayer,
        xDim, yDim, numLayers);
    } else {
      printf("LANodeCommit Error Node %d: current node and parent node are not aligned collinearly\n", nodeId);
    }
  }
}


// To do: large-net with highest depth should be done in CPU mode
// instead of GPU mode
// It seems that we do not the batches and netBatchPtr in this function
void FlexGRGPUDB::layerAssign_CUDA(
  std::vector<frNet*>& sortedNets,
  std::vector<int>& batches,
  std::vector<int>& netBatchPtr,
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

  if (debugMode_) {
    for (auto& node : nodes) {
      // check if the node is aligned with its children 
      for (int i = 0; i < node.childCnt; i++) {
        auto& childNode = nodes[node.children[i]];
        if (node.x != childNode.x && node.y != childNode.y) {    
          std::cout << "[ERROR] FlexGRGPUDB::layerAssign_CUDA: "
                    << "Node " << node.nodeIdx
                    << " is not aligned with its child node "
                    << childNode.nodeIdx << ".\n";
          std::cout << "Number of children: " << static_cast<int>(node.childCnt) << "\n";
          // print all the child nodes
          std::cout << "Child nodes: ";
          for (int j = 0; j < node.childCnt; j++) {
            auto& child = nodes[node.children[j]];
            std::cout << child.nodeIdx << " (x: " << child.x
                      << ", y: " << child.y << ") ";
          }
          std::cout << "\n";
          std::cout << "node.x = " << node.x
                    << ", node.y = " << node.y
                    << ", childNode.x = " << childNode.x
                    << ", childNode.y = " << childNode.y << "\n";
          if (childNode.parentIdx == -1) {
            std::cout << "Child node is a root node, no parent to check.\n";
          } else {
            // Print parent node information
            auto parentNode = nodes[childNode.parentIdx];
            std::cout << "Parent node: "
                      << parentNode.nodeIdx << ", "
                      << "x = " << parentNode.x
                      << ", y = " << parentNode.y << "\n";
          }
          std::cout << "Exiting due to misalignment.\n";
          exit(1); 
        }
      }
      
      
      if (node.parentIdx == -1) {
        continue; // Skip root nodes
      }

      auto& parentNode = nodes[node.parentIdx];
      if (node.x != parentNode.x && node.y != parentNode.y) {
        std::cout << "[ERROR] FlexGRGPUDB::layerAssign_CUDA: "
                  << "Node " << node.nodeIdx
                  << " is not aligned with its parent node "
                  << parentNode.nodeIdx << ".\n";
        std::cout << "node.x = " << node.x
                  << ", node.y = " << node.y
                  << ", parentNode.x = " << parentNode.x
                  << ", parentNode.y = " << parentNode.y << "\n";
        exit(1); 
      }
    }
  }


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
   
    for (int depth = 0; depth < maxDepth; depth++) {
      layerAssignNodeCommit__kernel<<<numBlocks, numThreads>>>(
        d_nodes, d_bestLayerCombs, d_bestLayerCosts,
        nodeStartIdx, nodeEndIdx, depth, 
        d_cmap_bits_3D, xDim, yDim, zDim);
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
