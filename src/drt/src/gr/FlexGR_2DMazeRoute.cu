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

constexpr int GRGRIDGRAPHHISTCOSTSIZE = 8;
constexpr int GRSUPPLYSIZE = 8;
constexpr int GRDEMANDSIZE = 16;
constexpr int GRFRACSIZE = 1;
  

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
    uint8_t forward_visited_flag_prev: 1; // 1 if the forward node is visited
    uint8_t backward_visited_flag_prev: 1; // 1 if the backward node is visited
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
  nd.flags.forward_visited_flag = 0;
  nd.flags.backward_visited_flag = 0;
  nd.flags.forward_visited_flag_prev = 0;
  nd.flags.backward_visited_flag_prev = 0;
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


// Bit related functions
__host__ __device__ __forceinline__
bool getBit(const uint64_t* cmap, unsigned idx, unsigned pos)
{
  return (cmap[idx] >> pos) & 1;
}
 
__host__ __device__ __forceinline__
unsigned getBits(const uint64_t* cmap, unsigned idx, unsigned pos, unsigned length)
{
  auto tmp = cmap[idx] & (((1ull << length) - 1) << pos);
  return tmp >> pos;
}
 

__host__ __device__ __forceinline__
unsigned getHistoryCost(const uint64_t* cmap, int idx)
{
  return getBits(cmap, idx, 8, GRGRIDGRAPHHISTCOSTSIZE);
}


__host__ __device__ __forceinline__
float getCongCost(unsigned supply, unsigned demand)
{
  float exp_val = exp(std::min(10.0f, static_cast<float>(supply) - demand));  
  float factor = 4.0f / (1.0f + exp_val); 
  float congCost = demand * (1.0f + factor) / (supply + 1.0f);
  return congCost;  
}



__host__ __device__
unsigned getRawDemand2D(const uint64_t* cmap, int idx, Directions2D dir)
{
  unsigned demand = 0;
  switch (dir) {
    case Directions2D::DIR_RIGHT:
      demand = getBits(cmap, idx, 48, CMAPDEMANDSIZE);
      break;
    case Directions2D::DIR_NORTH:
      demand = getBits(cmap, idx, 32, CMAPDEMANDSIZE);
      break;
    default:;
  }
  return demand;
}


__host__ __device__
unsigned getRawSupply2D(const uint64_t* cmap, int idx, Directions2D dir)
{
  unsigned supply = 0;
  switch (dir) {
    case Directions2D::DIR_RIGHT:
      supply = getBits(cmap, idx, 24, CMAPSUPPLYSIZE);
      break;
    case Directions2D::DIR_NORTH:
      supply = getBits(cmap, idx, 16, CMAPSUPPLYSIZE);
      break;
    default:;
  }
  return supply << CMAPFRACSIZE;
}


__host__ __device__
bool hasBlock2D(const uint64_t* cmap, int idx, Directions2D dir)
{
  bool sol = false;
  switch (dir) {
    case Directions2D::DIR_RIGHT:
      sol = getBit(cmap, idx, 3);
      break;
    case Directions2D::DIR_NORTH:
      sol = getBit(cmap, idx, 2);
      break;
    default:;
  }
  return sol;
}


__host__ __device__
bool hasEdge2D(const uint64_t* bits, int idx, Directions2D dir)
{
  switch (dir) {
    case Directions2D::DIR_RIGHT:
      return getBit(bits, idx, 0);
    case Directions2D::DIR_NORTH:
      return getBit(bits, idx, 1);
    default:;
  }  
}


__host__ __device__
unsigned getEdgeLength2D(const int* xCoords, const int* yCoords, 
  int x, int y, Directions2D dir)
{
  switch (dir) {
    case Directions2D::DIR_RIGHT:
      return xCoords[x + 1] - xCoords[x];
    case Directions2D::DIR_NORTH:
      return yCoords[y + 1] - yCoords[y];
    default:
      return 0;
  }
}



