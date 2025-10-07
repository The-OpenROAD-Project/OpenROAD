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

void DbvOut::writeFile(const std::string& filename, odb::dbDatabase* db)
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

void DbvOut::writeYamlContent(YAML::Node& root, odb::dbDatabase* db)
{
  YAML::Node header_node = root["Header"];

  writeHeader(header_node, db);

  YAML::Node chiplets_node = root["ChipletsDef"];

  writeChipletDefs(chiplets_node, db);
}

void DbvOut::writeHeader(YAML::Node& header_node, odb::dbDatabase* db)
{
  header_node["version"] = "2.5";
  header_node["unit"] = "microns";
  header_node["precision"] = db->getDbuPerMicron();
}

void DbvOut::writeChipletDefs(YAML::Node& chiplets_node, odb::dbDatabase* db)
{
  for (auto chiplet : db->getChips()) {
    YAML::Node chiplet_node = chiplets_node[chiplet->getName()];
    writeChiplet(chiplet_node, chiplet);
  }
}

void DbvOut::writeChiplet(YAML::Node& chiplet_node, odb::dbChip* chiplet)
{
  auto chip_type = chiplet->getChipType();
  switch (chip_type) {
    case (odb::dbChip::ChipType::DIE):
      chiplet_node["type"] = "die";
      break;
    case (odb::dbChip::ChipType::RDL):
      chiplet_node["type"] = "rdl";
      break;
    case (odb::dbChip::ChipType::IP):
      chiplet_node["type"] = "ip";
      break;
    case (odb::dbChip::ChipType::SUBSTRATE):
      chiplet_node["type"] = "substrate";
      break;
    case (odb::dbChip::ChipType::HIER):
      chiplet_node["type"] = "hier";
      break;
    default:
      break;
  }
  auto width = chiplet->getWidth();
  auto height = chiplet->getHeight();

  // Offset
  auto offset_x = chiplet->getOffset().getX();
  auto offset_y = chiplet->getOffset().getY();

  // Seal ring
  auto seal_ring_west = chiplet->getSealRingWest();
  auto seal_ring_north = chiplet->getSealRingNorth();
  auto seal_ring_east = chiplet->getSealRingEast();
  auto seal_ring_south = chiplet->getSealRingSouth();

  // Scribe line
  auto scribe_line_west = chiplet->getScribeLineWest();
  auto scribe_line_north = chiplet->getScribeLineNorth();
  auto scribe_line_east = chiplet->getScribeLineEast();
  auto scribe_line_south = chiplet->getScribeLineSouth();

  chiplet_node["thickness"] = chiplet->getThickness();
  chiplet_node["shrink"] = chiplet->getShrink();
  chiplet_node["tsv"] = chiplet->isTsv();

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

// void DbvOut::writeRegions(YAML::Node& regions_node,
//                           std::map<std::string, ChipletRegion>& regions)
// {
// }

// void DbvOut::writeRegion(YAML::Node& region_node, const ChipletRegion&
// region)
// {
// }

// void DbvOut::writeExternal(YAML::Node& external_node,
//                            const ChipletExternal& external)
// {
// }

// void DbvOut::writeCoordinates(YAML::Node& coords_node,
//                               const std::vector<Coordinate>& coords)
// {
// }

}  // namespace odb