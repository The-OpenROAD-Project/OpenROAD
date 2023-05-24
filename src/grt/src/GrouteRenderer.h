#pragma once

#include "AbstractGrouteRenderer.h"
#include "grt/GlobalRouter.h"
#include "gui/gui.h"

namespace grt {

class GrouteRenderer : public gui::Renderer, public AbstractGrouteRenderer
{
 public:
  GrouteRenderer(GlobalRouter* groute, odb::dbTech* tech);

  void highlightRoute(odb::dbNet* net, bool show_pin_locations) override;

  void clearRoute() override;

  void drawLayer(odb::dbTechLayer* layer, gui::Painter& painter) override;

 private:
  GlobalRouter* groute_;
  odb::dbTech* tech_;
  std::set<odb::dbNet*> nets_;
  std::unordered_map<odb::dbNet*, bool> show_pin_locations_;
};

}  // namespace grt
