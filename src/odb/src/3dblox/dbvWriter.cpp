// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbvWriter.h"

#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>
#include <string>

#include "objects.h"
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
  BaseWriter::writeYamlContent(root, db);

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
  bool cad_layer = false;  // TODO: use cad layer
  // Chiplet basic information
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
  YAML::Emitter dim_out;
  dim_out << YAML::Flow << YAML::BeginSeq
          << width / (double) db->getDbuPerMicron()
          << height / (double) db->getDbuPerMicron() << YAML::EndSeq;
  chiplet_node["design_area"] = YAML::Load(dim_out.c_str());
  chiplet_node["thickness"]
      = chiplet->getThickness() / (double) db->getDbuPerMicron();
  chiplet_node["shrink"] = chiplet->getShrink();
  chiplet_node["tsv"] = chiplet->isTsv();

  // Write cad_layer if enabled
  if (cad_layer) {
    auto offset_x = chiplet->getOffset().getX();
    auto offset_y = chiplet->getOffset().getY();
    YAML::Emitter off_out;
    off_out << YAML::Flow << YAML::BeginSeq
            << offset_x / (double) db->getDbuPerMicron()
            << offset_y / (double) db->getDbuPerMicron() << YAML::EndSeq;
    chiplet_node["offset"] = YAML::Load(off_out.c_str());
    YAML::Node cad_layer = chiplet_node["cad_layer"];

    auto seal_ring_west
        = chiplet->getSealRingWest() / (double) db->getDbuPerMicron();
    auto scribe_line_west
        = chiplet->getScribeLineWest() / (double) db->getDbuPerMicron();

    if (seal_ring_west != -1.0) {
      auto seal_ring_north
          = chiplet->getSealRingNorth() / (double) db->getDbuPerMicron();
      auto seal_ring_east
          = chiplet->getSealRingEast() / (double) db->getDbuPerMicron();
      auto seal_ring_south
          = chiplet->getSealRingSouth() / (double) db->getDbuPerMicron();
      YAML::Emitter sr_out;
      sr_out << YAML::Flow << YAML::BeginSeq
             << chiplet->getSealRingWest() / (double) db->getDbuPerMicron()
             << chiplet->getSealRingSouth() / (double) db->getDbuPerMicron()
             << chiplet->getSealRingEast() / (double) db->getDbuPerMicron()
             << chiplet->getSealRingNorth() / (double) db->getDbuPerMicron()
             << YAML::EndSeq;
      YAML::Node layer_id = cad_layer["108;102"];
      layer_id["type"] = "seal_ring";
      layer_id["seal_ring_width"] = YAML::Load(sr_out.c_str());
    } else if (scribe_line_west != -1.0) {
      auto scribe_line_north
          = chiplet->getScribeLineNorth() / (double) db->getDbuPerMicron();
      auto scribe_line_east
          = chiplet->getScribeLineEast() / (double) db->getDbuPerMicron();
      auto scribe_line_south
          = chiplet->getScribeLineSouth() / (double) db->getDbuPerMicron();
      YAML::Emitter sc_out;
      sc_out << YAML::Flow << YAML::BeginSeq
             << chiplet->getScribeLineWest() / (double) db->getDbuPerMicron()
             << chiplet->getScribeLineSouth() / (double) db->getDbuPerMicron()
             << chiplet->getScribeLineEast() / (double) db->getDbuPerMicron()
             << chiplet->getScribeLineNorth() / (double) db->getDbuPerMicron()
             << YAML::EndSeq;
      YAML::Node layer_id = cad_layer["108;103"];
      layer_id["type"] = "scribe_line";
      layer_id["scribe_line_remaining_width"] = YAML::Load(sc_out.c_str());
    } else {
      YAML::Node layer_id = cad_layer["108;101"];
      layer_id["type"] = "design_area";
    }
  }
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

static const char* chip_region_side_to_string(odb::dbChipRegion::Side side)
{
  switch (side) {
    case odb::dbChipRegion::Side::FRONT:
      return "front";
    case odb::dbChipRegion::Side::BACK:
      return "back";
    case odb::dbChipRegion::Side::INTERNAL:
      return "internal";
    case odb::dbChipRegion::Side::INTERNAL_EXT:
      return "internal_ext";
  }
  return "front";
}

void DbvWriter::writeRegion(YAML::Node& region_node,
                            odb::dbChipRegion* region,
                            odb::dbDatabase* db)
{
  region_node["side"] = chip_region_side_to_string(region->getSide());
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
  BaseWriter::writeDef(external_node, db, chiplet);
}

