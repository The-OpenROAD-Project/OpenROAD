// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

#include <string>

#include "AbstractPowerDensityDataSource.h"
#include "gui/heatMap.h"

namespace sta {
class dbSta;
class Corner;

class PowerDensityDataSource : public gui::RealValueHeatMapDataSource,
                               public AbstractPowerDensityDataSource
{
 public:
  PowerDensityDataSource(dbSta* sta, utl::Logger* logger);

 protected:
  bool populateMap() override;
  void combineMapData(bool base_has_value,
                      double& base,
                      double new_data,
                      double data_area,
                      double intersection_area,
                      double rect_area) override;

 private:
  sta::dbSta* sta_;

  bool include_internal_ = true;
  bool include_leakage_ = true;
  bool include_switching_ = true;

  std::string corner_;

  sta::Corner* getCorner() const;
};

}  // namespace sta
