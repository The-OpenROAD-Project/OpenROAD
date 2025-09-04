// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "baseParser.h"

#include <sstream>

#include "objects.h"
#include "utl/Logger.h"

namespace odb {

BaseParser::BaseParser(utl::Logger* logger) : logger_(logger)
{
}

template <typename T>
void BaseParser::extractValue(const YAML::Node& node,
                              const std::string& key,
                              T& value)
{
  try {
    value = node[key].as<T>();
  } catch (const YAML::Exception& e) {
    logError("Error parsing value for " + key + ": " + std::string(e.what()));
  }
}

void BaseParser::parseCoordinate(Coordinate& coord,
                                 const YAML::Node& coord_node)
{
  if (!coord_node.IsSequence() || coord_node.size() != 2) {
    logError("Coordinate must be an array [x, y]");
  }
  try {
    coord.x = coord_node[0].as<double>();
    coord.y = coord_node[1].as<double>();
  } catch (const YAML::Exception& e) {
    logError("Error parsing coordinate: " + std::string(e.what()));
  }
}

void BaseParser::parseCoordinates(std::vector<Coordinate>& coords,
                                  const YAML::Node& coords_node)
{
  coords.clear();
  if (coords_node.IsSequence()) {
    coords.resize(coords_node.size());
    for (int i = 0; i < coords_node.size(); i++) {
      parseCoordinate(coords[i], coords_node[i]);
    }
  }
}

void BaseParser::parseHeader(Header& header, const YAML::Node& header_node)
{
  if (header_node["version"]) {
    extractValue(header_node, "version", header.version);
  }

  if (header_node["unit"]) {
    extractValue(header_node, "unit", header.unit);
  }

  if (header_node["precision"]) {
    extractValue(header_node, "precision", header.precision);
  }

  if (header_node["include"]) {
    extractValue(header_node, "include", header.includes);
  }
}

void BaseParser::parseIncludes(std::vector<std::string>& includes,
                               const std::string& content)
{
  includes.clear();

  std::istringstream stream(content);
  std::string line;

  while (std::getline(stream, line)) {
    if (line.find("#!include") == 0) {
      std::string include_file = line.substr(9);  // Skip "#!include "
      include_file = trim(include_file);
      includes.push_back(include_file);
    }
  }
}

void BaseParser::logError(const std::string& message)
{
  logger_->error(utl::ODB, 520, "Parser Error: {}", message);
}

std::string BaseParser::trim(const std::string& str)
{
  size_t start = str.find_first_not_of(" \t\n\r");
  if (start == std::string::npos) {
    return "";
  }
  size_t end = str.find_last_not_of(" \t\n\r");
  return str.substr(start, end - start + 1);
}

// Explicit template instantiations for common types
template void BaseParser::extractValue<std::string>(const YAML::Node& node,
                                                    const std::string& key,
                                                    std::string& value);
template void BaseParser::extractValue<int>(const YAML::Node& node,
                                            const std::string& key,
                                            int& value);
template void BaseParser::extractValue<double>(const YAML::Node& node,
                                               const std::string& key,
                                               double& value);
template void BaseParser::extractValue<bool>(const YAML::Node& node,
                                             const std::string& key,
                                             bool& value);
template void BaseParser::extractValue<std::vector<double>>(
    const YAML::Node& node,
    const std::string& key,
    std::vector<double>& value);
template void BaseParser::extractValue<std::vector<std::string>>(
    const YAML::Node& node,
    const std::string& key,
    std::vector<std::string>& value);

}  // namespace odb
