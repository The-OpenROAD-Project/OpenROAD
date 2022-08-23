/* Author: Mahfouz-z */
/*
 * Copyright (c) 2022, The Regents of the University of California
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

#include "ord/OpenRoad.hh"

// Stubs out functions from OpenRoad that aren't needed by Testing but
// are referenced from dst modules or its dependencies.

namespace ord {

OpenRoad::OpenRoad()
    : tcl_interp_(nullptr),
      logger_(nullptr),
      db_(nullptr),
      verilog_network_(nullptr),
      sta_(nullptr),
      resizer_(nullptr),
      ioPlacer_(nullptr),
      opendp_(nullptr),
      optdp_(nullptr),
      finale_(nullptr),
      macro_placer_(nullptr),
      macro_placer2_(nullptr),
      global_router_(nullptr),
      restructure_(nullptr),
      tritonCts_(nullptr),
      tapcell_(nullptr),
      extractor_(nullptr),
      detailed_router_(nullptr),
      antenna_checker_(nullptr),
      replace_(nullptr),
      pdnsim_(nullptr),
      partitionMgr_(nullptr),
      pdngen_(nullptr),
      distributer_(nullptr),
      stt_builder_(nullptr),
      threads_(0)
{
}

OpenRoad* OpenRoad::openRoad()
{
  return nullptr;
}

}  // namespace ord
