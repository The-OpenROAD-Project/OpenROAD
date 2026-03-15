// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "verilogWriter.h"

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
      if (bump_inst == nullptr || path.empty()) {
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

  // Write module header.
  fmt::print(verilog_file, "module {} ();\n", chip->getName());

  // Write wire declarations for each net.
  for (dbChipNet* net : chip->getChipNets()) {
    fmt::print(verilog_file, "  wire {};\n", net->getName());
  }

  // Write instance declarations.
  for (dbChipInst* chip_inst : chip->getChipInsts()) {
    fmt::print(verilog_file,
               "  {} {} (\n",
               chip_inst->getMasterChip()->getName(),
               chip_inst->getName());
    const auto it = inst_connections.find(chip_inst);
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
