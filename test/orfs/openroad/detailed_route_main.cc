// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
//
// Standalone detailed_route binary.
// Links drt + dst + stt + odb + utl — no TCL, no framework.
//
// Usage:
//   detailed_route --read_db grouted.odb --write_db routed.odb [-verbose 1]
//   detailed_route --read_lef tech.lef --read_lef cells.lef \
//     --read_def preroute.def --read_guides route.guide \
//     --write_def routed.def [-verbose 1]

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "AbstractGraphicsFactory.h"
#include "drt/TritonRoute.h"
#include "dst/Distributed.h"
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

  // DRT params (same flags as TCL)
  std::string output_drc;
  std::string output_maze;
  std::string output_cmap;
  std::string output_guide_coverage;
  std::string db_process_node;
  int droute_end_iter = -1;
  std::string via_in_pin_bottom_layer;
  std::string via_in_pin_top_layer;
  std::string via_access_layer;
  std::string bottom_routing_layer;
  std::string top_routing_layer;
  int or_seed = 0;
  double or_k = 0;
  int verbose = 1;
  bool enable_via_gen = true;
  bool clean_patches = false;
  bool no_pin_access = false;
  int min_access_points = -1;
  std::string repair_pdn_vias;
  int num_threads = 0;
};

void usage(const char* prog)
{
  std::cerr << "Usage: " << prog << " [options]\n"
            << "\n"
            << "I/O:\n"
            << "  --read_db FILE              Read ODB (with guides from "
               "global_route)\n"
            << "  --read_lef FILE             Read LEF (repeatable)\n"
            << "  --read_def FILE             Read DEF\n"
            << "  --write_db FILE             Write ODB\n"
            << "  --write_def FILE            Write DEF\n"
            << "\n"
            << "Routing options (same flags as TCL detailed_route):\n"
            << "  -output_drc FILE            DRC report output\n"
            << "  -output_maze FILE           Maze debug output\n"
            << "  -db_process_node NAME       PDK process node\n"
            << "  -droute_end_iter N          Max routing iterations\n"
            << "  -via_in_pin_bottom_layer L  Bottom via layer for pin access\n"
            << "  -via_in_pin_top_layer L     Top via layer for pin access\n"
            << "  -bottom_routing_layer L     Bottom routing layer\n"
            << "  -top_routing_layer L        Top routing layer\n"
            << "  -or_seed N                  Random seed\n"
            << "  -or_k N                     OR parameter\n"
            << "  -verbose N                  Verbosity (default: 1)\n"
            << "  -disable_via_gen            Disable via generation\n"
            << "  -clean_patches              Clean patches\n"
            << "  -no_pin_access              Skip pin access\n"
            << "  -min_access_points N        Min access points\n"
            << "  -repair_pdn_vias LAYER      Repair PDN vias\n"
            << "  -threads N                  Thread count\n";
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
    auto next_int = [&]() -> int { return std::stoi(next_str()); };
    auto next_dbl = [&]() -> double { return std::stod(next_str()); };

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
    } else if (arg == "-output_drc") {
      opts.output_drc = next_str();
    } else if (arg == "-output_maze") {
      opts.output_maze = next_str();
    } else if (arg == "-output_cmap") {
      opts.output_cmap = next_str();
    } else if (arg == "-output_guide_coverage") {
      opts.output_guide_coverage = next_str();
    } else if (arg == "-db_process_node") {
      opts.db_process_node = next_str();
    } else if (arg == "-droute_end_iter") {
      opts.droute_end_iter = next_int();
    } else if (arg == "-via_in_pin_bottom_layer") {
      opts.via_in_pin_bottom_layer = next_str();
    } else if (arg == "-via_in_pin_top_layer") {
      opts.via_in_pin_top_layer = next_str();
    } else if (arg == "-via_access_layer") {
      opts.via_access_layer = next_str();
    } else if (arg == "-bottom_routing_layer") {
      opts.bottom_routing_layer = next_str();
    } else if (arg == "-top_routing_layer") {
      opts.top_routing_layer = next_str();
    } else if (arg == "-or_seed") {
      opts.or_seed = next_int();
    } else if (arg == "-or_k") {
      opts.or_k = next_dbl();
    } else if (arg == "-verbose") {
      opts.verbose = next_int();
    } else if (arg == "-disable_via_gen") {
      opts.enable_via_gen = false;
    } else if (arg == "-clean_patches") {
      opts.clean_patches = true;
    } else if (arg == "-no_pin_access") {
      opts.no_pin_access = true;
    } else if (arg == "-min_access_points") {
      opts.min_access_points = next_int();
    } else if (arg == "-repair_pdn_vias") {
      opts.repair_pdn_vias = next_str();
    } else if (arg == "-threads") {
      opts.num_threads = next_int();
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
  utl::CallBackHandler cb_handler(&logger);
  dst::Distributed dist(&logger);
  stt::SteinerTreeBuilder stt_builder(db, &logger);

  // Create TritonRoute
  drt::TritonRoute router(db, &logger, &cb_handler, &dist, &stt_builder);

  // Provide a no-op graphics factory (required — initDesign calls initGraphics)
  class NoOpGraphicsFactory : public drt::AbstractGraphicsFactory
  {
   public:
    void reset(drt::frDebugSettings*,
               drt::frDesign*,
               odb::dbDatabase*,
               utl::Logger*,
               drt::RouterConfiguration*) override
    {
    }
    bool guiActive() override { return false; }
    std::unique_ptr<drt::AbstractDRGraphics> makeUniqueDRGraphics() override
    {
      return nullptr;
    }
    std::unique_ptr<drt::AbstractTAGraphics> makeUniqueTAGraphics() override
    {
      return nullptr;
    }
    std::unique_ptr<drt::AbstractPAGraphics> makeUniquePAGraphics() override
    {
      return nullptr;
    }
  };
  router.initGraphics(std::make_unique<NoOpGraphicsFactory>());

  // Set parameters (num_threads must be initialized — uninitialized causes
  // hangs)
  drt::ParamStruct params;
  params.num_threads
      = std::max(1, static_cast<int>(std::thread::hardware_concurrency()));
  params.outputMazeFile = opts.output_maze;
  params.outputDrcFile = opts.output_drc;
  params.outputCmapFile = opts.output_cmap;
  params.outputGuideCoverageFile = opts.output_guide_coverage;
  params.dbProcessNode = opts.db_process_node;
  params.enableViaGen = opts.enable_via_gen;
  params.drouteEndIter = opts.droute_end_iter;
  params.viaInPinBottomLayer = opts.via_in_pin_bottom_layer;
  params.viaInPinTopLayer = opts.via_in_pin_top_layer;
  params.viaAccessLayer = opts.via_access_layer;
  params.orSeed = opts.or_seed;
  params.orK = opts.or_k;
  params.verbose = opts.verbose;
  params.cleanPatches = opts.clean_patches;
  params.doPa = !opts.no_pin_access;
  params.minAccessPoints = opts.min_access_points;
  params.repairPDNLayerName = opts.repair_pdn_vias;
  if (opts.num_threads > 0) {
    params.num_threads = opts.num_threads;
  }
  router.setParams(params);

  // Run detailed routing
  router.main();

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
