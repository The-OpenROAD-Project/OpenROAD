//////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2025, Precision Innovations Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include "GraphicsFactory.h"

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