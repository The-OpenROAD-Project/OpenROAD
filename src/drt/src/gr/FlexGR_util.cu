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

#include "FlexGR_util.h"
#include <iostream>
#include <sys/resource.h>

namespace drt {

constexpr int GRGRIDGRAPHHISTCOSTSIZE = 8;
constexpr int GRSUPPLYSIZE = 8;
constexpr int GRDEMANDSIZE = 16;
constexpr int GRFRACSIZE = 1;


__host__ __device__
void correct(frMIdx& x, frMIdx& y, frMIdx& z, frDirEnum& dir) 
{
  switch (dir) {
    case frDirEnum::W:
      x--;
      dir = frDirEnum::E;
      break;
    case frDirEnum::S:
      y--;
      dir = frDirEnum::N;
      break;
    case frDirEnum::D:
      z--;
      dir = frDirEnum::U;
      break;
    default:;
  }
}


__host__ __device__
void correctU(frMIdx& x, frMIdx& y, frMIdx& z, frDirEnum& dir) 
{
  switch (dir) {
    case frDirEnum::D:
      z--;
      dir = frDirEnum::U;
      break;
    default:;
  }
}


__host__ __device__
void reverse(frMIdx& x, frMIdx& y, frMIdx& z, frDirEnum& dir)
{
  switch (dir) {
    case frDirEnum::E:
      x++;
      dir = frDirEnum::W;
      break;
    case frDirEnum::S:
      y--;
      dir = frDirEnum::N;
      break;
    case frDirEnum::W:
      x--;
      dir = frDirEnum::E;
      break;
    case frDirEnum::N:
      y++;
      dir = frDirEnum::S;
      break;
    case frDirEnum::U:
      z++;
      dir = frDirEnum::D;
      break;
    case frDirEnum::D:
      z--;
      dir = frDirEnum::U;
      break;
    default:;
  }
}



__host__ __device__
bool addEdge(uint64_t* bits,
  const bool* zDir, int xDim, int yDim, int zDim, 
  frMIdx x, frMIdx y, frMIdx z, frDirEnum dir) 
{
  correct(x, y, z, dir);
  if (!isValid(x, y, z, xDim, yDim, zDim)) {
    return false;
  }
  
  auto idx = getIdx(x, y, z, xDim, yDim, zDir);
  switch (dir) {
    case frDirEnum::E:
      setBit(bits, idx, 0);
      break;
    case frDirEnum::N:
      setBit(bits, idx, 1);
      break;
    case frDirEnum::U:
      setBit(bits, idx, 2);
      break;
    default:
      return false;
  }  
  
  return true; 
}


__host__ __device__
bool removeEdge(uint64_t* bits,
  const bool* zDir, int xDim, int yDim, int zDim, 
  frMIdx x, frMIdx y, frMIdx z, frDirEnum dir) 
{
  correct(x, y, z, dir);
  if (!isValid(x, y, z, xDim, yDim, zDim)) {
    return false;
  }
  
  auto idx = getIdx(x, y, z, xDim, yDim, zDir);
  switch (dir) {
    case frDirEnum::E:
      resetBit(bits, idx, 0);
      break;
    case frDirEnum::N:
      resetBit(bits, idx, 1);
      break;
    case frDirEnum::U:
      resetBit(bits, idx, 2);
      break;
    default:
      return false;
  }  
  
  return true; 
}


__host__ __device__
bool setBlock(uint64_t* bits,
  const bool* zDir, int xDim, int yDim, int zDim, 
  frMIdx x, frMIdx y, frMIdx z, frDirEnum dir) 
{
  correct(x, y, z, dir);
  if (!isValid(x, y, z, xDim, yDim, zDim)) {
    return false;
  }
  
  auto idx = getIdx(x, y, z, xDim, yDim, zDir);
  switch (dir) {
    case frDirEnum::E:
      setBit(bits, idx, 3);
      break;
    case frDirEnum::N:
      setBit(bits, idx, 4);
      break;
    case frDirEnum::U:
      setBit(bits, idx, 5);
      break;
    default:
      return false;
  }  
  
  return true; 
}



__host__ __device__
bool resetBlock(uint64_t* bits,
  const bool* zDir, int xDim, int yDim, int zDim, 
  frMIdx x, frMIdx y, frMIdx z, frDirEnum dir) 
{
  correct(x, y, z, dir);
  if (!isValid(x, y, z, xDim, yDim, zDim)) {
    return false;
  }
  
  auto idx = getIdx(x, y, z, xDim, yDim, zDir);
  switch (dir) {
    case frDirEnum::E:
      resetBit(bits, idx, 3);
      break;
    case frDirEnum::N:
      resetBit(bits, idx, 4);
      break;
    case frDirEnum::U:
      resetBit(bits, idx, 5);
      break;
    default:
      return false;
  }  
  
  return true; 
}


__host__ __device__
void setHistoryCost(uint64_t* bits, 
  const bool* zDir, int xDim, int yDim, int zDim, 
  frMIdx x, frMIdx y, frMIdx z, unsigned histCostIn)
{
  if (!isValid(x, y, z, xDim, yDim, zDim)) {
    return;
  }

  auto idx = getIdx(x, y, z, xDim, yDim, zDir);
  setBits(bits, idx, 8, GRGRIDGRAPHHISTCOSTSIZE, histCostIn);
}


__host__ __device__
void addHistoryCost(uint64_t* bits, 
  const bool* zDir, int xDim, int yDim, int zDim, 
  frMIdx x, frMIdx y, frMIdx z, unsigned in)
{
  if (!isValid(x, y, z, xDim, yDim, zDim)) {
    return;
  }

  auto idx = getIdx(x, y, z, xDim, yDim, zDir);  
  addToBits(bits, idx, 8, GRGRIDGRAPHHISTCOSTSIZE, in);
}


__host__ __device__
void decayHistoryCost(uint64_t* bits, 
  const bool* zDir, int xDim, int yDim, int zDim, 
  frMIdx x, frMIdx y, frMIdx z)
{
  if (!isValid(x, y, z, xDim, yDim, zDim)) {
    return;
  }

  auto idx = getIdx(x, y, z, xDim, yDim, zDir);  
  subToBits(bits, idx, 8, GRGRIDGRAPHHISTCOSTSIZE, 1);
}


__host__ __device__
void decayHistoryCost(uint64_t* bits, 
  const bool* zDir, int xDim, int yDim, int zDim, 
  frMIdx x, frMIdx y, frMIdx z, double d)
{
  if (!isValid(x, y, z, xDim, yDim, zDim)) {
    return;
  }

  auto idx = getIdx(x, y, z, xDim, yDim, zDir);  
  int currCost = (getBits(bits, idx, 8, GRGRIDGRAPHHISTCOSTSIZE));
  currCost *= d;
  currCost = std::max(0, currCost);
  setBits(bits, idx, 8, GRGRIDGRAPHHISTCOSTSIZE, currCost);
}



// E == H; N == V; currently no U / D
__host__ __device__
void setSupply(uint64_t* bits,
  const bool* zDir, int xDim, int yDim, int zDim,
  frMIdx x, frMIdx y, frMIdx z, frDirEnum dir, unsigned supplyIn)
{
  correct(x, y, z, dir);
  if (!isValid(x, y, z, xDim, yDim, zDim)) {
    return;
  }

  auto idx = getIdx(x, y, z, xDim, yDim, zDir);
  switch (dir) {
    case frDirEnum::E:
      setBits(bits, idx, 16, GRSUPPLYSIZE, supplyIn);
      break;
    case frDirEnum::N:
      setBits(bits, idx, 24, GRSUPPLYSIZE, supplyIn);
      break;
    default:
      return;
  }
}


// E == H; N == V; currently no U / D
__host__ __device__
void setDemand(uint64_t* bits,
  const bool* zDir, int xDim, int yDim, int zDim,
  frMIdx x, frMIdx y, frMIdx z, frDirEnum dir, unsigned demandIn)
{
  correct(x, y, z, dir);
  if (!isValid(x, y, z, xDim, yDim, zDim)) {
    return;
  }

  auto idx = getIdx(x, y, z, xDim, yDim, zDir);
  switch (dir) {
    case frDirEnum::E:
      return setBits(
        bits, idx, 48 + GRFRACSIZE, GRDEMANDSIZE - GRFRACSIZE, demandIn);
    case frDirEnum::N:
      return setBits(
        bits, idx, 32 + GRFRACSIZE, GRDEMANDSIZE - GRFRACSIZE, demandIn);
    default:
      return;
  }
}


// E == H; N == V; currently no U / D
__host__ __device__
void setRawDemand(uint64_t* bits,
  const bool* zDir, int xDim, int yDim, int zDim,
  frMIdx x, frMIdx y, frMIdx z, frDirEnum dir, unsigned rawDemandIn)
{
  correct(x, y, z, dir);
  if (!isValid(x, y, z, xDim, yDim, zDim)) {
    return;
  }

  auto idx = getIdx(x, y, z, xDim, yDim, zDir);
  switch (dir) {
    case frDirEnum::E:
      return setBits(bits, idx, 48, GRDEMANDSIZE, rawDemandIn);
    case frDirEnum::N:
      return setBits(bits, idx, 32, GRDEMANDSIZE, rawDemandIn);
    default:
      return;
  }
}


// E == H; N == V; currently no U / D
__host__ __device__
void addDemand(uint64_t* bits,
  const bool* zDir, int xDim, int yDim, int zDim,
  frMIdx x, frMIdx y, frMIdx z, frDirEnum dir, unsigned delta)
{
  correct(x, y, z, dir);
  if (!isValid(x, y, z, xDim, yDim, zDim)) {
    return;
  }

  auto idx = getIdx(x, y, z, xDim, yDim, zDir);
  switch (dir) {
    case frDirEnum::E:
      addToBits(bits, idx, 48 + GRFRACSIZE, GRDEMANDSIZE - GRFRACSIZE, delta);
      break;
    case frDirEnum::N:
      addToBits(bits, idx, 32 + GRFRACSIZE, GRDEMANDSIZE - GRFRACSIZE, delta);
      break;
    default:
      return;
  }
}



// E == H; N == V; currently no U / D
__host__ __device__ 
void addRawDemand(uint64_t* bits,
  const bool* zDir, int xDim, int yDim, int zDim,
  frMIdx x, frMIdx y, frMIdx z, frDirEnum dir, unsigned delta)
{
  correct(x, y, z, dir);
  if (!isValid(x, y, z, xDim, yDim, zDim)) {
    return;
  }

  auto idx = getIdx(x, y, z, xDim, yDim, zDir);
  switch (dir) {
    case frDirEnum::E:
      addToBits(bits, idx, 48, GRDEMANDSIZE, delta);
      break;
    case frDirEnum::N:
      addToBits(bits, idx, 32, GRDEMANDSIZE, delta);
      break;
    default:
      return;
  }
}


// E == H; N == V; currently no U / D
__host__ __device__
void subDemand(uint64_t* bits,
  const bool* zDir, int xDim, int yDim, int zDim,
  frMIdx x, frMIdx y, frMIdx z, frDirEnum dir, unsigned delta)
{
  correct(x, y, z, dir);
  if (!isValid(x, y, z, xDim, yDim, zDim)) {
    return;
  }

  auto idx = getIdx(x, y, z, xDim, yDim, zDir);
  switch (dir) {
    case frDirEnum::E:
      subToBits(bits, idx, 48 + GRFRACSIZE, GRDEMANDSIZE - GRFRACSIZE, delta);
      break;
    case frDirEnum::N:
      subToBits(bits, idx, 32 + GRFRACSIZE, GRDEMANDSIZE - GRFRACSIZE, delta);
      break;
    default:
      return;
  }
}

// E == H; N == V; currently no U / D
__host__ __device__
void subRawDemand(uint64_t* bits,
  const bool* zDir, int xDim, int yDim, int zDim,
  frMIdx x, frMIdx y, frMIdx z, frDirEnum dir, unsigned delta)
{
  correct(x, y, z, dir);
  if (!isValid(x, y, z, xDim, yDim, zDim)) {
    return;
  }

  auto idx = getIdx(x, y, z, xDim, yDim, zDir);
  switch (dir) {
    case frDirEnum::E:
      subToBits(bits, idx, 48, GRDEMANDSIZE, delta);
      break;
    case frDirEnum::N:
      subToBits(bits, idx, 32, GRDEMANDSIZE, delta);
      break;
    default:
      return;
  }
} 


// E == H; N == V; currently no U / D
__host__ __device__
unsigned getSupply(const uint64_t* bits,
  const bool* zDir, int xDim, int yDim, int zDim,
  frMIdx x, frMIdx y, frMIdx z, frDirEnum dir, bool isRaw)
{
  correct(x, y, z, dir);
  if (!isValid(x, y, z, xDim, yDim, zDim)) {
    return 0;
  }
    
  unsigned supply = 0;
  auto idx = getIdx(x, y, z, xDim, yDim, zDir);
  switch (dir) {
    case frDirEnum::E:
      supply = getBits(bits, idx, 24, GRSUPPLYSIZE);
      break;
    case frDirEnum::N:
      supply = getBits(bits, idx, 16, GRSUPPLYSIZE);
      break;
    default:
      return 0;
  }
  
  // Return the raw or processed supply value based on the isRaw flag
  return isRaw ? (supply << GRFRACSIZE) : supply;
}


__host__ __device__
unsigned getRawSupply(const uint64_t* bits,
  const bool* zDir, int xDim, int yDim, int zDim,
  frMIdx x, frMIdx y, frMIdx z, frDirEnum dir)
{
  return getSupply(bits, zDir, xDim, yDim, zDim, x, y, z, dir, true);
}


// E == H; N == V; currently no U / D
__host__ __device__
unsigned getDemand(const uint64_t* bits,
  const bool* zDir, int xDim, int yDim, int zDim,
  frMIdx x, frMIdx y, frMIdx z, frDirEnum dir)
{
  correct(x, y, z, dir);
  if (!isValid(x, y, z, xDim, yDim, zDim)) {
    return 0;
  }
    
  auto idx = getIdx(x, y, z, xDim, yDim, zDir);
  switch (dir) {
    case frDirEnum::E:
      return getBits(bits, idx, 48 + GRFRACSIZE, GRDEMANDSIZE - GRFRACSIZE);
    case frDirEnum::N:
      return getBits(bits, idx, 32 + GRFRACSIZE, GRDEMANDSIZE - GRFRACSIZE);
    default:
      return 0;
  }
}


// E == H; N == V; currently no U / D
__host__ __device__
unsigned getRawDemand(const uint64_t* bits,
  const bool* zDir, int xDim, int yDim, int zDim,
  frMIdx x, frMIdx y, frMIdx z, frDirEnum dir)
{
  correct(x, y, z, dir);
  if (!isValid(x, y, z, xDim, yDim, zDim)) {
    return 0;
  }
    
  auto idx = getIdx(x, y, z, xDim, yDim, zDir);
  switch (dir) {
    case frDirEnum::E:
      return getBits(bits, idx, 48, GRDEMANDSIZE);
    case frDirEnum::N:
      return getBits(bits, idx, 32, GRDEMANDSIZE);
    default:
      return 0;
  }
}


__host__ __device__
bool hasBlock(const uint64_t* bits, 
  const bool* zDir, int xDim, int yDim, int zDim, 
  frMIdx x, frMIdx y, frMIdx z, frDirEnum dir)
{
  correct(x, y, z, dir);
  if (!isValid(x, y, z, xDim, yDim, zDim)) {
    return false;
  }
  
  auto idx = getIdx(x, y, z, xDim, yDim, zDir);
  switch (dir) {
    case frDirEnum::E:
      return getBit(bits, idx, 3);
    case frDirEnum::N:
      return getBit(bits, idx, 4);
    case frDirEnum::U:
      return getBit(bits, idx, 5);
    default:
      return false;
  }
}


__host__ __device__
bool hasHistoryCost(const uint64_t* bits, 
  const bool* zDir, int xDim, int yDim, int zDim, 
  frMIdx x, frMIdx y, frMIdx z)
{
  if (!isValid(x, y, z, xDim, yDim, zDim)) {
    return false;
  }

  auto idx = getIdx(x, y, z, xDim, yDim, zDir);
  return getBits(bits, idx, 8, GRGRIDGRAPHHISTCOSTSIZE);
}


__host__ __device__
bool hasEdge(const uint64_t* bits,
  const bool* zDir, int xDim, int yDim, int zDim,
  frMIdx x, frMIdx y, frMIdx z, frDirEnum dir)
{
  correct(x, y, z, dir);
  if (!isValid(x, y, z, xDim, yDim, zDim)) {
    return false;
  }
  
  auto idx = getIdx(x, y, z, xDim, yDim, zDir);
  switch (dir) {
    case frDirEnum::E:
      return getBit(bits, idx, 0);
    case frDirEnum::N:
      return getBit(bits, idx, 1);
    case frDirEnum::U:
      return getBit(bits, idx, 2);
    default:
      return false;
  }  
}


__host__ __device__
frCoord getEdgeLength(
  const frCoord* xCoords, const frCoord* yCoords, const frCoord* zHeights,
  const bool* zDirs, int xDim , int yDim, int zDim,
  frMIdx x, frMIdx y, frMIdx z, frDirEnum dir)
{
  correct(x, y, z, dir);
  if (!isValid(x, y, z, xDim, yDim, zDim)) {
    return false;
  }

  switch (dir) {
    case frDirEnum::E:
      return xCoords[x + 1] - xCoords[x];
    case frDirEnum::N:
      return yCoords[y + 1] - yCoords[y];
    case frDirEnum::U:
      return zHeights[z + 1] - zHeights[z];
    default:
      return 0;
  }
}




__host__ __device__
bool hasCongCost(const uint64_t* bits, 
  const bool* zDir, int xDim, int yDim, int zDim, 
  frMIdx x, frMIdx y, frMIdx z, frDirEnum dir)
{
  return (getRawDemand(bits, zDir, xDim, yDim, zDim, x, y, z, dir) > 
          getRawSupply(bits, zDir, xDim, yDim, zDim, x, y, z, dir));
}


__host__ __device__
unsigned getHistoryCost(const uint64_t* bits, 
  const bool* zDir, int xDim, int yDim, int zDim, 
  frMIdx x, frMIdx y, frMIdx z)
{
  if (!isValid(x, y, z, xDim, yDim, zDim)) {
    return 0;
  }

  auto idx = getIdx(x, y, z, xDim, yDim, zDir);
  return getBits(bits, idx, 8, GRGRIDGRAPHHISTCOSTSIZE);
}


} // namespace drt

