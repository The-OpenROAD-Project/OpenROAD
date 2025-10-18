// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <map>
#include <string>
#include <vector>

#include "baseWriter.h"
#include "chipletHierarchy.h"
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

  // Hierarchical writing method
  void writeHierarchicalDbv(const std::string& base_filename,
                            odb::dbDatabase* db);

 protected:
  void writeYamlContent(YAML::Node& root, odb::dbDatabase* db) override;

 private:
  void writeLevelToFile(const std::string& filename,
                        const std::vector<odb::dbChip*>& chiplets,
                        odb::dbDatabase* db);
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
  void writeLevelDependencies(YAML::Node& header_node,
                              const std::vector<odb::dbChip*>& chiplets,
                              odb::dbDatabase* db);

  // Helper methods for file naming and path management
  std::string generateLevelFilename(const std::string& base_filename,
                                    int level);
  std::string getBaseName(const std::string& filename);
  std::string getDirectory(const std::string& filename);
  std::string getDependencyFilename(const std::string& base_filename,
                                    int dependency_level);
};

}  // namespace odb
