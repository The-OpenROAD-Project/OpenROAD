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
#include "gr/FlexGRCMap.h"
#include "frBaseTypes.h"
#include "frDesign.h"
#include "FlexGR_util.h"

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

class FlexGRGPUDB
{
  public:
    FlexGRGPUDB() = default;
    
    FlexGRGPUDB(frDesign* design,
                utl::Logger* logger,
                FlexGRCMap* cmap,
                FlexGRCMap* cmap2D)
      : logger_(logger), cmap_(cmap), cmap2D_(cmap2D)
    {
      init(design, cmap, cmap2D);
    }
 
    // Global Settings for GPUDB (congestion map)
    // 2D Congestion Map  
    uint64_t* d_cmap_bits_2D = nullptr;
    int cmap_bits_2D_size = 0;
        
    // 3D Congestion Map
    uint64_t* d_cmap_bits_3D = nullptr;
    int cmap_bits_3D_size = 0;

    // store the layer direction
    // 0: horizontal, 1: vertical
    bool* d_layerDir = nullptr;

    int xDim = 0;
    int yDim = 0;
    int zDim = 0;
    
    void freeCUDAMem();

  private:
    utl::Logger* logger_ = nullptr;
    FlexGRCMap* cmap_ = nullptr;
    FlexGRCMap* cmap2D_ = nullptr;
  
    void init(frDesign* design, FlexGRCMap* cmap, FlexGRCMap* cmap2D);  
};


}


