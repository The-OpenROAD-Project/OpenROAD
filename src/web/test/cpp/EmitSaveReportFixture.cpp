// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
//
// Standalone driver that builds a small Nangate45 block, places a handful of
// instances, and calls WebServer::saveReport to emit a single HTML fixture.
// Consumed by //src/web/test:save_report_fixture_html (genrule) whose output
// drives the jsdom-based static-cache js_test.

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/lefin.h"
#include "tools/cpp/runfiles/runfiles.h"
#include "utl/Logger.h"
#include "utl/deleter.h"
#include "web/web.h"

using bazel::tools::cpp::runfiles::Runfiles;

int main(int argc, char** argv)
{
  if (argc != 2) {
    std::cerr << "usage: " << argv[0] << " <out.html>\n";
    return 1;
  }
  const std::string out_path = argv[1];

  std::string err;
  std::unique_ptr<Runfiles> rf(Runfiles::Create(argv[0], &err));
  if (!rf) {
    std::cerr << "runfiles: " << err << "\n";
    return 1;
  }
  const std::string lef_path
      = rf->Rlocation("_main/test/Nangate45/Nangate45.lef");

  utl::Logger logger;
  utl::UniquePtrWithDeleter<odb::dbDatabase> db(odb::dbDatabase::create(),
                                                odb::dbDatabase::destroy);
  db->setLogger(&logger);

  odb::lefin lef_reader(db.get(), &logger, /*ignore_non_routing_layers=*/false);
  odb::dbLib* lib
      = lef_reader.createTechAndLib("ng45", "ng45", lef_path.c_str());

  odb::dbChip* chip = odb::dbChip::create(db.get(), db->getTech());
  odb::dbBlock* block = odb::dbBlock::create(chip, "top");
  block->setDefUnits(lib->getTech()->getLefUnits());
  block->setDieArea(odb::Rect(0, 0, 20000, 20000));

  odb::dbMaster* master = lib->findMaster("BUF_X16");
  if (!master) {
    std::cerr << "BUF_X16 not found in library\n";
    return 1;
  }
  for (int i = 0; i < 10; ++i) {
    const std::string nm = "buf" + std::to_string(i);
    odb::dbInst* inst = odb::dbInst::create(block, master, nm.c_str());
    inst->setLocation(2000 + i * 1500, 2000 + i * 1500);
    inst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
  }

  web::WebServer server(db.get(), /*sta=*/nullptr, &logger, /*interp=*/nullptr);
  server.saveReport(out_path, 100, 100);

  return 0;
}
