// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "baseWriter.h"

#include <fstream>
#include <sstream>

#include "utl/Logger.h"
#include "utl/ScopedTemporaryFile.h"

namespace odb {

BaseWriter::BaseWriter(utl::Logger* logger) : logger_(logger)
{
}

void BaseWriter::writeHeader(YAML::Node& header_node, odb::dbDatabase* db)
{
  header_node["version"] = "2.5"; // TODO: add version to DB
  header_node["unit"] = "micron";
  header_node["precision"] = db->getDbuPerMicron();
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

void BaseWriter::writeLef(YAML::Node& external_node,
                          odb::dbDatabase* db,
                          odb::dbChip* chiplet)
{
  auto libs = db->getLibs();
  int num_libs = libs.size();
  if (num_libs > 0) {
    if (num_libs > 1) {
      logger_->info(
          utl::ODB,
          535,
          "More than one lib exists, multiple files will be written.");
    }
    int cnt = 0;
    for (auto lib : libs) {
      std::string name(std::string(lib->getName()) + ".lef");
      if (cnt > 0) {
        auto pos = name.rfind('.');
        if (pos != std::string::npos) {
          name.insert(pos, "_" + std::to_string(cnt));
        } else {
          name += "_" + std::to_string(cnt);
        }
        utl::OutStreamHandler stream_handler(name.c_str());
        odb::lefout lef_writer(logger_, stream_handler.getStream());
        lef_writer.writeLib(lib);
      } else {
        utl::OutStreamHandler stream_handler(name.c_str());
        odb::lefout lef_writer(logger_, stream_handler.getStream());
        lef_writer.writeTechAndLib(lib);
      }
      YAML::Node list_node;
      list_node.SetStyle(YAML::EmitterStyle::Flow);
      list_node.push_back(name.c_str());
      if (name.find("_tech") != std::string::npos) {
        external_node["APR_tech_file"] = list_node;
      } else {
        external_node["LEF_file"] = list_node;
      }
      ++cnt;
    }
  } else if (db->getTech()) {
    utl::OutStreamHandler stream_handler(
        (std::string(chiplet->getName()) + ".lef").c_str());
    odb::lefout lef_writer(logger_, stream_handler.getStream());
    lef_writer.writeTech(db->getTech());
  }
}

void BaseWriter::writeDef(YAML::Node& external_node,
                          odb::dbDatabase* db,
                          odb::dbChip* chiplet)
{
  odb::DefOut def_writer(logger_);
  auto block = chiplet->getBlock();
  def_writer.writeBlock(block,
                        (std::string(chiplet->getName()) + ".def").c_str());
  external_node["DEF_file"] = std::string(chiplet->getName()) + ".def";
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
