// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <string>
#include <vector>

#include "yaml-cpp/yaml.h"

namespace utl {
class Logger;
}

namespace odb {

struct Coordinate;
struct Header;

class BaseParser
{
 public:
  BaseParser(utl::Logger* logger);
  virtual ~BaseParser() = default;

 protected:
  // YAML helper methods
  template <typename T>
  void extractValue(const YAML::Node& node, const std::string& key, T& value);
  template <typename T>
  void extractValue(const YAML::Node& node, T& value);
  void parseCoordinate(Coordinate& coord, const YAML::Node& coord_node);
  void parseCoordinates(std::vector<Coordinate>& coords,
                        const YAML::Node& coords_node);
  void parseHeader(Header& header, const YAML::Node& header_node);
  void parseDefines(std::string& content);
  std::string resolvePath(const std::string& path);
  void resolvePaths(const std::string& path, std::vector<std::string>& paths);
  // Extracts a single resolved path from a YAML list-of-strings under
  // `parent[key]`. Returns an empty string if the key is absent. Emits a
  // parser error when more than one entry is provided (cardinality > 1 is
  // currently unsupported per the 3DBlox external spec). `context` is woven
  // into the error message to identify the owning element.
  std::string extractSinglePathFromList(const YAML::Node& parent,
                                        const std::string& key,
                                        const std::string& context);

  // Utility methods
  void logError(const std::string& message);
  std::string trim(const std::string& str);

  // Member variables
  utl::Logger* logger_ = nullptr;
  std::string current_file_path_;
};

}  // namespace odb
