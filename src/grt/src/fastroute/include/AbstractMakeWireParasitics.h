// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#pragma once

#include <cmath>

#include "grt/GRoute.h"

namespace odb {
class dbNet;
}
namespace grt {
class AbstractMakeWireParasitics
{
 public:
  ~AbstractMakeWireParasitics() = default;

  virtual void estimateParasitcs(odb::dbNet* net, GRoute& route) = 0;

  virtual void clearParasitics() = 0;

  virtual float getNetSlack(odb::dbNet* net) = 0;
};

}  // namespace grt
