// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

#include "AbstractRoutingCongestionDataSource.h"
#include "grt/GlobalRouter.h"
#include "grt/Rudy.h"
#include "gui/heatMap.h"
#include "odb/dbBlockCallBackObj.h"
#include "odb/util.h"

namespace odb {
class dbDatabase;
}

namespace grt {

class RUDYDataSource : public gui::GlobalRoutingDataSource,
                       public AbstractRoutingCongestionDataSource,
                       public odb::dbBlockCallBackObj
{
 public:
  RUDYDataSource(utl::Logger* logger,
                 grt::GlobalRouter* grouter,
                 odb::dbDatabase* db);

  void registerHeatMap() override { gui::HeatMapDataSource::registerHeatMap(); }
  void update() override { gui::HeatMapDataSource::update(); }

  void onShow() override;
  void onHide() override;

  // from dbBlockCallBackObj API
  void inDbInstCreate(odb::dbInst*) override;
  void inDbInstCreate(odb::dbInst*, odb::dbRegion*) override;
  void inDbInstDestroy(odb::dbInst*) override;
  void inDbInstPlacementStatusBefore(odb::dbInst*,
                                     const odb::dbPlacementStatus&) override;
  void inDbInstSwapMasterAfter(odb::dbInst*) override;
  void inDbPostMoveInst(odb::dbInst*) override;
  void inDbITermPostDisconnect(odb::dbITerm*, odb::dbNet*) override;
  void inDbITermPostConnect(odb::dbITerm*) override;
  void inDbBTermPostConnect(odb::dbBTerm*) override;
  void inDbBTermPostDisConnect(odb::dbBTerm*, odb::dbNet*) override;

 protected:
  void populateXYGrid() override;
  bool populateMap() override;
  void combineMapData(bool base_has_value,
                      double& base,
                      const double new_data,
                      const double data_area,
                      const double intersection_area,
                      const double rect_area) override;

 private:
  grt::GlobalRouter* grouter_;
  odb::dbDatabase* db_;
  grt::Rudy* rudy_;
};

}  // namespace grt
