// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "odb/3dblox.h"

#include <cstddef>
#include <filesystem>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "bmapParser.h"
#include "bmapWriter.h"
#include "checker.h"
#include "dbvParser.h"
#include "dbvWriter.h"
#include "dbxParser.h"
#include "dbxWriter.h"
#include "objects.h"
#include "odb/db.h"
#include "odb/dbTransform.h"
#include "odb/dbTypes.h"
#include "odb/defin.h"
#include "odb/defout.h"
#include "odb/geom.h"
#include "odb/lefin.h"
#include "odb/lefout.h"
#include "sta/Sta.hh"
#include "utl/Logger.h"
#include "utl/ScopedTemporaryFile.h"
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

ThreeDBlox::ThreeDBlox(utl::Logger* logger, odb::dbDatabase* db, sta::Sta* sta)
    : logger_(logger), db_(db), sta_(sta)
{
}

void ThreeDBlox::readDbv(const std::string& dbv_file)
{
  read_files_.insert(std::filesystem::absolute(dbv_file).string());
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
  readHeaderIncludes(data.header.includes);
  for (const auto& [_, chiplet] : data.chiplet_defs) {
    createChiplet(chiplet);
  }
}

void ThreeDBlox::readDbx(const std::string& dbx_file)
{
  read_files_.insert(std::filesystem::absolute(dbx_file).string());
  DbxParser parser(logger_);
  DbxData data = parser.parseFile(dbx_file);
  readHeaderIncludes(data.header.includes);
  dbChip* chip = createDesignTopChiplet(data.design);
  for (const auto& [_, chip_inst] : data.chiplet_instances) {
    createChipInst(chip_inst);
  }
  for (const auto& [_, connection] : data.connections) {
    createConnection(connection);
  }
  calculateSize(chip);
}

void ThreeDBlox::check()
{
  Checker checker(logger_);
  checker.check(db_->getChip());
}

namespace {
std::unordered_set<odb::dbTech*> getUsedTechs(odb::dbChip* chip)
{
  std::unordered_set<odb::dbTech*> techs;
  for (auto inst : chip->getChipInsts()) {
    if (inst->getMasterChip()->getTech() != nullptr) {
      techs.insert(inst->getMasterChip()->getTech());
    }
  }
  return techs;
}
std::unordered_set<odb::dbLib*> getUsedLibs(odb::dbChip* chip)
{
  std::unordered_set<odb::dbLib*> libs;
  for (auto inst : chip->getChipInsts()) {
    auto master_chip = inst->getMasterChip();
    if (master_chip->getBlock() != nullptr) {
      for (auto inst : master_chip->getBlock()->getInsts()) {
        libs.insert(inst->getMaster()->getLib());
      }
    }
  }
  return libs;
}
std::unordered_set<odb::dbChipRegion*> getChipletRegions(odb::dbChip* chip)
{
  std::unordered_set<odb::dbChipRegion*> regions;
  for (const auto chipinst : chip->getChipInsts()) {
    for (const auto region : chipinst->getMasterChip()->getChipRegions()) {
      regions.insert(region);
    }
  }
  return regions;
}
std::string getResultsDirectoryPath(const std::string& file_path)
{
  std::string current_dir_path;
  auto path = std::filesystem::path(file_path);
  if (path.has_parent_path()) {
    current_dir_path = path.parent_path().string() + "/";
  }
  return current_dir_path;
}
}  // namespace
void ThreeDBlox::writeDbv(const std::string& dbv_file, odb::dbChip* chip)
{
  if (chip == nullptr) {
    return;
  }
  ///////////Results Directory Path ///////////
  std::string current_dir_path = getResultsDirectoryPath(dbv_file);
  ////////////////////////////////////////////

  for (auto inst : chip->getChipInsts()) {
    auto master_chip = inst->getMasterChip();
    if (master_chip->getChipType() == odb::dbChip::ChipType::HIER) {
      writeDbx(current_dir_path + master_chip->getName() + ".3dbx",
               master_chip);
    }
  }
  // write used techs
  for (auto tech : getUsedTechs(chip)) {
    if (written_techs_.contains(tech)) {
      continue;
    }
    written_techs_.insert(tech);
    std::string tech_file_path = current_dir_path + tech->getName() + ".lef";
    utl::OutStreamHandler stream_handler(tech_file_path.c_str());
    odb::lefout lef_writer(logger_, stream_handler.getStream());
    lef_writer.writeTech(tech);
  }
  // write used libs
  for (auto lib : getUsedLibs(chip)) {
    if (written_libs_.contains(lib)) {
      continue;
    }
    written_libs_.insert(lib);
    std::string lib_file_path = current_dir_path + lib->getName() + "_lib.lef";
    utl::OutStreamHandler stream_handler(lib_file_path.c_str());
    odb::lefout lef_writer(logger_, stream_handler.getStream());
    lef_writer.writeLib(lib);
  }
  // write bmaps
  for (auto region : getChipletRegions(chip)) {
    std::string bmap_file_path = current_dir_path
                                 + std::string(region->getChip()->getName())
                                 + "_" + region->getName() + ".bmap";
    writeBMap(bmap_file_path, region);
  }

  DbvWriter writer(logger_, db_);
  writer.writeChiplet(dbv_file, chip);
}

