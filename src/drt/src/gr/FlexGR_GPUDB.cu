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

#include "FlexGR_GPUDB.h"

#include <omp.h>
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>

#include "db/grObj/grShape.h"
#include "db/grObj/grVia.h"
#include "db/infra/frTime.h"
#include "db/obj/frGuide.h"
#include "odb/db.h"
#include "utl/exception.h"

namespace drt {

void FlexGRGPUDB::init(FlexGRCMap* cmap, FlexGRCMap* cmap2D)
{
  cmap->getDim(xDim, yDim, zDim);
  auto& cmap_bits = cmap->getBits();
  auto& cmap2D_bits = cmap2D->getBits();

  cmap_bits_3D_size = cmap_bits.size();
  cmap_bits_2D_size = cmap2D_bits.size();
  
  // Allocate memory on the GPU side
  cudaMalloc((void**)&cmap_bits_3D, cmap_bits_3D_size * sizeof(uint64_t));
  cudaMalloc((void**)&cmap_bits_2D, cmap_bits_2D_size * sizeof(uint64_t));
  cudaCheckError();
  
  // Copy the data from the host to the device
  cudaMemcpy(cmap_bits_3D, cmap_bits.data(), cmap_bits_3D_size * sizeof(uint64_t), cudaMemcpyHostToDevice);
  cudaMemcpy(cmap_bits_2D, cmap2D_bits.data(), cmap_bits_2D_size * sizeof(uint64_t), cudaMemcpyHostToDevice);
  cudaCheckError();


  std::string msg = std::string("[INFO] ")
                  + std::string("FlexGRGPUDB initialized with dimensions: \n")
                  + "\t xDim: " + std::to_string(xDim) + ", "
                  + "yDim: " + std::to_string(yDim) + ", "
                  + "zDim: " + std::to_string(zDim) + "\n"
                  + "\t cmap_bits_2D_size: " + std::to_string(cmap_bits_2D_size) + ", "
                  + "cmap_bits_3D_size: " + std::to_string(cmap_bits_3D_size);

  logger_->report(msg);
}

void FlexGRGPUDB::freeCUDAMem()
{
  if (cmap_bits_3D) {
    cudaFree(cmap_bits_3D);
    cmap_bits_3D = nullptr;
  }
  
  if (cmap_bits_2D) {
    cudaFree(cmap_bits_2D);
    cmap_bits_2D = nullptr;
  }
  
  cudaCheckError();
  
  logger_->report("FlexGRGPUDB CUDA memory freed ....");
}

}  // namespace drt
