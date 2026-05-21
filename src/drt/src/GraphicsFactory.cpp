// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "src/drt/src/GraphicsFactory.h"

#include <memory>
#include <utility>

#include "src/drt/src/dr/AbstractDRGraphics.h"
#include "src/drt/src/dr/FlexDR_graphics.h"
#include "src/drt/src/frBaseTypes.h"
#include "src/drt/src/frDesign.h"
#include "src/drt/src/global.h"
#include "src/drt/src/pa/AbstractPAGraphics.h"
#include "src/drt/src/pa/FlexPA_graphics.h"
#include "src/drt/src/ta/AbstractTAGraphics.h"
#include "src/drt/src/ta/FlexTA_graphics.h"
#include "src/gui/include/gui/gui.h"
#include "src/odb/include/odb/db.h"

namespace drt {

GraphicsFactory::GraphicsFactory() = default;
GraphicsFactory::~GraphicsFactory() = default;

void GraphicsFactory::reset(frDebugSettings* settings,
                            frDesign* design,
                            odb::dbDatabase* db,
                            utl::Logger* logger,
                            RouterConfiguration* router_cfg)
{
  settings_ = settings;
  design_ = design;
  db_ = db;
  logger_ = logger;
  router_cfg_ = router_cfg;
}

bool GraphicsFactory::guiActive()
{
  return gui::Gui::enabled();
}

std::unique_ptr<AbstractDRGraphics> GraphicsFactory::makeUniqueDRGraphics()
{
  if (!guiActive()) {
    return nullptr;
  }
  auto dr_graphics
      = std::make_unique<FlexDRGraphics>(settings_, design_, db_, logger_);
  dr_graphics->init();
  return std::move(dr_graphics);
}

std::unique_ptr<AbstractTAGraphics> GraphicsFactory::makeUniqueTAGraphics()
{
  if (!guiActive()) {
    return nullptr;
  }
  return std::make_unique<FlexTAGraphics>(settings_, design_, db_);
}

std::unique_ptr<AbstractPAGraphics> GraphicsFactory::makeUniquePAGraphics()
{
  if (!guiActive()) {
    return nullptr;
  }
  return std::make_unique<FlexPAGraphics>(
      settings_, design_, db_, logger_, router_cfg_);
}

}  // namespace drt
