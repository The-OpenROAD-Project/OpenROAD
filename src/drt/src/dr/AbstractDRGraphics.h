// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

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
