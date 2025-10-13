// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbxWriter.h"

#include <fstream>
#include <string>

#include "objects.h"
#include "utl/Logger.h"

namespace odb {

DbxWriter::DbxWriter(utl::Logger* logger) : BaseWriter(logger)
{
}

void DbxWriter::writeFile(const std::string& filename, odb::dbDatabase* db)
{
  YAML::Node root;
  writeYamlContent(root, db);
  writeYamlToFile(filename, root);
}

void DbxWriter::writeYamlContent(YAML::Node& root, odb::dbDatabase* db)
{
  BaseWriter::writeYamlContent(root, db);

  YAML::Node design_node = root["Design"];
  writeDesign(design_node, db);

  YAML::Node instances_node = root["ChipletInst"];
  writeChipletInsts(instances_node, db);

  YAML::Node stack_node = root["Stack"];
  writeStack(stack_node, db);

  YAML::Node connections_node = root["Connection"];
  writeConnections(connections_node, db);
}

void DbxWriter::writeDesign(YAML::Node& design_node, odb::dbDatabase* db)
{
  // TODO: Implement design writing
  design_node["name"] = "TopDesign";

  YAML::Node external_node = design_node["external"];
  writeDesignExternal(external_node, db);
}

void DbxWriter::writeDesignExternal(YAML::Node& external_node,
                                    odb::dbDatabase* db)
{
  // TODO: Implement design external writing
  external_node["verilog_file"] = "/path/to/top.v";
}

void DbxWriter::writeChipletInsts(YAML::Node& instances_node,
                                  odb::dbDatabase* db)
{
  // TODO: Implement chiplet instances writing
  for (auto chiplet : db->getChips()) {
    YAML::Node instance_node
        = instances_node[std::string(chiplet->getName()) + "_inst"];
    writeChipletInst(instance_node, chiplet, db);
  }
}

void DbxWriter::writeChipletInst(YAML::Node& instance_node,
                                 odb::dbChip* chiplet,
                                 odb::dbDatabase* db)
{
  // TODO: Implement chiplet instance writing
  instance_node["reference"] = chiplet->getName();

  YAML::Node external_node = instance_node["external"];
  writeChipletInstExternal(external_node, chiplet, db);
}

void DbxWriter::writeChipletInstExternal(YAML::Node& external_node,
                                         odb::dbChip* chiplet,
                                         odb::dbDatabase* db)
{
  // TODO: Implement chiplet instance external writing
  external_node["verilog_file"]
      = "/path/to/" + std::string(chiplet->getName()) + ".v";
  external_node["sdc_file"]
      = "/path/to/" + std::string(chiplet->getName()) + ".sdc";
  external_node["def_file"]
      = "/path/to/" + std::string(chiplet->getName()) + ".def";
}

void DbxWriter::writeStack(YAML::Node& stack_node, odb::dbDatabase* db)
{
  // TODO: Implement stack writing
  int z_offset = 0;
  for (auto chiplet : db->getChips()) {
    YAML::Node stack_instance_node
        = stack_node[std::string(chiplet->getName()) + "_inst"];
    writeStackInstance(stack_instance_node, chiplet, db);
    z_offset += chiplet->getThickness() / (double) db->getDbuPerMicron();
  }
}

void DbxWriter::writeStackInstance(YAML::Node& stack_instance_node,
                                   odb::dbChip* chiplet,
                                   odb::dbDatabase* db)
{
  // TODO: Implement stack instance writing
  auto offset_x = chiplet->getOffset().getX() / db->getDbuPerMicron();
  auto offset_y = chiplet->getOffset().getY() / db->getDbuPerMicron();

  YAML::Emitter loc_out;
  loc_out << YAML::Flow << YAML::BeginSeq << offset_x << offset_y
          << YAML::EndSeq;
  stack_instance_node["loc"] = YAML::Load(loc_out.c_str());

  stack_instance_node["z"] = 0.0;        // TODO: Calculate proper z position
  stack_instance_node["orient"] = "R0";  // TODO: Get proper orientation
}

void DbxWriter::writeConnections(YAML::Node& connections_node,
                                 odb::dbDatabase* db)
{
  // TODO: Implement connections writing
}

void DbxWriter::writeConnection(YAML::Node& connection_node,
                                odb::dbDatabase* db)
{
  // TODO: Implement connection writing
}

}  // namespace odb