// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
//
// Standalone tapcell binary.
// Links only tap + odb + utl.
//
// Usage:
//   tapcell --read_db floorplanned.odb --write_db tapped.odb \
//     -tapcell_master TAPCELL_X1 -endcap_master TAPCELL_X1 -distance 20

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "odb/db.h"
#include "odb/defin.h"
#include "odb/defout.h"
#include "odb/lefin.h"
#include "tap/tapcell.h"
#include "utl/Logger.h"

namespace {

struct Options
{
  std::vector<std::string> read_lef;
  std::string read_def;
  std::string read_db;
  std::string write_def;
  std::string write_db;
  // tapcell options (same flags as TCL)
  std::string tapcell_master;
  std::string endcap_master;
  int distance = -1;
  int halo_x = -1;
  int halo_y = -1;
};

void usage(const char* prog)
{
  std::cerr << "Usage: " << prog << " [options]\n"
            << "\n"
            << "I/O:\n"
            << "  --read_db FILE              Read ODB database\n"
            << "  --read_lef FILE             Read LEF (repeatable)\n"
            << "  --read_def FILE             Read DEF\n"
            << "  --write_db FILE             Write ODB database\n"
            << "  --write_def FILE            Write DEF\n"
            << "\n"
            << "Tapcell options (same flags as TCL):\n"
            << "  -tapcell_master NAME        Tapcell master cell\n"
            << "  -endcap_master NAME         Endcap master cell\n"
            << "  -distance N                 Distance between tapcells (um)\n"
            << "  -halo_x N                   Horizontal halo (um)\n"
            << "  -halo_y N                   Vertical halo (um)\n";
}

odb::dbMaster* find_master(odb::dbDatabase* db,
                           const std::string& name,
                           utl::Logger& logger)
{
  for (auto* lib : db->getLibs()) {
    auto* master = lib->findMaster(name.c_str());
    if (master) {
      return master;
    }
  }
  logger.error(utl::TAP, 100, "Cannot find master: {}", name);
  return nullptr;
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
      } else if (arg == "--write_db") {
        opts.write_db = next();
      } else if (arg == "--write_def") {
        opts.write_def = next();
      } else if (arg == "-tapcell_master") {
        opts.tapcell_master = next();
      } else if (arg == "-endcap_master") {
        opts.endcap_master = next();
      } else if (arg == "-distance") {
        opts.distance = std::stoi(next());
      } else if (arg == "-halo_x") {
        opts.halo_x = std::stoi(next());
      } else if (arg == "-halo_y") {
        opts.halo_y = std::stoi(next());
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

  tap::Tapcell tapcell(db, &logger);

  tap::Options tap_opts;
  if (!opts.tapcell_master.empty()) {
    tap_opts.tapcell_master = find_master(db, opts.tapcell_master, logger);
  }
  if (!opts.endcap_master.empty()) {
    tap_opts.endcap_master = find_master(db, opts.endcap_master, logger);
  }
  tap_opts.dist = opts.distance;
  tap_opts.halo_x = opts.halo_x;
  tap_opts.halo_y = opts.halo_y;

  tapcell.run(tap_opts);

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

  odb::dbDatabase::destroy(db);
  return 0;
}
