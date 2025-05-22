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
 #include "FlexGR_util.h"
 #include "stt/SteinerTreeBuilder.h"
 #include "gr/FlexGR_util_update.h"
 #include "gr/FlexGRCMap.h"
 
 
 
namespace drt {
 
 
__device__
int getIdx_device(int x, int y, int z, int x_dim, int y_dim, int z_dim)
{
  return x + y * x_dim + z * x_dim * y_dim;
}
 
__device__
bool getBit_device(uint64_t* cmap, unsigned idx, unsigned pos)
{
  return (cmap[idx] >> pos) & 1;
}
 
__device__
unsigned getBits_device(uint64_t* cmap, unsigned idx, unsigned pos, unsigned length)
{
  auto tmp = cmap[idx] & (((1ull << length) - 1) << pos);
  return tmp >> pos;
}
 
 
__device__
void setBits_device(uint64_t* cmap, unsigned idx, unsigned pos, unsigned length, unsigned val)
{
  cmap[idx] &= ~(((1ull << length) - 1) << pos);  // clear related bits to 0
  cmap[idx] |= ((uint64_t) val & ((1ull << length) - 1))
                 << pos;  // only get last length bits of val
}
 
 
__device__
void addToBits_device(uint64_t* cmap, unsigned idx, unsigned pos, unsigned length, unsigned val)
{
  auto tmp = getBits_device(cmap, idx, pos, length) + val;
  tmp = (tmp > (1u << length)) ? (1u << length) : tmp;
  setBits_device(cmap, idx, pos, length, tmp);
}
 
 
__device__
unsigned getRawDemand_device(uint64_t* cmap,
  int xDim, int yDim, int zDim,
  unsigned x, unsigned y, unsigned z, frDirEnum dir)
{
  unsigned demand = 0;
  auto idx = getIdx_device(x, y, z, xDim, yDim, zDim);
  switch (dir) {
    case frDirEnum::E:
      demand = getBits_device(cmap, idx, 48, CMAPDEMANDSIZE);
      break;
    case frDirEnum::N:
      demand = getBits_device(cmap, idx, 32, CMAPDEMANDSIZE);
      break;
    case frDirEnum::U:;
      break;
    default:;
  }
 
  return demand;
}
 
 
__device__
unsigned getRawSupply_device(uint64_t* cmap, 
  int xDim, int yDim, int zDim,
  unsigned x, unsigned y, unsigned z, frDirEnum dir)
{
  unsigned supply = 0;
  auto idx = getIdx_device(x, y, z, xDim, yDim, zDim);
  switch (dir) {
    case frDirEnum::E:
      supply = getBits_device(cmap, idx, 24, CMAPSUPPLYSIZE);
      break;
    case frDirEnum::N:
      supply = getBits_device(cmap, idx, 16, CMAPSUPPLYSIZE);
      break;
    case frDirEnum::U:;
      break;
    default:;
  }
 
  return supply << CMAPFRACSIZE;
}
 
 
__device__
bool hasBlock_device(uint64_t* cmap, 
  int xDim, int yDim, int zDim,
  unsigned x, unsigned y, unsigned z, frDirEnum dir)
{
  bool sol = false;
  auto idx = getIdx_device(x, y, z, xDim, yDim, zDim);
  switch (dir) {
    case frDirEnum::E:
      sol = getBit_device(cmap, idx, 3);
      break;
    case frDirEnum::N:
      sol = getBit_device(cmap, idx, 2);
      break;
    case frDirEnum::U:;
      break;
    default:;
  }
   
  return sol;
}
 
/*
__device__
void addRawDemand_device(
  uint64_t* cmap,
  int xDim, int yDim, int zDim,
  unsigned x, unsigned y, unsigned z, frDirEnum dir, unsigned delta)
{
  int idx = getIdx_device(x, y, z, xDim, yDim, zDim);
  switch (dir) {
    case frDirEnum::E:
      addToBits_device(cmap, idx, 48, CMAPDEMANDSIZE, delta);
      break;
    case frDirEnum::N:
      addToBits_device(cmap, idx, 32, CMAPDEMANDSIZE, delta);
      break;
    case frDirEnum::U:
      break;
    default:;
  }
}
*/


__device__
void atomicAddBits(uint64_t* bits, size_t idx, int bitOffset, int bitSize, unsigned delta) {
    uint64_t mask = ((1ULL << bitSize) - 1) << bitOffset; // Create a mask for the target bits

    unsigned long long oldVal, newVal; // Use atomicCAS for atomic updates
    do {
        oldVal = atomicCAS((unsigned long long*)&bits[idx], bits[idx], bits[idx]); // Read current value
        uint64_t currentBits = (oldVal & mask) >> bitOffset; // Extract relevant bits
        uint64_t updatedBits = min(
            static_cast<uint64_t>(currentBits + delta),
            static_cast<uint64_t>((1ULL << bitSize) - 1)
        ); // Add delta and saturate
        newVal = (oldVal & ~mask) | (updatedBits << bitOffset); // Update the relevant bits
    } while (atomicCAS((unsigned long long*)&bits[idx], oldVal, newVal) != oldVal); // Repeat until successful
}



__device__
void addRawDemand_device(
  uint64_t* cmap,
  int xDim, int yDim, int zDim,
  unsigned x, unsigned y, unsigned z, frDirEnum dir, unsigned delta)
{
  int idx = getIdx_device(x, y, z, xDim, yDim, zDim);
  int shiftAmount = 0;

  switch (dir) {
    case frDirEnum::E:
      shiftAmount = 48;
      break;
    case frDirEnum::N:
      shiftAmount = 32;
      break;
    default:
      return;
  }

  // Use atomic operations to safely update the relevant bits
  atomicAddBits(cmap, idx, shiftAmount, GRDEMANDSIZE, delta);
}


__device__
unsigned getHistoryCost_device(
  uint64_t* cmap,
  int xDim, int yDim, int zDim,
  unsigned x, unsigned y, unsigned z)
{
  int idx = getIdx_device(x, y, z, xDim, yDim, zDim);
  return getBits_device(cmap, idx, 8, GRGRIDGRAPHHISTCOSTSIZE);
}



__device__
float getCongCost_device(unsigned supply, unsigned demand)
{
  //return demand * (1.0 + 8.0 / (1.0 + exp(supply - demand))) / (supply + 1);
  // Modify the cost function to avoid overflow
  // Clamping to avoid overflow in exp()
  float exp_val = exp(std::min(10.0f, static_cast<float>(supply) - demand));  
  // Calculate the factor safely
  float factor = 8.0f / (1.0f + exp_val); 
  // Compute congestion cost with safety limits
  float congCost = demand * (1.0f + factor) / (supply + 1.0f);
  return congCost;  
}


} // namespace drt 

