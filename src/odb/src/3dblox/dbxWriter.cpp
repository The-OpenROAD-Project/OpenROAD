// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbxWriter.h"

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

#include "baseWriter.h"
#include "odb/db.h"
#include "odb/defout.h"
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
  auto path = std::filesystem::path(filename);
  if (path.has_parent_path()) {
    current_dir_path_ = path.parent_path().string() + "/";
  }
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
  YAML::Node external_node = design_node["external"];
  external_node["verilog_file"] = std::string(chiplet->getName()) + ".v";
}

void DbxWriter::writeChipletInsts(YAML::Node& instances_node,
                                  odb::dbChip* chiplet)
{
  // The DEF is master (design) data shared by all instances of a chiplet, so it
  // is emitted for exactly one instance per master chiplet. Choose that
  // instance deterministically as the one with the smallest name.
  std::unordered_map<odb::dbChip*, std::string> def_inst_by_master;
  for (auto inst : chiplet->getChipInsts()) {
    odb::dbChip* master = inst->getMasterChip();
    if (master->getBlock() == nullptr) {
      continue;
    }
    const std::string& name = inst->getName();
    auto it = def_inst_by_master.find(master);
    if (it == def_inst_by_master.end() || name < it->second) {
      def_inst_by_master[master] = name;
    }
  }

  for (auto inst : chiplet->getChipInsts()) {
    YAML::Node instance_node = instances_node[std::string(inst->getName())];
    auto it = def_inst_by_master.find(inst->getMasterChip());
    const bool write_def
        = it != def_inst_by_master.end() && it->second == inst->getName();
    writeChipletInst(instance_node, inst, write_def);
  }
}

void DbxWriter::writeChipletInst(YAML::Node& instance_node,
                                 odb::dbChipInst* inst,
                                 bool write_def)
{
  odb::dbChip* master = inst->getMasterChip();
  instance_node["reference"] = master->getName();

  // Per the 3DBlox standard the DEF file is attached to a chiplet instance.
  // Dump the master block to a DEF and reference it from a single instance of
  // each master chiplet.
  if (write_def) {
    std::string def_file = std::string(master->getName()) + ".def";
    odb::DefOut def_writer(logger_);
    def_writer.writeBlock(master->getBlock(),
                          (current_dir_path_ + def_file).c_str());
    YAML::Node external_node = instance_node["external"];
    external_node["def_file"] = def_file;
  }
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