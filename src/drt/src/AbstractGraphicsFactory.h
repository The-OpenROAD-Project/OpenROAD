// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#pragma once

#include <memory>
#include <vector>

#include "db/obj/frBlockObject.h"
#include "dr/AbstractDRGraphics.h"
#include "frBaseTypes.h"
#include "pa/AbstractPAGraphics.h"
#include "ta/AbstractTAGraphics.h"

namespace drt {
class AbstractGraphicsFactory
{
 public:
  virtual ~AbstractGraphicsFactory() = default;
  virtual void reset(frDebugSettings* settings,
                     frDesign* design,
                     odb::dbDatabase* db,
                     Logger* logger,
                     RouterConfiguration* router_cfg)
      = 0;
  virtual bool guiActive() = 0;
  virtual std::unique_ptr<AbstractDRGraphics> makeUniqueDRGraphics() = 0;
  virtual std::unique_ptr<AbstractTAGraphics> makeUniqueTAGraphics() = 0;
  virtual std::unique_ptr<AbstractPAGraphics> makeUniquePAGraphics() = 0;
};

}  // namespace drt
