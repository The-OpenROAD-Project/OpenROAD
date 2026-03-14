// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

#include "odb/def2gds.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <regex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>

#include "odb/db.h"
#include "odb/dbShape.h"
#include "odb/gdsin.h"
#include "utl/Logger.h"

namespace odb::gds {

// ---- LayerMap ----

LayerMap LayerMap::parseEdiLayerMap(const std::string& filename)
{
  LayerMap map;
  std::ifstream file(filename);
  if (!file.is_open()) {
    throw std::runtime_error("Cannot open layer map file: " + filename);
  }

  std::string line;
  while (std::getline(file, line)) {
    // Skip comments and blank lines
    if (line.empty() || line[0] == '#') {
      continue;
    }

    std::istringstream iss(line);
    std::string lef_name, edi_type;
    int gds_layer, gds_datatype;

    if (!(iss >> lef_name >> edi_type >> gds_layer >> gds_datatype)) {
      continue;  // Skip malformed lines
    }

    GdsLayerSpec spec;
    spec.layer = static_cast<int16_t>(gds_layer);
    spec.datatype = static_cast<int16_t>(gds_datatype);

    // Handle NAME entries which have format "Metal1/PIN"
    if (lef_name == "NAME") {
      const size_t slash = edi_type.find('/');
      if (slash != std::string::npos) {
        std::string name = edi_type.substr(0, slash);
        map.map_[{name, "LABEL"}] = spec;
      }
      continue;
    }

    // EDI type field can be comma-separated (e.g., "NET,SPNET,PIN,LEFPIN,VIA")
    // Split and create a mapping for each type.
    std::istringstream type_stream(edi_type);
    std::string single_type;
    while (std::getline(type_stream, single_type, ',')) {
      std::string purpose;
      if (single_type == "NET" || single_type == "SPNET") {
        purpose = "NET";
      } else if (single_type == "PIN" || single_type == "LEFPIN") {
        purpose = "PIN";
      } else if (single_type == "FILL" || single_type == "FILLOPC") {
        purpose = "FILL";
      } else if (single_type == "VIA" || single_type == "VIAFILL"
                 || single_type == "VIAFILLOPC") {
        purpose = "VIA";
      } else if (single_type == "LEFOBS") {
        purpose = "OBS";
      } else if (single_type == "ALL") {
        purpose = "";
      } else {
        purpose = single_type;
      }
      // Later entries override earlier ones (more specific lines come after
      // general comma-separated lines in EDI maps).
      map.map_[{lef_name, purpose}] = spec;
    }
  }

  return map;
}

LayerMap LayerMap::parseLytLayerMap(const std::string& filename)
{
  LayerMap map;
  std::ifstream file(filename);
  if (!file.is_open()) {
    throw std::runtime_error("Cannot open .lyt file: " + filename);
  }

  // Read entire file
  std::string content((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());

  // Find the lefdef layer-map section
  // Format: layer_map('name : layer/datatype';'name : layer/datatype';...)
  const std::string lefdef_tag = "<lefdef>";
  const std::string lefdef_end = "</lefdef>";
  auto lefdef_start = content.find(lefdef_tag);
  auto lefdef_stop = content.find(lefdef_end);

  if (lefdef_start == std::string::npos || lefdef_stop == std::string::npos) {
    return map;  // No lefdef section
  }

  std::string lefdef_section
      = content.substr(lefdef_start, lefdef_stop - lefdef_start);

  // Find the layer-map within lefdef section
  const std::string map_tag = "<layer-map>";
  const std::string map_end = "</layer-map>";
  auto map_start = lefdef_section.find(map_tag);
  auto map_stop = lefdef_section.find(map_end);

  if (map_start == std::string::npos || map_stop == std::string::npos) {
    return map;
  }

  std::string map_content = lefdef_section.substr(
      map_start + map_tag.size(), map_stop - map_start - map_tag.size());

  // Parse entries like: layer_map('metal1 : 11/0';'metal1.LABEL : 11/0';...)
  // Each entry is between single quotes: 'name : layer/datatype'
  std::regex entry_re("'([^']+)'");
  auto entries_begin
      = std::sregex_iterator(map_content.begin(), map_content.end(), entry_re);
  auto entries_end = std::sregex_iterator();

  for (auto it = entries_begin; it != entries_end; ++it) {
    std::string entry = (*it)[1].str();
    // Parse "name : layer/datatype"
    auto colon_pos = entry.find(':');
    if (colon_pos == std::string::npos) {
      continue;
    }
    std::string name = entry.substr(0, colon_pos);
    std::string layer_str = entry.substr(colon_pos + 1);

    // Trim whitespace
    auto trim = [](std::string& s) {
      s.erase(0, s.find_first_not_of(" \t"));
      s.erase(s.find_last_not_of(" \t") + 1);
    };
    trim(name);
    trim(layer_str);

    auto slash_pos = layer_str.find('/');
    if (slash_pos == std::string::npos) {
      continue;
    }

    int16_t gds_layer
        = static_cast<int16_t>(std::stoi(layer_str.substr(0, slash_pos)));
    int16_t gds_datatype
        = static_cast<int16_t>(std::stoi(layer_str.substr(slash_pos + 1)));

    GdsLayerSpec spec{.layer = gds_layer, .datatype = gds_datatype};

    // Split name into layer and purpose
    // e.g., "metal1.LABEL" -> layer="metal1", purpose="LABEL"
    // e.g., "metal1.PIN"   -> layer="metal1", purpose="PIN"
    // e.g., "metal1"       -> layer="metal1", purpose="NET"
    auto dot_pos = name.find('.');
    std::string lef_name;
    std::string purpose;
    if (dot_pos != std::string::npos) {
      lef_name = name.substr(0, dot_pos);
      purpose = name.substr(dot_pos + 1);
    } else {
      lef_name = name;
      purpose = "NET";
    }

    map.map_[{lef_name, purpose}] = spec;
  }

  // If the inline layer_map() was empty, check for a <map-file> reference
  // to an EDI-format layer map (e.g., ihp-sg13g2 uses this pattern).
  if (map.empty()) {
    const std::string mapfile_tag = "<map-file>";
    const std::string mapfile_end = "</map-file>";
    auto mf_start = content.find(mapfile_tag);
    auto mf_stop = content.find(mapfile_end);
    if (mf_start != std::string::npos && mf_stop != std::string::npos) {
      std::string map_path = content.substr(
          mf_start + mapfile_tag.size(),
          mf_stop - mf_start - mapfile_tag.size());
      // Trim whitespace
      map_path.erase(0, map_path.find_first_not_of(" \t\n\r"));
      map_path.erase(map_path.find_last_not_of(" \t\n\r") + 1);
      if (!map_path.empty()) {
        // Resolve relative paths against the .lyt file's directory
        std::filesystem::path mp(map_path);
        if (mp.is_relative()) {
          mp = std::filesystem::path(filename).parent_path() / mp;
        }
        if (std::filesystem::exists(mp)) {
          map = parseEdiLayerMap(mp.string());
        }
      }
    }
  }

  return map;
}

bool LayerMap::lookup(const std::string& lef_layer,
                      const std::string& purpose,
                      GdsLayerSpec& spec) const
{
  // Try exact match first
  auto it = map_.find({lef_layer, purpose});
  if (it != map_.end()) {
    spec = it->second;
    return true;
  }
  // KLayout .lyt files use lowercase purposes (e.g., "pin", "label")
  // while our code uses uppercase (e.g., "PIN", "LABEL").  Try lowercase.
  // But skip "net"/"via" — sky130 has both "met1.drawing" (68/20, primary)
  // and "met1.net" (68/23, secondary); we want the primary layer.
  std::string lower_purpose = purpose;
  std::transform(
      lower_purpose.begin(), lower_purpose.end(), lower_purpose.begin(),
      [](unsigned char c) { return std::tolower(c); });
  if (lower_purpose != purpose && lower_purpose != "net"
      && lower_purpose != "via") {
    it = map_.find({lef_layer, lower_purpose});
    if (it != map_.end()) {
      spec = it->second;
      return true;
    }
  }
  // KLayout uses "drawing" as the default purpose for routing geometry.
  // Fall back to "drawing" for purposes like "NET" and "VIA".
  it = map_.find({lef_layer, "drawing"});
  if (it != map_.end()) {
    spec = it->second;
    return true;
  }
  // Fallback to default purpose
  it = map_.find({lef_layer, ""});
  if (it != map_.end()) {
    spec = it->second;
    return true;
  }
  return false;
}

bool LayerMap::lookup(const std::string& lef_layer, GdsLayerSpec& spec) const
{
  return lookup(lef_layer, "NET", spec);
}

// ---- DefToGds ----

DefToGds::DefToGds(utl::Logger* logger) : logger_(logger)
{
}

void DefToGds::addBoundary(dbGDSStructure* str,
                           dbTechLayer* layer,
                           const Rect& rect,
                           const std::string& purpose)
{
  if (layer == nullptr) {
    return;
  }

  GdsLayerSpec spec;
  if (!layer_map_.lookup(layer->getName(), purpose, spec)) {
    // Try without purpose
    if (!layer_map_.lookup(layer->getName(), spec)) {
      return;  // No mapping for this layer
    }
  }

  dbGDSBoundary* bnd = dbGDSBoundary::create(str);
  bnd->setLayer(spec.layer);
  bnd->setDatatype(spec.datatype);

  // GDS boundary: 5 points for a rectangle (closed polygon)
  std::vector<Point> xy;
  xy.emplace_back(rect.xMin(), rect.yMin());
  xy.emplace_back(rect.xMax(), rect.yMin());
  xy.emplace_back(rect.xMax(), rect.yMax());
  xy.emplace_back(rect.xMin(), rect.yMax());
  xy.emplace_back(rect.xMin(), rect.yMin());
  bnd->setXy(xy);
}

void DefToGds::addInstance(dbGDSStructure* top_str, dbGDSLib* lib, dbInst* inst)
{
  const std::string cell_name = inst->getMaster()->getName();

  // Ensure a structure exists for this cell (may be empty placeholder)
  dbGDSStructure* cell_str = lib->findGDSStructure(cell_name.c_str());
  if (cell_str == nullptr) {
    cell_str = dbGDSStructure::create(lib, cell_name.c_str());
  }

  dbGDSSRef* sref = dbGDSSRef::create(top_str, cell_str);

  // Get the instance transform
  dbTransform transform = inst->getTransform();
  Point origin = transform.getOffset();

  sref->setOrigin(origin);

  // Convert dbOrientType to GDS transform
  dbGDSSTrans strans;
  strans.flipX_ = false;
  strans.mag_ = 1.0;
  strans.angle_ = 0.0;

  dbOrientType orient = transform.getOrient();
  switch (orient.getValue()) {
    case dbOrientType::R0:
      break;
    case dbOrientType::R90:
      strans.angle_ = 90.0;
      break;
    case dbOrientType::R180:
      strans.angle_ = 180.0;
      break;
    case dbOrientType::R270:
      strans.angle_ = 270.0;
      break;
    case dbOrientType::MY:
      strans.flipX_ = true;
      strans.angle_ = 180.0;
      break;
    case dbOrientType::MYR90:
      strans.flipX_ = true;
      strans.angle_ = 270.0;
      break;
    case dbOrientType::MX:
      strans.flipX_ = true;
      break;
    case dbOrientType::MXR90:
      strans.flipX_ = true;
      strans.angle_ = 90.0;
      break;
  }

  sref->setTransform(strans);
}

void DefToGds::addWireShapes(dbGDSStructure* str, dbWire* wire)
{
  if (wire == nullptr) {
    return;
  }

  dbWireShapeItr itr;
  dbShape shape;
  itr.begin(wire);

  while (itr.next(shape)) {
    if (shape.isVia()) {
      // Via shapes: expand into rectangles on each cut/metal layer
      std::vector<dbShape> via_shapes;
      dbShape::getViaBoxes(shape, via_shapes);
      for (const auto& vs : via_shapes) {
        addBoundary(str, vs.getTechLayer(), vs.getBox(), "VIA");
      }
    } else {
      addBoundary(str, shape.getTechLayer(), shape.getBox(), "NET");
    }
  }
}

void DefToGds::addSpecialWireShapes(dbGDSStructure* str, dbSWire* swire)
{
  for (dbSBox* sbox : swire->getWires()) {
    if (sbox->isVia()) {
      std::vector<dbShape> shapes;
      sbox->getViaBoxes(shapes);
      for (const auto& vs : shapes) {
        addBoundary(str, vs.getTechLayer(), vs.getBox(), "VIA");
      }
    } else {
      Rect rect = sbox->getBox();
      dbTechLayer* layer = sbox->getTechLayer();
      addBoundary(str, layer, rect, "NET");
    }
  }
}

dbGDSLib* DefToGds::convert(dbBlock* block, dbDatabase* db)
{
  const std::string design_name = block->getName();
  dbGDSLib* lib = createEmptyGDSLib(db, design_name);

  // Set GDS UNITS record based on the block's DEF units (DBU per micron).
  // GDS UNITS has two fields:
  //   1. uu_per_dbu: size of one database unit in user units (microns)
  //   2. dbu_per_meter: size of one database unit in meters
  // With user unit = 1 micron (standard convention):
  //   uu_per_dbu = 1e-6 / dbu_per_micron (microns per DBU)
  //   dbu_per_meter = 1e-6 / dbu_per_micron (meters per DBU)
  const int dbu_per_micron = block->getDefUnits();
  const double dbu_in_microns = 1.0 / dbu_per_micron;
  const double dbu_in_meters = 1e-6 / dbu_per_micron;
  lib->setUnits(dbu_in_microns, dbu_in_meters);

  dbGDSStructure* top_str = dbGDSStructure::create(lib, design_name.c_str());

  // Add instances as SREFs
  for (dbInst* inst : block->getInsts()) {
    addInstance(top_str, lib, inst);
  }

  // Add routing wire shapes
  for (dbNet* net : block->getNets()) {
    dbWire* wire = net->getWire();
    addWireShapes(top_str, wire);

    // Add special wires (power/ground straps etc.)
    for (dbSWire* swire : net->getSWires()) {
      addSpecialWireShapes(top_str, swire);
    }
  }

  // Add block pin shapes
  for (dbBTerm* bterm : block->getBTerms()) {
    for (dbBPin* bpin : bterm->getBPins()) {
      for (dbBox* box : bpin->getBoxes()) {
        dbTechLayer* layer = box->getTechLayer();
        Rect rect = box->getBox();
        addBoundary(top_str, layer, rect, "PIN");
      }
    }

    // Add label for the pin
    for (dbBPin* bpin : bterm->getBPins()) {
      for (dbBox* box : bpin->getBoxes()) {
        dbTechLayer* layer = box->getTechLayer();
        if (layer == nullptr) {
          continue;
        }

        GdsLayerSpec spec;
        if (!layer_map_.lookup(layer->getName(), "LABEL", spec)) {
          continue;
        }

        Rect rect = box->getBox();
        dbGDSText* text = dbGDSText::create(top_str);
        text->setLayer(spec.layer);
        text->setDatatype(spec.datatype);
        text->setText(bterm->getName());
        // Place text at center of pin box
        text->setOrigin(Point((rect.xMin() + rect.xMax()) / 2,
                              (rect.yMin() + rect.yMax()) / 2));
      }
      break;  // Only label the first BPin
    }
  }

  // Add fill shapes
  for (dbFill* fill : block->getFills()) {
    dbTechLayer* layer = fill->getTechLayer();
    Rect rect;
    fill->getRect(rect);
    addBoundary(top_str, layer, rect, "FILL");
  }

  // Add obstructions
  for (dbObstruction* obs : block->getObstructions()) {
    dbBox* bbox = obs->getBBox();
    dbTechLayer* layer = bbox->getTechLayer();
    Rect rect = bbox->getBox();
    addBoundary(top_str, layer, rect, "OBS");
  }

  return lib;
}

// Scale a Point by an integer ratio (numerator / denominator).
static Point scalePoint(const Point& p, int num, int denom)
{
  if (num == denom) {
    return p;
  }
  return Point(static_cast<int>(static_cast<int64_t>(p.x()) * num / denom),
               static_cast<int>(static_cast<int64_t>(p.y()) * num / denom));
}

// Scale a vector of Points by an integer ratio.
static std::vector<Point> scalePoints(const std::vector<Point>& pts,
                                      int num,
                                      int denom)
{
  if (num == denom) {
    return pts;
  }
  std::vector<Point> result;
  result.reserve(pts.size());
  for (const auto& p : pts) {
    result.push_back(scalePoint(p, num, denom));
  }
  return result;
}

void DefToGds::mergeCells(dbGDSLib* lib,
                          const std::vector<std::string>& gds_files)
{
  // Target DBU: extract from lib's UNITS (uu_per_dbu = microns per DBU).
  auto [target_uu, target_meters] = lib->getUnits();
  // target DBU per micron = 1/target_uu
  // source DBU per micron = 1/source_uu
  // scale = source_dbu_per_um / target_dbu_per_um = target_uu / source_uu

  for (const auto& gds_file : gds_files) {
    if (gds_file.empty()) {
      continue;
    }

    GDSReader reader(logger_);
    dbDatabase* temp_db = dbDatabase::create();
    dbGDSLib* source_lib = reader.read_gds(gds_file, temp_db);

    if (source_lib == nullptr) {
      logger_->warn(utl::ODB, 500, "Failed to read GDS file: {}", gds_file);
      dbDatabase::destroy(temp_db);
      continue;
    }

    // Compute integer scale factor from source DBU to target DBU.
    // Both DBU values are microns-per-unit, so to convert source coords
    // to target coords: multiply by (source_uu / target_uu).
    // For precision, use integer ratio: source_dbu_per_um / target_dbu_per_um.
    auto [source_uu, source_meters] = source_lib->getUnits();
    // Round to get integer DBU-per-micron values
    const int source_dbu_per_um = static_cast<int>(std::round(1.0 / source_uu));
    const int target_dbu_per_um = static_cast<int>(std::round(1.0 / target_uu));

    if (source_dbu_per_um != target_dbu_per_um) {
      logger_->info(utl::ODB,
                    504,
                    "Scaling merged GDS '{}' from {} to {} DBU/um",
                    gds_file,
                    source_dbu_per_um,
                    target_dbu_per_um);
    }

    const int scale_num = target_dbu_per_um;
    const int scale_denom = source_dbu_per_um;

    // Merge each structure from the source into our library
    for (dbGDSStructure* source_str : source_lib->getGDSStructures()) {
      const char* cell_name = source_str->getName();

      dbGDSStructure* target_str = lib->findGDSStructure(cell_name);
      if (target_str == nullptr) {
        target_str = dbGDSStructure::create(lib, cell_name);
      }

      for (dbGDSBoundary* bnd : source_str->getGDSBoundaries()) {
        dbGDSBoundary* new_bnd = dbGDSBoundary::create(target_str);
        new_bnd->setLayer(bnd->getLayer());
        new_bnd->setDatatype(bnd->getDatatype());
        new_bnd->setXy(scalePoints(bnd->getXY(), scale_num, scale_denom));
        new_bnd->getPropattr() = bnd->getPropattr();
      }

      for (dbGDSPath* path : source_str->getGDSPaths()) {
        dbGDSPath* new_path = dbGDSPath::create(target_str);
        new_path->setLayer(path->getLayer());
        new_path->setDatatype(path->getDatatype());
        new_path->setXy(scalePoints(path->getXY(), scale_num, scale_denom));
        new_path->setWidth(static_cast<int>(
            static_cast<int64_t>(path->getWidth()) * scale_num / scale_denom));
        new_path->setPathType(path->getPathType());
        new_path->getPropattr() = path->getPropattr();
      }

      for (dbGDSBox* box : source_str->getGDSBoxs()) {
        dbGDSBox* new_box = dbGDSBox::create(target_str);
        new_box->setLayer(box->getLayer());
        new_box->setDatatype(box->getDatatype());
        Rect bounds = box->getBounds();
        Point ll = scalePoint(Point(bounds.xMin(), bounds.yMin()),
                              scale_num,
                              scale_denom);
        Point ur = scalePoint(Point(bounds.xMax(), bounds.yMax()),
                              scale_num,
                              scale_denom);
        new_box->setBounds(Rect(ll.x(), ll.y(), ur.x(), ur.y()));
        new_box->getPropattr() = box->getPropattr();
      }

      for (dbGDSText* text : source_str->getGDSTexts()) {
        dbGDSText* new_text = dbGDSText::create(target_str);
        new_text->setLayer(text->getLayer());
        new_text->setDatatype(text->getDatatype());
        new_text->setText(text->getText());
        new_text->setOrigin(
            scalePoint(text->getOrigin(), scale_num, scale_denom));
        new_text->setTransform(text->getTransform());
        new_text->setPresentation(text->getPresentation());
        new_text->getPropattr() = text->getPropattr();
      }

      for (dbGDSSRef* sref : source_str->getGDSSRefs()) {
        const char* ref_name = sref->getStructure()->getName();
        dbGDSStructure* ref_str = lib->findGDSStructure(ref_name);
        if (ref_str == nullptr) {
          ref_str = dbGDSStructure::create(lib, ref_name);
        }
        dbGDSSRef* new_sref = dbGDSSRef::create(target_str, ref_str);
        new_sref->setOrigin(
            scalePoint(sref->getOrigin(), scale_num, scale_denom));
        new_sref->setTransform(sref->getTransform());
        new_sref->getPropattr() = sref->getPropattr();
      }

      for (dbGDSARef* aref : source_str->getGDSARefs()) {
        const char* ref_name = aref->getStructure()->getName();
        dbGDSStructure* ref_str = lib->findGDSStructure(ref_name);
        if (ref_str == nullptr) {
          ref_str = dbGDSStructure::create(lib, ref_name);
        }
        dbGDSARef* new_aref = dbGDSARef::create(target_str, ref_str);
        new_aref->setOrigin(
            scalePoint(aref->getOrigin(), scale_num, scale_denom));
        new_aref->setLr(scalePoint(aref->getLr(), scale_num, scale_denom));
        new_aref->setUl(scalePoint(aref->getUl(), scale_num, scale_denom));
        new_aref->setNumRows(aref->getNumRows());
        new_aref->setNumColumns(aref->getNumColumns());
        new_aref->setTransform(aref->getTransform());
        new_aref->getPropattr() = aref->getPropattr();
      }
    }

    dbDatabase::destroy(temp_db);
  }
}

void DefToGds::mergeSeal(dbGDSLib* lib,
                         const std::string& top_cell_name,
                         const std::string& seal_file)
{
  if (seal_file.empty()) {
    return;
  }

  dbGDSStructure* top_str = lib->findGDSStructure(top_cell_name.c_str());
  if (top_str == nullptr) {
    return;
  }

  // Record which structures existed before the merge so we can identify
  // which ones came from the seal file.
  std::set<std::string> pre_merge;
  for (dbGDSStructure* str : lib->getGDSStructures()) {
    pre_merge.insert(str->getName());
  }

  // Merge all seal cells into the main library (single file read)
  mergeCells(lib, {seal_file});

  // Find seal top cells: structures added by the merge that are not
  // referenced by any SREF/AREF within the seal's own structures.
  std::set<std::string> seal_referenced;
  for (dbGDSStructure* str : lib->getGDSStructures()) {
    if (pre_merge.count(str->getName())) {
      continue;  // skip pre-existing structures
    }
    for (dbGDSSRef* sref : str->getGDSSRefs()) {
      seal_referenced.insert(sref->getStructure()->getName());
    }
    for (dbGDSARef* aref : str->getGDSARefs()) {
      seal_referenced.insert(aref->getStructure()->getName());
    }
  }

  for (dbGDSStructure* str : lib->getGDSStructures()) {
    if (pre_merge.count(str->getName())) {
      continue;  // skip pre-existing structures
    }
    if (seal_referenced.count(str->getName())) {
      continue;  // referenced by another seal cell, not a top cell
    }
    if (str == top_str) {
      continue;
    }
    dbGDSSRef* sref = dbGDSSRef::create(top_str, str);
    sref->setOrigin(Point(0, 0));
    sref->setTransform(dbGDSSTrans(false, 1.0, 0.0));
    logger_->info(utl::ODB,
                  501,
                  "Merging '{}' as child of '{}'",
                  str->getName(),
                  top_cell_name);
  }
}

void DefToGds::collectReferencedCells(dbGDSStructure* str,
                                      std::set<std::string>& referenced)
{
  for (dbGDSSRef* sref : str->getGDSSRefs()) {
    std::string name = sref->getStructure()->getName();
    if (referenced.insert(name).second) {
      collectReferencedCells(sref->getStructure(), referenced);
    }
  }
  for (dbGDSARef* aref : str->getGDSARefs()) {
    std::string name = aref->getStructure()->getName();
    if (referenced.insert(name).second) {
      collectReferencedCells(aref->getStructure(), referenced);
    }
  }
}

int DefToGds::pruneUnreferencedCells(dbGDSLib* lib,
                                     const std::string& top_cell_name)
{
  dbGDSStructure* top_str = lib->findGDSStructure(top_cell_name.c_str());
  if (top_str == nullptr) {
    return 0;
  }

  // Collect all cells transitively referenced from the top cell
  std::set<std::string> referenced;
  referenced.insert(top_cell_name);
  collectReferencedCells(top_str, referenced);

  // Find unreferenced cells
  std::vector<dbGDSStructure*> to_remove;
  for (dbGDSStructure* str : lib->getGDSStructures()) {
    if (referenced.find(str->getName()) == referenced.end()) {
      to_remove.push_back(str);
    }
  }

  // Remove them
  for (dbGDSStructure* str : to_remove) {
    dbGDSStructure::destroy(str);
  }

  if (!to_remove.empty()) {
    logger_->info(
        utl::ODB, 505, "Pruned {} unreferenced cells.", to_remove.size());
  }

  return static_cast<int>(to_remove.size());
}

int DefToGds::validate(dbGDSLib* lib,
                       const std::string& top_cell_name,
                       const std::string& allow_empty_regex)
{
  int errors = 0;

  dbGDSStructure* top_str = lib->findGDSStructure(top_cell_name.c_str());
  if (top_str == nullptr) {
    logger_->error(utl::ODB, 502, "Top cell '{}' not found", top_cell_name);
    return 1;
  }

  std::regex allow_re;
  bool has_allow_re = !allow_empty_regex.empty();
  if (has_allow_re) {
    allow_re = std::regex(allow_empty_regex);
    logger_->info(utl::ODB, 503, "GDS_ALLOW_EMPTY={}", allow_empty_regex);
  }

  // Collect all cells referenced from the top cell tree
  std::set<std::string> referenced;
  referenced.insert(top_cell_name);
  collectReferencedCells(top_str, referenced);

  // Check for empty cells (LEF cells without GDS content)
  bool has_empty = false;
  for (dbGDSStructure* str : lib->getGDSStructures()) {
    if (str->getName() == top_cell_name) {
      continue;
    }

    bool is_empty = str->getGDSBoundaries().empty()
                    && str->getGDSPaths().empty() && str->getGDSBoxs().empty()
                    && str->getGDSSRefs().empty() && str->getGDSARefs().empty()
                    && str->getGDSTexts().empty();

    if (is_empty && referenced.count(str->getName())) {
      has_empty = true;
      if (has_allow_re && std::regex_match(str->getName(), allow_re)) {
        logger_->warn(utl::ODB,
                      504,
                      "LEF Cell '{}' ignored. Matches GDS_ALLOW_EMPTY.",
                      str->getName());
      } else {
        logger_->warn(utl::ODB,
                      505,
                      "LEF Cell '{}' has no matching GDS/OAS cell."
                      " Cell will be empty.",
                      str->getName());
        errors++;
      }
    }
  }

  if (!has_empty) {
    logger_->info(utl::ODB, 506, "All LEF cells have matching GDS/OAS cells");
  }

  // Check for orphan cells (cells not reachable from top cell)
  bool has_orphan = false;
  for (dbGDSStructure* str : lib->getGDSStructures()) {
    if (referenced.count(str->getName()) == 0) {
      has_orphan = true;
      logger_->warn(utl::ODB, 507, "Found orphan cell '{}'", str->getName());
      errors++;
    }
  }

  if (!has_orphan) {
    logger_->info(utl::ODB, 508, "No orphan cells in the final layout");
  }

  return errors;
}

}  // namespace odb::gds
