// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#pragma once

#include <memory>
#include <vector>

#include "src/drt/src/db/obj/frBlockObject.h"
#include "src/drt/src/dr/AbstractDRGraphics.h"
#include "src/drt/src/frBaseTypes.h"
#include "src/drt/src/frDesign.h"
#include "src/drt/src/pa/AbstractPAGraphics.h"
#include "src/drt/src/ta/AbstractTAGraphics.h"

namespace drt {
class AbstractGraphicsFactory
{
 public:
  virtual ~AbstractGraphicsFactory() = default;
  virtual void reset(frDebugSettings* settings,
                     frDesign* design,
                     odb::dbDatabase* db,
                     utl::Logger* logger,
                     RouterConfiguration* router_cfg) = 0;
  virtual bool guiActive() = 0;
  virtual std::unique_ptr<AbstractDRGraphics> makeUniqueDRGraphics() = 0;
  virtual std::unique_ptr<AbstractTAGraphics> makeUniqueTAGraphics() = 0;
  virtual std::unique_ptr<AbstractPAGraphics> makeUniquePAGraphics() = 0;
};

}  // namespace drt
