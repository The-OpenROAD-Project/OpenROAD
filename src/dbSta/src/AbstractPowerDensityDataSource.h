// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

namespace sta {

class AbstractPowerDensityDataSource
{
 public:
  virtual ~AbstractPowerDensityDataSource() = default;
  virtual bool populateMap() = 0;
  virtual void combineMapData(bool base_has_value,
                              double& base,
                              double new_data,
                              double data_area,
                              double intersection_area,
                              double rect_area)
      = 0;
};

}  // namespace sta
