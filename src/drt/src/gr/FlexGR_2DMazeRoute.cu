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
#include <future>

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
constexpr int VERBOSE = 0; 

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


__host__ __device__
void printNode2D(NodeData2D& nd) {
  printf(" forward_g_cost = %d, backward_g_cost = %d ", nd.forward_g_cost, nd.backward_g_cost);
  printf(" forward_g_cost_prev = %d, backward_g_cost_prev = %d ", nd.forward_g_cost_prev, nd.backward_g_cost_prev);
  printf(" forward_direction = %d, backward_direction = %d ", nd.forward_direction, nd.backward_direction);
  printf(" forward_visited_flag = %d, backward_visited_flag = %d ", nd.flags.forward_visited_flag, nd.flags.backward_visited_flag);
  printf(" forward_visited_flag_prev = %d, backward_visited_flag_prev = %d ", nd.flags.forward_visited_flag_prev, nd.flags.backward_visited_flag_prev);
  printf(" parent_x = %d, parent_y = %d ", nd.golden_parent_x, nd.golden_parent_y);
  printf(" src_flag = %d, dst_flag = %d\n", nd.flags.src_flag, nd.flags.dst_flag);
}



__device__  
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
__device__  
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
__device__ __host__  
int2 idxToLoc_2D(int idx, int xDim) {
  int x = idx % xDim;
  int y = idx / xDim;
  return make_int2(x,y);
}


// Define the locToIdx_2D function
// Convert (x,y) -> linear index
__device__ __host__  
int locToIdx_2D(int x, int y, int xDim) {
  return y * xDim + x;
}


// Bit related functions
__host__ __device__ 
bool getBit(const uint64_t* cmap, unsigned idx, unsigned pos)
{
  return (cmap[idx] >> pos) & 1;
}
 
__host__ __device__ 
unsigned getBits(const uint64_t* cmap, unsigned idx, unsigned pos, unsigned length)
{
  auto tmp = cmap[idx] & (((1ull << length) - 1) << pos);
  return tmp >> pos;
}
 

__host__ __device__ 
unsigned getHistoryCost(const uint64_t* cmap, int idx)
{
  return getBits(cmap, idx, 8, GRGRIDGRAPHHISTCOSTSIZE);
}


/*
__host__ __device__ 
float getCongCost(unsigned demand, unsigned supply)
{
  float exp_val = exp(std::min(10.0f, static_cast<float>(supply) - demand));  
  float factor = 4.0f / (1.0f + exp_val); 
  float congCost = demand * (1.0f + factor) / (supply + 1.0f);
  return congCost;  
}
*/


// Please DO NOT TOUCH the following function
// The performance of the function is critical to the overall performance of the router.
__host__ __device__
double getCongCost(unsigned demand, unsigned supply)
{
  return (demand * (4 / (1.0 + exp(static_cast<double>(supply) - demand))) / (supply + 1));
}


