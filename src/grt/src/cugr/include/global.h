#pragma once

#include "enum.h"
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

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

using boostPoint = bg::model::point<DBU, 2, bg::cs::cartesian>;
using boostBox = bg::model::box<boostPoint>;
using RTree = bgi::rtree<std::pair<boostBox, int>, bgi::rstar<32>>;

struct Parameters
{
  std::string lef_file;
  std::string def_file;
  std::string out_file;
  int threads = 1;
  //
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

  Parameters(int argc, char* argv[])
  {
    if (argc <= 1) {
      log() << "Too few args..." << std::endl;
      exit(1);
    }
    for (int i = 1; i < argc; i++) {
      if (strcmp(argv[i], "-lef") == 0) {
        lef_file = argv[++i];
      } else if (strcmp(argv[i], "-def") == 0) {
        def_file = argv[++i];
      } else if (strcmp(argv[i], "-output") == 0) {
        out_file = argv[++i];
      } else if (strcmp(argv[i], "-threads") == 0) {
        threads = std::stoi(argv[++i]);
      } else {
        log() << "Unrecognized arg..." << std::endl;
        log() << argv[i] << std::endl;
      }
    }
    log() << "lef file: " << lef_file << std::endl;
    log() << "def file: " << def_file << std::endl;
    log() << "output  : " << out_file << std::endl;
    log() << "threads : " << threads << std::endl;
    log() << std::endl;
  }
};