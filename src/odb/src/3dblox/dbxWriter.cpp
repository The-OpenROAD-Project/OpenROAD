// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbxWriter.h"

#include <yaml-cpp/emitterstyle.h>
#include <yaml-cpp/node/node.h>

#include <string>
#include <vector>

#include "baseWriter.h"
#include "dbvWriter.h"
#include "odb/db.h"
#include "utl/Logger.h"

namespace odb {

DbxWriter::DbxWriter(utl::Logger* logger) : BaseWriter(logger)
{
}

void DbxWriter::writeFile(const std::string& filename, odb::dbDatabase* db)
{
  DbvWriter dbvwriter(logger_);
  dbvwriter.writeFile(std::string(db->getChip()->getName()) + ".3dbv", db);
  YAML::Node root;
  writeYamlContent(root, db);
  writeYamlToFile(filename, root);
}

void DbxWriter::writeYamlContent(YAML::Node& root, odb::dbDatabase* db)
{
  YAML::Node header_node = root["Header"];
  writeHeader(header_node, db);
  YAML::Node includes_node = header_node["include"];
  includes_node.push_back(std::string(db->getChip()->getName()) + ".3dbv");

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
  design_node["name"] = db->getChip()->getName();
}

void DbxWriter::writeChipletInsts(YAML::Node& instances_node,
                                  odb::dbDatabase* db)
{
  for (auto chiplet : db->getChips()) {
    for (auto inst : chiplet->getChipInsts()) {
      YAML::Node instance_node = instances_node[std::string(inst->getName())];
      writeChipletInst(instance_node, inst, db);
    }
  }
}

void DbxWriter::writeChipletInst(YAML::Node& instance_node,
                                 odb::dbChipInst* inst,
                                 odb::dbDatabase* db)
{
  auto master_name = inst->getMasterChip()->getName();
  instance_node["reference"] = master_name;
  // TODO: Identify clone instances
  instance_node["is_master"] = true;
}

void DbxWriter::writeChipletInstExternal(YAML::Node& external_node,
                                         odb::dbChip* chiplet,
                                         odb::dbDatabase* db)
{
  BaseWriter::writeDef(external_node, db, chiplet);
}

void DbxWriter::writeStack(YAML::Node& stack_node, odb::dbDatabase* db)
{
  for (auto chiplet : db->getChips()) {
    for (auto inst : chiplet->getChipInsts()) {
      YAML::Node stack_instance_node = stack_node[std::string(inst->getName())];
      writeStackInstance(stack_instance_node, inst, db);
    }
  }
}

void DbxWriter::writeStackInstance(YAML::Node& stack_instance_node,
                                   odb::dbChipInst* inst,
                                   odb::dbDatabase* db)
{
  auto loc_x = inst->getLoc().x() / db->getDbuPerMicron();
  auto loc_y = inst->getLoc().y() / db->getDbuPerMicron();
  YAML::Node loc_out;
  loc_out.SetStyle(YAML::EmitterStyle::Flow);
  loc_out.push_back(loc_x);
  loc_out.push_back(loc_y);
  stack_instance_node["loc"] = loc_out;
  stack_instance_node["z"] = inst->getLoc().z() / db->getDbuPerMicron();
  stack_instance_node["orient"] = inst->getOrient().getString();
}

void DbxWriter::writeConnections(YAML::Node& connections_node,
                                 odb::dbDatabase* db)
{
  for (auto chiplet : db->getChips()) {
    for (auto conn : chiplet->getChipConns()) {
      YAML::Node connection_node
          = connections_node[std::string(conn->getName())];
      writeConnection(connection_node, conn, db);
    }
  }
}

void DbxWriter::writeConnection(YAML::Node& connection_node,
                                odb::dbChipConn* conn,
                                odb::dbDatabase* db)
{
  connection_node["top"]
      = buildPath(conn->getTopRegionPath(), conn->getTopRegion());
  connection_node["bot"]
      = buildPath(conn->getBottomRegionPath(), conn->getBottomRegion());
  connection_node["thicness"] = conn->getThickness() / db->getDbuPerMicron();
}

std::string DbxWriter::buildPath(const std::vector<dbChipInst*>& path_insts,
                                 odb::dbChipRegionInst* region)
{
  if (region == nullptr) {
    return "~";
  }

  std::string path = "";
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