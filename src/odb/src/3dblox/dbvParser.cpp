// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbvParser.h"

#include <exception>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "baseParser.h"
#include "objects.h"
#include "odb/db.h"
#include "utl/Logger.h"
#include "yaml-cpp/yaml.h"

namespace odb {

DbvParser::DbvParser(utl::Logger* logger) : BaseParser(logger)
{
}

DbvData DbvParser::parseFile(const std::string& filename)
{
  current_file_path_ = filename;
  std::ifstream file(filename);
  if (!file.is_open()) {
    logError("3DBV Parser Error: Cannot open file: " + filename);
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string content = buffer.str();
  file.close();

  DbvData data;
  parseDefines(content);

  parseYamlContent(data, content);

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
    logError("3DBV YAML parsing error: " + std::string(e.what()));
  } catch (const std::exception& e) {
    logError("3DBV Error parsing YAML content: " + std::string(e.what()));
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
      logError("3DBV Error parsing chiplet name: " + std::string(e.what()));
    }
    const YAML::Node& chiplet_node = chiplet_pair.second;

    parseChiplet(chiplet, chiplet_node);
    chiplet_defs[chiplet.name] = chiplet;
  }
}

void DbvParser::parseChiplet(ChipletDef& chiplet,
                             const YAML::Node& chiplet_node)
{
  if (!chiplet_node["type"]) {
    logError("3DBV Chiplet type is required for chiplet " + chiplet.name);
  }
  extractValue(chiplet_node, "type", chiplet.type);

  if (chiplet_node["design_area"]) {
    std::vector<double> design_area;
    extractValue(chiplet_node, "design_area", design_area);
    if (design_area.size() == 2) {
      chiplet.design_width = design_area[0];
      chiplet.design_height = design_area[1];
    } else {
      logError("3DBV design_area must have 2 values for chiplet "
               + chiplet.name);
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
      logError("3DBV seal_ring_width must have 4 values for chiplet "
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
      logError(
          "3DBV scribe_line_remaining_width must have 4 values for chiplet "
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
          resolvePaths(lef_file.as<std::string>(), chiplet.external.lef_files);
        } catch (const YAML::Exception& e) {
          logError("3DBV Error parsing external LEF file name for chiplet "
                   + chiplet.name + ": " + std::string(e.what()));
        }
      }
    }
    if (chiplet_node["external"]["APR_tech_file"]) {
      for (const auto& tech_lef_file :
           chiplet_node["external"]["APR_tech_file"]) {
        try {
          resolvePaths(tech_lef_file.as<std::string>(),
                       chiplet.external.tech_lef_files);
        } catch (const YAML::Exception& e) {
          logError("3DBV Error parsing external APR tech file name for chiplet "
                   + chiplet.name + ": " + std::string(e.what()));
        }
      }
    }
    if (chiplet_node["external"]["liberty_file"]) {
      for (const auto& lib_file : chiplet_node["external"]["liberty_file"]) {
        try {
          resolvePaths(lib_file.as<std::string>(), chiplet.external.lib_files);
        } catch (const YAML::Exception& e) {
          logError("3DBV Error parsing external liberty file name for chiplet "
                   + chiplet.name + ": " + std::string(e.what()));
        }
      }
    }
    if (chiplet_node["external"]["DEF_file"]) {
      extractValue(
          chiplet_node["external"], "DEF_file", chiplet.external.def_file);
      chiplet.external.def_file = resolvePath(chiplet.external.def_file);
    }
    if (chiplet_node["external"]["verilog_file"]) {
      extractValue(chiplet_node["external"],
                   "verilog_file",
                   chiplet.external.verilog_file);
      chiplet.external.verilog_file
          = resolvePath(chiplet.external.verilog_file);
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
      logError("3DBV Error parsing region name: " + std::string(e.what()));
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
    region.bmap = resolvePath(region.bmap);
  }

  if (region_node["pmap"]) {
    extractValue(region_node, "pmap", region.pmap);
    region.pmap = resolvePath(region.pmap);
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

}  // namespace odb
