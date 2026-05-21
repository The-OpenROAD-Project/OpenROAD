// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#pragma once

#include <memory>
#include <vector>

#include "src/drt/src/AbstractGraphicsFactory.h"
#include "src/drt/src/db/obj/frBlockObject.h"
#include "src/drt/src/dr/AbstractDRGraphics.h"
#include "src/drt/src/dr/FlexDR_graphics.h"
#include "src/drt/src/frBaseTypes.h"
#include "src/drt/src/pa/AbstractPAGraphics.h"
#include "src/drt/src/pa/FlexPA_graphics.h"
#include "src/drt/src/ta/AbstractTAGraphics.h"
#include "src/drt/src/ta/FlexTA_graphics.h"
#include "src/gui/include/gui/gui.h"

namespace drt {
class GraphicsFactory : public AbstractGraphicsFactory
{
 public:
  GraphicsFactory();
  ~GraphicsFactory() override;
  void reset(frDebugSettings* settings,
             frDesign* design,
             odb::dbDatabase* db,
             utl::Logger* logger,
             RouterConfiguration* router_cfg) override;
  bool guiActive() override;
  std::unique_ptr<AbstractDRGraphics> makeUniqueDRGraphics() override;
  std::unique_ptr<AbstractTAGraphics> makeUniqueTAGraphics() override;
  std::unique_ptr<AbstractPAGraphics> makeUniquePAGraphics() override;

 private:
  frDebugSettings* settings_;
  frDesign* design_;
  odb::dbDatabase* db_;
  utl::Logger* logger_;
  RouterConfiguration* router_cfg_;
};

}  // namespace drt
