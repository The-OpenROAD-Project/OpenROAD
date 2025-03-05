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



enum Directions3D {
  DIR_NORTH_3D   = 0,
  DIR_RIGHT_3D   = 1,
  DIR_SOUTH_3D   = 2,
  DIR_LEFT_3D    = 3,
  DIR_UP_3D      = 4,
  DIR_DOWN_3D    = 5,
  DIR_NONE_3D    = 255
};


__host__ __device__ 
void initNodeData3D(NodeData3D& nd) {
  nd.forward_h_cost = INF32;
  nd.forward_g_cost = INF32;
  nd.backward_h_cost = INF32;
  nd.backward_g_cost = INF32;
  nd.forward_h_cost_prev = INF32;
  nd.forward_g_cost_prev = INF32;
  nd.backward_h_cost_prev = INF32;
  nd.backward_g_cost_prev = INF32;
  nd.forward_direction = DIR_NONE_3D;
  nd.backward_direction = DIR_NONE_3D;
  nd.forward_direction_prev = DIR_NONE_3D;
  nd.backward_direction_prev = DIR_NONE_3D;
  nd.golden_parent_x = -1;
  nd.golden_parent_y = -1;
  nd.golden_parent_z = -1;
  nd.flags.src_flag = 0;
  nd.flags.dst_flag = 0;
  nd.flags.forward_update_flag = 0;
  nd.flags.backward_update_flag = 0;
  nd.flags.forward_visited_flag = 0;
  nd.flags.backward_visited_flag = 0;
  nd.flags.forward_visited_flag_prev = 0;
  nd.flags.backward_visited_flag_prev = 0;
}


__device__  
uint8_t computeParentDirection3D(int d) {
  switch(d) {
    case 0: return DIR_NORTH_3D;
    case 1: return DIR_RIGHT_3D;
    case 2: return DIR_SOUTH_3D;
    case 3: return DIR_LEFT_3D;
    case 4: return DIR_UP_3D;
    case 5: return DIR_DOWN_3D;
    default: return DIR_NONE_3D;
  }
}


// Invert direction for backtracking
__device__  
uint8_t invertDirection3D(uint8_t d) {
  switch(d) {
    case DIR_NORTH_3D:    return DIR_SOUTH_3D;
    case DIR_SOUTH_3D:    return DIR_NORTH_3D;
    case DIR_LEFT_3D:     return DIR_RIGHT_3D;
    case DIR_RIGHT_3D:    return DIR_LEFT_3D;
    case DIR_UP_3D:       return DIR_DOWN_3D;
    case DIR_DOWN_3D:     return DIR_UP_3D;
    default:           return DIR_NONE_3D;
  }
}


__device__ __host__  
int3 idxToLoc_3D(int idx, int xDim, int yDim) {
  int z = idx / (xDim * yDim);
  int temp = idx % (xDim * yDim);
  int y = temp / xDim;
  int x = temp % xDim;
  return make_int3(x,y,z);
}


__device__ __host__  
int locToIdx_3D(int x, int y, int z, int xDim, int yDim) {
  return z * xDim * yDim + y * xDim + x;
}


__host__ __device__
unsigned getEdgeLength3D(
  const int* xCoords, 
  const int* yCoords, 
  const int* zHeights,
  int x, int y, int z, Directions3D dir)
{
  switch (dir) {
    case Directions3D::DIR_RIGHT_3D:
      return xCoords[x + 1] - xCoords[x];
    case Directions3D::DIR_NORTH_3D:
      return yCoords[y + 1] - yCoords[y];
    case Directions3D::DIR_UP_3D:
      return zHeights[z + 1] - zHeights[z];
    default:
      return 0;
  }
}


// We do not consider bending cost in this version
__host__ __device__
uint32_t getEdgeCost3D(
  const uint64_t* d_costMap,
  const int* d_xCoords,
  const int* d_yCoords,
  const int* d_zHeights,
  float congThreshold,
  unsigned BLOCKCOST,
  unsigned OVERFLOWCOST,
  unsigned HISTCOST, 
  int idx, int x, int y, int z,
  Directions3D dir)
{
  return getEdgeLength3D(d_xCoords, d_yCoords, d_zHeights, x, y, z, dir);
}



