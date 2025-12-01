#pragma once

#include <set>
#include <vector>

#include "CUGR.h"
#include "GeoTypes.h"
#include "Layers.h"
#include "Netlist.h"
#include "geo.h"

namespace odb {
class dbBlock;
class dbDatabase;
class dbITerm;
class dbNet;
class dbTech;
}  // namespace odb

namespace sta {
class dbNetwork;
class dbSta;
}  // namespace sta

namespace utl {
class Logger;
}  // namespace utl

namespace grt {

using CostT = double;

class Design
{
 public:
  Design(odb::dbDatabase* db,
         utl::Logger* logger,
         sta::dbSta* sta,
         const Constants& constants,
         int min_routing_layer,
         int max_routing_layer,
         const std::set<odb::dbNet*>& clock_nets);
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

  void getAllObstacles(std::vector<std::vector<BoxT>>& all_obstacles,
                       bool skip_m1 = true) const;

  int getGridlineSize() const { return default_gridline_spacing_; }

  BoxT getDieRegion() const { return die_region_; }

 private:
  void read();
  void readLayers();
  void readNetlist();
  void readInstanceObstructions();
  int readSpecialNetObstructions();
  void readDesignObstructions();
  void computeGrid();
  void setUnitCosts();

  // debug functions
  void printNets() const;
  void printBlockages() const;

  int lib_dbu_;
  BoxT die_region_;
  std::vector<MetalLayer> layers_;
  std::vector<CUGRNet> nets_;
  std::vector<BoxOnLayer> obstacles_;

  odb::dbBlock* block_;
  odb::dbTech* tech_;
  utl::Logger* logger_;
  sta::dbSta* sta_;

  // For detailed routing
  CostT unit_length_wire_cost_;
  CostT unit_via_cost_;
  std::vector<CostT> unit_length_short_costs_;

  // For global routing
  int default_gridline_spacing_;
  std::vector<std::vector<int>> gridlines_;

  const Constants constants_;
  const int min_routing_layer_;
  const int max_routing_layer_;
  std::set<odb::dbNet*> clock_nets_;
};

}  // namespace grt
