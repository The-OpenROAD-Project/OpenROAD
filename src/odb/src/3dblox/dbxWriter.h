// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <yaml-cpp/node/node.h>

#include <string>
#include <vector>

#include "baseWriter.h"
#include "odb/db.h"

namespace utl {
class Logger;
}

namespace odb {

class DbxWriter : public BaseWriter
{
 public:
  DbxWriter(utl::Logger* logger);

  void writeFile(const std::string& filename, odb::dbDatabase* db) override;
  void writeChiplet(odb::dbChip* chiplet);

 private:
  void writeYamlContent(YAML::Node& root, odb::dbChip* chiplet);
  void writeDesign(YAML::Node& design_node, odb::dbChip* chiplet);
  void writeDesignExternal(YAML::Node& external_node, odb::dbChip* chiplet);
  void writeChipletInsts(YAML::Node& instances_node, odb::dbChip* chiplet);
  void writeChipletInst(YAML::Node& instance_node, odb::dbChipInst* inst);
  void writeStack(YAML::Node& stack_node, odb::dbChip* chiplet);
  void writeStackInstance(YAML::Node& stack_instance_node,
                          odb::dbChipInst* inst);
  void writeConnections(YAML::Node& connections_node, odb::dbChip* chiplet);
  void writeConnection(YAML::Node& connection_node, odb::dbChipConn* conn);
  std::string buildPath(const std::vector<dbChipInst*>& path_insts,
                        odb::dbChipRegionInst* region);
};

}  // namespace odb