__device__ 
uint32_t getNeighorCost3D(
  const uint64_t* d_costMap,
  const int* d_xCoords,
  const int* d_yCoords,
  const int* d_zHeights,
  float congThreshold,
  unsigned BLOCKCOST,
  unsigned OVERFLOWCOST,
  unsigned HISTCOST,
  int idx, int x, int y, int z,
  int nbrIdx, int nx, int ny, int nz)
{
  uint32_t newG = 0;
  if (nx == x && ny == y - 1 && nz == z) {
    newG += getEdgeCost3D(d_costMap,     
      d_xCoords, d_yCoords, d_zHeights,
      congThreshold, BLOCKCOST, OVERFLOWCOST, HISTCOST,
      nbrIdx, nx, ny, nz, Directions3D::DIR_NORTH_3D);
  } else if (nx == x && ny == y + 1 && nz == z) {
    newG += getEdgeCost3D(d_costMap, 
      d_xCoords, d_yCoords, d_zHeights,
      congThreshold, BLOCKCOST, OVERFLOWCOST, HISTCOST,
      idx, x, y, z, Directions3D::DIR_NORTH_3D);
  } else if (nx == x - 1 && ny == y && nz == z) {
    newG += getEdgeCost3D(d_costMap, 
      d_xCoords, d_yCoords, d_zHeights,
      congThreshold, BLOCKCOST, OVERFLOWCOST, HISTCOST,
      nbrIdx, nx, ny, nz, Directions3D::DIR_RIGHT_3D);
  } else if (nx == x + 1 && ny == y && nz == z) {
    newG += getEdgeCost3D(d_costMap, 
      d_xCoords, d_yCoords, d_zHeights,
      congThreshold, BLOCKCOST, OVERFLOWCOST, HISTCOST,
      idx, x, y, z, Directions3D::DIR_RIGHT_3D);
  } else if (nx == x && ny == y && nz == z - 1) {
    newG += getEdgeCost3D(d_costMap, 
      d_xCoords, d_yCoords, d_zHeights,
      congThreshold, BLOCKCOST, OVERFLOWCOST, HISTCOST,
      nbrIdx, nx, ny, nz, Directions3D::DIR_UP_3D);
  } else if (nx == x && ny == y && nz == z + 1) {
    newG += getEdgeCost3D(d_costMap, 
      d_xCoords, d_yCoords, d_zHeights,
      congThreshold, BLOCKCOST, OVERFLOWCOST, HISTCOST,
      idx, x, y, z, Directions3D::DIR_UP_3D);
  }

  return newG;
}



__device__
void initNodeData3D__device(
  NodeData3D* d_nodes,
  int* d_pins, int pinIterStart, int pinIter,  // Pin related variables
  int LLX, int LLY, int URX, int URY, // Bounding box
  int xDim, int yDim, int zDim) 
{ 
  int total = (URX - LLX + 1) * (URY - LLY + 1) * zDim;
  int xDimTemp = URX - LLX + 1;
  int yDimTemp = URY - LLY + 1;
  int tid = threadIdx.x;
  int stride = blockDim.x;
  
  for (int localIdx = tid; localIdx < total; localIdx += stride) {
    int3 local = idxToLoc_3D(localIdx, xDimTemp, yDimTemp);
    int x = local.x + LLX;
    int y = local.y + LLY;
    int z = local.z;
    int idx = locToIdx_3D(x, y, z, xDim, yDim);
  
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

    d_nodes[idx].forward_direction = DIR_NONE_3D;
    d_nodes[idx].backward_direction = DIR_NONE_3D;
    d_nodes[idx].forward_direction_prev = DIR_NONE_3D;
    d_nodes[idx].backward_direction_prev = DIR_NONE_3D;
    d_nodes[idx].flags.forward_update_flag = false;
    d_nodes[idx].flags.backward_update_flag = false;
    d_nodes[idx].flags.forward_visited_flag = false;
    d_nodes[idx].flags.backward_visited_flag = false;
    d_nodes[idx].flags.forward_visited_flag_prev = false;
    d_nodes[idx].flags.backward_visited_flag_prev = false;
  } 
}



