// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <yaml-cpp/node/node.h>

#include <string>

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

 private:
  void writeYamlContent(YAML::Node& root, odb::dbDatabase* db);
  void writeDesign(YAML::Node& design_node, odb::dbDatabase* db);
  void writeDesignExternal(YAML::Node& external_node, odb::dbDatabase* db);
  void writeChipletInsts(YAML::Node& instances_node, odb::dbDatabase* db);
  void writeChipletInst(YAML::Node& instance_node,
                        odb::dbChipInst* inst,
                        odb::dbDatabase* db);
  void writeChipletInstExternal(YAML::Node& external_node,
                                odb::dbChip* chiplet,
                                odb::dbDatabase* db);
  void writeStack(YAML::Node& stack_node, odb::dbDatabase* db);
  void writeStackInstance(YAML::Node& stack_instance_node,
                          odb::dbChipInst* inst,
                          odb::dbDatabase* db);
  void writeConnections(YAML::Node& connections_node, odb::dbDatabase* db);
  void writeConnection(YAML::Node& connection_node,
                       odb::dbChipConn* conn,
                       odb::dbDatabase* db);
  std::string buildPath(const std::vector<dbChipInst*>& path_insts,
                        odb::dbChipRegionInst* region);
};

}  // namespace odb