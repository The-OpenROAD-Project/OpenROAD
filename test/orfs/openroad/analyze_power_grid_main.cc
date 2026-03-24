// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
//
// Standalone analyze_power_grid binary (PDNSim).
// Links psm + est + dpl + grt + stt + ant + dbSta + sta + odb + utl.
//
// Usage:
//   analyze_power_grid --read_db routed.odb -net VDD [-voltage_file v.csv]

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "ant/AntennaChecker.hh"
#include "db_sta/dbSta.hh"
#include "dpl/Opendp.h"
#include "est/EstimateParasitics.h"
#include "grt/GlobalRouter.h"
#include "odb/db.h"
#include "odb/defin.h"
#include "odb/lefin.h"
#include "psm/pdnsim.h"
#include "stt/SteinerTreeBuilder.h"
#include "utl/CallBackHandler.h"
#include "utl/Logger.h"

namespace {

struct Options
{
  std::vector<std::string> read_lef;
  std::string read_def;
  std::string read_db;
  std::string net;
  std::string voltage_file;
  std::string em_file;
  std::string error_file;
  bool enable_em = false;
};

void usage(const char* prog)
{
  std::cerr << "Usage: " << prog << " [options]\n"
            << "\n"
            << "I/O:\n"
            << "  --read_db FILE          Read ODB database\n"
            << "  --read_lef FILE         Read LEF (repeatable)\n"
            << "  --read_def FILE         Read DEF\n"
            << "\n"
            << "Options (same flags as TCL analyze_power_grid):\n"
            << "  -net NAME               Power net name (e.g. VDD)\n"
            << "  -voltage_file FILE      Voltage output file\n"
            << "  -em_file FILE           EM report file\n"
            << "  -error_file FILE        Error output file\n"
            << "  -enable_em              Enable EM analysis\n";
}

bool parse_args(int argc, char* argv[], Options& opts)
{
  try {
    for (int i = 1; i < argc; i++) {
      std::string arg = argv[i];
      auto next = [&]() -> std::string {
        if (i + 1 >= argc) {
          throw std::invalid_argument(std::string(arg)
                                      + " requires an argument");
        }
        return argv[++i];
      };

      if (arg == "--read_db") {
        opts.read_db = next();
      } else if (arg == "--read_lef") {
        opts.read_lef.push_back(next());
      } else if (arg == "--read_def") {
        opts.read_def = next();
      } else if (arg == "-net") {
        opts.net = next();
      } else if (arg == "-voltage_file") {
        opts.voltage_file = next();
      } else if (arg == "-em_file") {
        opts.em_file = next();
      } else if (arg == "-error_file") {
        opts.error_file = next();
      } else if (arg == "-enable_em") {
        opts.enable_em = true;
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

  if (opts.read_db.empty()) {
    std::cerr << "Error: --read_db is required\n";
    return false;
  }
  if (opts.net.empty()) {
    std::cerr << "Error: -net is required\n";
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

  std::ifstream f(opts.read_db, std::ios::binary);
  if (!f) {
    std::cerr << "Error: cannot open " << opts.read_db << "\n";
    return 1;
  }
  db->read(f);

  auto* chip = db->getChip();
  if (!chip || !chip->getBlock()) {
    std::cerr << "Error: no design loaded\n";
    return 1;
  }

  // Create dependency chain
  auto sta = std::make_unique<sta::dbSta>(nullptr, db, &logger);
  utl::CallBackHandler cb_handler(&logger);
  stt::SteinerTreeBuilder stt_builder(db, &logger);
  ant::AntennaChecker antenna_checker(db, &logger);
  dpl::Opendp opendp(db, &logger);
  grt::GlobalRouter global_router(&logger,
                                  &cb_handler,
                                  &stt_builder,
                                  db,
                                  sta.get(),
                                  &antenna_checker,
                                  &opendp);
  est::EstimateParasitics estimate_parasitics(
      &logger, &cb_handler, db, sta.get(), &stt_builder, &global_router);

  psm::PDNSim pdnsim(&logger, db, sta.get(), &estimate_parasitics, &opendp);

  // Find net
  auto* block = chip->getBlock();
  auto* net = block->findNet(opts.net.c_str());
  if (!net) {
    std::cerr << "Error: net " << opts.net << " not found\n";
    _exit(1);
  }

  pdnsim.analyzePowerGrid(net,
                          nullptr,  // corner (default)
                          psm::GeneratedSourceType::kFull,
                          opts.voltage_file,
                          false,  // use_prev_solution
                          opts.enable_em,
                          opts.em_file,
                          opts.error_file,
                          "");  // voltage_source_file

  _exit(0);
}
