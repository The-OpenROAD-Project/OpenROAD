// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbvWriter.h"

#include <yaml-cpp/emitterstyle.h>
#include <yaml-cpp/node/node.h>
#include <yaml-cpp/yaml.h>

#include <string>
#include <unordered_set>
#include <vector>

#include "baseWriter.h"
#include "chipletHierarchy.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "utl/Logger.h"

namespace odb {

DbvWriter::DbvWriter(utl::Logger* logger) : BaseWriter(logger)
{
}

void DbvWriter::writeFile(const std::string& filename, odb::dbDatabase* db)
{
  YAML::Node root;
  writeYamlContent(root, db);
  writeYamlToFile(filename, root);
}

void DbvWriter::writeYamlContent(YAML::Node& root, odb::dbDatabase* db)
{
  YAML::Node header_node = root["Header"];
  writeHeader(header_node, db);
  YAML::Node chiplets_node = root["ChipletDef"];
  writeChipletDefs(chiplets_node, db);
}

void DbvWriter::writeChipletDefs(YAML::Node& chiplets_node, odb::dbDatabase* db)
{
  for (auto chiplet : db->getChips()) {
    YAML::Node chiplet_node = chiplets_node[chiplet->getName()];
    writeChipletInternal(chiplet_node, chiplet, db);
  }
}

void DbvWriter::writeChipletInternal(YAML::Node& chiplet_node,
                                     odb::dbChip* chiplet,
                                     odb::dbDatabase* db)
{
  // Chiplet basic information
  const auto chip_type = chiplet->getChipType();
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
  const double u = db->getDbuPerMicron();
  auto width = chiplet->getWidth();
  auto height = chiplet->getHeight();
  YAML::Node dim_out;
  dim_out.SetStyle(YAML::EmitterStyle::Flow);
  dim_out.push_back(width / u);
  dim_out.push_back(height / u);
  chiplet_node["design_area"] = dim_out;
  chiplet_node["thickness"] = chiplet->getThickness() / u;
  chiplet_node["shrink"] = chiplet->getShrink();
  chiplet_node["tsv"] = chiplet->isTsv();

  // Offset
  auto offset_x = chiplet->getOffset().getX();
  auto offset_y = chiplet->getOffset().getY();
  YAML::Node off_out;
  off_out.SetStyle(YAML::EmitterStyle::Flow);
  off_out.push_back(offset_x / u);
  off_out.push_back(offset_y / u);
  chiplet_node["offset"] = off_out;

  // Seal Ring
  YAML::Node sr_out;
  sr_out.SetStyle(YAML::EmitterStyle::Flow);
  sr_out.push_back(chiplet->getSealRingWest() / u);
  sr_out.push_back(chiplet->getSealRingSouth() / u);
  sr_out.push_back(chiplet->getSealRingEast() / u);
  sr_out.push_back(chiplet->getSealRingNorth() / u);
  chiplet_node["seal_ring_width"] = sr_out;

  // Scribe Line
  YAML::Node sl_out;
  sl_out.SetStyle(YAML::EmitterStyle::Flow);
  sl_out.push_back(chiplet->getScribeLineWest() / u);
  sl_out.push_back(chiplet->getScribeLineSouth() / u);
  sl_out.push_back(chiplet->getScribeLineEast() / u);
  sl_out.push_back(chiplet->getScribeLineNorth() / u);
  chiplet_node["scribe_line_remaining_width"] = sl_out;

  // External files
  YAML::Node external_node = chiplet_node["external"];
  writeExternal(external_node, chiplet, db);

  // Regions
  YAML::Node regions_node = chiplet_node["regions"];
  writeRegions(regions_node, chiplet, db);
}

void DbvWriter::writeRegions(YAML::Node& regions_node,
                             odb::dbChip* chiplet,
                             odb::dbDatabase* db)
{
  for (auto region : chiplet->getChipRegions()) {
    YAML::Node region_node = regions_node[region->getName()];
    writeRegion(region_node, region, db);
  }
}

void DbvWriter::writeRegion(YAML::Node& region_node,
                            odb::dbChipRegion* region,
                            odb::dbDatabase* db)
{
  const auto side = region->getSide();
  switch (side) {
    case odb::dbChipRegion::Side::FRONT:
      region_node["side"] = "front";
      break;
    case odb::dbChipRegion::Side::BACK:
      region_node["side"] = "back";
      break;
    case odb::dbChipRegion::Side::INTERNAL:
      region_node["side"] = "internal";
      break;
    case odb::dbChipRegion::Side::INTERNAL_EXT:
      region_node["side"] = "internal_ext";
      break;
  }
  if (auto layer = region->getLayer(); layer != nullptr) {
    region_node["layer"] = layer->getName();
  }
  YAML::Node coords_node = region_node["coords"];
  writeCoordinates(coords_node, region->getBox(), db);
}

void DbvWriter::writeExternal(YAML::Node& external_node,
                              odb::dbChip* chiplet,
                              odb::dbDatabase* db)
{
  BaseWriter::writeLef(external_node, db, chiplet);
  if (db->getChip()->getBlock() != nullptr) {
    BaseWriter::writeDef(external_node, db, chiplet);
  }
}

void DbvWriter::writeCoordinates(YAML::Node& coords_node,
                                 const odb::Rect& rect,
                                 odb::dbDatabase* db)
{
  const double u = db->getDbuPerMicron();
  YAML::Node c0, c1, c2, c3;
  c0.SetStyle(YAML::EmitterStyle::Flow);
  c1.SetStyle(YAML::EmitterStyle::Flow);
  c2.SetStyle(YAML::EmitterStyle::Flow);
  c3.SetStyle(YAML::EmitterStyle::Flow);
  c0.push_back(rect.xMin() / u);
  c0.push_back(rect.yMin() / u);
  c1.push_back(rect.xMax() / u);
  c1.push_back(rect.yMin() / u);
  c2.push_back(rect.xMax() / u);
  c2.push_back(rect.yMax() / u);
  c3.push_back(rect.xMin() / u);
  c3.push_back(rect.yMax() / u);
  coords_node.push_back(c0);
  coords_node.push_back(c1);
  coords_node.push_back(c2);
  coords_node.push_back(c3);
}

void DbvWriter::writeChipDependencies(YAML::Node& header_node,
                                      const ChipletNode* node)
{
  std::unordered_set<std::string> included;

  for (auto child : node->children) {
    auto* child_chip = child->chip;
    if (child_chip->getChipType() == odb::dbChip::ChipType::HIER) {
      // Add the child's .3dbx file include
      std::string child_3dbx = std::string(child_chip->getName()) + ".3dbx";
      included.insert(child_3dbx);
      // TODO: Call dbxWriter::writeChiplet(child_chip)
    }
  }
  if (!included.empty()) {
    YAML::Node includes_node = header_node["include"];
    for (const auto& include : included) {
      includes_node.push_back(include);
    }
  }
}

void DbvWriter::writeChipletToFile(const std::string& filename,
                                   odb::dbChip* chiplet,
                                   odb::dbDatabase* db,
                                   ChipletNode* node)
{
  YAML::Node root;
  YAML::Node header_node = root["Header"];
  writeHeader(header_node, db);

  writeChipDependencies(header_node, node);

  YAML::Node chiplets_node = root["ChipletDef"];
  for (auto dependecy : node->children) {
    YAML::Node chiplet_node = chiplets_node[dependecy->chip->getName()];
    writeChipletInternal(chiplet_node, dependecy->chip, db);
  }

  writeYamlToFile(filename, root);
}

void DbvWriter::writeChiplet(const std::string& base_filename,
                             odb::dbDatabase* db,
                             odb::dbChip* top_chip)
{
  std::vector<odb::dbChip*> all_chips;
  for (auto chiplet : db->getChips()) {
    all_chips.push_back(chiplet);
  }

  ChipletHierarchy hierarchy;
  hierarchy.buildHierarchy(all_chips);

  auto chiplet_node = hierarchy.findNodeForChip(top_chip);

  writeChipletToFile(base_filename, top_chip, db, chiplet_node);
}

}  // namespace odb