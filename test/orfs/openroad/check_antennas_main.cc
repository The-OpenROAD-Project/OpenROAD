// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
//
// Standalone check_antennas binary.
// Links only ant + odb + utl.
//
// Usage:
//   check_antennas --read_db routed.odb [-verbose] [-threads N]

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "ant/AntennaChecker.hh"
#include "odb/db.h"
#include "odb/defin.h"
#include "odb/lefin.h"
#include "utl/Logger.h"

namespace {

struct Options
{
  std::vector<std::string> read_lef;
  std::string read_def;
  std::string read_db;
  bool verbose = false;
  int num_threads = 1;
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
            << "Options:\n"
            << "  -verbose                Verbose output\n"
            << "  -threads N              Thread count (default: 1)\n";
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
      } else if (arg == "-verbose") {
        opts.verbose = true;
      } else if (arg == "-threads") {
        opts.num_threads = std::stoi(next());
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

  ant::AntennaChecker checker(db, &logger);
  int violations
      = checker.checkAntennas(nullptr, opts.num_threads, opts.verbose);

  std::cout << "Antenna violations: " << violations << "\n";

  odb::dbDatabase::destroy(db);
  return violations > 0 ? 1 : 0;
}
