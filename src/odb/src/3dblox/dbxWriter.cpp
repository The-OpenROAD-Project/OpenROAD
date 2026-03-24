// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbxWriter.h"

#include <string>
#include <vector>

#include "baseWriter.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "utl/Logger.h"
#include "yaml-cpp/yaml.h"

namespace odb {

DbxWriter::DbxWriter(utl::Logger* logger, odb::dbDatabase* db)
    : BaseWriter(logger, db)
{
}

void DbxWriter::writeChiplet(const std::string& filename, odb::dbChip* chiplet)
{
  YAML::Node root;
  writeYamlContent(root, chiplet);
  writeYamlToFile(filename, root);
}

void DbxWriter::writeYamlContent(YAML::Node& root, odb::dbChip* chiplet)
{
  YAML::Node header_node = root["Header"];
  writeHeader(header_node);
  YAML::Node includes_node = header_node["include"];
  includes_node.push_back(std::string(chiplet->getName()) + ".3dbv");

  YAML::Node design_node = root["Design"];
  writeDesign(design_node, chiplet);

  YAML::Node instances_node = root["ChipletInst"];
  writeChipletInsts(instances_node, chiplet);

  YAML::Node stack_node = root["Stack"];
  writeStack(stack_node, chiplet);

  YAML::Node connections_node = root["Connection"];
  writeConnections(connections_node, chiplet);
}

void DbxWriter::writeDesign(YAML::Node& design_node, odb::dbChip* chiplet)
{
  design_node["name"] = chiplet->getName();
}

void DbxWriter::writeChipletInsts(YAML::Node& instances_node,
                                  odb::dbChip* chiplet)
{
  for (auto inst : chiplet->getChipInsts()) {
    YAML::Node instance_node = instances_node[std::string(inst->getName())];
    writeChipletInst(instance_node, inst);
  }
}

void DbxWriter::writeChipletInst(YAML::Node& instance_node,
                                 odb::dbChipInst* inst)
{
  auto master_name = inst->getMasterChip()->getName();
  instance_node["reference"] = master_name;
}

void DbxWriter::writeStack(YAML::Node& stack_node, odb::dbChip* chiplet)
{
  for (auto inst : chiplet->getChipInsts()) {
    YAML::Node stack_instance_node = stack_node[std::string(inst->getName())];
    writeStackInstance(stack_instance_node, inst);
  }
}

void DbxWriter::writeStackInstance(YAML::Node& stack_instance_node,
                                   odb::dbChipInst* inst)
{
  const double u = inst->getDb()->getDbuPerMicron();
  const double loc_x = inst->getLoc().x() / u;
  const double loc_y = inst->getLoc().y() / u;
  YAML::Node loc_out;
  loc_out.SetStyle(YAML::EmitterStyle::Flow);
  loc_out.push_back(loc_x);
  loc_out.push_back(loc_y);
  stack_instance_node["loc"] = loc_out;
  stack_instance_node["z"] = inst->getLoc().z() / u;
  stack_instance_node["orient"] = inst->getOrient().getString();
}

void DbxWriter::writeConnections(YAML::Node& connections_node,
                                 odb::dbChip* chiplet)
{
  for (auto conn : chiplet->getChipConns()) {
    YAML::Node connection_node = connections_node[std::string(conn->getName())];
    writeConnection(connection_node, conn);
  }
}

void DbxWriter::writeConnection(YAML::Node& connection_node,
                                odb::dbChipConn* conn)
{
  const double u = conn->getDb()->getDbuPerMicron();
  connection_node["top"]
      = buildPath(conn->getTopRegionPath(), conn->getTopRegion());
  connection_node["bot"]
      = buildPath(conn->getBottomRegionPath(), conn->getBottomRegion());
  connection_node["thickness"] = conn->getThickness() / u;
}

std::string DbxWriter::buildPath(const std::vector<dbChipInst*>& path_insts,
                                 odb::dbChipRegionInst* region)
{
  if (region == nullptr) {
    return "~";
  }

  std::string path;
  for (auto inst : path_insts) {
    if (!path.empty()) {
      path += "/";
    }
    path += inst->getName();
  }

  if (!path.empty()) {
    path += ".regions." + region->getChipRegion()->getName();
  }
  return path;
}

}  // namespace odb