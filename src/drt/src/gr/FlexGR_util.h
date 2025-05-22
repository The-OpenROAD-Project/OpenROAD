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

  NetStruct(frNet* _net, int _netId, int _nodeCntStart, int _numNodes) 
    : net(_net), netId(_netId), nodeCntStart(_nodeCntStart), numNodes(_numNodes) { }
 
};




struct IntPair {
  int start = -1;
  int end = -1;

  IntPair() = default;

  IntPair(int _start, int _end) : start(_start), end(_end) { }

  __host__ __device__
  int x() const { return start; }
  
  __host__ __device__
  int y() const { return end; }

  __host__ __device__
  int start() const { return start; }

  __host__ __device__
  int end() const { return end; }

  __host__ __device__
  void set(int _start, int _end) {
    start = _start;
    end = _end;
  }
};


void printPeakMemoryUsage();


// -----------------------------------------------------------------------------
// CUDA Related Device functions
// -----------------------------------------------------------------------------

// For a node in a routed tree, there are at most 4 children nodes
struct __align__(16) NodeStruct 
{
  uint16_t nodeIdx; // 16-bit index
  uint16_t parentIdx; // 16-bit index
  uint16_t level; // 16-bit index
  uint8_t childCnt;
  uint8_t layerNum;
  uint8_t minLayerNum;
  uint8_t maxLayerNum;
  uint16_t children[4];
  uint32_t x;
  uint32_t y;
  int netId;
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