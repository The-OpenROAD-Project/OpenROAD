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


__device__
int getIdx__device(int x, int y, int z, int xDim, int yDim, int zDim)
{
  if (z < 0 || z >= zDim) {
    return -1;
  }
  return z * xDim * yDim + y * xDim + x;    
}


__device__
bool getBit__device(uint64_t* d_cmap, unsigned idx, unsigned pos)
{
  return (d_cmap[idx] >> pos) & 1;
}


__device__
unsigned getBits__device(uint64_t* d_cmap, unsigned idx, unsigned pos, unsigned length)
{
  auto tmp = d_cmap[idx] & (((1ull << length) - 1) << pos);
  return tmp >> pos;
}



// Non-atomic version (for reference only)
__device__
void setBits_naive__device(uint64_t* d_cmap, unsigned idx, unsigned pos, unsigned length, unsigned val)
{
  d_cmap[idx] &= ~(((1ull << length) - 1) << pos);  // clear related bits to 0
  d_cmap[idx] |= ((uint64_t) val & ((1ull << length) - 1))
              << pos;  // only get last length bits of val
}



__device__
void setBits__device(uint64_t* d_cmap, unsigned idx, unsigned pos, unsigned length, unsigned val)
{
  uint64_t* address = &d_cmap[idx];
  const uint64_t clear_mask = ((1ull << length) - 1) << pos; // clear related bits to 0
  const uint64_t set_val = (static_cast<uint64_t>(val) & ((1ull << length) - 1)) << pos; // only get last length bits of val
  uint64_t old = *address;
  uint64_t new_val;
  do {
    old = *address;
    new_val = (old & ~clear_mask) | set_val;
  } while (atomicCAS(reinterpret_cast<unsigned long long int*>(address),
           static_cast<unsigned long long int>(old),
           static_cast<unsigned long long int>(new_val)) != old);
}



// Non-atomic version (for reference only)
__device__
void addToBits_naive__device(uint64_t* d_cmap, unsigned idx, unsigned pos, unsigned length, unsigned val)
{
  auto tmp = getBits__device(d_cmap, idx, pos, length) + val;
  tmp = (tmp > (1u << length)) ? (1u << length) : tmp;
  setBits_naive__device(d_cmap, idx, pos, length, tmp);
}


__device__
void addToBits__device(uint64_t* d_cmap, unsigned idx, unsigned pos, unsigned length, unsigned val)
{
  uint64_t* address = &d_cmap[idx];
  const uint64_t clear_mask = ((1ull << length) - 1) << pos;
  const uint64_t val_mask = (1ull << length) - 1;
  uint64_t old, new_val;
  old = *address;
  do {
    old = *address;
    uint64_t current_val = (old >> pos) & val_mask;
    current_val += val;
    // Clamp to original behavior 
    current_val = min(current_val, static_cast<uint64_t>(1ull << length));
    new_val = (old & ~clear_mask) | ((current_val & val_mask) << pos);
  } while (atomicCAS(reinterpret_cast<unsigned long long int*>(address),
           static_cast<unsigned long long int>(old),
           static_cast<unsigned long long int>(new_val)) != old);
}



__device__
bool hasBlock__device(uint64_t* d_cmap, 
  int xDim, int yDim, int zDim,
  unsigned x, unsigned y, unsigned z, frDirEnum dir)
{
  bool sol = false;
  auto idx = getIdx__device(x, y, z, xDim, yDim, zDim);
  switch (dir) {
    case frDirEnum::E:
      sol = getBit__device(d_cmap, idx, 3);
      break;
    case frDirEnum::N:
      sol = getBit__device(d_cmap, idx, 2);
      break;
    case frDirEnum::U:;
      break;
    default:;
  }  
  return sol;
}



__device__
unsigned getRawSupply__device(uint64_t* d_cmap, 
  int xDim, int yDim, int zDim,
  unsigned x, unsigned y, unsigned z, frDirEnum dir)
{
  unsigned supply = 0;
  auto idx = getIdx__device(x, y, z, xDim, yDim, zDim);
  switch (dir) {
    case frDirEnum::E:
      supply = getBits__device(d_cmap, idx, 24, CMAPSUPPLYSIZE);
      break;
    case frDirEnum::N:
      supply = getBits__device(d_cmap, idx, 16, CMAPSUPPLYSIZE);
      break;
    case frDirEnum::U:;
      break;
    default:;
  }
  return supply << CMAPFRACSIZE;
}


__device__
unsigned getRawDemand__device(uint64_t* d_cmap,
  int xDim, int yDim, int zDim,
  unsigned x, unsigned y, unsigned z, frDirEnum dir)
{
  unsigned demand = 0;
  auto idx = getIdx__device(x, y, z, xDim, yDim, zDim);
  switch (dir) {
    case frDirEnum::E:
      demand = getBits__device(d_cmap, idx, 48, CMAPDEMANDSIZE);
      break;
    case frDirEnum::N:
      demand = getBits__device(d_cmap, idx, 32, CMAPDEMANDSIZE);
      break;
    case frDirEnum::U:;
      break;
    default:;
  }

  return demand;
}


__device__
void addRawDemand__device(
  uint64_t* d_cmap,
  int xDim, int yDim, int zDim,
  unsigned x, unsigned y, unsigned z, frDirEnum dir, unsigned delta)
{
  int idx = getIdx__device(x, y, z, xDim, yDim, zDim);
  switch (dir) {
    case frDirEnum::E:
      addToBits__device(d_cmap, idx, 48, CMAPDEMANDSIZE, delta);
      break;
    case frDirEnum::N:
      addToBits__device(d_cmap, idx, 32, CMAPDEMANDSIZE, delta);
      break;
    case frDirEnum::U:
      break;
    default:;
  }
}


} // namespace drt

