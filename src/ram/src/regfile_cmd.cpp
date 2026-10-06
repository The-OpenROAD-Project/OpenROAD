// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <exception>
#include <string>
#include <vector>

#include "odb/db.h"
#include "odb/defout.h"
#include "ram/regfile.h"
#include "regfile.h"
#include "utl/Logger.h"
#include "views.h"

namespace ram {

void generateRegfile(odb::dbDatabase* db,
                     utl::Logger* logger,
                     const std::string& spec_path,
                     const std::string& verilog_path,
                     const std::string& def_path,
                     const std::string& lef_path,
                     const std::string& liberty_path,
                     const std::string& check_ports)
{
  try {
    regfile::Spec spec = regfile::ReadSpec(spec_path);
    if (!check_ports.empty()) {
      // The spec against the RTL it stands in for, before anything is
      // built: a block whose ports are not the module's is a silent
      // miswire at the parent.
      auto ports = regfile::ReadModulePorts(check_ports, spec.module);
      auto problems = regfile::CheckPorts(spec, ports);
      if (!problems.empty()) {
        std::string all;
        for (const auto& p : problems) {
          all += "\n  " + p;
        }
        throw std::runtime_error("spec does not match module " + spec.module
                                 + " in " + check_ports + ":" + all);
      }
    }
    odb::dbBlock* block = regfile::Generate(db, logger, spec);
    if (!verilog_path.empty()) {
      regfile::WriteVerilog(block, verilog_path);
    }
    if (!def_path.empty()) {
      odb::DefOut writer(logger);
      if (!writer.writeBlock(block, def_path.c_str())) {
        throw std::runtime_error("cannot write " + def_path);
      }
    }
    if (!lef_path.empty()) {
      regfile::WriteLef(block, logger, lef_path);
    }
    if (!liberty_path.empty()) {
      // A flow's memories directory holds <m>.lib and <m>_pre_layout.lib;
      // the model is the same file twice, said so in its comment.
      regfile::WriteLiberty(block, spec, spec.lib, liberty_path, false);
      std::string pre = liberty_path;
      auto dot = pre.rfind(".lib");
      if (dot != std::string::npos) {
        pre = pre.substr(0, dot) + "_pre_layout.lib";
        regfile::WriteLiberty(block, spec, spec.lib, pre, true);
      }
    }
  } catch (const std::exception& e) {
    logger->error(utl::RAM, 50, "generate_regfile: {}", e.what());
  }
}

}  // namespace ram
