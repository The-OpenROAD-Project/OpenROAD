// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbvOut.h"

#include <fstream>
#include <string>

#include "objects.h"
#include "utl/Logger.h"

namespace odb {

DbvOut::DbvOut(utl::Logger* logger) : logger_(logger)
{
}

void DbvOut::writeFile(const std::string& filename, const odb::dbDatabase& db)
{
  YAML::Node root;
  writeYamlContent(root, db);

  std::ofstream out(filename);
  if (!out) {
    if (logger_ != nullptr) {
      logger_->error(
          utl::ODB, 530, "3DBV Writer Error: cannot open {}", filename);
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

  // writeChipletDefs(chiplets_node, db);
}

void DbvOut::writeHeader(YAML::Node& header_node, const odb::dbDatabase& db)
{
  header_node["version"] = "2.5";
  header_node["unit"] = "microns";
  header_node["precision"] = db->getDbuPerMicron();
}

void DbvOut::writeChipletDefs(YAML::Node& chiplets_node, odb::dbDatabase& db)
{
  for (const auto & db->getChips() : chiplet) {
    YAML::Node chiplet_node = chiplets_node[chiplet.getName()];
    writeChiplet(chiplet_node, chiplet);
  }
}

void DbvOut::writeChiplet(YAML::Node& chiplet_node, const odb::dbChip& chiplet)
{
  auto type = chiplet->getType();
  auto width = chiplet->getWidth();
  auto height = chiplet->getHeight();

  // Offset
  auto offset_x = chiplet->getOffset().getX();
  auto offset_y = chiplet->getOffset().geY();

  // Seal ring
  auto seal_ring_west = chiplet->getSealRingWest();
  auto seal_ring_south = chiplet->getSealRingSouth();
  auto seal_ring_east = chiplet->getSealRingEast();
  auto seal_ring_south = chiplet->getSealRingSouth();

  // Scribe line
  auto scribe_line_west = chiplet->getScribeLineWest();
  auto scribe_line_south = chiplet->getScribeLineSouth();
  auto scribe_line_east = chiplet->getScribeLineEast();
  auto scribe_linesouth = chiplet->getScribeLineSouth();

  chiplet_node["thickness"] = chiplet->getThickness();
  chiplet_node["shrink"] = chiplet->getShrink();
  chiplet_node["tsv"] = chiplet->getTsv();

  // External files
  YAML::Node external_node = chiplet_node["external"];
  // writeExternal(external_node, );

  // Regions
  // if (!chiplet.regions.empty()) {
  //   YAML::Node regions_node = chiplet_node["regions"];
  //   writeRegions(regions_node, db);
  // }
}

// Placeholder functions

void DbvOut::writeRegions(YAML::Node& regions_node,
                          std::map<std::string, ChipletRegion>& regions)
{
}

void DbvOut::writeRegion(YAML::Node& region_node, const ChipletRegion& region)
{
}

void DbvOut::writeExternal(YAML::Node& external_node,
                           const ChipletExternal& external)
{
}

void DbvOut::writeCoordinates(YAML::Node& coords_node,
                              const std::vector<Coordinate>& coords)
{
}

}  // namespace odb