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

#include "FlexGR.h"
#include <omp.h> 
#include <cmath>
#include <fstream>
#include <iostream>
 
#include "db/grObj/grShape.h"
#include "db/grObj/grVia.h"
#include "db/infra/frTime.h"
#include "db/obj/frGuide.h"
#include "odb/db.h"
#include "utl/exception.h"

#include <iostream>
#include <cuda_runtime.h>
#include <cuda.h>
#include <thrust/device_vector.h>
#include <thrust/functional.h>
#include <thrust/host_vector.h>
#include <thrust/transform_reduce.h>
#include <string>
#include <stdint.h> // For fixed-width integers
#include <cooperative_groups.h>
#include <cooperative_groups/reduce.h>
#include <map>
#include <queue>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include <numeric>
#include <algorithm>

// We always work on the entire grid 
// The grid system is always in terms of global coordinates
// So for each net, we need to translate the local index system into global system

namespace drt {

namespace cg = cooperative_groups;

#define cudaCheckError()                                                   \
{                                                                          \
    cudaError_t err = cudaGetLastError();                                  \
    if (err != cudaSuccess) {                                              \
        fprintf(stderr, "CUDA error at %s:%d: %s\n",                       \
                __FILE__, __LINE__, cudaGetErrorString(err));              \
        exit(1);                                                           \
    }                                                                      \
}

// We treat 0xFFFF as "infinite" cost for 32-bit fields
__device__ __host__ __constant__ uint32_t INF32 = 0xFFFFFFFF;


struct Point2D_CUDA {
  int x;
  int y;

  Point2D_CUDA(int x, int y) : x(x), y(y) {}
};

struct Rect2D_CUDA {
  int xMin;
  int yMin;
  int xMax;
  int yMax;

  Rect2D_CUDA(int xMin, int yMin, int xMax, int yMax) : xMin(xMin), yMin(yMin), xMax(xMax), yMax(yMax) {}
};


enum Directions2D {
  DIR_NORTH    = 0,
  DIR_RIGHT = 1,
  DIR_SOUTH  = 2,
  DIR_LEFT  = 3,
  DIR_NONE  = 255
};


struct NodeData2D {
  // forward and backward propagation (heuristic and real cost) (32 bits each)
  uint32_t forward_h_cost; // heuristic cost
  uint32_t forward_g_cost; // real cost
  uint32_t backward_h_cost; // heuristic cost
  uint32_t backward_g_cost; // real cost
  uint32_t forward_h_cost_prev; 
  uint32_t forward_g_cost_prev;
  uint32_t backward_h_cost_prev;
  uint32_t backward_g_cost_prev;
  
  // Store the direction (for turning point cost and path reconstruction)
  uint8_t forward_direction;
  uint8_t backward_direction;
  uint8_t forward_direction_prev;
  uint8_t backward_direction_prev;
  int golden_parent_x;
  int golden_parent_y;

