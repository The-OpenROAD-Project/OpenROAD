// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#pragma once

#include <memory>
#include <vector>

#include "FlexTA.h"
#include "db/obj/frBlockObject.h"
#include "frBaseTypes.h"

namespace drt {
class AbstractTAGraphics
{
 public:
  virtual ~AbstractTAGraphics() = default;

  // Update status and optionally pause
  virtual void endIter(int iter) = 0;
};

}  // namespace drt
