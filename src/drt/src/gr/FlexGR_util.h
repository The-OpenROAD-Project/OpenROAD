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

#pragma once



#include "odb/db.h"
#include "db/grObj/grShape.h"
#include "db/grObj/grVia.h"
#include "db/grObj/grPin.h"
#include "db/infra/frTime.h"
#include "db/obj/frGuide.h"
#include "dr/FlexMazeTypes.h"
#include "utl/exception.h"
#include "stt/SteinerTreeBuilder.h"
#include "gr/FlexGRWavefront.h"
#include "frBaseTypes.h"
#include "frDesign.h"

#include <iostream>
#include <queue>
#include <thread>
#include <tuple>
#include <sys/resource.h>
#include <cuda_runtime.h>
#include <cuda.h>
#include <thrust/device_vector.h>
#include <thrust/functional.h>
#include <thrust/host_vector.h>
#include <thrust/transform_reduce.h>

namespace drt {

// ------------------------------------------------------------------------------
// Hyperparameters for the FlexGR
// ------------------------------------------------------------------------------
constexpr int GRGRIDGRAPHHISTCOSTSIZE = 8;
constexpr int GRSUPPLYSIZE = 8;
constexpr int GRDEMANDSIZE = 16;
constexpr int GRFRACSIZE = 1;
constexpr int CMAPHISTSIZE = 8;
constexpr int CMAPSUPPLYSIZE = 8;
constexpr int CMAPDEMANDSIZE = 16;
constexpr int CMAPFRACSIZE = 1;
constexpr int VERBOSE = 0; 
constexpr uint32_t INF32 = 0xFFFFFFFF;


#define cudaCheckError()                                                   \
{                                                                          \
    cudaError_t err = cudaGetLastError();                                  \
    if (err != cudaSuccess) {                                              \
        fprintf(stderr, "CUDA error at %s:%d: %s\n",                       \
                __FILE__, __LINE__, cudaGetErrorString(err));              \
        exit(1);                                                           \
    }                                                                      \
}



// -------------------------------------------------------------------------------
// Some basic structures used in the FlexGR
// -------------------------------------------------------------------------------


// In the ripup and reroute stage, we need to keep tracking gcells with overflow
// and the nets passing through the grid
struct GridStruct {
  int xIdx = -1;
  int yIdx = -1;
  int zIdx = 0; // Default is 2D grid

  bool isOverflowH = false;
  bool isOverflowV = false;

  std::set<frNet*> nets; // All the nets passing through the grid
  
  GridStruct() = default;
  
  GridStruct(int _xIdx, int _yIdx, int _zIdx) 
    : xIdx(_xIdx), yIdx(_yIdx), zIdx(_zIdx) { }
    
};



// This is used for the net-level parallelization
struct NetStruct {
  frNet* net = nullptr;
  int netId = -1;
  
  int nodeCntStart = -1; // starting node index in the net
  int numNodes = -1;

  std::vector<int> points; // Store all the steiner points in the net
  std::vector<std::pair<int, int> > vSegments;
  std::vector<std::pair<int, int> > hSegments;

  NetStruct() = default;

  NetStruct(frNet* _net, int _netId) : net(_net), netId(_netId) { }

  void setNodeCntStart(int _nodeCntStart) { nodeCntStart = _nodeCntStart; }
  void setNumNodes(int _numNodes) { numNodes = _numNodes; }
};



struct IntPair {
  int start_val = -1;
  int end_val = -1;

  IntPair() = default;

  IntPair(int _start, int _end) : start_val(_start), end_val(_end) { }

  __host__ __device__
  int x() const { return start_val; }
  
  __host__ __device__
  int y() const { return end_val; }

  __host__ __device__
  int start() const { return start_val; }

  __host__ __device__
  int end() const { return end_val; }

