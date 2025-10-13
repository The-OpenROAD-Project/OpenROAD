// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <map>
#include <string>
#include <vector>

#include "baseWriter.h"
#include "objects.h"

namespace utl {
class Logger;
}

namespace odb {

class DbvWriter : public BaseWriter
{
 public:
  DbvWriter(utl::Logger* logger);

  void writeFile(const std::string& filename, odb::dbDatabase* db) override;

 protected:
  void writeYamlContent(YAML::Node& root, odb::dbDatabase* db) override;

 private:
  void writeChipletDefs(YAML::Node& chiplets_node, odb::dbDatabase* db);
  void writeChiplet(YAML::Node& chiplet_node,
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
};

}  // namespace odb
