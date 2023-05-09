// Copyright 2019-2023 The Regents of the University of California, Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

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