// We do not consider bending cost in this version
__host__ __device__
uint32_t getEdgeCost2D(
  const uint64_t* d_costMap,
  const int* d_xCoords,
  const int* d_yCoords,
  float congThreshold,
  unsigned BLOCKCOST,
  unsigned OVERFLOWCOST,
  unsigned HISTCOST,
  int idx, int x, int y,
  Directions2D dir)
{
  bool blockCost = hasBlock2D(d_costMap, idx, dir);
  unsigned histCost = getHistoryCost(d_costMap, idx);
  unsigned rawDemand = getRawDemand2D(d_costMap, idx, dir) * congThreshold;
  unsigned rawSupply = getRawSupply2D(d_costMap, idx, dir);
  bool overflowCost = (rawDemand >= rawSupply);
  float congCost = getCongCost(rawSupply, rawDemand);
  unsigned edgeLength = getEdgeLength2D(d_xCoords, d_yCoords, x, y, dir);

  // cost 
  uint32_t edgeCost = edgeLength
    + edgeLength * congCost
    + (histCost ? HISTCOST * edgeLength * congCost * HISTCOST : 0)
    + (blockCost ? BLOCKCOST * edgeLength : 0)
    + (overflowCost ? OVERFLOWCOST * edgeLength : 0);

  return edgeCost;
}


// Define the device function for node initialization

__device__
void initNodeData2D__device(
  NodeData2D* d_nodes,
  int* d_pins, int pinIterStart, int pinIter,  // Pin related variables
  int LLX, int LLY, int URX, int URY, // Bounding box
  int xDim)
{ 
  int numNodes = (URX - LLX + 1) * (URY - LLY + 1);
  int xDimTemp = URX - LLX + 1;
  for (int localIdx = threadIdx.x; localIdx < numNodes; localIdx += blockDim.x) {
    int local_x = localIdx % xDimTemp + LLX;
    int local_y = localIdx / xDimTemp + LLY;
    int idx = locToIdx_2D(local_x, local_y, xDim);
    int2 xy = idxToLoc_2D(idx, xDim);
    int2 src = idxToLoc_2D(d_pins[pinIterStart + pinIter - 1], xDim);
    int2 dst = idxToLoc_2D(d_pins[pinIterStart + pinIter], xDim);
  
    // The experimental results show that the heuristic cost is not needed
    d_nodes[idx].forward_h_cost = 0;
    d_nodes[idx].backward_h_cost = 0;

    if (d_nodes[idx].flags.src_flag) {
      d_nodes[idx].forward_g_cost = 0;
      d_nodes[idx].forward_g_cost_prev = 0;
      d_nodes[idx].flags.forward_visited_flag = true;
      d_nodes[idx].flags.forward_visited_flag_prev = true;
    } else {
      d_nodes[idx].forward_g_cost = INF32;
      d_nodes[idx].forward_g_cost_prev = INF32;
      d_nodes[idx].flags.forward_visited_flag = false;
      d_nodes[idx].flags.forward_visited_flag_prev = false;
    }

    if (d_nodes[idx].flags.dst_flag) {
      d_nodes[idx].backward_g_cost = 0;
      d_nodes[idx].backward_g_cost_prev = 0;
      d_nodes[idx].flags.backward_visited_flag = true;
      d_nodes[idx].flags.backward_visited_flag_prev = true;
    } else {
      d_nodes[idx].backward_g_cost = INF32;
      d_nodes[idx].backward_g_cost_prev = INF32;
      d_nodes[idx].flags.backward_visited_flag = false;
      d_nodes[idx].flags.backward_visited_flag_prev = false;
    }

    d_nodes[idx].forward_direction = DIR_NONE;
    d_nodes[idx].backward_direction = DIR_NONE;
    d_nodes[idx].forward_direction_prev = DIR_NONE;
    d_nodes[idx].backward_direction_prev = DIR_NONE;
    d_nodes[idx].flags.forward_update_flag = false;
    d_nodes[idx].flags.backward_update_flag = false;
    d_nodes[idx].flags.forward_visited_flag = false;
    d_nodes[idx].flags.backward_visited_flag = false;
    d_nodes[idx].flags.forward_visited_flag_prev = false;
    d_nodes[idx].flags.backward_visited_flag_prev = false;
  }  
}


__device__ __forceinline__
uint32_t getNeighorCost2D(
  const uint64_t* d_costMap,
  const int* d_xCoords,
  const int* d_yCoords,
  float congThreshold,
  unsigned BLOCKCOST,
  unsigned OVERFLOWCOST,
  unsigned HISTCOST,
  int idx, int x, int y,
  int nbrIdx, int nx, int ny)
{
  uint32_t newG = 0;
  if (nx == x && ny == y - 1) {
    newG += getEdgeCost2D(d_costMap, d_xCoords, d_yCoords, 
      congThreshold, BLOCKCOST, OVERFLOWCOST, HISTCOST,
      nbrIdx, nx, ny, Directions2D::DIR_NORTH);
  } else if (nx == x && ny == y + 1) {
    newG += getEdgeCost2D(d_costMap, d_xCoords, d_yCoords, 
      congThreshold, BLOCKCOST, OVERFLOWCOST, HISTCOST,
      idx, x, y, Directions2D::DIR_NORTH);
  } else if (nx == x - 1 && ny == y) {
    newG += getEdgeCost2D(d_costMap, d_xCoords, d_yCoords, 
      congThreshold, BLOCKCOST, OVERFLOWCOST, HISTCOST,
      nbrIdx, nx, ny, Directions2D::DIR_RIGHT);
  } else if (nx == x + 1 && ny == y) {
    newG += getEdgeCost2D(d_costMap, d_xCoords, d_yCoords, 
      congThreshold, BLOCKCOST, OVERFLOWCOST, HISTCOST,
      idx, x, y, Directions2D::DIR_RIGHT);
  }

  return newG;
}


