/* Author: Matt Liberty */
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

#include <algorithm>
#include <cstdio>
#include <limits>

#include "FlexPA.h"
#include "FlexPA_graphics.h"

namespace fr {

FlexPAGraphics::FlexPAGraphics(frDebugSettings* settings,
                               frDesign* design,
                               odb::dbDatabase* db,
                               Logger* logger)
    : logger_(logger),
      settings_(settings),
      gui_(gui::Gui::get()),
      pin_(nullptr),
      inst_term_(nullptr),
      top_block_(design->getTopBlock()),
      pa_ap_(nullptr),
      pa_markers_(nullptr)
{
  // Build the layer map between opendb & tr
  auto odb_tech = db->getTech();

  layer_map_.resize(odb_tech->getLayerCount(), -1);

  for (auto& tr_layer : design->getTech()->getLayers()) {
    auto odb_layer = odb_tech->findLayer(tr_layer->getName().c_str());
    if (odb_layer) {
      layer_map_[odb_layer->getNumber()] = tr_layer->getLayerNum();
    }
  }

  if (MAX_THREADS > 1) {
    logger_->info(DRT, 115, "Setting MAX_THREADS=1 for use with the PA GUI.");
    MAX_THREADS = 1;
  }

  gui_->registerRenderer(this);
}

void FlexPAGraphics::drawLayer(odb::dbTechLayer* layer, gui::Painter& painter)
{
  frLayerNum layerNum;
  if (!shapes_.empty()) {
    layerNum = layer_map_.at(layer->getNumber());
    for (auto& b : shapes_) {
      if (b.second != layerNum) {
        continue;
      }
      painter.drawRect(
          {b.first.left(), b.first.bottom(), b.first.right(), b.first.top()});
    }
  }

  if (!pin_) {
    return;
  }

  layerNum = layer_map_.at(layer->getNumber());
  if (layerNum < 0) {
    return;
  }

  for (auto via : pa_vias_) {
    auto* via_def = via->getViaDef();
    frBox bbox;
    bool skip = false;
    if (via_def->getLayer1Num() == layerNum) {
      via->getLayer1BBox(bbox);
    } else if (via_def->getLayer2Num() == layerNum) {
      via->getLayer2BBox(bbox);
    } else {
      skip = true;
    }
    if (!skip) {
      painter.setPen(layer, /* cosmetic */ true);
      painter.setBrush(layer);
      painter.drawRect({bbox.left(), bbox.bottom(), bbox.right(), bbox.top()});
    }
  }

  if (pa_markers_) {
    painter.setPen(gui::Painter::yellow, /* cosmetic */ true);
    painter.setBrush(gui::Painter::transparent);
    for (auto& marker : *pa_markers_) {
      if (marker->getLayerNum() == layerNum) {
        frBox bbox;
        marker->getBBox(bbox);
        painter.drawRect(
            {bbox.left(), bbox.bottom(), bbox.right(), bbox.top()});
      }
    }
  }

  for (const auto& ap : aps_) {
    if (ap.getLayerNum() != layerNum) {
      continue;
    }
    auto color = ap.hasAccess() ? gui::Painter::green : gui::Painter::red;
    painter.setPen(color, /* cosmetic */ true);

    frPoint pt = ap.getPoint();
    painter.drawLine({pt.x() - 50, pt.y() - 50}, {pt.x() + 50, pt.y() + 50});
    painter.drawLine({pt.x() - 50, pt.y() + 50}, {pt.x() + 50, pt.y() - 50});
  }
}

void FlexPAGraphics::startPin(frPin* pin, frInstTerm* inst_term)
{
  pin_ = nullptr;
  aps_.clear();

  frTerm* term = pin->getTerm();
  //  frBlock* block = term->getBlock();
  //  std::string name = block->getName() + ':' + term->getName()
  std::string name = (inst_term ? inst_term->getInst()->getName() : "") + ':'
                     + term->getName();
  if (!settings_->pinName.empty() && name != settings_->pinName) {
    return;
  }

  status("Start pin: " + name);
  pin_ = pin;
  inst_term_ = inst_term;

  frBox box;
  inst_term->getInst()->getBBox(box);
  gui_->zoomTo({box.left(), box.bottom(), box.right(), box.top()});
  gui_->pause();
}

static const char* to_string(frAccessPointEnum e)
{
  switch (e) {
    case frAccessPointEnum::OnGrid:
      return "on-grid";
    case frAccessPointEnum::HalfGrid:
      return "half-grid";
    case frAccessPointEnum::Center:
      return "center";
    case frAccessPointEnum::EncOpt:
      return "enclose";
    case frAccessPointEnum::NearbyGrid:
      return "nearby";
  }
  return "unknown";
}

void FlexPAGraphics::setAPs(
    const std::vector<std::unique_ptr<frAccessPoint>>& aps,
    frAccessPointEnum lower_type,
    frAccessPointEnum upper_type)
{
  if (!pin_) {
    return;
  }

  // We make a copy of the aps
  for (auto& ap : aps) {
    aps_.emplace_back(*ap.get());
  }
  status("add " + std::to_string(aps.size()) + " ( " + to_string(lower_type)
         + " / " + to_string(upper_type) + " ) "
         + " AP; total: " + std::to_string(aps_.size()));
  gui_->redraw();
  gui_->pause();
}

void FlexPAGraphics::setViaAP(
    const frAccessPoint* ap,
    const frVia* via,
    const std::vector<std::unique_ptr<frMarker>>& markers)
{
  if (!pin_ || !settings_->paMarkers) {
    return;
  }

  pa_ap_ = ap;
  pa_vias_ = {via};
  pa_markers_ = &markers;
  for (auto& marker : markers) {
    frBox bbox;
    marker->getBBox(bbox);
    logger_->info(DRT,
                  119,
                  "Marker {} at ({}, {}) ({}, {}).",
                  marker->getConstraint()->typeId(),
                  bbox.left(),
                  bbox.bottom(),
                  bbox.right(),
                  bbox.top());
  }

  gui_->redraw();
  gui_->pause();

  // These are going away once we return
  pa_ap_ = nullptr;
  pa_vias_.clear();
  pa_markers_ = nullptr;
}

void FlexPAGraphics::setObjsAndMakers(
    const vector<pair<frConnFig*, frBlockObject*>>& objs,
    const std::vector<std::unique_ptr<frMarker>>& markers)
{
  if (!settings_->paCombining) {
    return;
  }

  for (auto [obj, parent] : objs) {
    if (obj->typeId() == frcVia) {
      auto via = static_cast<frVia*>(obj);
      pa_vias_.push_back(via);
    } else {
      logger_->warn(DRT, 280, "Unknown type {} in setObjAP", obj->typeId());
    }
  }
  pa_markers_ = &markers;
  for (auto& marker : markers) {
    frBox bbox;
    marker->getBBox(bbox);
    logger_->info(DRT,
                  279,
                  "Marker {} at ({}, {}) ({}, {}).",
                  marker->getConstraint()->typeId(),
                  bbox.left(),
                  bbox.bottom(),
                  bbox.right(),
                  bbox.top());
  }

  gui_->redraw();
  gui_->pause();

  // These are going away once we return
  pa_markers_ = nullptr;
  pa_vias_.clear();
}

void FlexPAGraphics::status(const std::string& message)
{
  gui_->status(message);
}

/* static */
bool FlexPAGraphics::guiActive()
{
  return gui::Gui::get() != nullptr;
}

}  // namespace fr
