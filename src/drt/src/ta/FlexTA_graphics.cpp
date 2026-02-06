// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#include "ta/FlexTA_graphics.h"

#include <string>

#include "db/obj/frShape.h"
#include "frBaseTypes.h"
#include "frDesign.h"
#include "gui/gui.h"
#include "ta/FlexTA.h"

namespace drt {

FlexTAGraphics::FlexTAGraphics(frDebugSettings* settings,
                               frDesign* design,
                               odb::dbDatabase* db)
    : settings_(settings),
      gui_(gui::Gui::get()),
      top_block_(design->getTopBlock()),
      net_(nullptr)
{
  // Build the layer map between opendb & tr
  auto odb_tech = db->getTech();

  layer_map_.resize(odb_tech->getLayerCount(), -1);

  for (auto& tr_layer : design->getTech()->getLayers()) {
    auto odb_layer = tr_layer->getDbLayer();
    if (odb_layer) {
      layer_map_[odb_layer->getNumber()] = tr_layer->getLayerNum();
    }
  }

  gui_->registerRenderer(this);
}

void FlexTAGraphics::drawIrouteGuide(frNet* net,
                                     odb::dbTechLayer* layer,
                                     gui::Painter& painter)
{
  frLayerNum layerNum;

  layerNum = layer_map_.at(layer->getNumber());
  if (layerNum < 0) {
    return;
  }

  for (auto& guide : net->getGuides()) {
    for (auto& uConnFig : guide->getRoutes()) {
      auto connFig = uConnFig.get();
      if (connFig->typeId() == frcPathSeg) {
        auto seg = static_cast<frPathSeg*>(connFig);
        if (seg->getLayerNum() == layerNum) {
          painter.drawRect(seg->getBBox());
        }
      }
    }
  }
}

void FlexTAGraphics::drawLayer(odb::dbTechLayer* layer, gui::Painter& painter)
{
  if (net_) {
    drawIrouteGuide(net_, layer, painter);
  } else {
    for (auto& net : top_block_->getNets()) {
      drawIrouteGuide(net.get(), layer, painter);
    }
  }
}

void FlexTAGraphics::status(const std::string& message)
{
  gui_->status(message);
}

void FlexTAGraphics::endIter(int iter)
{
  if (!settings_->netName.empty()) {
    for (auto& net : top_block_->getNets()) {
      if (net->getName() == settings_->netName) {
        net_ = net.get();
      }
    }
  }

  status("End iter: " + std::to_string(iter));
  if (settings_->allowPause) {
    gui_->redraw();
    gui_->pause();
  }
}

/* static */
bool FlexTAGraphics::guiActive()
{
  return gui::Gui::enabled();
}

}  // namespace drt
