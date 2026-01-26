// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <string>

namespace utl {
class Logger;
}

namespace YAML {
class Node;
}
namespace odb {
class dbDatabase;
class Rect;
class Point;

class BaseWriter
{
 public:
  BaseWriter(utl::Logger* logger, odb::dbDatabase* db);
  virtual ~BaseWriter() = default;

 protected:
  // Common YAML content writing
  void writeHeader(YAML::Node& header_node);
  void writeCoordinates(YAML::Node& coords_node, const odb::Rect& rect);
  void writeCoordinate(YAML::Node& coord_node, const odb::Point& point);
  void logError(const std::string& message);
  std::string trim(const std::string& str);
  void writeYamlToFile(const std::string& filename, const YAML::Node& root);
  template <typename IntType>
  std::string dbuToMicron(IntType dbu) const;

  // Member variables
  utl::Logger* logger_ = nullptr;
  std::string current_dir_path_;
  unsigned int dbu_per_micron_ = 0;
};

}  // namespace odb