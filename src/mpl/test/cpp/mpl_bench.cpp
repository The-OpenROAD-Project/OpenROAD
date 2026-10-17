// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// mpl_bench: run the macro placer on a floorplanned design (an ORFS
// 2_1_floorplan.odb) as many times and with as large an annealing budget as
// asked, and print one CSV row per run: wall time, whether it placed, and
// the half-perimeter wirelength of the nets that touch a macro, a quality
// measure independent of mpl's own cost. Not a test: long runs belong on a
// developer's machine, not in CI.
//
//   mpl_bench <odb> [--repeats N] [--steps S] [--threads T]
//             [--channel W H] [--pin-aware] [--target-util U]
//             [--max-num-level L] [--seed-runs-from-odb-each-time]

#include <algorithm>
#include <chrono>
#include <climits>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "db_sta/dbSta.hh"
#include "mpl/rtl_mp.h"
#include "odb/db.h"
#include "par/PartitionMgr.h"
#include "sta/Sta.hh"
#include "tcl.h"
#include "utl/Logger.h"

namespace {

// Sum over nets with a pin on a block instance of the bounding box
// half-perimeter of all their pins, in microns.
double macroNetHpwl(odb::dbBlock* block)
{
  double total = 0;
  for (odb::dbNet* net : block->getNets()) {
    if (net->getSigType().isSupply()) {
      continue;
    }
    bool touches_macro = false;
    odb::Rect box;
    box.mergeInit();
    for (odb::dbITerm* iterm : net->getITerms()) {
      touches_macro |= iterm->getInst()->isBlock();
      int x, y;
      if (iterm->getAvgXY(&x, &y)) {
        box.merge(odb::Rect(x, y, x, y));
      }
    }
    if (!touches_macro) {
      continue;
    }
    for (odb::dbBTerm* bterm : net->getBTerms()) {
      const odb::Rect bbox = bterm->getBBox();
      box.merge(odb::Rect(
          bbox.xCenter(), bbox.yCenter(), bbox.xCenter(), bbox.yCenter()));
    }
    if (box.xMin() <= box.xMax()) {
      total += box.dx() + box.dy();
    }
  }
  return total / block->getDbUnitsPerMicron();
}

}  // namespace

int main(int argc, char** argv)
{
  if (argc < 2) {
    std::cerr << "usage: mpl_bench <odb> [--repeats N] [--steps S] "
                 "[--threads T] [--channel W H] [--pin-aware] "
                 "[--target-util U] [--max-num-level L]\n";
    return 2;
  }
  const std::string odb_path = argv[1];
  int repeats = 1;
  int steps = 2000;
  int threads = 10;
  float channel_width = 0;
  float channel_height = 0;
  bool pin_aware = false;
  float target_util = 0.25;
  int max_num_level = 2;
  for (int i = 2; i < argc; i++) {
    const std::string arg = argv[i];
    auto next = [&]() -> const char* {
      if (i + 1 >= argc) {
        std::cerr << arg << " needs a value\n";
        std::exit(2);
      }
      return argv[++i];
    };
    if (arg == "--repeats") {
      repeats = std::atoi(next());
    } else if (arg == "--steps") {
      steps = std::atoi(next());
    } else if (arg == "--threads") {
      threads = std::atoi(next());
    } else if (arg == "--channel") {
      channel_width = std::atof(next());
      channel_height = std::atof(next());
    } else if (arg == "--pin-aware") {
      pin_aware = true;
    } else if (arg == "--target-util") {
      target_util = std::atof(next());
    } else if (arg == "--max-num-level") {
      max_num_level = std::atoi(next());
    } else {
      std::cerr << "unknown argument " << arg << "\n";
      return 2;
    }
  }

  sta::initSta();
  std::cout << "run,seconds,placed,macro_net_hpwl_um\n";
  for (int run = 0; run < repeats; run++) {
    utl::Logger logger;
    Tcl_Interp* interp = Tcl_CreateInterp();
    odb::dbDatabase* db = odb::dbDatabase::create();
    db->setLogger(&logger);
    std::ifstream file(odb_path, std::ios::binary);
    if (!file) {
      std::cerr << "cannot read " << odb_path << "\n";
      return 1;
    }
    db->read(file);
    auto sta = std::make_unique<sta::dbSta>(interp, db, &logger);
    sta->postReadDb(db);
    par::PartitionMgr partitioner(db, sta->getDbNetwork(), sta.get(), &logger);
    mpl::MacroPlacer placer(db, sta.get(), &logger, &partitioner);
    odb::dbBlock* block = db->getChip()->getBlock();
    placer.setMinChannelSize(block->micronsToDbu(channel_width),
                             block->micronsToDbu(channel_height));
    placer.setAnnealingSteps(steps);

    const auto start = std::chrono::steady_clock::now();
    bool placed = false;
    try {
      placed = placer.place(threads,
                            0,
                            0,
                            0,
                            0,
                            0.1,
                            max_num_level,
                            10.0,
                            50,
                            odb::Rect(0, 0, 0, 0),
                            0.1,
                            100.0,
                            100.0,
                            10.0,
                            10.0,
                            50.0,
                            50.0,
                            10.0,
                            target_util,
                            0.33,
                            "mpl_bench_reports",
                            false,
                            pin_aware);
    } catch (const std::exception& error) {
      std::cerr << error.what() << "\n";
    }
    const std::chrono::duration<double> elapsed
        = std::chrono::steady_clock::now() - start;
    std::cout << run << "," << elapsed.count() << "," << placed << ","
              << (placed ? macroNetHpwl(block) : 0.0) << "\n";
    sta.reset();
    odb::dbDatabase::destroy(db);
    Tcl_DeleteInterp(interp);
  }
  return 0;
}