  // Flags (1 bit each, packed into a single 8-bit field)
  struct Flags {
    uint8_t src_flag : 1; // 1 if this node is the source
    uint8_t dst_flag : 1; // 1 if this node is the destination
    uint8_t forward_update_flag: 1; // 1 if the forward cost is updated
    uint8_t backward_update_flag: 1; // 1 if the backward cost is updated
    uint8_t forward_visited_flag: 1; // 1 if the forward node is visited
    uint8_t backward_visited_flag: 1; // 1 if the backward node is visited
    uint8_t not_used: 2; // 2 bits not used
  }  flags;
};


__host__ __device__ 
void initNodeData2D(NodeData2D& nd) {
  nd.forward_h_cost = INF32;
  nd.forward_g_cost = INF32;
  nd.backward_h_cost = INF32;
  nd.backward_g_cost = INF32;
  nd.forward_h_cost_prev = INF32;
  nd.forward_g_cost_prev = INF32;
  nd.backward_h_cost_prev = INF32;
  nd.backward_g_cost_prev = INF32;
  nd.forward_direction = DIR_NONE;
  nd.backward_direction = DIR_NONE;
  nd.forward_direction_prev = DIR_NONE;
  nd.backward_direction_prev = DIR_NONE;
  nd.golden_parent_x = -1;
  nd.golden_parent_y = -1;
  nd.flags.src_flag = 0;
  nd.flags.dst_flag = 0;
  nd.flags.forward_update_flag = 0;
  nd.flags.backward_update_flag = 0;
}


__device__ __forceinline__ 
uint8_t computeParentDirection2D(int d) {
  switch(d) {
    case 0: return DIR_NORTH;
    case 1: return DIR_RIGHT;
    case 2: return DIR_SOUTH;
    case 3: return DIR_LEFT;
    default: return DIR_NONE;
  }
}


// Invert direction for backtracking
__device__ __forceinline__ 
uint8_t invertDirection2D(uint8_t d) {
  switch(d) {
    case DIR_NORTH:    return DIR_SOUTH;
    case DIR_SOUTH:    return DIR_NORTH;
    case DIR_LEFT:     return DIR_RIGHT;
    case DIR_RIGHT:    return DIR_LEFT;
    default:           return DIR_NONE;
  }
}


// Define the idxToLoc_2D function
// Convert linear index -> (x,y)
__device__ __host__ __forceinline__ 
int2 idxToLoc_2D(int idx, int xDim) {
  int x = idx % xDim;
  int y = idx / xDim;
  return make_int2(x,y);
}


// Define the locToIdx_2D function
// Convert (x,y) -> linear index
__device__ __host__ __forceinline__ 
int locToIdx_2D(int x, int y, int xDim) {
  return y * xDim + x;
}



__global__
void initNodeData__kernel(
  NodeData2D* d_nodes,
  int* d_doneFlag, int* d_meetId,  int netId,  // Net related variables
  int* d_pins, int pinIterStart, int numPins,  // Pin related variables
  int LLX, int LLY, int URX, int URY, // Bounding box
  int xDim, int pinIter, int avgCost)
{ 
  int local_idx = blockDim.x * blockIdx.x + threadIdx.x;
  int numNodes = (URX - LLX + 1) * (URY - LLY + 1);
  if (local_idx > numNodes) {
    return;
  }

  int xDimTemp = URX - LLX + 1;
  int x = local_idx % xDimTemp + LLX;
  int y = local_idx / xDimTemp + LLY;
  int idx = locToIdx_2D(x, y, xDim);

  if (idx == 0) {
    d_doneFlag[netId] = 0;
    d_meetId[netId] = 0x7FFFFFFF;
  }

  int2 xy = idxToLoc_2D(idx, xDim);
  int2 src = idxToLoc_2D(d_pins[pinIterStart + pinIter - 1], xDim);
  int2 dst = idxToLoc_2D(d_pins[pinIterStart + pinIter], xDim);
  
  d_nodes[idx].forward_h_cost = (abs(xy.x - dst.x) + abs(xy.y - dst.y)) * avgCost;
  d_nodes[idx].backward_h_cost = (abs(xy.x - src.x) + abs(xy.y - src.y)) * avgCost;
  //d_nodes[idx].forward_h_cost = 0;
  //d_nodes[idx].backward_h_cost = 0;

  if (d_nodes[idx].flags.src_flag) {
    d_nodes[idx].forward_g_cost = 0;
    d_nodes[idx].forward_g_cost_prev = 0;
  } else {
    d_nodes[idx].forward_g_cost = INF32;
    d_nodes[idx].forward_g_cost_prev = INF32;
  }

  if (d_nodes[idx].flags.dst_flag) {
    d_nodes[idx].backward_g_cost = 0;
    d_nodes[idx].backward_g_cost_prev = 0;
  } else {
    d_nodes[idx].backward_g_cost = INF32;
    d_nodes[idx].backward_g_cost_prev = INF32;
  }

  d_nodes[idx].forward_direction = DIR_NONE;
  d_nodes[idx].backward_direction = DIR_NONE;
  d_nodes[idx].forward_direction_prev = DIR_NONE;
  d_nodes[idx].backward_direction_prev = DIR_NONE;
  d_nodes[idx].flags.forward_update_flag = false;
  d_nodes[idx].flags.backward_update_flag = false;
  d_nodes[idx].flags.forward_visited_flag = false;
  d_nodes[idx].flags.backward_visited_flag = false;
}



////////////////////////////////////////////////////////////////////////////////
// Kernel or device function that uses a grid-wide cooperative group:
////////////////////////////////////////////////////////////////////////////////
__device__
void runBiBellmanFord2D__device(
  cooperative_groups::grid_group& g,   // grid-level cooperative group
  NodeData2D* nodes,
  int* costMap, 
  int* d_dX,
  int* d_dY,
  int* d_doneFlag,
  int LLX, int LLY, int URX, int URY,
  int netId,
  int xDim, int yDim,
  int TURNCOST,
  int maxIters)
{
  // This need to be updated later 
}



// Kernel to find the meetId and launch traceback kernels
// We always work on the entire grid
__global__
void findMeetIdAndTraceBack__kernel(
  NodeData2D* nodes,
  int* d_meetId, int netId, 
  int xDim,
  int LLX, int LLY, int URX, int URY)
{ 
  int idx = blockIdx.x * blockDim.x + threadIdx.x;
  int numNodes = (URX - LLX + 1) * (URY - LLY + 1);  
  int xDimTemp = URX - LLX + 1;
  auto& meetId = d_meetId[netId];
  if (idx < numNodes) {
    int x = idx % xDimTemp + LLX;
    int y = idx / xDimTemp + LLY;
    int nodeIdx = locToIdx_2D(x, y, xDim);    
    if (nodes[idx].flags.forward_visited_flag && nodes[idx].flags.backward_visited_flag) {
      atomicMin(&meetId, idx); // Ensure only the smallest meetId is stored
    }
  }

  // Synchronize threads to ensure meetId is updated
  __syncthreads();
}


// Kernel for forward traceback  
// Please note that this is only called by single thread
__global__
void forwardTraceBack__single_thread__kernel(
  NodeData2D* nodes, 
  int* d_meetId, int netId, 
  int* d_dX, int* d_dY,
  int xDim,
  int LLX, int LLY, int URX, int URY)
{
  auto& meetId = d_meetId[netId];  
  if (meetId == 0x7FFFFFFF) {
    return; // No meetId found
  }
  
  int curId = meetId;
  int maxIterations = (URX - LLX + 1) * (URY - LLY + 1);
  int iteration = 0;
  while (nodes[curId].flags.src_flag == 0 && iteration < maxIterations) {
    uint8_t forwardDirection = nodes[curId].forward_direction;
    nodes[curId].flags.src_flag = 1;
    int2 xy = idxToLoc_2D(curId, xDim);
    int nx = xy.x + d_dX[forwardDirection];
    int ny = xy.y + d_dY[forwardDirection];
    if (nx < LLX || nx > URX || ny < LLY || ny > URY) {
      break;
    }  
    
    nodes[curId].golden_parent_x = nx;
    nodes[curId].golden_parent_y = ny;
    curId = locToIdx_2D(nx, ny, xDim);
    iteration++;
  }

  if (iteration >= maxIterations) {
    printf("Warning: Forward traceback exceeded maximum iterations.\n");
  }
}


// Kernel for backward traceback
__global__
void backwardTraceBack__single__thread__kernel(
  NodeData2D* nodes, 
  int* d_meetId, int netId, 
  int* d_dX, int* d_dY,
  int xDim,
  int LLX, int LLY, int URX, int URY)
{  
  auto& meetId = d_meetId[netId];
  if (meetId == 0x7FFFFFFF) {
    return; // No meetId found
  }
  
  int curId = meetId;
  if (nodes[curId].flags.dst_flag == 1) { 
    nodes[curId].flags.dst_flag = 0; // change the dst flag to 0
    nodes[curId].flags.src_flag = 1;
    return;
  }
  
  int maxIterations = (URX - LLX + 1) * (URY - LLY + 1);
  int iteration = 0;

  while (iteration < maxIterations) {
    int2 xy = idxToLoc_2D(curId, xDim);
    uint8_t backwardDirection = nodes[curId].backward_direction;
    int nx = xy.x + d_dX[backwardDirection];
    int ny = xy.y + d_dY[backwardDirection];
    if (nx < LLX || nx > URX || ny < LLY || ny > URY) {
      break;
    }  
    
    curId = locToIdx_2D(nx, ny, xDim);
    nodes[curId].flags.src_flag = 1;
    nodes[curId].golden_parent_x = xy.x;
    nodes[curId].golden_parent_y = xy.y;
    if (nodes[curId].flags.dst_flag == 1) {
      nodes[curId].flags.dst_flag = 0; // change the dst flag to 0
      break;
    }
    iteration++;
  }

  if (iteration >= maxIterations) {
    printf("Warning: Backward traceback exceeded maximum iterations.\n");
  }
}


void FlexGR::GPUAccelerated2DMazeRoute(
  std::vector<grNet*>& nets,
  std::vector<uint64_t>& h_costMap,
  int xDim, int yDim)
{
  std::cout << "[INFO] GPU accelerated 2D Maze Routing" << std::endl;
  std::cout << "[INFO] Number of nets: " << nets.size() << std::endl;
  
  int numGrids = xDim * yDim;
  int numNets = nets.size();
  
  std::vector<Point2D_CUDA> netVec;
  std::vector<int> netPtr; 
  std::vector<Rect2D_CUDA> netBBoxVec;

  netPtr.push_back(0);
  for (auto& net : nets) {
    for (auto& idx : net->getPinGCellAbsIdxs()) {
      netVec.push_back(Point2D_CUDA(idx.x(), idx.y()));
    }
    netPtr.push_back(netVec.size());
    auto netBBox = net->getRouteAbsBBox();
    netBBoxVec.push_back(
      Rect2D_CUDA(netBBox.xMin(), netBBox.yMin(), netBBox.xMax(), netBBox.yMax()));
  }

  // We need to define the needed utility variables
  std::vector<int> h_dX = {0, 1, 0, -1};
  std::vector<int> h_dY = {1, 0, -1, 0};
  
  int* d_dX;
  int* d_dY;
  int* d_doneFlag; // This is allocated for each net seperately
  int* d_meetId; // This is allocated for each net seperately

  // For the design specific variables
  uint64_t* d_costMap;
  Point2D_CUDA* d_netVec;
  Rect2D_CUDA* d_netBBoxVec;

  // Allocate the device memory for the d_dX and d_dY
  cudaMalloc(&d_dX, 4 * sizeof(int));
  cudaMalloc(&d_dY, 4 * sizeof(int));
  cudaMalloc(&d_doneFlag, nets.size() * sizeof(int));
  cudaMalloc(&d_meetId, nets.size() * sizeof(int));

  cudaMalloc(&d_costMap, numGrids * sizeof(uint64_t));
  cudaMalloc(&d_netVec, netVec.size() * sizeof(Point2D_CUDA));
  cudaMalloc(&d_netBBoxVec, netBBoxVec.size() * sizeof(Rect2D_CUDA));

  cudaMemcpy(d_dX, h_dX.data(), 4 * sizeof(int), cudaMemcpyHostToDevice);
  cudaMemcpy(d_dY, h_dY.data(), 4 * sizeof(int), cudaMemcpyHostToDevice);
  cudaMemcpy(d_costMap, h_costMap.data(), numGrids * sizeof(uint64_t), cudaMemcpyHostToDevice);
  cudaMemcpy(d_netVec, netVec.data(), netVec.size() * sizeof(Point2D_CUDA), cudaMemcpyHostToDevice);
  cudaMemcpy(d_netBBoxVec, netBBoxVec.data(), netBBoxVec.size() * sizeof(Rect2D_CUDA), cudaMemcpyHostToDevice);

  cudaCheckError();

  // Clear the memory
  cudaFree(d_dX);
  cudaFree(d_dY);
  cudaFree(d_doneFlag);
  cudaFree(d_meetId);
  cudaFree(d_costMap);
  cudaFree(d_netVec);
  cudaFree(d_netBBoxVec);
}

} // namespace drt




