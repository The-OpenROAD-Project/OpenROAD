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

namespace grt {

RoutingCongestionDataSource::RoutingCongestionDataSource(utl::Logger* logger,
                                                         odb::dbDatabase* db)
    : gui::HeatMapDataSource(logger,
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
        } else {
          return layer_->getName();
        }
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

double RoutingCongestionDataSource::getGridXSize() const
{
  if (getBlock() == nullptr) {
    return default_grid_;
  }

  auto* gcellgrid = getBlock()->getGCellGrid();
  if (gcellgrid == nullptr) {
    return default_grid_;
  }

  std::vector<int> grid;
  gcellgrid->getGridX(grid);

  if (grid.size() < 2) {
    return default_grid_;
  } else {
    const double delta = grid[1] - grid[0];
    return delta / getBlock()->getDbUnitsPerMicron();
  }
}

double RoutingCongestionDataSource::getGridYSize() const
{
  if (getBlock() == nullptr) {
    return default_grid_;
  }

  auto* gcellgrid = getBlock()->getGCellGrid();
  if (gcellgrid == nullptr) {
    return default_grid_;
  }

  std::vector<int> grid;
  gcellgrid->getGridY(grid);

  if (grid.size() < 2) {
    return default_grid_;
  } else {
    const double delta = grid[1] - grid[0];
    return delta / getBlock()->getDbUnitsPerMicron();
  }
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

  auto gcell_congestion_data = grid->getCongestionMap(layer_);
  if (gcell_congestion_data.numElems() == 0) {
    return false;
  }

  std::vector<int> x_grid, y_grid;
  grid->getGridX(x_grid);
  const odb::uint x_grid_sz = x_grid.size();
  grid->getGridY(y_grid);
  const odb::uint y_grid_sz = y_grid.size();

  for (odb::uint x_idx = 0; x_idx < gcell_congestion_data.numRows(); ++x_idx) {
    for (odb::uint y_idx = 0; y_idx < gcell_congestion_data.numCols();
         ++y_idx) {
      const auto& cong_data = gcell_congestion_data(x_idx, y_idx);

      const int next_x = (x_idx + 1) == x_grid_sz
                             ? getBlock()->getDieArea().xMax()
                             : x_grid[x_idx + 1];
      const int next_y = (y_idx + 1) == y_grid_sz
                             ? getBlock()->getDieArea().yMax()
                             : y_grid[y_idx + 1];

      const odb::Rect gcell_rect(x_grid[x_idx], y_grid[y_idx], next_x, next_y);

      const auto hor_capacity = cong_data.horizontal_capacity;
      const auto hor_usage = cong_data.horizontal_usage;
      const auto ver_capacity = cong_data.vertical_capacity;
      const auto ver_usage = cong_data.vertical_usage;

      //-1 indicates capacity is not well defined...
      const double hor_congestion
          = hor_capacity != 0 ? static_cast<double>(hor_usage) / hor_capacity
                              : -1;
      const double ver_congestion
          = ver_capacity != 0 ? static_cast<double>(ver_usage) / ver_capacity
                              : -1;

      double value = 0.0;
      switch (type_) {
        case Congestion:
          if (direction_ == ALL) {
            value = std::max(hor_congestion, ver_congestion);
          } else if (direction_ == HORIZONTAL) {
            value = hor_congestion;
          } else {
            value = ver_congestion;
          }
          value *= 100.0;
          break;
        case Usage:
          if (direction_ == ALL) {
            value = std::max(hor_usage, ver_usage);
          } else if (direction_ == HORIZONTAL) {
            value = hor_usage;
          } else {
            value = ver_usage;
          }
          break;
        case Capacity:
          if (direction_ == ALL) {
            value = std::max(hor_capacity, ver_capacity);
          } else if (direction_ == HORIZONTAL) {
            value = hor_capacity;
          } else {
            value = ver_capacity;
          }
          break;
      }

      if (value < 0) {
        continue;
      }

      addToMap(gcell_rect, value);
    }
  }

  return true;
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