void DbvWriter::writeCoordinates(YAML::Node& coords_node,
                                 const odb::Rect& rect,
                                 odb::dbDatabase* db)
{
  const double u = db->getDbuPerMicron();
  YAML::Emitter c0;
  c0 << YAML::Flow << YAML::BeginSeq << rect.xMin() / u << rect.yMin() / u
     << YAML::EndSeq;
  YAML::Emitter c1;
  c1 << YAML::Flow << YAML::BeginSeq << rect.xMax() / u << rect.yMin() / u
     << YAML::EndSeq;
  YAML::Emitter c2;
  c2 << YAML::Flow << YAML::BeginSeq << rect.xMax() / u << rect.yMax() / u
     << YAML::EndSeq;
  YAML::Emitter c3;
  c3 << YAML::Flow << YAML::BeginSeq << rect.xMin() / u << rect.yMax() / u
     << YAML::EndSeq;
  coords_node.push_back(YAML::Load(c0.c_str()));
  coords_node.push_back(YAML::Load(c1.c_str()));
  coords_node.push_back(YAML::Load(c2.c_str()));
  coords_node.push_back(YAML::Load(c3.c_str()));
}

void DbvWriter::writeLevelDependencies(
    YAML::Node& header_node,
    const std::vector<odb::dbChip*>& chiplets,
    odb::dbDatabase* db)
{
  // Find all dependencies for the current level chiplets
  std::set<int> dependency_levels;

  for (auto chiplet : chiplets) {
    // Find all chiplets that this chiplet depends on
    for (auto inst : chiplet->getChipInsts()) {
      auto master_chip = inst->getMasterChip();
      if (master_chip != nullptr) {
        // Find the level of the master chip
        ChipletHierarchyAnalyzer analyzer(logger_);
        auto levels = analyzer.analyzeHierarchy(db);

        for (const auto& level : levels) {
          for (auto level_chiplet : level.chiplets) {
            if (level_chiplet == master_chip) {
              dependency_levels.insert(level.level);
              break;
            }
          }
        }
      }
    }
  }

  // Add include statements for dependency levels
  if (!dependency_levels.empty()) {
    YAML::Node includes_node = header_node["include"];

    for (int dep_level : dependency_levels) {
      // Generate filename for the dependency level
      std::string dep_filename
          = getDependencyFilename(current_file_path_, dep_level);
      includes_node.push_back(dep_filename);
    }
  }
}

void DbvWriter::writeHierarchicalDbv(const std::string& base_filename,
                                     odb::dbDatabase* db)
{
  ChipletHierarchyAnalyzer analyzer(logger_);
  auto levels = analyzer.analyzeHierarchy(db);

  if (levels.empty()) {
    logger_->warn(utl::ODB, 537, "No chiplets found in database");
    return;
  }

  if (levels.size() == 1) {
    writeFile(base_filename, db);
  } else {
    // Write each level to a separate file
    for (const auto& level : levels) {
      if (level.chiplets.empty()) {
        continue;
      }

      std::string level_filename
          = generateLevelFilename(base_filename, level.level);
      writeLevelToFile(level_filename, level.chiplets, db);

      logger_->info(utl::ODB,
                    538,
                    "Wrote level {} with {} chiplets to {}",
                    level.level,
                    level.chiplets.size(),
                    level_filename);
    }
  }
}

void DbvWriter::writeLevelToFile(const std::string& filename,
                                 const std::vector<odb::dbChip*>& chiplets,
                                 odb::dbDatabase* db)
{
  current_file_path_ = filename;

  YAML::Node root;

  // Write header with dependencies
  YAML::Node header_node = root["Header"];
  writeHeader(header_node, db);
  writeLevelDependencies(header_node, chiplets, db);

  // Write chiplets
  YAML::Node chiplets_node = root["ChipletDef"];
  for (auto chiplet : chiplets) {
    YAML::Node chiplet_node = chiplets_node[chiplet->getName()];
    writeChipletInternal(chiplet_node, chiplet, db);
  }

  writeYamlToFile(filename, root);
}

std::string DbvWriter::generateLevelFilename(const std::string& base_filename,
                                             int level)
{
  std::filesystem::path base_path(base_filename);
  std::string base_name = base_path.stem().string();
  std::string extension = base_path.extension().string();
  std::string directory = base_path.parent_path().string();

  std::ostringstream filename;
  if (!directory.empty()) {
    filename << directory << "/";
  }
  filename << base_name << "_level" << level << extension;

  return filename.str();
}

std::string DbvWriter::getBaseName(const std::string& filename)
{
  std::filesystem::path path(filename);
  return path.stem().string();
}

std::string DbvWriter::getDirectory(const std::string& filename)
{
  std::filesystem::path path(filename);
  return path.parent_path().string();
}

std::string DbvWriter::getDependencyFilename(const std::string& base_filename,
                                             int dependency_level)
{
  std::filesystem::path base_path(base_filename);
  std::string base_name = base_path.stem().string();
  std::string extension = base_path.extension().string();
  std::string directory = base_path.parent_path().string();

  std::ostringstream filename;
  if (!directory.empty()) {
    filename << directory << "/";
  }
  filename << base_name << "_level" << dependency_level << extension;

  return filename.str();
}

}  // namespace odb