__host__ __device__
unsigned getRawDemand2D(const uint64_t* cmap, int idx, Directions2D dir)
{
  unsigned demand = 0;
  switch (dir) {
    case Directions2D::DIR_RIGHT:
      demand = getBits(cmap, idx, 48, GRDEMANDSIZE);
      break;
    case Directions2D::DIR_NORTH:
      demand = getBits(cmap, idx, 32, GRDEMANDSIZE);
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
      supply = getBits(cmap, idx, 24, GRSUPPLYSIZE);
      break;
    case Directions2D::DIR_NORTH:
      supply = getBits(cmap, idx, 16, GRSUPPLYSIZE);
      break;
    default:;
  }
  return supply << GRFRACSIZE;
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
  unsigned rawDemand = getRawDemand2D(d_costMap, idx, dir);
  unsigned rawSupply = getRawSupply2D(d_costMap, idx, dir) * congThreshold;
  bool overflowCost = (rawDemand >= rawSupply);
  double congCost = getCongCost(rawDemand, rawSupply);
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


__device__ 
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



// Define the device function for the biwaveBellmanFord_2D_v3__device
__device__
void runBiBellmanFord2D_v3__device(
  NodeData2D* d_nodes,
  uint64_t* d_costMap, 
  const int* __restrict__ d_dX, 
  const int* __restrict__ d_dY,
  const int* __restrict__ d_xCoords,
  const int* __restrict__ d_yCoords,
  int LLX, int LLY, int URX, int URY,
  int xDim, 
  int maxIters,
  float congThreshold,
  unsigned BLOCKCOST,
  unsigned OVERFLOWCOST,
  unsigned HISTCOST)
{
  // Each device function is handled by a single block
  int total = (URX - LLX + 1) * (URY - LLY + 1);
  int tid = threadIdx.x;
  int stride = blockDim.x; 
  int xDimTemp = URX - LLX + 1;
  
  
  // define the shared memory for d_dx and d_dy
  __shared__ int s_dX[4];
  __shared__ int s_dY[4];
  volatile __shared__ int s_doneFlag;
  volatile __shared__ int s_minCost;
  volatile __shared__ int s_meetId;
  __shared__ int tracebackError;   // 0: no error; 1: error detected


  // Load the d_dX and d_dY into shared memory
  if (tid < 4) {
    s_dX[tid] = d_dX[tid];
    s_dY[tid] = d_dY[tid];
    if (tid == 0) {
      s_doneFlag = 0;
      s_minCost = 0x7FFFFFFF;
      s_meetId = 0x7FFFFFFF;
      tracebackError = 0;
    }
  }

  __syncthreads();

  // We’ll do up to maxIters or until no changes / front-meet
  for (int iter = 0; iter < maxIters && (s_doneFlag == 0); iter++)
  {
    bool localFrontsMeet = false;
    ////////////////////////////////////////////////////////////////////////////
    // (1) Forward & backward relaxation phase
    ////////////////////////////////////////////////////////////////////////////
    for (int localIdx = tid; localIdx < total; localIdx += stride) {
      int x = localIdx % xDimTemp + LLX;
      int y = localIdx / xDimTemp + LLY;
      int idx = locToIdx_2D(x, y, xDim);
      NodeData2D &nd = d_nodes[idx];

      // Forward relaxation
      // Typically: newCost = min over neighbors of (neighborCost + edgeWeight).
      // Be sure to skip if src_flag is set (source node may be pinned).
      if (!nd.flags.src_flag) {
        uint32_t bestCost = nd.forward_g_cost_prev;
        int      bestD    = -1;
        for (int d = 0; d < 4; d++) {
          int nx = x + s_dX[d];
          int ny = y + s_dY[d];
          if (nx < LLX || nx > URX || ny < LLY || ny > URY) {
            continue;  // out of bounds
          }
        
          int nbrIdx = locToIdx_2D(nx, ny, xDim);
          // We want neighbor's cost plus the edge weight, e.g. 100
          uint32_t neighborCost = d_nodes[nbrIdx].forward_g_cost_prev;
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
          nd.flags.forward_update_flag = 1;
        }
      } // end forward

      // Backward relaxation
      // Typically: newCost = min over neighbors of (neighbor.backward_cost + edgeWeight)
      // Skip if dst_flag is set (destination node may be pinned).
      if (!nd.flags.dst_flag) {
        uint32_t bestCost = nd.backward_g_cost_prev;
        int      bestD    = -1;
        for (int d = 0; d < 4; d++) {
          int nx = x + s_dX[d];
          int ny = y + s_dY[d];
          if (nx < LLX || nx > URX || ny < LLY || ny > URY) {
            continue;  // out of bounds
          }
        
          int nbrIdx = locToIdx_2D(nx, ny, xDim);
          uint32_t neighborCost = d_nodes[nbrIdx].backward_g_cost_prev;
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
          nd.flags.backward_update_flag = 1;
        }
      } // end backward
    } // end “for each node” (forward + backward)

    __syncthreads();

    ////////////////////////////////////////////////////////////////////////////
    // (2) Commit updated costs (double-buffering technique)
    ////////////////////////////////////////////////////////////////////////////
    for (int localIdx = tid; localIdx < total; localIdx += stride) {
      int local_x = localIdx % xDimTemp + LLX;
      int local_y = localIdx / xDimTemp + LLY;
      int idx = locToIdx_2D(local_x, local_y, xDim);
      NodeData2D &nd = d_nodes[idx];
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

    __syncthreads();


    // Needs to be updated
    ////////////////////////////////////////////////////////////////////////////
    // (3) Check if forward and backward fronts meet
    ////////////////////////////////////////////////////////////////////////////
    for (int localIdx = tid; localIdx < total; localIdx += stride) {
      int x = localIdx % xDimTemp + LLX;
      int y = localIdx / xDimTemp + LLY;
      int idx = locToIdx_2D(x, y, xDim);
      NodeData2D &nd = d_nodes[idx];

      // if (idx == 6007 && (nd.forward_g_cost == 91514 || nd.flags.forward_visited_flag_prev)) {
      //  printf("idx = %d, x = %d, y = %d, cost = %d, forward_visited_flag = %d\n", 
      //    idx, x, y, nd.forward_g_cost, nd.flags.forward_visited_flag_prev);
      //}

      // Check the forward visited flag
      if (!nd.flags.forward_visited_flag_prev) {
        bool localForwardMin = (nd.forward_g_cost_prev != 0xFFFFFFFF);
      

        for (int d = 0; d < 4; d++) {
          int nx = x + s_dX[d];
          int ny = y + s_dY[d];
          if (nx < LLX || nx > URX || ny < LLY || ny > URY) {
            continue;  // out of bounds
          }
         
                  
          int nbrIdx = locToIdx_2D(nx, ny, xDim);
          NodeData2D &nbr = d_nodes[nbrIdx];
          
          /*
          int neighborCost = nbr.forward_g_cost_prev;          
          uint32_t newG = neighborCost +
          getNeighorCost2D(d_costMap, d_xCoords, d_yCoords,
            congThreshold, BLOCKCOST, OVERFLOWCOST, HISTCOST,
            idx, x, y, nbrIdx, nx, ny);

          if (idx == 6007 && nd.forward_g_cost_prev == 91514) {
            printf("nbrIdx = %d, neighborCost = %d, nd.cost = %d\n", nbrIdx, neighborCost, nd.forward_g_cost_prev);
          }*/

      
          // Check forward minimum
          if (!nbr.flags.forward_visited_flag_prev && 
              (nbr.forward_g_cost_prev + nbr.forward_h_cost < nd.forward_g_cost_prev + nd.forward_h_cost)) {
            localForwardMin = false;
          } 

        }

        if (localForwardMin) {
          nd.flags.forward_visited_flag = true;
        }
      }

      // Check the backward visited flag
      if (!nd.flags.backward_visited_flag_prev) {
        bool localBackwardMin = (nd.backward_g_cost_prev != 0xFFFFFFFF);
        for (int d = 0; d < 4; d++) {
          int nx = x + s_dX[d];
          int ny = y + s_dY[d];
          if (nx < LLX || nx > URX || ny < LLY || ny > URY) {
            continue;  // out of bounds
          }
          
          int nbrIdx = locToIdx_2D(nx, ny, xDim);
          NodeData2D &nbr = d_nodes[nbrIdx];
          

          /*
          int neighborCost = nbr.backward_g_cost_prev;          
          uint32_t newG = neighborCost +
          getNeighorCost2D(d_costMap, d_xCoords, d_yCoords,
            congThreshold, BLOCKCOST, OVERFLOWCOST, HISTCOST,
            idx, x, y, nbrIdx, nx, ny);
          */
       
          
          // Check forward minimum
          if (!nbr.flags.backward_visited_flag_prev && 
              (nbr.backward_g_cost_prev + nbr.backward_h_cost < nd.backward_g_cost_prev + nd.backward_h_cost)) {
            localBackwardMin = false;
          } 
        }

        if (localBackwardMin) {
          nd.flags.backward_visited_flag = true;
        }
      }
    } // end “for each node”
    
  
    __syncthreads();

    // Check if any thread found a front-meet
    for (int localIdx = tid; localIdx < total; localIdx += stride) {
      int local_x = localIdx % xDimTemp + LLX;
      int local_y = localIdx / xDimTemp + LLY;
      int idx = locToIdx_2D(local_x, local_y, xDim);
      NodeData2D &nd = d_nodes[idx];
      if (nd.flags.forward_visited_flag && nd.flags.backward_visited_flag) {
        localFrontsMeet = true;
      }
    }

    __syncthreads();

    if (localFrontsMeet) {
      atomicExch((int*)&s_doneFlag, 1);
    }
    
    __syncthreads();

  } // end for (iter)

  __syncthreads();

  // Ensure all threads know the doneFlag
  bool converged = (s_doneFlag == 1);
  if (!converged) {
    if (tid == 0) {
      printf("Error! biwaveBellmanFord_2D_v3__device did not converge. doneFlag = false \n");
    }
    __syncthreads();
    return;
  }

  __syncthreads();  

  // identify the minimum cost
  for (int localIdx = tid; localIdx < total; localIdx += stride) {
    int local_x = localIdx % xDimTemp + LLX;
    int local_y = localIdx / xDimTemp + LLY;
    int idx = locToIdx_2D(local_x, local_y, xDim);
    auto& nd = d_nodes[idx];
    if (nd.flags.forward_visited_flag && nd.flags.backward_visited_flag) {
      int32_t cost = nd.forward_g_cost + nd.backward_g_cost;
      atomicMin((int*)&s_minCost, cost);
    }    
  }

  // identify the meetId
  __syncthreads();
  for (int localIdx = tid; localIdx < total; localIdx += stride) {
    int local_x = localIdx % xDimTemp + LLX;
    int local_y = localIdx / xDimTemp + LLY;
    int idx = locToIdx_2D(local_x, local_y, xDim);
    auto& nd = d_nodes[idx];
    if ((nd.forward_g_cost != INF32 && nd.backward_g_cost != INF32) &&
        (nd.forward_g_cost + nd.backward_g_cost == s_minCost)) {
      atomicMin((int*)&s_meetId, idx);      
    }
  }

  // identify the parent
  __syncthreads();

  
  // Check if s_meetId is valid. (All threads in warp 0 & 1 perform the check.)
  if (s_meetId == 0x7FFFFFFF) {
    if (threadIdx.x == 0 || threadIdx.x == 1) {
      printf("Error! biwaveBellmanFord_2D_v3__device did not converge. meetId = 0x7FFFFFFF\n");
    }
    // Set the error flag (using atomicExch for safety in parallel regions)
    if (threadIdx.x == 0 || threadIdx.x == 1) {
      atomicExch(&tracebackError, 1);
    }
  }
  __syncthreads();
  
  // Only threads 0 and 1 perform the traceback if no error occurred.
  if (tracebackError == 0) {
    // ----- Forward Traceback (Thread 0) -----
    if (threadIdx.x == 0) {
      printf("Start the traceback\n");
    
      int tempIter = 0;
      // Update the meetId accrodingly to remove reduant path
      while (d_nodes[s_meetId].forward_direction == d_nodes[s_meetId].backward_direction && tempIter < total) {
        if (d_nodes[s_meetId].forward_direction == DIR_NONE) {
          printf("Error: forward_direction == DIR_NONE\n");
          break;
        }
      
        printf("s_meetId = %d, forward_direction = %d, backward_direction = %d\n", s_meetId, d_nodes[s_meetId].forward_direction, d_nodes[s_meetId].backward_direction);
        int2 xy = idxToLoc_2D(s_meetId, xDim);
        auto direction = d_nodes[s_meetId].forward_direction;
        int nx = xy.x + s_dX[direction];
        int ny = xy.y + s_dY[direction];
        s_meetId = locToIdx_2D(nx, ny, xDim);
        tempIter++;
      }

      if (tempIter >= total) {
        printf("Error: Forward traceback exceeded maximum iterations.\n");
      }
  
      printf("Start the traceback\n");

      int forwardCurId = s_meetId;
      int forwardIteration = 0;
      while (!d_nodes[forwardCurId].flags.src_flag && forwardIteration < total) {
        uint8_t fwdDir = d_nodes[forwardCurId].forward_direction;
        int2 xy = idxToLoc_2D(forwardCurId, xDim);
        int nx = xy.x + s_dX[fwdDir];
        int ny = xy.y + s_dY[fwdDir];
        // Break if the next position is out of bounds.
        if (nx < LLX || nx > URX || ny < LLY || ny > URY) {
          break;
        }
        // Record the golden parent before moving on.
        d_nodes[forwardCurId].golden_parent_x = nx;
        d_nodes[forwardCurId].golden_parent_y = ny;
        // Mark the node as processed.
        d_nodes[forwardCurId].flags.src_flag = 1;
        
        // Move to the next node.
        forwardCurId = locToIdx_2D(nx, ny, xDim);
        forwardIteration++;
      }
      if (forwardIteration >= total) {
        printf("Warning: Forward traceback exceeded maximum iterations.\n");
      }
    // }

    printf("Start the backward traceback\n");
      
    // ----- Backward Traceback (Thread 1) -----
    // if (threadIdx.x == 1) {
      int backwardCurId = s_meetId;
      int backwardIteration = 0;
      // Special handling if the starting node is already marked as destination.
      if (d_nodes[backwardCurId].flags.dst_flag == 1) {
        d_nodes[backwardCurId].flags.dst_flag = 0; // Reset dst flag.
        d_nodes[backwardCurId].flags.src_flag = 1;
      } else {
        while (!d_nodes[backwardCurId].flags.dst_flag && backwardIteration < total) {
          int2 xy = idxToLoc_2D(backwardCurId, xDim);
          uint8_t backwardDir = d_nodes[backwardCurId].backward_direction;
          int nx = xy.x + s_dX[backwardDir];
          int ny = xy.y + s_dY[backwardDir];
          if (nx < LLX || nx > URX || ny < LLY || ny > URY) {
            break;
          }
          int nextId = locToIdx_2D(nx, ny, xDim);
          // Check if backward traceback collides with the forward traceback.
          if (d_nodes[nextId].golden_parent_x != -1 || d_nodes[nextId].golden_parent_y != -1) {
            printf("Error: Backward traceback meets forward traceback.\n");
            printf("meetId = %d\n", s_meetId); 
            for (int localIdx = 0; localIdx < total; localIdx++) {
              int local_x = localIdx % xDimTemp + LLX;
              int local_y = localIdx / xDimTemp + LLY;
              int idx = locToIdx_2D(local_x, local_y, xDim);
              printf("node id = %d, x = %d, y = %d ", idx, local_x, local_y);
              printNode2D(d_nodes[idx]);
            }
            
            printf("nextId = %d, x = %d, y = %d,  golden_parent_x = %d, golden_parent_y = %d, dst_flag = %d, src_flag = %d\n", 
              nextId, nx, ny, 
              d_nodes[nextId].golden_parent_x, d_nodes[nextId].golden_parent_y, 
              d_nodes[nextId].flags.dst_flag, d_nodes[nextId].flags.src_flag);
            atomicExch(&tracebackError, 1);
            break;      
          }
          
          // Update parent's information.
          d_nodes[nextId].flags.src_flag = 1;
          d_nodes[nextId].golden_parent_x = xy.x;
          d_nodes[nextId].golden_parent_y = xy.y;
          
          backwardCurId = nextId;
          backwardIteration++;
        }
        // Reset dst flag at the final node.
        d_nodes[backwardCurId].flags.dst_flag = 0;
        if (backwardIteration >= total) {
          printf("Warning: Backward traceback exceeded maximum iterations.\n");
        }
      }

      printf("End the traceback\n");
    }
  }
  __syncthreads();


  /*
  // We only need the first thread to update the parent
  if (tid == 0) {
    if (s_meetId == 0x7FFFFFFF) {
      printf("Error! biwaveBellmanFord_2D_v3__device did not converge. meetId = 0x7FFFFFFF \n");
      return;
    }

    int curId = s_meetId;
    int maxIterations = total;
    int iteration = 0;

    // Forward traceback
    while (d_nodes[curId].flags.src_flag == 0 && iteration < maxIterations) {
      // Ensure forward_direction is valid (e.g., 0 <= forward_direction < 4)
      uint8_t fwdDir = d_nodes[curId].forward_direction;
      // Record the golden parent BEFORE moving on.
      int2 xy = idxToLoc_2D(curId, xDim);
      int nx = xy.x + s_dX[fwdDir];
      int ny = xy.y + s_dY[fwdDir];
      if (nx < LLX || nx > URX || ny < LLY || ny > URY) { break; }
      
      d_nodes[curId].golden_parent_x = nx;
      d_nodes[curId].golden_parent_y = ny;
      
      // Mark this node as processed.
      d_nodes[curId].flags.src_flag = 1;
      
      // Move to the next node.
      curId = locToIdx_2D(nx, ny, xDim);
      iteration++;
    }
   
    if (iteration >= maxIterations) {
      printf("Warning: Forward traceback exceeded maximum iterations.\n");
    }
  } else if (tid == 1) {   
    // Backward traceback
    if (s_meetId == 0x7FFFFFFF) {
      printf("Error! biwaveBellmanFord_2D_v3__device did not converge. meetId = 0x7FFFFFFF \n");
      return;
    }
    
    int maxIterations = total;
    int curId = s_meetId;
    int iteration = 0;
    
    if (d_nodes[curId].flags.dst_flag == 1) { 
      d_nodes[curId].flags.dst_flag = 0; // change the dst flag to 0
      d_nodes[curId].flags.src_flag = 1;
      return;
    }
  
    while (d_nodes[curId].flags.dst_flag == 0 && iteration < maxIterations) {
      int2 xy = idxToLoc_2D(curId, xDim);
      uint8_t backwardDirection = d_nodes[curId].backward_direction;
      int nx = xy.x + s_dX[backwardDirection];
      int ny = xy.y + s_dY[backwardDirection];
      if (nx < LLX || nx > URX || ny < LLY || ny > URY) {
        break;
      }  
     
      int nextId = locToIdx_2D(nx, ny, xDim);
      if (d_nodes[nextId].golden_parent_x != -1) {
        printf("Error: Backward traceback meets forward traceback.\n");
      }    
      
      d_nodes[nextId].flags.src_flag = 1;
      d_nodes[nextId].golden_parent_x = xy.x;
      d_nodes[nextId].golden_parent_y = xy.y;
      
      curId = nextId;
      iteration++;
    }
    
    d_nodes[curId].flags.dst_flag = 0; // change the dst flag to 0
    if (iteration >= maxIterations) {
      printf("Warning: Backward traceback exceeded maximum iterations.\n");
    }
  }

  __syncthreads();
  */
}



// Define the device function for the biwaveBellmanFord_2D_v4__device
__device__
void runBiBellmanFord2D_v4__device(
  NodeData2D* d_nodes,
  uint64_t* d_costMap, 
  const int* __restrict__ d_dX, 
  const int* __restrict__ d_dY,
  const int* __restrict__ d_xCoords,
  const int* __restrict__ d_yCoords,
  int LLX, int LLY, int URX, int URY,
  int xDim, 
  int maxIters,
  float congThreshold,
  unsigned BLOCKCOST,
  unsigned OVERFLOWCOST,
  unsigned HISTCOST)
{
  // Each device function is handled by a single block
  int total = (URX - LLX + 1) * (URY - LLY + 1);
  int tid = threadIdx.x;
  int stride = blockDim.x; 
  int xDimTemp = URX - LLX + 1;
  
  // Define the shared memory for d_dx and d_dy
  __shared__ int s_dX[4];
  __shared__ int s_dY[4];
  volatile __shared__ int s_doneFlag;
  volatile __shared__ int s_minCost;
  volatile __shared__ int s_meetId;
  __shared__ int tracebackError;   // 0: no error; 1: error detected

  // Load the d_dX and d_dY into shared memory
  if (tid < 4) {
    s_dX[tid] = d_dX[tid];
    s_dY[tid] = d_dY[tid];
    if (tid == 0) {
      s_doneFlag = 0;
      s_minCost = 0x7FFFFFFF;
      s_meetId = 0x7FFFFFFF;
      tracebackError = 0;
    }
  }
  __syncthreads();

  // We'll do up to maxIters or until no changes / front-meet
  for (int iter = 0; iter < maxIters && (s_doneFlag == 0); iter++)
  {
    bool localFrontsMeet = false;
    ////////////////////////////////////////////////////////////////////////////
    // (1) Forward & backward relaxation phase
    ////////////////////////////////////////////////////////////////////////////
    for (int localIdx = tid; localIdx < total; localIdx += stride) {
      int x = localIdx % xDimTemp + LLX;
      int y = localIdx / xDimTemp + LLY;
      int idx = locToIdx_2D(x, y, xDim);
      NodeData2D &nd = d_nodes[idx];

      // Forward relaxation:
      // newCost = min over neighbors of (neighborCost + edgeWeight).
      // Skip if src_flag is set.
      if (!nd.flags.src_flag) {
        uint32_t bestCost = nd.forward_g_cost_prev;
        int      bestD    = -1;
        for (int d = 0; d < 4; d++) {
          int nx = x + s_dX[d];
          int ny = y + s_dY[d];
          if (nx < LLX || nx > URX || ny < LLY || ny > URY) {
            continue;  // out of bounds
          }
          int nbrIdx = locToIdx_2D(nx, ny, xDim);
          uint32_t neighborCost = d_nodes[nbrIdx].forward_g_cost_prev;
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

        if (bestD != -1) { // Found an improvement
          nd.forward_g_cost = bestCost;
          nd.forward_direction = computeParentDirection2D(bestD);
          nd.flags.forward_update_flag = 1;
        }
      } // end forward

      // Backward relaxation:
      // newCost = min over neighbors of (neighbor.backward_cost + edgeWeight).
      // Skip if dst_flag is set.
      if (!nd.flags.dst_flag) {
        uint32_t bestCost = nd.backward_g_cost_prev;
        int      bestD    = -1;
        for (int d = 0; d < 4; d++) {
          int nx = x + s_dX[d];
          int ny = y + s_dY[d];
          if (nx < LLX || nx > URX || ny < LLY || ny > URY) {
            continue;  // out of bounds
          }
          int nbrIdx = locToIdx_2D(nx, ny, xDim);
          uint32_t neighborCost = d_nodes[nbrIdx].backward_g_cost_prev;
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
          nd.flags.backward_update_flag = 1;
        }
      } // end backward
    } // end for each node (relaxation)
    __syncthreads();

    ////////////////////////////////////////////////////////////////////////////
    // (2) Commit updated costs (double-buffering technique)
    ////////////////////////////////////////////////////////////////////////////
    for (int localIdx = tid; localIdx < total; localIdx += stride) {
      int local_x = localIdx % xDimTemp + LLX;
      int local_y = localIdx / xDimTemp + LLY;
      int idx = locToIdx_2D(local_x, local_y, xDim);
      NodeData2D &nd = d_nodes[idx];
      if (nd.flags.forward_update_flag) {
        nd.flags.forward_update_flag = false;
        nd.forward_g_cost_prev = nd.forward_g_cost;
      }
      if (nd.flags.backward_update_flag) {
        nd.flags.backward_update_flag = false;
        nd.backward_g_cost_prev = nd.backward_g_cost;
      }
      // Save previous visited flags for later comparisons.
      nd.flags.forward_visited_flag_prev = nd.flags.forward_visited_flag;
      nd.flags.backward_visited_flag_prev = nd.flags.backward_visited_flag;
    }
    __syncthreads();

    ////////////////////////////////////////////////////////////////////////////
    // (3) Mark nodes using parallel reduction to select the minimum cost nodes.
    //     For the forward side, we find the node with the minimum forward_g_cost
    //     (among nodes not already visited) and mark its forward_visited_flag.
    //     Similarly for the backward side.
    ////////////////////////////////////////////////////////////////////////////
    __shared__ int s_minForwardCost;
    __shared__ int s_minBackwardCost;
  
    if (threadIdx.x == 0) {
      s_minForwardCost = 0x7FFFFFFF;
      s_minBackwardCost = 0x7FFFFFFF;
    }

    __syncthreads();

    // Each thread examines a subset of nodes.
    for (int localIdx = tid; localIdx < total; localIdx += stride) {
      int x = localIdx % xDimTemp + LLX;
      int y = localIdx / xDimTemp + LLY;
      int idx = locToIdx_2D(x, y, xDim);
      NodeData2D &nd = d_nodes[idx];

      // For forward: only consider nodes not yet visited with a valid cost.
      if (!nd.flags.forward_visited_flag_prev && nd.forward_g_cost_prev != 0xFFFFFFFF) {
        atomicMin(&s_minForwardCost, nd.forward_g_cost_prev);
      }
      
      // For backward: only consider nodes not yet visited with a valid cost.
      if (!nd.flags.backward_visited_flag_prev && nd.backward_g_cost_prev != 0xFFFFFFFF) {
        atomicMin(&s_minBackwardCost, nd.backward_g_cost_prev);
      }
    }
    __syncthreads();
    
    // Alternatively, one could scan all nodes to check for any node that has both flags true.
    for (int localIdx = tid; localIdx < total; localIdx += stride) {
      int local_x = localIdx % xDimTemp + LLX;
      int local_y = localIdx / xDimTemp + LLY;
      int idx = locToIdx_2D(local_x, local_y, xDim);
      NodeData2D &nd = d_nodes[idx];
      if (nd.forward_g_cost_prev == s_minForwardCost) {
        nd.flags.forward_visited_flag = true;
      }

      if (nd.backward_g_cost_prev == s_minBackwardCost) {
        nd.flags.backward_visited_flag = true;
      }

      // Check if the forward and backward fronts meet.
      if (nd.flags.forward_visited_flag && nd.flags.backward_visited_flag) {
        localFrontsMeet = true;
      }
    }

    __syncthreads();

    if (localFrontsMeet) {
      atomicExch((int*)&s_doneFlag, 1);
    }
    __syncthreads();

  } // end for (iter)

  __syncthreads();

  // Ensure all threads know the doneFlag
  bool converged = (s_doneFlag == 1);
  if (!converged) {
    if (tid == 0) {
      printf("Error! biwaveBellmanFord_2D_v3__device did not converge. doneFlag = false \n");
    }
    __syncthreads();
    return;
  } 
  __syncthreads();  

  // Identify the minimum total cost (forward + backward) among meeting nodes.
  for (int localIdx = tid; localIdx < total; localIdx += stride) {
    int local_x = localIdx % xDimTemp + LLX;
    int local_y = localIdx / xDimTemp + LLY;
    int idx = locToIdx_2D(local_x, local_y, xDim);
    auto& nd = d_nodes[idx];
    if (nd.flags.forward_visited_flag && nd.flags.backward_visited_flag) {
      int32_t cost = nd.forward_g_cost + nd.backward_g_cost;
      atomicMin((int*)&s_minCost, cost);
    }    
  }

  // Identify the meetId corresponding to the minimum cost.
  __syncthreads();
  for (int localIdx = tid; localIdx < total; localIdx += stride) {
    int local_x = localIdx % xDimTemp + LLX;
    int local_y = localIdx / xDimTemp + LLY;
    int idx = locToIdx_2D(local_x, local_y, xDim);
    auto& nd = d_nodes[idx];
    if ((nd.forward_g_cost != INF32 && nd.backward_g_cost != INF32) &&
        (nd.forward_g_cost + nd.backward_g_cost == s_minCost)) {
      atomicMin((int*)&s_meetId, idx);      
    }
  }
  __syncthreads();

  // Check if s_meetId is valid.
  if (s_meetId == 0x7FFFFFFF) {
    if (threadIdx.x == 0 || threadIdx.x == 1) {
      printf("Error! biwaveBellmanFord_2D_v3__device did not converge. meetId = 0x7FFFFFFF\n");
    }
    if (threadIdx.x == 0 || threadIdx.x == 1) {
      atomicExch(&tracebackError, 1);
    }
  }
  __syncthreads();
  
  // Only threads 0 and 1 perform the traceback if no error occurred.
  if (tracebackError == 0) {
    // ----- Forward Traceback (Thread 0) -----
    if (threadIdx.x == 0) {
      // printf("Start the traceback\n");
      int tempIter = 0;      
      // Update the meetId accordingly to remove redundant path
      while (d_nodes[s_meetId].forward_direction == d_nodes[s_meetId].backward_direction && tempIter < total) {
        if (d_nodes[s_meetId].forward_direction == DIR_NONE) {
          printf("Warning: forward_direction == DIR_NONE\n");
          break;
        }
        
        int2 xy = idxToLoc_2D(s_meetId, xDim);
        auto direction = d_nodes[s_meetId].forward_direction;
        int nx = xy.x + s_dX[direction];
        int ny = xy.y + s_dY[direction];
        s_meetId = locToIdx_2D(nx, ny, xDim);
        tempIter++;
      }

      if (tempIter >= total) {
        printf("Warning: reduce iteration exceeded maximum iterations.\n");
      }
            
      // printf("Start the forward traceback\n");

      int forwardCurId = s_meetId;
      int forwardIteration = 0;
      while (!d_nodes[forwardCurId].flags.src_flag && forwardIteration < total) {
        uint8_t fwdDir = d_nodes[forwardCurId].forward_direction;
        int2 xy = idxToLoc_2D(forwardCurId, xDim);
        int nx = xy.x + s_dX[fwdDir];
        int ny = xy.y + s_dY[fwdDir];
        if (nx < LLX || nx > URX || ny < LLY || ny > URY) {
          break;
        }
        d_nodes[forwardCurId].golden_parent_x = nx;
        d_nodes[forwardCurId].golden_parent_y = ny;
        d_nodes[forwardCurId].flags.src_flag = 1;
        forwardCurId = locToIdx_2D(nx, ny, xDim);
        forwardIteration++;
      }
      if (forwardIteration >= total) {
        printf("Warning: Forward traceback exceeded maximum iterations.\n");
      }
    // }
    

     //  printf("Start the backward traceback\n");
    // ----- Backward Traceback (Thread 1) -----
    // if (threadIdx.x == 1) {
      int backwardCurId = s_meetId;
      int backwardIteration = 0;
      if (d_nodes[backwardCurId].flags.dst_flag == 1) {
        d_nodes[backwardCurId].flags.dst_flag = 0; // Reset dst flag.
        d_nodes[backwardCurId].flags.src_flag = 1;
      } else {
        while (!d_nodes[backwardCurId].flags.dst_flag && backwardIteration < total) {
          int2 xy = idxToLoc_2D(backwardCurId, xDim);
          uint8_t backwardDir = d_nodes[backwardCurId].backward_direction;
          int nx = xy.x + s_dX[backwardDir];
          int ny = xy.y + s_dY[backwardDir];
          if (nx < LLX || nx > URX || ny < LLY || ny > URY) {
            break;
          }
          int nextId = locToIdx_2D(nx, ny, xDim);
          
          /*
          if (d_nodes[nextId].golden_parent_x != -1 || d_nodes[nextId].golden_parent_y != -1) {
            printf("Error: Backward traceback meets forward traceback.\n");
            printf("meetId = %d\n", s_meetId); 
            for (int localIdx = 0; localIdx < total; localIdx++) {
              int local_x = localIdx % xDimTemp + LLX;
              int local_y = localIdx / xDimTemp + LLY;
              int idx = locToIdx_2D(local_x, local_y, xDim);
              printf("node id = %d, x = %d, y = %d ", idx, local_x, local_y);
              printNode2D(d_nodes[idx]);
            }
            printf("nextId = %d, x = %d, y = %d,  golden_parent_x = %d, golden_parent_y = %d, dst_flag = %d, src_flag = %d\n", 
              nextId, nx, ny, 
              d_nodes[nextId].golden_parent_x, d_nodes[nextId].golden_parent_y, 
              d_nodes[nextId].flags.dst_flag, d_nodes[nextId].flags.src_flag);
            atomicExch(&tracebackError, 1);
            break;      
          } */

          d_nodes[nextId].flags.src_flag = 1;
          d_nodes[nextId].golden_parent_x = xy.x;
          d_nodes[nextId].golden_parent_y = xy.y;
          backwardCurId = nextId;
          backwardIteration++;
        }
        d_nodes[backwardCurId].flags.dst_flag = 0;
        if (backwardIteration >= total) {
          printf("Warning: Backward traceback exceeded maximum iterations.\n");
        }
      }
      // printf("End the traceback\n");

    }
  }
  __syncthreads();
}






// Define the device function for the biwaveBellmanFord_2D_v4__device
__device__
void runBiBellmanFord2D_v5__device(
  int netId,
  NodeData2D* d_nodes,
  uint64_t* d_costMap, 
  const int* __restrict__ d_dX, 
  const int* __restrict__ d_dY,
  const int* __restrict__ d_xCoords,
  const int* __restrict__ d_yCoords,
  int LLX, int LLY, int URX, int URY,
  int xDim, 
  int maxIters,
  float congThreshold,
  unsigned BLOCKCOST,
  unsigned OVERFLOWCOST,
  unsigned HISTCOST)
{
  // Each device function is handled by a single block
  int total = (URX - LLX + 1) * (URY - LLY + 1);
  int tid = threadIdx.x;
  int stride = blockDim.x; 
  int xDimTemp = URX - LLX + 1;
  
  // Define the shared memory for d_dx and d_dy
  __shared__ int s_dX[4];
  __shared__ int s_dY[4];
  __shared__ volatile int s_doneFlag;
  __shared__ volatile int s_minCost;
  //__shared__ volatile int s_meetId;
  __shared__ unsigned long long s_meet;
  __shared__ volatile int tracebackError;   // 0: no error; 1: error detected
  __shared__ int s_minForwardCost;
  __shared__ int s_minBackwardCost;
  
  // Load the d_dX and d_dY into shared memory
  if (tid < 4) {
    s_dX[tid] = d_dX[tid];
    s_dY[tid] = d_dY[tid];
    if (tid == 0) {
      s_doneFlag = 0;
      s_minCost = 0x7FFFFFFF;
      //s_meetId = 0x7FFFFFFF;
      s_meet = 0xFFFFFFFFFFFFFFFFULL;
      tracebackError = 0;
      s_minForwardCost = 0x7FFFFFFF;
      s_minBackwardCost = 0x7FFFFFFF;
    }
  }
  __syncthreads();

  for (int localIdx = tid; localIdx < total; localIdx += stride) {
    int x = localIdx % xDimTemp + LLX;
    int y = localIdx / xDimTemp + LLY;
    int idx = locToIdx_2D(x, y, xDim);
    if (d_nodes[idx].flags.dst_flag) {
      atomicExch((int*)&s_doneFlag, 1);
      break; // Exit early if any destination node is found.
    }
  }
  __syncthreads();

  if (!s_doneFlag) {
    return;
  } 
  
  if (tid == 0) {
    s_doneFlag = 0;
  }

  __syncthreads();

  // We'll do up to maxIters or until no changes / front-meet
  for (int iter = 0; iter < maxIters && (s_doneFlag == 0); iter++)
  {
    bool localFrontsMeet = false;

    ////////////////////////////////////////////////////////////////////////////
    // (1) Forward & backward relaxation phase
    ////////////////////////////////////////////////////////////////////////////
    for (int localIdx = tid; localIdx < total; localIdx += stride) {
      int x = localIdx % xDimTemp + LLX;
      int y = localIdx / xDimTemp + LLY;
      int idx = locToIdx_2D(x, y, xDim);
      NodeData2D &nd = d_nodes[idx];

      // Forward relaxation:
      // Skip if src_flag is set.
      if (!nd.flags.src_flag) {
        uint32_t bestCost = nd.forward_g_cost_prev;
        int      bestD    = -1;
        for (int d = 0; d < 4; d++) {
          int nx = x + s_dX[d];
          int ny = y + s_dY[d];
          if (nx < LLX || nx > URX || ny < LLY || ny > URY) {
            continue;  // out of bounds
          }
          int nbrIdx = locToIdx_2D(nx, ny, xDim);
          uint32_t neighborCost = d_nodes[nbrIdx].forward_g_cost_prev;
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

        if (bestD != -1) { // Found an improvement
          nd.forward_g_cost = bestCost;
          nd.forward_direction = computeParentDirection2D(bestD);
          nd.flags.forward_update_flag = 1;
        }
      } // end forward

      // Backward relaxation:
      // newCost = min over neighbors of (neighbor.backward_cost + edgeWeight).
      // Skip if dst_flag is set.
      if (!nd.flags.dst_flag) {
        uint32_t bestCost = nd.backward_g_cost_prev;
        int      bestD    = -1;
        for (int d = 0; d < 4; d++) {
          int nx = x + s_dX[d];
          int ny = y + s_dY[d];
          if (nx < LLX || nx > URX || ny < LLY || ny > URY) {
            continue;  // out of bounds
          }
          int nbrIdx = locToIdx_2D(nx, ny, xDim);
          uint32_t neighborCost = d_nodes[nbrIdx].backward_g_cost_prev;
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
          nd.flags.backward_update_flag = 1;
        }
      } // end backward
    } // end for each node (relaxation)
    __syncthreads();

    ////////////////////////////////////////////////////////////////////////////
    // (2) Commit updated costs (double-buffering technique)
    ////////////////////////////////////////////////////////////////////////////
    for (int localIdx = tid; localIdx < total; localIdx += stride) {
      int local_x = localIdx % xDimTemp + LLX;
      int local_y = localIdx / xDimTemp + LLY;
      int idx = locToIdx_2D(local_x, local_y, xDim);
      NodeData2D &nd = d_nodes[idx];
      if (nd.flags.forward_update_flag) {
        nd.flags.forward_update_flag = false;
        nd.forward_g_cost_prev = nd.forward_g_cost;
        atomicMin(&s_minForwardCost, nd.forward_g_cost);
      }
      if (nd.flags.backward_update_flag) {
        nd.flags.backward_update_flag = false;
        nd.backward_g_cost_prev = nd.backward_g_cost;
        atomicMin(&s_minBackwardCost, nd.backward_g_cost);
      }
      // Save previous visited flags for later comparisons.
      nd.flags.forward_visited_flag_prev = nd.flags.forward_visited_flag;
      nd.flags.backward_visited_flag_prev = nd.flags.backward_visited_flag;
    }
    __syncthreads();

    ////////////////////////////////////////////////////////////////////////////
    // (3) Mark nodes using parallel reduction to select the minimum cost nodes.
    //     For the forward side, we find the node with the minimum forward_g_cost
    //     (among nodes not already visited) and mark its forward_visited_flag.
    //     Similarly for the backward side.    
    // Alternatively, one could scan all nodes to check for any node that has both flags true.
    for (int localIdx = tid; localIdx < total; localIdx += stride) {
      int local_x = localIdx % xDimTemp + LLX;
      int local_y = localIdx / xDimTemp + LLY;
      int idx = locToIdx_2D(local_x, local_y, xDim);
      NodeData2D &nd = d_nodes[idx];
      if (nd.forward_g_cost_prev <= s_minForwardCost) {
        nd.flags.forward_visited_flag = true;
      }

      if (nd.backward_g_cost_prev <= s_minBackwardCost) {
        nd.flags.backward_visited_flag = true;
      }

      // Check if the forward and backward fronts meet.
      if (nd.flags.forward_visited_flag && nd.flags.backward_visited_flag) {
        localFrontsMeet = true;
      }
    }

    __syncthreads();

    if (localFrontsMeet) {
      atomicExch((int*)&s_doneFlag, 1);
    }
    
    if (tid == 0) {
      s_minForwardCost = 0x7FFFFFFF;
      s_minBackwardCost = 0x7FFFFFFF;
    }
    
    __syncthreads();

  } // end for (iter)

  __syncthreads();

  // Ensure all threads know the doneFlag
  bool converged = (s_doneFlag == 1);
  if (!converged) {
    if (tid == 0) {
      printf("Error! biwaveBellmanFord_2D_v3__device did not converge. doneFlag = false netId = %d\n", netId);
      for (int localIdx = 0; localIdx < total; localIdx++) {
        int local_x = localIdx % xDimTemp + LLX;
        int local_y = localIdx / xDimTemp + LLY;
        int idx = locToIdx_2D(local_x, local_y, xDim);
        printf("node id = %d, x = %d, y = %d ", idx, local_x, local_y);
        printNode2D(d_nodes[idx]);
      }
    }
    __syncthreads();
    return;
  } 
  __syncthreads();  


  /*
  // Identify the meetId corresponding to the minimum cost.
  for (int localIdx = tid; localIdx < total; localIdx += stride) {
    int local_x = localIdx % xDimTemp + LLX;
    int local_y = localIdx / xDimTemp + LLY;
    int idx = locToIdx_2D(local_x, local_y, xDim);
    auto& nd = d_nodes[idx];
    if (nd.flags.forward_visited_flag && nd.flags.backward_visited_flag) {
      atomicMin((int*)&s_meetId, idx);      
    }
  }
  __syncthreads();
  */


  // Iterate over your domain. Assume tid and stride are defined appropriately.
  for (int localIdx = tid; localIdx < total; localIdx += stride) {
    int local_x = localIdx % xDimTemp + LLX;
    int local_y = localIdx / xDimTemp + LLY;
    int idx = locToIdx_2D(local_x, local_y, xDim);
    auto& nd = d_nodes[idx];
  
    // Only consider nodes visited from both directions.
    if (nd.flags.forward_visited_flag && nd.flags.backward_visited_flag) {
      // Assume each node has an integer cost value.
      int cost = nd.forward_g_cost + nd.backward_g_cost;
      // Pack the cost and idx into a 64-bit value.
      // Lower cost (in high bits) gives a lower overall value.
      unsigned long long candidate = (((unsigned long long)cost) << 32) | ((unsigned int)idx);
      atomicMin(&s_meet, candidate);
    }
  }
  __syncthreads();


  // After the loop, the meetId is stored in the lower 32 bits of s_meet.
  int s_meetId = (int)(s_meet & 0xFFFFFFFF);


  // Check if s_meetId is valid.
  if (s_meetId == 0x7FFFFFFF) {
    if (threadIdx.x == 0 || threadIdx.x == 1) {
      printf("Error! biwaveBellmanFord_2D_v3__device did not converge. meetId = 0x7FFFFFFF, netId = %d\n", netId);
    }
    if (threadIdx.x == 0 || threadIdx.x == 1) {
      atomicExch((int*)&tracebackError, 1);
    }
  }
  __syncthreads();
  
  // Only threads 0 and 1 perform the traceback if no error occurred.
  if (tracebackError == 0) {
    // ----- Forward Traceback (Thread 0) -----
    if (threadIdx.x == 0) {
      
      /*
      if (netId == 537) {
        printf("*****************************************************************\n");
        printf("meetId = %d, netId = %d\n", s_meetId, netId);
        for (int idx = 0; idx < total; idx++) {
          int local_x = idx % xDimTemp + LLX;
          int local_y = idx / xDimTemp + LLY;
          int id = locToIdx_2D(local_x, local_y, xDim);
          printf("node id = %d, x = %d, y = %d ", id, local_x, local_y);
          printNode2D(d_nodes[id]);
        }
        printf("\n");
      }*/
      
      
      
      // printf("Start the traceback\n");
      int tempIter = 0;      
      // Update the meetId accordingly to remove redundant path
      while (d_nodes[s_meetId].forward_direction == d_nodes[s_meetId].backward_direction && tempIter < total) {
        if (d_nodes[s_meetId].forward_direction == DIR_NONE) {
          printf("Warning: forward_direction == DIR_NONE netId = %d s_meetId = %d\n", netId, s_meetId);
          for (int idx = 0; idx < total; idx++) {
            int local_x = idx % xDimTemp + LLX;
            int local_y = idx / xDimTemp + LLY;
            int id = locToIdx_2D(local_x, local_y, xDim);
            printf("node id = %d, x = %d, y = %d ", id, local_x, local_y);
            printNode2D(d_nodes[id]);
          }
          break;
        }
        
        int2 xy = idxToLoc_2D(s_meetId, xDim);
        auto direction = d_nodes[s_meetId].forward_direction;
        int nx = xy.x + s_dX[direction];
        int ny = xy.y + s_dY[direction];
        s_meetId = locToIdx_2D(nx, ny, xDim);
        tempIter++;
      }

      if (tempIter >= total) {
        printf("Warning: reduce iteration exceeded maximum iterations. netId = %d\n", netId);
      }
            
      // printf("Start the forward traceback\n");

      int forwardCurId = s_meetId;
      int forwardIteration = 0;
      while (!d_nodes[forwardCurId].flags.src_flag && forwardIteration < total) {
        uint8_t fwdDir = d_nodes[forwardCurId].forward_direction;
        int2 xy = idxToLoc_2D(forwardCurId, xDim);
        int nx = xy.x + s_dX[fwdDir];
        int ny = xy.y + s_dY[fwdDir];
        if (nx < LLX || nx > URX || ny < LLY || ny > URY) {
          break;
        }
        d_nodes[forwardCurId].golden_parent_x = nx;
        d_nodes[forwardCurId].golden_parent_y = ny;
        d_nodes[forwardCurId].flags.src_flag = 1;
        forwardCurId = locToIdx_2D(nx, ny, xDim);
        forwardIteration++;
      }
      if (forwardIteration >= total) {
        printf("Warning: Forward traceback exceeded maximum iterations. netId = %d\n", netId);
      }
    // }
    

     //  printf("Start the backward traceback\n");
    // ----- Backward Traceback (Thread 1) -----
    // if (threadIdx.x == 1) {
      int backwardCurId = s_meetId;
      int backwardIteration = 0;
      if (d_nodes[backwardCurId].flags.dst_flag == 1) {
        d_nodes[backwardCurId].flags.dst_flag = 0; // Reset dst flag.
        d_nodes[backwardCurId].flags.src_flag = 1;
      } else {
        while (!d_nodes[backwardCurId].flags.dst_flag && backwardIteration < total) {
          int2 xy = idxToLoc_2D(backwardCurId, xDim);
          uint8_t backwardDir = d_nodes[backwardCurId].backward_direction;
          int nx = xy.x + s_dX[backwardDir];
          int ny = xy.y + s_dY[backwardDir];
          if (nx < LLX || nx > URX || ny < LLY || ny > URY) {
            break;
          }
          int nextId = locToIdx_2D(nx, ny, xDim);
          
          /*
          if (d_nodes[nextId].golden_parent_x != -1 || d_nodes[nextId].golden_parent_y != -1) {
            printf("Error: Backward traceback meets forward traceback.\n");
            printf("meetId = %d\n", s_meetId); 
            for (int localIdx = 0; localIdx < total; localIdx++) {
              int local_x = localIdx % xDimTemp + LLX;
              int local_y = localIdx / xDimTemp + LLY;
              int idx = locToIdx_2D(local_x, local_y, xDim);
              printf("node id = %d, x = %d, y = %d ", idx, local_x, local_y);
              printNode2D(d_nodes[idx]);
            }
            printf("nextId = %d, x = %d, y = %d,  golden_parent_x = %d, golden_parent_y = %d, dst_flag = %d, src_flag = %d\n", 
              nextId, nx, ny, 
              d_nodes[nextId].golden_parent_x, d_nodes[nextId].golden_parent_y, 
              d_nodes[nextId].flags.dst_flag, d_nodes[nextId].flags.src_flag);
            atomicExch(&tracebackError, 1);
            break;      
          } */
           
          d_nodes[nextId].flags.src_flag = 1;
          d_nodes[nextId].golden_parent_x = xy.x;
          d_nodes[nextId].golden_parent_y = xy.y;
          backwardCurId = nextId;
          backwardIteration++;
        }
        d_nodes[backwardCurId].flags.dst_flag = 0;
        if (backwardIteration >= total) {
          printf("Warning: Backward traceback exceeded maximum iterations. netId = %d\n", netId);
        }
      }
      // printf("End the traceback\n");

    }
  }
  __syncthreads();
}




__device__
void initNodeData2D_v3__device(
  NodeData2D* d_nodes,
  int* d_pins, int pinIterStart, int pinIter,  // Pin related variables
  int LLX, int LLY, int URX, int URY, // Bounding box
  int xDim)
{ 
  int total = (URX - LLX + 1) * (URY - LLY + 1);
  int xDimTemp = URX - LLX + 1;
  int tid = threadIdx.x;
  int stride = blockDim.x;
  
  for (int localIdx = tid; localIdx < total; localIdx += stride) {
    int local_x = localIdx % xDimTemp + LLX;
    int local_y = localIdx / xDimTemp + LLY;
    int idx = locToIdx_2D(local_x, local_y, xDim);
    // int2 xy = idxToLoc_2D(idx, xDim);
    // int2 src = idxToLoc_2D(d_pins[pinIterStart + pinIter - 1], xDim);
    // int2 dst = idxToLoc_2D(d_pins[pinIterStart + pinIter], xDim);
  
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


__device__
void checkConnectivity__device(
  NodeData2D* d_nodes,
  int LLX, int LLY, int URX, int URY, // Bounding box
  int xDim)
{
  int total = (URX - LLX + 1) * (URY - LLY + 1);
  int xDimTemp = URX - LLX + 1;
  int tid = threadIdx.x;
  int stride = blockDim.x;
  
  for (int localIdx = tid; localIdx < total; localIdx += stride) {
    int local_x = localIdx % xDimTemp + LLX;
    int local_y = localIdx / xDimTemp + LLY;
    int idx = locToIdx_2D(local_x, local_y, xDim);
    if (d_nodes[idx].flags.dst_flag) {
      printf("Error: dst_flag is set. idx = %d, x = %d, y = %d\n", idx, local_x, local_y);
    }

    if (d_nodes[idx].golden_parent_x == -1 && d_nodes[idx].golden_parent_y != d_nodes[idx].golden_parent_x) {
      printf("Error: golden_parent_x = %d, golden_parent_y = %d, idx = %d, x = %d, y = %d\n", 
        d_nodes[idx].golden_parent_x, d_nodes[idx].golden_parent_y, idx, local_x, local_y);
    }
  }
}




__device__ 
void biwaveBellmanFord2D_v3__device(
  int netId,
  int* d_netPtr,
  Rect2D_CUDA* d_netBBoxVec,
  int* d_pins,
  uint64_t* d_costMap,
  NodeData2D* d_nodes,
  int* d_dX,
  int* d_dY,
  const int* d_xCoords,
  const int* d_yCoords,
  int maxIters,
  int xDim,
  int yDim,
  float congThreshold,
  int BLOCKCOST,
  int OVERFLOWCOST, 
  int HISTCOST)
{
  // for this net
  int pinIdxStart = d_netPtr[netId];
  int pinIdxEnd = d_netPtr[netId + 1];
  int numPins = pinIdxEnd - pinIdxStart;
  Rect2D_CUDA netBBox = d_netBBoxVec[netId];
  int LLX = netBBox.xMin;
  int LLY = netBBox.yMin;
  int URX = netBBox.xMax;
  int URY = netBBox.yMax;

  for (int pinIter = 1; pinIter < numPins; pinIter++) {
    // Initialize the node data
    initNodeData2D_v3__device(
      d_nodes,
      d_pins, pinIdxStart, pinIter, 
      LLX, LLY, URX, URY, 
      xDim);

    __syncthreads(); // Synchronize all threads in the block

    // Run the Bellman Ford algorithm
    runBiBellmanFord2D_v5__device(
      netId, 
      d_nodes, d_costMap, d_dX, d_dY, d_xCoords, d_yCoords,
      LLX, LLY, URX, URY, xDim, maxIters,
      congThreshold, BLOCKCOST, OVERFLOWCOST, HISTCOST);  

    __syncthreads(); // Synchronize all threads in the block
  }

  // Check the connectivity
  checkConnectivity__device(
    d_nodes,
    LLX, LLY, URX, URY, xDim);
}



__global__ 
void biwaveBellmanFord2D_v3__kernel(
  int netStartId,
  int netEndId,
  int* d_netBatchIdx,
  int* d_netPtr,
  Rect2D_CUDA* d_netBBoxVec,
  int* d_pins,
  uint64_t* d_costMap,
  NodeData2D* d_nodes,
  int* d_dX,
  int* d_dY,
  const int* d_xCoords,
  const int* d_yCoords,
  int maxIters,
  int xDim,
  int yDim,
  float congThreshold,
  int BLOCKCOST,
  int OVERFLOWCOST, 
  int HISTCOST)
{
  // Each net is handled by a single block
  for (int netId = netStartId + blockIdx.x; netId < netEndId; netId += gridDim.x) {
    biwaveBellmanFord2D_v3__device(
      netId,
      d_netPtr,
      d_netBBoxVec,
      d_pins,
      d_costMap,
      d_nodes + d_netBatchIdx[netId] * xDim * yDim,
      d_dX,
      d_dY,
      d_xCoords,
      d_yCoords,
      maxIters,
      xDim,
      yDim,
      congThreshold,
      BLOCKCOST,
      OVERFLOWCOST, 
      HISTCOST);
  }
}



__global__ 
void initBatchNodeData2D_v3__kernel(
  NodeData2D* d_nodes,
  int numNodes)
{
  for (int idx = blockIdx.x * blockDim.x + threadIdx.x; idx < numNodes; idx += gridDim.x * blockDim.x) {
    initNodeData2D(d_nodes[idx]);
  }
}


__global__
void initParent2D__kernel(
  Point2D_CUDA* d_parents,
  int numParents)
{
  for (int idx = blockIdx.x * blockDim.x + threadIdx.x; idx < numParents; idx += gridDim.x * blockDim.x) {
    d_parents[idx].x = -1;
    d_parents[idx].y = -1;
  }
}


__global__
void copyParents2D__kernel(
  NodeData2D* d_nodes,
  Point2D_CUDA* d_parents,
  int numNodes)
{
  for (int idx = blockIdx.x * blockDim.x + threadIdx.x; idx < numNodes; idx += gridDim.x * blockDim.x) {
    d_parents[idx].x = d_nodes[idx].golden_parent_x;
    d_parents[idx].y = d_nodes[idx].golden_parent_y;
  }
}


__global__
void initBatchPin2D_v3__kernel(
  NodeData2D* d_nodes,
  int* d_pins,
  int* d_netPtr,
  int* d_netBatchIdx,
  int netIdStart,
  int netIdEnd,
  int numGrids)
{  
  int tid = blockIdx.x * blockDim.x + threadIdx.x;
  int stride = blockDim.x * gridDim.x;
  int numNets = netIdEnd - netIdStart;
  for (tid; tid < numNets; tid += stride) {
    int netId = netIdStart + tid;
    int batchId = d_netBatchIdx[netId];
    int baseNodeId = batchId * numGrids;
    int pinIdxStart = d_netPtr[netId];
    int pinIdxEnd = d_netPtr[netId + 1];
    if (batchId < 0 || batchId >= 200) {
      printf("Error: Invalid batchId = %d\n", batchId);
    }

    int pinId = d_pins[pinIdxStart] + baseNodeId;
    if (pinId > numGrids * 200) {
      printf("Error: Invalid pinId = %d\n", pinId);
    }

    d_nodes[pinId].flags.src_flag = true;

    for (int pinIter = pinIdxStart + 1; pinIter < pinIdxEnd; pinIter++) {
      pinId = d_pins[pinIter] + baseNodeId;
      if (pinId > numGrids * 200) {
        printf("Error: Invalid pinId = %d\n", pinId);
      }
      d_nodes[pinId].flags.dst_flag = true;
    }
  }
}



float ChunkPathSyncUp(
  std::vector<std::unique_ptr<FlexGRWorker>>& uworkers,
  std::vector<grNet*>& nets,
  std::vector<Rect2D_CUDA>& netBBoxVec,
  std::vector<Point2D_CUDA>& h_parents,
  int netIdStart,
  int netIdEnd,
  int xDim)
{ 
  auto syncupTimeStart = std::chrono::high_resolution_clock::now();
  for (int netId = netIdStart; netId < netIdEnd; netId++) {
    auto& net = nets[netId];
    auto& uworker = uworkers[net->getWorkerId()];
    auto& gridGraph = uworker->getGridGraph();
    auto workerLL = uworkers[net->getWorkerId()]->getRouteGCellIdxLL();
    int workerLX = workerLL.x();
    int workerLY = workerLL.y();  
    auto& netBBox = netBBoxVec[netId];
    int LLX = netBBox.xMin;
    int LLY = netBBox.yMin;
    int URX = netBBox.xMax;
    int URY = netBBox.yMax;
    int xDimTemp = URX - LLX + 1;
    int numNodes = (URX - LLX + 1) * (URY - LLY + 1);
    for (int localIdx = 0; localIdx < numNodes; localIdx++) {
      int localX = localIdx % xDimTemp;
      int localY = localIdx / xDimTemp;

      int x = localX + LLX;
      int y = localY + LLY;      
      int idx = locToIdx_2D(x, y, xDim);

      x -= workerLX;
      y -= workerLY;

      int parentX = h_parents[idx].x - workerLX;
      int parentY = h_parents[idx].y - workerLY;

      //int parentX = nodes[idx].golden_parent_x - workerLX;
      //int parentY = nodes[idx].golden_parent_y - workerLY;

      gridGraph.setGoldenParent2D(x, y, parentX, parentY);      
      /*
      if (nodes[idx].golden_parent_x != -1 || nodes[idx].golden_parent_y != -1) {
        std::cout << "Net " << netId << " x = " << x << " y = " << y << " "
                  << "Parent " << parentX << "  " << parentY << std::endl;
      }
      */
    }    
  }

  auto syncupTimeEnd = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double, std::milli> syncupTime = syncupTimeEnd - syncupTimeStart;
  return syncupTime.count();
}


void FlexGR::allocateCUDAMem(
  std::vector<uint64_t>& h_costMap,
  std::vector<int>& h_xCoords,
  std::vector<int>& h_yCoords,
  std::vector<Point2D_CUDA>& h_parents,
  std::vector<int>& pinIdxVec,
  std::vector<int>& netPtr,
  std::vector<Rect2D_CUDA>& netBBoxVec,
  std::vector<int>& netBatchIdxVec,
  int numGrids,
  int numNodes)
{  
  // We have defined the following variables
  // h_costMap_size_
  // h_xCoords_size_
  // h_yCoords_size_
  // h_parents_size_
  // h_pinIdxVec_size_
  // h_netPtr_size_
  // h_netBBoxVec_size_
  // h_netBatchIdxVec_size_ 
  // h_nodes_size_

  if (d_dX_ == nullptr) {
    std::vector<int> h_dX = {0, 1, 0, -1};
    std::vector<int> h_dY = {1, 0, -1, 0};
    cudaMalloc(&d_dX_, 4 * sizeof(int));
    cudaMalloc(&d_dY_, 4 * sizeof(int));
    cudaMemcpy(d_dX_, h_dX.data(), 4 * sizeof(int), cudaMemcpyHostToDevice);
    cudaMemcpy(d_dY_, h_dY.data(), 4 * sizeof(int), cudaMemcpyHostToDevice);
  }

  if (h_xCoords.size() > h_costMap_size_) {
    h_xCoords_size_ = h_xCoords.size();
    cudaFree(d_xCoords_);
    cudaMalloc(&d_xCoords_, h_xCoords.size() * sizeof(int));
    cudaMemcpy(d_xCoords_, h_xCoords.data(), h_xCoords.size() * sizeof(int), cudaMemcpyHostToDevice); 
  }

  if (h_yCoords.size() > h_costMap_size_) {
    h_yCoords_size_ = h_yCoords.size();
    cudaFree(d_yCoords_);
    cudaMalloc(&d_yCoords_, h_yCoords.size() * sizeof(int));
    cudaMemcpy(d_yCoords_, h_yCoords.data(), h_yCoords.size() * sizeof(int), cudaMemcpyHostToDevice);
  }

  if (h_costMap.size() > h_costMap_size_) {
    h_costMap_size_ = h_costMap.size();
    cudaFree(d_costMap_);
    cudaMalloc(&d_costMap_, h_costMap.size() * sizeof(uint64_t));
  }
  cudaMemcpy(d_costMap_, h_costMap.data(), h_costMap.size() * sizeof(uint64_t), cudaMemcpyHostToDevice);

  if (h_parents.size() > h_parents_size_) {
    h_parents_size_ = h_parents.size();
    cudaFree(d_parents_);
    cudaMalloc(&d_parents_, h_parents.size() * sizeof(Point2D_CUDA));
  }
  //cudaMemcpy(d_parents_, h_parents.data(), h_parents.size() * sizeof(Point2D_CUDA), cudaMemcpyHostToDevice);

  if (pinIdxVec.size() > h_pinIdxVec_size_) {
    h_pinIdxVec_size_ = pinIdxVec.size();
    cudaFree(d_pinIdxVec_);
    cudaMalloc(&d_pinIdxVec_, pinIdxVec.size() * sizeof(int));
  }
  cudaMemcpy(d_pinIdxVec_, pinIdxVec.data(), pinIdxVec.size() * sizeof(int), cudaMemcpyHostToDevice);

  if (netPtr.size() > h_netPtr_size_) {
    h_netPtr_size_ = netPtr.size();
    cudaFree(d_netPtr_);
    cudaMalloc(&d_netPtr_, netPtr.size() * sizeof(int));
  }
  cudaMemcpy(d_netPtr_, netPtr.data(), netPtr.size() * sizeof(int), cudaMemcpyHostToDevice);

  if (netBBoxVec.size() > h_netBBoxVec_size_) {
    h_netBBoxVec_size_ = netBBoxVec.size();
    cudaFree(d_netBBox_);
    cudaMalloc(&d_netBBox_, netBBoxVec.size() * sizeof(Rect2D_CUDA));
  }
  cudaMemcpy(d_netBBox_, netBBoxVec.data(), netBBoxVec.size() * sizeof(Rect2D_CUDA), cudaMemcpyHostToDevice);

  if (netBatchIdxVec.size() > h_netBatchIdxVec_size_) {
    h_netBatchIdxVec_size_ = netBatchIdxVec.size();
    cudaFree(d_netBatchIdx_);
    cudaMalloc(&d_netBatchIdx_, netBatchIdxVec.size() * sizeof(int));
  }
  cudaMemcpy(d_netBatchIdx_, netBatchIdxVec.data(), netBatchIdxVec.size() * sizeof(int), cudaMemcpyHostToDevice);

  if (numNodes > h_nodes_size_) {
    h_nodes_size_ = numNodes;
    cudaFree(d_nodes_);
    cudaMalloc(&d_nodes_, numNodes * sizeof(NodeData2D));
  }
  cudaCheckError();
}



void FlexGR::freeCUDAMem()
{
  cudaFree(d_dX_);
  cudaFree(d_dY_);
  cudaFree(d_costMap_);
  cudaFree(d_xCoords_);
  cudaFree(d_yCoords_);
  cudaFree(d_nodes_);
  cudaFree(d_parents_);
  cudaFree(d_pinIdxVec_);
  cudaFree(d_netPtr_);
  cudaFree(d_netBBox_);
  cudaFree(d_netBatchIdx_);

  d_dX_ = nullptr;
  d_dY_ = nullptr;
  d_costMap_ = nullptr;
  d_xCoords_ = nullptr;
  d_yCoords_ = nullptr;
  d_nodes_ = nullptr;
  d_parents_ = nullptr;
  d_pinIdxVec_ = nullptr;
  d_netPtr_ = nullptr;
  d_netBBox_ = nullptr;
  d_netBatchIdx_ = nullptr;

  h_costMap_size_ = 0;
  h_xCoords_size_ = 0;
  h_yCoords_size_ = 0;
  h_parents_size_ = 0;
  h_pinIdxVec_size_ = 0;
  h_netPtr_size_ = 0;
  h_netBBoxVec_size_ = 0;
  h_netBatchIdxVec_size_ = 0;
  h_nodes_size_ = 0;

  cudaCheckError();
}


float FlexGR::GPUAccelerated2DMazeRoute_update_v3(
  std::vector<std::unique_ptr<FlexGRWorker> >& uworkers,
  std::vector<std::vector<grNet*> >& netBatches,
  std::vector<int>& validBatches,
  std::vector<Point2D_CUDA>& h_parents,
  std::vector<uint64_t>& h_costMap,
  std::vector<int>& h_xCoords,
  std::vector<int>& h_yCoords,
  RouterConfiguration* router_cfg,
  float relaxThreshold,
  float congThreshold,
  int xDim, int yDim)
{
  // Start overall timing.
  auto totalStart = std::chrono::high_resolution_clock::now();
  int numGrids = xDim * yDim;
  int numBatches = validBatches.size();  
  
  if (VERBOSE > 0) {
    std::cout << "[INFO] Number of batches: " << numBatches << std::endl;
  }
  
  if (numBatches == 0) {
    return 0.0;
  }

  // Set the GPU device to 1.
  cudaSetDevice(1);

  std::vector<Point2D_CUDA> netVec;
  std::vector<int> netPtr;
  std::vector<int> netBatchIdxVec; 
  std::vector<Rect2D_CUDA> netBBoxVec;
  std::vector<int> pinIdxVec;
  int maxHPWL = 0; // We will run the algorithm for maxHPWL * relaxThreshold iteratively
  
  // We need to divide the batches into multiple chunks due to the memory limitation
  int maxChunkSize = 200;  // Basically we allows 200 batches to be processed in one chunk
  // For testing 
  // int maxChunkSize = 1;
  
  std::vector<int> chunkNetPtr; // store the first netIdx of each chunk
  
  if (VERBOSE > 0) {
    std::cout << "[INFO] Max chunk size: " << maxChunkSize << std::endl;
  }

  netPtr.push_back(0);
  chunkNetPtr.push_back(0);

  int maxBatchSize = 0;
  int minBatchSize = std::numeric_limits<int>::max();

  int batchChunkIdx = 0;
  for (int batchIdx = 0; batchIdx < numBatches; batchIdx++) {
    auto& batch = netBatches[validBatches[batchIdx]];
    for (auto& net : batch) {
      for (auto& idx : net->getPinGCellAbsIdxs()) {
        netVec.push_back(Point2D_CUDA(idx.x(), idx.y()));
        pinIdxVec.push_back(locToIdx_2D(idx.x(), idx.y(), xDim));
      }
      netBatchIdxVec.push_back(batchChunkIdx);
      netPtr.push_back(netVec.size());
      auto netBBox = net->getRouteAbsBBox();
      netBBoxVec.push_back(
        Rect2D_CUDA(netBBox.xMin(), netBBox.yMin(), netBBox.xMax(), netBBox.yMax()));
      // maxHPWL = std::max(maxHPWL, net->getHPWL());
      maxHPWL = std::max(maxHPWL, static_cast<int>((netBBox.xMax() - netBBox.xMin()) * (netBBox.yMax() - netBBox.yMin())));
    }
   
    batchChunkIdx++;

    if (batchChunkIdx % maxChunkSize == 0) {
      chunkNetPtr.push_back(netPtr.size() - 1);
      batchChunkIdx = 0;
    }
    
    maxBatchSize = std::max(maxBatchSize, static_cast<int>(batch.size()));
    minBatchSize = std::min(minBatchSize, static_cast<int>(batch.size()));
  }
 
  if (batchChunkIdx != 0) {
    chunkNetPtr.push_back(netPtr.size() - 1);
  }

  int numNets = static_cast<int>(netBBoxVec.size());
  int numChunks = static_cast<int>(chunkNetPtr.size()) - 1;
  
  if (numChunks > numBatches) {
    std::cout << "[ERROR] Number of chunks is larger than the number of batches." << std::endl;
    exit(1);
    return 0.0;
  }  
  
  int chunkSize = std::min(maxChunkSize, numBatches);
  int maxIters = static_cast<int>(maxHPWL * relaxThreshold);
  int numNodes = numGrids * chunkSize;

  if (VERBOSE > 0) {
    std::cout << "[INFO] Number of nets: " << numNets << std::endl;
    std::cout << "[INFO] Number of chunks: " << numChunks << std::endl;
    std::cout << "[INFO] Chunk size: " << chunkSize << std::endl; 
    std::cout << "[INFO] Max batch size: " << maxBatchSize << std::endl;
    std::cout << "[INFO] Min batch size: " << minBatchSize << std::endl;
    std::cout << "[INFO] Max HPWL: " << maxHPWL << std::endl;
    std::cout << "[INFO] Max iterations: " << maxIters << std::endl;
    std::cout << "[INFO] Number of nodes: " << numNodes << std::endl;
    std::cout << "[INFO] Number of grids: " << numGrids << std::endl;
  }

  for (int i = 0; i < netBatchIdxVec.size(); i++) {
    if (netBatchIdxVec[i] < 0 || netBatchIdxVec[i] >= chunkSize) {
      std::cout << "[ERROR] Net " << i << " is in batch " << netBatchIdxVec[i] << std::endl;
    }
  }


  allocateCUDAMem(
    h_costMap,
    h_xCoords,
    h_yCoords,
    h_parents,
    pinIdxVec,
    netPtr,
    netBBoxVec,
    netBatchIdxVec,
    numGrids, 
    numNodes);


  if (VERBOSE > 0) {
    std::cout << "[INFO] Device memory allocation is done." << std::endl;
  }

  // According to the original code
  unsigned BLOCKCOST = router_cfg->BLOCKCOST * 100;
  unsigned OVERFLOWCOST = 128;
  unsigned HISTCOST = 4;
 
  for (int chunkId = 0; chunkId < numChunks; chunkId++) {
    int netStartId = chunkNetPtr[chunkId];
    int netEndId = chunkNetPtr[chunkId + 1];
    
    // Perform Global Initialization
    int numThreads = 1024;
    int numBatchBlocks = (numNodes + numThreads - 1) / numThreads;
    
    int numParentsBlocks = (h_parents_size_ + numThreads - 1) / numThreads;
    initParent2D__kernel<<<numParentsBlocks, numThreads>>>(d_parents_, h_parents.size());

    initBatchNodeData2D_v3__kernel<<<numBatchBlocks, numThreads>>>(
      d_nodes_, 
      numNodes);
    cudaDeviceSynchronize();

    cudaCheckError();

  
    int numNets = netEndId - netStartId;
    if (VERBOSE > 0) {
      std::cout << "[INFO] Chunk " << chunkId << " has " << numNets << " nets." << std::endl;
    }
  
    int numNetBlocks = (numNets + numThreads - 1) / numThreads;
    initBatchPin2D_v3__kernel<<<numNetBlocks, numThreads>>>(
      d_nodes_,
      d_pinIdxVec_, 
      d_netPtr_,
      d_netBatchIdx_,
      netStartId,
      netEndId,
      numGrids);
    cudaDeviceSynchronize();
  

    cudaCheckError();
    // std::cout << "[INFO] Initialization is done." << std::endl;

    //int numThreads = 1024;
    auto netRouteStart = std::chrono::high_resolution_clock::now();
    int numBlocks = numNets;
    biwaveBellmanFord2D_v3__kernel<<<numBlocks, numThreads>>>(
      netStartId,
      netEndId,
      d_netBatchIdx_,
      d_netPtr_,
      d_netBBox_,
      d_pinIdxVec_,
      d_costMap_,
      d_nodes_,
      d_dX_,
      d_dY_,
      d_xCoords_,
      d_yCoords_,
      maxIters,
      xDim,
      yDim,
      congThreshold,
      BLOCKCOST,
      OVERFLOWCOST,
      HISTCOST);
    
    std::cout << "Congestion threshold: " << congThreshold << std::endl;

    cudaCheckError();
    
    cudaDeviceSynchronize();
    auto netRouteEnd = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> netRouteTime = netRouteEnd - netRouteStart;
   
    // copy the back results to the d_parents
    copyParents2D__kernel<<<numBatchBlocks, numThreads>>>(
      d_nodes_, 
      d_parents_, 
      numNodes);

    cudaDeviceSynchronize();
    cudaCheckError();


    cudaMemcpy(h_parents.data(), d_parents_, h_parents.size() * sizeof(Point2D_CUDA), cudaMemcpyDeviceToHost);

    // Check the parents
    cudaCheckError();
  }  
 
  for (auto& parent : h_parents) {
    if (parent.x < 0 || parent.y < 0) {
      if (parent.x != -1 || parent.y != -1) {
        std::cout << "[ERROR] Invalid parent: " << parent.x << " " << parent.y << std::endl;
      } 
    }
  }  

  cudaCheckError();
  
  if (VERBOSE > 0) {
    std::cout << "[INFO] Kernel execution is done." << std::endl;
  }

  auto totalEnd = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double, std::milli> totalTime = totalEnd - totalStart;
  return totalTime.count();
}



// In the V3 version, we will use node-level parallelism directly
// Basically, we do not do the batch-level synchronization anymore
// Just let the tool run for maxHPWL * value iteratively
// And see how it works
float FlexGR::GPUAccelerated2DMazeRoute_update_v3_old(
  std::vector<std::unique_ptr<FlexGRWorker> >& uworkers,
  std::vector<std::vector<grNet*> >& netBatches,
  std::vector<int>& validBatches,
  std::vector<Point2D_CUDA>& h_parents,
  std::vector<uint64_t>& h_costMap,
  std::vector<int>& h_xCoords,
  std::vector<int>& h_yCoords,
  RouterConfiguration* router_cfg,
  float relaxThreshold,
  float congThreshold,
  int xDim, int yDim)
{
  // Start overall timing.
  auto totalStart = std::chrono::high_resolution_clock::now();
  int numGrids = xDim * yDim;
  int numBatches = validBatches.size();  
  
  if (VERBOSE > 0) {
    std::cout << "[INFO] Number of batches: " << numBatches << std::endl;
  }
  
  if (numBatches == 0) {
    return 0.0;
  }

  // Set the GPU device to 1.
  cudaSetDevice(1);

  std::vector<Point2D_CUDA> netVec;
  std::vector<int> netPtr;
  std::vector<int> netBatchIdxVec; 
  std::vector<Rect2D_CUDA> netBBoxVec;
  std::vector<int> pinIdxVec;
  int maxHPWL = 0; // We will run the algorithm for maxHPWL * relaxThreshold iteratively
  
  // We need to divide the batches into multiple chunks due to the memory limitation
  int maxChunkSize = 200;  // Basically we allows 200 batches to be processed in one chunk
  // For testing 
  // int maxChunkSize = 1;
  
  std::vector<int> chunkNetPtr; // store the first netIdx of each chunk
  
  if (VERBOSE > 0) {
    std::cout << "[INFO] Max chunk size: " << maxChunkSize << std::endl;
  }

  netPtr.push_back(0);
  chunkNetPtr.push_back(0);

  int maxBatchSize = 0;
  int minBatchSize = std::numeric_limits<int>::max();

  int batchChunkIdx = 0;
  for (int batchIdx = 0; batchIdx < numBatches; batchIdx++) {
    auto& batch = netBatches[validBatches[batchIdx]];
    for (auto& net : batch) {
      for (auto& idx : net->getPinGCellAbsIdxs()) {
        netVec.push_back(Point2D_CUDA(idx.x(), idx.y()));
        pinIdxVec.push_back(locToIdx_2D(idx.x(), idx.y(), xDim));
      }
      netBatchIdxVec.push_back(batchChunkIdx);
      netPtr.push_back(netVec.size());
      auto netBBox = net->getRouteAbsBBox();
      netBBoxVec.push_back(
        Rect2D_CUDA(netBBox.xMin(), netBBox.yMin(), netBBox.xMax(), netBBox.yMax()));
      // maxHPWL = std::max(maxHPWL, net->getHPWL());
      maxHPWL = std::max(maxHPWL, static_cast<int>((netBBox.xMax() - netBBox.xMin()) * (netBBox.yMax() - netBBox.yMin())));
    }
   
    batchChunkIdx++;

    if (batchChunkIdx % maxChunkSize == 0) {
      chunkNetPtr.push_back(netPtr.size() - 1);
      batchChunkIdx = 0;
    }
    
    maxBatchSize = std::max(maxBatchSize, static_cast<int>(batch.size()));
    minBatchSize = std::min(minBatchSize, static_cast<int>(batch.size()));
  }
 
  if (batchChunkIdx != 0) {
    chunkNetPtr.push_back(netPtr.size() - 1);
  }

  int numNets = static_cast<int>(netBBoxVec.size());
  int numChunks = static_cast<int>(chunkNetPtr.size()) - 1;
  
  if (numChunks > numBatches) {
    std::cout << "[ERROR] Number of chunks is larger than the number of batches." << std::endl;
    exit(1);
    return 0.0;
  }
  
  
  int chunkSize = std::min(maxChunkSize, numBatches);
  int maxIters = static_cast<int>(maxHPWL * relaxThreshold);
  int numNodes = numGrids * chunkSize;


  // if (VERBOSE > 0) {
  if (1) {
    std::cout << "[INFO] Number of nets: " << numNets << std::endl;
    std::cout << "[INFO] Number of chunks: " << numChunks << std::endl;
    std::cout << "[INFO] Chunk size: " << chunkSize << std::endl; 
    std::cout << "[INFO] Max batch size: " << maxBatchSize << std::endl;
    std::cout << "[INFO] Min batch size: " << minBatchSize << std::endl;
    std::cout << "[INFO] Max HPWL: " << maxHPWL << std::endl;
    std::cout << "[INFO] Max iterations: " << maxIters << std::endl;
    std::cout << "[INFO] Number of nodes: " << numNodes << std::endl;
    std::cout << "[INFO] Number of grids: " << numGrids << std::endl;
  }

  for (int i = 0; i < netBatchIdxVec.size(); i++) {
    if (netBatchIdxVec[i] < 0 || netBatchIdxVec[i] >= chunkSize) {
      std::cout << "[ERROR] Net " << i << " is in batch " << netBatchIdxVec[i] << std::endl;
    }
  }

  // We need to define the needed utility variables
  std::vector<int> h_dX = {0, 1, 0, -1};
  std::vector<int> h_dY = {1, 0, -1, 0};
 
  /*
  int* d_dX = nullptr;
  int* d_dY = nullptr;

  // For the design specific variables (numGrids)
  uint64_t* d_costMap = nullptr;
  int* d_xCoords = nullptr;
  int* d_yCoords = nullptr;  
  // For the chunk specific variables
  NodeData2D* d_nodes = nullptr; // (numGrids * chunkSize);
  
  // Point2D_CUDA* d_parents = nullptr;
  int* d_pinIdxVec = nullptr;
  int* d_netPtr = nullptr;
  Rect2D_CUDA* d_netBBox = nullptr;
  int* d_netBatchIdx = nullptr;
  Point2D_CUDA* d_parents = nullptr;
  */

 


  
  cudaCheckError();

  /*
  allocateCUDAMem(
    d_dX_,
    d_dY_,
    d_costMap_,
    d_xCoords_,
    d_yCoords_,
    d_nodes_,
    d_parents_,
    d_pinIdxVec_,
    d_netPtr_,
    d_netBBox_,
    d_netBatchIdx_,
    h_costMap,
    h_xCoords,
    h_yCoords,
    h_parents,
    pinIdxVec,
    netPtr,
    netBBoxVec,
    netBatchIdxVec,
    numGrids, 
    numNodes);
  */

  int* d_dX = d_dX_;
  int* d_dY = d_dY_;
  uint64_t* d_costMap = d_costMap_;
  int* d_xCoords = d_xCoords_;
  int* d_yCoords = d_yCoords_;
  Point2D_CUDA* d_parents = d_parents_;
  int* d_pinIdxVec = d_pinIdxVec_;
  int* d_netPtr = d_netPtr_;
  Rect2D_CUDA* d_netBBox = d_netBBox_;
  int* d_netBatchIdx = d_netBatchIdx_;
  NodeData2D* d_nodes = d_nodes_;
   
  cudaCheckError();

  std::cout << "Finish Memory Allocation" << std::endl;




  /*  
  allocateCUDAMem(
    d_dX_,
    d_dY_,
    d_costMap_,
    d_xCoords_,
    d_yCoords_,
    d_nodes_,
    d_parents_,
    d_pinIdxVec_,
    d_netPtr_,
    d_netBBox_,
    d_netBatchIdx_,
    h_costMap,
    h_xCoords,
    h_yCoords,
    h_parents,
    pinIdxVec,
    netPtr,
    netBBoxVec,
    netBatchIdxVec,
    numNodes);

  int* d_dX = d_dX_;
  int* d_dY = d_dY_;

  uint64_t* d_costMap = d_costMap_;
  int* d_xCoords = d_xCoords_;
  int* d_yCoords = d_yCoords_;
  Point2D_CUDA* d_parents = d_parents_;
  int* d_pinIdxVec = d_pinIdxVec_;
  int* d_netPtr = d_netPtr_;
  Rect2D_CUDA* d_netBBox = d_netBBox_;
  int* d_netBatchIdx = d_netBatchIdx_;
  NodeData2D* d_nodes = d_nodes_;
 
    
  if (d_dX == nullptr) {
    std::vector<int> h_dX = {0, 1, 0, -1};
    std::vector<int> h_dY = {1, 0, -1, 0};
    int* d_dX = nullptr;
    int* d_dY = nullptr;
    cudaMalloc(&d_dX, 4 * sizeof(int));
    cudaMalloc(&d_dY, 4 * sizeof(int));
    cudaMemcpy(d_dX, h_dX.data(), 4 * sizeof(int), cudaMemcpyHostToDevice);
    cudaMemcpy(d_dY, h_dY.data(), 4 * sizeof(int), cudaMemcpyHostToDevice);
    cudaCheckError();
  }

  if (h_xCoords.size() > h_xCoords_size_) {
    h_xCoords_size_ = h_xCoords.size();
    cudaFree(d_xCoords);
    cudaMalloc(&d_xCoords, h_xCoords.size() * sizeof(int));
    cudaMemcpy(d_xCoords, h_xCoords.data(), h_xCoords.size() * sizeof(int), cudaMemcpyHostToDevice);
    cudaCheckError();
  }

  if (h_yCoords.size() > h_yCoords_size_) {
    h_yCoords_size_ = h_yCoords.size();
    cudaFree(d_yCoords);
    cudaMalloc(&d_yCoords, h_yCoords.size() * sizeof(int));
    cudaMemcpy(d_yCoords, h_yCoords.data(), h_yCoords.size() * sizeof(int), cudaMemcpyHostToDevice);
    cudaCheckError();
  }

  if (h_costMap.size() > h_costMap_size_) {
    h_costMap_size_ = h_costMap.size();
    cudaFree(d_costMap);
    cudaMalloc(&d_costMap, h_costMap.size() * sizeof(uint64_t));
  }
  cudaMemcpy(d_costMap, h_costMap.data(), h_costMap.size() * sizeof(uint64_t), cudaMemcpyHostToDevice);

  if (h_parents.size() > h_parents_size_) {
    h_parents_size_ = h_parents.size();
    cudaFree(d_parents);
    cudaMalloc(&d_parents, h_parents.size() * sizeof(Point2D_CUDA));
  }
  cudaMemcpy(d_parents, h_parents.data(), h_parents.size() * sizeof(Point2D_CUDA), cudaMemcpyHostToDevice);

  if (pinIdxVec.size() > h_pinIdxVec_size_) {
    h_pinIdxVec_size_ = pinIdxVec.size();
    cudaFree(d_pinIdxVec);
    cudaMalloc(&d_pinIdxVec, pinIdxVec.size() * sizeof(int));
  }
  cudaMemcpy(d_pinIdxVec, pinIdxVec.data(), pinIdxVec.size() * sizeof(int), cudaMemcpyHostToDevice);

  if (netPtr.size() > h_netPtr_size_) {
    h_netPtr_size_ = netPtr.size();
    cudaFree(d_netPtr);
    cudaMalloc(&d_netPtr, netPtr.size() * sizeof(int));
  }
  cudaMemcpy(d_netPtr, netPtr.data(), netPtr.size() * sizeof(int), cudaMemcpyHostToDevice);
  

  if (netBBoxVec.size() > h_netBBoxVec_size_) {
    h_netBBoxVec_size_ = netBBoxVec.size();
    cudaFree(d_netBBox);
    cudaMalloc(&d_netBBox, netBBoxVec.size() * sizeof(Rect2D_CUDA));
  }
  cudaMemcpy(d_netBBox, netBBoxVec.data(), netBBoxVec.size() * sizeof(Rect2D_CUDA), cudaMemcpyHostToDevice);

  if (netBatchIdxVec.size() > h_netBatchIdxVec_size_) {
    h_netBatchIdxVec_size_ = netBatchIdxVec.size();
    cudaFree(d_netBatchIdx);
    cudaMalloc(&d_netBatchIdx, netBatchIdxVec.size() * sizeof(int));
  }
  cudaMemcpy(d_netBatchIdx, netBatchIdxVec.data(), netBatchIdxVec.size() * sizeof(int), cudaMemcpyHostToDevice);

  
  if (numNodes > h_nodes_size_) {
    h_nodes_size_ = numNodes;
    std::cout << "h_nodes_size_ = " << h_nodes_size_ << std::endl;
    cudaFree(d_nodes);
    cudaMalloc(&d_nodes, numNodes * sizeof(NodeData2D));
  }

  cudaCheckError();

  if (h_nodes_size_ != h_parents_size_) {
    std::cout << "Error: h_nodes_size_ != h_parents_size_" << std::endl;
    exit(1);
  }

  std::cout << "[INFO] Number of nodes = " << numNodes << std::endl;
  std::cout << "[INFO] Number of parents = " << h_parents.size() << std::endl;
  std::cout << "[INFO] h_nodes_size_ = " << h_nodes_size_ << std::endl;
  std::cout << "[INFO] h_parents_size_ = " << h_parents_size_ << std::endl;
  */



  if (VERBOSE > 0) {
    std::cout << "[INFO] Device memory allocation is done." << std::endl;
  }

  // According to the original code
  unsigned BLOCKCOST = router_cfg->BLOCKCOST * 100;
  unsigned OVERFLOWCOST = 128;
  unsigned HISTCOST = 4;
 
  for (int chunkId = 0; chunkId < numChunks; chunkId++) {
    int netStartId = chunkNetPtr[chunkId];
    int netEndId = chunkNetPtr[chunkId + 1];
    
    // Perform Global Initialization
    int numThreads = 1024;
    int numBatchBlocks = (numNodes + numThreads - 1) / numThreads;
    
    initBatchNodeData2D_v3__kernel<<<numBatchBlocks, numThreads>>>(
      d_nodes, 
      numNodes);
    cudaDeviceSynchronize();

    cudaCheckError();

    std::cout << "Test a " << std::endl;

  
    int numNets = netEndId - netStartId;
    if (VERBOSE > 0) {
      std::cout << "[INFO] Chunk " << chunkId << " has " << numNets << " nets." << std::endl;
    }
  
    int numNetBlocks = (numNets + numThreads - 1) / numThreads;
    initBatchPin2D_v3__kernel<<<numNetBlocks, numThreads>>>(
      d_nodes,
      d_pinIdxVec, 
      d_netPtr,
      d_netBatchIdx,
      netStartId,
      netEndId,
      numGrids);
    cudaDeviceSynchronize();
  

    cudaCheckError();
    // std::cout << "[INFO] Initialization is done." << std::endl;

    std::cout << "Test b " << std::endl;

    //int numThreads = 1024;
    auto netRouteStart = std::chrono::high_resolution_clock::now();
    int numBlocks = numNets;
    biwaveBellmanFord2D_v3__kernel<<<numBlocks, numThreads>>>(
      netStartId,
      netEndId,
      d_netBatchIdx,
      d_netPtr,
      d_netBBox,
      d_pinIdxVec,
      d_costMap,
      d_nodes,
      d_dX,
      d_dY,
      d_xCoords,
      d_yCoords,
      maxIters,
      xDim,
      yDim,
      congThreshold,
      BLOCKCOST,
      OVERFLOWCOST,
      HISTCOST);
    
    cudaCheckError();
    
    cudaDeviceSynchronize();
    auto netRouteEnd = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> netRouteTime = netRouteEnd - netRouteStart;

    std::cout << "Test c" << std::endl;
   
    /*
    int numPins = netPtr[netStartId + 1] - netPtr[netStartId];
    int HPWL = (netBBoxVec[netStartId].xMax - netBBoxVec[netStartId].xMin) + 
               (netBBoxVec[netStartId].yMax - netBBoxVec[netStartId].yMin);

    std::cout << "[INFO] Net: HPWL = " << HPWL << "  "
              << "numPins = " << numPins << " "
              << "netRouteTime = " << netRouteTime.count() << " ms" << std::endl;
    */

    // copy the back results to the d_parents
    copyParents2D__kernel<<<numBatchBlocks, numThreads>>>(
      d_nodes, 
      d_parents, 
      numNodes);

    cudaDeviceSynchronize();
    cudaCheckError();

    std::cout << "Test d" << std::endl;

    cudaMemcpy(h_parents.data(), d_parents, h_parents.size() * sizeof(Point2D_CUDA), cudaMemcpyDeviceToHost);

    // Check the parents
    cudaCheckError();

    /*
    ChunkPathSyncUp(
      uworkers,
      netBatches[validBatches[0]],
      netBBoxVec,
      h_parents,
      netStartId,
      netEndId,
      xDim);
    */
  }  
  
  cudaCheckError();
  
  if (VERBOSE > 0) {
    std::cout << "[INFO] Kernel execution is done." << std::endl;
  }

  /*
  // Clear the memory
  cudaFree(d_dX);
  cudaFree(d_dY);
  cudaFree(d_costMap);
  cudaFree(d_xCoords);
  cudaFree(d_yCoords);
  cudaFree(d_nodes);
  cudaFree(d_parents);
  cudaFree(d_pinIdxVec);
  cudaFree(d_netPtr);
  cudaFree(d_netBBox);
  cudaFree(d_netBatchIdx);
  cudaCheckError();
  */

  auto totalEnd = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double, std::milli> totalTime = totalEnd - totalStart;
  return totalTime.count();
}



// ------------------------------------------------------------------------------------



// Define the device function for the biwaveBellmanFord2D
__device__
void runBiBellmanFord_2D__device(
  cooperative_groups::grid_group& g,   // grid-level cooperative group
  NodeData2D* nodes,
  uint64_t* d_costMap, 
  int* d_dX, int* d_dY,
  int* d_doneFlag,
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
  maxIters = total;

  /*
  if (tid == 0) 
  { 
    printf("total = %d\n", total);
    for (int id = 0; id < total; id++) {
      int local_x = id % xDimTemp + LLX;
      int local_y = id / xDimTemp + LLY;
      int idx = locToIdx_2D(local_x, local_y, xDim);
      NodeData2D &nd = nodes[idx];
      if (nd.flags.src_flag == 1) {
        printf("device src_flag : id = %d, x = %d, y = %d cost = %d\n", idx, local_x, local_y, nd.forward_g_cost_prev);
      }
      
      if (nd.flags.dst_flag == 1) {
        printf("device dst_flag : id = %d, x = %d, y = %d, cost = %d\n", 
          idx, local_x, local_y, nd.backward_g_cost_prev);
      }
    }
  }
  */

  g.sync();

  bool globalDone = false;

  // We’ll do up to maxIters or until no changes / front-meet
  for (int iter = 0; iter < maxIters && !globalDone; iter++)
  {
    bool localFrontsMeet = false;
    ////////////////////////////////////////////////////////////////////////////
    // (1) Forward & backward relaxation phase
    ////////////////////////////////////////////////////////////////////////////
    for (int localIdx = tid; localIdx < total; localIdx += stride) {
      int x = localIdx % xDimTemp + LLX;
      int y = localIdx / xDimTemp + LLY;
      int idx = locToIdx_2D(x, y, xDim);
      NodeData2D &nd = nodes[idx];

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

          //printf("id = %d, x = %d, y = %d, d = %d, newG = %d, bestCost = %d, bestD = %d\n", 
          //  idx, x, y, d, newG, bestCost, bestD);

          // Check if we found a better cost
          if (newG < bestCost) {
            bestCost = newG;
            bestD    = d;
          }
        } // end neighbor loop

        if (bestD != -1) { // We found an improvement
          nd.forward_g_cost = bestCost;
          nd.forward_direction = computeParentDirection2D(bestD);
          nd.flags.forward_update_flag = 1;
          // printf("id = %d, x = %d, y = %d, forward_cost = %d, forward_update_flag = %d\n", 
          //  idx, x, y, nd.forward_g_cost, nd.flags.forward_update_flag);
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
          nd.flags.backward_update_flag = 1;
          //printf("id = %d, x = %d, y = %d, backward_cost = %d, backward_update_flag = %d\n", 
          //  idx, x, y, nd.backward_g_cost, nd.flags.backward_update_flag);
        }
      } // end backward
    } // end “for each node” (forward + backward)

    g.sync();


    /*
    if (tid == 0) {
      printf("iter = %d, maxIters = %d\n", iter, maxIters);
      for (int id = 0; id < total; id++) {
        int local_x = id % xDimTemp + LLX;
        int local_y = id / xDimTemp + LLY;
        int idx = locToIdx_2D(local_x, local_y, xDim);
        NodeData2D &nd = nodes[idx];
        if (nd.flags.backward_update_flag == 1) {
          printf("summary id = %d, x = %d, y = %d,  backward_cost = %d\n", idx, local_x, local_y,  nd.backward_g_cost);
        }

        if (nd.flags.forward_update_flag == 1) {
          printf("summary id = %d, x = %d, y = %d,  forward_cost = %d\n", idx, local_x, local_y, nd.forward_g_cost);
        }
      }
    }
    */
 
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
      int x = localIdx % xDimTemp + LLX;
      int y = localIdx / xDimTemp + LLY;
      int idx = locToIdx_2D(x, y, xDim);
      NodeData2D &nd = nodes[idx];
      /*
      // If either side is "unreached," skip
      if (nd.forward_g_cost_prev == 0xFFFFFFFF || 
          nd.backward_g_cost_prev == 0xFFFFFFFF)
      {
        continue;
      }
      */

      // Check the visited flag
      // bool localForwardMin = true;
      // bool localBackwardMin = true;
      
      
      if (!nd.flags.forward_visited_flag_prev) {
        bool localForwardMin = (nd.forward_g_cost_prev != 0xFFFFFFFF);
        for (int d = 0; d < 4; d++) {
          int nx = x + d_dX[d];
          int ny = y + d_dY[d];
          if (nx < LLX || nx > URX || ny < LLY || ny > URY) {
            continue;  // out of bounds
          }
          
          int nbrIdx = locToIdx_2D(nx, ny, xDim);
          NodeData2D &nbr = nodes[nbrIdx];
          
          // Check forward minimum
          if (!nbr.flags.forward_visited_flag_prev && 
              (nbr.forward_g_cost_prev + nbr.forward_h_cost < nd.forward_g_cost_prev + nd.forward_h_cost)) {
              localForwardMin = false;
          }
        }
  
        if (localForwardMin) {
          nd.flags.forward_visited_flag = true;
        }
      }


      if (!nd.flags.backward_visited_flag_prev) {
        bool localBackwardMin = (nd.backward_g_cost_prev != 0xFFFFFFFF);
        for (int d = 0; d < 4; d++) {
          int nx = x + d_dX[d];
          int ny = y + d_dY[d];
          if (nx < LLX || nx > URX || ny < LLY || ny > URY) {
            continue;  // out of bounds
          }
          
          int nbrIdx = locToIdx_2D(nx, ny, xDim);
          NodeData2D &nbr = nodes[nbrIdx];
          
          // Check forward minimum
          if (!nbr.flags.backward_visited_flag_prev && 
              (nbr.backward_g_cost_prev + nbr.backward_h_cost < nd.backward_g_cost_prev + nd.backward_h_cost)) {
              localBackwardMin = false;
          }
        }
  
        if (localBackwardMin) {
          nd.flags.backward_visited_flag = true;
        }
      }
    
    
      /*
      bool localForwardMin = (nd.forward_g_cost_prev != 0xFFFFFFFF);
      bool localBackwardMin = (nd.backward_g_cost_prev != 0xFFFFFFFF);
      for (int d = 0; d < 4; d++) {
        int nx = x + d_dX[d];
        int ny = y + d_dY[d];
        if (nx < LLX || nx > URX || ny < LLY || ny > URY) {
          continue;  // out of bounds
        }
        
        int nbrIdx = locToIdx_2D(nx, ny, xDim);
        NodeData2D &nbr = nodes[nbrIdx];
        
        // Check forward minimum
        if (!nbr.flags.forward_visited_flag_prev && 
            (nbr.forward_g_cost_prev + nbr.forward_h_cost < nd.forward_g_cost_prev + nd.forward_h_cost)) {
            localForwardMin = false;
        }
    
        // Check backward minimum
        if (!nbr.flags.backward_visited_flag_prev &&
            (nbr.backward_g_cost_prev + nbr.backward_h_cost < nd.backward_g_cost_prev + nd.backward_h_cost)) {
            localBackwardMin = false;
        }
      }
      */

      /*
      if (localForwardMin) {
        nd.flags.forward_visited_flag = true;
      }

      if (localBackwardMin) {
        nd.flags.backward_visited_flag = true;  
      } */

      /*
      if (localForwardMin && localBackwardMin) {
        nd.flags.forward_visited_flag = true;
        nd.flags.backward_visited_flag = true;
      }*/
      
      /*
      for (int d = 0; d < 4; d++) {
        int nx = x + d_dX[d];
        int ny = y + d_dY[d];
        if (nx < LLX || nx > URX || ny < LLY || ny > URY) {
          continue;
        }

        int nbrIdx = locToIdx_2D(nx, ny, xDim);
        if (nodes[nbrIdx].forward_g_cost_prev + nodes[nbrIdx].forward_h_cost >= nd.forward_g_cost_prev + nd.forward_h_cost_prev &&
            nodes[nbrIdx].backward_g_cost_prev + nodes[nbrIdx].backward_h_cost >= nd.backward_g_cost_prev + nd.backward_h_cost_prev) {
          nd.flags.forward_visited_flag = true;
          nd.flags.backward_visited_flag = true;
        }
      }
      */

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
        // printf("localFrontsMeet = %d", localFrontsMeet);
      }


      /*
      // Check neighbors for meeting fronts.
      if (nd.flags.forward_visited_flag || nd.flags.backward_visited_flag) {
        for (int d = 0; d < 4; d++) {
          int nx = local_x + d_dX[d];
          int ny = local_y + d_dY[d];
          if (nx < LLX || nx > URX || ny < LLY || ny > URY) { continue; }
          int nbrIdx = locToIdx_2D(nx, ny, xDim);
          NodeData2D &nbr = nodes[nbrIdx];
          if (nd.flags.forward_visited_flag && nbr.flags.backward_visited_flag) { localFrontsMeet = true; }
          if (nd.flags.backward_visited_flag && nbr.flags.forward_visited_flag) { localFrontsMeet = true; }
        }
      } */
    }
    
    g.sync();

    /*
    if (blockIdx.x * blockDim.x + threadIdx.x == 0) {
      printf("iter = %d, maxIters = %d\n", iter, maxIters);
      for (int id = 0; id < total; id++) {
        int local_x = id % xDimTemp + LLX;
        int local_y = id / xDimTemp + LLY;
        int idx = locToIdx_2D(local_x, local_y, xDim);
        NodeData2D &nd = nodes[idx];
        if (nd.flags.backward_visited_flag == true) {
          printf("backward_visited_flag id = %d, backward_cost = %d", idx, nd.backward_g_cost);
        }

        if (nd.flags.forward_visited_flag == true) {
          printf("forward_visited_flag id = %d, forward_cost = %d", idx, nd.forward_g_cost);
        }
      }
    } */
 
    if (localFrontsMeet) {
      atomicExch(d_doneFlag, 1);
    }
    
    g.sync();

    if (*d_doneFlag == 1) {
      globalDone = true;
    }

    g.sync();

    //if (*d_doneFlag == 1) {
    //  *d_doneFlag = 0x7FFFFFFF;
    //  return;
    //}
  } // end for (iter)

  if (tid == 0) {
    if (*d_doneFlag == 1) {
      *d_doneFlag = 0x7FFFFFFF;
    }
  }
  g.sync();
  return;
}


// Define the device function for the meetId check
__device__
void findMeetIdAndTraceBackCost2D__device(
  NodeData2D* nodes,
  int* d_doneFlag, 
  int LLX, int LLY, int URX, int URY,
  int xDim)
{ 
  if (blockIdx.x * blockDim.x + threadIdx.x == 0) {
    if (*d_doneFlag == 0) { printf("Error ! d_doneFlag = 0\n"); }
  }

  int xDimTemp = URX - LLX + 1;
  int numNodes = (URX - LLX + 1) * (URY - LLY + 1);
  for (int localIdx = threadIdx.x; localIdx < numNodes; localIdx += blockDim.x) {
    int local_x = localIdx % xDimTemp + LLX;
    int local_y = localIdx / xDimTemp + LLY;
    int idx = locToIdx_2D(local_x, local_y, xDim);
    if (nodes[idx].forward_g_cost != INF32 && nodes[idx].backward_g_cost != INF32) {
      int32_t cost = nodes[idx].forward_g_cost + nodes[idx].backward_g_cost;
      atomicMin(d_doneFlag, cost);      
    }
    /*
    if (nodes[idx].flags.forward_visited_flag && nodes[idx].flags.backward_visited_flag) {
      int32_t cost = nodes[idx].forward_g_cost + nodes[idx].backward_g_cost;
      atomicMin(d_doneFlag, cost);      
    } */
  }

  //if (blockIdx.x * blockDim.x + threadIdx.x == 0) {
  //  printf("Cost2D MinCost = %d\n", *d_doneFlag);
  //}
}

__device__
void findMeetIdAndTraceBackId2D__device(
  NodeData2D* nodes,
  int* d_doneFlag, 
  int* d_meetId,
  int LLX, int LLY, int URX, int URY,
  int xDim)
{ 
  int xDimTemp = URX - LLX + 1;
  int numNodes = (URX - LLX + 1) * (URY - LLY + 1);
  /*
  for (int localIdx = threadIdx.x; localIdx < numNodes; localIdx += blockDim.x) {
    int local_x = localIdx % xDimTemp + LLX;
    int local_y = localIdx / xDimTemp + LLY;
    int idx = locToIdx_2D(local_x, local_y, xDim);
    if (nodes[idx].flags.forward_visited_flag && nodes[idx].flags.backward_visited_flag && 
        (nodes[idx].forward_g_cost + nodes[idx].backward_g_cost == *d_doneFlag)) {
      atomicMin(d_meetId, idx);      
    }
  }

  if (*d_meetId == 0x7FFFFFFF) {
    for (int localIdx = threadIdx.x; localIdx < numNodes; localIdx += blockDim.x) {
      int local_x = localIdx % xDimTemp + LLX;
      int local_y = localIdx / xDimTemp + LLY;
      int idx = locToIdx_2D(local_x, local_y, xDim);
      if (nodes[idx].flags.forward_visited_flag && nodes[idx].flags.dst_flag) {
        atomicMin(d_meetId, idx);      
      }
    }
  }*/
  
  for (int localIdx = threadIdx.x; localIdx < numNodes; localIdx += blockDim.x) {
    int local_x = localIdx % xDimTemp + LLX;
    int local_y = localIdx / xDimTemp + LLY;
    int idx = locToIdx_2D(local_x, local_y, xDim);
    if ((nodes[idx].forward_g_cost != INF32 && nodes[idx].backward_g_cost != INF32) &&
        (nodes[idx].forward_g_cost + nodes[idx].backward_g_cost == *d_doneFlag)) {
      atomicMin(d_meetId, idx);      
    }
  }
}

__device__
void forwardTraceBack2D__single_thread__device(
  NodeData2D* nodes, 
  int* d_meetId, 
  int* d_dX, int* d_dY,
  int LLX, int LLY, int URX, int URY,
  int xDim)
{
  if (*d_meetId == 0x7FFFFFFF) {
    return; // No meetId found
  }
  
  int curId = *d_meetId;
  int maxIterations = (URX - LLX + 1) * (URY - LLY + 1);
  int iteration = 0;
  
  /*
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
  }*/

  while (nodes[curId].flags.src_flag == 0 && iteration < maxIterations) {
    // Ensure forward_direction is valid (e.g., 0 <= forward_direction < 4)
    uint8_t fwdDir = nodes[curId].forward_direction;
    
    // Record the golden parent BEFORE moving on.
    int2 xy = idxToLoc_2D(curId, xDim);
    int nx = xy.x + d_dX[fwdDir];
    int ny = xy.y + d_dY[fwdDir];
    if (nx < LLX || nx > URX || ny < LLY || ny > URY) break;
    
    nodes[curId].golden_parent_x = nx;
    nodes[curId].golden_parent_y = ny;
    
    // Mark this node as processed.
    nodes[curId].flags.src_flag = 1;
    
    // Move to the next node.
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
  int* d_meetId, 
  int* d_dX, int* d_dY,
  int LLX, int LLY, int URX, int URY,
  int xDim)
{  
  if (*d_meetId == 0x7FFFFFFF) {
    return; // No meetId found
  }
  
  int curId = *d_meetId;
  if (nodes[curId].flags.dst_flag == 1) { 
    nodes[curId].flags.dst_flag = 0; // change the dst flag to 0
    nodes[curId].flags.src_flag = 1;
    return;
  }
  
  int maxIterations = (URX - LLX + 1) * (URY - LLY + 1);
  int iteration = 0;

  while (nodes[curId].flags.dst_flag == 0 && iteration < maxIterations) {
    int2 xy = idxToLoc_2D(curId, xDim);
    uint8_t backwardDirection = nodes[curId].backward_direction;
    int nx = xy.x + d_dX[backwardDirection];
    int ny = xy.y + d_dY[backwardDirection];
    if (nx < LLX || nx > URX || ny < LLY || ny > URY) {
      break;
    }  
   
    int nextId = locToIdx_2D(nx, ny, xDim);
    if (nodes[nextId].golden_parent_x != -1) {
      printf("Error: Backward traceback meets forward traceback.\n");
    }    
    
    nodes[nextId].flags.src_flag = 1;
    nodes[nextId].golden_parent_x = xy.x;
    nodes[nextId].golden_parent_y = xy.y;
    
    curId = nextId;
    iteration++;
  }
  
  nodes[curId].flags.dst_flag = 0; // change the dst flag to 0
  /*
  while (iteration < maxIterations) {
    // Record current position.
    int2 xy = idxToLoc_2D(curId, xDim);
    uint8_t bwdDir = nodes[curId].backward_direction;
    int nx = xy.x + d_dX[bwdDir];
    int ny = xy.y + d_dY[bwdDir];
    if (nx < LLX || nx > URX || ny < LLY || ny > URY) break;
    
    // Set the golden parent pointer for the current node before moving.
    nodes[curId].golden_parent_x = xy.x;
    nodes[curId].golden_parent_y = xy.y;
    // Mark the node as processed.
    nodes[curId].flags.src_flag = 1;
    
    // Check if moving to the next node would conflict with an already set pointer.
    int nextId = locToIdx_2D(nx, ny, xDim);
    if (nodes[nextId].golden_parent_x != -1) {
      break;
    }
    
    curId = nextId;
    
    // If we reach a node flagged as destination, clear the flag and stop.
    if (nodes[curId].flags.dst_flag == 1) {
      nodes[curId].flags.dst_flag = 0;
      break;
    }
    iteration++;
  } */

  if (iteration >= maxIterations) {
    printf("Warning: Backward traceback exceeded maximum iterations.\n");
  }
}



// Fused cooperative kernel that processes a single net.
__device__ 
void biwaveBellmanFord2D__device(
  cooperative_groups::grid_group& grid,   // grid-level cooperative group		
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

  int* d_doneFlag = d_doneFlags + netId;
  int* d_meetId = d_meetIds + netId;

  // Connect the pin one by one
  //for (int pinIter = 1; pinIter < 2; pinIter++) {
  for (int pinIter = 1; pinIter < numPins; pinIter++) {
    // Initilization
    if (blockIdx.x * blockDim.x + threadIdx.x == 0) {
      *d_doneFlag = 0;
      *d_meetId = 0x7FFFFFFF;

      /*
      for (int i = 0; i < xDim * yDim; i++) {
        int2 xy = idxToLoc_2D(i, xDim);
        if (d_nodes[i].flags.src_flag == 1) {
          printf("pinIter = %d, src_flag : id = %d, x = %d, y = %d\n", pinIter, i, xy.x, xy.y);
        }

        if (d_nodes[i].flags.dst_flag == 1) {
          printf("pinIter = %d, dst_flag : id = %d, x = %d, y = %d\n", pinIter, i, xy.x, xy.y);
        }
      }
      */

      // Check the temp
      // printf("pinIter = %d, netId = %d, numPins = %d, LLX = %d, LLY = %d, URX = %d, URY = %d\n", pinIter, netId, numPins, LLX, LLY, URX, URY);
      
      /*
      int total = (URX - LLX + 1) * (URY - LLY + 1);
      int xDimTemp = URX - LLX + 1;
      for (int id = 0; id < total; id++) {
        int local_x = id % xDimTemp + LLX;
        int local_y = id / xDimTemp + LLY;
        int idx = locToIdx_2D(local_x, local_y, xDim);
        NodeData2D &nd = d_nodes[idx];
        if (nd.flags.src_flag == true) {
          printf("local src_flag : id = %d, x = %d, y = %d\n", idx, local_x, local_y);
        }
          
        if (nd.flags.dst_flag == true) {
          printf("local dst_flag : id = %d, x = %d, y = %d\n", idx, local_x, local_y);
        }
      }
      */
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
      // printf("d_doneFlag = %d, d_meetId = %d\n",  *d_doneFlag,  *d_meetId);
      
      // trace back
      forwardTraceBack2D__single_thread__device(
        d_nodes, d_meetId, d_dX, d_dY, 
        LLX, LLY, URX, URY, xDim);

      // printf("finish forward traceback\n");    

      backwardTraceBack2D__single__thread__device(
        d_nodes, d_meetId, d_dX, d_dY, 
        LLX, LLY, URX, URY, xDim);

      // printf("finish backward traceback\n");
    }

    grid.sync(); // Synchronize all threads in the grid
  }
  //grid.sync();
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
  biwaveBellmanFord2D__device(
    grid,
    netId,
    d_netHPWL,
    d_netPtr,
    d_netBBoxVec,
    d_pins,
    d_costMap,
    d_nodes,
    d_dX,
    d_dY,
    d_doneFlags,
    d_meetIds,
    xDim, yDim,
    d_xCoords,
    d_yCoords,
    congThreshold,
    BLOCKCOST,
    OVERFLOWCOST,
    HISTCOST);    
  grid.sync(); // Synchronize all threads in the grid
}


// Just a wrapper function to call the kernel
void launchMazeRouteStream(
  int* d_netHPWL,
  int* d_netPtr,
  Rect2D_CUDA* d_netBBox,
  int* d_pins,
  uint64_t* d_costMap,
  NodeData2D* d_nodes,
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


  /*
  // Calculate the maximum number of blocks that can run cooperatively
  int deviceId = 0;
  cudaDeviceProp deviceProp;
  cudaGetDeviceProperties(&deviceProp, deviceId);
  if (deviceProp.cooperativeLaunch == 0) {
    printf("Device does not support cooperative grid launch.\n");
    exit(1);
  } 

  if (VERBOSE > 0) {
    printf("Device supports cooperative grid launch.\n");
    printf("Cooperative launch: %d\n", deviceProp.cooperativeLaunch);
    printf("Multi-device coop: %d\n", deviceProp.cooperativeMultiDeviceLaunch);
    printf("Max threads per block: %d\n", deviceProp.maxThreadsPerBlock);
  }

  int threadsPerBlock = 1024;
  int numBlocksPerSm = 0;
  cudaError_t occErr = cudaOccupancyMaxActiveBlocksPerMultiprocessor(&numBlocksPerSm, biwaveBellmanFord2D__kernel, threadsPerBlock, 0);
  if (occErr != cudaSuccess) {
    printf("Occupancy calculation error: %s\n", cudaGetErrorString(occErr));
  } 

  if (numBlocksPerSm == 0) {
    numBlocksPerSm = 1;
    printf("Reset numBlocksPerSm to 1\n");
  }

  int numSms = deviceProp.multiProcessorCount;
  int numBlocks = numBlocksPerSm * numSms;
  //  printf("numBlocksPerSm = %d, numSms = %d, numBlocks = %d\n", numBlocksPerSm, numSms, numBlocks);

  // Ensure the grid size does not exceed the maximum allowed for cooperative launch
  int maxBlocksPerGrid = 0;
  cudaDeviceGetAttribute(&maxBlocksPerGrid, cudaDevAttrMaxGridDimX, deviceId);
  // printf("maxBlocksPerGrid = %d\n", maxBlocksPerGrid);

  // For cooperative kernels that use grid-wide sync, you must launch the full grid,
  // even if totalThreads would allow a smaller grid. (Inside the kernel, extra blocks
  // should simply exit if their blockIdx.x exceeds the work limit.)
  numBlocks = min(numBlocks, maxBlocksPerGrid);
  // printf("Launching kernel with %d blocks\n", numBlocks);
  */

  int threadsPerBlock = 1024;
  int numBlocks = 108;

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
float batchPathSyncUp(
  std::vector<std::unique_ptr<FlexGRWorker>>& uworkers,
  std::vector<grNet*>& nets,
  std::vector<Rect2D_CUDA>& netBBoxVec,
  std::vector<NodeData2D>& nodes,
  int xDim)
{ 
  auto syncupTimeStart = std::chrono::high_resolution_clock::now();
  
  for (int netId = 0; netId < nets.size(); netId++) {
    auto& net = nets[netId];
    auto& uworker = uworkers[net->getWorkerId()];
    auto& gridGraph = uworker->getGridGraph();
    auto workerLL = uworkers[net->getWorkerId()]->getRouteGCellIdxLL();
    int workerLX = workerLL.x();
    int workerLY = workerLL.y();  
    auto& netBBox = netBBoxVec[netId];
    int LLX = netBBox.xMin;
    int LLY = netBBox.yMin;
    int URX = netBBox.xMax;
    int URY = netBBox.yMax;
    int xDimTemp = URX - LLX + 1;
    int numNodes = (URX - LLX + 1) * (URY - LLY + 1);
    for (int localIdx = 0; localIdx < numNodes; localIdx++) {
      int localX = localIdx % xDimTemp;
      int localY = localIdx / xDimTemp;

      int x = localX + LLX;
      int y = localY + LLY;      
      int idx = locToIdx_2D(x, y, xDim);

      x -= workerLX;
      y -= workerLY;

      int parentX = nodes[idx].golden_parent_x - workerLX;
      int parentY = nodes[idx].golden_parent_y - workerLY;

      gridGraph.setGoldenParent2D(x, y, parentX, parentY);      
      /*
      if (nodes[idx].golden_parent_x != -1 || nodes[idx].golden_parent_y != -1) {
        std::cout << "Net " << netId << " x = " << x << " y = " << y << " "
                  << "Parent " << parentX << "  " << parentY << std::endl;
      }
      */
    }    
  }

  auto syncupTimeEnd = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double, std::milli> syncupTime = syncupTimeEnd - syncupTimeStart;
  return syncupTime.count();
}




__global__ 
void initBatchNodeData2D__kernel(
  NodeData2D* d_nodes,
  int numNodes)
{
  for (int idx = blockIdx.x * blockDim.x + threadIdx.x; idx < numNodes; idx += gridDim.x * blockDim.x) {
    initNodeData2D(d_nodes[idx]);
  }
}



__global__
void initBatchPin2D__kernel(
  NodeData2D* d_nodes,
  int* d_pins,
  int* d_netPtr,
  int netIdStart,
  int numNets)
{  
  int netId = blockIdx.x * blockDim.x + threadIdx.x;
  if (netId >= numNets) {
    return;
  }

  netId += netIdStart;

  int pinIdxStart = d_netPtr[netId];
  int pinIdxEnd = d_netPtr[netId + 1];
  d_nodes[d_pins[pinIdxStart]].flags.src_flag = true;
  for (int pinIter = pinIdxStart + 1; pinIter < pinIdxEnd; pinIter++) {
    d_nodes[d_pins[pinIter]].flags.dst_flag = true;
  }
}



__global__
void initBatchPin2D_v2__kernel(
  NodeData2D* d_nodes,
  int* d_pins,
  int* d_netPtr,
  int* d_netBatchIdx,
  int numNets,
  int numGrids)
{  
  for (int netId = blockIdx.x * blockDim.x + threadIdx.x; netId < numNets; netId += gridDim.x * blockDim.x) {
    int batchId = d_netBatchIdx[netId];
    int baseNodeId = batchId * numGrids;
    int pinIdxStart = d_netPtr[netId];
    int pinIdxEnd = d_netPtr[netId + 1];
    d_nodes[d_pins[pinIdxStart] + baseNodeId].flags.src_flag = true;
    for (int pinIter = pinIdxStart + 1; pinIter < pinIdxEnd; pinIter++) {
      d_nodes[d_pins[pinIter] + baseNodeId].flags.dst_flag = true;
    }
  }
}






// Just a wrapper function to call the kernel
void launchMazeRouteStream_update_v2(
  int* d_netHPWL,
  int* d_netPtr,
  Rect2D_CUDA* d_netBBox,
  int* d_pins,
  uint64_t* d_costMap,
  NodeData2D* d_nodes,
  int* d_dX,
  int* d_dY,
  int* d_doneFlag,
  int* d_meetId,
  int xDim, int yDim,
  const int* d_xCoords,
  const int* d_yCoords,
  Point2D_CUDA* d_parents,
  float congThreshold,
  int BLOCKCOST,
  int CONGCOST,
  int HISTCOST,
  int netId,
  int netIdx,
  int batchIdx,
  int totalThreads,
  int nodeBaseIdx,
  cudaStream_t stream)
{
  void* kernelArgs[] = {
    &netId,
    &netIdx,
    &batchIdx,
    &nodeBaseIdx,
    &d_parents,
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

  int threadsPerBlock = 1024;
  int numBlocks = 108;

  /*
  cudaError_t err = cudaLaunchCooperativeKernel(
    (void*)biwaveBellmanFord2D_update_v2__kernel,
    numBlocks, threadsPerBlock,
    kernelArgs,
    0,       // additional dynamic shared memory (if needed)
    stream); // launch on the given stream


  if (err != cudaSuccess) {
    printf("Kernel launch error (net %d): %s\n", netId, cudaGetErrorString(err));
  }*/
}































float FlexGR::GPUAccelerated2DMazeRoute_update_v2(
  std::vector<std::unique_ptr<FlexGRWorker> >& uworkers,
  std::vector<std::vector<grNet*> >& netBatches,
  std::vector<int>& validBatches,
  std::vector<Point2D_CUDA>& h_parents,
  std::vector<uint64_t>& h_costMap,
  std::vector<int>& h_xCoords,
  std::vector<int>& h_yCoords,
  RouterConfiguration* router_cfg,
  float congThreshold,
  int xDim, int yDim)
{
  // Start overall timing.
  auto totalStart = std::chrono::high_resolution_clock::now();
  int numGrids = xDim * yDim;
  
  std::vector<Point2D_CUDA> netVec;
  std::vector<int> netPtr;
  std::vector<int> netBatchIdxVec; 
  std::vector<int> netHWPL;
  std::vector<Rect2D_CUDA> netBBoxVec;
  std::vector<int> pinIdxVec;
  std::vector<int> batchPtr;
  
  netPtr.push_back(0);
  batchPtr.push_back(0);  
  int maxBatchSize = 0;
  int minBatchSize = std::numeric_limits<int>::max();
  int batchIdx = 0;
  for (auto& batchId : validBatches) {
    auto& batch = netBatches[batchId];
    for (auto& net : batch) {
      for (auto& idx : net->getPinGCellAbsIdxs()) {
        netVec.push_back(Point2D_CUDA(idx.x(), idx.y()));
        pinIdxVec.push_back(locToIdx_2D(idx.x(), idx.y(), xDim));
      }
      netBatchIdxVec.push_back(batchIdx);
      netPtr.push_back(netVec.size());
      auto netBBox = net->getRouteAbsBBox();
      netBBoxVec.push_back(
        Rect2D_CUDA(netBBox.xMin(), netBBox.yMin(), netBBox.xMax(), netBBox.yMax()));
      netHWPL.push_back(net->getHPWL());
    }
    batchPtr.push_back(netHWPL.size());
    batchIdx++;
    maxBatchSize = std::max(maxBatchSize, static_cast<int>(batch.size()));
    minBatchSize = std::min(minBatchSize, static_cast<int>(batch.size()));
  }

  int numBatches = validBatches.size();
  int numNets = netHWPL.size();
  
  // std::cout << "[INFO] Number of batches: " << numBatches << std::endl;
  std::cout << "[INFO] Max batch size: " << maxBatchSize << std::endl;
  std::cout << "[INFO] Min batch size: " << minBatchSize << std::endl;
  // std::vector<Point2D_CUDA> h_parents(numGrids * numBatches, Point2D_CUDA(-1, -1));

  // Allocate and copy device memory
  // We need to define the needed utility variables
  std::vector<int> h_dX = {0, 1, 0, -1};
  std::vector<int> h_dY = {1, 0, -1, 0};
  
  int* d_dX = nullptr;
  int* d_dY = nullptr;
  

  int* d_doneFlag = nullptr; // This is allocated for each net seperately (maxBatchSize)
  int* d_meetId = nullptr; // This is allocated for each net seperately (maxBatchSize)
  
  // For the design specific variables (numGrids)
  uint64_t* d_costMap = nullptr;
  int* d_xCoords = nullptr;
  int* d_yCoords = nullptr;
  NodeData2D* d_nodes = nullptr;
  Point2D_CUDA* d_parents = nullptr;
  
  int* d_pinIdxVec = nullptr;
  int* d_netHPWL = nullptr;
  int* d_netPtr = nullptr;
  Rect2D_CUDA* d_netBBox = nullptr;
  int* d_netBatchIdx = nullptr;

  int numBatchesParallel = numBatches;
  //int numBatchesParallel = 500;  
  int numNodes = numGrids * numBatchesParallel;

  // Allocate the device memory for the d_dX and d_dY
  cudaMalloc(&d_dX, 4 * sizeof(int));
  cudaMalloc(&d_dY, 4 * sizeof(int));
  
  cudaMalloc(&d_doneFlag, numNets * sizeof(int));
  cudaMalloc(&d_meetId, numNets * sizeof(int));
  
  cudaMalloc(&d_costMap, numNodes * sizeof(uint64_t));
  cudaMalloc(&d_xCoords, h_xCoords.size() * sizeof(int));
  cudaMalloc(&d_yCoords, h_yCoords.size() * sizeof(int));
  cudaMalloc(&d_nodes, numNodes * sizeof(NodeData2D));
  cudaMalloc(&d_parents, numNodes * sizeof(Point2D_CUDA));

  cudaMalloc(&d_pinIdxVec, pinIdxVec.size() * sizeof(int));
  cudaMalloc(&d_netHPWL, netHWPL.size() * sizeof(int));
  cudaMalloc(&d_netPtr, netPtr.size() * sizeof(int));
  cudaMalloc(&d_netBBox, netBBoxVec.size() * sizeof(Rect2D_CUDA));
  cudaMalloc(&d_netBatchIdx, netBatchIdxVec.size() * sizeof(int));

  cudaMemcpy(d_dX, h_dX.data(), 4 * sizeof(int), cudaMemcpyHostToDevice);
  cudaMemcpy(d_dY, h_dY.data(), 4 * sizeof(int), cudaMemcpyHostToDevice);


  cudaMemcpy(d_costMap, h_costMap.data(), numNodes * sizeof(uint64_t), cudaMemcpyHostToDevice);
  cudaMemcpy(d_xCoords, h_xCoords.data(), h_xCoords.size() * sizeof(int), cudaMemcpyHostToDevice);
  cudaMemcpy(d_yCoords, h_yCoords.data(), h_yCoords.size() * sizeof(int), cudaMemcpyHostToDevice);
  cudaMemcpy(d_parents, h_parents.data(), numNodes * sizeof(Point2D_CUDA), cudaMemcpyHostToDevice);

  cudaMemcpy(d_pinIdxVec, pinIdxVec.data(), pinIdxVec.size() * sizeof(int), cudaMemcpyHostToDevice);
  cudaMemcpy(d_netHPWL, netHWPL.data(), netHWPL.size() * sizeof(int), cudaMemcpyHostToDevice);
  cudaMemcpy(d_netPtr, netPtr.data(), netPtr.size() * sizeof(int), cudaMemcpyHostToDevice);
  cudaMemcpy(d_netBBox, netBBoxVec.data(), netBBoxVec.size() * sizeof(Rect2D_CUDA), cudaMemcpyHostToDevice);
  cudaMemcpy(d_netBatchIdx, netBatchIdxVec.data(), netBatchIdxVec.size() * sizeof(int), cudaMemcpyHostToDevice);  
  
  cudaCheckError();


  std::vector<cudaStream_t> netStreams(maxBatchSize);
  for (auto& stream : netStreams) {
    cudaStreamCreateWithFlags(&stream, cudaStreamNonBlocking);
  }
 
  int numStreams = static_cast<int>(netStreams.size());


  // According to the original code
  unsigned BLOCKCOST = router_cfg->BLOCKCOST * 100;
  unsigned OVERFLOWCOST = 128;
  unsigned HISTCOST = 4;
 
  // Perform Global Initialization
  int numThreads = 1024;
  int numBatchBlocks = (numNodes + numThreads - 1) / numThreads;
  initBatchNodeData2D__kernel<<<numBatchBlocks, numThreads>>>(
    d_nodes, numNodes);

  cudaDeviceSynchronize();

  int numNetBlocks = (numNets + numThreads - 1) / numThreads;
  initBatchPin2D_v2__kernel<<<numNetBlocks, numThreads>>>(
    d_nodes,
    d_pinIdxVec, 
    d_netPtr,
    d_netBatchIdx,
    numNets,
    numGrids);
  cudaDeviceSynchronize();

  for (int netId = 0; netId < numNets; netId++) {
    // pick which stream to use
    int streamIdx = netId % numStreams;
    cudaStream_t stream = netStreams[streamIdx];
    auto& netBBox = netBBoxVec[netId];
    int localGridSize = (netBBox.xMax - netBBox.xMin + 1) * (netBBox.yMax - netBBox.yMin + 1);
    int nodeBaseIdx = netBatchIdxVec[netId] * numGrids;

    launchMazeRouteStream_update_v2(
      d_netHPWL, d_netPtr, d_netBBox, d_pinIdxVec, 
      d_costMap, d_nodes, 
      d_dX, d_dY, d_doneFlag, d_meetId,
      xDim, yDim, 
      d_xCoords,
      d_yCoords,
      d_parents,
      congThreshold,
      BLOCKCOST,
      OVERFLOWCOST,
      HISTCOST,
      netId,
      netId,
      netBatchIdxVec[netId], 
      localGridSize,
      nodeBaseIdx,
      stream);
  }

  
  // Sync up the golden parents
  cudaCheckError();

  for (int i = 0; i < maxBatchSize; i++) {
    cudaStreamSynchronize(netStreams[i]);
  }

  cudaMemcpy(h_parents.data(), d_parents, numNodes * sizeof(Point2D_CUDA), cudaMemcpyDeviceToHost);
  cudaCheckError();
  
  for (int i = 0; i < maxBatchSize; i++) {
    cudaStreamDestroy(netStreams[i]);
  }


  // Clear the memory
  cudaFree(d_dX);
  cudaFree(d_dY);
  cudaFree(d_doneFlag);
  cudaFree(d_meetId);
  cudaFree(d_costMap);
  cudaFree(d_xCoords);
  cudaFree(d_yCoords);
  cudaFree(d_nodes);
  cudaFree(d_parents);
  
  cudaFree(d_pinIdxVec);
  cudaFree(d_netHPWL);
  cudaFree(d_netPtr);
  cudaFree(d_netBBox);

  cudaCheckError();

  auto totalEnd = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double, std::milli> totalTime = totalEnd - totalStart;
  return totalTime.count();
}























// Fused cooperative kernel that processes a single net.
__device__ 
void biwaveBellmanFord2D_update__device(
  cooperative_groups::grid_group& grid,   // grid-level cooperative group		
  int netId,
  int netIdx,
  int batchIdx,
  Point2D_CUDA* d_parents,
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

  int* d_doneFlag = d_doneFlags + netIdx;
  int* d_meetId = d_meetIds + netIdx;

  // if (blockIdx.x * blockDim.x + threadIdx.x == 0) {
  //  printf("netId = %d, netIdx = %d, batchIdx = %d, pinIdxStart = %d, pinIdxEnd = %d, numPins = %d, maxIters = %d\n", 
  //          netId, netIdx, batchIdx, pinIdxStart, pinIdxEnd, numPins, maxIters);
  //}


  // Connect the pin one by one
  //for (int pinIter = 1; pinIter < 2; pinIter++) {
  for (int pinIter = 1; pinIter < numPins; pinIter++) {
    // Initilization
    if (blockIdx.x * blockDim.x + threadIdx.x == 0) {
      *d_doneFlag = 0;
      *d_meetId = 0x7FFFFFFF;
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
      // printf("d_doneFlag = %d, d_meetId = %d\n",  *d_doneFlag,  *d_meetId);
      
      // trace back
      forwardTraceBack2D__single_thread__device(
        d_nodes, d_meetId, d_dX, d_dY, 
        LLX, LLY, URX, URY, xDim);

      // printf("finish forward traceback\n");    

      backwardTraceBack2D__single__thread__device(
        d_nodes, d_meetId, d_dX, d_dY, 
        LLX, LLY, URX, URY, xDim);

      // printf("finish backward traceback\n");
    }

    grid.sync(); // Synchronize all threads in the grid
  }

  //grid.sync();
}


// Fused cooperative kernel that processes a single net.
__global__
void biwaveBellmanFord2D_update__kernel(
  int netId,
  int netIdx,
  int batchIdx,
  Point2D_CUDA* d_parents,
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
  biwaveBellmanFord2D_update__device(
    grid,
    netId,
    netIdx,
    batchIdx,
    d_parents,
    d_netHPWL,
    d_netPtr,
    d_netBBoxVec,
    d_pins,
    d_costMap,
    d_nodes,
    d_dX,
    d_dY,
    d_doneFlags,
    d_meetIds,
    xDim, yDim,
    d_xCoords,
    d_yCoords,
    congThreshold,
    BLOCKCOST,
    OVERFLOWCOST,
    HISTCOST);    
  grid.sync(); // Synchronize all threads in the grid
}


// Just a wrapper function to call the kernel
void launchMazeRouteStream_update(
  int* d_netHPWL,
  int* d_netPtr,
  Rect2D_CUDA* d_netBBox,
  int* d_pins,
  uint64_t* d_costMap,
  NodeData2D* d_nodes,
  int* d_dX,
  int* d_dY,
  int* d_doneFlag,
  int* d_meetId,
  int xDim, int yDim,
  const int* d_xCoords,
  const int* d_yCoords,
  Point2D_CUDA* d_parents,
  float congThreshold,
  int BLOCKCOST,
  int CONGCOST,
  int HISTCOST,
  int netIdx,
  int netId,
  int batchIdx,
  int totalThreads,
  cudaStream_t stream)
{
  void* kernelArgs[] = {
    &netId,
    &netIdx,
    &batchIdx,
    &d_parents,
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

  int threadsPerBlock = 1024;
  int numBlocks = 108;

  cudaError_t err = cudaLaunchCooperativeKernel(
    (void*)biwaveBellmanFord2D_update__kernel,
    numBlocks, threadsPerBlock,
    kernelArgs,
    0,       // additional dynamic shared memory (if needed)
    stream); // launch on the given stream

  if (err != cudaSuccess) {
    printf("Kernel launch error (net %d): %s\n", netId, cudaGetErrorString(err));
  }
}




// Fused cooperative kernel that processes a single net.
__global__
void biwaveBellmanFord2D_update_v2__kernel(
  int netId,
  int netIdx,
  int batchIdx,
  int nodeBaseIdx,
  Point2D_CUDA* d_parents,
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
  biwaveBellmanFord2D_update__device(
    grid,
    netId,
    netIdx,
    batchIdx,
    d_parents,
    d_netHPWL,
    d_netPtr,
    d_netBBoxVec,
    d_pins,
    d_costMap,
    d_nodes + nodeBaseIdx,
    d_dX,
    d_dY,
    d_doneFlags,
    d_meetIds,
    xDim, yDim,
    d_xCoords,
    d_yCoords,
    congThreshold,
    BLOCKCOST,
    OVERFLOWCOST,
    HISTCOST);    
  grid.sync(); // Synchronize all threads in the grid
}





float FlexGR::GPUAccelerated2DMazeRoute_update(
  std::vector<std::unique_ptr<FlexGRWorker> >& uworkers,
  std::vector<std::vector<grNet*> >& netBatches,
  std::vector<int>& validBatches,
  std::vector<Point2D_CUDA>& h_parents,
  std::vector<uint64_t>& h_costMap,
  std::vector<int>& h_xCoords,
  std::vector<int>& h_yCoords,
  RouterConfiguration* router_cfg,
  float congThreshold,
  int xDim, int yDim)
{
  // Start overall timing.
  auto totalStart = std::chrono::high_resolution_clock::now();
  int numGrids = xDim * yDim;
  
  std::vector<Point2D_CUDA> netVec;
  std::vector<int> netPtr; 
  std::vector<int> netHWPL;
  std::vector<Rect2D_CUDA> netBBoxVec;
  std::vector<int> pinIdxVec;
  std::vector<int> batchPtr;
  
  netPtr.push_back(0);
  batchPtr.push_back(0);  
  int maxBatchSize = 0;
  int minBatchSize = std::numeric_limits<int>::max();
  for (auto& batchId : validBatches) {
    auto& batch = netBatches[batchId];
    for (auto& net : batch) {
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
    batchPtr.push_back(netHWPL.size());
    maxBatchSize = std::max(maxBatchSize, static_cast<int>(batch.size()));
    minBatchSize = std::min(minBatchSize, static_cast<int>(batch.size()));
  }

  int numBatches = validBatches.size();
  // std::cout << "[INFO] Number of batches: " << numBatches << std::endl;
  std::cout << "[INFO] Max batch size: " << maxBatchSize << std::endl;
  std::cout << "[INFO] Min batch size: " << minBatchSize << std::endl;
  // std::vector<Point2D_CUDA> h_parents(numGrids * numBatches, Point2D_CUDA(-1, -1));

  // Allocate and copy device memory
  // We need to define the needed utility variables
  std::vector<int> h_dX = {0, 1, 0, -1};
  std::vector<int> h_dY = {1, 0, -1, 0};
  
  int* d_dX = nullptr;
  int* d_dY = nullptr;
  

  int* d_doneFlag = nullptr; // This is allocated for each net seperately (maxBatchSize)
  int* d_meetId = nullptr; // This is allocated for each net seperately (maxBatchSize)
  
  // For the design specific variables (numGrids)
  uint64_t* d_costMap = nullptr;
  int* d_xCoords = nullptr;
  int* d_yCoords = nullptr;
  NodeData2D* d_nodes = nullptr;
  Point2D_CUDA* d_parents = nullptr;
  
  int* d_pinIdxVec = nullptr;
  int* d_netHPWL = nullptr;
  int* d_netPtr = nullptr;
  Rect2D_CUDA* d_netBBox = nullptr;

  // Allocate the device memory for the d_dX and d_dY
  cudaMalloc(&d_dX, 4 * sizeof(int));
  cudaMalloc(&d_dY, 4 * sizeof(int));
  
  cudaMalloc(&d_doneFlag, maxBatchSize * sizeof(int));
  cudaMalloc(&d_meetId, maxBatchSize * sizeof(int));
  
  cudaMalloc(&d_costMap, numGrids * sizeof(uint64_t));
  cudaMalloc(&d_xCoords, h_xCoords.size() * sizeof(int));
  cudaMalloc(&d_yCoords, h_yCoords.size() * sizeof(int));
  cudaMalloc(&d_nodes, numGrids * sizeof(NodeData2D));
  cudaMalloc(&d_parents, numGrids * numBatches * sizeof(Point2D_CUDA));

  cudaMalloc(&d_pinIdxVec, pinIdxVec.size() * sizeof(int));
  cudaMalloc(&d_netHPWL, netHWPL.size() * sizeof(int));
  cudaMalloc(&d_netPtr, netPtr.size() * sizeof(int));
  cudaMalloc(&d_netBBox, netBBoxVec.size() * sizeof(Rect2D_CUDA));


  cudaMemcpy(d_dX, h_dX.data(), 4 * sizeof(int), cudaMemcpyHostToDevice);
  cudaMemcpy(d_dY, h_dY.data(), 4 * sizeof(int), cudaMemcpyHostToDevice);

  cudaMemcpy(d_costMap, h_costMap.data(), numGrids * sizeof(uint64_t), cudaMemcpyHostToDevice);
  cudaMemcpy(d_xCoords, h_xCoords.data(), h_xCoords.size() * sizeof(int), cudaMemcpyHostToDevice);
  cudaMemcpy(d_yCoords, h_yCoords.data(), h_yCoords.size() * sizeof(int), cudaMemcpyHostToDevice);
  cudaMemcpy(d_parents, h_parents.data(), numGrids * numBatches * sizeof(Point2D_CUDA), cudaMemcpyHostToDevice);

  cudaMemcpy(d_pinIdxVec, pinIdxVec.data(), pinIdxVec.size() * sizeof(int), cudaMemcpyHostToDevice);
  cudaMemcpy(d_netHPWL, netHWPL.data(), netHWPL.size() * sizeof(int), cudaMemcpyHostToDevice);
  cudaMemcpy(d_netPtr, netPtr.data(), netPtr.size() * sizeof(int), cudaMemcpyHostToDevice);
  cudaMemcpy(d_netBBox, netBBoxVec.data(), netBBoxVec.size() * sizeof(Rect2D_CUDA), cudaMemcpyHostToDevice);
  cudaCheckError();



  std::vector<cudaStream_t> netStreams(maxBatchSize);
  for (auto& stream : netStreams) {
    cudaStreamCreateWithFlags(&stream, cudaStreamNonBlocking);
  }
  

  // According to the original code
  unsigned BLOCKCOST = router_cfg->BLOCKCOST * 100;
  unsigned OVERFLOWCOST = 128;
  unsigned HISTCOST = 4;

  for (int batchIdx = 0; batchIdx < numBatches; batchIdx++) {
    int netBatchStart = batchPtr[batchIdx];
    int netBatchEnd = batchPtr[batchIdx + 1];
    int numNets = netBatchEnd - netBatchStart;

    // Initialize the nodes accordingly
    int numThreads = 1024;
    int numBatchBlocks = (numGrids + numThreads - 1) / numThreads;
    initBatchNodeData2D__kernel<<<numBatchBlocks, numThreads>>>(
      d_nodes, numGrids);
    cudaCheckError();
    cudaDeviceSynchronize();

    int numNetBlocks = (numNets + numThreads - 1) / numThreads;
    initBatchPin2D__kernel<<<numNetBlocks, numThreads>>>(
      d_nodes,
      d_pinIdxVec, 
      d_netPtr,
      netBatchStart,
      numNets);
    cudaCheckError();
    cudaDeviceSynchronize();
      
    std::vector<NodeData2D> h_nodes(numGrids);
    cudaMemcpy(h_nodes.data(), d_nodes, numGrids * sizeof(NodeData2D), cudaMemcpyDeviceToHost);
    cudaCheckError();

    /*
    std::cout << "numGrids = " << numGrids << std::endl;
    for (int nodeId = 0; nodeId < numGrids; nodeId++) {
      if (h_nodes[nodeId].flags.src_flag == true) {
        std::cout << "src_flag : id = " << nodeId << std::endl;
      }

      if (h_nodes[nodeId].flags.dst_flag == true) {
        std::cout << "dst_flag : id = " << nodeId << std::endl;
      }
    } 
    */

    // Perform the routing here
    for (int netIdx = 0; netIdx < numNets; netIdx++) {
      int netId = netBatchStart + netIdx;
      auto& netBBox = netBBoxVec[netId];
      int localGridSize = (netBBox.xMax - netBBox.xMin + 1) * (netBBox.yMax - netBBox.yMin + 1);
    
      launchMazeRouteStream_update(
        d_netHPWL, d_netPtr, d_netBBox, d_pinIdxVec, 
        d_costMap, d_nodes, 
        d_dX, d_dY, d_doneFlag, d_meetId,
        xDim, yDim, 
        d_xCoords,
        d_yCoords,
        d_parents,
        congThreshold,
        BLOCKCOST,
        OVERFLOWCOST,
        HISTCOST,
        netIdx, 
        netId,
        batchIdx,  
        (netBBox.xMax - netBBox.xMin + 1) * (netBBox.yMax - netBBox.yMin + 1),
        netStreams[netIdx]);
    }

    // Wait for all streams to finish
    for (int i = 0; i < numNets; i++) {
      cudaStreamSynchronize(netStreams[i]);
    }

    //std::cout << "numNets = " << numNets << std::endl;
  }

  // Sync up the golden parents
  cudaCheckError();

  cudaMemcpy(h_parents.data(), d_parents, numGrids * numBatches * sizeof(Point2D_CUDA), cudaMemcpyDeviceToHost);
  cudaCheckError();

  
  for (int i = 0; i < maxBatchSize; i++) {
    cudaStreamDestroy(netStreams[i]);
  }


  // Clear the memory
  cudaFree(d_dX);
  cudaFree(d_dY);
  cudaFree(d_doneFlag);
  cudaFree(d_meetId);
  cudaFree(d_costMap);
  cudaFree(d_xCoords);
  cudaFree(d_yCoords);
  cudaFree(d_nodes);
  cudaFree(d_parents);
  
  cudaFree(d_pinIdxVec);
  cudaFree(d_netHPWL);
  cudaFree(d_netPtr);
  cudaFree(d_netBBox);

  cudaCheckError();

  auto totalEnd = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double, std::milli> totalTime = totalEnd - totalStart;
  return totalTime.count();
}








float FlexGR::GPUAccelerated2DMazeRoute(
  std::vector<std::unique_ptr<FlexGRWorker>>& uworkers,
  std::vector<grNet*>& nets,
  std::vector<uint64_t>& h_costMap,
  std::vector<int>& h_xCoords,
  std::vector<int>& h_yCoords,
  RouterConfiguration* router_cfg,
  float congThreshold,
  int xDim, int yDim)
{
  // Start overall timing.
  auto totalStart = std::chrono::high_resolution_clock::now();

  if (VERBOSE > 0) {
    std::cout << "[INFO] GPU accelerated 2D Maze Routing" << std::endl;
    std::cout << "[INFO] Number of nets: " << nets.size() << std::endl;
  }

  int numGrids = xDim * yDim;
  int numNets = nets.size();
  
  std::vector<Point2D_CUDA> netVec;
  std::vector<int> netPtr; 
  std::vector<int> netHWPL;
  std::vector<Rect2D_CUDA> netBBoxVec;
  std::vector<int> pinIdxVec;
 

  netPtr.push_back(0);
  for (auto& net : nets) {
    for (auto& idx : net->getPinGCellAbsIdxs()) {
      netVec.push_back(Point2D_CUDA(idx.x(), idx.y()));
      pinIdxVec.push_back(locToIdx_2D(idx.x(), idx.y(), xDim));
      if (VERBOSE > 0) {
        std::cout << "Pin x = " << idx.x() << " y = " << idx.y() << " idx = " << locToIdx_2D(idx.x(), idx.y(), xDim) << std::endl;
      }
    }
    netPtr.push_back(netVec.size());
    auto netBBox = net->getRouteAbsBBox();
    netBBoxVec.push_back(
      Rect2D_CUDA(netBBox.xMin(), netBBox.yMin(), netBBox.xMax(), netBBox.yMax()));
    netHWPL.push_back(net->getHPWL());
  }


  //===========================================================================
  NodeData2D* h_nodesPinned = nullptr;
  cudaHostAlloc(&h_nodesPinned, numGrids * sizeof(NodeData2D), cudaHostAllocDefault);
  for (int i = 0; i < numGrids; i++) {
    initNodeData2D(h_nodesPinned[i]);
  }

  // Mark the source and destination nodes for each net.
  for (int netId = 0; netId < numNets; netId++) {
    int pinIdxStart = netPtr[netId];
    int pinIdxEnd   = netPtr[netId + 1];
    h_nodesPinned[pinIdxVec[pinIdxStart]].flags.src_flag = 1;
    for (int idx = pinIdxStart + 1; idx < pinIdxEnd; idx++) {
      h_nodesPinned[pinIdxVec[idx]].flags.dst_flag = 1;
    }
  }



  // Allocate and copy device memory
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
  NodeData2D* d_nodes = nullptr;
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
  cudaMalloc(&d_nodes, numGrids * sizeof(NodeData2D));
  cudaMalloc(&d_netHPWL, netHWPL.size() * sizeof(int));
  cudaMalloc(&d_netPtr, netPtr.size() * sizeof(int));
  cudaMalloc(&d_netBBox, netBBoxVec.size() * sizeof(Rect2D_CUDA));


  cudaMemcpy(d_dX, h_dX.data(), 4 * sizeof(int), cudaMemcpyHostToDevice);
  cudaMemcpy(d_dY, h_dY.data(), 4 * sizeof(int), cudaMemcpyHostToDevice);
  cudaMemcpy(d_costMap, h_costMap.data(), numGrids * sizeof(uint64_t), cudaMemcpyHostToDevice);
  cudaMemcpy(d_xCoords, h_xCoords.data(), h_xCoords.size() * sizeof(int), cudaMemcpyHostToDevice);
  cudaMemcpy(d_yCoords, h_yCoords.data(), h_yCoords.size() * sizeof(int), cudaMemcpyHostToDevice);

  cudaMemcpy(d_pinIdxVec, pinIdxVec.data(), pinIdxVec.size() * sizeof(int), cudaMemcpyHostToDevice);
  // Initialize d_nodes from h_nodesPinned.
  cudaMemcpy(d_nodes, h_nodesPinned, numGrids * sizeof(NodeData2D), cudaMemcpyHostToDevice);
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

    // Determine the bounding box region for this net.
    int xMin   = netBBox.xMin;
    int yMin   = netBBox.yMin;
    int width  = netBBox.xMax - netBBox.xMin + 1;
    int height = netBBox.yMax - netBBox.yMin + 1;

    // Source pointer in device memory and destination pointer in pinned host memory.
    NodeData2D* srcPtr = d_nodes + yMin * xDim + xMin;
    NodeData2D* dstPtr = h_nodesPinned + yMin * xDim + xMin;
    size_t pitch = xDim * sizeof(NodeData2D);

    // Enqueue an asynchronous 2D memory copy in the same stream.
    cudaMemcpy2DAsync(dstPtr, pitch, srcPtr, pitch,
      width * sizeof(NodeData2D), height,
      cudaMemcpyDeviceToHost,
      netStreams[netId]);
  }

  if (VERBOSE > 0) {
    std::cout << "Finish launchMazeRouteStream" << std::endl;
  }

  cudaCheckError();


  //===========================================================================
  // Launch asynchronous CPU tasks (one per net) to process the restored paths.
  //     Each task waits on its corresponding stream (i.e. for its GPU kernel and async copy
  //     to finish) and then processes the net's bounding-box region.
  std::vector<std::future<void> > cpuFutures;
  cpuFutures.reserve(numNets);
  for (int netId = 0; netId < numNets; netId++) {
    cpuFutures.push_back(std::async(std::launch::async, [&, netId]() {
      // Wait for the GPU work (kernel + async memcopy) in this stream to complete.
      cudaStreamSynchronize(netStreams[netId]);

      // Process the CPU-side path sync for this net.
      auto& net    = nets[netId];
      auto& uworker= uworkers[net->getWorkerId()];
      auto& gridGraph = uworker->getGridGraph();
      auto workerLL  = uworker->getRouteGCellIdxLL();
      int workerLX = workerLL.x();
      int workerLY = workerLL.y();
      auto& netBBox = netBBoxVec[netId];
      int LLX = netBBox.xMin;
      int LLY = netBBox.yMin;
      int URX = netBBox.xMax;
      int URY = netBBox.yMax;
      int xDimTemp = URX - LLX + 1;
      int numNodesLocal = xDimTemp * (URY - LLY + 1);
      
      for (int localIdx = 0; localIdx < numNodesLocal; localIdx++) {
        int localX = localIdx % xDimTemp;
        int localY = localIdx / xDimTemp;
        int x = localX + LLX;
        int y = localY + LLY;
        int idx = locToIdx_2D(x, y, xDim);

        int xRel = x - workerLX;
        int yRel = y - workerLY;
        int parentX = h_nodesPinned[idx].golden_parent_x - workerLX;
        int parentY = h_nodesPinned[idx].golden_parent_y - workerLY;
        gridGraph.setGoldenParent2D(xRel, yRel, parentX, parentY);
      }
    }));
  }

  // Wait for all CPU tasks to complete.
  for (auto& f : cpuFutures) {
    f.get();
  }


  /*  
  // cudaDeviceSynchronize();
  // Wait for all nets to finish
  for (int i = 0; i < numNets; i++) {
    cudaStreamSynchronize(netStreams[i]);
  }
  
  // We need to trace back the routing path on the CPU side
  cudaMemcpy(nodes.data(), d_nodes, numGrids * sizeof(NodeData2D), cudaMemcpyDeviceToHost);
  cudaCheckError();
  */

  /*
  int LX = netBBoxVec[0].xMin;
  int LY = netBBoxVec[0].yMin;
  int UX = netBBoxVec[0].xMax;
  int UY = netBBoxVec[0].yMax;
  if (LX == 74 && LY == 57 && UX == 83 && UY == 72) {
    for (int id = 0; id < nodes.size(); id++) {
      int2 xy = idxToLoc_2D(id, xDim);
      if (xy.x < LX || xy.x > UX || xy.y < LY || xy.y > UY) {
        continue;
      }
      std::cout << "id = " << id << " "
                << "x = " << xy.x << " y = " << xy.y << " "
                << "isSrc = " << (nodes[id].flags.src_flag == 1) << " "
                << "isDst = " << (nodes[id].flags.dst_flag == 1) << " "
                << "forward_visited_flag = " << (nodes[id].flags.forward_visited_flag == 1) << " "
                << "backward_visited_flag = " << (nodes[id].flags.backward_visited_flag == 1) << " "
                << "forward_g_cost = " << nodes[id].forward_g_cost << " "
                << "backward_g_cost = " << nodes[id].backward_g_cost << " "
                << "golden_parent_x = " << nodes[id].golden_parent_x << " "
                << "golden_parent_y = " << nodes[id].golden_parent_y << " "
                << std::endl;
    }
  }
  */

  if (VERBOSE > 0) {
    std::cout << "Finish the GPU routing" << std::endl;
  }

  // float syncupTime = batchPathSyncUp(uworkers, nets, netBBoxVec, nodes, xDim);
  // Reconstruct the nets similar to the CPU version
  // batchPathSyncUp(uworkers, nets, netBBoxVec, nodes, xDim);

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

  cudaFreeHost(h_nodesPinned);

  cudaCheckError();

  auto totalEnd = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double, std::milli> totalTime = totalEnd - totalStart;
  return totalTime.count();
}

} // namespace drt




