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

namespace psm {

IRDropDataSource::IRDropDataSource(PDNSim* psm, utl::Logger* logger)
    : gui::RealValueHeatMapDataSource(logger,
                                      "V",
                                      "IR Drop",
                                      "IRDrop",
                                      "IRDrop"),
      psm_(psm),
      tech_(nullptr),
      layer_(nullptr)
{
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
}

void IRDropDataSource::setBlock(odb::dbBlock* block)
{
  gui::HeatMapDataSource::setBlock(block);
  if (block != nullptr) {
    tech_ = block->getDb()->getTech();
  }
}

double IRDropDataSource::getGridSizeMinimumValue() const
{
  odb::dbBlock* block = getBlock();
  if (block == nullptr || psm_ == nullptr) {
    return RealValueHeatMapDataSource::getGridSizeMinimumValue();
  }

  try {
    const double resolution = psm_->getMinimumResolution();
    double resolution_um = resolution / block->getDbUnitsPerMicron();
    if (resolution_um > getGridSizeMaximumValue()) {
      resolution_um
          = gui::RealValueHeatMapDataSource::getGridSizeMinimumValue();
    }
    return resolution_um;
  } catch (const std::runtime_error& /* e */) {
    // psm is not setup up
    return gui::RealValueHeatMapDataSource::getGridSizeMinimumValue();
  }
}

bool IRDropDataSource::populateMap()
{
  if (getBlock() == nullptr || psm_ == nullptr || tech_ == nullptr) {
    return false;
  }

  ensureLayer();

  std::map<odb::dbTechLayer*, std::map<odb::Point, double>> ir_drops;
  psm_->getIRDropMap(ir_drops);

  if (ir_drops.empty()) {
    return false;
  }

  // track min/max here to make it constant across all layers
  double min = std::numeric_limits<double>::max();
  double max = std::numeric_limits<double>::min();
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

void IRDropDataSource::setLayer(const std::string& name)
{
  if (tech_ == nullptr) {
    return;
  }

  layer_ = tech_->findLayer(name.c_str());
}

}  // namespace psm
