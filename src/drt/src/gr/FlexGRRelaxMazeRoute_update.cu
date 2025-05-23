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
#include "FlexGR_util.h"
#include "gr/FlexGR.h"
#include "stt/SteinerTreeBuilder.h"
#include "gr/FlexGR_util_update.h"
#include "gr/FlexGRCMap.h"
#include "gr/FlexGR_util_update.h"

namespace drt {

// Device function for obtaining the cost for each grid
__device__
float getGridCost2D_device(
  uint64_t* d_cmap,
  int xDim,
  int yDim,
  int xIdx,
  int yIdx,
  frDirEnum dir,
  float congestionThresh,
  unsigned BLOCKCOST_DEVICE)
{
  int zIdx = 0;
  int zDim = 1; // 2D Map
  float cost = 0;
  auto supply = getRawSupply_device(d_cmap, xDim, yDim, zDim, xIdx, yIdx, zIdx, dir) * congestionThresh;
  auto demand = getRawDemand_device(d_cmap, xDim, yDim, zDim, xIdx, yIdx, zIdx, dir);
  
  // congestion cost
  float exp_val = exp(min(10.0f, static_cast<float>(supply) - demand));  
  // Calculate the factor safely
  float factor = 8.0f / (1.0f + exp_val); 
  // Compute congestion cost with safety limits
  float congCost = demand * (1.0f + factor) / (supply + 1.0f);
  cost += congCost;

  // blockage cost
  if (hasBlock_device(d_cmap, xDim, yDim, zDim, xIdx, yIdx, zIdx, dir)) {
    cost += BLOCKCOST_DEVICE * 100;
  }
   
  // overflow cost
  if (demand >= supply) {
    cost += 128;
  }

  // history cost
  cost += 4 * congCost * getHistoryCost_device(d_cmap, xDim, yDim, zDim, xIdx, yIdx, zIdx);    
  return cost;
}


// Device function for computing segment cost
__device__
float compute_segment_cost_device(
  uint64_t* d_cmap,
  int xDim,
  int yDim,
  int startIdx,
  int endIdx,
  int fixedIdx, 
  frDirEnum dir,
  float congestionThresh,
  unsigned BLOCKCOST_DEVICE)
{
  float cost = 0;
  int minIdx = min(startIdx, endIdx);
  int maxIdx = max(startIdx, endIdx);
  for (int idx = minIdx; idx <= maxIdx; idx++) {
    int xIdx = (dir == frDirEnum::E) ? idx : fixedIdx;
    int yIdx = (dir == frDirEnum::E) ? fixedIdx : idx;
    cost += getGridCost2D_device(d_cmap, xDim, yDim, xIdx, yIdx, dir, congestionThresh, BLOCKCOST_DEVICE);
  }
  return cost;
}


// define the device function for I-Shape kernel
__device__
float compute_I_shape_device(
  uint64_t* d_cmap,
  int xDim,
  int yDim,
  IntPair srcLoc,
  IntPair dstLoc,
  float congestionThresh,
  unsigned BLOCKCOST_DEVICE) 
{
  if (srcLoc.x() == dstLoc.x() && srcLoc.y() == dstLoc.y()) {
    return 0.0;
  } 
  
  if (srcLoc.x() == dstLoc.x()) { // vertical
    return compute_segment_cost_device(
      d_cmap, xDim, yDim, srcLoc.y(), dstLoc.y(), srcLoc.x(), frDirEnum::N, congestionThresh, BLOCKCOST_DEVICE);
  } 
  
  if (srcLoc.y() == dstLoc.y()) { // horizontal
    return compute_segment_cost_device(
      d_cmap, xDim, yDim, srcLoc.x(), dstLoc.x(), srcLoc.y(), frDirEnum::E, congestionThresh, BLOCKCOST_DEVICE);
  } 

  return 0;
}


// define the device function for L-Shape kernel
__device__ 
float compute_L_Shape_device(
  uint64_t* d_cmap,
  int xDim,
  int yDim,
  IntPair srcLoc,
  IntPair dstLoc,
  float congestionThresh,
  unsigned BLOCKCOST_DEVICE)
{
  if (srcLoc.x() == dstLoc.x() || srcLoc.y() == dstLoc.y()) {
    return compute_I_shape_device(d_cmap, xDim, yDim, srcLoc, dstLoc, congestionThresh, BLOCKCOST_DEVICE);
  }

  // Check the two possible L-Shape
  IntPair corner1 = {srcLoc.x(), dstLoc.y()};
  IntPair corner2 = {dstLoc.x(), srcLoc.y()};

  float cost1 = compute_segment_cost_device(d_cmap, xDim, yDim, srcLoc.y(), corner1.y(), srcLoc.x(), frDirEnum::N, congestionThresh, BLOCKCOST_DEVICE) 
    + compute_segment_cost_device(d_cmap, xDim, yDim, corner1.x(), dstLoc.x(), corner1.y(), frDirEnum::E, congestionThresh, BLOCKCOST_DEVICE);  

  float cost2 = compute_segment_cost_device(d_cmap, xDim, yDim, srcLoc.x(), corner2.x(), srcLoc.y(), frDirEnum::E, congestionThresh, BLOCKCOST_DEVICE) 
    + compute_segment_cost_device(d_cmap, xDim, yDim, corner2.y(), dstLoc.y(), corner2.x(), frDirEnum::N, congestionThresh, BLOCKCOST_DEVICE);
  
  return std::min(cost1, cost2);
}


__device__
float compute_Z_Shape_util_device(
  uint64_t* d_cmap,
  int xDim,
  int yDim,
  IntPair srcLoc,
  IntPair dstLoc,
  IntPair corner1, 
  IntPair corner2,
  float  congestionThresh,
  unsigned BLOCKCOST_DEVICE)
{
  // I-Shape
  if (srcLoc.x() == dstLoc.x() || srcLoc.y() == dstLoc.y()) {
    return compute_I_shape_device(d_cmap, xDim, yDim, srcLoc, dstLoc, congestionThresh, BLOCKCOST_DEVICE);
  }

  // L-Shape
  if (corner1.x() == corner2.x() && corner1.y() == corner2.y()) {
    if (corner1.x() == srcLoc.x()) { // vertical first
      float cost = 0.0;
      cost += compute_segment_cost_device(d_cmap, xDim, yDim, srcLoc.y(), corner1.y(), srcLoc.x(), frDirEnum::N, congestionThresh, BLOCKCOST_DEVICE);
      cost += compute_segment_cost_device(d_cmap, xDim, yDim, corner1.x(), dstLoc.x(), dstLoc.y(), frDirEnum::E, congestionThresh, BLOCKCOST_DEVICE);
      return cost;
    } else { // horizontal first
      float cost = 0.0;
      cost += compute_segment_cost_device(d_cmap, xDim, yDim, srcLoc.x(), corner1.x(), srcLoc.y(), frDirEnum::E, congestionThresh, BLOCKCOST_DEVICE);
      cost += compute_segment_cost_device(d_cmap, xDim, yDim, corner1.y(), dstLoc.y(), dstLoc.x(), frDirEnum::N, congestionThresh, BLOCKCOST_DEVICE);
      return cost;
    }
  }

  if (srcLoc.x() == corner1.x()) { // vertical first
    float cost = 0.0;
    cost += compute_segment_cost_device(d_cmap, xDim, yDim, srcLoc.y(), corner1.y(), srcLoc.x(), frDirEnum::N, congestionThresh, BLOCKCOST_DEVICE);
    cost += compute_segment_cost_device(d_cmap, xDim, yDim, corner1.x(), corner2.x(), corner1.y(), frDirEnum::E, congestionThresh, BLOCKCOST_DEVICE);
    cost += compute_segment_cost_device(d_cmap, xDim, yDim, corner2.y(), dstLoc.y(), dstLoc.x(), frDirEnum::N, congestionThresh, BLOCKCOST_DEVICE);
    return cost;
  } else { // horizontal first
    float cost = 0.0;
    cost += compute_segment_cost_device(d_cmap, xDim, yDim, srcLoc.x(), corner1.x(), srcLoc.y(), frDirEnum::E, congestionThresh, BLOCKCOST_DEVICE);
    cost += compute_segment_cost_device(d_cmap, xDim, yDim, corner1.y(), corner2.y(), corner1.x(), frDirEnum::N, congestionThresh, BLOCKCOST_DEVICE);
    cost += compute_segment_cost_device(d_cmap, xDim, yDim, corner2.x(), dstLoc.x(), dstLoc.y(), frDirEnum::E, congestionThresh, BLOCKCOST_DEVICE);
    return cost;
  }
 
  printf("Error_A !!!\n");
  return FLT_MAX;
}




__device__
IntPair compute_L_shape_corner_device(
  uint64_t* d_cmap,
  int xDim,
  int yDim,
  IntPair srcLoc,
  IntPair dstLoc,
  float congestionThresh,
  unsigned BLOCKCOST_DEVICE)
{
  if (srcLoc.x() == dstLoc.x() || srcLoc.y() == dstLoc.y()) {
    return IntPair(-1, -1);
  }

  // Check the two possible L-Shape
  IntPair corner1 = {srcLoc.x(), dstLoc.y()};
  IntPair corner2 = {dstLoc.x(), srcLoc.y()};

  float cost1 = compute_segment_cost_device(d_cmap, xDim, yDim, srcLoc.y(), corner1.y(), srcLoc.x(), frDirEnum::N, congestionThresh, BLOCKCOST_DEVICE) 
    + compute_segment_cost_device(d_cmap, xDim, yDim, corner1.x(), dstLoc.x(), corner1.y(), frDirEnum::E, congestionThresh, BLOCKCOST_DEVICE);  

  float cost2 = compute_segment_cost_device(d_cmap, xDim, yDim, srcLoc.x(), corner2.x(), srcLoc.y(), frDirEnum::E, congestionThresh, BLOCKCOST_DEVICE) 
    + compute_segment_cost_device(d_cmap, xDim, yDim, corner2.y(), dstLoc.y(), corner2.x(), frDirEnum::N, congestionThresh, BLOCKCOST_DEVICE);

  //printf("cost1 = %f, cost2 = %f\n, corner1 = (%d, %d), corner2 = (%d, %d), srcLoc = (%d, %d), dstLoc = (%d, %d)\n", 
  //  cost1, cost2, corner1.x(), corner1.y(), corner2.x(), corner2.y(), srcLoc.x(), srcLoc.y(), dstLoc.x(), dstLoc.y());

  if (cost1 < cost2) {
    return corner1;
  } else {
    return corner2;
  }
}


// Device function for commit the segment
__device__
void commit_segment_device(
  uint64_t* d_cmap,
  int xDim,
  int yDim,
  IntPair srcLoc,
  IntPair dstLoc)
{
  int zIdx = 0;
  int zDim = 1; // 2D Map
  int minX = min(srcLoc.start, dstLoc.start);
  int minY = min(srcLoc.end, dstLoc.end);
  int maxX = max(srcLoc.start, dstLoc.start);
  int maxY = max(srcLoc.end, dstLoc.end);
  
  if (minY == maxY) { // horizontal segment
    for (int xIdx = minX; xIdx < minY; xIdx++) {
      addRawDemand_device(d_cmap, xDim, yDim, zDim, xIdx, minY, zIdx, frDirEnum::E);
      addRawDemand_device(d_cmap, xDim, yDim, zDim, xIdx + 1, minY, zIdx, frDirEnum::E);
    }
  } else if (minX == maxX) { // vertical segment
    for (int yIdx = minY; yIdx < maxY; yIdx++) {
      addRawDemand_device(d_cmap, xDim, yDim, zDim, minX, yIdx, zIdx, frDirEnum::N);
      addRawDemand_device(d_cmap, xDim, yDim, zDim, minX, yIdx + 1, zIdx, frDirEnum::N);
    }
  } else {
    printf("Error ! current node and parent node are are not aligned collinearly in commit_segment_device\n");
  }
}



// Define unions for atomic operations
union FloatUInt {
  float f = FLT_MAX;
  unsigned int ui;
};

union CostComb {
  unsigned long long int uint64;
  struct {
    unsigned int costBits;
    unsigned int combIdx;
  } data;
};

// Atomic function to update best cost and combination index
__device__
void atomicMinCostComb(unsigned long long int* address, float cost, unsigned int combIdx) {
  CostComb old, assumed, desired;

  // Convert cost to its bit representation
  FloatUInt costBits;
  costBits.f = cost;
  desired.data.costBits = costBits.ui;
  desired.data.combIdx = combIdx;
  old.uint64 = *address;
  while (true) {
    assumed.uint64 = old.uint64;
    // Extract the assumed cost
    FloatUInt assumedCostBits;
    assumedCostBits.ui = assumed.data.costBits;
    float assumedCost = assumedCostBits.f;
    // If the current best cost is less than or equal, no need to update
    if (assumedCost <= cost) {
      break;
    }

    // Attempt to update atomically
    old.uint64 = atomicCAS(address, assumed.uint64, desired.uint64);
    // If the value hasn't changed, update was successful
    if (old.uint64 == assumed.uint64) {
      break;
    }
  }
}


// Define the device function for general Z-shape pattern routing
__device__
float compute_Z_Shape_device(
  uint64_t* d_cmap,
  int xDim,
  int yDim,
  IntPair srcLoc,
  IntPair dstLoc,
  float  congestionThresh,
  unsigned BLOCKCOST_DEVICE)
{
  if (srcLoc.x() == dstLoc.x() || srcLoc.y() == dstLoc.y()) {
    return compute_I_shape_device(d_cmap, xDim, yDim, srcLoc, dstLoc, congestionThresh, BLOCKCOST_DEVICE);
  }
   
  float cost = FLT_MAX;
  IntPair corner1, corner2;
  int lx = min(srcLoc.x(), dstLoc.x());
  int ux = max(srcLoc.x(), dstLoc.x());
  int ly = min(srcLoc.y(), dstLoc.y());
  int uy = max(srcLoc.y(), dstLoc.y());

  // horizontal first
  for (int x = lx + 1; x <= ux; x++) {
    corner1 = {x, srcLoc.y()};
    corner2 = {x, dstLoc.y()};
    float tempCost = compute_Z_Shape_util_device(
      d_cmap, xDim, yDim, srcLoc, dstLoc, corner1, corner2, congestionThresh, BLOCKCOST_DEVICE);
    if (tempCost < cost) {
      cost = tempCost;
    } 
  }

  // vertical first
  for (int y = ly + 1; y <= uy; y++) {
    corner1 = {srcLoc.x(), y};
    corner2 = {dstLoc.x(), y};
    float tempCost = compute_Z_Shape_util_device(
      d_cmap, xDim, yDim, srcLoc, dstLoc, corner1, corner2, congestionThresh, BLOCKCOST_DEVICE);
    if (tempCost < cost) {
      cost = tempCost;
    } 
  }

  if (cost == FLT_MAX) {
    printf("Error_B !!! Cost is FLT_MAX !!!\n");
  }

  return cost;
}



__device__
IntPair compute_Z_shape_corner_device(
  uint64_t* d_cmap,
  int xDim,
  int yDim,
  IntPair srcLoc,
  IntPair dstLoc,
  float congestionThresh,
  unsigned BLOCKCOST_DEVICE)
{
  if (srcLoc.x() == dstLoc.x() || srcLoc.y() == dstLoc.y()) {
    return IntPair(-1, -1);
  }

  float bestCost = FLT_MAX;
  IntPair bestCorner = IntPair(-1, -1);
  
  IntPair corner1, corner2;
  int lx = min(srcLoc.x(), dstLoc.x());
  int ux = max(srcLoc.x(), dstLoc.x());
  int ly = min(srcLoc.y(), dstLoc.y());
  int uy = max(srcLoc.y(), dstLoc.y());

  // horizontal first
  for (int x = lx + 1; x <= ux; x++) {
    corner1 = {x, srcLoc.y()};
    corner2 = {x, dstLoc.y()};
    float cost = compute_Z_Shape_util_device(
      d_cmap, xDim, yDim, srcLoc, dstLoc, corner1, corner2, congestionThresh, BLOCKCOST_DEVICE);
    if (cost < bestCost) {
      bestCost = cost;
      bestCorner = corner1;
    } 
  }

  // vertical first
  for (int y = ly + 1; y <= uy; y++) {
    corner1 = {srcLoc.x(), y};
    corner2 = {dstLoc.x(), y};
    float cost = compute_Z_Shape_util_device(
      d_cmap, xDim, yDim, srcLoc, dstLoc, corner1, corner2, congestionThresh, BLOCKCOST_DEVICE);
    if (cost < bestCost) {
      bestCost = cost;
      bestCorner = corner1;
    } 
  }

  return bestCorner;
}


__global__ 
void steiner_loc_process_combinations_kernel(
    float* d_nodeLocBestCost,
    unsigned* d_nodeLocBestComb,
    uint64_t* d_cmap,
    const IntPair* d_nodeLoc,
    const IntPair* d_nodeLocPtr,
    const int* d_nodeEdgeIdx,
    const int* d_nodeLevel,
    int nodeId,
    int depth,
    int nodeLocStartIdx,
    int numLocs,
    int numComb,
    int childStartIdx,
    int childEndIdx,
    int maxNumLocs,
    int xDim,
    int yDim,
    float congestionThresh,
    unsigned BLOCKCOST_DEVICE)
{
    extern __shared__ char sharedMemory[];
    IntPair* sharedLoc = (IntPair*)sharedMemory;
    // Ensure proper alignment for unsigned long long int
    size_t offset = sizeof(IntPair);
    size_t padding = (8 - (offset % 8)) % 8;
    unsigned long long int* sharedBestCostComb = (unsigned long long int*)(sharedMemory + offset + padding);
    unsigned long long int& bestCostComb = sharedBestCostComb[0];

    int locIdx = blockIdx.x; // Parent location index
    // Calculate thread's unique index
    int threadId = threadIdx.x + threadIdx.y * blockDim.x;
    int totalThreads = blockDim.x * blockDim.y;
    bool active = true;
    if (locIdx >= numLocs) { active = false; }

    // Load parent location into shared memory
    int nodeLocIdx = nodeLocStartIdx + locIdx;
    if (threadId == 0) {
      sharedLoc[0] = d_nodeLoc[nodeLocIdx];
      FloatUInt costBits;
      costBits.f = FLT_MAX;
      bestCostComb = ((unsigned long long int)(UINT_MAX) << 32) | costBits.ui;
    }
    __syncthreads();



    // Process combinations in a loop
    for (int combIdx = threadId; combIdx < numComb; combIdx += totalThreads) {
      if (!active || combIdx >= numComb) { continue; }
      const auto& parentLoc = sharedLoc[0];
      int numChildren = childEndIdx - childStartIdx;
      unsigned currComb = combIdx;
      float cost = 0.0f;
      bool valid = true;
      int validError = -1;

      // Compute cost for this combination
      for (int childIdx = childStartIdx; childIdx < childEndIdx; childIdx++) {
        int childNodeIdx = d_nodeEdgeIdx[childIdx];    
        int numChildLocs = d_nodeLocPtr[childNodeIdx].end - d_nodeLocPtr[childNodeIdx].start;
        if (numChildLocs <= 0) {
          valid = false;
          validError = 1;
          break;
        }
 
        int locOffset = currComb % maxNumLocs;
        currComb /= maxNumLocs;

        int childLocIdx = d_nodeLocPtr[childNodeIdx].start + locOffset;
        // Bounds check (optional now, but can keep for safety)
        if (childLocIdx >= d_nodeLocPtr[childNodeIdx].end) {
          valid = false;
          validError = 2;
          break;
        }
      
        float childCost = d_nodeLocBestCost[childLocIdx];
        if (childCost == FLT_MAX) {
          validError = 3;
          // check the child information
          for (int idx = d_nodeLocPtr[childNodeIdx].start; idx < d_nodeLocPtr[childNodeIdx].end; idx++) {
            printf("childLocIdx = %d, childCost = %f, childNodeIdx = %d, childDepth = %d, locIdx = %d, nodeId = %d, depth = %d, childLoc.x = %d, childLoc.y = %d\n", 
              idx, d_nodeLocBestCost[idx], childNodeIdx, d_nodeLevel[childNodeIdx], locIdx, nodeId, depth,
              d_nodeLoc[idx].x(), d_nodeLoc[idx].y());
          }
          printf("Child cost is FLT_MAX !!! locIdx = %d, nodeId = %d, depth = %d, comIdx = %d\n", 
            locIdx, nodeId, depth, combIdx);
          valid = false;
          break;
        } else {
          cost += childCost;
        }

        //float L_cost = compute_L_Shape_device(
        //  d_cmap, xDim, yDim, parentLoc, d_nodeLoc[childLocIdx], congestionThresh, BLOCKCOST_DEVICE);
        
        float L_cost = compute_Z_Shape_device(
            d_cmap, xDim, yDim, parentLoc, d_nodeLoc[childLocIdx], congestionThresh, BLOCKCOST_DEVICE);

        if (L_cost == FLT_MAX) {
          validError = 4;
          printf("L_cost is FLT_MAX !!! locIdx = %d, nodeId = %d, depth = %d, comIdx = %d\n", 
            locIdx, nodeId, depth, combIdx);
          valid = false;
          break;
        } else {
          cost += L_cost;
        }
      }

      //if (nodeId == 8613 || nodeId == 7495) {
      //  printf("nodeId = %d, valid = %d, validError = %d, cost = %f, locIdx = %d, CombIdx = %d, numComb = %d, numChildren = %d, childStartIdx = %d, childEndIdx = %d\n", 
      //    nodeId, valid, validError,
      //    cost, locIdx, combIdx, numComb, numChildren, childStartIdx, childEndIdx);
      //} 

      if (valid) {
        atomicMinCostComb(&bestCostComb, cost, combIdx);
        if (cost == FLT_MAX) {
          printf("Cost is FLT_MAX !!! locIdx = %d, nodeId = %d, depth = %d, comIdx = %d, numComb = %d\n", 
            locIdx, nodeId, depth, combIdx, numComb);
        }
      }          
    }
    __syncthreads();

    // Write final results back to global memory
    if (threadId == 0) {
      // Extract cost and combIdx from bestCostComb
      CostComb result;
      result.uint64 = bestCostComb;
      FloatUInt costBits;
      costBits.ui = result.data.costBits;
      float bestCost = costBits.f;
      unsigned int bestComb = result.data.combIdx;
      d_nodeLocBestCost[nodeLocIdx] = bestCost;
      d_nodeLocBestComb[nodeLocIdx] = bestComb;
    }

    __syncthreads();

    if (d_nodeLocBestCost[nodeLocIdx] == FLT_MAX) {
      printf("BestCost is FLT_MAX !!! locIdx = %d, bestComb = %d, depth = %d, nodeId = %d, numComb = %d\n", 
        locIdx, d_nodeLocBestComb[nodeLocIdx], depth, nodeId, numComb);
    }
}


/*
__global__
void steiner_loc_process_combinations_kernel(
  float* d_nodeLocBestCost,
  unsigned* d_nodeLocBestComb,
  uint64_t* d_cmap,
  const IntPair* d_nodeLoc,
  const IntPair* d_nodeLocPtr,
  const int* d_nodeEdgeIdx,
  const int* d_nodeLevel,
  int nodeId, // just for debugging
  int depth,
  int nodeLocStartIdx,
  int numLocs,
  int numComb,
  int childStartIdx,
  int childEndIdx,
  int maxNumLocs,
  int xDim,
  int yDim,
  float congestionThresh,
  unsigned BLOCKCOST_DEVICE)
{
  extern __shared__ char sharedMemory[];
  IntPair* sharedLoc = (IntPair*)sharedMemory;
  size_t offset = sizeof(IntPair);
  size_t padding = (8 - (offset % 8)) % 8;
  unsigned long long int* sharedBestCostComb = (unsigned long long int*)(sharedMemory + offset + padding);
  unsigned long long int& bestCostComb = sharedBestCostComb[0];

  int locIdx = blockIdx.x; // Parent location index
  int totalThreadsPerBlock = blockDim.x * blockDim.y;
  int combIdx = threadIdx.x + threadIdx.y * blockDim.x; // Combination index
  bool active = true;
  if (locIdx >= numLocs || combIdx >= numComb) { active = false; }

  // Load parent location into shared memory (1 location per block)
  int nodeLocIdx = nodeLocStartIdx + locIdx;
  if (threadIdx.x == 0 && threadIdx.y == 0) {
    sharedLoc[0] = d_nodeLoc[nodeLocIdx];
    FloatUInt costBits;
    costBits.f = FLT_MAX;
    bestCostComb = ((unsigned long long int)(UINT_MAX) << 32) | costBits.ui;
  }  
  __syncthreads();

  if (active) {
    const auto& parentLoc = sharedLoc[0];
    int numChildren = childEndIdx - childStartIdx;
    unsigned currComb = combIdx;
    float cost = 0.0f;
    bool valid = true;

    // Compute cost for this combination
    for (int childIdx = childStartIdx; childIdx < childEndIdx; childIdx++) {
      int childNodeIdx = d_nodeEdgeIdx[childIdx];    
      int numChildLocs = d_nodeLocPtr[childNodeIdx].end - d_nodeLocPtr[childNodeIdx].start;
      if (numChildLocs <= 0) {
        valid = false;
        break;
      }
 
      int locOffset = currComb % maxNumLocs;
      currComb /= maxNumLocs;
    
      int childLocIdx = d_nodeLocPtr[childNodeIdx].start + locOffset;
      // Bounds check (optional now, but can keep for safety)
      if (childLocIdx >= d_nodeLocPtr[childNodeIdx].end) {
        valid = false;
        break;
      }
      
      float childCost = d_nodeLocBestCost[childLocIdx];
      if (childCost == FLT_MAX) {
        // check the child information
        for (int idx = d_nodeLocPtr[childNodeIdx].start; idx < d_nodeLocPtr[childNodeIdx].end; idx++) {
          printf("childLocIdx = %d, childCost = %f, childNodeIdx = %d, childDepth = %d, locIdx = %d, nodeId = %d, depth = %d, childLoc.x = %d, childLoc.y = %d\n", 
            idx, d_nodeLocBestCost[idx], childNodeIdx, d_nodeLevel[childNodeIdx], locIdx, nodeId, depth,
            d_nodeLoc[idx].x(), d_nodeLoc[idx].y());
        }
        printf("Child cost is FLT_MAX !!! locIdx = %d, nodeId = %d, depth = %d, comIdx = %d\n", 
          locIdx, nodeId, depth, combIdx);
        valid = false;
        break;
      } else {
        cost += childCost;
      }

      float L_cost = compute_L_Shape_device(
        d_cmap, xDim, yDim, parentLoc, d_nodeLoc[childLocIdx], congestionThresh, BLOCKCOST_DEVICE);

      if (L_cost == FLT_MAX) {
        printf("L_cost is FLT_MAX !!! locIdx = %d, nodeId = %d, depth = %d, comIdx = %d\n", 
          locIdx, nodeId, depth, combIdx);
        valid = false;
        break;
      } else {
        cost += L_cost;
      }
    }
  
    if (valid) {
      atomicMinCostComb(&bestCostComb, cost, combIdx);
    }  
  }

  __syncthreads();
  // Write final results back to global memory
  if (threadIdx.x == 0 && threadIdx.y == 0) {
    // Extract cost and combIdx from bestCostComb
    CostComb result;
    result.uint64 = bestCostComb;
    FloatUInt costBits;
    costBits.ui = result.data.costBits;
    float bestCost = costBits.f;
    unsigned int bestComb = result.data.combIdx;
    d_nodeLocBestCost[nodeLocIdx] = bestCost;
    d_nodeLocBestComb[nodeLocIdx] = bestComb;
   
    //if (bestCost == FLT_MAX) {
    //  printf("BestCost is FLT_MAX !!! locIdx = %d, bestComb = %d, depth = %d, nodeId = %d\n", 
    //    locIdx, bestComb, depth, nodeId);
    //}
  }

  __syncthreads();

  if (d_nodeLocBestCost[nodeLocIdx] == FLT_MAX) {
    printf("BestCost is FLT_MAX !!! locIdx = %d, bestComb = %d, depth = %d, nodeId = %d\n", 
      locIdx, d_nodeLocBestComb[nodeLocIdx], depth, nodeId);
  }
}
*/

__global__ 
void steiner_node_compute_update_kernel(
  float* d_nodeLocBestCost,
  unsigned* d_nodeLocBestComb,
  uint64_t* d_cmap, // congestion map
  const int* d_netBatch,
  const int* d_nodeCntPtr, // store the connection of the net
  const int* d_nodeLevel,
  const IntPair* d_nodeLoc,
  const IntPair* d_nodeLocPtr,
  const int* d_nodeEdgeIdx, // store the relationship between the nodes
  const IntPair* d_nodeEdgePtr,
  int xDim,
  int yDim,
  int maxNumNodes,
  int maxNumLocs, // maximum number of locations for each node
  int batchStartIdx,
  int batchEndIdx,
  int depth,
  float congestionThresh,
  unsigned BLOCKCOST_DEVICE)
{
  //int tIdx = threadIdx.x + blockIdx.x * blockDim.x;
  int threadId = threadIdx.x + threadIdx.y * blockDim.x;
  int tIdx = blockIdx.x * (blockDim.x * blockDim.y) + threadId;
  //int tIdx = threadIdx.x + threadIdx.y * blockDim.x;
  int netIdx = tIdx / maxNumNodes + batchStartIdx;
  if (netIdx >= batchEndIdx)  { return; }

  int netId = d_netBatch[netIdx];
  int nodeId = tIdx % maxNumNodes + d_nodeCntPtr[netId];  
  if (nodeId >= d_nodeCntPtr[netId + 1]) { return; }

  int nodeLevel = d_nodeLevel[nodeId];
  if (nodeLevel != depth) { return; }
  
  int childStartIdx = d_nodeEdgePtr[nodeId].start;
  int childEndIdx = d_nodeEdgePtr[nodeId].end; 

  int nodeLocStartIdx = d_nodeLocPtr[nodeId].start;
  int nodeLocEndIdx = d_nodeLocPtr[nodeId].end;
  int numLocs = nodeLocEndIdx - nodeLocStartIdx;  
  int numChildren = childEndIdx - childStartIdx;

  if (numChildren == 0) {
    d_nodeLocBestCost[nodeLocStartIdx] = 0.0f;
    d_nodeLocBestComb[nodeLocStartIdx] = 0;
    return;
  }

  int numComb = 1;
  for (int i = 0; i < numChildren; i++) {
    numComb *= maxNumLocs;
  }

  //dim3 blockDim(256); // Threads per block
  //dim3 gridDim(numLocs, (numComb + blockDim.x - 1) / blockDim.x);
  //dim3 blockDim(32, (numComb + 32 - 1) / 32);
  int blockDimY = min(32, (numComb + 32 - 1) / 32);
  dim3 blockDim(32, blockDimY);
  dim3 gridDim(numLocs, 1);
 
  //printf("numLocs = %d, numComb = %d  "
  //       "blockDim = %d  %d  "
  //       "gridDim = %d  %d\n", 
  //       numLocs, numComb, blockDim.x, blockDim.y, gridDim.x, gridDim.y);
  
  size_t offset = sizeof(IntPair);
  size_t padding = (8 - (offset % 8)) % 8;
  size_t sharedMemSize = offset + padding + sizeof(unsigned long long int);
  // Launch the kernel for loc level computation
  steiner_loc_process_combinations_kernel<<<gridDim, blockDim, sharedMemSize>>>(
    d_nodeLocBestCost,
    d_nodeLocBestComb,
    d_cmap,
    d_nodeLoc,
    d_nodeLocPtr,
    d_nodeEdgeIdx,
    d_nodeLevel,
    nodeId,
    depth,
    nodeLocStartIdx,
    numLocs,
    numComb,
    childStartIdx,
    childEndIdx,
    maxNumLocs,
    xDim,
    yDim,
    congestionThresh,
    BLOCKCOST_DEVICE);

  // cudaDeviceSynchronize();
  // Check for errors after kernel launch
  cudaError_t cudaStatus = cudaGetLastError();
  if (cudaStatus != cudaSuccess) {
    printf("Error in kernel launch (steiner_node_compute_update_kernel (test)): %s\n", cudaGetErrorString(cudaStatus));
  }
}


// Determine the optimal location for the each steiner node
// from level 0 to the last level
__global__ 
void steiner_node_commit_update_kernel(
  float* d_nodeLocBestCost,
  unsigned* d_nodeLocBestComb,
  int* d_bestOptimalLoc, // store the loc id
  const int* d_netBatch,
  const int* d_nodeCntPtr, // store the connection of the net
  const int* d_nodeLevel,
  const IntPair* d_nodeLoc,
  const IntPair* d_nodeLocPtr,
  const int* d_nodeEdgeIdx, // store the relationship between the nodes
  const IntPair* d_nodeEdgePtr,
  const int* d_nodeParentIdx,
  int maxNumNodes,
  int maxNumLocs,
  int batchStartIdx,
  int batchEndIdx,
  int depth)
{
  //int tIdx = threadIdx.x + blockIdx.x * blockDim.x;
  //int tIdx = threadIdx.x + threadIdx.y * blockDim.x;
  int threadId = threadIdx.x + threadIdx.y * blockDim.x;
  int tIdx = blockIdx.x * (blockDim.x * blockDim.y) + threadId;
  int netIdx = tIdx / maxNumNodes + batchStartIdx;

  // Check if the thread is out of bound
  if (netIdx >= batchEndIdx) return;

  int netId = d_netBatch[netIdx];
  int nodeId = tIdx % maxNumNodes + d_nodeCntPtr[netId];  
  if (nodeId >= d_nodeCntPtr[netId + 1]) return;

  int nodeLevel = d_nodeLevel[nodeId];
  if (nodeLevel != depth) return;

  int nodeLocStartIdx = d_nodeLocPtr[nodeId].start;
  int nodeLocEndIdx = d_nodeLocPtr[nodeId].end;
  int numLocs = nodeLocEndIdx - nodeLocStartIdx;

  int parentIdx = d_nodeParentIdx[nodeId];
  if (parentIdx == -1) { // root node
    int bestLocId = -1;
    float bestCost = FLT_MAX;
    for (int idx = nodeLocStartIdx; idx < nodeLocEndIdx; idx++) {
      if (d_nodeLocBestCost[idx] < bestCost) {
        bestCost = d_nodeLocBestCost[idx];
        bestLocId = idx;
      }
    }
    d_bestOptimalLoc[nodeId] = bestLocId;
  }  
  
  // update the location of the child nodes
  int childStartIdx = d_nodeEdgePtr[nodeId].start;
  int childEndIdx = d_nodeEdgePtr[nodeId].end; 
  int comb = d_nodeLocBestComb[d_bestOptimalLoc[nodeId]];
  //if (comb > 0) { 
  //  printf("nodeId = %d, bestLocId = %d, comb = %d\n", nodeId, d_bestOptimalLoc[nodeId], comb);
  //}

  for (int childIdx = childStartIdx; childIdx < childEndIdx; childIdx++) {
    int childNodeIdx = d_nodeEdgeIdx[childIdx];
    d_bestOptimalLoc[childNodeIdx] = comb % maxNumLocs + d_nodeLocPtr[childNodeIdx].start;
    comb /= maxNumLocs;
  }

  cudaError_t cudaStatus = cudaGetLastError();
  if (cudaStatus != cudaSuccess) {
    printf("Error in kernel launch (steiner_node_commit_update_kernel): %s\n", cudaGetErrorString(cudaStatus));
  }
}
    

// Compute the corner location for the nodes and commit the segment
// From level 0 to the last level
__global__ 
void corner_node_update_kernel(
  IntPair* d_cornerLoc,
  int* d_bestOptimalLoc, // store the loc id
  uint64_t* d_cmap, // congestion map
  const int* d_netBatch,
  const int* d_nodeCntPtr, // store the connection of the net
  const int* d_nodeLevel,
  const IntPair* d_nodeLoc,
  const IntPair* d_nodeLocPtr,
  const int* d_nodeEdgeIdx, // store the relationship between the nodes
  const IntPair* d_nodeEdgePtr,
  int xDim,
  int yDim,
  int maxNumNodes,
  int batchStartIdx,
  int batchEndIdx,
  int depth,
  float congestionThresh,
  unsigned BLOCKCOST_DEVICE)
{
  //int tIdx = threadIdx.x + blockIdx.x * blockDim.x;
  //int tIdx = threadIdx.x + threadIdx.y * blockDim.x;
  int threadId = threadIdx.x + threadIdx.y * blockDim.x;
  int tIdx = blockIdx.x * (blockDim.x * blockDim.y) + threadId;
  int netIdx = tIdx / maxNumNodes + batchStartIdx;

  // Check if the thread is out of bound
  if (netIdx >= batchEndIdx) return;
  int netId = d_netBatch[netIdx];
  
  int nodeId = tIdx % maxNumNodes + d_nodeCntPtr[netId];  
  if (nodeId >= d_nodeCntPtr[netId + 1]) return;

  int nodeLevel = d_nodeLevel[nodeId];
  if (nodeLevel != depth) return;
  
  int childStartIdx = d_nodeEdgePtr[nodeId].start;
  int childEndIdx = d_nodeEdgePtr[nodeId].end; 

  if (childEndIdx == childStartIdx) { // leaf node
    return;
  }

  auto& parentLoc = d_nodeLoc[d_bestOptimalLoc[nodeId]];
  // Determine the corner location for each child node
  for (int childIdx = childStartIdx; childIdx < childEndIdx; childIdx++) {
    int childNodeIdx = d_nodeEdgeIdx[childIdx];
    auto& childLoc = d_nodeLoc[d_bestOptimalLoc[childNodeIdx]];
    d_cornerLoc[childNodeIdx] = compute_L_shape_corner_device(d_cmap,
      xDim, yDim, parentLoc, childLoc, congestionThresh, BLOCKCOST_DEVICE);
    // Commit the segment
    if (d_cornerLoc[childNodeIdx].x() == -1 || d_cornerLoc[childNodeIdx].y() == -1) {
      commit_segment_device(d_cmap, xDim, yDim, parentLoc, childLoc);
    } else {
      if (childLoc.x() == d_cornerLoc[childNodeIdx].x() && 
          childLoc.y() == d_cornerLoc[childNodeIdx].y()) {
        printf("Error ! The corner location is the same as the child location\n");
      } else if (parentLoc.x() == d_cornerLoc[childNodeIdx].x() && 
                 parentLoc.y() == d_cornerLoc[childNodeIdx].y()) {
        printf("Error ! The corner location is the same as the parent location\n");
      }
      
      commit_segment_device(d_cmap, xDim, yDim, parentLoc, d_cornerLoc[childNodeIdx]);
      commit_segment_device(d_cmap, xDim, yDim, d_cornerLoc[childNodeIdx], childLoc);
    }
  }

  // Check for errors after kernel launch
  cudaError_t cudaStatus = cudaGetLastError();
  if (cudaStatus != cudaSuccess) {
    printf("Error in kernel launch (corner_node_update_kernel): %s\n", cudaGetErrorString(cudaStatus));
  }
}

// Use DAG-Based Approach for updating the Steiner Tree 
// Basic idea: We do not explicitly change the structure of the Steiner Tree.
// We assume the location of the Steiner nodes are not fixed.
// Then use greedy approach to determine the location of the Steiner nodes.
// Let's work on the 2D Grid Grpah
// Here we only need to consider the nets with congestion
void FlexGR::RRR_SteinerTreeShift(std::vector<frNet*>& nets2RR)
{
  logger_->report("[INFO][FlexGR] Start Steiner Tree Shift...");
  int gridXSize = xDim_;
  int gridYSize = yDim_;
  for (auto& net : nets2RR) {
    initGR_ripup_update(net);
  }

  int deviceCount = 0;
  cudaGetDeviceCount(&deviceCount);
  printf("Number of GPUs available: %d\n", deviceCount);
  cudaSetDevice(1); // Switch to GPU 1

  // Function for converting 2D index to 1d index  
  auto getIdx = [&](int x, int y) { return y * gridXSize + x; };
  std::vector<uint64_t> congestion_map;
  congestion_map.resize(gridXSize * gridYSize);
  for (int xIdx = 0; xIdx < gridXSize; xIdx++) {
    for (int yIdx = 0; yIdx < gridYSize; yIdx++) {
      congestion_map[getIdx(xIdx, yIdx)] = cmap2D_->getOriginalBits(xIdx, yIdx, 0);
    }
  }  

  std::cout << "test a" << std::endl;
  removeLoop(nets2RR);
  checkValidNet_update();

  //removeLoop(nets2RR);
  // Step 1:  Perform overlap detection to generate net batch
  std::vector<std::vector<int> > netBatch;
  batchGeneration_update(nets2RR, netBatch);  

  // Step 2: node levelization
  int totalNumNodes = 0;
  int maxNumNodes = 0; // Maximum number of nodes for each net
  int maxNumChildren = 0; // Maximum number of children for each node 
  int maxNumLocs = 0; // Maximum number of locations for each node
  std::vector<int> nodeCntPtrVec; // store the connection of the net
  std::vector<int> nodeLevel; // store the depth of the node
  std::vector<IntPair> locVec; // store the location of the node
  std::vector<IntPair> nodeLocPtr;
  std::vector<int> nodeParentIdx;  // store the parent and children relationship
  std::vector<int> nodeEdgeIdx;
  std::vector<IntPair> nodeEdgePtr;
  std::vector<int> netBatchMaxDepth;

  initGR_node_levelization_update(
    nets2RR, 
    netBatch, 
    gridXSize, gridYSize, 
    totalNumNodes, maxNumNodes, maxNumChildren, maxNumLocs,
    nodeCntPtrVec, 
    nodeLevel, 
    locVec, nodeLocPtr,
    nodeParentIdx, nodeEdgeIdx, nodeEdgePtr, 
    netBatchMaxDepth);

  // Define the block cost
  unsigned BLOCKCOST_DEVICE = BLOCKCOST;
  std::vector<float> nodeLocBestCost(locVec.size(), std::numeric_limits<float>::max());
  std::vector<unsigned> nodeLocBestComb(locVec.size(), std::numeric_limits<unsigned>::max());
  
  // translate the netBatch into 1D array
  std::vector<int> netBatch_1D;
  netBatch_1D.reserve(nets2RR.size());
  std::vector<int> netBatchPtr;
  netBatchPtr.reserve(netBatch.size() + 1);
  for (auto& netVec : netBatch) {
    netBatchPtr.push_back(netBatch_1D.size());
    netBatch_1D.insert(netBatch_1D.end(), 
                       std::make_move_iterator(netVec.begin()),
                       std::make_move_iterator(netVec.end()));
  }
  netBatchPtr.push_back(netBatch_1D.size());
  std::vector<int> bestOptimalLoc(totalNumNodes, -1);
  std::vector<IntPair> cornerLoc(totalNumNodes, {-1, -1}); // We use the L-Shape as the initial connection

  int* d_netBatch;
  uint64_t* d_congestion_map;  
  int* d_nodeCntPtr;  
  int* d_nodeLevel;
  IntPair* d_nodeLoc;
  IntPair* d_nodeLocPtr;
  int* d_nodeParentIdx;
  int* d_nodeEdgeIdx;
  IntPair* d_nodeEdgePtr;
  int* d_netBatchMaxDepth;
  float* d_nodeLocBestCost;
  unsigned* d_nodeLocBestComb;
  int* d_bestOptimalLoc;
  IntPair* d_cornerLoc;
  
  cudaMalloc(&d_netBatch, netBatch_1D.size() * sizeof(int));
  cudaMalloc(&d_congestion_map, congestion_map.size() * sizeof(uint64_t));
  cudaMalloc(&d_nodeCntPtr, nodeCntPtrVec.size() * sizeof(int));
  cudaMalloc(&d_nodeLevel, nodeLevel.size() * sizeof(int));
  cudaMalloc(&d_nodeLoc, locVec.size() * sizeof(IntPair));
  cudaMalloc(&d_nodeLocPtr, nodeLocPtr.size() * sizeof(IntPair));
  cudaMalloc(&d_nodeParentIdx, nodeParentIdx.size() * sizeof(int));
  cudaMalloc(&d_nodeEdgeIdx, nodeEdgeIdx.size() * sizeof(int));
  cudaMalloc(&d_nodeEdgePtr, nodeEdgePtr.size() * sizeof(IntPair));
  cudaMalloc(&d_netBatchMaxDepth, netBatchMaxDepth.size() * sizeof(int));
  cudaMalloc(&d_nodeLocBestCost, nodeLocBestCost.size() * sizeof(float));
  cudaMalloc(&d_nodeLocBestComb, nodeLocBestComb.size() * sizeof(unsigned));
  cudaMalloc(&d_bestOptimalLoc, bestOptimalLoc.size() * sizeof(int));
  cudaMalloc(&d_cornerLoc, cornerLoc.size() * sizeof(IntPair));

  cudaMemcpy(d_netBatch, netBatch_1D.data(), netBatch_1D.size() * sizeof(int), cudaMemcpyHostToDevice);
  cudaMemcpy(d_congestion_map, congestion_map.data(), congestion_map.size() * sizeof(uint64_t), cudaMemcpyHostToDevice);  
  cudaMemcpy(d_nodeCntPtr, nodeCntPtrVec.data(), nodeCntPtrVec.size() * sizeof(int), cudaMemcpyHostToDevice);
  cudaMemcpy(d_nodeLevel, nodeLevel.data(), nodeLevel.size() * sizeof(int), cudaMemcpyHostToDevice);
  cudaMemcpy(d_nodeLoc, locVec.data(), locVec.size() * sizeof(IntPair), cudaMemcpyHostToDevice);
  cudaMemcpy(d_nodeLocPtr, nodeLocPtr.data(), nodeLocPtr.size() * sizeof(IntPair), cudaMemcpyHostToDevice);
  cudaMemcpy(d_nodeParentIdx, nodeParentIdx.data(), nodeParentIdx.size() * sizeof(int), cudaMemcpyHostToDevice);
  cudaMemcpy(d_nodeEdgeIdx, nodeEdgeIdx.data(), nodeEdgeIdx.size() * sizeof(int), cudaMemcpyHostToDevice);
  cudaMemcpy(d_nodeEdgePtr, nodeEdgePtr.data(), nodeEdgePtr.size() * sizeof(IntPair), cudaMemcpyHostToDevice);
  cudaMemcpy(d_netBatchMaxDepth, netBatchMaxDepth.data(), netBatchMaxDepth.size() * sizeof(int), cudaMemcpyHostToDevice);
  cudaMemcpy(d_nodeLocBestCost, nodeLocBestCost.data(), nodeLocBestCost.size() * sizeof(float), cudaMemcpyHostToDevice);
  cudaMemcpy(d_nodeLocBestComb, nodeLocBestComb.data(), nodeLocBestComb.size() * sizeof(unsigned), cudaMemcpyHostToDevice);
  cudaMemcpy(d_bestOptimalLoc, bestOptimalLoc.data(), bestOptimalLoc.size() * sizeof(int), cudaMemcpyHostToDevice);
  cudaMemcpy(d_cornerLoc, cornerLoc.data(), cornerLoc.size() * sizeof(IntPair), cudaMemcpyHostToDevice);
 
  logger_->report("[INFO][FlexGR] Done CUDA memory initialization...\n");
  
  // Launch the kernel
  int numBatch = netBatch.size();
  int maxDepth = 0;
  std::cout << "Number of net batches: " << numBatch << std::endl;
  std::cout << "Launching the kernel..." << std::endl;

  for (int batchId = 0; batchId < numBatch; batchId++) {
    maxDepth = netBatchMaxDepth[batchId];
    //std::cout << "Batch id : " << batchId << "  " << "Max depth: " << maxDepth << std::endl;
    // Launch the kernel
    int startIdx = netBatchPtr[batchId];
    int endIdx = netBatchPtr[batchId + 1];
    int numNets = endIdx - startIdx;
    int numNodes = numNets * maxNumNodes; // total number of threads needed
    int numThreads = 64;
    int numBlocks = (numNodes + numThreads - 1) / numThreads;

    /* 
    int numSubBatches = (numThreads * numBlocks) / 1024 + 1;
    numSubBatches = std::min(numSubBatches, numNets > 0 ? numNets : 1); // Ensure numSubBatches <= numNets
    int subBatchSize = (numNets + numSubBatches - 1) / numSubBatches;
    subBatchSize = std::max(subBatchSize, 1); // Ensure subBatchSize >= 1

    int subBatchStartIdx = 0;
    int subBatchEndIdx = 0;
    for (int subBatchId = 0; subBatchId < numSubBatches; subBatchId++) {
      subBatchStartIdx = subBatchEndIdx;
      subBatchEndIdx = std::min(subBatchStartIdx + subBatchSize, numNets);
      int subBatchNumNets = subBatchEndIdx - subBatchStartIdx;
    
      if (subBatchNumNets == 0) {
        continue; // Skip empty sub-batches
      }
    
      int subBatchNumNodes = subBatchNumNets * maxNumNodes;
      int subBatchNumThreads = 64;
      int subBatchNumBlocks = (subBatchNumNodes + subBatchNumThreads - 1) / subBatchNumThreads;
      subBatchNumBlocks = std::max(subBatchNumBlocks, 1); // Ensure at l

      std::cout << "subBatchNumNets = " << subBatchNumNets << "  "
                << "subBatchNumNodes = " << subBatchNumNodes << "  "
                << "subBatchNumThreads = " << subBatchNumThreads << "  "
                << "subBatchNumBlocks = " << subBatchNumBlocks << std::endl;
     
      std::cout << "subBatchStartIdx = " << subBatchStartIdx + startIdx << "  "
                << "subBatchEndIdx = " << subBatchEndIdx  + startIdx << std::endl;

      int blockDimX = 32;
      int blockDimY = 2; // 32 * 2 = 64 threads per block
      dim3 blockDim(blockDimX, blockDimY);
                
      // Calculate total threads per block
      int threadsPerBlock = blockDimX * blockDimY; // Should match subBatchNumThreads (64)
      // Calculate grid dimensions
      int totalThreadsNeeded = subBatchNumNodes;
      int blocksNeeded = (totalThreadsNeeded + threadsPerBlock - 1) / threadsPerBlock;
      std::cout << "totalThreadsNeeded = " << totalThreadsNeeded << "  "
                << "threadsPerBlock = " << threadsPerBlock << "  "
                << "blocksNeeded = " << blocksNeeded << std::endl;      
      dim3 gridDim(blocksNeeded, 1);

      // node-level compute
      for (int depth = maxDepth; depth >= 0; depth--) {
        //std::cout << "depth = " << depth << std::endl;
        //std::cout << "depth = " << depth << "  "
        //          << "numNodes = " << numNodes << "  "
        //          << "numThreads = " << numThreads << "  "
        //          << "numBlocks = " << numBlocks << std::endl;

        steiner_node_compute_update_kernel<<<gridDim, blockDim>>>(
          d_nodeLocBestCost,
          d_nodeLocBestComb,
          d_congestion_map,
          d_netBatch,
          d_nodeCntPtr,
          d_nodeLevel,
          d_nodeLoc,
          d_nodeLocPtr,
          d_nodeEdgeIdx,
          d_nodeEdgePtr,
          gridXSize,
          gridYSize,
          maxNumNodes,
          maxNumLocs,
          startIdx + subBatchStartIdx,
          startIdx + subBatchEndIdx,
          depth,
          congestionThresh_,
          BLOCKCOST_DEVICE);
        //cudaDeviceSynchronize();
      }

      //cudaDeviceSynchronize();

      // add other kernels functions
      // perform the node-level commit
      for (int depth = 0; depth <= maxDepth; depth++) {
        steiner_node_commit_update_kernel<<<gridDim, blockDim>>>(
          d_nodeLocBestCost,
          d_nodeLocBestComb,
          d_bestOptimalLoc,
          d_netBatch,
          d_nodeCntPtr,
          d_nodeLevel,
          d_nodeLoc,
          d_nodeLocPtr,
          d_nodeEdgeIdx,
          d_nodeEdgePtr,
          d_nodeParentIdx,
          maxNumNodes,
          maxNumLocs,
          startIdx + subBatchStartIdx,
          startIdx + subBatchEndIdx,
          depth);
      }

      // update the corner location
      for (int depth = 0; depth <= maxDepth; depth++) {
        corner_node_update_kernel<<<gridDim, blockDim>>>(
          d_cornerLoc,
          d_bestOptimalLoc,
          d_congestion_map,
          d_netBatch,
          d_nodeCntPtr,
          d_nodeLevel,
          d_nodeLoc,
          d_nodeLocPtr,
          d_nodeEdgeIdx,
          d_nodeEdgePtr,
          gridXSize,
          gridYSize,
          maxNumNodes,
          startIdx + subBatchStartIdx,
          startIdx + subBatchEndIdx,
          depth,
          congestionThresh_,
          BLOCKCOST_DEVICE);
      }
    }
    */


  
    // node-level compute
    for (int depth = maxDepth; depth >= 0; depth--) {
      //std::cout << "depth = " << depth << std::endl;
      /*
      std::cout << "depth = " << depth << "  "
                << "numNodes = " << numNodes << "  "
                << "numThreads = " << numThreads << "  "
                << "numBlocks = " << numBlocks << std::endl;
      */
      steiner_node_compute_update_kernel<<<numBlocks, numThreads>>>(
        d_nodeLocBestCost,
        d_nodeLocBestComb,
        d_congestion_map,
        d_netBatch,
        d_nodeCntPtr,
        d_nodeLevel,
        d_nodeLoc,
        d_nodeLocPtr,
        d_nodeEdgeIdx,
        d_nodeEdgePtr,
        gridXSize,
        gridYSize,
        maxNumNodes,
        maxNumLocs,
        startIdx,
        endIdx,
        depth,
        congestionThresh_,
        BLOCKCOST_DEVICE);
      //cudaDeviceSynchronize();
    }

    //cudaDeviceSynchronize();

    // for debugging,  check the cost and combination
    //cudaMemcpy(nodeLocBestCost.data(), d_nodeLocBestCost, nodeLocBestCost.size() * sizeof(float), cudaMemcpyDeviceToHost);
    //for (int i = 0; i < 10; i++) {
    //  std::cout << "i = " << i << "cost: " << nodeLocBestCost[i] << std::endl;
    //}

    //std::cout << "finish node compute update kernel" << std::endl;

    // perform the node-level commit
    for (int depth = 0; depth <= maxDepth; depth++) {
      steiner_node_commit_update_kernel<<<numBlocks, numThreads>>>(
        d_nodeLocBestCost,
        d_nodeLocBestComb,
        d_bestOptimalLoc,
        d_netBatch,
        d_nodeCntPtr,
        d_nodeLevel,
        d_nodeLoc,
        d_nodeLocPtr,
        d_nodeEdgeIdx,
        d_nodeEdgePtr,
        d_nodeParentIdx,
        maxNumNodes,
        maxNumLocs,
        startIdx,
        endIdx,
        depth);
    }
  
    
    // for debugging,  check the cost and combination
    //cudaMemcpy(bestOptimalLoc.data(), d_bestOptimalLoc, bestOptimalLoc.size() * sizeof(int), cudaMemcpyDeviceToHost);
    //for (int i = 0; i < 10; i++) {
    //  std::cout << "i = " << i << "bestOptimalLoc: " << bestOptimalLoc[i] << std::endl;
    //}

    //std::cout << "finish node commit update kernel" << std::endl;

    for (int depth = 0; depth <= maxDepth; depth++) {
      corner_node_update_kernel<<<numBlocks, numThreads>>>(
        d_cornerLoc,
        d_bestOptimalLoc,
        d_congestion_map,
        d_netBatch,
        d_nodeCntPtr,
        d_nodeLevel,
        d_nodeLoc,
        d_nodeLocPtr,
        d_nodeEdgeIdx,
        d_nodeEdgePtr,
        gridXSize,
        gridYSize,
        maxNumNodes,
        startIdx,
        endIdx,
        depth,
        congestionThresh_,
        BLOCKCOST_DEVICE);
    }   
  }


  // for debugging,  check the cost and combination
  cudaMemcpy(nodeLocBestCost.data(), d_nodeLocBestCost, nodeLocBestCost.size() * sizeof(float), cudaMemcpyDeviceToHost);
  cudaMemcpy(nodeLocBestComb.data(), d_nodeLocBestComb, nodeLocBestComb.size() * sizeof(unsigned), cudaMemcpyDeviceToHost);
  cudaMemcpy(bestOptimalLoc.data(), d_bestOptimalLoc, bestOptimalLoc.size() * sizeof(int), cudaMemcpyDeviceToHost); 
  cudaMemcpy(cornerLoc.data(), d_cornerLoc, cornerLoc.size() * sizeof(IntPair), cudaMemcpyDeviceToHost);
  
  for (int i = 0; i < nodeLocBestCost.size(); i++) {
    if (nodeLocBestCost[i] == FLT_MAX) {
      std::cout << "Error ! nodeLocBestCost is FLT_MAX !!!" << "  "
                << "Bestcomb = " << nodeLocBestComb[i] << "  "
                << "i = " << i << std::endl;
    }
  }
  
  std::cout << "Finish the kernel launch" << std::endl;


  // update the location for the steiner node 
  std::function<void(frNode*, frNet*, int)> traverse_net_dfs_lambda = 
    [&](frNode* currNode, frNet* net, int baseIdx) {
    if (currNode->getType() != frNodeTypeEnum::frcSteiner) {
      return;
    }


    //if (currNode->isDontMove()) {
    //  
    //}

    int currNodeIdx = currNode->getIntProp();    
    Point loc(locVec[bestOptimalLoc[currNodeIdx]].x(), locVec[bestOptimalLoc[currNodeIdx]].y());
    Point realLoc = design_->getTopBlock()->getGCellCenter(loc);
     
    if (currNode->isDontMove()) {
      Point newLoc = currNode->getLoc();  
      Point gcellLoc = design_->getTopBlock()->getGCellIdx(newLoc);
      if (gcellLoc.x() != loc.x() || gcellLoc.y() != loc.y()) {
        std::cout << "Error ! The location is not the same as the optimal location\n";
        exit(1);
      }
    } else {
      currNode->setLoc(realLoc);
    }

    //currNode->setLoc(realLoc);
    // currNode->setLoc(design_->getTopBlock()->getGCellCenter(loc));
    Point newLoc = design_->getTopBlock()->getGCellIdx(currNode->getLoc());
    

    /*
    // check the parent loc
    if (currNode->getParent() != nullptr && currNode->getParent()->getType() == frNodeTypeEnum::frcSteiner) {
      auto parentLoc = currNode->getParent()->getLoc();
      auto parentGcellLoc = design_->getTopBlock()->getGCellIdx(parentLoc);
      if (parentGcellLoc.x() == newLoc.x() && parentGcellLoc.y() == newLoc.y()) {
        std::cout << "Error ! The parent loc is the same as the child loc\n";
      }
    }
    */

    
    // Traverse the children
    for (auto child : currNode->getChildren()) {
      traverse_net_dfs_lambda(child, net, baseIdx);
    } 
       
    // if (gcellLoc.x() != loc.x() || gcellLoc.y() != loc.y()) {
    //   std::cout << "Previous location: " << gcellLoc.x() << " " << gcellLoc.y() << "  "
    //            << "New location: " << loc.x() << " " << loc.y() << "  "
    //            << "node loc : " << newLoc.x() << " " << newLoc.y() << " "
    //            << "real loc : " << realLoc.x() << " " << realLoc.y() << " "
    //            << "nodeId=" << currNodeIdx << std::endl;
    //}
  };    

  auto printNetInfo = [&](frNet* net) {
    for (auto& node : net->getNodes()) {
      if (node->getType() != frNodeTypeEnum::frcSteiner) {
        continue;
      }

      auto parent = node->getParent();
      if (parent == nullptr || parent->getType() != frNodeTypeEnum::frcSteiner) {
        continue;
      }
  
      Point parentLoc = design_->getTopBlock()->getGCellIdx(parent->getLoc());
      Point nodeLoc = design_->getTopBlock()->getGCellIdx(node->getLoc());

      if (parentLoc.x() == nodeLoc.x() && parentLoc.y() == nodeLoc.y()) {
        std::cout << "netId = " << net->getId() << " "
                  << "nodeId = " << node->getIntProp() << " "
                  << "parentId = " << parent->getIntProp() << " "
                  << "Parent loc: " << parentLoc.x() << " " << parentLoc.y() << " "
                  << "Node loc: " << nodeLoc.x() << " " << nodeLoc.y() << " "
                  << "Error: two steiner nodes are at the same location\n";
        exit(1);
      }
    }
  };


  std::function<void(frNet*)> traverse_net_add_node_dfs_lambda = [&](frNet* net) {
    std::vector<frNode*> nodes;
    for (auto& node : net->getNodes()) {
      if (node->getType() != frNodeTypeEnum::frcSteiner) {
        continue;
      }

      auto parent = node->getParent();
      if (parent == nullptr || parent->getType() != frNodeTypeEnum::frcSteiner) {
        continue;
      }
      
      nodes.push_back(node.get());
    }
   
    /*
    // Print the node
    for (auto& node : nodes) {
      auto parentLoc = design_->getTopBlock()->getGCellIdx(node->getParent()->getLoc());
      auto childLoc = design_->getTopBlock()->getGCellIdx(node->getLoc());
      std::cout << "nodeId = " << node->getIntProp() << "  "
                << "Parent Node id: " << node->getParent()->getIntProp() << "  "
                << "Parent loc: " << parentLoc.x() << " " << parentLoc.y() << "  "
                << "Child loc: " << childLoc.x() << " " << childLoc.y() << std::endl;
    }
    */


    for (auto& node : nodes) {
      int nodeIdx = node->getIntProp();
      if (nodeIdx >= cornerLoc.size()) {
        std::cout << "Error ! nodeIdx is out of bound "
                  << "nodeIdx = " << nodeIdx << " corNocLoc.size() = " << cornerLoc.size() << std::endl;
        continue;
      }
      
      auto corner = cornerLoc[nodeIdx];
      if (corner.x() == -1 || corner.y() == -1) {
        continue;
      }
      
      Point parentLoc = design_->getTopBlock()->getGCellIdx(node->getParent()->getLoc());
      Point childLoc = design_->getTopBlock()->getGCellIdx(node->getLoc());


      /*
      std::cout << "New netId = " << net->getId() << "  "
                << "nodeId = " << node->getIntProp() << "  "
                << "Parent Node id: " << node->getParent()->getIntProp() << "  "
                << "parentLoc = " << parentLoc.x() << " " << parentLoc.y() << "  "
                << "childLoc = " << childLoc.x() << " " << childLoc.y() << "  "
                << "cornerLoc = " << corner.x() << " " << corner.y() << std::endl;
      */

      if (((parentLoc.x() != corner.x() && parentLoc.y() != corner.y()) ||  
         (childLoc.x() != corner.x() && childLoc.y() != corner.y()))) {
        std::cout << "Node id : " << nodeIdx << " "
                  << "Parent Node id: " << node->getParent()->getIntProp() << " "
                  << "Parent loc: " << parentLoc.x() << " " << parentLoc.y() << " "
                  << " Child loc: " << childLoc.x() << " " << childLoc.y() 
                  << " Corner loc: " << corner.x() << " " << corner.y() << std::endl;
      }

      if ((parentLoc.x() == corner.x() && parentLoc.y() == corner.y()) 
        || (childLoc.x() == corner.x() && childLoc.y() == corner.y())) {
        std::cout << "Node id : " << nodeIdx << " Parent loc: " << parentLoc.x() << " " << parentLoc.y() 
                  << " Child loc: " << childLoc.x() << " " << childLoc.y() 
                  << " Corner loc: " << corner.x() << " " << corner.y() << std::endl;
      }

      Point loc = design_->getTopBlock()->getGCellCenter(Point(corner.x(), corner.y()));            
      createCornerNode2D_update(net, node, node->getParent(), Point(corner.x(), corner.y()));

      /*
      // check the location
      Point loc1 = design_->getTopBlock()->getGCellIdx(node->getParent()->getLoc());
      Point loc0 = design_->getTopBlock()->getGCellIdx(node->getLoc());
      Point loc2 = design_->getTopBlock()->getGCellIdx(node->getParent()->getParent()->getLoc());
      // detect the non-colinear corner
      if ((loc0.x() != loc1.x() && loc0.y() != loc1.y()) ||
          (loc1.x() != loc2.x() && loc1.y() != loc2.y())) {
        std::cout << "Node id : " << nodeIdx << " Parent loc: " << parentLoc.x() << " " << parentLoc.y() 
                  << " Child loc: " << childLoc.x() << " " << childLoc.y() 
                  << " Corner loc: " << corner.x() << " " << corner.y() << std::endl;
        
        std::cout << "Error ! The corner is not colinear" << " "
                  << "loc0 = " << loc0.x() << " " << loc0.y() << " "
                  << "loc1 = " << loc1.x() << " " << loc1.y() << " "
                  << "loc2 = " << loc2.x() << " " << loc2.y() << std::endl;
      }

      if (loc0.x() == loc1.x() && loc0.y() == loc1.y()) {
        std::cout << "Error ! The corner location is the same as the parent location" << std::endl;
        std::cout << "Node id : " << nodeIdx << " Parent loc: " << parentLoc.x() << " " << parentLoc.y() 
                  << " Child loc: " << childLoc.x() << " " << childLoc.y() 
                  << " Corner loc: " << corner.x() << " " << corner.y() << std::endl;
        
        std::cout << "Error ! The corner is not colinear" << " "
                  << "loc0 = " << loc0.x() << " " << loc0.y() << " "
                  << "loc1 = " << loc1.x() << " " << loc1.y() << " "
                  << "loc2 = " << loc2.x() << " " << loc2.y() << std::endl; 
      }

      if (loc1.x() == loc2.x() && loc1.y() == loc2.y()) {
        std::cout << "Error ! The corner location is the same as the parent location" << std::endl;
        std::cout << "Node id : " << nodeIdx << " Parent loc: " << parentLoc.x() << " " << parentLoc.y() 
                  << " Child loc: " << childLoc.x() << " " << childLoc.y() 
                  << " Corner loc: " << corner.x() << " " << corner.y() << std::endl;
        
        std::cout << "Error ! The corner is not colinear" << " "
                  << "loc0 = " << loc0.x() << " " << loc0.y() << " "
                  << "loc1 = " << loc1.x() << " " << loc1.y() << " "
                  << "loc2 = " << loc2.x() << " " << loc2.y() << std::endl; 
      }
      */
    }

    for (auto& node : net->getNodes()) {
      if (node->getType() != frNodeTypeEnum::frcSteiner) {
        continue;
      }

      auto parent = node->getParent();
      if (parent == nullptr || parent->getType() != frNodeTypeEnum::frcSteiner) {
        continue;
      }
  
      Point parentLoc = design_->getTopBlock()->getGCellIdx(parent->getLoc());
      Point nodeLoc = design_->getTopBlock()->getGCellIdx(node->getLoc());

      if (parentLoc.x() == nodeLoc.x() && parentLoc.y() == nodeLoc.y()) {
        std::cout << "netId = " << net->getId() << " "
                  << "nodeId = " << node->getIntProp() << " "
                  << "parentId = " << parent->getIntProp() << " "
                  << "Parent loc: " << parentLoc.x() << " " << parentLoc.y() << " "
                  << "Node loc: " << nodeLoc.x() << " " << nodeLoc.y() << " "
                  << "Error: two steiner nodes are at the same location\n";
        exit(1);
      }
    }
  };

  int batchId = 0;
  int baseIdx = 0;
  // traverse the list in a DFS manner
  for (auto& batch : netBatch) {
    for (auto netId : batch) {
      auto& net = nets2RR[netId];
      baseIdx = nodeCntPtrVec[netId];
      //std::cout << "NetId = " << net->getId() << std::endl;
      //std::cout << "before  traverse_net_dfs_lambda " << std::endl;
      //printNetInfo(net);
      traverse_net_dfs_lambda(net->getRootGCellNode(), net, baseIdx);
      //std::cout << "before traverse_net_add_node_dfs_lambda " << std::endl;
      //printNetInfo(net);
      //traverse_net_add_node_dfs_lambda(net);
      //std::cout << "after traverse_net_add_node_dfs_lambda " << std::endl;
      //printNetInfo(net);
      //initGR_updateCongestion2D_net_update(net);
    }
  } 

  removeLoop(nets2RR);
  
  // traverse the list in a DFS manner
  for (auto& batch : netBatch) {
    for (auto netId : batch) {
      auto& net = nets2RR[netId];
      baseIdx = nodeCntPtrVec[netId];
      //std::cout << "NetId = " << net->getId() << std::endl;
      //std::cout << "before  traverse_net_dfs_lambda " << std::endl;
      //printNetInfo(net);
      //traverse_net_dfs_lambda(net->getRootGCellNode(), net, baseIdx);
      //std::cout << "before traverse_net_add_node_dfs_lambda " << std::endl;
      //printNetInfo(net);
      traverse_net_add_node_dfs_lambda(net);
      //std::cout << "after traverse_net_add_node_dfs_lambda " << std::endl;
      //printNetInfo(net);
      initGR_updateCongestion2D_net_update(net);
    }
  } 
  
  std::cout << "finish location update" << std::endl;
  checkValidNet_update();
  // exit(1);

  // Remove the CUDA memory
  cudaFree(d_netBatch);
  cudaFree(d_congestion_map);
  cudaFree(d_nodeCntPtr);
  cudaFree(d_nodeLevel);
  cudaFree(d_nodeLoc);
  cudaFree(d_nodeLocPtr);
  cudaFree(d_nodeParentIdx);
  cudaFree(d_nodeEdgeIdx);
  cudaFree(d_nodeEdgePtr);
  cudaFree(d_netBatchMaxDepth);
  cudaFree(d_nodeLocBestCost);
  cudaFree(d_nodeLocBestComb);
  cudaFree(d_bestOptimalLoc); 
  cudaFree(d_cornerLoc);
}

 
 
} // namespace drt

