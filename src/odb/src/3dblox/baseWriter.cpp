// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "baseWriter.h"

#include <fstream>
#include <sstream>

#include "utl/Logger.h"

namespace odb {

BaseWriter::BaseWriter(utl::Logger* logger) : logger_(logger)
{
}

void BaseWriter::writeYamlContent(YAML::Node& root, odb::dbDatabase* db)
{
  // Default implementation - just write the header
  // Derived classes should override this method
  YAML::Node header_node = root["Header"];
  writeHeader(header_node, db);
}

void BaseWriter::writeHeader(YAML::Node& header_node, odb::dbDatabase* db)
{
  header_node["version"] = "2.5";
  header_node["unit"] = "micron";
  header_node["precision"] = db->getDbuPerMicron();
}

void BaseWriter::writeYamlToFile(const std::string& filename,
                                 const YAML::Node& root)
{
  std::ofstream out(filename);
  if (!out) {
    if (logger_ != nullptr) {
      logger_->error(utl::ODB, 530, "Writer Error: cannot open {}", filename);
    }
    return;
  }

  out << root;
}

void BaseWriter::logError(const std::string& message)
{
  if (logger_ != nullptr) {
    logger_->error(utl::ODB, 531, "Writer Error: {}", message);
  }
}

std::string BaseWriter::trim(const std::string& str)
{
  size_t first = str.find_first_not_of(' ');
  if (first == std::string::npos) {
    return "";
  }
  size_t last = str.find_last_not_of(' ');
  return str.substr(first, (last - first + 1));
}

}  // namespace odb
