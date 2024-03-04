/*
 * Copyright (c) 2020, The Regents of the University of California
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "FlexTA_graphics.h"

#include "FlexTA.h"

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
