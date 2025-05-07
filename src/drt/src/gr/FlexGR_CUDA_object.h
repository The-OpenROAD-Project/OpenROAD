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

#include <vector>
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
#include "frDesign.h"
#include "FlexGRCMap.h"

namespace drt {

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


}

