// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "odb/db.h"

namespace utl {
class Logger;
}

namespace odb::gds {

// Maps a (LEF layer name, purpose) pair to a (GDS layer, GDS datatype) pair.
// Purpose values: "NET", "PIN", "LABEL", "OBS", "BLK", "FILL", "VIA", etc.
// If no purpose-specific mapping exists, the default purpose "" is used.
struct GdsLayerSpec
{
  int16_t layer;
  int16_t datatype;
};

class LayerMap
{
 public:
  // Parse a layer map file in EDI/Innovus format:
  //   LEFLayerName  EDILayerType  GDSLayerNumber  GDSDataType
  // Lines starting with '#' are comments, blank lines are skipped.
  static LayerMap parseEdiLayerMap(const std::string& filename);

  // Parse a KLayout .lyt tech file's <lefdef><layer-map> section.
  // Extracts the layer_map('name : layer/datatype';...) entries.
  static LayerMap parseLytLayerMap(const std::string& filename);

  // Look up the GDS layer/datatype for a given LEF layer name and purpose.
  // Returns false if no mapping exists.
  bool lookup(const std::string& lef_layer,
              const std::string& purpose,
              GdsLayerSpec& spec) const;

  // Look up with just the layer name (defaults to "NET" purpose, then "").
  bool lookup(const std::string& lef_layer, GdsLayerSpec& spec) const;

  bool empty() const { return map_.empty(); }

 private:
  // Key: (LEF layer name, purpose), Value: GDS layer spec
  std::map<std::pair<std::string, std::string>, GdsLayerSpec> map_;
};

class DefToGds
{
 public:
  DefToGds(utl::Logger* logger);

  // Set the layer mapping used for converting design geometry to GDS layers.
  void setLayerMap(const LayerMap& map) { layer_map_ = map; }

  // Convert a dbBlock design to a dbGDSLib.
  // The block must have LEF/DEF loaded (tech layers, instances, wires, etc.)
  // The resulting GDS library has:
  //   - A top-level structure named after the block
  //   - SREFs for each instance (pointing to structures that will be empty
  //     until macro GDS files are merged)
  //   - Boundaries for routing wires, special wires, pins, fills,
  //     obstructions
  //   - Text labels for pin names
  dbGDSLib* convert(dbBlock* block, dbDatabase* db);

  // Merge additional GDS files into the library. Cell structures from the
  // files replace any empty placeholder structures created during convert().
  void mergeCells(dbGDSLib* lib, const std::vector<std::string>& gds_files);

  // Merge a seal GDS file as a child of the top cell.
  void mergeSeal(dbGDSLib* lib,
                 const std::string& top_cell_name,
                 const std::string& seal_file);

  // Validate the GDS library:
  //  - Flag empty cells (LEF cells with no GDS geometry)
  //  - Flag orphan cells (cells not referenced by the top cell tree)
  // Returns the number of errors found.
  // If allow_empty_regex is non-empty, empty cells matching the regex
  // are downgraded to warnings.
  int validate(dbGDSLib* lib,
               const std::string& top_cell_name,
               const std::string& allow_empty_regex = "");

 private:
  // Convert a rectangle on a tech layer to a GDS boundary.
  void addBoundary(dbGDSStructure* str,
                   dbTechLayer* layer,
                   const Rect& rect,
                   const std::string& purpose);

  // Convert an instance to a GDS SREF.
  void addInstance(dbGDSStructure* top_str, dbGDSLib* lib, dbInst* inst);

  // Add routing wire shapes for a net.
  void addWireShapes(dbGDSStructure* str, dbWire* wire);

  // Add special wire shapes for a net.
  void addSpecialWireShapes(dbGDSStructure* str, dbSWire* swire);

  // Collect all cell names referenced in the top cell's SREF tree.
  void collectReferencedCells(dbGDSStructure* str,
                              std::set<std::string>& referenced);

  utl::Logger* logger_;
  LayerMap layer_map_;
};

}  // namespace odb::gds
