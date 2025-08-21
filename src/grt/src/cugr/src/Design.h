#pragma once

#include "GeoTypes.h"
#include "Layers.h"
#include "Netlist.h"

namespace odb {
class dbDatabase;
class dbBlock;
class dbTech;
}  // namespace odb

namespace utl {
class Logger;
}  // namespace utl

namespace grt {

using CostT = double;

class Design
{
 public:
  Design(odb::dbDatabase* db, utl::Logger* logger);
  int getLibDBU() const { return libDBU; }

  CostT getUnitLengthWireCost() const { return unit_length_wire_cost; }
  CostT getUnitViaCost() const { return unit_via_cost; }
  CostT getUnitLengthShortCost(const int layerIndex) const
  {
    return unit_length_short_costs[layerIndex];
  }

  int getNumLayers() const { return layers_.size(); }
  const MetalLayer& getLayer(int layerIndex) const
  {
    return layers_[layerIndex];
  }

  // For global routing
  const std::vector<std::vector<int>>& getGridlines() const
  {
    return gridlines;
  }
  const std::vector<CUGRNet>& getAllNets() const { return nets_; }
  void getAllObstacles(std::vector<std::vector<BoxT<int>>>& allObstacles,
                       bool skipM1 = true) const;

 private:
  void read();
  void readLayers();
  void readNetlist();
  void readInstanceObstructions();
  void readSpecialNetObstructions(int& numSpecialNets);
  void computeGrid();
  void setUnitCosts();

  int libDBU;
  BoxT<int> dieRegion;
  std::vector<MetalLayer> layers_;
  std::vector<CUGRNet> nets_;
  std::vector<BoxOnLayer> obstacles_;

  odb::dbBlock* block_;
  odb::dbTech* tech_;
  utl::Logger* logger_;

  // For detailed routing
  CostT unit_length_wire_cost;
  CostT unit_via_cost;
  std::vector<CostT> unit_length_short_costs;

  // For global routing
  int defaultGridlineSpacing = 3000;
  std::vector<std::vector<int>> gridlines;

  const double weight_wire_length = 0.5;
  const double weight_via_number = 4.0;
  const double weight_short_area = 500.0;
  //
  const int min_routing_layer = 1;
  const double cost_logistic_slope = 1.0;
  const double max_detour_ratio
      = 0.25;  // allowed stem length increase to trunk length ratio
  const int target_detour_count = 20;
  const double via_multiplier = 2.0;
  //
  const double maze_logistic_slope = 0.5;
  //
  const double pin_patch_threshold = 20.0;
  const int pin_patch_padding = 1;
  const double wire_patch_threshold = 2.0;
  const double wire_patch_inflation_rate = 1.2;
  //
  const bool write_heatmap = false;
};

}  // namespace grt