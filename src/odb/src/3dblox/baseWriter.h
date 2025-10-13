// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <yaml-cpp/yaml.h>

#include <map>
#include <string>
#include <vector>

#include "objects.h"
#include "odb/db.h"
#include "odb/dbTypes.h"

namespace utl {
class Logger;
}

namespace odb {

class BaseWriter
{
 public:
  BaseWriter(utl::Logger* logger);
  virtual ~BaseWriter() = default;
  virtual void writeFile(const std::string& filename, odb::dbDatabase* db) = 0;

 protected:
  // Common YAML content writing
  virtual void writeYamlContent(YAML::Node& root, odb::dbDatabase* db);
  void writeHeader(YAML::Node& header_node, odb::dbDatabase* db);

  // Utility methods
  void logError(const std::string& message);
  std::string trim(const std::string& str);
  void writeYamlToFile(const std::string& filename, const YAML::Node& root);

  // Member variables
  utl::Logger* logger_ = nullptr;
  std::string current_file_path_;
};

}  // namespace odb