// Define the device function for the biwaveBellmanFord2D
__device__
void runBiBellmanFord_2D__device(
  cooperative_groups::grid_group& g,   // grid-level cooperative group
  NodeData2D* nodes,
  uint64_t* d_costMap, 
  int* d_dX, int* d_dY,
  int& d_doneFlag,
  int LLX, int LLY, int URX, int URY,
  int xDim, int maxIters,
  const int* d_xCoords,
  const int* d_yCoords,
  float congThreshold,
  unsigned BLOCKCOST,
  unsigned OVERFLOWCOST,
  unsigned HISTCOST)
{
  // A typical 1D decomposition over the entire 2D domain
  int total = (URX - LLX + 1) * (URY - LLY + 1);
  int tid     = blockDim.x * blockIdx.x + threadIdx.x;
  int stride  = blockDim.x * gridDim.x;
  int xDimTemp = URX - LLX + 1;

  // We’ll do up to maxIters or until no changes / front-meet
  for (int iter = 0; iter < maxIters; iter++)
  {
    bool localFrontsMeet = false;
    ////////////////////////////////////////////////////////////////////////////
    // (1) Forward & backward relaxation phase
    ////////////////////////////////////////////////////////////////////////////
    for (int localIdx = tid; localIdx < total; localIdx += stride) {
      int local_x = localIdx % xDimTemp + LLX;
      int local_y = localIdx / xDimTemp + LLY;
      int idx = locToIdx_2D(local_x, local_y, xDim);
      NodeData2D &nd = nodes[idx];
      int2 xy = idxToLoc_2D(idx, xDim);
      int  x  = xy.x;
      int  y  = xy.y;

      // Forward relaxation
      // Typically: newCost = min over neighbors of (neighborCost + edgeWeight).
      // Be sure to skip if src_flag is set (source node may be pinned).
      if (!nd.flags.src_flag) {
        uint32_t bestCost = nd.forward_g_cost_prev;
        int      bestD    = -1;
        for (int d = 0; d < 4; d++) {
          int nx = x + d_dX[d];
          int ny = y + d_dY[d];
          if (nx < LLX || nx > URX || ny < LLY || ny > URY) {
            continue;  // out of bounds
          }
          
          int nbrIdx = locToIdx_2D(nx, ny, xDim);
          // We want neighbor's cost plus the edge weight, e.g. 100
          uint32_t neighborCost = nodes[nbrIdx].forward_g_cost_prev;
          // If neighbor is effectively infinite, skip
          if (neighborCost == 0xFFFFFFFF) {
            continue;
          }
          
          uint32_t newG = neighborCost +
            getNeighorCost2D(d_costMap, d_xCoords, d_yCoords,
              congThreshold, BLOCKCOST, OVERFLOWCOST, HISTCOST,
              idx, x, y, nbrIdx, nx, ny);

          // Check if we found a better cost
          if (newG < bestCost) {
            bestCost = newG;
            bestD    = d;
          }
        } // end neighbor loop

        if (bestD != -1) { // We found an improvement
          nd.forward_g_cost = bestCost;
          nd.forward_direction = computeParentDirection2D(bestD);
          nd.flags.forward_update_flag = true;
        }
      } // end forward

      // Backward relaxation
      // Typically: newCost = min over neighbors of (neighbor.backward_cost + edgeWeight)
      // Skip if dst_flag is set (destination node may be pinned).
      if (!nd.flags.dst_flag) {
        uint32_t bestCost = nd.backward_g_cost_prev;
        int      bestD    = -1;
        for (int d = 0; d < 4; d++) {
          int nx = x + d_dX[d];
          int ny = y + d_dY[d];
          if (nx < LLX || nx > URX || ny < LLY || ny > URY) {
            continue;  // out of bounds
          }
          
          int nbrIdx = locToIdx_2D(nx, ny, xDim);
          uint32_t neighborCost = nodes[nbrIdx].backward_g_cost_prev;
          if (neighborCost == 0xFFFFFFFF) {
            continue;
          }
          
          uint32_t newG = neighborCost +
          getNeighorCost2D(d_costMap, d_xCoords, d_yCoords,
            congThreshold, BLOCKCOST, OVERFLOWCOST, HISTCOST,
            idx, x, y, nbrIdx, nx, ny);
          
          if (newG < bestCost) {
            bestCost = newG;
            bestD    = d;
          }
        } // end neighbor loop

        if (bestD != -1) {
          nd.backward_g_cost = bestCost;
          nd.backward_direction = computeParentDirection2D(bestD);
          nd.flags.backward_update_flag = true;
        }
      } // end backward
    } // end “for each node” (forward + backward)

    g.sync();

    ////////////////////////////////////////////////////////////////////////////
    // (2) Commit updated costs (double-buffering technique)
    ////////////////////////////////////////////////////////////////////////////
    for (int localIdx = tid; localIdx < total; localIdx += stride) {
      int local_x = localIdx % xDimTemp + LLX;
      int local_y = localIdx / xDimTemp + LLY;
      int idx = locToIdx_2D(local_x, local_y, xDim);
      NodeData2D &nd = nodes[idx];
      // If forward_update_flag is set, copy forward_g_cost -> forward_g_cost_prev
      if (nd.flags.forward_update_flag) {
        nd.flags.forward_update_flag = false;
        nd.forward_g_cost_prev = nd.forward_g_cost;
      }
      
      // If backward_update_flag is set, copy backward_g_cost -> backward_g_cost_prev
      if (nd.flags.backward_update_flag) {
        nd.flags.backward_update_flag = false;
        nd.backward_g_cost_prev = nd.backward_g_cost;
      }

      nd.flags.forward_visited_flag_prev = nd.flags.forward_visited_flag;
      nd.flags.backward_visited_flag_prev = nd.flags.backward_visited_flag;
    }

    // Another full grid sync before the “stop” checks:
    g.sync();


    // Needs to be updated
    ////////////////////////////////////////////////////////////////////////////
    // (3) Check if forward and backward fronts meet
    ////////////////////////////////////////////////////////////////////////////
    for (int localIdx = tid; localIdx < total; localIdx += stride) {
      int local_x = localIdx % xDimTemp + LLX;
      int local_y = localIdx / xDimTemp + LLY;
      int idx = locToIdx_2D(local_x, local_y, xDim);
      NodeData2D &nd = nodes[idx];
      // If either side is "unreached," skip
      if (nd.forward_g_cost_prev == 0xFFFFFFFF || 
          nd.backward_g_cost_prev == 0xFFFFFFFF)
      {
        continue;
      }

      // Check the visited flag
      bool localForwardMin = true;
      bool localBackwardMin = true;

      int2 xy = idxToLoc_2D(idx, xDim);
      int  x  = xy.x;
      int  y  = xy.y;

      for (int d = 0; d < 4; d++) {
        int nx = x + d_dX[d];
        int ny = y + d_dY[d];
        if (nx < LLX || nx > URX || ny < LLY || ny > URY) {
          continue;  // out of bounds
        }
        
        int nbrIdx = locToIdx_2D(nx, ny, xDim);
        // check forward case
        if ((nodes[nbrIdx].flags.forward_visited_flag_prev == false) && 
            (nodes[nbrIdx].forward_g_cost_prev + nodes[nbrIdx].forward_h_cost < nd.forward_g_cost_prev + nd.forward_h_cost_prev)) {
          localForwardMin = false;
        } 

        if ((nodes[nbrIdx].flags.backward_visited_flag_prev == false) &&
            (nodes[nbrIdx].backward_g_cost_prev + nodes[nbrIdx].backward_h_cost >= nd.backward_g_cost_prev + nd.backward_h_cost_prev)) {
          localBackwardMin = false;
        }      
      }

      if (localForwardMin == true) {
        nd.flags.forward_visited_flag = true;
      }

      if (localBackwardMin == true) {
        nd.flags.backward_visited_flag = true;
      }
    } // end “for each node”
    
    g.sync();

    // Check if any thread found a front-meet
    for (int localIdx = tid; localIdx < total; localIdx += stride) {
      int local_x = localIdx % xDimTemp + LLX;
      int local_y = localIdx / xDimTemp + LLY;
      int idx = locToIdx_2D(local_x, local_y, xDim);
      NodeData2D &nd = nodes[idx];
      if (nd.flags.forward_visited_flag && nd.flags.backward_visited_flag) {
        localFrontsMeet = true;
      }
    }
    
    g.sync();

    bool localDone = localFrontsMeet;
    if (localDone) {
      atomicExch(&d_doneFlag, 1);
    }
    
    g.sync();

    if (d_doneFlag == 1) {
      d_doneFlag = INF32;
      return;
    }
  } // end for (iter)
}


