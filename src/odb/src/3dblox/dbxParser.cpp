// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbxParser.h"

#include <exception>
#include <fstream>
#include <map>
#include <sstream>
#include <string>

#include "baseParser.h"
#include "objects.h"
#include "odb/db.h"
#include "utl/Logger.h"
#include "yaml-cpp/yaml.h"

namespace odb {

DbxParser::DbxParser(utl::Logger* logger) : BaseParser(logger)
{
}

DbxData DbxParser::parseFile(const std::string& filename)
{
  current_file_path_ = filename;
  std::ifstream file(filename);
  if (!file.is_open()) {
    logError("DBX Parser Error: Cannot open file: " + filename);
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string content = buffer.str();
  file.close();

  DbxData data;
  parseDefines(content);

  parseYamlContent(data, content);

  return data;
}

void DbxParser::parseYamlContent(DbxData& data, const std::string& content)
{
  try {
    YAML::Node root = YAML::Load(content);  // throws exception on error

    if (root["Header"]) {
      parseHeader(data.header, root["Header"]);
    }

    if (root["Design"]) {
      parseDesign(data.design, root["Design"]);
    }
    if (root["ChipletInst"]) {
      parseChipletInsts(data.chiplet_instances, root["ChipletInst"]);
    }
    if (root["Stack"]) {
      parseStack(data.chiplet_instances, root["Stack"]);
    }
    if (root["Connection"]) {
      parseConnections(data.connections, root["Connection"]);
    }

  } catch (const YAML::Exception& e) {
    logError("DBX YAML parsing error: " + std::string(e.what()));
  } catch (const std::exception& e) {
    logError("DBX Error parsing YAML content: " + std::string(e.what()));
  }
}

void DbxParser::parseDesign(DesignDef& design, const YAML::Node& design_node)
{
  if (!design_node["name"]) {
    logError("DBX Design name is required");
  }
  extractValue(design_node, "name", design.name);

  if (design_node["external"]) {
    parseDesignExternal(design.external, design_node["external"]);
  }
}

void DbxParser::parseDesignExternal(DesignExternal& external,
                                    const YAML::Node& external_node)
{
  if (external_node["verilog_file"]) {
    extractValue(external_node, "verilog_file", external.verilog_file);
    external.verilog_file = resolvePath(external.verilog_file);
  }
}

void DbxParser::parseChipletInsts(std::map<std::string, ChipletInst>& instances,
                                  const YAML::Node& instances_node)
{
  instances.clear();

  for (const auto& instance_pair : instances_node) {
    ChipletInst instance;
    try {
      instance.name = instance_pair.first.as<std::string>();
    } catch (const YAML::Exception& e) {
      logError("DBX Error parsing chiplet instance name: "
               + std::string(e.what()));
      continue;
    }

    const YAML::Node& instance_node = instance_pair.second;
    parseChipletInst(instance, instance_node);
    instances[instance.name] = instance;
  }
}

void DbxParser::parseChipletInst(ChipletInst& instance,
                                 const YAML::Node& instance_node)
{
  if (!instance_node["reference"]) {
    logError("DBX ChipletInst reference is required for instance "
             + instance.name);
  }
  extractValue(instance_node, "reference", instance.reference);

  if (instance_node["external"]) {
    parseChipletInstExternal(instance.external, instance_node["external"]);
  }
}

void DbxParser::parseChipletInstExternal(ChipletInstExternal& external,
                                         const YAML::Node& external_node)
{
  if (external_node["verilog_file"]) {
    extractValue(external_node, "verilog_file", external.verilog_file);
    external.verilog_file = resolvePath(external.verilog_file);
  }

  if (external_node["sdc_file"]) {
    extractValue(external_node, "sdc_file", external.sdc_file);
    external.sdc_file = resolvePath(external.sdc_file);
  }

  if (external_node["def_file"]) {
    extractValue(external_node, "def_file", external.def_file);
    external.def_file = resolvePath(external.def_file);
  }
}

void DbxParser::parseStack(std::map<std::string, ChipletInst>& instances,
                           const YAML::Node& stack_node)
{
  for (const auto& stack_pair : stack_node) {
    std::string instance_name;
    try {
      instance_name = stack_pair.first.as<std::string>();
    } catch (const YAML::Exception& e) {
      logError("DBX Error parsing stack instance name: "
               + std::string(e.what()));
    }

    // Find the corresponding chiplet instance
    auto it = instances.find(instance_name);
    if (it == instances.end()) {
      logError("DBX Stack instance '" + instance_name
               + "' not found in ChipletInst");
    }

    const YAML::Node& stack_instance_node = stack_pair.second;
    parseStackInstance(it->second, stack_instance_node);
  }
}

void DbxParser::parseStackInstance(ChipletInst& instance,
                                   const YAML::Node& stack_instance_node)
{
  // Parse location [x, y]
  if (stack_instance_node["loc"]) {
    parseCoordinate(instance.loc, stack_instance_node["loc"]);
  } else {
    logError("DBX Stack location is required for instance " + instance.name);
  }

  if (!stack_instance_node["z"]) {
    logError("DBX Stack z is required for instance " + instance.name);
  }
  extractValue(stack_instance_node, "z", instance.z);

  // Parse orientation
  if (!stack_instance_node["orient"]) {
    logError("DBX Stack orientation is required for instance " + instance.name);
  }
  extractValue(stack_instance_node, "orient", instance.orient);
}

void DbxParser::parseConnections(std::map<std::string, Connection>& connections,
                                 const YAML::Node& connections_node)
{
  connections.clear();

  for (const auto& connection_pair : connections_node) {
    Connection connection;
    try {
      connection.name = connection_pair.first.as<std::string>();
    } catch (const YAML::Exception& e) {
      logError("DBX Error parsing connection name: " + std::string(e.what()));
      continue;
    }

    const YAML::Node& connection_node = connection_pair.second;
    parseConnection(connection, connection_node);
    connections[connection.name] = connection;
  }
}

void DbxParser::parseConnection(Connection& connection,
                                const YAML::Node& connection_node)
{
  if (!connection_node["top"]) {
    logError("DBX Connection top is required for connection "
             + connection.name);
  }
  extractValue(connection_node, "top", connection.top);

  // Parse bot region (required, can be "~")
  if (!connection_node["bot"]) {
    logError("DBX Connection bot is required for connection "
             + connection.name);
  }
  extractValue(connection_node, "bot", connection.bot);

  if (connection_node["thickness"]) {
    extractValue(connection_node, "thickness", connection.thickness);
  }
}

}  // namespace odb
