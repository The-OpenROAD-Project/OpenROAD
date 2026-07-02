#pragma once

#include <set>
#include <unordered_map>
#include <vector>

#include "CUGR.h"
#include "GeoTypes.h"
#include "Layers.h"
#include "Netlist.h"
#include "geo.h"
#include "odb/PtrSetMap.h"

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
         const Constants& constants,
         int min_routing_layer,
         int max_routing_layer,
         const odb::PtrSet<odb::dbNet>& clock_nets,
         bool verbose);
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
  // Effective via demand length (enclosure_along * tracks_blocked) for the via
  // between routing layers i and i+1, charged to the lower/upper layer.
  double getViaDemandLengthLower(int i) const
  {
    return via_demand_length_lower_[i];
  }
  double getViaDemandLengthUpper(int i) const
  {
    return via_demand_length_upper_[i];
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

  void updateNet(odb::dbNet* db_net);
  void removeNet(odb::dbNet* db_net);

 private:
  void read();
  void readLayers();
  void readNetlist();
  std::vector<CUGRPin> makeNetPins(odb::dbNet* db_net);
  void readInstanceObstructions();
  int readSpecialNetObstructions();
  void readDesignObstructions();
  void computeGrid();
  void setUnitCosts();
  // Fill via_demand_length_{lower,upper}_ from the tech's default vias.
  void computeViaDemandLengths();
  // Effective demand length of a via pad on one layer: its enclosure extent
  // along the routing direction scaled by the number of tracks it blocks
  // across.
  double viaDemandLength(const MetalLayer& layer, int dx, int dy) const;

  // debug functions
  void printNets() const;
  void printBlockages() const;

  int lib_dbu_;
  BoxT die_region_;
  std::vector<MetalLayer> layers_;
  // Indexed by the via's lower routing-layer; lower_ is charged to layer i,
  // upper_ to layer i+1. Falls back to min_length * via_multiplier when the
  // tech exposes no default via for the pair.
  std::vector<double> via_demand_length_lower_;
  std::vector<double> via_demand_length_upper_;
  std::vector<CUGRNet> nets_;
  std::unordered_map<odb::dbNet*, int> db_net_to_id_;
  std::vector<BoxOnLayer> obstacles_;

  odb::dbBlock* block_;
  odb::dbTech* tech_;
  utl::Logger* logger_;

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
  odb::PtrSet<odb::dbNet> clock_nets_;
  const bool verbose_;
};

}  // namespace grt
