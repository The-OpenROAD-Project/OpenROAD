// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
//
// Standalone init_floorplan binary.
// Links ifp + dbSta + sta + odb + utl.
//
// Usage:
//   init_floorplan --read_db synth.odb --write_db floorplan.odb \
//     -die_area "0 0 100 100" -core_area "10 10 90 90" -site
//     FreePDK45_38x28_10R_NP_162NW_34O
//   init_floorplan --read_db synth.odb --write_db floorplan.odb \
//     -utilization 50 -aspect_ratio 1.0 -core_space 10 -site
//     FreePDK45_38x28_10R_NP_162NW_34O

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "db_sta/dbSta.hh"
#include "ifp/InitFloorplan.hh"
#include "odb/db.h"
#include "odb/defin.h"
#include "odb/defout.h"
#include "odb/lefin.h"
#include "utl/Logger.h"

namespace {

struct Options
{
  std::vector<std::string> read_lef;
  std::string read_def;
  std::string read_db;
  std::string write_def;
  std::string write_db;
  // floorplan options (same flags as TCL)
  double utilization = -1;
  double aspect_ratio = 1.0;
  int core_space = -1;
  std::string die_area;   // "lx ly ux uy"
  std::string core_area;  // "lx ly ux uy"
  std::string site;
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
            << "Floorplan options (same flags as TCL initialize_floorplan):\n"
            << "  -utilization N              Target utilization (%)\n"
            << "  -aspect_ratio N             Aspect ratio (default: 1.0)\n"
            << "  -core_space N               Core spacing (um)\n"
            << "  -die_area \"lx ly ux uy\"     Die area coordinates\n"
            << "  -core_area \"lx ly ux uy\"    Core area coordinates\n"
            << "  -site NAME                  Site name\n";
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
      } else if (arg == "-utilization") {
        opts.utilization = std::stod(next());
      } else if (arg == "-aspect_ratio") {
        opts.aspect_ratio = std::stod(next());
      } else if (arg == "-core_space") {
        opts.core_space = std::stoi(next());
      } else if (arg == "-die_area") {
        opts.die_area = next();
      } else if (arg == "-core_area") {
        opts.core_area = next();
      } else if (arg == "-site") {
        opts.site = next();
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

odb::Rect parse_rect(const std::string& s, odb::dbBlock* block)
{
  std::istringstream iss(s);
  double lx, ly, ux, uy;
  if (!(iss >> lx >> ly >> ux >> uy)) {
    std::cerr << "Error: invalid rect format: " << s << "\n";
    std::exit(1);
  }
  int dbu = block->getDbUnitsPerMicron();
  return odb::Rect(static_cast<int>(lx * dbu),
                   static_cast<int>(ly * dbu),
                   static_cast<int>(ux * dbu),
                   static_cast<int>(uy * dbu));
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
    if (!opts.read_def.empty()) {
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
  }

  auto* chip = db->getChip();
  if (!chip || !chip->getBlock()) {
    std::cerr << "Error: no design loaded\n";
    odb::dbDatabase::destroy(db);
    return 1;
  }

  auto* block = chip->getBlock();
  auto sta = std::make_unique<sta::dbSta>(nullptr, db, &logger);

  ifp::InitFloorplan ifp(block, &logger, sta->getDbNetwork());

  // Find site
  odb::dbSite* site = nullptr;
  if (!opts.site.empty()) {
    for (auto* lib : db->getLibs()) {
      site = lib->findSite(opts.site.c_str());
      if (site) {
        break;
      }
    }
    if (!site) {
      std::cerr << "Error: site " << opts.site << " not found\n";
      _exit(1);
    }
  }

  if (!opts.die_area.empty() && !opts.core_area.empty()) {
    // Die/core area mode
    odb::Rect die = parse_rect(opts.die_area, block);
    odb::Rect core = parse_rect(opts.core_area, block);
    ifp.initFloorplan(die, core, site);
  } else if (opts.utilization >= 0) {
    // Utilization mode
    int space = opts.core_space >= 0
                    ? opts.core_space * block->getDbUnitsPerMicron()
                    : 0;
    ifp.initFloorplan(
        opts.utilization, opts.aspect_ratio, space, space, space, space, site);
  } else {
    std::cerr << "Error: specify -die_area/-core_area or -utilization\n";
    _exit(1);
  }

  if (!opts.write_db.empty()) {
    std::ofstream f(opts.write_db, std::ios::binary);
    db->write(f);
  }
  if (!opts.write_def.empty()) {
    odb::DefOut def_writer(&logger);
    def_writer.setVersion(odb::DefOut::Version::DEF_5_8);
    def_writer.writeBlock(block, opts.write_def.c_str());
  }

  _exit(0);
}
