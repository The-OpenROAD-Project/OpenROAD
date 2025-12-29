// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "baseParser.h"

#include <cstddef>
#include <filesystem>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "objects.h"
#include "utl/Logger.h"
#include "yaml-cpp/yaml.h"
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
    std::vector<std::string> includes;
    extractValue(header_node, "include", includes);
    for (auto& include : includes) {
      resolvePaths(include, header.includes);
    }
  }
}

void BaseParser::parseDefines(std::string& content)
{
  std::map<std::string, std::string> defines;
  std::istringstream stream(content);
  std::string line;
  std::string processed_content;

  while (std::getline(stream, line)) {
    if (line.starts_with("#!define")) {
      std::string define_statement = line.substr(8);
      define_statement = trim(define_statement);

      // Split the define statement into key and value
      size_t space_pos = define_statement.find(' ');
      if (space_pos != std::string::npos) {
        std::string key = define_statement.substr(0, space_pos);
        std::string value = define_statement.substr(space_pos + 1);
        key = trim(key);
        value = trim(value);
        defines[key] = value;
      }
      // Don't include define statements in processed content
    } else {
      std::string resolved_line = line;
      for (const auto& [macro, replacement] : defines) {
        size_t pos = 0;
        while ((pos = resolved_line.find(macro, pos)) != std::string::npos) {
          resolved_line.replace(pos, macro.length(), replacement);
          pos += replacement.length();
        }
      }
      processed_content += resolved_line + "\n";
    }
  }
  content = processed_content;
}

std::string BaseParser::resolvePath(const std::string& path)
{
  std::filesystem::path include_fs_path(path);
  if (include_fs_path.is_absolute()) {
    return include_fs_path.string();
  }
  std::filesystem::path current_fs_path(current_file_path_);
  std::filesystem::path current_dir = current_fs_path.parent_path();
  std::filesystem::path resolved_path = current_dir / include_fs_path;
  return resolved_path.string();
}

namespace {
inline bool matchesPattern(const std::string& filename,
                           const std::string& pattern)
{
  const size_t star_pos = pattern.find('*');
  const std::string prefix = pattern.substr(0, star_pos);
  const std::string suffix = pattern.substr(star_pos + 1);
  if (filename.length() < prefix.length() + suffix.length()) {
    return false;
  }
  return filename.substr(0, prefix.length()) == prefix
         && filename.substr(filename.length() - suffix.length()) == suffix;
}
}  // namespace

void BaseParser::resolvePaths(const std::string& path,
                              std::vector<std::string>& paths)
{
  const std::string resolved_path = resolvePath(path);

  if (resolved_path.find('*') != std::string::npos) {
    std::filesystem::path path_fs(resolved_path);
    std::filesystem::path directory = path_fs.parent_path();
    const std::string filename_pattern = path_fs.filename().string();

    if (!std::filesystem::exists(directory)
        || !std::filesystem::is_directory(directory)) {
      logError("Directory does not exist: " + directory.string());
    }

    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
      if (entry.is_regular_file()) {
        const std::string filename = entry.path().filename().string();
        if (matchesPattern(filename, filename_pattern)) {
          paths.push_back(entry.path().string());
        }
      }
    }
  } else {
    paths.push_back(resolved_path);
  }
}

void BaseParser::logError(const std::string& message)
{
  logger_->error(utl::ODB, 521, "Parser Error: {}", message);
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
