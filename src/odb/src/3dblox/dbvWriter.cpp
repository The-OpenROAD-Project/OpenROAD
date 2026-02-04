// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbvWriter.h"

#include <filesystem>
#include <string>
#include <unordered_set>

#include "baseWriter.h"
#include "odb/db.h"
#include "odb/defout.h"
#include "utl/Logger.h"
#include "yaml-cpp/yaml.h"
namespace odb {

DbvWriter::DbvWriter(utl::Logger* logger, odb::dbDatabase* db)
    : BaseWriter(logger, db)
{
}

void DbvWriter::writeChiplet(const std::string& base_filename,
                             odb::dbChip* top_chip)
{
  // get dir using std::filesystem
  auto path = std::filesystem::path(base_filename);
  if (path.has_parent_path()) {
    current_dir_path_ = path.parent_path().string() + "/";
  }
  std::unordered_set<odb::dbChip*> chips;
  for (const auto chipinst : top_chip->getChipInsts()) {
    chips.insert(chipinst->getMasterChip());
  }
  YAML::Node root;
  YAML::Node header_node = root["Header"];
  writeHeader(header_node);
  writeHeaderIncludes(header_node, chips);
  YAML::Node chiplets_node = root["ChipletDef"];
  writeChipletDefs(chiplets_node, chips);
  writeYamlToFile(base_filename, root);
}

void DbvWriter::writeHeaderIncludes(
    YAML::Node& header_node,
    const std::unordered_set<odb::dbChip*>& chips)
{
  for (const auto chip : chips) {
    if (chip->getChipType() == odb::dbChip::ChipType::HIER) {
      // TODO: write 3dbx file
      header_node["include"].push_back(std::string(chip->getName()) + ".3dbx");
    }
  }
}

void DbvWriter::writeChipletDefs(YAML::Node& chiplets_node,
                                 const std::unordered_set<odb::dbChip*>& chips)
{
  for (const auto chiplet : chips) {
    YAML::Node chiplet_node = chiplets_node[chiplet->getName()];
    writeChipletInternal(chiplet_node, chiplet);
  }
}

void DbvWriter::writeChipletInternal(YAML::Node& chiplet_node,
                                     odb::dbChip* chiplet)
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
  auto width = chiplet->getWidth();
  auto height = chiplet->getHeight();
  if (width >= 0 && height >= 0) {
    YAML::Node dim_out;
    dim_out.SetStyle(YAML::EmitterStyle::Flow);
    dim_out.push_back(dbuToMicron(width));
    dim_out.push_back(dbuToMicron(height));
    chiplet_node["design_area"] = dim_out;
  }
  if (chiplet->getThickness() >= 0) {
    chiplet_node["thickness"] = dbuToMicron(chiplet->getThickness());
  }
  if (chiplet->getShrink() >= 0) {
    chiplet_node["shrink"] = chiplet->getShrink();
  }
  chiplet_node["tsv"] = chiplet->isTsv();
  // Offset
  YAML::Node offset_node = chiplet_node["offset"];
  writeCoordinate(offset_node, chiplet->getOffset());

  // Seal Ring
  if (chiplet->getSealRingWest() >= 0 && chiplet->getSealRingSouth() >= 0
      && chiplet->getSealRingEast() >= 0 && chiplet->getSealRingNorth() >= 0) {
    YAML::Node sr_out;
    sr_out.SetStyle(YAML::EmitterStyle::Flow);
    sr_out.push_back(dbuToMicron(chiplet->getSealRingWest()));
    sr_out.push_back(dbuToMicron(chiplet->getSealRingSouth()));
    sr_out.push_back(dbuToMicron(chiplet->getSealRingEast()));
    sr_out.push_back(dbuToMicron(chiplet->getSealRingNorth()));
    chiplet_node["seal_ring_width"] = sr_out;
  }

