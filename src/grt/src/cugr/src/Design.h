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
  int getLibDBU() const { return lib_dbu_; }

  CostT getUnitLengthWireCost() const { return unit_length_wire_cost_; }
  CostT getUnitViaCost() const { return unit_via_cost_; }
  CostT getUnitLengthShortCost(const int layer_index) const
  {
    return unit_length_short_costs_[layer_index];
  }

  int getNumLayers() const { return layers_.size(); }
  const MetalLayer& getLayer(int layer_index) const
  {
    return layers_[layer_index];
  }

  // For global routing
  const std::vector<std::vector<int>>& getGridlines() const
  {
    return gridlines_;
  }

  const std::vector<CUGRNet>& getAllNets() const { return nets_; }

  void getAllObstacles(std::vector<std::vector<BoxT<int>>>& all_obstacles,
                       bool skip_m1 = true) const;

 private:
  void read();
  void readLayers();
  void readNetlist();
  void readInstanceObstructions();
  void readSpecialNetObstructions(int& num_special_nets);
  void computeGrid();
  void setUnitCosts();

  int lib_dbu_;
  BoxT<int> die_region_;
  std::vector<MetalLayer> layers_;
  std::vector<CUGRNet> nets_;
  std::vector<BoxOnLayer> obstacles_;

  odb::dbBlock* block_;
  odb::dbTech* tech_;
  utl::Logger* logger_;

  // For detailed routing
  CostT unit_length_wire_cost_;
  CostT unit_via_cost_;
  std::vector<CostT> unit_length_short_costs_;

  // For global routing
  int default_gridline_spacing_ = 3000;
  std::vector<std::vector<int>> gridlines_;

  const double weight_wire_length_ = 0.5;
  const double weight_via_number_ = 4.0;
  const double weight_short_area_ = 500.0;
  //
  const int min_routing_layer_ = 1;
  const double cost_logistic_slope_ = 1.0;
  const double max_detour_ratio_
      = 0.25;  // allowed stem length increase to trunk length ratio
  const int target_detour_count_ = 20;
  const double via_multiplier_ = 2.0;
  //
  const double maze_logistic_slope_ = 0.5;
  //
  const double pin_patch_threshold_ = 20.0;
  const int pin_patch_padding_ = 1;
  const double wire_patch_threshold_ = 2.0;
  const double wire_patch_inflation_rate_ = 1.2;
  //
  const bool write_heatmap_ = false;
};

}  // namespace grt