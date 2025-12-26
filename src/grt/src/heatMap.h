// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

#include <string>

#include "AbstractRoutingCongestionDataSource.h"
#include "gui/heatMap.h"
#include "odb/db.h"

namespace grt {

class RoutingCongestionDataSource : public gui::GlobalRoutingDataSource,
                                    public AbstractRoutingCongestionDataSource
{
 public:
  RoutingCongestionDataSource(utl::Logger* logger, odb::dbDatabase* db);

  void registerHeatMap() override { gui::HeatMapDataSource::registerHeatMap(); }
  void update() override { gui::HeatMapDataSource::update(); }

 protected:
  bool populateMap() override;
  void combineMapData(bool base_has_value,
                      double& base,
                      double new_data,
                      double data_area,
                      double intersection_area,
                      double rect_area) override;
  void correctMapScale(HeatMapDataSource::Map& map) override;
  std::string formatValue(double value, bool legend) const override;

 private:
  enum Direction
  {
    ALL,
    HORIZONTAL,
    VERTICAL
  };
  enum MapType
  {
    Congestion,
    Usage,
    Capacity
  };

  bool populateMapForLayer(odb::dbTechLayer* layer, odb::dbGCellGrid* grid);
  bool populateMapForDirection(Direction direction, odb::dbGCellGrid* grid);
  double defineValue(double capacity,
                     double usage,
                     double congestion,
                     bool show_data);
  void setCongestionValues(const odb::dbGCellGrid::GCellData& hor_cong_data,
                           const odb::dbGCellGrid::GCellData& ver_cong_data,
                           double& capacity,
                           double& usage,
                           double& congestion);

  odb::dbDatabase* db_;
  Direction direction_;
  odb::dbTechLayer* layer_;

  MapType type_;
  double max_;
};

}  // namespace grt
