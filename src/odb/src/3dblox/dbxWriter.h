// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <yaml-cpp/yaml.h>

#include <map>
#include <string>
#include <vector>

#include "baseWriter.h"
#include "objects.h"

namespace utl {
class Logger;
}

namespace odb {

class DbxWriter : public BaseWriter
{
 public:
  DbxWriter(utl::Logger* logger);

  void writeFile(const std::string& filename, odb::dbDatabase* db) override;

 protected:
  void writeYamlContent(YAML::Node& root, odb::dbDatabase* db) override;

 private:
  void writeDesign(YAML::Node& design_node, odb::dbDatabase* db);
  void writeDesignExternal(YAML::Node& external_node, odb::dbDatabase* db);
  void writeChipletInsts(YAML::Node& instances_node, odb::dbDatabase* db);
  void writeChipletInst(YAML::Node& instance_node,
                        odb::dbChip* chiplet,
                        odb::dbDatabase* db);
  void writeChipletInstExternal(YAML::Node& external_node,
                                odb::dbChip* chiplet,
                                odb::dbDatabase* db);
  void writeStack(YAML::Node& stack_node, odb::dbDatabase* db);
  void writeStackInstance(YAML::Node& stack_instance_node,
                          odb::dbChip* chiplet,
                          odb::dbDatabase* db);
  void writeConnections(YAML::Node& connections_node, odb::dbDatabase* db);
  void writeConnection(YAML::Node& connection_node, odb::dbDatabase* db);
};

}  // namespace odb