// Define the device function for the biwaveBellmanFord_2D_v4__device
__device__
void runBiBellmanFord3D__device(
  int netId,
  NodeData3D* d_nodes,
  uint64_t* d_costMap, 
  const int* __restrict__ d_dX, 
  const int* __restrict__ d_dY,
  const int* __restrict__ d_dZ,
  const int* __restrict__ d_xCoords,
  const int* __restrict__ d_yCoords,
  const int* __restrict__ d_zHeights,
  int LLX, int LLY, int URX, int URY,
  int xDim, int yDim, int zDim, 
  int maxIters,
  float congThreshold,
  unsigned BLOCKCOST,
  unsigned OVERFLOWCOST,
  unsigned HISTCOST)
{
  // To handle the case where maxIters is too small 
  if (URX - LLX == 0) {
    maxIters = maxIters < 2 * (URY - LLY) * zDim ? 2 * (URY - LLY) * zDim : maxIters;
  }

  if (URY - LLY == 0) {
    maxIters = maxIters < 2 * (URX - LLX) * zDim ? 2 * (URX - LLX) * zDim : maxIters;
  }
    
  // Each device function is handled by a single block
  int total = (URX - LLX + 1) * (URY - LLY + 1) * zDim;
  int tid = threadIdx.x;
  int stride = blockDim.x; 
  int xDimTemp = URX - LLX + 1;
  int yDimTemp = URY - LLY + 1;  

  // Define the shared memory for d_dx and d_dy
  __shared__ int s_dX[6];
  __shared__ int s_dY[6];
  __shared__ int s_dZ[6];
  __shared__ volatile int s_doneFlag;
  __shared__ volatile int s_minCost;
  //__shared__ volatile int s_meetId;
  __shared__ unsigned long long s_meet;
  __shared__ volatile int tracebackError;   // 0: no error; 1: error detected
  __shared__ int s_minForwardCost;
  __shared__ int s_minBackwardCost;
  
  // Load the d_dX and d_dY into shared memory
  if (tid < 6) {
    s_dX[tid] = d_dX[tid];
    s_dY[tid] = d_dY[tid];
    s_dZ[tid] = d_dZ[tid];
    if (tid == 0) {
      s_doneFlag = 0;
      s_minCost = 0x7FFFFFFF;
      s_meet = 0xFFFFFFFFFFFFFFFFULL;
      tracebackError = 0;
      s_minForwardCost = 0x7FFFFFFF;
      s_minBackwardCost = 0x7FFFFFFF;
    }
  }
  __syncthreads();

  for (int localIdx = tid; localIdx < total; localIdx += stride) {
    int3 local = idxToLoc_3D(localIdx, xDimTemp, yDimTemp);
    int x = local.x + LLX;
    int y = local.y + LLY;
    int z = local.z;
    int idx = locToIdx_3D(x, y, z, xDim, yDim);
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
      int3 local = idxToLoc_3D(localIdx, xDimTemp, yDimTemp);
      int x = local.x + LLX;
      int y = local.y + LLY;
      int z = local.z;
      int idx = locToIdx_3D(x, y, z, xDim, yDim);
      NodeData3D &nd = d_nodes[idx];

      // Forward relaxation:
      // Skip if src_flag is set.
      if (!nd.flags.src_flag) {
        uint32_t bestCost = nd.forward_g_cost_prev;
        int      bestD    = -1;
        for (int d = 0; d < 6; d++) {
          int nx = x + s_dX[d];
          int ny = y + s_dY[d];
          int nz = z + s_dZ[d];
          if (nx < LLX || nx > URX || ny < LLY || ny > URY || nz < 0 || nz >= zDim) {
            continue;  // out of bounds
          }
          int nbrIdx = locToIdx_3D(nx, ny, nz, xDim, yDim);
          uint32_t neighborCost = d_nodes[nbrIdx].forward_g_cost_prev;
          if (neighborCost == 0xFFFFFFFF) {
            continue;
          }
          uint32_t newG = neighborCost +
            getNeighorCost3D(d_costMap, d_xCoords, d_yCoords, d_zHeights,
              congThreshold, BLOCKCOST, OVERFLOWCOST, HISTCOST,
              idx, x, y, z, nbrIdx, nx, ny, nz);
          if (newG < bestCost) {
            bestCost = newG;
            bestD    = d;
          }
        } // end neighbor loop

        if (bestD != -1) { // Found an improvement
          nd.forward_g_cost = bestCost;
          nd.forward_direction = computeParentDirection3D(bestD);
          nd.flags.forward_update_flag = 1;
        }
      } // end forward

      // Backward relaxation:
      // newCost = min over neighbors of (neighbor.backward_cost + edgeWeight).
      // Skip if dst_flag is set.
      if (!nd.flags.dst_flag) {
        uint32_t bestCost = nd.backward_g_cost_prev;
        int      bestD    = -1;
        for (int d = 0; d < 6; d++) {
          int nx = x + s_dX[d];
          int ny = y + s_dY[d];
          int nz = z + s_dZ[d];
          if (nx < LLX || nx > URX || ny < LLY || ny > URY || nz < 0 || nz >= zDim) {
            continue;  // out of bounds
          }
          
          int nbrIdx = locToIdx_3D(nx, ny, nz, xDim, yDim);
          uint32_t neighborCost = d_nodes[nbrIdx].backward_g_cost_prev;
          if (neighborCost == 0xFFFFFFFF) {
            continue;
          }
          uint32_t newG = neighborCost +
            getNeighorCost3D(d_costMap, 
              d_xCoords, d_yCoords, d_zHeights,
              congThreshold, BLOCKCOST, OVERFLOWCOST, HISTCOST,
              idx, x, y, z, nbrIdx, nx, ny, nz);
          if (newG < bestCost) {
            bestCost = newG;
            bestD    = d;
          }
        } // end neighbor loop

        if (bestD != -1) {
          nd.backward_g_cost = bestCost;
          nd.backward_direction = computeParentDirection3D(bestD);
          nd.flags.backward_update_flag = 1;
        }
      } // end backward
    } // end for each node (relaxation)
    __syncthreads();

    ////////////////////////////////////////////////////////////////////////////
    // (2) Commit updated costs (double-buffering technique)
    ////////////////////////////////////////////////////////////////////////////
    for (int localIdx = tid; localIdx < total; localIdx += stride) {
      int3 local = idxToLoc_3D(localIdx, xDimTemp, yDimTemp);
      int x = local.x + LLX;
      int y = local.y + LLY;
      int z = local.z;
      int idx = locToIdx_3D(x, y, z, xDim, yDim);
      NodeData3D &nd = d_nodes[idx];
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
      int3 local = idxToLoc_3D(localIdx, xDimTemp, yDimTemp);
      int x = local.x + LLX;
      int y = local.y + LLY;
      int z = local.z;
      int idx = locToIdx_3D(x, y, z, xDim, yDim);
      NodeData3D &nd = d_nodes[idx];
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
      printf("Error! biwaveBellmanFord3D__device did not converge. doneFlag = false netId = %d\n", netId);
    }
    __syncthreads();
    return;
  } 
  __syncthreads();  

  // Iterate over your domain. Assume tid and stride are defined appropriately.
  for (int localIdx = tid; localIdx < total; localIdx += stride) {
    int3 local = idxToLoc_3D(localIdx, xDimTemp, yDimTemp);
    int x = local.x + LLX;
    int y = local.y + LLY;
    int z = local.z;
    int idx = locToIdx_3D(x, y, z, xDim, yDim);
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
      printf("Error! biwaveBellmanFord3D__device did not converge. meetId = 0x7FFFFFFF, netId = %d\n", netId);
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
      int tempIter = 0;      
      // Update the meetId accordingly to remove redundant path
      while (d_nodes[s_meetId].forward_direction == d_nodes[s_meetId].backward_direction && tempIter < total) {
        if (d_nodes[s_meetId].forward_direction == DIR_NONE_3D) {
          printf("Warning: forward_direction == DIR_NONE_3D netId = %d s_meetId = %d\n", netId, s_meetId);
          break;
        }
        
        int3 xyz = idxToLoc_3D(s_meetId, xDim, yDim);
        auto direction = d_nodes[s_meetId].forward_direction;
        int nx = xyz.x + s_dX[direction];
        int ny = xyz.y + s_dY[direction];
        int nz = xyz.z + s_dZ[direction];
        s_meetId = locToIdx_3D(nx, ny, nz, xDim, yDim);
        tempIter++;
      }

      if (tempIter >= total) {
        printf("Warning: reduce iteration exceeded maximum iterations. netId = %d\n", netId);
      }

      // forward trace
      int forwardCurId = s_meetId;
      int forwardIteration = 0;
      while (!d_nodes[forwardCurId].flags.src_flag && forwardIteration < total) {
        uint8_t fwdDir = d_nodes[forwardCurId].forward_direction;
        int3 xyz = idxToLoc_3D(forwardCurId, xDim, yDim);
        int nx = xyz.x + s_dX[fwdDir];
        int ny = xyz.y + s_dY[fwdDir];
        int nz = xyz.z + s_dZ[fwdDir];
        if (nx < LLX || nx > URX || ny < LLY || ny > URY || nz < 0 || nz >= zDim) {
          break;
        }
        d_nodes[forwardCurId].golden_parent_x = nx;
        d_nodes[forwardCurId].golden_parent_y = ny;
        d_nodes[forwardCurId].golden_parent_z = nz;
        d_nodes[forwardCurId].flags.src_flag = 1;
        forwardCurId = locToIdx_3D(nx, ny, nz, xDim, yDim);
        forwardIteration++;
      }
      
      if (forwardIteration >= total) {
        printf("Warning: Forward traceback exceeded maximum iterations. netId = %d\n", netId);
      }
   
      // backward trace
      int backwardCurId = s_meetId;
      int backwardIteration = 0;
      if (d_nodes[backwardCurId].flags.dst_flag == 1) {
        d_nodes[backwardCurId].flags.dst_flag = 0; // Reset dst flag.
        d_nodes[backwardCurId].flags.src_flag = 1;
      } else {
        while (!d_nodes[backwardCurId].flags.dst_flag && backwardIteration < total) {
          int3 xyz = idxToLoc_3D(backwardCurId, xDim, yDim);
          uint8_t backwardDir = d_nodes[backwardCurId].backward_direction;
          int nx = xyz.x + s_dX[backwardDir];
          int ny = xyz.y + s_dY[backwardDir];
          int nz = xyz.z + s_dZ[backwardDir];      
          if (nx < LLX || nx > URX || ny < LLY || ny > URY || nz < 0 || nz >= zDim) {
            break;
          }
          int nextId = locToIdx_3D(nx, ny, nz, xDim, yDim);       
          d_nodes[nextId].flags.src_flag = 1;
          d_nodes[nextId].golden_parent_x = xyz.x;
          d_nodes[nextId].golden_parent_y = xyz.y;
          d_nodes[nextId].golden_parent_z = xyz.z;
          backwardCurId = nextId;
          backwardIteration++;
        }
        
        d_nodes[backwardCurId].flags.dst_flag = 0;
        if (backwardIteration >= total) {
          printf("Warning: Backward traceback exceeded maximum iterations. netId = %d\n", netId);
        }
      }
    }
  }
  __syncthreads();
}


