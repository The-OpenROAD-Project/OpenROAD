// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
//
// Standalone repair_design binary.
// Links rsz + est + grt + dpl + stt + dbSta + sta + odb + utl.
//
// Usage:
//   repair_design --read_db placed.odb --write_db repaired.odb [-verbose]

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "ant/AntennaChecker.hh"
#include "db_sta/dbSta.hh"
#include "dpl/Opendp.h"
#include "est/EstimateParasitics.h"
#include "grt/GlobalRouter.h"
#include "odb/db.h"
#include "odb/defin.h"
#include "odb/defout.h"
#include "odb/lefin.h"
#include "rsz/Resizer.hh"
#include "stt/SteinerTreeBuilder.h"
#include "utl/CallBackHandler.h"
#include "utl/Logger.h"

namespace {

struct Options
{
  std::vector<std::string> read_lef;
  std::string read_def;
  std::string read_db;
  std::string write_def;
  std::string write_db;
  // repair_design flags (same as TCL)
  double max_wire_length = 0;
  double slew_margin = 0;
  double cap_margin = 0;
  double buffer_gain = 0;
  bool match_cell_footprint = false;
  bool verbose = false;
};

void usage(const char* prog)
{
  std::cerr
      << "Usage: " << prog << " [options]\n"
      << "\n"
      << "I/O:\n"
      << "  --read_db FILE              Read ODB database\n"
      << "  --read_lef FILE             Read LEF (repeatable)\n"
      << "  --read_def FILE             Read DEF\n"
      << "  --write_db FILE             Write ODB database\n"
      << "  --write_def FILE            Write DEF\n"
      << "\n"
      << "Options (same flags as TCL repair_design):\n"
      << "  -max_wire_length N          Max wire length (um)\n"
      << "  -slew_margin N              Slew margin (0.0-1.0)\n"
      << "  -cap_margin N               Cap margin (0.0-1.0)\n"
      << "  -match_cell_footprint       Match cell footprint\n"
      << "  -verbose                    Verbose output\n";
}

bool parse_args(int argc, char* argv[], Options& opts)
{
  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];
    auto next_str = [&]() -> std::string {
      if (i + 1 >= argc) {
        std::cerr << "Error: " << arg << " requires an argument\n";
        std::exit(1);
      }
      return argv[++i];
    };

    if (arg == "--read_db") {
      opts.read_db = next_str();
    } else if (arg == "--read_lef") {
      opts.read_lef.push_back(next_str());
    } else if (arg == "--read_def") {
      opts.read_def = next_str();
    } else if (arg == "--write_db") {
      opts.write_db = next_str();
    } else if (arg == "--write_def") {
      opts.write_def = next_str();
    } else if (arg == "-max_wire_length") {
      opts.max_wire_length = std::stod(next_str());
    } else if (arg == "-slew_margin") {
      opts.slew_margin = std::stod(next_str());
    } else if (arg == "-cap_margin") {
      opts.cap_margin = std::stod(next_str());
    } else if (arg == "-buffer_gain") {
      opts.buffer_gain = std::stod(next_str());
    } else if (arg == "-match_cell_footprint") {
      opts.match_cell_footprint = true;
    } else if (arg == "-verbose") {
      opts.verbose = true;
    } else if (arg == "-help" || arg == "--help" || arg == "-h") {
      usage(argv[0]);
      std::exit(0);
    } else {
      std::cerr << "Error: unknown argument: " << arg << "\n";
      usage(argv[0]);
      return false;
    }
  }

  bool has_db = !opts.read_db.empty();
  bool has_lef_def = !opts.read_lef.empty() || !opts.read_def.empty();
  if (!has_db && !has_lef_def) {
    std::cerr << "Error: specify --read_db or --read_lef/--read_def\n";
    return false;
  }
  return true;
}

}  // namespace

int main(int argc, char* argv[])
{
  Options opts;
  if (!parse_args(argc, argv, opts)) {
    return 1;
  }

  utl::Logger logger;
  auto* db = odb::dbDatabase::create();

  if (!opts.read_db.empty()) {
    std::ifstream f(opts.read_db, std::ios::binary);
    if (!f) {
      std::cerr << "Error: cannot open " << opts.read_db << "\n";
      return 1;
    }
    db->read(f);
  } else {
    odb::lefin lef_reader(db, &logger, false);
    for (const auto& lef_file : opts.read_lef) {
      const char* name = lef_file.c_str();
      if (!db->getTech()) {
        lef_reader.createTechAndLib(name, name, lef_file.c_str());
      } else {
        lef_reader.createLib(db->getTech(), name, lef_file.c_str());
      }
    }
    odb::defin def_reader(db, &logger);
    auto* tech = db->getTech();
    std::vector<odb::dbLib*> libs;
    for (auto* lib : db->getLibs()) {
      if (lib->getTech() == tech) {
        libs.push_back(lib);
      }
    }
    auto* chip = odb::dbChip::create(db, tech);
    def_reader.readChip(libs, opts.read_def.c_str(), chip);
  }

  auto* chip = db->getChip();
  if (!chip || !chip->getBlock()) {
    std::cerr << "Error: no design loaded\n";
    odb::dbDatabase::destroy(db);
    return 1;
  }

  // Create dependency chain
  auto sta = std::make_unique<sta::dbSta>(nullptr, db, &logger);
  utl::CallBackHandler cb_handler(&logger);
  stt::SteinerTreeBuilder stt_builder(db, &logger);
  ant::AntennaChecker antenna_checker(db, &logger);
  dpl::Opendp opendp(db, &logger);
  grt::GlobalRouter global_router(
      &logger, &cb_handler, &stt_builder, db, sta.get(),
      &antenna_checker, &opendp);
  est::EstimateParasitics estimate_parasitics(
      &logger, &cb_handler, db, sta.get(), &stt_builder, &global_router);

  rsz::Resizer resizer(&logger, db, sta.get(), &stt_builder,
                        &global_router, &opendp, &estimate_parasitics);

  // Run repair_design
  resizer.repairDesign(opts.max_wire_length, opts.slew_margin,
                       opts.cap_margin, opts.buffer_gain,
                       opts.match_cell_footprint, opts.verbose);

  // Write output
  if (!opts.write_db.empty()) {
    std::ofstream f(opts.write_db, std::ios::binary);
    if (!f) {
      std::cerr << "Error: cannot open " << opts.write_db << " for writing\n";
      return 1;
    }
    db->write(f);
  }
  if (!opts.write_def.empty()) {
    odb::DefOut def_writer(&logger);
    def_writer.setVersion(odb::DefOut::Version::DEF_5_8);
    def_writer.writeBlock(chip->getBlock(), opts.write_def.c_str());
  }

  // _exit: sta::ReportTcl::~ReportTcl() segfaults on nullptr Tcl_Interp
  _exit(0);
}