void ThreeDBlox::writeDbx(const std::string& dbx_file, odb::dbChip* chip)
{
  if (chip == nullptr) {
    return;
  }
  ///////////Results Directory Path ///////////
  std::string current_dir_path = getResultsDirectoryPath(dbx_file);
  ////////////////////////////////////////////

  writeDbv(current_dir_path + chip->getName() + ".3dbv", chip);

  DbxWriter writer(logger_, db_);
  writer.writeChiplet(dbx_file, chip);
}

void ThreeDBlox::writeBMap(const std::string& bmap_file,
                           odb::dbChipRegion* region)
{
  BmapWriter writer(logger_);
  writer.writeFile(bmap_file, region);
}

void ThreeDBlox::calculateSize(dbChip* chip)
{
  Cuboid cuboid;
  cuboid.mergeInit();
  for (auto inst : chip->getChipInsts()) {
    cuboid.merge(inst->getCuboid());
  }
  chip->setWidth(cuboid.dx());
  chip->setHeight(cuboid.dy());
  chip->setThickness(cuboid.dz());
}

void ThreeDBlox::readHeaderIncludes(const std::vector<std::string>& includes)
{
  for (const auto& include : includes) {
    // Resolve full path to check against read_files_
    // Note: This logic assumes 'include' is relative to CWD or is absolute.
    // If recursively parsed files have includes relative to themselves,
    // the parser (DbxParser) handles finding them, but we might check the wrong
    // string here if we don't know the base path.
    // However, since we don't have base path info readily available without API
    // change, we use absolute() as a best-effort de-duplication key.
    std::string full_path = std::filesystem::absolute(include).string();
    if (!read_files_.insert(std::move(full_path)).second) {
      continue;
    }

    if (include.find(".3dbv") != std::string::npos) {
      readDbv(include);
    } else if (include.find(".3dbx") != std::string::npos) {
      readDbx(include);
    }
  }
}

static dbChip::ChipType getChipType(const std::string& type,
                                    utl::Logger* logger)
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

static std::string getFileName(const std::string& tech_file_path)
{
  std::filesystem::path tech_file_path_fs(tech_file_path);
  return tech_file_path_fs.stem().string();
}

