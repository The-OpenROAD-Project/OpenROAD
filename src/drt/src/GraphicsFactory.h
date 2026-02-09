// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#pragma once

#include <memory>
#include <vector>

#include "AbstractGraphicsFactory.h"
#include "db/obj/frBlockObject.h"
#include "dr/AbstractDRGraphics.h"
#include "dr/FlexDR_graphics.h"
#include "frBaseTypes.h"
#include "gui/gui.h"
#include "pa/AbstractPAGraphics.h"
#include "pa/FlexPA_graphics.h"
#include "ta/AbstractTAGraphics.h"
#include "ta/FlexTA_graphics.h"

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
