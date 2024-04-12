///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2021, The Regents of the University of California
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

#pragma once

#include "AbstractRoutingCongestionDataSource.h"
#include "gui/heatMap.h"

namespace grt {

class RoutingCongestionDataSource : public gui::GlobalRoutingDataSource,
                                    public AbstractRoutingCongestionDataSource
{
 public:
  RoutingCongestionDataSource(utl::Logger* logger, odb::dbDatabase* db);

  void registerHeatMap() override { gui::HeatMapDataSource::registerHeatMap(); }
  void update() override { gui::HeatMapDataSource::update(); }

 protected:
  virtual bool populateMap() override;
  virtual void combineMapData(bool base_has_value,
                              double& base,
                              const double new_data,
                              const double data_area,
                              const double intersection_area,
                              const double rect_area) override;
  virtual void correctMapScale(HeatMapDataSource::Map& map) override;
  virtual std::string formatValue(double value, bool legend) const override;

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
  double defineValue(int capacity,
                     int usage,
                     double congestion,
                     bool show_data);
  void setCongestionValues(const odb::dbGCellGrid::GCellData& hor_cong_data,
                           const odb::dbGCellGrid::GCellData& ver_cong_data,
                           int& capacity,
                           int& usage,
                           double& congestion);

  odb::dbDatabase* db_;
  Direction direction_;
  odb::dbTechLayer* layer_;

  MapType type_;
  double max_;
};

}  // namespace grt