__device__ 
void biwaveBellmanFord3D__device(
  int netId,
  int* d_netPtr,
  Rect2D_CUDA* d_netBBoxVec,
  int* d_pins,
  uint64_t* d_costMap,
  NodeData3D* d_nodes,
  int* d_dX,
  int* d_dY,
  int* d_dZ,
  const int* d_xCoords,
  const int* d_yCoords,
  const int* d_zHeights,
  int maxIters,
  int xDim,
  int yDim,
  int zDim,
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
    initNodeData3D__device(
      d_nodes,
      d_pins, pinIdxStart, pinIter, 
      LLX, LLY, URX, URY, 
      xDim, yDim, zDim);

    __syncthreads(); // Synchronize all threads in the block

    // Run the Bellman Ford algorithm
    runBiBellmanFord3D__device(
      netId, 
      d_nodes, d_costMap, 
      d_dX, d_dY, d_dZ,
      d_xCoords, d_yCoords, d_zHeights,
      LLX, LLY, URX, URY, 
      xDim, yDim, zDim, maxIters,
      congThreshold, BLOCKCOST, OVERFLOWCOST, HISTCOST);  

    __syncthreads(); // Synchronize all threads in the block
  }
}


__global__ 
void biwaveBellmanFord3D__kernel(
  int netStartId,
  int netEndId,
  int* d_netBatchIdx,
  int* d_netPtr,
  Rect2D_CUDA* d_netBBoxVec,
  int* d_pins,
  uint64_t* d_costMap,
  NodeData3D* d_nodes,
  int* d_dX,
  int* d_dY,
  int* d_dZ,
  const int* d_xCoords,
  const int* d_yCoords,
  const int* d_zHeights,
  int maxIters,
  int xDim,
  int yDim,
  int zDim,
  float congThreshold,
  int BLOCKCOST,
  int OVERFLOWCOST, 
  int HISTCOST)
{
  // Each net is handled by a single block
  for (int netId = netStartId + blockIdx.x; netId < netEndId; netId += gridDim.x) {
    biwaveBellmanFord3D__device(
      netId,
      d_netPtr,
      d_netBBoxVec,
      d_pins,
      d_costMap,
      d_nodes + d_netBatchIdx[netId] * xDim * yDim * zDim,
      d_dX,
      d_dY,
      d_dZ,
      d_xCoords,
      d_yCoords,
      d_zHeights,
      maxIters,
      xDim,
      yDim,
      zDim,
      congThreshold,
      BLOCKCOST,
      OVERFLOWCOST, 
      HISTCOST);
  }
}



