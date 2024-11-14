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

#include "FlexPA_graphics.h"

#include <algorithm>
#include <cstdio>
#include <limits>

#include "FlexPA.h"

namespace drt {

FlexPAGraphics::FlexPAGraphics(frDebugSettings* settings,
                               frDesign* design,
                               odb::dbDatabase* db,
                               Logger* logger,
                               RouterConfiguration* router_cfg)
    : logger_(logger),
      settings_(settings),
      inst_(nullptr),
      gui_(gui::Gui::get()),
      pin_(nullptr),
      inst_term_(nullptr),
      top_block_(design->getTopBlock()),
      pa_ap_(nullptr),
      pa_markers_(nullptr)
{
  // Build the layer map between opendb & tr
  auto odb_tech = db->getTech();

  layer_map_.resize(odb_tech->getLayerCount(), std::make_pair(-1, "none"));

  for (auto& tr_layer : design->getTech()->getLayers()) {
    auto odb_layer = tr_layer->getDbLayer();
    if (odb_layer) {
      layer_map_[odb_layer->getNumber()]
          = std::make_pair(tr_layer->getLayerNum(), odb_layer->getName());
    }
  }

  if (router_cfg->MAX_THREADS > 1) {
    logger_->info(DRT, 115, "Setting MAX_THREADS=1 for use with the PA GUI.");
    router_cfg->MAX_THREADS = 1;
  }

  if (!settings_->pinName.empty()) {
    // Break pinName into inst_name_ and term_name_ at ':'
    size_t pos = settings_->pinName.rfind(':');
    if (pos == std::string::npos) {
      logger_->error(
          DRT, 293, "pin name {} has no ':' delimiter", settings_->pinName);
    }
    term_name_ = settings_->pinName.substr(pos + 1);
    auto inst_name = settings_->pinName.substr(0, pos);
    logger_->info(
        DRT, 4000, "DEBUGGING inst {} term {}", inst_name, term_name_);
    if (inst_name == "PIN") {  // top level bterm
      inst_ = nullptr;
    } else {
      inst_ = design->getTopBlock()->getInst(inst_name);
      if (!inst_) {
        logger_->warn(DRT, 5000, "INST NOT FOUND!");
      }
    }
  }

  gui_->registerRenderer(this);
}

void FlexPAGraphics::drawLayer(odb::dbTechLayer* layer, gui::Painter& painter)
{
  frLayerNum layer_num;
  if (!shapes_.empty()) {
    layer_num = layer_map_.at(layer->getNumber()).first;
    for (auto& b : shapes_) {
      if (b.second != layer_num) {
        continue;
      }
      painter.drawRect(
          {b.first.xMin(), b.first.yMin(), b.first.xMax(), b.first.yMax()});
    }
  }

  if (!pin_) {
    return;
  }

  layer_num = layer_map_.at(layer->getNumber()).first;
  if (layer_num < 0) {
    return;
  }

  for (auto via : pa_vias_) {
    auto* via_def = via->getViaDef();
    Rect bbox;
    bool skip = false;
    if (via_def->getLayer1Num() == layer_num) {
      bbox = via->getLayer1BBox();
    } else if (via_def->getLayer2Num() == layer_num) {
      bbox = via->getLayer2BBox();
    } else {
      skip = true;
    }
    if (!skip) {
      painter.setPen(layer, /* cosmetic */ true);
      painter.setBrush(layer);
      painter.drawRect({bbox.xMin(), bbox.yMin(), bbox.xMax(), bbox.yMax()});
    }
  }

  for (auto seg : pa_segs_) {
    if (seg->getLayerNum() == layer_num) {
      Rect bbox = seg->getBBox();
      painter.setPen(layer, /* cosmetic */ true);
      painter.setBrush(layer);
      painter.drawRect({bbox.xMin(), bbox.yMin(), bbox.xMax(), bbox.yMax()});
    }
  }

  if (pa_markers_) {
    painter.setPen(gui::Painter::yellow, /* cosmetic */ true);
    painter.setBrush(gui::Painter::transparent);
    for (auto& marker : *pa_markers_) {
      if (marker->getLayerNum() == layer_num) {
        painter.drawRect(marker->getBBox());
      }
    }
  }

  for (const auto& ap : aps_) {
    if (ap.getLayerNum() != layer_num) {
      continue;
    }
    auto color = ap.hasAccess() ? gui::Painter::green : gui::Painter::red;
    painter.setPen(color, /* cosmetic */ true);

    const Point& pt = ap.getPoint();
    painter.drawX(pt.x(), pt.y(), 50);
  }
}

void FlexPAGraphics::startPin(frMPin* pin,
                              frInstTerm* inst_term,
                              std::set<frInst*, frBlockObjectComp>* inst_class)
{
  pin_ = nullptr;

  frMTerm* term = pin->getTerm();
  if (!settings_->pinName.empty()) {
    if (term_name_ != "*" && term->getName() != term_name_) {
      return;
    }
    if (inst_class->find(inst_) == inst_class->end()) {
      return;
    }
  }

  if (inst_term == nullptr) {
    logger_->error(DRT, 158, "Instance for MPin {} is null.", term->getName());
  }
  const std::string name
      = inst_term->getInst()->getName() + ':' + term->getName();
  status("Start pin: " + name);
  pin_ = pin;
  inst_term_ = inst_term;

  gui_->zoomTo(inst_term->getInst()->getBBox());
  gui_->pause();
}

void FlexPAGraphics::startPin(frBPin* pin,
                              frInstTerm* inst_term,
                              std::set<frInst*, frBlockObjectComp>* inst_class)
{
  pin_ = nullptr;

  frBTerm* term = pin->getTerm();
  if (!settings_->pinName.empty()) {
    if (inst_ != nullptr) {
      return;
    }
    if (term_name_ != "*" && term->getName() != term_name_) {
      return;
    }
  }

  const std::string name = "PIN:" + term->getName();
  status("Start pin: " + name);
  pin_ = pin;
  inst_term_ = nullptr;

  Rect box = term->getBBox();
  gui_->zoomTo({box.xMin(), box.yMin(), box.xMax(), box.yMax()});
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
    aps_.emplace_back(*ap);
  }
  status("add " + std::to_string(aps.size()) + " ( " + to_string(lower_type)
         + " / " + to_string(upper_type) + " ) "
         + " AP; total: " + std::to_string(aps_.size()));
  gui_->redraw();
  gui_->pause();
  aps_.clear();
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
  pa_segs_.clear();
  pa_markers_ = &markers;
  for (auto& marker : markers) {
    Rect bbox = marker->getBBox();
    std::string layer_name;
    for (auto& layer : layer_map_) {
      if (layer.first == marker->getLayerNum()) {
        layer_name = layer.second;
        break;
      }
    }
    logger_->info(DRT,
                  119,
                  "Marker ({}, {}) ({}, {}) on {}:",
                  bbox.xMin(),
                  bbox.yMin(),
                  bbox.xMax(),
                  bbox.yMax(),
                  layer_name);
    marker->getConstraint()->report(logger_);
  }

