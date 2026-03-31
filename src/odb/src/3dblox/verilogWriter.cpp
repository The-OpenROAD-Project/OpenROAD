// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "verilogWriter.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "odb/db.h"
#include "utl/Logger.h"

namespace odb {

VerilogWriter::VerilogWriter(utl::Logger* logger) : logger_(logger)
{
}

void VerilogWriter::writeChiplet(const std::string& filename, odb::dbChip* chip)
{
  std::ofstream verilog_file(filename);
  if (!verilog_file.is_open()) {
    logger_->error(
        utl::ODB, 563, "Unable to open Verilog file for writing: {}", filename);
    return;
  }

  // chip_inst -> list of (port_name, net_name)
  std::map<dbChipInst*, std::vector<std::pair<std::string, std::string>>>
      inst_connections;

  for (dbChipNet* net : chip->getChipNets()) {
    const std::string net_name = net->getName();
    const uint32_t num_bumps = net->getNumBumpInsts();
    for (uint32_t i = 0; i < num_bumps; i++) {
      std::vector<dbChipInst*> path;
      dbChipBumpInst* bump_inst = net->getBumpInst(i, path);
      if (bump_inst == nullptr || path.size() != 1) {
        continue;
      }
      // Only handle direct children (path length 1) — "single bump
      // connections".
      dbChipInst* chip_inst = path[0];
      dbChipBump* bump = bump_inst->getChipBump();
      if (bump == nullptr) {
        continue;
      }
      dbBTerm* bterm = bump->getBTerm();
      if (bterm == nullptr) {
        continue;
      }
      inst_connections[chip_inst].emplace_back(bterm->getName(), net_name);
    }
  }

  // Sort each instance's port connections alphabetically by port name.
  for (std::pair<dbChipInst* const,
                 std::vector<std::pair<std::string, std::string>>>& entry :
       inst_connections) {
    std::ranges::sort(entry.second);
  }

  // Write module header.
  fmt::print(verilog_file, "module {} ();\n", chip->getName());

  // Collect and sort net names alphabetically for deterministic wire order.
  std::vector<std::string> net_names;
  for (dbChipNet* net : chip->getChipNets()) {
    net_names.push_back(net->getName());
  }
  std::ranges::sort(net_names);

  // Write wire declarations in sorted order.
  for (const std::string& name : net_names) {
    fmt::print(verilog_file, "  wire {};\n", name);
  }

  // Collect and sort instances alphabetically by instance name.
  std::vector<dbChipInst*> chip_insts;
  for (dbChipInst* chip_inst : chip->getChipInsts()) {
    chip_insts.push_back(chip_inst);
  }
  std::ranges::sort(chip_insts, [](dbChipInst* a, dbChipInst* b) {
    return a->getName() < b->getName();
  });

  // Write instance declarations in sorted order.
  for (dbChipInst* chip_inst : chip_insts) {
    fmt::print(verilog_file,
               "  {} {} (\n",
               chip_inst->getMasterChip()->getName(),
               chip_inst->getName());
    auto it = inst_connections.find(chip_inst);
    if (it != inst_connections.end()) {
      const std::vector<std::pair<std::string, std::string>>& conns
          = it->second;
      for (size_t j = 0; j < conns.size(); j++) {
        const bool is_last = (j + 1 == conns.size());
        fmt::print(verilog_file,
                   "    .{}({}){}\n",
                   conns[j].first,
                   conns[j].second,
                   is_last ? "" : ",");
      }
    }
    fmt::print(verilog_file, "  );\n");
  }

  fmt::print(verilog_file, "endmodule\n");
  verilog_file.close();
}

}  // namespace odb
