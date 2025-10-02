// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <yaml-cpp/yaml.h>

#include <map>
#include <string>
#include <vector>

#include "objects.h"

namespace utl {
class Logger;
}

namespace odb {

struct odb::dbDatabase;

class DbvOut
{
 public:
  DbvOut(utl::Logger* logger);

  // Write to a .3dbv YAML file
  void writeFile(const std::string& filename, const odb::dbDatabase& db);

 private:
  void writeYamlContent(YAML::Node& root, const odb::dbDatabase& db);
  void writeHeader(YAML::Node& header_node, const Header& header);
  void writeChipletDefs(YAML::Node& chiplets_node, const std::map<std::string, ChipletDef>& chiplet_defs);
  void writeChiplet(YAML::Node& chiplet_node, const ChipletDef& chiplet);
  void writeRegions(YAML::Node& regions_node, const std::map<std::string, ChipletRegion>& regions);
  void writeRegion(YAML::Node& region_node, const ChipletRegion& region);
  void writeExternal(YAML::Node& external_node, const ChipletExternal& external);
  void writeCoordinates(YAML::Node& coords_node, const std::vector<Coordinate>& coords);

  utl::Logger* logger_ = nullptr;
};

}  // namespace odb
