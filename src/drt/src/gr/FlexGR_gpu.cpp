/* Authors: Lutong Wang and Bangqi Xu */
/*
 * Copyright (c) 2019, The Regents of the University of California
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

namespace drt {

using utl::ThreadException;

void FlexGR::main_gpu(odb::dbDatabase* db)
{  
  logger_->report(std::string("*", 80)); 
  logger_->report("GPU mode is enabled ....");  
  auto grRuntimeStart = std::chrono::high_resolution_clock::now();
  
  db_ = db;
  init();
  
  // resource analysis
  ra();
  // cmap->print(true);

  FlexGRCMap baseCMap(cmap_.get(), router_cfg_);
  FlexGRCMap baseCMap2D(cmap2D_.get(), router_cfg_);

  // Allow the GPU Memory to be used
  // Do not frquently allocate and deallocate the GPU memory

  // Reserve the nets for the batch generation
  // Only once
  nets2Ripup_.clear();
  nets2Ripup_.reserve(design_->getTopBlock()->getNets().size());

  auto grRuntimeEnd = std::chrono::high_resolution_clock::now();
  auto grRuntime = std::chrono::duration_cast<std::chrono::milliseconds>(grRuntimeEnd - grRuntimeStart);
  logger_->report("[INFO] Runtime for Global Routing : %d ms", grRuntime.count());

  exit(1);
}


}  // namespace drt
