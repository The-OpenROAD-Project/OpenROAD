// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#include "heatMap.h"

#include <algorithm>
#include <cstdint>
#include <vector>

#include "gui/heatMap.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "utl/Logger.h"

namespace grt {

RoutingCongestionDataSource::RoutingCongestionDataSource(utl::Logger* logger,
                                                         odb::dbDatabase* db)
    : gui::GlobalRoutingDataSource(logger,
                                   "Routing Congestion",
                                   "Routing",
                                   "RoutingCongestion"),
      db_(db),
      direction_(ALL),
      layer_(nullptr),
      type_(Congestion),
      max_(1.0)
{
  addMultipleChoiceSetting(
      "Direction",
      "Direction:",
      []() {
        return std::vector<std::string>{"All", "Horizontal", "Vertical"};
      },
      [this]() -> std::string {
        switch (direction_) {
          case ALL:
            return "All";
          case HORIZONTAL:
            return "Horizontal";
          case VERTICAL:
            return "Vertical";
        }
        return "All";  // default to all
      },
      [this](const std::string& value) {
        if (value == "All") {
          direction_ = ALL;
        } else if (value == "Horizontal") {
          direction_ = HORIZONTAL;
        } else if (value == "Vertical") {
          direction_ = VERTICAL;
        } else {
          direction_ = ALL;  // default to all
        }
      });
  addMultipleChoiceSetting(
      "Layer",
      "Layer:",
      [this]() {
        std::vector<std::string> layers{"All"};
        auto* tech = db_->getTech();
        if (tech == nullptr) {
          return layers;
        }
        for (auto* layer : tech->getLayers()) {
          if (layer->getRoutingLevel() != 0) {
            layers.push_back(layer->getName());
          }
        }
        return layers;
      },
      [this]() -> std::string {
        if (layer_ == nullptr) {
          return "All";  // default to all
        }
        return layer_->getName();
      },
      [this](const std::string& value) {
        auto* tech = db_->getTech();
        if (value == "All" || tech == nullptr) {
          layer_ = nullptr;
        } else {
          layer_ = tech->findLayer(value.c_str());
        }
      });
  addMultipleChoiceSetting(
      "Type",
      "Type:",
      []() -> std::vector<std::string> {
        return {"Congestion", "Capacity", "Usage"};
      },
      [this]() -> std::string {
        switch (type_) {
          case Congestion:
            return "Congestion";
          case Capacity:
            return "Capacity";
          case Usage:
            return "Usage";
        }
        return "Congestion";
      },
      [this](const std::string& value) {
        if (value == "Congestion") {
          type_ = Congestion;
        } else if (value == "Capacity") {
          type_ = Capacity;
        } else if (value == "Usage") {
          type_ = Usage;
        }
      });
}

bool RoutingCongestionDataSource::populateMap()
{
  if (getBlock() == nullptr) {
    return false;
  }

  auto* grid = getBlock()->getGCellGrid();
  if (grid == nullptr) {
    return false;
  }

  if (layer_ != nullptr) {
    return populateMapForLayer(layer_, grid);
  }

  return populateMapForDirection(direction_, grid);
}

bool RoutingCongestionDataSource::populateMapForLayer(odb::dbTechLayer* layer,
                                                      odb::dbGCellGrid* grid)
{
  auto congestion_data = grid->getLayerCongestionMap(layer_);
  bool is_vertical = layer_->getDirection() == odb::dbTechLayerDir::VERTICAL;
  bool show_data = direction_ == ALL || (is_vertical && direction_ == VERTICAL)
                   || (!is_vertical && direction_ == HORIZONTAL);

  std::vector<int> x_grid, y_grid;
  grid->getGridX(x_grid);
  const auto x_grid_sz = x_grid.size();
  grid->getGridY(y_grid);
  const auto y_grid_sz = y_grid.size();

  for (uint32_t x_idx = 0; x_idx < congestion_data.numRows(); ++x_idx) {
    for (uint32_t y_idx = 0; y_idx < congestion_data.numCols(); ++y_idx) {
      const auto& cong_data = congestion_data(x_idx, y_idx);

      const int next_x = (x_idx + 1) == x_grid_sz
                             ? getBlock()->getDieArea().xMax()
                             : x_grid[x_idx + 1];
      const int next_y = (y_idx + 1) == y_grid_sz
                             ? getBlock()->getDieArea().yMax()
                             : y_grid[y_idx + 1];

      const odb::Rect gcell_rect(x_grid[x_idx], y_grid[y_idx], next_x, next_y);

      auto capacity = cong_data.capacity;
      auto usage = cong_data.usage;

      //-1 indicates capacity is not well defined...
      const double congestion
          = capacity != 0 ? static_cast<double>(usage) / capacity : -1;

      double value = defineValue(capacity, usage, congestion, show_data);

      if (value < 0) {
        continue;
      }

      addToMap(gcell_rect, value);
    }
  }

  return true;
}

bool RoutingCongestionDataSource::populateMapForDirection(
    Direction direction,
    odb::dbGCellGrid* grid)
{
  auto hor_congestion_data
      = grid->getDirectionCongestionMap(odb::dbTechLayerDir::HORIZONTAL);
  auto ver_congestion_data
      = grid->getDirectionCongestionMap(odb::dbTechLayerDir::VERTICAL);
  if (hor_congestion_data.numElems() == 0
      || ver_congestion_data.numElems() == 0) {
    return false;
  }

  std::vector<int> x_grid, y_grid;
  grid->getGridX(x_grid);
  const auto x_grid_sz = x_grid.size();
  grid->getGridY(y_grid);
  const auto y_grid_sz = y_grid.size();

  for (uint32_t x_idx = 0; x_idx < hor_congestion_data.numRows(); ++x_idx) {
    for (uint32_t y_idx = 0; y_idx < hor_congestion_data.numCols(); ++y_idx) {
      const auto& hor_cong_data = hor_congestion_data(x_idx, y_idx);
      const auto& ver_cong_data = ver_congestion_data(x_idx, y_idx);

      const int next_x = (x_idx + 1) == x_grid_sz
                             ? getBlock()->getDieArea().xMax()
                             : x_grid[x_idx + 1];
      const int next_y = (y_idx + 1) == y_grid_sz
                             ? getBlock()->getDieArea().yMax()
                             : y_grid[y_idx + 1];

      const odb::Rect gcell_rect(x_grid[x_idx], y_grid[y_idx], next_x, next_y);

      double capacity, usage;
      double congestion;
      setCongestionValues(
          hor_cong_data, ver_cong_data, capacity, usage, congestion);

      double value = defineValue(capacity, usage, congestion, true);

      if (value < 0) {
        continue;
      }

      addToMap(gcell_rect, value);
    }
  }

  return true;
}

double RoutingCongestionDataSource::defineValue(const double capacity,
                                                const double usage,
                                                const double congestion,
                                                const bool show_data)
{
  double value = 0.0;
  switch (type_) {
    case Congestion:
      if (show_data) {
        value = congestion;
      }
      value *= 100.0;
      break;
    case Usage:
      if (show_data) {
        value = usage;
      }
      break;
    case Capacity:
      if (show_data) {
        value = capacity;
      }
      break;
  }

  return value;
}

void RoutingCongestionDataSource::setCongestionValues(
    const odb::dbGCellGrid::GCellData& hor_cong_data,
    const odb::dbGCellGrid::GCellData& ver_cong_data,
    double& capacity,
    double& usage,
    double& congestion)
{
  auto hor_capacity = hor_cong_data.capacity;
  auto hor_usage = hor_cong_data.usage;
  auto ver_capacity = ver_cong_data.capacity;
  auto ver_usage = ver_cong_data.usage;

  //-1 indicates capacity is not well defined...
  const double hor_congestion
      = hor_capacity != 0 ? static_cast<double>(hor_usage) / hor_capacity : -1;
  const double ver_congestion
      = ver_capacity != 0 ? static_cast<double>(ver_usage) / ver_capacity : -1;

  if (direction_ == HORIZONTAL) {
    capacity = hor_capacity;
    usage = hor_usage;
    congestion = hor_congestion;
  } else if (direction_ == VERTICAL) {
    capacity = ver_capacity;
    usage = ver_usage;
    congestion = ver_congestion;
  } else {
    capacity = hor_capacity + ver_capacity;
    usage = hor_usage + ver_usage;
    congestion = std::max(hor_congestion, ver_congestion);
  }
}

void RoutingCongestionDataSource::correctMapScale(HeatMapDataSource::Map& map)
{
  if (type_ == Congestion) {
    return;
  }

  max_ = 0.0;
  for (const auto& map_col : map) {
    for (const auto& map_pt : map_col) {
      max_ = std::max(map_pt->value, max_);
    }
  }

  if (max_ == 0.0) {
    return;
  }

  for (const auto& map_col : map) {
    for (const auto& map_pt : map_col) {
      map_pt->value = map_pt->value * 100 / max_;
    }
  }
}

std::string RoutingCongestionDataSource::formatValue(double value,
                                                     bool legend) const
{
  if (type_ == Congestion) {
    return HeatMapDataSource::formatValue(value, legend);
  }

  return HeatMapDataSource::formatValue(value / 100 * max_, legend);
}

void RoutingCongestionDataSource::combineMapData(bool base_has_value,
                                                 double& base,
                                                 const double new_data,
                                                 const double data_area,
                                                 const double intersection_area,
                                                 const double rect_area)
{
  base += new_data * intersection_area / rect_area;
}

}  // namespace grt
