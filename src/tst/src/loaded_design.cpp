// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "tst/loaded_design.h"

#include <memory>
#include <mutex>
#include <string>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbReadVerilog.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "odb/geom.h"
#include "odb/lefin.h"
#include "sta/MinMax.hh"
#include "sta/Sta.hh"
#include "sta/VerilogReader.hh"
#include "sta/VerilogWriter.hh"
#include "tcl.h"  // IWYU pragma: keep
#include "tst/db_fixture.h"
#include "utl/Logger.h"
#include "utl/deleter.h"

namespace tst {

namespace {

std::once_flag init_sta_flag;

struct TechnologyFiles
{
  const char* liberty;
  const char* lef;
  const char* tech_name;
  const char* lib_name;
};

TechnologyFiles technologyFiles(Technology tech)
{
  switch (tech) {
    case Technology::kNangate45:
      return {"_main/test/Nangate45/Nangate45_typ.lib",
              "_main/test/Nangate45/Nangate45.lef",
              "Nangate45",
              "Nangate45"};
    case Technology::kSky130hd:
      return {"_main/test/sky130hd/sky130_fd_sc_hd_tt.lib",
              "_main/test/sky130hd/sky130_fd_sc_hd.tlef",
              "sky130",
              "sky130_fd_sc_hd"};
  }
  return {};
}

}  // namespace

odb::dbLib* loadTechnology(Technology tech,
                           odb::dbDatabase* db,
                           sta::dbSta* sta,
                           utl::Logger* logger)
{
  const TechnologyFiles files = technologyFiles(tech);

  sta->readLiberty(getRunfilePath(files.liberty).c_str(),
                   sta->findScene("default"),
                   sta::MinMaxAll::all(),
                   /*infer_latches=*/false);

  odb::lefin lef_reader(db, logger, /*ignore_non_routing_layers=*/false);
  odb::dbLib* lib = lef_reader.createTechAndLib(
      files.tech_name, files.lib_name, getRunfilePath(files.lef).c_str());
  sta->postReadLef(/*tech=*/nullptr, lib);
  return lib;
}

odb::dbBlock* readVerilogAndLink(const std::string& verilog_path,
                                 const char* top,
                                 bool hierarchy,
                                 odb::dbDatabase* db,
                                 sta::dbSta* sta,
                                 utl::Logger* logger,
                                 odb::dbLib* lib)
{
  // Hierarchy is a db-wide flag consulted by dbLinkDesign and, later, by
  // write_verilog via dbNetwork::hasHierarchy(). Set it explicitly either way
  // rather than relying on the db's initial state.
  sta::dbNetwork* db_network = sta->getDbNetwork();
  if (hierarchy) {
    db_network->setHierarchy();
  } else {
    db_network->disableHierarchy();
  }

  ord::dbVerilogNetwork verilog_network(sta);
  sta::VerilogReader verilog_reader(&verilog_network);
  verilog_reader.read(verilog_path.c_str());

  ord::dbLinkDesign(top, &verilog_network, db, logger, hierarchy);

  sta->postReadDb(db);

  odb::dbBlock* block = db->getChip()->getBlock();
  block->setDefUnits(lib->getTech()->getLefUnits());
  block->setDieArea(odb::Rect(0, 0, 1000, 1000));
  sta->postReadDef(block);

  return block;
}

LoadedDesign::LoadedDesign(Technology tech,
                           const std::string& verilog_path,
                           const char* top,
                           bool hierarchy)
{
  std::call_once(init_sta_flag, []() { sta::initSta(); });

  db_ = utl::UniquePtrWithDeleter<odb::dbDatabase>(odb::dbDatabase::create(),
                                                   odb::dbDatabase::destroy);
  db_->setLogger(&logger_);

  interp_ = utl::UniquePtrWithDeleter<Tcl_Interp>(Tcl_CreateInterp(),
                                                  Tcl_DeleteInterp);
  sta_ = std::make_unique<sta::dbSta>(interp_.get(), db_.get(), &logger_);

  lib_ = loadTechnology(tech, db_.get(), sta_.get(), &logger_);
  db_network_ = sta_->getDbNetwork();

  block_ = readVerilogAndLink(
      verilog_path, top, hierarchy, db_.get(), sta_.get(), &logger_, lib_);
}

LoadedDesign::~LoadedDesign() = default;

void LoadedDesign::writeVerilog(const std::string& path, bool include_pwr_gnd)
{
  sta::writeVerilog(path.c_str(), include_pwr_gnd, {}, sta_->network());
}

}  // namespace tst
