// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbvOut.h"

#include <fstream>
#include <string>

#include "objects.h"
#include "utl/Logger.h"

namespace odb {

DbvOut::DbvOut(utl::Logger* logger) : logger_(logger) {}

void DbvOut::writeFile(const std::string& filename, const odb::dbDatabase& db)
{
  YAML::Node root;
  writeYamlContent(root, db);
  
  std::ofstream out(filename);
  if (!out) {
    if (logger_ != nullptr) {
      logger_->error(utl::ODB, 530, "3DBV Writer Error: cannot open {}", filename);
    }
    return;
  }
  
  out << root;
}

void DbvOut::writeYamlContent(YAML::Node& root, const odb::dbDatabase& db)
{
  YAML::Node header_node = root["Header"];

  writeHeader(header_node, db);
  
  YAML::Node chiplets_node = root["ChipletsDef"];

  writeChipletDefs(chiplets_node, db);
}

void DbvOut::writeHeader(YAML::Node& header_node, const odb::dbDatabase& db)
{
  header_node["version"] = "2.5"
  header_node["unit"] = "microns";
  header_node["precision"] = db->getDbuPerMicron();;
}

void DbvOut::writeChipletDefs(YAML::Node& chiplets_node, odb::dbDatabase& db)
{
  for (const auto& db->getChips() : chiplet) {
    YAML::Node chiplet_node = chiplets_node[chiplet.getName()];
    writeChiplet(chiplet_node, chiplet);
  }
}

void DbvOut::writeChiplet(YAML::Node& chiplet_node, const odb::dbChip& chiplet)
{
  chiplet_node["type"] = chiplet.getType();
  chiplet_node["design_width"] = chiplet.getWidth();
  chiplet_node["design_height"] = chiplet.getHeight();
  
  // Offset
  chiplet_node["offset"][0] = chiplet.getOffset().getX();
  chiplet_node["offset"][1] = chiplet.getOffset().geY();
  
  // Seal ring
  chiplet_node["seal_ring"][0] = chiplet.getSealRingWest();
  chiplet_node["seal_ring"][1] = chiplet.getSealRingSouth();
  chiplet_node["seal_ring"][2] = chiplet.getSealRingEast();
  chiplet_node["seal_ring"][3] = chiplet.getSealRingSouth();
  
  // Scribe line
  chiplet_node["scribe_line"][0] = chiplet.getScribeLineWest();
  chiplet_node["scribe_line"][1] = chiplet.getScribeLineSouth();
  chiplet_node["scribe_line"][2] = chiplet.getScribeLineEast();
  chiplet_node["scribe_line"][3] = chiplet.getScribeLineSouth();
  
  chiplet_node["thickness"] = chiplet.getThickness();
  chiplet_node["shrink"] = chiplet.getShrink();
  chiplet_node["tsv"] = chiplet.getTsv();
  
  // External files
  YAML::Node external_node = chiplet_node["external"];
  writeExternal(external_node, );
  
  // Regions
  if (!chiplet.regions.empty()) {
    YAML::Node regions_node = chiplet_node["regions"];
    writeRegions(regions_node, db);
  }
}

void DbvOut::writeRegions(YAML::Node& regions_node, std::map<std::string, ChipletRegion>& regions)
{
  for (const auto& [name, region] : regions) {
    YAML::Node region_node = regions_node[name];
    writeRegion(region_node, region);
  }
}

void DbvOut::writeRegion(YAML::Node& region_node, const ChipletRegion& region)
{
  
}

void DbvOut::writeExternal(YAML::Node& external_node, const ChipletExternal& external)
{
  
}

void DbvOut::writeCoordinates(YAML::Node& coords_node, const std::vector<Coordinate>& coords)
{
  
}

}  // namespace odb