void ThreeDBlox::createChiplet(const ChipletDef& chiplet)
{
  dbTech* tech = nullptr;

  // Read tech LEF file
  if (!chiplet.external.tech_lef_files.empty()) {
    if (chiplet.external.tech_lef_files.size() > 1) {
      logger_->error(
          utl::ODB,
          529,
          "3DBV Parser Error: Multiple tech LEF files are not supported");
    }
    auto tech_file = chiplet.external.tech_lef_files[0];
    auto tech_name = getFileName(tech_file);
    odb::lefin lef_reader(db_, logger_, false);
    tech = db_->findTech(tech_name.c_str());
    if (tech == nullptr) {
      lef_reader.createTechAndLib(
          tech_name.c_str(), tech_name.c_str(), tech_file.c_str());
      tech = db_->findTech(tech_name.c_str());
    }
  }
  // Read LEF files

  for (const auto& lef_file : chiplet.external.lef_files) {
    auto lib_name = getFileName(lef_file);
    if (db_->findLib(lib_name.c_str()) != nullptr) {
      continue;
    }
    odb::lefin lef_reader(db_, logger_, false);
    lef_reader.createLib(tech, lib_name.c_str(), lef_file.c_str());
  }
  if (sta_ != nullptr) {
    for (const auto& liberty_file : chiplet.external.lib_files) {
      sta_->readLiberty(
          liberty_file.c_str(), sta_->cmdScene(), sta::MinMaxAll::all(), true);
    }
  }
  // Check if chiplet already exists
  auto chip = db_->findChip(chiplet.name.c_str());
  if (chip != nullptr) {
    if (chip->getChipType() != getChipType(chiplet.type, logger_)
        || chip->getChipType() != dbChip::ChipType::HIER) {
      logger_->error(utl::ODB,
                     530,
                     "3DBV Parser Error: Chiplet {} already exists",
                     chiplet.name);
    }
    // chiplet already exists, update it
  } else {
    chip = dbChip::create(
        db_, tech, chiplet.name, getChipType(chiplet.type, logger_));
  }
  if (!chiplet.external.verilog_file.empty()) {
    if (odb::dbProperty::find(chip, "verilog_file") == nullptr) {
      odb::dbStringProperty::create(
          chip, "verilog_file", chiplet.external.verilog_file.c_str());
    }
  }
  // Read DEF file
  if (!chiplet.external.def_file.empty()) {
    odb::defin def_reader(db_, logger_, odb::defin::DEFAULT);
    std::vector<odb::dbLib*> search_libs;
    for (odb::dbLib* lib : db_->getLibs()) {
      search_libs.push_back(lib);
    }
    // No callbacks here as we are going to give one postRead3Dbx later
    def_reader.readChip(search_libs,
                        chiplet.external.def_file.c_str(),
                        chip,
                        /*issue_callback*/ false);
  }
  if (chiplet.design_width != -1.0) {
    chip->setWidth(chiplet.design_width * db_->getDbuPerMicron());
  }
  if (chiplet.design_height != -1.0) {
    chip->setHeight(chiplet.design_height * db_->getDbuPerMicron());
  }
  if (chiplet.thickness != -1.0) {
    chip->setThickness(chiplet.thickness * db_->getDbuPerMicron());
  }
  if (chiplet.shrink != -1.0) {
    chip->setShrink(chiplet.shrink);
  }
  chip->setTsv(chiplet.tsv);

  if (chiplet.scribe_line_right != -1.0) {
    chip->setScribeLineEast(chiplet.scribe_line_right * db_->getDbuPerMicron());
    chip->setScribeLineWest(chiplet.scribe_line_left * db_->getDbuPerMicron());
    chip->setScribeLineNorth(chiplet.scribe_line_top * db_->getDbuPerMicron());
    chip->setScribeLineSouth(chiplet.scribe_line_bottom
                             * db_->getDbuPerMicron());
  }
  if (chiplet.seal_ring_right != -1.0) {
    chip->setSealRingEast(chiplet.seal_ring_right * db_->getDbuPerMicron());
    chip->setSealRingWest(chiplet.seal_ring_left * db_->getDbuPerMicron());
    chip->setSealRingNorth(chiplet.seal_ring_top * db_->getDbuPerMicron());
    chip->setSealRingSouth(chiplet.seal_ring_bottom * db_->getDbuPerMicron());
  }

  chip->setOffset(Point(chiplet.offset.x * db_->getDbuPerMicron(),
                        chiplet.offset.y * db_->getDbuPerMicron()));
  if (chip->getChipType() != dbChip::ChipType::HIER
      && chip->getBlock() == nullptr) {
    // blackbox stage, create block
    auto block = odb::dbBlock::create(chip, chiplet.name.c_str());
    const int x_min = chip->getScribeLineWest() + chip->getSealRingWest();
    const int y_min = chip->getScribeLineSouth() + chip->getSealRingSouth();
    const int x_max = x_min + chip->getWidth();
    const int y_max = y_min + chip->getHeight();
    block->setDieArea(Rect(x_min, y_min, x_max, y_max));
    block->setCoreArea(Rect(x_min, y_min, x_max, y_max));
  }
  for (const auto& [_, region] : chiplet.regions) {
    createRegion(region, chip);
  }
}

static dbChipRegion::Side getChipRegionSide(const std::string& side,
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
    dbTech* tech = chip->getTech();
    if (tech) {
      layer = tech->findLayer(region.layer.c_str());
    }
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
  // Read bump map file
  if (!region.bmap.empty()) {
    BmapParser parser(logger_);
    BumpMapData data = parser.parseFile(region.bmap);
    for (const auto& entry : data.entries) {
      createBump(entry, chip_region);
    }
  }
}

