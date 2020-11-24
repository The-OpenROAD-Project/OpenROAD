#include <algorithm>
#include <cstdio>
#include <limits>

#include "FlexDR_graphics.h"
#include "FlexDR.h"

namespace fr {

FlexDRGraphics::FlexDRGraphics(frDebugSettings* settings)
  : worker_(nullptr),
    net_(nullptr),
    settings_(settings)
{
  gui::Gui::get()->register_renderer(this);
}

void FlexDRGraphics::drawLayer(odb::dbTechLayer* layer, gui::Painter& painter)
{
  if (!net_) {
    return;
  }

  painter.setPen(layer);
  painter.setBrush(layer);

  for (auto& rect : net_->getOrigGuides()) {
    if (rect.getLayerNum() == layer->getNumber()) {
      // printf("tr layer %s vs db layer %s\n",

      frBox box;
      rect.getBBox(box);
      painter.drawRect({box.left(), box.bottom(), box.right(), box.top()});
    }
  }
}

void FlexDRGraphics::drawObjects(gui::Painter& painter)
{
  if (!worker_) {
    return;
  }

  painter.setBrush(gui::Painter::transparent);
  painter.setPen(gui::Painter::yellow, /* cosmetic */ true);

  frBox box;
  worker_->getRouteBox(box);
  painter.drawRect({box.left(), box.bottom(), box.right(), box.top()});

  box = worker_->getDrcBox();
  painter.drawRect({box.left(), box.bottom(), box.right(), box.top()});

  worker_->getExtBox(box);
  painter.drawRect({box.left(), box.bottom(), box.right(), box.top()});

  if (net_) {
    for (auto& pin : net_->getPins()) {
      for (auto& ap : pin->getAccessPatterns()) {
        frPoint pt;
        ap->getPoint(pt);
        painter.drawLine({pt.x() - 100, pt.y() - 100},
                         {pt.x() + 100, pt.y() + 100});
        painter.drawLine({pt.x() - 100, pt.y() + 100},
                         {pt.x() + 100, pt.y() - 100});
      }
    }
  }
}

void FlexDRGraphics::startWorker(FlexDRWorker* in)
{
  status("start worker");
  worker_ = in;
  net_ = nullptr;

  auto* gui = gui::Gui::get();


  if (settings_->netName.empty()) {
    frBox box;
    worker_->getExtBox(box);
    gui->zoomTo({box.left(), box.bottom(), box.right(), box.top()});
    gui->pause();
  }
}

void FlexDRGraphics::startNet(drNet* net)
{
  if (!settings_->netName.empty() &&
      net->getFrNet()->getName() != settings_->netName) {
    return;
  }

  status("start net: " + net->getFrNet()->getName());
  net_ = net;

  auto* gui = gui::Gui::get();
  frBox box;
  worker_->getExtBox(box);
  gui->zoomTo({box.left(), box.bottom(), box.right(), box.top()});
  gui->pause();
}

void FlexDRGraphics::status(const std::string& message)
{
  gui::Gui::get()->status(message);
}

/* static */
bool FlexDRGraphics::guiActive()
{
  return gui::Gui::get() != nullptr;
}

}  // namespace fr
