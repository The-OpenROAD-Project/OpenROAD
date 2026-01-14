// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <map>
#include <string>
#include <vector>

#include "baseParser.h"
#include "objects.h"
#include "yaml-cpp/yaml.h"

namespace odb {

class dbDatabase;

class DbvParser : public BaseParser
{
 public:
  DbvParser(utl::Logger* logger);

  DbvData parseFile(const std::string& filename);

 private:
  void parseYamlContent(DbvData& data, const std::string& content);
  void parseChipletDefs(std::map<std::string, ChipletDef>& chiplet_defs,
                        const YAML::Node& chiplets_node);
  void parseChiplet(ChipletDef& chiplet, const YAML::Node& chiplet_node);
  void parseRegions(std::map<std::string, ChipletRegion>& regions,
                    const YAML::Node& regions_node);
  void parseRegion(ChipletRegion& region, const YAML::Node& region_node);
};

}  // namespace odb