__global__ 
void initBatchNodeData3D__kernel(
  NodeData3D* d_nodes,
  int numNodes)
{
  for (int idx = blockIdx.x * blockDim.x + threadIdx.x; idx < numNodes; idx += gridDim.x * blockDim.x) {
    initNodeData3D(d_nodes[idx]);
  }
}

__global__
void initParent3D__kernel(
  Point3D_CUDA* d_parents,
  int numParents)
{
  for (int idx = blockIdx.x * blockDim.x + threadIdx.x; idx < numParents; idx += gridDim.x * blockDim.x) {
    d_parents[idx].x = -1;
    d_parents[idx].y = -1;
    d_parents[idx].z = -1;
  }
}

__global__
void copyParents3D__kernel(
  NodeData3D* d_nodes,
  Point3D_CUDA* d_parents,
  int numNodes)
{
  for (int idx = blockIdx.x * blockDim.x + threadIdx.x; idx < numNodes; idx += gridDim.x * blockDim.x) {
    d_parents[idx].x = d_nodes[idx].golden_parent_x;
    d_parents[idx].y = d_nodes[idx].golden_parent_y;
    d_parents[idx].z = d_nodes[idx].golden_parent_z;
  }
}

__global__
void initBatchPin3D__kernel(
  NodeData3D* d_nodes,
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
    int pinId = d_pins[pinIdxStart] + baseNodeId;
    d_nodes[pinId].flags.src_flag = true;
    for (int pinIter = pinIdxStart + 1; pinIter < pinIdxEnd; pinIter++) {
      pinId = d_pins[pinIter] + baseNodeId;
      d_nodes[pinId].flags.dst_flag = true;
    }
  }
}


