// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "GraphicsFactory.h"

#include <memory>
#include <utility>

namespace drt {

GraphicsFactory::GraphicsFactory() = default;
GraphicsFactory::~GraphicsFactory() = default;

void GraphicsFactory::reset(frDebugSettings* settings,
                            frDesign* design,
                            odb::dbDatabase* db,
                            Logger* logger,
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
