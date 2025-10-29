// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <map>
#include <string>

#include "baseParser.h"
#include "objects.h"
#include "yaml-cpp/yaml.h"

namespace odb {

class dbDatabase;

class DbxParser : public BaseParser
{
 public:
  DbxParser(utl::Logger* logger);

  DbxData parseFile(const std::string& filename);

 private:
  void parseYamlContent(DbxData& data, const std::string& content);
  void parseDesign(DesignDef& design, const YAML::Node& design_node);
  void parseDesignExternal(DesignExternal& external,
                           const YAML::Node& external_node);
  void parseChipletInsts(std::map<std::string, ChipletInst>& instances,
                         const YAML::Node& instances_node);
  void parseChipletInst(ChipletInst& instance, const YAML::Node& instance_node);
  void parseChipletInstExternal(ChipletInstExternal& external,
                                const YAML::Node& external_node);
  void parseStack(std::map<std::string, ChipletInst>& instances,
                  const YAML::Node& stack_node);
  void parseStackInstance(ChipletInst& instance,
                          const YAML::Node& stack_instance_node);
  void parseConnections(std::map<std::string, Connection>& connections,
                        const YAML::Node& connections_node);
  void parseConnection(Connection& connection,
                       const YAML::Node& connection_node);
};

}  // namespace odb
