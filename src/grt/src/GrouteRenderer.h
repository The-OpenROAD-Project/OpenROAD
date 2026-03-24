// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#pragma once

#include <set>
#include <unordered_map>

#include "AbstractGrouteRenderer.h"
#include "grt/GRoute.h"
#include "grt/GlobalRouter.h"
#include "gui/gui.h"
#include "odb/db.h"

namespace grt {

class GrouteRenderer : public gui::Renderer, public AbstractGrouteRenderer
{
 public:
  GrouteRenderer(GlobalRouter* groute, odb::dbTech* tech);

  void highlightRoute(odb::dbNet* net, bool show_pin_locations) override;

  void clearRoute() override;

  void drawLayer(odb::dbTechLayer* layer, gui::Painter& painter) override;

 private:
  void drawViaRect(const GSegment& seg,
                   odb::dbTechLayer* layer,
                   gui::Painter& painter);
  GlobalRouter* groute_;
  odb::dbTech* tech_;
  std::set<odb::dbNet*> nets_;
  std::unordered_map<odb::dbNet*, bool> show_segments_;
  std::unordered_map<odb::dbNet*, bool> show_pin_locations_;
};

}  // namespace grt
