// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#include "heatMap.h"

#include <algorithm>
#include <limits>
#include <map>
#include <string>
#include <vector>

#include "gui/heatMap.h"
#include "odb/dbTypes.h"
#include "psm/pdnsim.h"
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
        for (auto* scene : sta_->scenes()) {
          corners.emplace_back(scene->name());
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
  if (block && block->getParent()) {
    return;  // not the top block so ignore it
  }
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
  corner_ = sta_->findScene(name);
}

void IRDropDataSource::ensureCorner()
{
  if (corner_ != nullptr) {
    return;
  }

  corner_ = sta_->cmdScene();
}

}  // namespace psm
