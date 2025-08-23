#pragma once

#include "geo.h"

// STL libraries
#include <csignal>
#include <iostream>
#include <string>
#include <vector>
// #include <unordered_map>
// #include <unordered_set>
#include <bitset>
#include <fstream>
#include <mutex>
#include <set>
#include <sstream>
#include <thread>
#include <tuple>

// Boost libraries
#include <boost/foreach.hpp>
#include <boost/functional/hash.hpp>
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/index/rtree.hpp>
#include <boost/icl/split_interval_map.hpp>

#include "odb/geom.h"

namespace odb {
class dbDatabase;
}  // namespace odb

namespace stt {
class SteinerTreeBuilder;
}  // namespace stt

namespace utl {
class Logger;
}  // namespace utl

namespace grt {

class Design;
class GridGraph;
class GRNet;

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

using boostPoint = bg::model::point<int, 2, bg::cs::cartesian>;
using boostBox = bg::model::box<boostPoint>;
using RTree = bgi::rtree<std::pair<boostBox, int>, bgi::rstar<32>>;

class CUGR
{
 public:
  CUGR(odb::dbDatabase* db,
       utl::Logger* log,
       stt::SteinerTreeBuilder* stt_builder);
  void init();
  void route();
  void write(std::string guide_file = "");

 private:
  void sortNetIndices(std::vector<int>& netIndices) const;
  void getGuides(const GRNet* net,
                 std::vector<std::pair<int, grt::BoxT<int>>>& guides);
  void printStatistics() const;

  Design* design_;
  GridGraph* grid_graph_;
  std::vector<GRNet*> gr_nets_;

  odb::dbDatabase* db_;
  utl::Logger* logger_;
  stt::SteinerTreeBuilder* stt_builder_;

  int area_of_pin_patches_;
  int area_of_wire_patches_;

  // constants
  const double weight_wire_length_ = 0.5;
  const double weight_via_number_ = 4.0;
  const double weight_short_area_ = 500.0;

  const int min_routing_layer_ = 1;
  const double cost_logistic_slope_ = 1.0;
  const double max_detour_ratio_
      = 0.25;  // allowed stem length increase to trunk length ratio
  const int target_detour_count_ = 20;
  const double via_multiplier_ = 2.0;

  const double maze_logistic_slope_ = 0.5;

  const double pin_patch_threshold_ = 20.0;
  const int pin_patch_padding_ = 1;
  const double wire_patch_threshold_ = 2.0;
  const double wire_patch_inflation_rate_ = 1.2;

  const bool write_heatmap_ = false;
};

}  // namespace grt