void ThreeDBlox::createBump(const BumpMapEntry& entry,
                            dbChipRegion* chip_region)
{
  auto chip = chip_region->getChip();
  auto block = chip->getBlock();
  auto inst = block->findInst(entry.bump_inst_name.c_str());
  if (inst == nullptr) {
    // create inst
    auto master = db_->findMaster(entry.bump_cell_type.c_str());
    if (master == nullptr) {
      logger_->error(utl::ODB,
                     531,
                     "3DBV Parser Error: Bump cell type {} not found",
                     entry.bump_cell_type);
    }
    dbTech* master_tech = master->getLib()->getTech();
    if (master_tech != chip->getTech()) {
      logger_->error(utl::ODB,
                     532,
                     "3DBV Parser Error: Bump cell type {} is not in the same "
                     "tech as the chip region {}/{}",
                     entry.bump_cell_type,
                     chip->getName(),
                     chip_region->getName());
    }
    inst = dbInst::create(block, master, entry.bump_inst_name.c_str());
  }
  auto bump = dbChipBump::create(chip_region, inst);

  Rect bbox;
  inst->getMaster()->getPlacementBoundary(bbox);
  int x = (entry.x * db_->getDbuPerMicron()) - bbox.xCenter()
          + chip->getOffset().x();
  int y = (entry.y * db_->getDbuPerMicron()) - bbox.yCenter()
          + chip->getOffset().y();

  inst->setOrigin(x, y);
  inst->setPlacementStatus(dbPlacementStatus::FIRM);

  dbNet* net = nullptr;
  if (entry.net_name != "-") {
    net = block->findNet(entry.net_name.c_str());
    if (net == nullptr) {
      net = dbNet::create(block, entry.net_name.c_str());
      debugPrint(logger_,
                 utl::ODB,
                 "3dblox",
                 1,
                 "Creating missing net {} for bump {}",
                 entry.net_name,
                 entry.bump_inst_name);
    }
    bump->setNet(net);
    if (!inst->getITerms().empty()) {
      inst->getITerms().begin()->connect(net);
    }
  }
  if (entry.port_name != "-") {
    auto bterm = block->findBTerm(entry.port_name.c_str());
    if (bterm == nullptr) {
      if (net != nullptr) {
        bterm = dbBTerm::create(net, entry.port_name.c_str());
        debugPrint(logger_,
                   utl::ODB,
                   "3dblox",
                   1,
                   "Creating missing port {} for bump {}",
                   entry.port_name,
                   entry.bump_inst_name);
      } else {
        logger_->warn(utl::ODB,
                      545,
                      "Cannot create missing port {} for bump {} because no "
                      "net is specified.",
                      entry.port_name,
                      entry.bump_inst_name);
      }
    }
    if (bterm != nullptr) {
      bump->setBTerm(bterm);
      if (bump->getNet()) {
        bterm->connect(bump->getNet());
      }
    }
  }
}

