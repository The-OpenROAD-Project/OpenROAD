// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbvParser.h"

#include <fstream>
#include <sstream>

#include "objects.h"
#include "odb/db.h"
#include "utl/Logger.h"

namespace odb {
std::string trim(const std::string& str)
{
  size_t start = str.find_first_not_of(" \t\n\r");
  if (start == std::string::npos) {
    return "";
  }
  size_t end = str.find_last_not_of(" \t\n\r");
  return str.substr(start, end - start + 1);
}

DbvParser::DbvParser(utl::Logger* logger) : logger_(logger)
{
}

DbvParser::~DbvParser() = default;

DbvData DbvParser::parseFile(const std::string& filename)
{
  std::ifstream file(filename);
  if (!file.is_open()) {
    logError("Cannot open file: " + filename);
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string content = buffer.str();
  file.close();

  // Extract include directives from the content
  std::istringstream stream(content);
  std::string line;
  std::ostringstream yaml_content;
  DbvData data;
  while (std::getline(stream, line)) {
    if (line.find("#!include") == 0) {
      std::string include_file = line.substr(9);  // Skip "#!include "
      include_file = trim(include_file);
      data.includes.push_back(include_file);
      // Skip include directives in YAML content
    } else {
      yaml_content << line << "\n";
    }
  }

  parseYamlContent(data, yaml_content.str());

  return data;
}

void DbvParser::parseYamlContent(DbvData& data, const std::string& content)
{
  try {
    YAML::Node root = YAML::Load(content);  // throws exception on error
    if (root["Header"]) {
      parseHeader(data.header, root["Header"]);
    }
    if (root["ChipletDef"]) {
      parseChipletDefs(data.chiplet_defs, root["ChipletDef"]);
    }
  } catch (const YAML::Exception& e) {
    logError("YAML parsing error: " + std::string(e.what()));
  } catch (const std::exception& e) {
    logError("Error parsing YAML content: " + std::string(e.what()));
  }
}

void DbvParser::parseHeader(DBVHeader& header, const YAML::Node& header_node)
{
  if (header_node["version"]) {
    extractValue(header_node, "version", header.version);
  }

  if (header_node["unit"]) {
    extractValue(header_node, "unit", header.unit);
  }

  if (header_node["precision"]) {
    extractValue(header_node, "precision", header.precision);
  }
  if (header_node["include"]) {
    extractValue(header_node, "include", header.includes);
  }
}

void DbvParser::parseChipletDefs(
    std::map<std::string, ChipletDef>& chiplet_defs,
    const YAML::Node& chiplets_node)
{
  for (const auto& chiplet_pair : chiplets_node) {
    ChipletDef chiplet;
    try {
      chiplet.name = chiplet_pair.first.as<std::string>();
    } catch (const YAML::Exception& e) {
      logError("Error parsing chiplet name: " + std::string(e.what()));
    }
    const YAML::Node& chiplet_node = chiplet_pair.second;

    parseChiplet(chiplet, chiplet_node);
    chiplet_defs[chiplet.name] = chiplet;
  }
}

void DbvParser::parseChiplet(ChipletDef& chiplet,
                             const YAML::Node& chiplet_node)
{
  if (chiplet_node["type"]) {
    extractValue(chiplet_node, "type", chiplet.type);
  }

  if (chiplet_node["design_area"]) {
    std::vector<double> design_area;
    extractValue(chiplet_node, "design_area", design_area);
    if (design_area.size() == 2) {
      chiplet.design_width = design_area[0];
      chiplet.design_height = design_area[1];
    } else {
      logError("design_area must have 2 values for chiplet " + chiplet.name);
    }
  }

  if (chiplet_node["seal_ring_width"]) {
    std::vector<double> seal_ring_width;
    extractValue(chiplet_node, "seal_ring_width", seal_ring_width);
    if (seal_ring_width.size() == 4) {
      chiplet.seal_ring_left = seal_ring_width[0];
      chiplet.seal_ring_bottom = seal_ring_width[1];
      chiplet.seal_ring_right = seal_ring_width[2];
      chiplet.seal_ring_top = seal_ring_width[3];
    } else {
      logError("seal_ring_width must have 4 values for chiplet "
               + chiplet.name);
    }
  }

  if (chiplet_node["scribe_line_remaining_width"]) {
    std::vector<double> scribe_line_remaining_width;
    extractValue(chiplet_node,
                 "scribe_line_remaining_width",
                 scribe_line_remaining_width);
    if (scribe_line_remaining_width.size() == 4) {
      chiplet.scribe_line_left = scribe_line_remaining_width[0];
      chiplet.scribe_line_bottom = scribe_line_remaining_width[1];
      chiplet.scribe_line_right = scribe_line_remaining_width[2];
      chiplet.scribe_line_top = scribe_line_remaining_width[3];
    } else {
      logError("scribe_line_remaining_width must have 4 values for chiplet "
               + chiplet.name);
    }
  }

  if (chiplet_node["thickness"]) {
    extractValue(chiplet_node, "thickness", chiplet.thickness);
  }

  if (chiplet_node["shrink"]) {
    extractValue(chiplet_node, "shrink", chiplet.shrink);
  }

  if (chiplet_node["tsv"]) {
    extractValue(chiplet_node, "tsv", chiplet.tsv);
  }

  // Parse regions
  if (chiplet_node["regions"]) {
    parseRegions(chiplet.regions, chiplet_node["regions"]);
  }
  // Parse external references
  if (chiplet_node["external"]) {
    if (chiplet_node["external"]["LEF_file"]) {
      for (const auto& lef_file : chiplet_node["external"]["LEF_file"]) {
        try {
          chiplet.external.lef_files.push_back(lef_file.as<std::string>());
        } catch (const YAML::Exception& e) {
          logError("Error parsing extrernal LEF file name for chiplet "
                   + chiplet.name + ": " + std::string(e.what()));
        }
      }
    }
    if (chiplet_node["external"]["DEF_file"]) {
      extractValue(
          chiplet_node["external"], "DEF_file", chiplet.external.def_file);
    }
  }
}

void DbvParser::parseRegions(std::map<std::string, ChipletRegion>& regions,
                             const YAML::Node& regions_node)
{
  for (const auto& region_pair : regions_node) {
    ChipletRegion region;
    try {
      region.name = region_pair.first.as<std::string>();
    } catch (const YAML::Exception& e) {
      logError("Error parsing region name: " + std::string(e.what()));
    }
    const YAML::Node& region_node = region_pair.second;
    parseRegion(region, region_node);
    regions[region.name] = region;
  }
}

void DbvParser::parseRegion(ChipletRegion& region,
                            const YAML::Node& region_node)
{
  if (region_node["bmap"]) {
    extractValue(region_node, "bmap", region.bmap);
  }

  if (region_node["pmap"]) {
    extractValue(region_node, "pmap", region.pmap);
  }

  if (region_node["side"]) {
    extractValue(region_node, "side", region.side);
  }

  if (region_node["layer"]) {
    extractValue(region_node, "layer", region.layer);
  }

  if (region_node["gds_layer"]) {
    extractValue(region_node, "gds_layer", region.gds_layer);
  }

  if (region_node["coords"]) {
    parseCoordinates(region.coords, region_node["coords"]);
  }
}

void DbvParser::parseCoordinates(std::vector<Coordinate>& coords,
                                 const YAML::Node& coords_node)
{
  coords.clear();

  try {
    if (coords_node.IsSequence()) {
      for (const auto& coord_pair : coords_node) {
        if (coord_pair.IsSequence() && coord_pair.size() >= 2) {
          double x = coord_pair[0].as<double>();
          double y = coord_pair[1].as<double>();
          coords.emplace_back(x, y);
        }
      }
    }
  } catch (const YAML::Exception& e) {
    logError("Error parsing coordinates: " + std::string(e.what()));
  }
}

template <typename T>
void DbvParser::extractValue(const YAML::Node& node,
                             const std::string& key,
                             T& value)
{
  try {
    value = node[key].as<T>();
  } catch (const YAML::Exception& e) {
    logError("Error parsing value for " + key + ": " + std::string(e.what()));
  }
}

void DbvParser::logError(const std::string& message)
{
  logger_->error(utl::ODB, 514, "3DBV Parser Error: {}", message);
}

}  // namespace odb