  gui_->redraw();
  gui_->pause();

  // These are going away once we return
  pa_ap_ = nullptr;
  pa_vias_.clear();
  pa_markers_ = nullptr;
}

void FlexPAGraphics::setPlanarAP(
    const frAccessPoint* ap,
    const frPathSeg* seg,
    const std::vector<std::unique_ptr<frMarker>>& markers)
{
  if (!pin_ || !settings_->paMarkers) {
    return;
  }

  pa_ap_ = ap;
  pa_vias_.clear();
  pa_segs_ = {seg};
  pa_markers_ = &markers;
  for (auto& marker : markers) {
    Rect bbox = marker->getBBox();
    logger_->info(DRT,
                  292,
                  "Marker {} at ({}, {}) ({}, {}).",
                  marker->getConstraint()->typeId(),
                  bbox.xMin(),
                  bbox.yMin(),
                  bbox.xMax(),
                  bbox.yMax());
  }

  gui_->redraw();
  gui_->pause();

  // These are going away once we return
  pa_ap_ = nullptr;
  pa_segs_.clear();
  pa_markers_ = nullptr;
}

void FlexPAGraphics::setObjsAndMakers(
    const std::vector<std::pair<frConnFig*, frBlockObject*>>& objs,
    const std::vector<std::unique_ptr<frMarker>>& markers,
    const FlexPA::PatternType type)
{
  if ((!settings_->paCommit && !settings_->paEdge)
      || (settings_->paCommit && type != FlexPA::Commit)
      || (settings_->paEdge && type != FlexPA::Edge)) {
    return;
  }

  for (auto [obj, parent] : objs) {
    if (obj->typeId() == frcVia) {
      auto via = static_cast<frVia*>(obj);
      pa_vias_.push_back(via);
    } else if (obj->typeId() == frcPathSeg) {
      auto seg = static_cast<frPathSeg*>(obj);
      pa_segs_.push_back(seg);
    } else {
      logger_->warn(DRT, 280, "Unknown type {} in setObjAP", obj->typeId());
    }
  }
  pa_markers_ = &markers;
  for (auto& marker : markers) {
    Rect bbox = marker->getBBox();
    logger_->info(DRT,
                  281,
                  "Marker {} at ({}, {}) ({}, {}).",
                  marker->getConstraint()->typeId(),
                  bbox.xMin(),
                  bbox.yMin(),
                  bbox.xMax(),
                  bbox.yMax());
  }

  gui_->redraw();
  gui_->pause();

  // These are going away once we return
  pa_markers_ = nullptr;
  pa_vias_.clear();
  pa_segs_.clear();
}

void FlexPAGraphics::status(const std::string& message)
{
  gui_->status(message);
}

/* static */
bool FlexPAGraphics::guiActive()
{
  return gui::Gui::enabled();
}

}  // namespace drt
