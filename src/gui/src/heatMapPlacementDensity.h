// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#pragma once

#include "gui/heatMap.h"
#include "odb/dbBlockCallBackObj.h"

namespace gui {

class PlacementDensityDataSource : public HeatMapDataSource,
                                   public odb::dbBlockCallBackObj
{
 public:
  PlacementDensityDataSource(utl::Logger* logger);

  odb::Rect getBounds() const override { return getBlock()->getCoreArea(); }

  void onShow() override;
  void onHide() override;

  // from dbBlockCallBackObj API
  void inDbInstCreate(odb::dbInst*) override;
  void inDbInstCreate(odb::dbInst*, odb::dbRegion*) override;
  void inDbInstDestroy(odb::dbInst*) override;
  void inDbInstPlacementStatusBefore(odb::dbInst*,
                                     const odb::dbPlacementStatus&) override;
  void inDbInstSwapMasterBefore(odb::dbInst*, odb::dbMaster*) override;
  void inDbInstSwapMasterAfter(odb::dbInst*) override;
  void inDbPreMoveInst(odb::dbInst*) override;
  void inDbPostMoveInst(odb::dbInst*) override;

 protected:
  bool populateMap() override;
  void combineMapData(bool base_has_value,
                      double& base,
                      double new_data,
                      double data_area,
                      double intersection_area,
                      double rect_area) override;

  bool destroyMapOnNotVisible() const override { return true; }

 private:
  bool include_taps_{true};
  bool include_filler_{false};
  bool include_io_{false};
};

}  // namespace gui
