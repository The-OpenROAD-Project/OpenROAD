// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "odb/3dblox.h"

#include <filesystem>

#include "dbvParser.h"
#include "dbxParser.h"
#include "objects.h"
#include "odb/db.h"
#include "utl/Logger.h"

namespace odb {

static std::map<std::string, std::string> dup_orient_map
    = {{"MY_R180", "MX"},
       {"MY_R270", "MX_R90"},
       {"MX_R180", "MY"},
       {"MX_R180", "MY_R90"},
       {"MZ_MY_R180", "MZ_MX"},
       {"MZ_MY_R270", "MZ_MX_R90"},
       {"MZ_MX_R180", "MZ_MY"},
       {"MZ_MX_R270", "MZ_MY_R90"}};

ThreeDBlox::ThreeDBlox(utl::Logger* logger, odb::dbDatabase* db)
    : logger_(logger), db_(db)
{
}

void ThreeDBlox::readDbv(const std::string& dbv_file)
{
  DbvParser parser(logger_);
  DbvData data = parser.parseFile(dbv_file);
  if (db_->getDbuPerMicron() == 0) {
    db_->setDbuPerMicron(data.header.precision);
  } else {
    if (data.header.precision > db_->getDbuPerMicron()) {
      logger_->error(utl::ODB,
                     526,
                     "3DBV Parser Error: Precision is greater than dbu per "
                     "micron already set for database");
    } else if (db_->getDbuPerMicron() % data.header.precision != 0) {
      logger_->error(utl::ODB,
                     516,
                     "3DBV Parser Error: Database DBU per micron ({}) must be "
                     "a multiple of the precision ({}) in dbv file {}",
                     db_->getDbuPerMicron(),
                     data.header.precision,
                     dbv_file);
    }
  }
  for (const auto& [_, chiplet] : data.chiplet_defs) {
    createChiplet(chiplet);
  }
}

std::string ThreeDBlox::resolveIncludePath(const std::string& include_path,
                                           const std::string& current_file_path)
{
  std::filesystem::path include_fs_path(include_path);
  if (include_fs_path.is_absolute()) {
    return include_fs_path.string();
  }
  std::filesystem::path current_fs_path(current_file_path);
  std::filesystem::path current_dir = current_fs_path.parent_path();
  std::filesystem::path resolved_path = current_dir / include_fs_path;
  return resolved_path.string();
}

void ThreeDBlox::readDbx(const std::string& dbx_file)
{
  DbxParser parser(logger_);
  DbxData data = parser.parseFile(dbx_file);
  for (const auto& include : data.header.includes) {
    std::string resolved_path = resolveIncludePath(include, dbx_file);
    if (include.find(".3dbv") != std::string::npos) {
      readDbv(resolved_path);
    } else if (include.find(".3dbx") != std::string::npos) {
      readDbx(resolved_path);
    }
  }
  createDesignTopChiplet(data.design);
  for (const auto& [_, chip_inst] : data.chiplet_instances) {
    createChipInst(chip_inst);
  }
  for (const auto& [_, connection] : data.connections) {
    createConnection(connection);
  }
}

dbChip::ChipType getChipType(const std::string& type, utl::Logger* logger)
{
  if (type == "die") {
    return dbChip::ChipType::DIE;
  }
  if (type == "rdl") {
    return dbChip::ChipType::RDL;
  }
  if (type == "ip") {
    return dbChip::ChipType::IP;
  }
  if (type == "substrate") {
    return dbChip::ChipType::SUBSTRATE;
  }
  if (type == "hier") {
    return dbChip::ChipType::HIER;
  }
  logger->error(
      utl::ODB, 527, "3DBV Parser Error: Invalid chip type: {}", type);
}
void ThreeDBlox::createChiplet(const ChipletDef& chiplet)
{
  auto tech = db_->getTech();  // TODO: specify tech
  dbChip* chip = dbChip::create(
      db_, tech, chiplet.name, getChipType(chiplet.type, logger_));

  chip->setWidth(chiplet.design_width * db_->getDbuPerMicron());
  chip->setHeight(chiplet.design_height * db_->getDbuPerMicron());
  chip->setThickness(chiplet.thickness * db_->getDbuPerMicron());
  chip->setShrink(chiplet.shrink);
  chip->setTsv(chiplet.tsv);

  chip->setScribeLineEast(chiplet.scribe_line_right * db_->getDbuPerMicron());
  chip->setScribeLineWest(chiplet.scribe_line_left * db_->getDbuPerMicron());
  chip->setScribeLineNorth(chiplet.scribe_line_top * db_->getDbuPerMicron());
  chip->setScribeLineSouth(chiplet.scribe_line_bottom * db_->getDbuPerMicron());

  chip->setSealRingEast(chiplet.seal_ring_right * db_->getDbuPerMicron());
  chip->setSealRingWest(chiplet.seal_ring_left * db_->getDbuPerMicron());
  chip->setSealRingNorth(chiplet.seal_ring_top * db_->getDbuPerMicron());
  chip->setSealRingSouth(chiplet.seal_ring_bottom * db_->getDbuPerMicron());

  chip->setOffset(Point(chiplet.offset.x * db_->getDbuPerMicron(),
                        chiplet.offset.y * db_->getDbuPerMicron()));
  for (const auto& [_, region] : chiplet.regions) {
    createRegion(region, chip);
  }
}
dbChipRegion::Side getChipRegionSide(const std::string& side,
                                     utl::Logger* logger)
{
  if (side == "front") {
    return dbChipRegion::Side::FRONT;
  }
  if (side == "back") {
    return dbChipRegion::Side::BACK;
  }
  if (side == "internal") {
    return dbChipRegion::Side::INTERNAL;
  }
  if (side == "internal_ext") {
    return dbChipRegion::Side::INTERNAL_EXT;
  }
  logger->error(
      utl::ODB, 528, "3DBV Parser Error: Invalid chip region side: {}", side);
}
void ThreeDBlox::createRegion(const ChipletRegion& region, dbChip* chip)
{
  dbTechLayer* layer = nullptr;
  if (!region.layer.empty()) {
    // TODO: add layer
  }
  dbChipRegion* chip_region = dbChipRegion::create(
      chip, region.name, getChipRegionSide(region.side, logger_), layer);
  Rect box;
  box.mergeInit();
  for (const auto& coord : region.coords) {
    box.merge(Point(coord.x * db_->getDbuPerMicron(),
                    coord.y * db_->getDbuPerMicron()),
              box);
  }
  chip_region->setBox(box);
}
void ThreeDBlox::createDesignTopChiplet(const DesignDef& design)
{
  dbChip* chip
      = dbChip::create(db_, nullptr, design.name, dbChip::ChipType::HIER);
  db_->setTopChip(chip);
}
void ThreeDBlox::createChipInst(const ChipletInst& chip_inst)
{
  auto chip = db_->findChip(chip_inst.reference.c_str());
  if (chip == nullptr) {
    logger_->error(utl::ODB,
                   519,
                   "3DBX Parser Error: Chiplet instance reference {} not found "
                   "for chip inst {}",
                   chip_inst.reference,
                   chip_inst.name);
  }
  dbChipInst* inst = dbChipInst::create(db_->getChip(), chip, chip_inst.name);
  inst->setLoc(Point3D(chip_inst.loc.x * db_->getDbuPerMicron(),
                       chip_inst.loc.y * db_->getDbuPerMicron(),
                       chip_inst.z * db_->getDbuPerMicron()));
  auto orient_str = chip_inst.orient;
  if (dup_orient_map.find(orient_str) != dup_orient_map.end()) {
    orient_str = dup_orient_map[orient_str];
  }
  auto orient = dbOrientType3D::fromString(orient_str);
  if (!orient.has_value()) {
    logger_->error(utl::ODB,
                   525,
                   "3DBX Parser Error: Invalid orient {} for chip inst {}",
                   chip_inst.orient,
                   chip_inst.name);
  }
  inst->setOrient(orient.value());
}
std::vector<std::string> splitPath(const std::string& path)
{
  std::vector<std::string> parts;
  std::istringstream stream(path);
  std::string part;

  while (std::getline(stream, part, '/')) {
    if (!part.empty()) {
      parts.push_back(part);
    }
  }

  return parts;
}

dbChipRegionInst* ThreeDBlox::resolvePath(const std::string& path,
                                          std::vector<dbChipInst*>& path_insts)
{
  if (path == "~") {
    return nullptr;
  }
  // Split the path by '/'
  std::vector<std::string> path_parts = splitPath(path);

  if (path_parts.empty()) {
    logger_->error(utl::ODB, 524, "3DBX Parser Error: Invalid path {}", path);
  }

  // The last part should contain ".regions.regionName"
  std::string last_part = path_parts.back();
  size_t regions_pos = last_part.find(".regions.");
  if (regions_pos == std::string::npos) {
    return nullptr;  // Invalid format
  }

  // Extract chip instance name and region name from last part
  std::string last_chip_inst = last_part.substr(0, regions_pos);
  std::string region_name = last_part.substr(regions_pos + 9);

  // Replace the last part with just the chip instance name
  path_parts.back() = last_chip_inst;

  // TODO: Traverse hierarchy and find region
  path_insts.reserve(path_parts.size());
  dbChip* curr_chip = db_->getChip();
  dbChipInst* curr_chip_inst = nullptr;
  for (const auto& inst_name : path_parts) {
    curr_chip_inst = curr_chip->findChipInst(inst_name);
    if (curr_chip_inst == nullptr) {
      logger_->error(utl::ODB,
                     522,
                     "3DBX Parser Error: Chip instance {} not found in path {}",
                     inst_name,
                     path);
    }
    path_insts.push_back(curr_chip_inst);
    curr_chip = curr_chip_inst->getMasterChip();
  }
  auto region = curr_chip_inst->findChipRegionInst(region_name);
  if (region == nullptr) {
    logger_->error(utl::ODB,
                   523,
                   "3DBX Parser Error: Chip region {} not found in path {}",
                   region_name,
                   path);
  }
  return region;
}
void ThreeDBlox::createConnection(const Connection& connection)
{
  auto top_path = connection.top;
  auto bottom_path = connection.bot;
  std::vector<dbChipInst*> top_region_path;
  std::vector<dbChipInst*> bottom_region_path;
  auto top_region = resolvePath(top_path, top_region_path);
  auto bottom_region = resolvePath(bottom_path, bottom_region_path);
  auto conn = odb::dbChipConn::create(connection.name,
                                      db_->getChip(),
                                      top_region_path,
                                      top_region,
                                      bottom_region_path,
                                      bottom_region);
  conn->setThickness(connection.thickness * db_->getDbuPerMicron());
}
}  // namespace odb