// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "baseWriter.h"

#include <cstddef>
#include <fstream>
#include <string>

#include "odb/db.h"
#include "utl/Logger.h"
#include "yaml-cpp/yaml.h"
namespace odb {

BaseWriter::BaseWriter(utl::Logger* logger, odb::dbDatabase* db)
    : logger_(logger), dbu_per_micron_(db->getDbuPerMicron())
{
}

void BaseWriter::writeHeader(YAML::Node& header_node)
{
  header_node["version"] = "3";
  header_node["unit"] = "micron";
  header_node["precision"] = dbu_per_micron_;
}

void BaseWriter::writeYamlToFile(const std::string& filename,
                                 const YAML::Node& root)
{
  std::ofstream out(filename);
  if (!out) {
    if (logger_ != nullptr) {
      logError("cannot open " + filename);
    }
    return;
  }

  out << root;
}

void BaseWriter::writeCoordinate(YAML::Node& coord_node,
                                 const odb::Point& point)
{
  coord_node.SetStyle(YAML::EmitterStyle::Flow);
  coord_node.push_back(dbuToMicron(point.x()));
  coord_node.push_back(dbuToMicron(point.y()));
}

void BaseWriter::writeCoordinates(YAML::Node& coords_node,
                                  const odb::Rect& rect)
{
  YAML::Node c0, c1, c2, c3;
  writeCoordinate(c0, rect.ll());
  writeCoordinate(c1, rect.lr());
  writeCoordinate(c2, rect.ur());
  writeCoordinate(c3, rect.ul());
  coords_node.push_back(c0);
  coords_node.push_back(c1);
  coords_node.push_back(c2);
  coords_node.push_back(c3);
}

void BaseWriter::logError(const std::string& message)
{
  if (logger_ != nullptr) {
    logger_->error(utl::ODB, 540, "Writer Error: {}", message);
  }
}

std::string BaseWriter::trim(const std::string& str)
{
  std::size_t first = str.find_first_not_of(' ');
  if (first == std::string::npos) {
    return "";
  }
  std::size_t last = str.find_last_not_of(' ');
  return str.substr(first, (last - first + 1));
}

template <typename IntType>
std::string BaseWriter::dbuToMicron(IntType dbu) const
{
  return fmt::format("{:.11g}", dbu / static_cast<double>(dbu_per_micron_));
}

template std::string BaseWriter::dbuToMicron<int>(int dbu) const;
}  // namespace odb
