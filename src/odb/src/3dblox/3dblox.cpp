// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "odb/3dblox.h"

#include "dbvParser.h"
#include "dbxParser.h"
#include "objects.h"
#include "odb/db.h"
#include "utl/Logger.h"
namespace odb {
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
                     515,
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
  // If include_path is absolute, return as is
  if (!include_path.empty() && include_path[0] == '/') {
    return include_path;
  }

  // Get the directory of the current file
  size_t last_slash = current_file_path.find_last_of('/');
  if (last_slash == std::string::npos) {
    // No directory in current file path, return include_path as is
    return include_path;
  }

  std::string current_dir = current_file_path.substr(0, last_slash + 1);

  // Combine directory with include path
  return current_dir + include_path;
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
  } else if (type == "rdl") {
    return dbChip::ChipType::RDL;
  } else if (type == "ip") {
    return dbChip::ChipType::IP;
  } else if (type == "substrate") {
    return dbChip::ChipType::SUBSTRATE;
  } else if (type == "hier") {
    return dbChip::ChipType::HIER;
  }
  logger->error(
      utl::ODB, 517, "3DBV Parser Error: Invalid chip type: {}", type);
}
void ThreeDBlox::createChiplet(const ChipletDef& chiplet)
{
  dbChip* chip
      = dbChip::create(db_, chiplet.name, getChipType(chiplet.type, logger_));

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
  } else if (side == "back") {
    return dbChipRegion::Side::BACK;
  } else if (side == "internal") {
    return dbChipRegion::Side::INTERNAL;
  } else if (side == "internal_ext") {
    return dbChipRegion::Side::INTERNAL_EXT;
  }
  logger->error(
      utl::ODB, 518, "3DBV Parser Error: Invalid chip region side: {}", side);
}
void ThreeDBlox::createRegion(const ChipletRegion& region, dbChip* chip)
{
  dbTechLayer* layer = nullptr;
  if (region.layer != "") {
    // TODO: add layer
  }
  dbChipRegion* chip_region = dbChipRegion::create(
      chip, region.name, getChipRegionSide(region.side, logger_), layer);
  Rect box;
  bool init = true;
  for (const auto& coord : region.coords) {
    if (init) {
      box.init(coord.x * db_->getDbuPerMicron(),
               coord.y * db_->getDbuPerMicron(),
               coord.x * db_->getDbuPerMicron(),
               coord.y * db_->getDbuPerMicron());
      init = false;
    } else {
      box.merge(Point(coord.x * db_->getDbuPerMicron(),
                      coord.y * db_->getDbuPerMicron()),
                box);
    }
  }
  chip_region->setBox(box);
}
void ThreeDBlox::createDesignTopChiplet(const DesignDef& design)
{
  dbChip* chip = dbChip::create(db_, design.name, dbChip::ChipType::HIER);
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
}
void ThreeDBlox::createConnection(const Connection& connection)
{
  // dbChip* top_chip = db_->findChip(connection.top.c_str());
  // dbChip* bottom_chip = db_->findChip(connection.bot.c_str());
  // if (top_chip == nullptr || bottom_chip == nullptr) {
  //   logger_->error(utl::ODB,
  //                  520,
  //                  "3DBX Parser Error: Connection top or bottom chip not
  //                  found " "for connection {}", connection.name);
  // }
}
}  // namespace odb