// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#pragma once

#include "odb/db.h"
#include "odb/geom.h"

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
};

}  // namespace rsz
