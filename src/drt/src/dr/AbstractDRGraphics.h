//////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2025, Precision Innovations Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include <memory>
#include <vector>

#include "frBaseTypes.h"

namespace odb {
class dbDatabase;
}

namespace utl {
class Logger;
}

namespace drt {

class FlexGridGraph;
class FlexWavefrontGrid;
class FlexDRWorker;
class drNet;
struct RouterConfiguration;

class AbstractDRGraphics
{
 public:
  virtual ~AbstractDRGraphics() = default;

  virtual void startWorker(FlexDRWorker* worker) = 0;

  virtual void startIter(int iter, RouterConfiguration* router_cfg) = 0;

  virtual void endWorker(int iter) = 0;

  virtual void startNet(drNet* net) = 0;

  virtual void midNet(drNet* net) = 0;

  virtual void endNet(drNet* net) = 0;

  virtual void searchNode(const FlexGridGraph* grid_graph,
                          const FlexWavefrontGrid& grid)
      = 0;

  virtual void init() = 0;

  virtual void show(bool checkStopConditions) = 0;

  virtual void debugWholeDesign() = 0;
};

}  // namespace drt