// Define the device function for the meetId check
__device__
void findMeetIdAndTraceBackCost2D__device(
  NodeData2D* nodes,
  int& d_doneFlag, 
  int LLX, int LLY, int URX, int URY,
  int xDim)
{ 
  int xDimTemp = URX - LLX + 1;
  int numNodes = (URX - LLX + 1) * (URY - LLY + 1);
  for (int localIdx = threadIdx.x; localIdx < numNodes; localIdx += blockDim.x) {
    int local_x = localIdx % xDimTemp + LLX;
    int local_y = localIdx / xDimTemp + LLY;
    int idx = locToIdx_2D(local_x, local_y, xDim);
    if (nodes[idx].flags.forward_visited_flag && nodes[idx].flags.backward_visited_flag) {
      int32_t cost = nodes[idx].forward_g_cost + nodes[idx].backward_g_cost;
      atomicMin(&d_doneFlag, cost);      
    }
  }
}

__device__
void findMeetIdAndTraceBackId2D__device(
  NodeData2D* nodes,
  int& d_doneFlag, 
  int& d_meetId,
  int LLX, int LLY, int URX, int URY,
  int xDim)
{ 
  int xDimTemp = URX - LLX + 1;
  int numNodes = (URX - LLX + 1) * (URY - LLY + 1);
  for (int localIdx = threadIdx.x; localIdx < numNodes; localIdx += blockDim.x) {
    int local_x = localIdx % xDimTemp + LLX;
    int local_y = localIdx / xDimTemp + LLY;
    int idx = locToIdx_2D(local_x, local_y, xDim);
    if (nodes[idx].flags.forward_visited_flag && nodes[idx].flags.backward_visited_flag && 
        (nodes[idx].forward_g_cost + nodes[idx].backward_g_cost == d_doneFlag)) {
      atomicMin(&d_meetId, idx);      
    }
  }
}

