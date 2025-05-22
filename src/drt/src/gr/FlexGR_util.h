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

#pragma once



#include "db/grObj/grPin.h"
#include "dr/FlexMazeTypes.h"
#include "frBaseTypes.h"
#include "frDesign.h"
#include "gr/FlexGRWavefront.h"

#include <iostream>
#include <sys/resource.h>
#include <cuda_runtime.h>
#include <cuda.h>
#include <thrust/device_vector.h>
#include <thrust/functional.h>
#include <thrust/host_vector.h>
#include <thrust/transform_reduce.h>

namespace drt {

void printPeakMemoryUsage();


__host__ __device__
bool addEdge(uint64_t* bits,
  const bool* zDir, int xDim, int yDim, int zDim, 
  frMIdx x, frMIdx y, frMIdx z, frDirEnum dir);

__host__ __device__
bool removeEdge(uint64_t* bits,
  const bool* zDir, int xDim, int yDim, int zDim, 
  frMIdx x, frMIdx y, frMIdx z, frDirEnum dir);

__host__ __device__
bool setBlock(uint64_t* bits,
  const bool* zDir, int xDim, int yDim, int zDim, 
  frMIdx x, frMIdx y, frMIdx z, frDirEnum dir);

__host__ __device__
bool resetBlock(uint64_t* bits,
  const bool* zDir, int xDim, int yDim, int zDim, 
  frMIdx x, frMIdx y, frMIdx z, frDirEnum dir);

__host__ __device__
void setHistoryCost(uint64_t* bits, 
  const bool* zDir, int xDim, int yDim, int zDim, 
  frMIdx x, frMIdx y, frMIdx z, unsigned histCostIn);

__host__ __device__
void addHistoryCost(uint64_t* bits, 
  const bool* zDir, int xDim, int yDim, int zDim, 
  frMIdx x, frMIdx y, frMIdx z, unsigned in = 1);

__host__ __device__
void decayHistoryCost(uint64_t* bits, 
  const bool* zDir, int xDim, int yDim, int zDim, 
  frMIdx x, frMIdx y, frMIdx z);

__host__ __device__
void decayHistoryCost(uint64_t* bits, 
  const bool* zDir, int xDim, int yDim, int zDim, 
  frMIdx x, frMIdx y, frMIdx z, double d);


// E == H; N == V; currently no U / D
__host__ __device__
void setSupply(uint64_t* bits,
  const bool* zDir, int xDim, int yDim, int zDim,
  frMIdx x, frMIdx y, frMIdx z, frDirEnum dir, unsigned supplyIn);


// E == H; N == V; currently no U / D
__host__ __device__
void setDemand(uint64_t* bits,
  const bool* zDir, int xDim, int yDim, int zDim,
  frMIdx x, frMIdx y, frMIdx z, frDirEnum dir, unsigned demandIn);


// E == H; N == V; currently no U / D
__host__ __device__
void setRawDemand(uint64_t* bits,
  const bool* zDir, int xDim, int yDim, int zDim,
  frMIdx x, frMIdx y, frMIdx z, frDirEnum dir, unsigned rawDemandIn);



// E == H; N == V; currently no U / D
__host__ __device__
void addDemand(uint64_t* bits,
  const bool* zDir, int xDim, int yDim, int zDim,
  frMIdx x, frMIdx y, frMIdx z, frDirEnum dir, unsigned delta = 1);



// E == H; N == V; currently no U / D
__host__ __device__
void addRawDemand(uint64_t* bits,
  const bool* zDir, int xDim, int yDim, int zDim,
  frMIdx x, frMIdx y, frMIdx z, frDirEnum dir, unsigned delta = 1);


// E == H; N == V; currently no U / D
__host__ __device__
void subDemand(uint64_t* bits,
  const bool* zDir, int xDim, int yDim, int zDim,
  frMIdx x, frMIdx y, frMIdx z, frDirEnum dir, unsigned delta = 1);


// E == H; N == V; currently no U / D
__host__ __device__
void subRawDemand(uint64_t* bits,
  const bool* zDir, int xDim, int yDim, int zDim,
  frMIdx x, frMIdx y, frMIdx z, frDirEnum dir, unsigned delta = 1);


// E == H; N == V; currently no U / D
__host__ __device__
unsigned getSupply(const uint64_t* bits,
  const bool* zDir, int xDim, int yDim, int zDim,
  frMIdx x, frMIdx y, frMIdx z, frDirEnum dir, bool isRaw = false);


__host__ __device__
unsigned getRawSupply(const uint64_t* bits,
  const bool* zDir, int xDim, int yDim, int zDim,
  frMIdx x, frMIdx y, frMIdx z, frDirEnum dir);



// E == H; N == V; currently no U / D
__host__ __device__
unsigned getDemand(const uint64_t* bits,
  const bool* zDir, int xDim, int yDim, int zDim,
  frMIdx x, frMIdx y, frMIdx z, frDirEnum dir);


// E == H; N == V; currently no U / D
__host__ __device__
unsigned getRawDemand(const uint64_t* bits,
  const bool* zDir, int xDim, int yDim, int zDim,
  frMIdx x, frMIdx y, frMIdx z, frDirEnum dir);


__host__ __device__
bool hasEdge(const uint64_t* bits, 
  const bool* zDir, int xDim, int yDim, int zDim, 
  frMIdx x, frMIdx y, frMIdx z, frDirEnum dir);


__host__ __device__
bool hasBlock(const uint64_t* bits, 
  const bool* zDir, int xDim, int yDim, int zDim, 
  frMIdx x, frMIdx y, frMIdx z, frDirEnum dir);


__host__ __device__
bool hasHistoryCost(const uint64_t* bits, 
  const bool* zDir, int xDim, int yDim, int zDim, 
  frMIdx x, frMIdx y, frMIdx z);


__host__ __device__
bool hasCongCost(const uint64_t* bits, 
  const bool* zDir, int xDim, int yDim, int zDim, 
  frMIdx x, frMIdx y, frMIdx z, frDirEnum dir);


__host__ __device__
unsigned getHistoryCost(const uint64_t* bits, 
  const bool* zDir, int xDim, int yDim, int zDim, 
  frMIdx x, frMIdx y, frMIdx z);

__host__ __device__
frCoord getEdgeLength(
  const frCoord* xCoords, const frCoord* yCoords, const frCoord* zHeights,
  const bool* zDirs, int xDim , int yDim, int zDim,
  frMIdx x, frMIdx y, frMIdx z, frDirEnum dir);


__host__ __device__
void correct(frMIdx& x, frMIdx& y, frMIdx& z, frDirEnum& dir);

__host__ __device__
void correctU(frMIdx& x, frMIdx& y, frMIdx& z, frDirEnum& dir);

__host__ __device__
void reverse(frMIdx& x, frMIdx& y, frMIdx& z, frDirEnum& dir);


// inline index function
__host__ __device__
inline double getCongCost(int demand, int supply)
{
  return (demand * (4 / (1.0 + exp(supply - demand))) / (supply + 1));
}

// inline index function
__host__ __device__
inline frMIdx getIdx(frMIdx xIdx, frMIdx yIdx, frMIdx zIdx,
              int xDim, int yDim, const bool* zDir)
{
  return (zDir[zIdx] == true) ? xIdx + yIdx * xDim + zIdx * xDim * yDim
                              : yIdx + xIdx * yDim + zIdx * xDim * yDim;
} 

__host__ __device__
inline frMIdx getIdx(frMIdx xIdx, frMIdx yIdx, int xDim, int yDim)
{ 
  return xIdx + yIdx * xDim;
}


__host__ __device__
inline bool isValid(frMIdx x, frMIdx y, frMIdx z, 
             int xDim, int yDim, int zDim) 
{
  if (x < 0 || y < 0 || z < 0 || x >= xDim || y >= yDim || z >= zDim) {
    return false;
  }
  
  return true;
}


// inline bit functions
__host__ __device__
inline bool getBit(const uint64_t* bits, frMIdx idx, frMIdx pos) {
  return (bits[idx] >> pos) & 1;
}


__host__ __device__
inline void setBit(uint64_t* bits, frMIdx idx, frMIdx pos) {
  bits[idx] |= 1 << pos; 
}

__host__ __device__
inline void resetBit(uint64_t* bits, frMIdx idx, frMIdx pos) {
  bits[idx] &= ~(1 << pos); 
}
__host__ __device__
inline unsigned getBits(const uint64_t* bits, frMIdx idx, frMIdx pos, unsigned length) {
  auto tmp = bits[idx] & (((1ull << length) - 1) << pos);  // mask
  return tmp >> pos;
}

__host__ __device__
inline void setBits(uint64_t* bits, frMIdx idx, frMIdx pos, frUInt4 length, frUInt4 val) {
  bits[idx] &= ~(((1ull << length) - 1) << pos);  // clear related bits to 0
  bits[idx] |= ((uint64_t) val & ((1ull << length) - 1))
                << pos;  // only get last length bits of val
}

__host__ __device__
inline void addToBits(uint64_t* bits, frMIdx idx, frMIdx pos, frUInt4 length, frUInt4 val) {
  auto tmp = getBits(bits, idx, pos, length) + val;
  tmp = (tmp > (1u << length)) ? (1u << length) : tmp;
  setBits(bits, idx, pos, length, tmp);
}

__host__ __device__
inline void subToBits(uint64_t* bits, frMIdx idx, frMIdx pos, frUInt4 length, frUInt4 val)
{
  int tmp = (int) getBits(bits, idx, pos, length) - (int) val;
  tmp = (tmp < 0) ? 0 : tmp;
  setBits(bits, idx, pos, length, tmp);
}



} // namespace drt