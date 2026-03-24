// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
//
// Standalone global_route binary.
// Links grt + ant + dpl + stt + dbSta + sta + odb + utl.
// First standalone binary to include static timing analysis.
//
// Usage:
//   global_route --read_db placed.odb --write_db grouted.odb [-verbose]
//   global_route --read_lef tech.lef --read_def placed.def --write_def
//   grouted.def

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "ant/AntennaChecker.hh"
#include "db_sta/dbSta.hh"
#include "dpl/Opendp.h"
#include "grt/GlobalRouter.h"
#include "odb/db.h"
#include "odb/defin.h"
#include "odb/defout.h"
#include "odb/lefin.h"
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
  std::string guide_file;
  // grt params (same flags as TCL)
  int congestion_iterations = 50;
  std::string congestion_report_file;
  int congestion_report_iter_step = 0;
  float critical_nets_percentage = -1;
  bool allow_congestion = false;
  bool verbose = false;
  bool use_cugr = false;
  bool resistance_aware = false;
};

void usage(const char* prog)
{
  std::cerr
      << "Usage: " << prog << " [options]\n"
      << "\n"
      << "I/O:\n"
      << "  --read_db FILE                  Read ODB database\n"
      << "  --read_lef FILE                 Read LEF (repeatable)\n"
      << "  --read_def FILE                 Read DEF\n"
      << "  --write_db FILE                 Write ODB database\n"
      << "  --write_def FILE                Write DEF\n"
      << "  -guide_file FILE                Write routing guides\n"
      << "\n"
      << "Routing options (same flags as TCL global_route):\n"
      << "  -congestion_iterations N        Max iterations (default: 50)\n"
      << "  -congestion_report_file FILE    Congestion report output\n"
      << "  -critical_nets_percentage N     Critical nets percentage\n"
      << "  -allow_congestion               Allow congested result\n"
      << "  -verbose                        Verbose output\n"
      << "  -use_cugr                       Use CUGR engine\n"
      << "  -resistance_aware               Resistance-aware routing\n";
}

bool parse_args(int argc, char* argv[], Options& opts)
{
  try {
    for (int i = 1; i < argc; i++) {
      std::string arg = argv[i];
      auto next_str = [&]() -> std::string {
        if (i + 1 >= argc) {
          throw std::invalid_argument(std::string(arg)
                                      + " requires an argument");
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
      } else if (arg == "-guide_file") {
        opts.guide_file = next_str();
      } else if (arg == "-congestion_iterations") {
        opts.congestion_iterations = std::stoi(next_str());
      } else if (arg == "-congestion_report_file") {
        opts.congestion_report_file = next_str();
      } else if (arg == "-congestion_report_iter_step") {
        opts.congestion_report_iter_step = std::stoi(next_str());
      } else if (arg == "-critical_nets_percentage") {
        opts.critical_nets_percentage = std::stof(next_str());
      } else if (arg == "-allow_congestion") {
        opts.allow_congestion = true;
      } else if (arg == "-verbose") {
        opts.verbose = true;
      } else if (arg == "-use_cugr") {
        opts.use_cugr = true;
      } else if (arg == "-resistance_aware") {
        opts.resistance_aware = true;
      } else if (arg == "-help" || arg == "--help" || arg == "-h") {
        usage(argv[0]);
        std::exit(0);
      } else {
        std::cerr << "Error: unknown argument: " << arg << "\n";
        usage(argv[0]);
        return false;
      }
    }
  } catch (const std::invalid_argument& e) {
    std::cerr << "Error: " << e.what() << '\n';
    return false;
  }

  bool has_db = !opts.read_db.empty();
  bool has_lef_def = !opts.read_lef.empty() || !opts.read_def.empty();
  if (!has_db && !has_lef_def) {
    std::cerr << "Error: specify --read_db or --read_lef/--read_def\n";
    return false;
  }
  if (has_db && has_lef_def) {
    std::cerr << "Error: cannot mix --read_db with --read_lef/--read_def\n";
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

  // Read input
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

  // Create dependencies
  // dbSta with nullptr Tcl_Interp — TCL not needed for routing.
  // Heap-allocated so we control destruction order (must die before db).
  auto sta = std::make_unique<sta::dbSta>(nullptr, db, &logger);
  utl::CallBackHandler cb_handler(&logger);
  stt::SteinerTreeBuilder stt_builder(db, &logger);
  ant::AntennaChecker antenna_checker(db, &logger);
  dpl::Opendp opendp(db, &logger);

  // Create GlobalRouter
  grt::GlobalRouter router(&logger,
                           &cb_handler,
                           &stt_builder,
                           db,
                           sta.get(),
                           &antenna_checker,
                           &opendp);

  // Configure
  router.setVerbose(opts.verbose);
  if (opts.use_cugr) {
    router.setUseCUGR(true);
  }
  if (opts.resistance_aware) {
    router.setResistanceAware(true);
  }
  if (opts.congestion_iterations != 50) {
    router.setCongestionIterations(opts.congestion_iterations);
  }
  if (opts.allow_congestion) {
    router.setAllowCongestion(true);
  }
  if (opts.critical_nets_percentage >= 0) {
    router.setCriticalNetsPercentage(opts.critical_nets_percentage);
  }
  if (!opts.congestion_report_file.empty()) {
    router.setCongestionReportFile(opts.congestion_report_file.c_str());
  }
  if (opts.congestion_report_iter_step > 0) {
    router.setCongestionReportIterStep(opts.congestion_report_iter_step);
  }

  // Run global routing
  router.globalRoute(true);

  // Write guide file
  if (!opts.guide_file.empty()) {
    router.writeSegments(opts.guide_file.c_str());
  }

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

  // _exit skips destructors. Needed because sta::ReportTcl::~ReportTcl()
  // calls Tcl_UnstackChannel on a nullptr Tcl_Interp, segfaulting.
  // This is a bug in sta's destructor — filed separately.
  _exit(0);
}
