// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
//
// Standalone place_pins binary (IOPlacer).
// Links only ppl + dbSta + sta + odb + utl.
//
// Usage:
//   place_pins --read_db floorplanned.odb --write_db io_placed.odb

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "odb/defin.h"
#include "odb/defout.h"
#include "odb/lefin.h"
#include "ppl/IOPlacer.h"
#include "utl/Logger.h"

namespace {

struct Options
{
  std::vector<std::string> read_lef;
  std::string read_def;
  std::string read_db;
  std::string write_def;
  std::string write_db;
};

void usage(const char* prog)
{
  std::cerr << "Usage: " << prog << " [options]\n"
            << "\n"
            << "I/O:\n"
            << "  --read_db FILE          Read ODB database\n"
            << "  --read_lef FILE         Read LEF (repeatable)\n"
            << "  --read_def FILE         Read DEF\n"
            << "  --write_db FILE         Write ODB database\n"
            << "  --write_def FILE        Write DEF\n"
            << "\n"
            << "Pin placement is configured via the ODB state from prior\n"
            << "stages. For CLI flag control, use openroad_cmd fallback.\n";
}

bool parse_args(int argc, char* argv[], Options& opts)
{
  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];
    auto next = [&]() -> std::string {
      if (i + 1 >= argc) {
        std::cerr << "Error: " << arg << " requires an argument\n";
        std::exit(1);
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

  ppl::IOPlacer io_placer(db, &logger);
  io_placer.runHungarianMatching();

  if (!opts.write_db.empty()) {
    std::ofstream f(opts.write_db, std::ios::binary);
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