dbChip* ThreeDBlox::createDesignTopChiplet(const DesignDef& design)
{
  dbChip* chip
      = dbChip::create(db_, nullptr, design.name, dbChip::ChipType::HIER);
  if (!design.external.verilog_file.empty()) {
    if (odb::dbProperty::find(chip, "verilog_file") == nullptr) {
      odb::dbStringProperty::create(
          chip, "verilog_file", design.external.verilog_file.c_str());
    }
  }
  db_->setTopChip(chip);
  return chip;
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
  auto orient_str = chip_inst.orient;
  if (dup_orient_map.contains(orient_str)) {
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
  inst->setLoc(Point3D{
      static_cast<int>(chip_inst.loc.x * db_->getDbuPerMicron()),
      static_cast<int>(chip_inst.loc.y * db_->getDbuPerMicron()),
      static_cast<int>(chip_inst.z * db_->getDbuPerMicron()),
  });
}
static std::vector<std::string> splitPath(const std::string& path)
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

  // Traverse hierarchy and find region
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

void ThreeDBlox::readBMap(const std::string& bmap_file)
{
  dbBlock* block = db_->getChip()->getBlock();

  BmapParser parser(logger_);
  BumpMapData data = parser.parseFile(bmap_file);
  std::vector<std::pair<odb::dbInst*, odb::dbBTerm*>> bumps;
  bumps.reserve(data.entries.size());
  for (const auto& entry : data.entries) {
    bumps.push_back(createBump(entry, block));
  }

  struct BPinInfo
  {
    dbTechLayer* layer = nullptr;
    odb::Rect rect;
  };

  // Populate where the bpins should be made
  std::map<odb::dbMaster*, BPinInfo> bpininfo;
  for (const auto& [inst, bterm] : bumps) {
    dbMaster* master = inst->getMaster();
    if (bpininfo.contains(master)) {
      continue;
    }

    odb::dbTechLayer* max_layer = nullptr;
    std::set<odb::Rect> top_shapes;

    for (dbMTerm* mterm : master->getMTerms()) {
      for (dbMPin* mpin : mterm->getMPins()) {
        for (dbBox* geom : mpin->getGeometry()) {
          auto* layer = geom->getTechLayer();
          if (layer == nullptr) {
            continue;
          }
          if (max_layer == nullptr) {
            max_layer = layer;
            top_shapes.insert(geom->getBox());
          } else if (max_layer->getRoutingLevel() <= layer->getRoutingLevel()) {
            if (max_layer->getRoutingLevel() < layer->getRoutingLevel()) {
              top_shapes.clear();
            }
            max_layer = layer;
            top_shapes.insert(geom->getBox());
          }
        }
      }
    }

    if (max_layer != nullptr) {
      odb::Rect master_box;
      master->getPlacementBoundary(master_box);
      const odb::Point center = master_box.center();
      const odb::Rect* top_shape_ptr = nullptr;
      for (const odb::Rect& shape : top_shapes) {
        if (shape.intersects(center)) {
          top_shape_ptr = &shape;
        }
      }

      if (top_shape_ptr == nullptr) {
        top_shape_ptr = &(*top_shapes.begin());
      }

      bpininfo.emplace(master,
                       BPinInfo{.layer = max_layer, .rect = *top_shape_ptr});
    }
  }

  // create bpins
  for (const auto& [inst, bterm] : bumps) {
    if (bterm == nullptr) {
      continue;
    }

    auto masterbpin = bpininfo.find(inst->getMaster());
    if (masterbpin == bpininfo.end()) {
      continue;
    }

    const BPinInfo& pin_info = masterbpin->second;

    const dbTransform xform = inst->getTransform();
    dbBPin* pin = dbBPin::create(bterm);
    Rect shape = pin_info.rect;
    xform.apply(shape);
    dbBox::create(pin,
                  pin_info.layer,
                  shape.xMin(),
                  shape.yMin(),
                  shape.xMax(),
                  shape.yMax());
    pin->setPlacementStatus(odb::dbPlacementStatus::FIRM);
  }
}

std::pair<dbInst*, odb::dbBTerm*> ThreeDBlox::createBump(
    const BumpMapEntry& entry,
    dbBlock* block)
{
  const int dbus = db_->getDbuPerMicron();
  dbInst* inst = block->findInst(entry.bump_inst_name.c_str());
  if (inst == nullptr) {
    // create inst
    dbMaster* master = db_->findMaster(entry.bump_cell_type.c_str());
    if (master == nullptr) {
      logger_->error(utl::ODB,
                     538,
                     "3DBV Parser Error: Bump cell type {} not found for {}",
                     entry.bump_cell_type,
                     entry.bump_inst_name);
    }
    inst = dbInst::create(block, master, entry.bump_inst_name.c_str());
  }
  inst->setOrigin(entry.x * dbus, entry.y * dbus);
  inst->setPlacementStatus(dbPlacementStatus::FIRM);

  dbNet* net = nullptr;
  dbBTerm* term = nullptr;

  // Find bterm
  if (entry.port_name != "-") {
    term = block->findBTerm(entry.port_name.c_str());
    if (term == nullptr) {
      logger_->error(utl::ODB,
                     539,
                     "3DBV Parser Error: Bump port {} not found for {}",
                     entry.port_name,
                     entry.bump_inst_name);
    }
    net = term->getNet();
  }

  // Find term via net
  if (term == nullptr && entry.net_name != "-") {
    net = block->findNet(entry.net_name.c_str());
    if (net == nullptr) {
      logger_->error(utl::ODB,
                     543,
                     "3DBV Parser Error: Bump net {} not found for {}",
                     entry.net_name,
                     entry.bump_inst_name);
    }
    if (net->getBTerms().empty()) {
      logger_->error(utl::ODB,
                     544,
                     "3DBV Parser Error: Bump net {} has no bterms for {}",
                     entry.net_name,
                     entry.bump_inst_name);
    }
    if (net->getBTerms().size() > 1) {
      logger_->error(
          utl::ODB,
          542,
          "3DBV Parser Error: Bump net {} has multiple bterms for {}",
          entry.net_name,
          entry.bump_inst_name);
    }
    term = net->get1stBTerm();
  }

  if (net != nullptr) {
    for (odb::dbITerm* iterm : inst->getITerms()) {
      iterm->connect(net);
    }
  }

  return {inst, term};
}

}  // namespace odb
