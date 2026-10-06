// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#pragma once

#include <optional>
#include <string>

#include "odb/dbBlockCallBackObj.h"
#include "web/heatMap.h"

namespace web {

class PlacementDensityDataSource : public HeatMapDataSource,
                                   public odb::dbBlockCallBackObj
{
 public:
  PlacementDensityDataSource(utl::Logger* logger);

  // See PinDensityDataSource::getBounds(): no block means no core to measure.
  odb::Rect getBounds() const override
  {
    odb::dbBlock* block = getBlock();
    return block != nullptr ? block->getCoreArea() : odb::Rect();
  }

  std::string getSelectionFilterLabel() const override
  {
    return "Only use selected instances";
  }

  // Global placement bins go below a micron on advanced nodes, so the size it
  // reports has to stay representable instead of being clamped.
  double getGridSizeMinimumValue() const override { return 0.1; }
  void restoreSettings(const Renderer::Settings& settings) override;

  void onShow() override;
  void onHide() override;

  // from dbBlockCallBackObj API
  void inDbInstCreate(odb::dbInst*) override;
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

  void populateXYGrid() override;

 private:
  // Bin size, in DBU, published by global placement under the given block
  // property. Empty until global placement runs.
  std::optional<int> getPlacementBinSize(const char* name) const;

  // Grid last taken from global placement; while the grid still matches it
  // the user has not picked their own size, so it may follow placement.
  double placement_grid_x_;
  double placement_grid_y_;

  bool include_taps_{true};
  bool include_filler_{false};
  bool include_io_{false};
};

}  // namespace web
