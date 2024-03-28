//////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, The Regents of the University of California
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

#include "heatMap.h"

#include "psm/pdnsim.h"
#include "sta/Corner.hh"
#include "sta/Sta.hh"

namespace psm {

IRDropDataSource::IRDropDataSource(PDNSim* psm,
                                   sta::Sta* sta,
                                   utl::Logger* logger)
    : gui::RealValueHeatMapDataSource(logger,
                                      "V",
                                      "IR Drop",
                                      "IRDrop",
                                      "IRDrop"),
      psm_(psm),
      sta_(sta)
{
  addMultipleChoiceSetting(
      "Net",
      "Net:",
      [this]() {
        std::vector<std::string> nets;
        if (getBlock() == nullptr) {
          return nets;
        }
        for (auto* net : getBlock()->getNets()) {
          if (net->getSigType().isSupply()) {
            nets.push_back(net->getName());
          }
        }
        return nets;
      },
      [this]() -> std::string {
        if (net_ == nullptr) {
          return "";
        }
        return net_->getName();
      },
      [this](const std::string& value) { setNet(value); });
  addMultipleChoiceSetting(
      "Layer",
      "Layer:",
      [this]() {
        std::vector<std::string> layers;
        for (auto* layer : tech_->getLayers()) {
          if (layer->getType() == odb::dbTechLayerType::ROUTING) {
            layers.push_back(layer->getName());
          }
        }
        return layers;
      },
      [this]() -> std::string {
        ensureLayer();
        if (layer_ == nullptr) {
          return "";
        }
        return layer_->getName();
      },
      [this](const std::string& value) {
        if (tech_ == nullptr) {
          return;
        }
        layer_ = tech_->findLayer(value.c_str());
      });
  addMultipleChoiceSetting(
      "Corner",
      "Corner:",
      [this]() {
        std::vector<std::string> corners;
        for (auto* corner : *sta_->corners()) {
          corners.emplace_back(corner->name());
        }
        return corners;
      },
      [this]() -> std::string {
        ensureCorner();
        if (corner_ == nullptr) {
          return "";
        }
        return corner_->name();
      },
      [this](const std::string& value) { setCorner(value); });
}

void IRDropDataSource::setBlock(odb::dbBlock* block)
{
  gui::HeatMapDataSource::setBlock(block);
  if (block != nullptr) {
    tech_ = block->getTech();
  }
}

bool IRDropDataSource::populateMap()
{
  if (getBlock() == nullptr || psm_ == nullptr || tech_ == nullptr) {
    return false;
  }

  ensureNet();
  if (net_ == nullptr) {
    return false;
  }

  ensureCorner();
  if (corner_ == nullptr) {
    return false;
  }
  ensureLayer();

  std::map<odb::dbTechLayer*, PDNSim::IRDropByPoint> ir_drops;

  for (auto* layer : tech_->getLayers()) {
    psm_->getIRDropForLayer(net_, corner_, layer, ir_drops[layer]);
  }

  bool empty = true;
  for (const auto& [layer, ir_drop] : ir_drops) {
    if (!ir_drop.empty()) {
      empty = false;
    }
  }

  if (empty) {
    return false;
  }

  // track min/max here to make it constant across all layers
  double min = std::numeric_limits<double>::max();
  double max = std::numeric_limits<double>::lowest();
  for (const auto& [layer, drop_map] : ir_drops) {
    for (const auto& [point, drop] : drop_map) {
      min = std::min(min, drop);
      max = std::max(max, drop);
    }
  }
  setMinValue(min);
  setMaxValue(max);

  auto& ir_drop = ir_drops[layer_];
  for (const auto& [point, drop] : ir_drop) {
    addToMap({point, point}, drop);
  }

  return true;
}

void IRDropDataSource::determineMinMax(const HeatMapDataSource::Map& map)
{
  // do nothing handled in populateMap
}

void IRDropDataSource::combineMapData(bool base_has_value,
                                      double& base,
                                      const double new_data,
                                      const double data_area,
                                      const double intersection_area,
                                      const double rect_area)
{
  if (!base_has_value) {
    base = new_data;
  } else {
    base = std::max(base, new_data);
  }
}

void IRDropDataSource::ensureLayer()
{
  if (layer_ != nullptr) {
    return;
  }

  if (tech_ == nullptr) {
    return;
  }

  layer_ = tech_->findRoutingLayer(1);
}

void IRDropDataSource::setNet(const std::string& name)
{
  if (getBlock() == nullptr) {
    return;
  }

  net_ = getBlock()->findNet(name.c_str());
}

void IRDropDataSource::ensureNet()
{
  if (net_ != nullptr) {
    return;
  }

  if (getBlock() == nullptr) {
    return;
  }

  for (auto* net : getBlock()->getNets()) {
    if (net->getSigType() == odb::dbSigType::POWER) {
      net_ = net;
      return;
    }
  }
}

void IRDropDataSource::setLayer(const std::string& name)
{
  if (tech_ == nullptr) {
    return;
  }

  layer_ = tech_->findLayer(name.c_str());
}

void IRDropDataSource::setCorner(const std::string& name)
{
  corner_ = sta_->findCorner(name.c_str());
}

void IRDropDataSource::ensureCorner()
{
  if (corner_ != nullptr) {
    return;
  }

  auto corners = sta_->corners()->corners();
  if (!corners.empty()) {
    corner_ = *corners.begin();
  }

  corner_ = sta_->cmdCorner();
}

}  // namespace psm
