// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <yaml-cpp/node/node.h>

#include <string>

#include "baseWriter.h"
#include "chipletHierarchy.h"
#include "odb/db.h"
#include "odb/geom.h"

namespace utl {
class Logger;
}

namespace odb {

class DbvWriter : public BaseWriter
{
 public:
  DbvWriter(utl::Logger* logger);

  void writeFile(const std::string& filename, odb::dbDatabase* db) override;

  void writeChiplet(const std::string& base_filename,
                    odb::dbDatabase* db,
                    odb::dbChip* top_chip);

 private:
  void writeYamlContent(YAML::Node& root, odb::dbDatabase* db);
  void writeChipletToFile(const std::string& filename,
                          odb::dbChip* chiplet,
                          odb::dbDatabase* db,
                          ChipletNode* node);
  void writeChipletDefs(YAML::Node& chiplets_node, odb::dbDatabase* db);
  void writeChipletInternal(YAML::Node& chiplet_node,
                            odb::dbChip* chiplet,
                            odb::dbDatabase* db);
  void writeRegions(YAML::Node& regions_node,
                    odb::dbChip* chiplet,
                    odb::dbDatabase* db);
  void writeRegion(YAML::Node& region_node,
                   odb::dbChipRegion* region,
                   odb::dbDatabase* db);
  void writeExternal(YAML::Node& external_node,
                     odb::dbChip* chiplet,
                     odb::dbDatabase* db);
  void writeCoordinates(YAML::Node& coords_node,
                        const odb::Rect& rect,
                        odb::dbDatabase* db);
  void writeChipDependencies(YAML::Node& header_node, const ChipletNode* node);
};

}  // namespace odb