  __host__ __device__
  void set(int _start, int _end) {
    start_val = _start;
    end_val = _end;
  }
};





// Do not use the default Point or Rect class in odb
// because they are not compatible with CUDA

struct Point2D {
  int x;
  int y;
  Point2D(int x, int y) : x(x), y(y) {}
};


// Point3D has been used in frPoint.h
struct Point3DCUDA {
  int x;
  int y;
  int z;
  Point3DCUDA(int x, int y, int z) : x(x), y(y), z(z) {}
};

struct Rect2D{
  int xMin;
  int yMin;
  int xMax;
  int yMax;

  Rect2D(int xMin, int yMin, int xMax, int yMax) : xMin(xMin), yMin(yMin), xMax(xMax), yMax(yMax) {}
};


enum Directions2D {
  DIR_NORTH    = 0,
  DIR_RIGHT = 1,
  DIR_SOUTH  = 2,
  DIR_LEFT  = 3,
  DIR_NONE  = 255
};


enum Directions3D {
  DIR_NORTH_3D   = 0,
  DIR_RIGHT_3D   = 1,
  DIR_SOUTH_3D   = 2,
  DIR_LEFT_3D    = 3,
  DIR_UP_3D      = 4,
  DIR_DOWN_3D    = 5,
  DIR_NONE_3D    = 255
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


struct NodeData3D {
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
  int golden_parent_z;

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


void printPeakMemoryUsage();


// -----------------------------------------------------------------------------
// CUDA Related Device functions
// -----------------------------------------------------------------------------

// For a node in a routed tree, there are at most 4 children nodes
struct NodeStruct 
{
  int x; // 4 bytes
  int y; // 4 bytes
  int netId; // 4 bytes
  int nodeIdx; // 16-bit index
  int parentIdx; // 16-bit index
  int children[4]; // 4 * 32-bit indices
  int level; // 16-bit index
  //uint16_t children[4];
  
  int childCnt;
  int layerNum;
  int minLayerNum;
  int maxLayerNum;
  
  NodeStruct()
    : x(-1), y(-1), netId(-1), nodeIdx(-1), parentIdx(-1), 
      level(-1), childCnt(0), 
      layerNum(-1), minLayerNum(100), maxLayerNum(0)
  {
    #pragma unroll
    for (int i = 0; i < 4; ++i) {
      children[i] = -1; // or 0, or another sentinel for "invalid"
    }
  }
};


// Bit-level Operations
__device__
int getIdx__device(int x, int y, int z, int xDim, int yDim, int zDim);

__device__
bool getBit__device(uint64_t* d_cmap, unsigned idx, unsigned pos);

__device__
unsigned getBits__device(uint64_t* d_cmap, unsigned idx, unsigned pos, unsigned length);

__device__
void setBits__device(uint64_t* d_cmap, unsigned idx, unsigned pos, unsigned length, unsigned val);

__device__
void addToBits__device(uint64_t* d_cmap, unsigned idx, unsigned pos, unsigned length, unsigned val);


// Note that we do not check if dir is aligned with the preferred direction
__device__
bool hasBlock__device(uint64_t* d_cmap, 
  int xDim, int yDim, int zDim,
  unsigned x, unsigned y, unsigned z, frDirEnum dir);

  
// Note that we do not check if dir is aligned with the preferred direction
__device__
unsigned getRawSupply__device(uint64_t* d_cmap, 
  int xDim, int yDim, int zDim,
  unsigned x, unsigned y, unsigned z, frDirEnum dir);


// Note that we do not check if dir is aligned with the preferred direction
__device__
unsigned getRawDemand__device(uint64_t* d_cmap,
  int xDim, int yDim, int zDim,
  unsigned x, unsigned y, unsigned z, frDirEnum dir);


__device__
void addRawDemand__device(uint64_t* d_cmap,
  int xDim, int yDim, int zDim,
  unsigned x, unsigned y, unsigned z, frDirEnum dir, unsigned delta = 1);


} // namespace drt