  // Scribe Line
  if (chiplet->getScribeLineWest() >= 0 && chiplet->getScribeLineSouth() >= 0
      && chiplet->getScribeLineEast() >= 0
      && chiplet->getScribeLineNorth() >= 0) {
    YAML::Node sl_out;
    sl_out.SetStyle(YAML::EmitterStyle::Flow);
    sl_out.push_back(dbuToMicron(chiplet->getScribeLineWest()));
    sl_out.push_back(dbuToMicron(chiplet->getScribeLineSouth()));
    sl_out.push_back(dbuToMicron(chiplet->getScribeLineEast()));
    sl_out.push_back(dbuToMicron(chiplet->getScribeLineNorth()));
    chiplet_node["scribe_line_remaining_width"] = sl_out;
  }
  // External files
  YAML::Node external_node = chiplet_node["external"];
  writeExternal(external_node, chiplet);

  // Regions
  YAML::Node regions_node = chiplet_node["regions"];
  writeRegions(regions_node, chiplet);
}

void DbvWriter::writeRegions(YAML::Node& regions_node, odb::dbChip* chiplet)
{
  for (auto region : chiplet->getChipRegions()) {
    YAML::Node region_node = regions_node[region->getName()];
    writeRegion(region_node, region);
  }
}

void DbvWriter::writeRegion(YAML::Node& region_node, odb::dbChipRegion* region)
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
  std::string bmap_file = std::string(region->getChip()->getName()) + "_"
                          + region->getName() + ".bmap";
  region_node["bmap"] = bmap_file;
  if (auto layer = region->getLayer(); layer != nullptr) {
    region_node["layer"] = layer->getName();
  }
  YAML::Node coords_node = region_node["coords"];
  writeCoordinates(coords_node, region->getBox());
}

void DbvWriter::writeExternal(YAML::Node& external_node, odb::dbChip* chiplet)
{
  if (chiplet->getChipType() != odb::dbChip::ChipType::HIER) {
    writeLef(external_node, chiplet);
    if (chiplet->getBlock() != nullptr) {
      writeDef(external_node, chiplet);
    }
    if (auto prop = odb::dbStringProperty::find(chiplet, "verilog_file")) {
      external_node["verilog_file"] = prop->getValue();
    }
  }
}

namespace {
std::unordered_set<odb::dbLib*> getLibs(odb::dbBlock* block)
{
  std::unordered_set<odb::dbLib*> libs;
  for (const auto inst : block->getInsts()) {
    const auto master = inst->getMaster();
    libs.insert(master->getLib());
  }
  return libs;
}
}  // namespace

void DbvWriter::writeLef(YAML::Node& external_node, odb::dbChip* chiplet)
{
  if (chiplet->getTech()) {
    auto tech = chiplet->getTech();
    std::string tech_file = tech->getName() + ".lef";
    YAML::Node list_node;
    list_node.SetStyle(YAML::EmitterStyle::Flow);
    list_node.push_back(tech_file);
    external_node["APR_tech_file"] = list_node;
  }
  if (chiplet->getBlock()) {
    const auto libs = getLibs(chiplet->getBlock());
    int num_libs = libs.size();
    if (num_libs == 0) {
      return;
    }
    if (num_libs > 1) {
      logger_->info(
          utl::ODB,
          541,
          "More than one lib exists, multiple files will be written.");
    }
    YAML::Node list_node;
    list_node.SetStyle(YAML::EmitterStyle::Flow);
    external_node["LEF_file"] = list_node;
    for (auto lib : libs) {
      std::string lef_file = std::string(lib->getName()) + "_lib.lef";
      external_node["LEF_file"].push_back(lef_file);
    }
  }
}

void DbvWriter::writeDef(YAML::Node& external_node, odb::dbChip* chiplet)
{
  std::string def_file = std::string(chiplet->getName()) + ".def";
  std::string def_file_path = current_dir_path_ + def_file;
  odb::DefOut def_writer(logger_);
  auto block = chiplet->getBlock();
  def_writer.writeBlock(block, def_file_path.c_str());
  external_node["DEF_file"] = def_file;
}

}  // namespace odb