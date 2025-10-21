// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <yaml-cpp/yaml.h>

#include <map>
#include <string>
#include <vector>

#include "objects.h"
#include "odb/db.h"
#include "odb/dbObject.h"
#include "odb/dbSet.h"
#include "odb/dbShape.h"
#include "odb/dbTransform.h"
#include "odb/dbTypes.h"
#include "odb/defout.h"
#include "odb/gdsout.h"
#include "odb/lefout.h"

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
  void writeHeader(YAML::Node& header_node, odb::dbDatabase* db);
  void writeLef(YAML::Node& external_node,
                odb::dbDatabase* db,
                odb::dbChip* chiplet);
  void writeDef(YAML::Node& external_node,
                odb::dbDatabase* db,
                odb::dbChip* chiplet);
  void logError(const std::string& message);
  std::string trim(const std::string& str);
  void writeYamlToFile(const std::string& filename, const YAML::Node& root);

  // Member variables
  utl::Logger* logger_ = nullptr;
  std::string current_file_path_;
};

}  // namespace odb