// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#pragma once

#include <string>

#include "gui/heatMap.h"
#include "odb/db.h"

namespace sta {
class Sta;
class Scene;
}  // namespace sta

namespace psm {
class PDNSim;

class IRDropDataSource : public gui::RealValueHeatMapDataSource
{
 public:
  IRDropDataSource(PDNSim* psm, sta::Sta* sta, utl::Logger* logger);

  void setBlock(odb::dbBlock* block) override;

  void setNet(odb::dbNet* net) { net_ = net; }
  void setCorner(sta::Scene* corner) { corner_ = corner; }

 protected:
  bool populateMap() override;
  void combineMapData(bool base_has_value,
                      double& base,
                      double new_data,
                      double data_area,
                      double intersection_area,
                      double rect_area) override;

  void determineMinMax(const gui::HeatMapDataSource::Map& map) override;

 private:
  void ensureLayer();
  void ensureCorner();
  void ensureNet();
  void setLayer(const std::string& name);
  void setCorner(const std::string& name);
  void setNet(const std::string& name);

  psm::PDNSim* psm_;
  sta::Sta* sta_;
  odb::dbTech* tech_ = nullptr;
  odb::dbTechLayer* layer_ = nullptr;
  odb::dbNet* net_ = nullptr;
  sta::Scene* corner_ = nullptr;
};

}  // namespace psm
