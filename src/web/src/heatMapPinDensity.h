// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

#include <string>

#include "odb/dbBlockCallBackObj.h"
#include "web/heatMap.h"

namespace web {

class PinDensityDataSource : public RealValueHeatMapDataSource,
                             public odb::dbBlockCallBackObj
{
 public:
  PinDensityDataSource(utl::Logger* logger);

  // Pin density is a property of a block's core, so a source bound to a chip
  // that has no block (the root of a 3DBlox stack) has nothing to show.
  // setupMap() reads empty bounds as exactly that.
  odb::Rect getBounds() const override
  {
    odb::dbBlock* block = getBlock();
    return block != nullptr ? block->getCoreArea() : odb::Rect();
  }

  std::string getSelectionFilterLabel() const override
  {
    return "Only use selected instances";
  }

  void onShow() override;
  void onHide() override;
  double getGridSizeMinimumValue() const override { return 0.1; }

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
};

}  // namespace web