__device__
void forwardTraceBack2D__single_thread__device(
  NodeData2D* nodes, 
  int& d_meetId, 
  int* d_dX, int* d_dY,
  int LLX, int LLY, int URX, int URY,
  int xDim)
{
  if (d_meetId == 0x7FFFFFFF) {
    return; // No meetId found
  }
  
  int curId = d_meetId;
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

__device__
void backwardTraceBack2D__single__thread__device(
  NodeData2D* nodes, 
  int& d_meetId, 
  int* d_dX, int* d_dY,
  int LLX, int LLY, int URX, int URY,
  int xDim)
{  
  if (d_meetId == 0x7FFFFFFF) {
    return; // No meetId found
  }
  
  int curId = d_meetId;
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



// Fused cooperative kernel that processes a single net.
__global__ 
void biwaveBellmanFord2D__kernel(
  int netId,
  int* d_netHPWL,
  int* d_netPtr,
  Rect2D_CUDA* d_netBBoxVec,
  int* d_pins,
  uint64_t* d_costMap,
  NodeData2D* d_nodes,
  int* d_dX,
  int* d_dY,
  int* d_doneFlags,
  int* d_meetIds,
  int xDim, int yDim,
  const int* d_xCoords,
  const int* d_yCoords,
  float congThreshold,
  int BLOCKCOST,
  int OVERFLOWCOST, 
  int HISTCOST)
{
  // Obtain a handle to the entire cooperative grid.
  cg::grid_group grid = cg::this_grid();

  // for this net
  int pinIdxStart = d_netPtr[netId];
  int pinIdxEnd = d_netPtr[netId + 1];
  int numPins = pinIdxEnd - pinIdxStart;
  int maxIters = d_netHPWL[netId];
  Rect2D_CUDA netBBox = d_netBBoxVec[netId];
  int LLX = netBBox.xMin;
  int LLY = netBBox.yMin;
  int URX = netBBox.xMax;
  int URY = netBBox.yMax;

  int& d_doneFlag = d_doneFlags[netId];
  int& d_meetId = d_meetIds[netId];

  // Connect the pin one by one
  for (int pinIter = 1; pinIter < numPins; pinIter++) {
    // Initilization
    if (blockIdx.x * blockDim.x + threadIdx.x == 0) {
      d_doneFlag = 0;
      d_meetId = 0x7FFFFFFF;
    }

    initNodeData2D__device(
      d_nodes,
      d_pins, pinIdxStart, pinIter, 
      LLX, LLY, URX, URY, 
      xDim);

    grid.sync(); // Synchronize all threads in the grid

    // Run the Bellman Ford algorithm
    runBiBellmanFord_2D__device(
      grid, d_nodes, d_costMap, d_dX, d_dY, 
      d_doneFlag, LLX, LLY, URX, URY, xDim, maxIters,
      d_xCoords, d_yCoords, congThreshold,
      BLOCKCOST, OVERFLOWCOST, HISTCOST);  

    grid.sync();

    // Find the d_meetId
    findMeetIdAndTraceBackCost2D__device(
      d_nodes, d_doneFlag, 
      LLX, LLY, URX, URY, 
      xDim);

    grid.sync(); // Synchronize all threads in the grid

    findMeetIdAndTraceBackId2D__device(
      d_nodes, d_doneFlag, d_meetId,
      LLX, LLY, URX, URY, 
      xDim);

    grid.sync(); // Synchronize all threads in the grid

    // Traceback
    if (blockIdx.x * blockDim.x + threadIdx.x == 0) {
      // trace back
      forwardTraceBack2D__single_thread__device(
        d_nodes, d_meetId, d_dX, d_dY, 
        LLX, LLY, URX, URY, xDim);
      
      backwardTraceBack2D__single__thread__device(
        d_nodes, d_meetId, d_dX, d_dY, 
        LLX, LLY, URX, URY, xDim);
    }

    grid.sync(); // Synchronize all threads in the grid
  }
}


// Just a wrapper function to call the kernel
void launchMazeRouteStream(
  int* d_netHPWL,
  int* d_netPtr,
  Rect2D_CUDA* d_netBBox,
  int* d_pins,
  uint64_t* d_costMap,
  int* d_nodes,
  int* d_dX,
  int* d_dY,
  int* d_doneFlag,
  int* d_meetId,
  int xDim, int yDim,
  const int* d_xCoords,
  const int* d_yCoords,
  float congThreshold,
  int BLOCKCOST,
  int CONGCOST,
  int HISTCOST,
  int netId,
  int totalThreads,
  cudaStream_t stream)
{
  void* kernelArgs[] = {
    &netId,
    &d_netHPWL,
    &d_netPtr,
    &d_netBBox,
    &d_pins,
    &d_costMap,
    &d_nodes,
    &d_dX,
    &d_dY,
    &d_doneFlag,
    &d_meetId,
    &xDim, 
    &yDim,
    &d_xCoords,
    &d_yCoords,
    &congThreshold,
    &BLOCKCOST, 
    &CONGCOST, 
    &HISTCOST
  };

  // Calculate the maximum number of blocks that can run cooperatively
  int deviceId = 0;
  cudaDeviceProp deviceProp;
  cudaGetDeviceProperties(&deviceProp, deviceId);

  int threadsPerBlock = 1024;
  int numBlocksPerSm = 0;
  cudaOccupancyMaxActiveBlocksPerMultiprocessor(&numBlocksPerSm, biwaveBellmanFord2D__kernel, threadsPerBlock, 0);
  int numSms = deviceProp.multiProcessorCount;
  int numBlocks = numBlocksPerSm * numSms;

  // Ensure the grid size does not exceed the maximum allowed for cooperative launch
  int maxBlocksPerGrid = 0;
  cudaDeviceGetAttribute(&maxBlocksPerGrid, cudaDevAttrMaxGridDimX, deviceId);
  numBlocks = min(numBlocks, maxBlocksPerGrid);
  numBlocks = min(numBlocks, (totalThreads + threadsPerBlock - 1) / threadsPerBlock);
  numBlocks = max(numBlocks, 1);

  printf("Launching kernel with %d blocks\n", numBlocks);

  cudaError_t err = cudaLaunchCooperativeKernel(
    (void*)biwaveBellmanFord2D__kernel,
    numBlocks, threadsPerBlock,
    kernelArgs,
    0,       // additional dynamic shared memory (if needed)
    stream); // launch on the given stream

  if (err != cudaSuccess) {
    printf("Kernel launch error (net %d): %s\n", netId, cudaGetErrorString(err));
  }
}


// We need to restore the connected path from the golden parent
void batchPathSyncUp(
  std::vector<std::unique_ptr<FlexGRWorker>>& uworkers,
  std::vector<grNet*>& nets,
  std::vector<Rect2D_CUDA>& netBBoxVec,
  std::vector<NodeData2D>& nodes,
  int xDim)
{
  for (int netId = 0; netId < nets.size(); netId++) {
    auto& net = nets[netId];
    auto& uworker = uworkers[netId];
    auto& gridGraph = uworker->getGridGraph();
    auto& netBBox = netBBoxVec[netId];
    int LLX = netBBox.xMin;
    int LLY = netBBox.yMin;
    int URX = netBBox.xMax;
    int URY = netBBox.yMax;
    int xDimTemp = URX - LLX + 1;
    int numNodes = (URX - LLX + 1) * (URY - LLY + 1);
    for (int localIdx = 0; localIdx < numNodes; localIdx++) {
      int x = localIdx % xDimTemp + LLX;
      int y = localIdx / xDimTemp + LLY;
      int idx = locToIdx_2D(x, y, xDim);
      gridGraph.setGoldenParent2D(x, y, nodes[idx].golden_parent_x, nodes[idx].golden_parent_y);
    }
  }
}

void FlexGR::GPUAccelerated2DMazeRoute(
  std::vector<std::unique_ptr<FlexGRWorker>>& uworkers,
  std::vector<grNet*>& nets,
  std::vector<uint64_t>& h_costMap,
  std::vector<int>& h_xCoords,
  std::vector<int>& h_yCoords,
  RouterConfiguration* router_cfg,
  float congThreshold,
  int xDim, int yDim)
{
  std::cout << "[INFO] GPU accelerated 2D Maze Routing" << std::endl;
  std::cout << "[INFO] Number of nets: " << nets.size() << std::endl;
  
  int numGrids = xDim * yDim;
  int numNets = nets.size();
  
  std::vector<Point2D_CUDA> netVec;
  std::vector<int> netPtr; 
  std::vector<int> netHWPL;
  std::vector<Rect2D_CUDA> netBBoxVec;
  std::vector<int> pinIdxVec;
  std::vector<NodeData2D> nodes;
  for (auto& node : nodes) {
    initNodeData2D(node);
  }

  netPtr.push_back(0);
  for (auto& net : nets) {
    for (auto& idx : net->getPinGCellAbsIdxs()) {
      netVec.push_back(Point2D_CUDA(idx.x(), idx.y()));
      pinIdxVec.push_back(locToIdx_2D(idx.x(), idx.y(), xDim));
    }
    netPtr.push_back(netVec.size());
    auto netBBox = net->getRouteAbsBBox();
    netBBoxVec.push_back(
      Rect2D_CUDA(netBBox.xMin(), netBBox.yMin(), netBBox.xMax(), netBBox.yMax()));
    netHWPL.push_back(net->getHPWL());
  }

  // Perform the initialization
  for (int netId = 0; netId < numNets; netId++) {
    // Mark the first pin of the net as src
    // and the remaining pins as dst
    int pinIdxStart = netPtr[netId];
    int pinIdxEnd = netPtr[netId + 1];
    nodes[pinIdxVec[pinIdxStart]].flags.src_flag = 1;    
    for (int idx = pinIdxStart + 1; idx < pinIdxEnd; idx++) {
      nodes[pinIdxVec[idx]].flags.dst_flag = 1;
    }
  }

  // We need to define the needed utility variables
  std::vector<int> h_dX = {0, 1, 0, -1};
  std::vector<int> h_dY = {1, 0, -1, 0};
  
  int* d_dX = nullptr;
  int* d_dY = nullptr;
  int* d_doneFlag = nullptr; // This is allocated for each net seperately
  int* d_meetId = nullptr; // This is allocated for each net seperately
  
  // For the design specific variables
  uint64_t* d_costMap = nullptr;
  int* d_xCoords = nullptr;
  int* d_yCoords = nullptr;
  int* d_pinIdxVec = nullptr;
  int* d_nodes = nullptr;
  int* d_netHPWL = nullptr;
  int* d_netPtr = nullptr;
  Rect2D_CUDA* d_netBBox = nullptr;


  // Allocate the device memory for the d_dX and d_dY
  cudaMalloc(&d_dX, 4 * sizeof(int));
  cudaMalloc(&d_dY, 4 * sizeof(int));
  cudaMalloc(&d_doneFlag, nets.size() * sizeof(int));
  cudaMalloc(&d_meetId, nets.size() * sizeof(int));
  cudaMalloc(&d_costMap, numGrids * sizeof(uint64_t));
  cudaMalloc(&d_xCoords, h_xCoords.size() * sizeof(int));
  cudaMalloc(&d_yCoords, h_yCoords.size() * sizeof(int));
  cudaMalloc(&d_pinIdxVec, pinIdxVec.size() * sizeof(int));

  cudaMemcpy(d_dX, h_dX.data(), 4 * sizeof(int), cudaMemcpyHostToDevice);
  cudaMemcpy(d_dY, h_dY.data(), 4 * sizeof(int), cudaMemcpyHostToDevice);
  cudaMemcpy(d_costMap, h_costMap.data(), numGrids * sizeof(uint64_t), cudaMemcpyHostToDevice);
  cudaMemcpy(d_xCoords, h_xCoords.data(), h_xCoords.size() * sizeof(int), cudaMemcpyHostToDevice);
  cudaMemcpy(d_yCoords, h_yCoords.data(), h_yCoords.size() * sizeof(int), cudaMemcpyHostToDevice);

  cudaMemcpy(d_pinIdxVec, pinIdxVec.data(), pinIdxVec.size() * sizeof(int), cudaMemcpyHostToDevice);
  cudaMemcpy(d_nodes, nodes.data(), numGrids * sizeof(NodeData2D), cudaMemcpyHostToDevice);
  cudaMemcpy(d_netHPWL, netHWPL.data(), netHWPL.size() * sizeof(int), cudaMemcpyHostToDevice);
  cudaMemcpy(d_netPtr, netPtr.data(), netPtr.size() * sizeof(int), cudaMemcpyHostToDevice);
  cudaMemcpy(d_netBBox, netBBoxVec.data(), netBBoxVec.size() * sizeof(Rect2D_CUDA), cudaMemcpyHostToDevice);

  cudaCheckError();

  // Unfortunately, the cooperative kernel launch is not supported
  // by the dynamic parallelism.
  // We have to lunch the kernel in the host side
  // So we need to use the cudaStream for each net

  // Create a stream per net
  cudaStream_t* netStreams = new cudaStream_t[numNets];
  for (int i = 0; i < numNets; i++) {
    cudaStreamCreate(&netStreams[i]);
  }

  // According to the original code
  unsigned BLOCKCOST = router_cfg->BLOCKCOST * 100;
  unsigned OVERFLOWCOST = 128;
  unsigned HISTCOST = 4;

  // launch one cooperative kernel per net concurrently using different streams
  // the d_netHPWL is used to determine the maximum iterations
  for (int netId = 0; netId < numNets; netId++) {
    auto& netBBox = netBBoxVec[netId];
    launchMazeRouteStream(
      d_netHPWL, d_netPtr, d_netBBox,
      d_pinIdxVec, d_costMap, d_nodes, 
      d_dX, d_dY, d_doneFlag, d_meetId,
      xDim, yDim, 
      d_xCoords,
      d_yCoords,
      congThreshold,
      BLOCKCOST,
      OVERFLOWCOST,
      HISTCOST,
      netId, 
      (netBBox.xMax - netBBox.xMin + 1) * (netBBox.yMax - netBBox.yMin + 1),
      netStreams[netId]);
  }

  // cudaDeviceSynchronize();
  // Wait for all nets to finish
  for (int i = 0; i < numNets; i++) {
    cudaStreamSynchronize(netStreams[i]);
  }
  
  // We need to trace back the routing path on the CPU side
  cudaMemcpy(nodes.data(), d_nodes, numGrids * sizeof(NodeData2D), cudaMemcpyDeviceToHost);
  cudaCheckError();

  // Reconstruct the nets similar to the CPU version
  batchPathSyncUp(uworkers, nets, netBBoxVec, nodes, xDim);

  for (int i = 0; i < numNets; i++) {
    cudaStreamDestroy(netStreams[i]);
  }

  delete[] netStreams;

  // Clear the memory
  cudaFree(d_dX);
  cudaFree(d_dY);
  cudaFree(d_doneFlag);
  cudaFree(d_meetId);
  cudaFree(d_costMap);
  cudaFree(d_xCoords);
  cudaFree(d_yCoords);
  cudaFree(d_pinIdxVec);
  cudaFree(d_nodes);
  cudaFree(d_netHPWL);
  cudaFree(d_netPtr);
  cudaFree(d_netBBox);
}

} // namespace drt




