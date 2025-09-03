// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <yaml-cpp/yaml.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "objects.h"

namespace utl {
class Logger;
}

namespace odb {

class dbDatabase;

class DbvParser
{
 public:
  DbvParser(utl::Logger* logger);
  ~DbvParser();

  DbvData parseFile(const std::string& filename);

 private:
  void parseYamlContent(DbvData& data, const std::string& content);
  void parseHeader(DBVHeader& header, const YAML::Node& header_node);
  void parseChipletDefs(std::map<std::string, ChipletDef>& chiplet_defs,
                        const YAML::Node& chiplets_node);
  void parseChiplet(ChipletDef& chiplet, const YAML::Node& chiplet_node);
  void parseRegions(std::map<std::string, ChipletRegion>& regions,
                    const YAML::Node& regions_node);
  void parseRegion(ChipletRegion& region, const YAML::Node& region_node);
  void parseCoordinates(std::vector<Coordinate>& coords,
                        const YAML::Node& coords_node);

  // YAML helper methods
  template <typename T>
  void extractValue(const YAML::Node& node, const std::string& key, T& value);

  void logError(const std::string& message);

  utl::Logger* logger_ = nullptr;
};

}  // namespace odb