float FlexGR::GPUAccelerated3DMazeRoute_update(
  std::vector<std::unique_ptr<FlexGRWorker> >& uworkers,
  std::vector<std::vector<grNet*> >& netBatches,
  std::vector<int>& validBatches,
  std::vector<Point3D_CUDA>& h_parents_3D,
  std::vector<uint64_t>& h_costMap_3D,
  std::vector<int>& h_xCoords,
  std::vector<int>& h_yCoords,
  std::vector<int>& h_zHeights,
  float relaxThreshold,
  float congThreshold,
  unsigned BLOCKCOST,
  unsigned OVERFLOWCOST,
  unsigned HISTCOST,
  int maxChunkSize,
  int xDim, int yDim, int zDim)
{
  // Start overall timing.
  auto totalStart = std::chrono::high_resolution_clock::now();
  int numGrids = xDim * yDim * zDim;
  int numBatches = validBatches.size();  
  if (numBatches == 0) {
    return 0.0;
  }

  int maxHPWL = 0; // We will run the algorithm for maxHPWL * relaxThreshold iteratively
  int maxBatchSize = 0;
  int minBatchSize = std::numeric_limits<int>::max();

  // To do list:
  // To avoid frequent memory allocation, we will allocate the memory once  
  std::vector<int> netPtr;
  std::vector<int> netBatchIdxVec; 
  std::vector<Rect2D_CUDA> netBBoxVec;
  std::vector<int> pinIdxVec;
  std::vector<int> chunkNetPtr; // store the first netIdx of each chunk
 
  netPtr.push_back(0);
  chunkNetPtr.push_back(0);
    
  int batchChunkIdx = 0;
  for (int batchIdx = 0; batchIdx < numBatches; batchIdx++) {
    auto& batch = netBatches[validBatches[batchIdx]];
    for (auto& net : batch) {
      for (auto& idx : net->getPinGCellAbsIdxs()) {
        pinIdxVec.push_back(locToIdx_3D(idx.x(), idx.y(), idx.z(), xDim, yDim));
      }
      netBatchIdxVec.push_back(batchChunkIdx);
      netPtr.push_back(pinIdxVec.size());    
      auto netBBox = net->getRouteAbsBBox();
      netBBoxVec.push_back(Rect2D_CUDA(netBBox.xMin(), netBBox.yMin(), netBBox.xMax(), netBBox.yMax()));
      // To be updated 
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
   
  int chunkSize = std::min(maxChunkSize, numBatches);
  int maxIters = static_cast<int>(maxHPWL * relaxThreshold);
  // numNodes == parentSize
  int numNodes = numGrids * chunkSize;
  int parentSize = numNodes;

  allocateCUDAMem3D(
    h_costMap_3D,
    h_xCoords,
    h_yCoords,
    h_zHeights,
    h_parents_3D,
    pinIdxVec,
    netPtr,
    netBBoxVec,
    netBatchIdxVec,
    numGrids, 
    maxChunkSize,
    numNodes);
  
  cudaCheckError();


  for (int chunkId = 0; chunkId < numChunks; chunkId++) {
    int netStartId = chunkNetPtr[chunkId];
    int netEndId = chunkNetPtr[chunkId + 1];
    
    // Perform Global Initialization
    // Just use the part that we need
    int numThreads = 1024;
    int numBatchBlocks = (numNodes + numThreads - 1) / numThreads;
    int numParentBlocks = (parentSize + numThreads - 1) / numThreads;    
    initParent3D__kernel<<<numParentBlocks, numThreads>>>(d_parents_3D_, parentSize);
    initBatchNodeData3D__kernel<<<numBatchBlocks, numThreads>>>(d_nodes_3D_, numNodes);
    
    cudaDeviceSynchronize();
    cudaCheckError();
  
    int numNets = netEndId - netStartId;
    int numNetBlocks = (numNets + numThreads - 1) / numThreads;
    initBatchPin3D__kernel<<<numNetBlocks, numThreads>>>(
      d_nodes_3D_,
      d_pinIdxVec_, 
      d_netPtr_,
      d_netBatchIdx_,
      netStartId,
      netEndId,
      numGrids);
    cudaDeviceSynchronize();
    cudaCheckError();

    int numBlocks = numNets;
    biwaveBellmanFord3D__kernel<<<numBlocks, numThreads>>>(
      netStartId,
      netEndId,
      d_netBatchIdx_,
      d_netPtr_,
      d_netBBox_,
      d_pinIdxVec_,
      d_costMap_,
      d_nodes_3D_,
      d_dX_,
      d_dY_,
      d_dZ_,
      d_xCoords_,
      d_yCoords_,
      d_zHeights_,
      maxIters,
      xDim,
      yDim,
      zDim,
      congThreshold,
      BLOCKCOST,
      OVERFLOWCOST,
      HISTCOST);
    cudaDeviceSynchronize();
    cudaCheckError();
   
    // copy the back results to the d_parents
    copyParents3D__kernel<<<numParentBlocks, numThreads>>>(
      d_nodes_3D_, 
      d_parents_3D_, 
      numNodes);
    cudaDeviceSynchronize();
    cudaCheckError();

    cudaMemcpy(
      h_parents_3D.data(), 
      d_parents_3D_, 
      h_parents_3D.size() * sizeof(Point3D_CUDA), 
      cudaMemcpyDeviceToHost);
    cudaCheckError();
  }  

  auto totalEnd = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double, std::milli> totalTime = totalEnd - totalStart;
  return totalTime.count();
}



void FlexGR::allocateCUDAMem3D(
  std::vector<uint64_t>& h_costMap,
  std::vector<int>& h_xCoords,
  std::vector<int>& h_yCoords,
  std::vector<int>& h_zHeights,
  std::vector<Point3D_CUDA>& h_parents,
  std::vector<int>& pinIdxVec,
  std::vector<int>& netPtr,
  std::vector<Rect2D_CUDA>& netBBoxVec,
  std::vector<int>& netBatchIdxVec,
  int numGrids,
  int maxChunkSize,
  int numNodes)
{  
  if (d_dZ_ == nullptr) {
    std::vector<int> h_dX = {1, 0, -1, 0, 0, 0};
    std::vector<int> h_dY = {0, 1, 0, -1, 0, 0};
    std::vector<int> h_dZ = {0, 0, 0, 0, 1, -1};
    cudaMalloc(&d_dX_, 6 * sizeof(int));
    cudaMalloc(&d_dY_, 6 * sizeof(int));
    cudaMalloc(&d_dZ_, 6 * sizeof(int));
    cudaMemcpy(d_dX_, h_dX.data(), 6 * sizeof(int), cudaMemcpyHostToDevice);
    cudaMemcpy(d_dY_, h_dY.data(), 6 * sizeof(int), cudaMemcpyHostToDevice);
    cudaMemcpy(d_dZ_, h_dZ.data(), 6 * sizeof(int), cudaMemcpyHostToDevice);
  }
  
  if (h_xCoords.size() > h_xCoords_size_) {
    h_xCoords_size_ = h_xCoords.size();
    cudaFree(d_xCoords_);
    cudaMalloc(&d_xCoords_, h_xCoords.size() * sizeof(int));
    cudaMemcpy(d_xCoords_, h_xCoords.data(), h_xCoords.size() * sizeof(int), cudaMemcpyHostToDevice); 
  }

  if (h_yCoords.size() > h_yCoords_size_) {
    h_yCoords_size_ = h_yCoords.size();
    cudaFree(d_yCoords_);
    cudaMalloc(&d_yCoords_, h_yCoords.size() * sizeof(int));
    cudaMemcpy(d_yCoords_, h_yCoords.data(), h_yCoords.size() * sizeof(int), cudaMemcpyHostToDevice);
  }

  if (h_zHeights.size() > h_zHeights_size_) {
    h_zHeights_size_ = h_zHeights.size();
    cudaFree(d_zHeights_);
    cudaMalloc(&d_zHeights_, h_zHeights.size() * sizeof(int));
    cudaMemcpy(d_zHeights_, h_zHeights.data(), h_zHeights.size() * sizeof(int), cudaMemcpyHostToDevice);
  }

  if (h_costMap.size() > h_costMap_size_) {
    h_costMap_size_ = h_costMap.size();
    cudaFree(d_costMap_);
    cudaMalloc(&d_costMap_, h_costMap.size() * sizeof(uint64_t));
  }
  cudaMemcpy(d_costMap_, h_costMap.data(), h_costMap.size() * sizeof(uint64_t), cudaMemcpyHostToDevice);


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

  if (h_parents.size() > h_parents_size_3D_) {
    // We only allocate once
    int maxParentSize = numGrids * maxChunkSize;
    h_parents_size_3D_ = maxParentSize;
    cudaFree(d_parents_3D_);
    cudaMalloc(&d_parents_3D_, maxParentSize * sizeof(Point3D_CUDA));
  }

  if (numNodes > h_nodes_size_) {
    int maxNodeSize = numGrids * maxChunkSize;
    h_nodes_size_3D_ = maxNodeSize;
    cudaFree(d_nodes_3D_);
    cudaMalloc(&d_nodes_3D_, maxNodeSize * sizeof(NodeData3D));    
  }
  cudaCheckError();
}


void FlexGR::freeCUDAMem3D()
{
  cudaFree(d_dX_);
  cudaFree(d_dY_);
  cudaFree(d_dZ_);
  cudaFree(d_costMap_);
  cudaFree(d_xCoords_);
  cudaFree(d_yCoords_);
  cudaFree(d_zHeights_);
  cudaFree(d_nodes_3D_);
  cudaFree(d_parents_3D_);
  cudaFree(d_pinIdxVec_);
  cudaFree(d_netPtr_);
  cudaFree(d_netBBox_);
  cudaFree(d_netBatchIdx_);

  d_dX_ = nullptr;
  d_dY_ = nullptr;
  d_dZ_ = nullptr;
  d_costMap_ = nullptr;
  d_xCoords_ = nullptr;
  d_yCoords_ = nullptr;
  d_zHeights_ = nullptr;
  d_nodes_3D_ = nullptr;
  d_parents_3D_ = nullptr;
  d_pinIdxVec_ = nullptr;
  d_netPtr_ = nullptr;
  d_netBBox_ = nullptr;
  d_netBatchIdx_ = nullptr;

  h_costMap_size_ = 0;
  h_xCoords_size_ = 0;
  h_yCoords_size_ = 0;
  h_zHeights_size_ = 0;
  h_nodes_size_3D_ = 0;
  h_parents_size_3D_ = 0;
  h_pinIdxVec_size_ = 0;
  h_netPtr_size_ = 0;
  h_netBBoxVec_size_ = 0;
  h_netBatchIdxVec_size_ = 0;

  cudaCheckError();
}










} // namespace drt
