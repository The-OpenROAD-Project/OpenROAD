// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#pragma once

#include "src/odb/include/odb/db.h"
#include "src/odb/include/odb/geom.h"
#include "src/rsz/src/BufferedNet.hh"

namespace rsz {

class ResizerObserver
{
 public:
  virtual ~ResizerObserver() = default;

  virtual void setNet(odb::dbNet* net) {}
  virtual void stopOnSubdivideStep(bool stop) {}

  virtual void subdivideStart(odb::dbNet* net) {}
  virtual void subdivide(const odb::Line& line) {}
  virtual void subdivideDone() {}

  virtual void repairNetStart(const BufferedNetPtr& bnet, odb::dbNet* net) {}
  virtual void makeBuffer(odb::dbInst* inst) {}
  virtual void repairNetDone() {}
};

}  // namespace rsz
