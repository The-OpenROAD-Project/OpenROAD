// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <string>
#include <unordered_set>

#include "baseWriter.h"

namespace utl {
class Logger;
}
namespace YAML {
class Node;
}

namespace odb {
class dbDatabase;
class dbChip;
class dbChipRegion;

class DbvWriter : public BaseWriter
{
 public:
  DbvWriter(utl::Logger* logger, odb::dbDatabase* db);

  void writeChiplet(const std::string& base_filename, odb::dbChip* top_chip);

 private:
  void writeHeaderIncludes(YAML::Node& header_node,
                           const std::unordered_set<odb::dbChip*>& chips);
  void writeChipletDefs(YAML::Node& chiplets_node,
                        const std::unordered_set<odb::dbChip*>& chips);
  void writeChipletInternal(YAML::Node& chiplet_node, odb::dbChip* chiplet);
  void writeRegions(YAML::Node& regions_node, odb::dbChip* chiplet);
  void writeRegion(YAML::Node& region_node, odb::dbChipRegion* region);
  void writeExternal(YAML::Node& external_node, odb::dbChip* chiplet);
  void writeLef(YAML::Node& external_node, odb::dbChip* chiplet);
  void writeDef(YAML::Node& external_node, odb::dbChip* chiplet);
};

}  // namespace odb
